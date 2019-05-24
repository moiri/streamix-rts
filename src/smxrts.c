/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/resource.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "smxrts.h"
#include "pthread.h"

#define XML_PATH        "app.xml"
#define XML_APP         "app"
#define XML_LOG         "log"

#define JSON_LOG_NET "{\"%s\":{\"name\":\"%s\",\"id\":%d}}"
#define JSON_LOG_CH "{\"%s\":{\"name\":\"%s\",\"id\":%d,\"len\":%d}}"
#define JSON_LOG_CONNECT "{\"connect\":{\"ch\":{\"name\":\"%s\",\"id\":%d,\"mode\":\"%s\"},\"net\":{\"name\":\"%s\",\"id\":%d}}}"
#define JSON_LOG_ACCESS "{\"%s\":{\"tgt\":\"%s\",\"count\":%d}}"
#define JSON_LOG_ACTION "{\"%s\":{\"tgt\":\"%s\"}}"
#define JSON_LOG_MSG "{\"%s\":{\"id\":%lu}}"


zlog_category_t* cat_ch;
zlog_category_t* cat_net;
zlog_category_t* cat_main;
zlog_category_t* cat_msg;

/*****************************************************************************/
void smx_cat_add_channel( smx_channel_t* ch, const char* name )
{
    if( ch == NULL || ch->source == NULL ) return;
    ch->cat = zlog_get_category( name );
}

/*****************************************************************************/
void smx_channel_change_collector_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->collector->state != state
            && ch->collector->state != SMX_CHANNEL_END )
    {
        zlog_debug( ch->cat, "collector state change %d -> %d",
                ch->collector->state, state );
        ch->collector->state = state;
        pthread_cond_signal( &ch->collector->col_cv );
    }
}

/*****************************************************************************/
void smx_channel_change_read_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->source->state != state && ch->source->state != SMX_CHANNEL_END )
    {
        zlog_debug( ch->cat, "read state change %d -> %d",
                ch->source->state, state );
        ch->source->state = state;
        pthread_cond_signal( &ch->source->ch_cv );
    }
}

/*****************************************************************************/
void smx_channel_change_write_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->sink->state != state && ch->sink->state != SMX_CHANNEL_END )
    {
        zlog_debug( ch->cat, "write state change %d -> %d",
                ch->sink->state, state );
        ch->sink->state = state;
        pthread_cond_signal( &ch->sink->ch_cv );
    }
}

/*****************************************************************************/
smx_channel_t* smx_channel_create( int len, smx_channel_type_t type,
        int id, const char* name )
{
    smx_channel_t* ch = smx_malloc( sizeof( struct smx_channel_s ) );
    if( ch == NULL )
        return NULL;

    zlog_info( cat_ch, JSON_LOG_CH, "create", name, id, len );
    ch->id = id;
    ch->type = type;
    ch->fifo = smx_fifo_create( len );
    ch->collector = NULL;
    ch->guard = NULL;
    ch->name = name;
    ch->sink = smx_channel_create_end();
    ch->source = smx_channel_create_end();
    ch->source->state = SMX_CHANNEL_PENDING;
    ch->sink->state = SMX_CHANNEL_READY;
    if( ( type == SMX_FIFO_D ) || ( type == SMX_D_FIFO_D ) )
    {
        // do not block on decouped output
        ch->source->state = SMX_CHANNEL_UNINITIALISED;
    }
    if( ch->sink == NULL || ch->source == NULL )
        return NULL;
    return ch;
}

/*****************************************************************************/
smx_channel_end_t* smx_channel_create_end()
{
    smx_channel_end_t* end = smx_malloc( sizeof( struct smx_channel_end_s ) );
    if( end == NULL )
        return NULL;

    pthread_mutex_init( &end->ch_mutex, NULL );
    pthread_cond_init( &end->ch_cv, NULL );
    return end;
}

/*****************************************************************************/
void smx_channel_destroy( smx_channel_t* ch )
{
    if( ch == NULL )
        return;
    zlog_debug( cat_ch, "destroy channel '%s(%d)' (msg count: %d)", ch->name,
            ch->id, ch->fifo->count);
    smx_guard_destroy( ch->guard );
    smx_fifo_destroy( ch->fifo );
    smx_channel_destroy_end( ch->sink );
    smx_channel_destroy_end( ch->source );
    free( ch );
}

/*****************************************************************************/
void smx_channel_destroy_end( smx_channel_end_t* end )
{
    if( end == NULL )
        return;
    pthread_mutex_destroy( &end->ch_mutex );
    pthread_cond_destroy( &end->ch_cv );
    free( end );
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( smx_channel_t* ch )
{
    smx_msg_t* msg = NULL;
    if( ch == NULL || ch->source ==  NULL )
    {
        zlog_fatal( cat_main, "channel not initialised" );
        return NULL;
    }

    pthread_mutex_lock( &ch->source->ch_mutex);
    while( ch->source->state == SMX_CHANNEL_PENDING )
    {
        zlog_debug( ch->cat, "waiting for message" );
        pthread_cond_wait( &ch->source->ch_cv, &ch->source->ch_mutex );
    }
    pthread_mutex_unlock( &ch->source->ch_mutex );
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            msg = smx_fifo_read( ch, ch->fifo );
            break;
        case SMX_FIFO_D:
        case SMX_D_FIFO_D:
            msg = smx_fifo_d_read( ch, ch->fifo );
            break;
        default:
            zlog_error( ch->cat, "undefined channel type '%d'",
                    ch->type );
            return NULL;
    }
    // notify producer that space is available
    pthread_mutex_lock( &ch->sink->ch_mutex );
    smx_channel_change_write_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->sink->ch_mutex );
    return msg;
}

/*****************************************************************************/
int smx_channel_ready_to_read( smx_channel_t* ch )
{
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            return ch->fifo->count;
        case SMX_D_FIFO_D:
        case SMX_FIFO_D:
            return 1;
        default:
            zlog_error( ch->cat, "undefined channel type '%d'",
                    ch->type );
            return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_channel_write( smx_channel_t* ch, smx_msg_t* msg )
{
    bool abort = false;
    if( ch == NULL || ch->sink == NULL )
    {
        zlog_fatal( cat_main, "channel not initialised" );
        return -1;
    }

    pthread_mutex_lock( &ch->sink->ch_mutex );
    while( ch->sink->state == SMX_CHANNEL_PENDING )
    {
        zlog_debug( ch->cat, "waiting for free space" );
        pthread_cond_wait( &ch->sink->ch_cv, &ch->sink->ch_mutex );
    }
    if( ch->sink->state == SMX_CHANNEL_END ) abort = true;
    pthread_mutex_unlock( &ch->sink->ch_mutex );
    if( abort )
    {
        zlog_notice( ch->cat, "write aborted" );
        smx_msg_destroy( msg, true );
        return -1;
    }
    switch( ch->type )
    {
        case SMX_FIFO:
        case SMX_FIFO_D:
            if( ch->guard != NULL )
                smx_guard_write( ch );
            smx_fifo_write( ch, ch->fifo, msg );
            break;
        case SMX_D_FIFO:
        case SMX_D_FIFO_D:
            if( ch->guard != NULL )
                // discard message if miat is not reached
                if( smx_d_guard_write( ch, msg ) ) return 0;
            smx_d_fifo_write( ch, ch->fifo, msg );
            break;
        default:
            zlog_error( ch->cat, "undefined channel type '%d'",
                    ch->type );
            return -1;
    }
    if( ch->collector != NULL )
    {
        pthread_mutex_lock( &ch->collector->col_mutex );
        ch->collector->count++;
        zlog_info( ch->cat, JSON_LOG_ACCESS, "write", "collector",
                ch->collector->count );
        smx_channel_change_collector_state( ch, SMX_CHANNEL_READY );
        pthread_mutex_unlock( &ch->collector->col_mutex );
    }
    // notify consumer that messages are available
    pthread_mutex_lock( &ch->source->ch_mutex );
    smx_channel_change_read_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->source->ch_mutex );
    return 0;
}

/*****************************************************************************/
void smx_connect( smx_channel_t** dest, smx_channel_t* src )
{
    const char* elem;
    if( dest ==  NULL || src == NULL )
    {
        elem = ( dest == NULL ) ? "dest" : "src";
        zlog_fatal( cat_main,
                "unable to connect channels: not initialised, %s", elem );
        return;
    }
    *dest = src;
}

/*****************************************************************************/
void smx_connect_arr( smx_channel_t** dest, int* idx, smx_channel_t* src,
        int dest_id, int src_id, const char* dest_name, const char* src_name,
        const char* mode )
{
    const char* elem;
    if( dest ==  NULL || idx == NULL || src == NULL )
    {
        elem = ( idx == NULL || dest == NULL ) ? "dest" : "src";
        zlog_fatal( cat_main,
                "unable to connect channels: not initialised, %s", elem );
        return;
    }
    zlog_info( cat_ch, JSON_LOG_CONNECT, src_name, src_id, mode, dest_name,
            dest_id );
    dest[*idx] = src;
    (*idx)++;
}

/*****************************************************************************/
void smx_connect_guard( smx_channel_t* ch, smx_guard_t* guard )
{
    const char* elem;
    if( ch == NULL || guard == NULL )
    {
        elem = ( ch == NULL ) ? "channel" : "guard";
        zlog_fatal( cat_main,
                "unable to connect guard: not initialised %s", elem );
        return;
    }
    ch->guard = guard;
}

/*****************************************************************************/
void smx_connect_rn( smx_channel_t* ch, smx_net_t* rn )
{
    const char* elem;
    if( ch == NULL || rn == NULL )
    {
        elem = ( ch == NULL ) ? "channel" : "rn";
        zlog_fatal( cat_main,
                "unable to connect routing node: not initialised %s", elem );
        return;
    }
    ch->collector = ( ( net_smx_rn_t* )SMX_SIG( rn ) )->in.collector;
}

/*****************************************************************************/
smx_msg_t* smx_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx )
{
    bool has_msg = false;
    int cur_count, i, ch_count, ready_cnt;
    smx_msg_t* msg = NULL;
    smx_channel_t* ch = NULL;
    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG( h, debug, "waiting for message on collector" );
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    }
    if( collector->count > 0 )
    {
        collector->count--;
        has_msg = true;
    }
    else
    {
        SMX_LOG( h, debug, "collector state change %d -> %d", collector->state,
                SMX_CHANNEL_PENDING );
        collector->state = SMX_CHANNEL_PENDING;
    }
    cur_count = collector->count;
    pthread_mutex_unlock( &collector->col_mutex );

    if( has_msg )
    {
        ch_count = count_in;
        i = *last_idx;
        while( ch_count > 0)
        {
            i++;
            if( i >= count_in )
                i = 0;
            ch_count--;
            ready_cnt = smx_channel_ready_to_read( in[i] );
            if( ready_cnt > 0 )
            {
                ch = in[i];
                *last_idx = i;
                break;
            }
        }
        if( ch == NULL )
        {
            SMX_LOG( h, error, "something went wrong: no msg ready in collector (count: %d)",
                    collector->count );
            return NULL;
        }
        SMX_LOG( h, info, JSON_LOG_ACCESS, "read", "collector", cur_count );
        msg = smx_channel_read( ch );
    }
    return msg;
}

/*****************************************************************************/
smx_fifo_t* smx_fifo_create( int length )
{
    smx_fifo_item_t* last_item = NULL;
    smx_fifo_t* fifo = smx_malloc( sizeof( struct smx_fifo_s ) );
    if( fifo == NULL )
        return NULL;

    for( int i=0; i < length; i++ ) {
        fifo->head = smx_malloc( sizeof( struct smx_fifo_item_s ) );
        if( fifo->head == NULL )
        {
            smx_fifo_destroy( fifo );
            return NULL;
        }

        fifo->head->msg = NULL;
        fifo->head->prev = last_item;
        if( last_item == NULL )
            fifo->tail = fifo->head;
        else
            last_item->next = fifo->head;
        last_item = fifo->head;
    }
    fifo->head->next = fifo->tail;
    fifo->tail->prev = fifo->head;
    fifo->tail = fifo->head;
    fifo->backup = NULL;
    pthread_mutex_init( &fifo->fifo_mutex, NULL );
    fifo->count = 0;
    fifo->overwrite = 0;
    fifo->copy = 0;
    fifo->length = length;
    return fifo;
}

/*****************************************************************************/
void smx_fifo_destroy( smx_fifo_t* fifo )
{
    if( fifo == NULL )
        return;

    pthread_mutex_destroy( &fifo->fifo_mutex );
    for( int i=0; i < fifo->length; i++ ) {
        if( fifo->head == NULL )
            continue;

        if( fifo->head->msg != NULL )
            smx_msg_destroy( fifo->head->msg, true );
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    if( fifo->backup != NULL )
        smx_msg_destroy( fifo->backup, true );
    free( fifo );
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    int new_count;
    if( ch == NULL || fifo == NULL )
        return NULL;

    if( fifo->count > 0 )
    {
        // messages are available
        pthread_mutex_lock( &fifo->fifo_mutex );
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;
        new_count = fifo->count;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        zlog_info( ch->cat, JSON_LOG_ACCESS, "read", "fifo",
                new_count );
        if( new_count == 0 )
        {
            pthread_mutex_lock( &ch->source->ch_mutex );
            smx_channel_change_read_state( ch, SMX_CHANNEL_PENDING );
            pthread_mutex_unlock( &ch->source->ch_mutex );
        }
    }
    else if( ch->source->state != SMX_CHANNEL_END )
    {
        zlog_error( ch->cat, "channel is ready but is empty" );
        return NULL;
    }

    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_d_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    int new_count;
    smx_msg_t* msg = NULL;
    smx_msg_t* old_backup = NULL;
    if( ch == NULL || fifo == NULL )
        return NULL;

    if( fifo->count > 0 )
    {
        // messages are available
        pthread_mutex_lock( &fifo->fifo_mutex );
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        if( fifo->count == 1 )
        {
            // last message, backup for later duplication
            if( fifo->backup != NULL ) // delete old backup
                old_backup = fifo->backup;
            fifo->backup = smx_msg_copy( msg );
        }
        fifo->count--;
        new_count = fifo->count;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        smx_msg_destroy( old_backup, true );
        zlog_info( ch->cat, JSON_LOG_ACCESS, "read", "fifo_d",
                new_count );
    }
    else
    {
        if( fifo->backup != NULL )
        {
            pthread_mutex_lock( &fifo->fifo_mutex );
            msg = smx_msg_copy( fifo->backup );
            fifo->copy++;
            pthread_mutex_unlock( &fifo->fifo_mutex );
            zlog_info( ch->cat, JSON_LOG_ACTION,
                    "duplicate backup", "fifo_d" );
        }
        else
            zlog_notice( ch->cat,
                    "nothing to read, fifo and its backup is empty" );
    }
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_dd_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    int new_count;
    if( ch == NULL || fifo == NULL )
        return NULL;

    if( fifo->count > 0 )
    {
        // messages are available
        pthread_mutex_lock( &fifo->fifo_mutex );
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;
        new_count = fifo->count;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        zlog_info( ch->cat, JSON_LOG_ACCESS, "read", "fifo_dd",
                new_count );
    }
    return msg;
}

/*****************************************************************************/
int smx_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    int new_count;
    if( ch == NULL || fifo == NULL || msg == NULL )
        return -1;

    if(fifo->count < fifo->length)
    {
        pthread_mutex_lock( &fifo->fifo_mutex );
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        new_count = fifo->count;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        zlog_info( ch->cat, JSON_LOG_ACCESS, "write", "fifo",
                new_count );
        if( new_count == fifo->length )
        {
            pthread_mutex_lock( &ch->sink->ch_mutex );
            smx_channel_change_write_state( ch, SMX_CHANNEL_PENDING );
            pthread_mutex_unlock( &ch->sink->ch_mutex );
        }
    }
    else
    {
        zlog_info( ch->cat, "channel is ready but has no space" );
        return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_d_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    int new_count;
    smx_msg_t* msg_tmp = NULL;
    if( ch == NULL || fifo == NULL || msg == NULL )
        return -1;

    if( fifo->count < fifo->length )
    {
        pthread_mutex_lock( &fifo->fifo_mutex );
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        new_count = fifo->count;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        zlog_info( ch->cat, JSON_LOG_ACCESS, "write", "d_fifo",
                new_count );
    }
    else
    {
        pthread_mutex_lock( &fifo->fifo_mutex );
        msg_tmp = fifo->tail->msg;
        fifo->tail->msg = msg;
        fifo->overwrite++;
        pthread_mutex_unlock( &fifo->fifo_mutex );

        smx_msg_destroy( msg_tmp, true );
        zlog_notice( ch->cat, "overwrite tail of fifo" );
        zlog_info( ch->cat, JSON_LOG_ACTION,
                "overwrite tail", "d_fifo" );
    }
    return 0;
}

/*****************************************************************************/
smx_guard_t* smx_guard_create( int iats, int iatns, smx_channel_t* ch )
{
    struct itimerspec itval;
    zlog_debug( ch->cat, "create guard" );
    smx_guard_t* guard = smx_malloc( sizeof( struct smx_guard_s ) );
    if( guard == NULL ) 
        return NULL;

    guard->iat.tv_sec = iats;
    guard->iat.tv_nsec = iatns;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = 1; // arm timer to immediately fire (1ns delay)
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    guard->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( guard->fd == -1 )
    {
        zlog_error( ch->cat, "failed to create guard timer \
                (timerfd_create returned %d", errno );
        free( guard );
        return NULL;
    }
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
    {
        zlog_error( ch->cat, "failed to arm guard timer \
                (timerfd_settime returned %d)", errno );
        free( guard );
        return NULL;
    }
    return guard;
}

/*****************************************************************************/
void smx_guard_destroy( smx_guard_t* guard )
{
    if( guard == NULL ) return;
    close( guard->fd );
    free( guard );
}

/*****************************************************************************/
int smx_guard_write( smx_channel_t* ch )
{
    uint64_t expired;
    struct itimerspec itval;
    if( ch == NULL || ch->guard == NULL )
        return -1;

    if( -1 == read( ch->guard->fd, &expired, sizeof( uint64_t ) ) )
    {
        zlog_error( ch->cat, "falied to read guard timer \
                (read returned %d", errno );
        return -1;
    }
    if( expired > 0 )
        zlog_error( ch->cat, "guard timer misses: %lu", expired );
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
    {
        zlog_error( ch->cat, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
        return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_d_guard_write( smx_channel_t* ch, smx_msg_t* msg )
{
    struct itimerspec itval;
    if( ch == NULL || ch->guard == NULL )
        return -1;

    if( -1 == timerfd_gettime( ch->guard->fd, &itval ) )
    {
        zlog_error( ch->cat, "falied to read guard timer \
                (timerfd_gettime returned %d", errno );
        return -1;
    }
    if( ( itval.it_value.tv_sec != 0 ) || ( itval.it_value.tv_nsec != 0 ) ) {
        zlog_notice( ch->cat, "rate_control: discard message '%lu'",
                msg->id );
        smx_msg_destroy( msg, true );
        return 1;
    }
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
    {
        zlog_error( ch->cat, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
        return -1;
    }
    return 0;
}

/*****************************************************************************/
void* smx_malloc( size_t size )
{
    void* mem = malloc( size );
    if( mem == NULL )
        zlog_fatal( cat_main, "unable to allocate memory: %s",
                strerror( errno ) );
    return mem;
}

/*****************************************************************************/
smx_msg_t* smx_msg_copy( smx_msg_t* msg )
{
    if( msg == NULL )
        return NULL;

    zlog_info( cat_msg, JSON_LOG_MSG, "copy", msg->id );
    return smx_msg_create( msg->copy( msg->data, msg->size ),
            msg->size, msg->copy, msg->destroy, msg->unpack );
}

/*****************************************************************************/
smx_msg_t* smx_msg_create( void* data, size_t size, void* copy( void*, size_t ),
        void destroy( void* ), void* unpack( void* ) )
{
    static unsigned long msg_count = 0;
    smx_msg_t* msg = smx_malloc( sizeof( struct smx_msg_s ) );
    if( msg == NULL )
        return NULL;

    msg->id = msg_count++;
    zlog_info( cat_msg, JSON_LOG_MSG, "create", msg->id );
    msg->data = data;
    msg->size = size;
    if( copy == NULL ) msg->copy = smx_msg_data_copy;
    else msg->copy = copy;
    if( destroy == NULL ) msg->destroy = smx_msg_data_destroy;
    else msg->destroy = destroy;
    if( unpack == NULL ) msg->unpack = smx_msg_data_unpack;
    else msg->unpack = unpack;
    return msg;
}

/*****************************************************************************/
void* smx_msg_data_copy( void* data, size_t size )
{
    void* data_copy = smx_malloc( size );
    if( data_copy == NULL ) 
        return NULL;

    memcpy( data_copy, data, size );
    return data_copy;
}

/*****************************************************************************/
void smx_msg_data_destroy( void* data )
{
    if( data == NULL )
        return;

    free( data );
}

/*****************************************************************************/
void* smx_msg_data_unpack( void* data )
{
    return data;
}

/*****************************************************************************/
void smx_msg_destroy( smx_msg_t* msg, int deep )
{
    if( msg == NULL )
        return;

    zlog_info( cat_msg, JSON_LOG_MSG, "destroy", msg->id );
    if( deep )
        msg->destroy( msg->data );
    free( msg );
}

/*****************************************************************************/
void* smx_msg_unpack( smx_msg_t* msg )
{
    return msg->unpack( msg->data );
}

/*****************************************************************************/
smx_net_t* smx_net_create( unsigned int id, const char* name,
        const char* cat_name, void* sig, void** conf )
{
    // sig is allocated in the macro, hence, the NULL check is done here
    if( sig == NULL )
        return NULL;

    xmlNodePtr cur = NULL;
    smx_net_t* net = smx_malloc( sizeof( struct smx_net_s ) );
    if( net == NULL )
        return NULL;

    net->id = id;
    net->cat = zlog_get_category( cat_name );
    net->sig = sig;
    net->conf = NULL;

    cur = xmlDocGetRootElement( (xmlDocPtr)(*conf) );
    cur = cur->xmlChildrenNode;
    while(cur != NULL)
    {
        if(!xmlStrcmp(cur->name, (const xmlChar*)name))
        {
            net->conf = cur;
            break;
        }
        cur = cur->next;
    }

    zlog_info( cat_net, JSON_LOG_NET, "create", name, id );
    return net;
}

/*****************************************************************************/
void smx_net_destroy( void* in, void* out, void* sig, void* h )
{
    if( in != NULL )
        free( in );
    if( out != NULL )
        free( out );
    if( sig != NULL )
        free( sig );
    if( h != NULL )
        free( h );
}

/*****************************************************************************/
void smx_net_init( int* in_cnt, smx_channel_t*** in_ports, int in_degree,
        int* out_cnt, smx_channel_t*** out_ports, int out_degree )
{
    if( in_cnt == NULL || in_ports == NULL || out_cnt == NULL || out_ports == NULL )
        return;

    *in_cnt = 0;
    *out_cnt = 0;
    *in_ports = smx_malloc( sizeof( smx_channel_t* ) * in_degree );
    *out_ports = smx_malloc( sizeof( smx_channel_t* ) * out_degree );
}

/*****************************************************************************/
void smx_net_rn_destroy( net_smx_rn_t* cp )
{
    if( cp == NULL )
        return;

    pthread_mutex_destroy( &cp->in.collector->col_mutex );
    pthread_cond_destroy( &cp->in.collector->col_cv );
    free( cp->in.collector );
}

/*****************************************************************************/
void smx_net_rn_init( net_smx_rn_t* cp )
{
    cp->in.collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( cp->in.collector == NULL )
        return;

    pthread_mutex_init( &cp->in.collector->col_mutex, NULL );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_UNINITIALISED;
}

/*****************************************************************************/
pthread_t smx_net_run( void* box_impl( void* arg ), void* h, int prio )
{
    pthread_attr_t sched_attr;
    struct sched_param fifo_param;
    pthread_t thread;
    int min_fifo, max_fifo;
    pthread_attr_init(&sched_attr);
    if( prio > 0 )
    {
        min_fifo = sched_get_priority_min( SCHED_FIFO );
        max_fifo = sched_get_priority_max( SCHED_FIFO );
        fifo_param.sched_priority = min_fifo;
        if( fifo_param.sched_priority > max_fifo )
        {
            SMX_LOG( h, warn, "cannot use therad priority of %d, falling back\
                    to the max priority %d", fifo_param.sched_priority,
                    max_fifo );
            fifo_param.sched_priority = max_fifo;
        }
        pthread_attr_setinheritsched(&sched_attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&sched_attr, SCHED_FIFO);
        pthread_attr_setschedparam(&sched_attr, &fifo_param);
        SMX_LOG( h, debug, "creating RT thread of priority %d",
                fifo_param.sched_priority );
    }
    if( ( errno = pthread_create( &thread, &sched_attr, box_impl, h ) ) != 0 )
        SMX_LOG( h, error, "failed to create a new thread: %s",
                strerror( errno ) );
    return thread;
}

/*****************************************************************************/
int smx_net_update_state( void* h, smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out, int state )
{
    int i;
    int done_cnt_in = 0;
    int done_cnt_out = 0;
    int trigger_cnt = 0;
    if( chs_in == NULL || chs_out == NULL )
    {
        zlog_fatal( cat_main,
                "net channels not initialised" );
        return SMX_NET_END;
    }

    // if state is forced by box implementation return forced state
    if( state != SMX_NET_RETURN ) return state;

    // check if a triggering input is still producing
    for( i=0; i<len_in; i++ )
    {
        if( ( chs_in[i]->type == SMX_FIFO )
                || ( chs_in[i]->type == SMX_D_FIFO ) )
        {
            trigger_cnt++;
            if( ( chs_in[i]->source->state == SMX_CHANNEL_END )
                    && ( chs_in[i]->fifo->count == 0 ) )
                done_cnt_in++;
        }
    }

    // check if consumer is available
    for( i=0; i<len_out; i++ )
    {
        if( chs_out[i]->sink->state == SMX_CHANNEL_END )
            done_cnt_out++;
    }

    // if all the triggering inputs are done, terminate the thread
    if( (trigger_cnt > 0) && (done_cnt_in >= trigger_cnt) )
    {
        SMX_LOG( h, debug, "all triggering producers have terminated" );
        return SMX_NET_END;
    }

    // if all the outputs are done, terminate the thread
    if( (len_out) > 0 && (done_cnt_out >= len_out) )
    {
        SMX_LOG( h, debug, "all consumers have terminated" );
        return SMX_NET_END;
    }

    return SMX_NET_CONTINUE;
}

/*****************************************************************************/
void smx_net_terminate( void* h, smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out )
{
    int i;
    if( chs_in == NULL || chs_out == NULL )
        return;

    SMX_LOG( h, notice, "send termination notice to neighbours" );
    for( i=0; i < len_in; i++ ) {
        zlog_notice( chs_in[i]->cat, "mark as stale" );
        pthread_mutex_lock( &chs_in[i]->sink->ch_mutex );
        smx_channel_change_write_state( chs_in[i], SMX_CHANNEL_END );
        pthread_mutex_unlock( &chs_in[i]->sink->ch_mutex );
    }
    for( i=0; i < len_out; i++ ) {
        zlog_notice( chs_out[i]->cat, "mark as stale" );
        pthread_mutex_lock( &chs_out[i]->source->ch_mutex );
        smx_channel_change_read_state( chs_out[i], SMX_CHANNEL_END );
        pthread_mutex_unlock( &chs_out[i]->source->ch_mutex );
        if( chs_out[i]->collector != NULL ) {
            zlog_notice( chs_out[i]->cat,
                    "mark collector as stale" );
            pthread_mutex_lock( &chs_out[i]->collector->col_mutex );
            smx_channel_change_collector_state( chs_out[i], SMX_CHANNEL_END );
            pthread_mutex_unlock( &chs_out[i]->collector->col_mutex );
        }
    }
}

/*****************************************************************************/
void smx_program_cleanup( void** doc )
{
    xmlFreeDoc( (xmlDocPtr)*doc );
    xmlCleanupParser();
    zlog_notice( cat_main, "end main thread" );
    zlog_fini();
    exit( EXIT_SUCCESS );
}

/*****************************************************************************/
void smx_program_init( void** doc )
{
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;

    /* required for thread safety */
    xmlInitParser();

    /*parse the file and get the DOM */
    *doc = xmlParseFile( XML_PATH );

    if( *doc == NULL )
    {
        printf( "error: could not parse the app config file '%s'\n", XML_PATH );
        exit( 0 );
    }

    cur = xmlDocGetRootElement( (xmlDocPtr)*doc );
    if( cur == NULL || xmlStrcmp(cur->name, ( const xmlChar* )XML_APP ) )
    {
        printf( "error: app config root node name is '%s' instead of '%s'\n",
                cur->name, XML_APP );
        exit( 0 );
    }
    conf = xmlGetProp( cur, ( const xmlChar* )XML_LOG );

    if( conf == NULL )
    {
        printf( "error: no log configuration found in app config\n" );
        exit( 0 );
    }

    int rc = zlog_init((const char*)conf);

    if( rc ) {
        printf( "error: zlog init failed with conf: '%s'\n", conf );
        exit( 0 );
    }

    cat_main = zlog_get_category( "main" );
    cat_msg = zlog_get_category( "msg" );
    cat_ch = zlog_get_category( "ch" );
    cat_net = zlog_get_category( "net" );

    xmlFree(conf);

    zlog_notice( cat_main, "start thread main" );
}

/*****************************************************************************/
int smx_rn( void* h, void* state )
{
    int* last_idx = ( int* )state;
    int i;
    int count_in = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.count;
    int count_out = ( ( net_smx_rn_t* )SMX_SIG( h ) )->out.count;
    smx_channel_t** chs_in = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.ports;
    smx_channel_t** chs_out = ( ( net_smx_rn_t* )SMX_SIG( h ) )->out.ports;
    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    smx_collector_t* collector = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.collector;

    msg = smx_collector_read( h, collector, chs_in, count_in, last_idx );
    if(msg != NULL)
    {
        for( i=0; i<count_out; i++ ) {
            msg_copy = smx_msg_copy( msg );
            smx_channel_write( chs_out[i], msg_copy );
        }
        smx_msg_destroy( msg, true );
    }
    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_rn_init( void* h, void** state )
{
    (void)(h);
    *state = smx_malloc( sizeof( int ) );
    if( *state == NULL )
        return -1;

    *( int* )( *state ) = -1;
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* state )
{
    if( state != NULL )
        free( state );
}

/*****************************************************************************/
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out, int id )
{
    if( timer == NULL )
    {
        zlog_fatal( cat_main, "unable to connect tf: timer not initialised" );
        return;
    }

    zlog_info( cat_ch, JSON_LOG_CONNECT, ch_in->name, ch_in->id, SMX_MODE_in,
            "smx_tf", id );
    zlog_info( cat_ch, JSON_LOG_CONNECT, ch_out->name, ch_out->id, SMX_MODE_out,
            "smx_tf", id );
    net_smx_tf_t* tf = smx_malloc( sizeof( struct net_smx_tf_s ) );
    if( tf == NULL )
        return;
    tf->in = ch_in;
    tf->out = ch_out;
    tf->next = timer->ports;
    timer->ports = tf;
    timer->count++;
}

/*****************************************************************************/
smx_timer_t* smx_tf_create( int sec, int nsec )
{
    smx_timer_t* timer = smx_malloc( sizeof( struct smx_timer_s ) );
    if( timer == NULL )
        return NULL;

    timer->itval.it_value.tv_sec = sec;
    timer->itval.it_value.tv_nsec = nsec;
    timer->itval.it_interval.tv_sec = sec;
    timer->itval.it_interval.tv_nsec = nsec;
    timer->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( timer->fd == -1 )
        zlog_error( cat_main, "timerfd_create: %d", errno );
    timer->count = 0;
    timer->ports = NULL;
    return timer;
}

/*****************************************************************************/
void smx_tf_destroy( smx_net_t* tt )
{
    if( tt == NULL )
        return;

    smx_timer_t* sig = SMX_SIG( tt );
    net_smx_tf_t* tf = sig->ports;
    net_smx_tf_t* tf_tmp;
    while( tf != NULL ) {
        tf_tmp = tf;
        tf = tf->next;
        free( tf_tmp );
    }
    close( sig->fd );
    free( sig );
    free( tt );
}

/*****************************************************************************/
void smx_tf_enable( void* h, smx_timer_t* timer )
{
    if( timer == NULL )
        return;

    if( -1 == timerfd_settime( timer->fd, 0, &timer->itval, NULL ) )
        SMX_LOG( h, error, "timerfd_settime: %d", errno );
}

/*****************************************************************************/
void smx_tf_propagate_msgs( smx_timer_t* tt, smx_channel_t** ch_in,
        smx_channel_t** ch_out, int copy )
{
    int i;
    smx_msg_t* msg;
    if( ch_in == NULL || ch_out == NULL )
        return;

    for( i = 0; i < tt->count; i++ ) {
        if( ch_in[i]->source->state == SMX_CHANNEL_UNINITIALISED )
            continue;
        if( ( ( ch_in[i]->fifo->count == 0 )
                    && ( ch_in[i]->source->state == SMX_CHANNEL_END ) )
            || ( ch_out[i]->sink->state == SMX_CHANNEL_END ) )
        {
            if( ch_in[i]->source->state == SMX_CHANNEL_END )
            {
                // propagate termination signal to consumer
                pthread_mutex_lock( &ch_out[i]->source->ch_mutex );
                smx_channel_change_read_state( ch_out[i], SMX_CHANNEL_END );
                pthread_mutex_unlock( &ch_out[i]->source->ch_mutex );
            }
            if( ch_out[i]->sink->state == SMX_CHANNEL_END )
            {
                // propagate termination signal to producer
                pthread_mutex_lock( &ch_in[i]->sink->ch_mutex );
                smx_channel_change_write_state( ch_in[i], SMX_CHANNEL_END );
                pthread_mutex_unlock( &ch_in[i]->sink->ch_mutex );
            }
            continue;
        }
        if( copy )
            msg = smx_fifo_d_read( ch_in[i], ch_in[i]->fifo );
        else
            msg = smx_fifo_dd_read( ch_in[i], ch_in[i]->fifo );

        // notify producer that space is available
        pthread_mutex_lock( &ch_in[i]->sink->ch_mutex );
        smx_channel_change_write_state( ch_in[i], SMX_CHANNEL_READY );
        pthread_mutex_unlock( &ch_in[i]->sink->ch_mutex );

        if( msg == NULL || ch_in[i]->fifo->copy )
        {
            pthread_mutex_lock( &ch_in[i]->fifo->fifo_mutex );
            ch_out[i]->fifo->copy = 0;
            pthread_mutex_unlock( &ch_in[i]->fifo->fifo_mutex );
            zlog_error( ch_in[i]->cat, "missed deadline to produce" );
        }
        if( msg != NULL )
        {
            smx_channel_write( ch_out[i], msg );
            if( ch_out[i]->fifo->overwrite )
            {
                pthread_mutex_lock( &ch_out[i]->fifo->fifo_mutex );
                ch_out[i]->fifo->overwrite = 0;
                pthread_mutex_unlock( &ch_out[i]->fifo->fifo_mutex );
                zlog_error( ch_out[i]->cat,
                        "missed deadline to consume" );
            }
        }
    }
}

/*****************************************************************************/
void smx_tf_wait( void* h, smx_timer_t* timer )
{
    uint64_t expired;
    struct pollfd pfd;
    int poll_res;
    if( timer == NULL )
        return;

    pfd.fd = timer->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_res = poll( &pfd, 1, 0 ); // non-blocking poll to check for timer event
    if( -1 == poll_res )
        SMX_LOG( h, error, "timerfd poll: %d", errno );
    else if( poll_res > 0 ) {
        SMX_LOG( h, error, "deadline missed" );
    }
    if( -1 == read( timer->fd, &expired, sizeof( uint64_t ) ) )
        SMX_LOG( h, error, "timerfd read: %d", errno );
}

/*****************************************************************************/
void* start_routine_net( int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void* ), void* h, smx_channel_t** chs_in, int* cnt_in,
        smx_channel_t** chs_out, int* cnt_out )
{
    if( h == NULL || chs_in ==  NULL || chs_out == NULL || cnt_in == NULL
            || cnt_out == NULL )
    {
        zlog_fatal( cat_main, "unable to start net: not initialised" );
        return NULL;
    }

    int state = SMX_NET_CONTINUE;
    SMX_LOG( h, notice, "init net" );
    void* net_state = NULL;
    if(init( h, &net_state ) == 0)
    {
        SMX_LOG( h, notice, "start net" );
        while( state == SMX_NET_CONTINUE )
        {
            SMX_LOG( h, info, JSON_LOG_ACTION, "start", "net loop" );
            state = impl( h, net_state );
            state = smx_net_update_state( h, chs_in, *cnt_in, chs_out, *cnt_out,
                    state );
        }
    }
    else
        SMX_LOG( h, error, "initialisation of net failed" );
    smx_net_terminate( h, chs_in, *cnt_in, chs_out, *cnt_out );
    SMX_LOG( h, notice, "cleanup net" );
    cleanup( net_state );
    SMX_LOG( h, notice, "terminate net" );
    return NULL;
}

/**
 * To my future self: The time might come when you think it is a good idea to
 * handle the termination process of tf like every other net or that it is
 * a good idea to make the channel state UNINITIALISED blocking. Those two
 * things **do not work** in the context of a tf because unlike any other net
 * multiple tfs might be combined into one single thread. This is why the
 * channel state END is propagated through the tf such that specific connecting
 * nets can terminate individually without having to wait for all connecting
 * nets to terminate.
 *
 * The blocking state UNINITIALISED must be avoided due to potential deadlocks.
 */
void* start_routine_tf( void* h )
{
    smx_timer_t* tt = SMX_SIG( h );
    net_smx_tf_t* tf = tt->ports;
    smx_channel_t* ch_in[tt->count];
    smx_channel_t* ch_out[tt->count];
    if( h == NULL || tt ==  NULL )
    {
        zlog_fatal( cat_main, "unable to start tf: not initialised" );
        return NULL;
    }

    SMX_LOG( h, notice, "init net" );
    int state = SMX_NET_CONTINUE;
    int tf_cnt = 0;
    while( tf != NULL )
    {
        ch_in[tf_cnt] = tf->in;
        ch_out[tf_cnt] = tf->out;
        tf_cnt++;
        tf = tf->next;
    }

    xmlNodePtr cur = SMX_NET_GET_CONF( h );
    xmlChar* copy_str = NULL;
    int copy = 0;

    if( cur != NULL )
    {
        copy_str = xmlGetProp(cur, (const xmlChar*)"copy");
        if( copy_str == NULL )
            SMX_LOG( h, error, "invalid tf configuartion, no property 'copy'" );
        else
        {
            copy = atoi( ( const char* )copy_str );
            xmlFree( copy_str );
        }
    }

    SMX_LOG( h, notice, "start net" );
    smx_tf_enable( h, tt );
    while( state == SMX_NET_CONTINUE )
    {
        SMX_LOG( h, info, JSON_LOG_ACTION, "start", "net loop" );
        smx_tf_propagate_msgs( tt, ch_in, ch_out, copy );
        SMX_LOG( h, debug, "wait for end of loop" );
        smx_tf_wait( h, tt );
        state = smx_net_update_state( h, ch_in, tt->count, ch_out, tt->count,
                SMX_NET_RETURN );
    }
    smx_net_terminate( h, ch_in, tt->count, ch_out, tt->count );
    SMX_LOG( h, notice, "terminate net" );
    return NULL;
}
