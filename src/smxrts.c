/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <time.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "smxrts.h"
#include "pthread.h"

smx_cat_th_t* cats_th = NULL;
smx_cat_ch_t* cats_ch_in = NULL;
smx_cat_ch_t* cats_ch_out = NULL;
zlog_category_t* cat_ch;
zlog_category_t* cat_main;
zlog_category_t* cat_msg;
zlog_category_t* cat_net_tf;

/*****************************************************************************/
void smx_cat_add_channel_in( uintptr_t ch, const char* name )
{
    smx_cat_ch_t* cat = smx_cat_ch_create( ch, name );
    HASH_ADD_INT( cats_ch_in, id, cat );
}

/*****************************************************************************/
void smx_cat_add_channel_out( uintptr_t ch, const char* name )
{
    smx_cat_ch_t* cat = smx_cat_ch_create( ch, name );
    HASH_ADD_INT( cats_ch_out, id, cat );
}

/*****************************************************************************/
smx_cat_ch_t* smx_cat_ch_create( uintptr_t ch, const char* name )
{
    smx_cat_ch_t* cat = malloc(sizeof(struct smx_cat_ch_s));
    if( cat == NULL ) smx_out_of_memory();
    cat->ptr = zlog_get_category( name );
    cat->id = ch;
    zlog_debug( cat_ch, "add zlog category for channel end '%s'", name );
    return cat;
}

/*****************************************************************************/
void smx_channel_change_collector_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->collector->state != state
            && ch->collector->state != SMX_CHANNEL_END )
    {
        SMX_LOG_CH( in, debug, (uintptr_t)ch, "collector state change %d -> %d",
                ch->collector->state, state );
        ch->collector->state = state;
        pthread_cond_signal( &ch->collector->col_cv );
    }
}

/*****************************************************************************/
void smx_channel_change_read_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->state_r != state && ch->state_r != SMX_CHANNEL_END )
    {
        SMX_LOG_CH( out, debug, (uintptr_t)ch, "read state change %d -> %d",
                ch->state_r, state );
        ch->state_r = state;
        pthread_cond_signal( &ch->ch_cv_r );
    }
}

/*****************************************************************************/
void smx_channel_change_write_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->state_w != state && ch->state_w != SMX_CHANNEL_END )
    {
        SMX_LOG_CH( in, debug, (uintptr_t)ch, "write state change %d -> %d",
                ch->state_w, state );
        ch->state_w = state;
        pthread_cond_signal( &ch->ch_cv_w );
    }
}

/*****************************************************************************/
smx_channel_t* smx_channel_create( int len, smx_channel_type_t type,
        int id, const char* name )
{
    smx_channel_t* ch = malloc( sizeof( struct smx_channel_s ) );
    if( ch == NULL ) smx_out_of_memory();
    zlog_debug( cat_ch, "create channel '%s(%d)' of length %d", name, id, len );
    ch->type = type;
    ch->fifo = smx_fifo_create( len );
    ch->collector = NULL;
    ch->guard = NULL;
    ch->name = name;
    pthread_mutex_init( &ch->ch_mutex_r, NULL );
    pthread_cond_init( &ch->ch_cv_r, NULL );
    pthread_mutex_init( &ch->ch_mutex_w, NULL );
    pthread_cond_init( &ch->ch_cv_w, NULL );
    ch->state_w = SMX_CHANNEL_READY;
    ch->state_r = SMX_CHANNEL_PENDING;
    if( ( type == SMX_FIFO_D ) || ( type == SMX_D_FIFO_D ) )
        ch->state_r = SMX_CHANNEL_READY; // do not block on decouped output
    return ch;
}

/*****************************************************************************/
void smx_channel_destroy( smx_channel_t* ch )
{
    zlog_debug( cat_ch, "destroy channel '%s' (msg count: %d)", ch->name,
            ch->fifo->count);
    pthread_mutex_destroy( &ch->ch_mutex_r );
    pthread_cond_destroy( &ch->ch_cv_r );
    pthread_mutex_destroy( &ch->ch_mutex_w );
    pthread_cond_destroy( &ch->ch_cv_w );
    smx_guard_destroy( ch->guard );
    smx_fifo_destroy( ch->fifo );
    free( ch );
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( smx_channel_t* ch )
{
    bool abort = false;
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &ch->ch_mutex_r );
    while( ch->state_r == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_CH( out, debug, (uintptr_t)ch, "waiting for message" );
        pthread_cond_wait( &ch->ch_cv_r, &ch->ch_mutex_r );
    }
    if( ch->state_r == SMX_CHANNEL_END ) abort = true;
    pthread_mutex_unlock( &ch->ch_mutex_r );
    if( abort )
    {
        SMX_LOG_CH( out, notice, (uintptr_t)ch, "read aborted" );
        return msg;
    }
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
            SMX_LOG_CH( out, error, (uintptr_t)ch,
                    "undefined channel type '%d'", ch->type );
    }
    // notify producer that space is available
    pthread_mutex_lock( &ch->ch_mutex_w );
    smx_channel_change_write_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->ch_mutex_w );
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
            SMX_LOG_CH( out, error, (uintptr_t)ch,
                    "undefined channel type '%d'", ch->type );
    }
    return 0;
}

/*****************************************************************************/
int smx_channel_write( smx_channel_t* ch, smx_msg_t* msg )
{
    int res = 0;
    bool abort = false;
    pthread_mutex_lock( &ch->ch_mutex_w );
    while( ch->state_w == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_CH( in, debug, (uintptr_t)ch, "waiting for free space" );
        pthread_cond_wait( &ch->ch_cv_w, &ch->ch_mutex_w );
    }
    if( ch->state_w == SMX_CHANNEL_END ) abort = true;
    pthread_mutex_unlock( &ch->ch_mutex_w );
    if( abort )
    {
        SMX_LOG_CH( in, notice, (uintptr_t)ch, "write aborted" );
        smx_msg_destroy( msg, true );
        return res;
    }
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_FIFO_D:
            smx_guard_write( ch );
            smx_fifo_write( ch, ch->fifo, msg );
            break;
        case SMX_D_FIFO:
        case SMX_D_FIFO_D:
            // discard message if miat is not reached
            if( smx_d_guard_write( ch, msg ) < 0 ) return res;
            res = smx_d_fifo_write( ch, ch->fifo, msg );
            break;
        default:
            SMX_LOG_CH( in, error, (uintptr_t)ch,
                    "undefined channel type '%d'", ch->type );
    }
    if( ch->collector != NULL ) {
        pthread_mutex_lock( &ch->collector->col_mutex );
        ch->collector->count++;
        SMX_LOG_CH( in, info, (uintptr_t)ch,
                "write to collector (new count: %d)", ch->collector->count );
        smx_channel_change_collector_state( ch, SMX_CHANNEL_READY );
        pthread_mutex_unlock( &ch->collector->col_mutex );
    }
    // notify consumer that messages are available
    pthread_mutex_lock( &ch->ch_mutex_r );
    smx_channel_change_read_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->ch_mutex_r );
    return res;
}

/*****************************************************************************/
void smx_connect( smx_channel_t** dest, smx_channel_t* src, int dest_id,
        int src_id, const char* dest_name, const char* src_name,
        const char* mode )
{
    const char* in = "->";
    const char* out = "<-";
    const char* dir = 0 == strcmp( mode, "in" ) ? in : out;
    zlog_debug( cat_ch, "connect '%s(%d)%s%s(%d)'", src_name,
            src_id, dir, dest_name, dest_id );
    *dest = src;
}

/*****************************************************************************/
void smx_connect_arr( smx_channel_t** dest, int idx, smx_channel_t* src,
        int dest_id, int src_id, const char* dest_name, const char* src_name,
        const char* mode )
{
    const char* in = "->";
    const char* out = "<-";
    const char* dir = 0 == strcmp( mode, "in" ) ? in : out;
    zlog_debug( cat_ch, "connect '%s(%d)%s%s(%d)' on index %d",
            src_name, src_id, dir, dest_name, dest_id, idx );
    dest[idx] = src;
}

/*****************************************************************************/
smx_fifo_t* smx_fifo_create( int length )
{
    smx_fifo_item_t* last_item = NULL;
    smx_fifo_t* fifo = malloc( sizeof( struct smx_fifo_s ) );
    if( fifo == NULL ) smx_out_of_memory();
    for( int i=0; i < length; i++ ) {
        fifo->head = malloc( sizeof( struct smx_fifo_item_s ) );
        if( fifo->head == NULL ) smx_out_of_memory();
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
    fifo->length = length;
    return fifo;
}

/*****************************************************************************/
void smx_fifo_destroy( smx_fifo_t* fifo )
{
    pthread_mutex_destroy( &fifo->fifo_mutex );
    for( int i=0; i < fifo->length; i++ ) {
        if( fifo->head->msg != NULL ) smx_msg_destroy( fifo->head->msg, true );
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    if( fifo->backup != NULL ) smx_msg_destroy( fifo->backup, true );
    free( fifo );
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    bool empty = false;
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if( fifo->count > 0 ) {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;

        SMX_LOG_CH( out, info, (uintptr_t)ch, "read from fifo (new count: %d)",
                fifo->count );
        if( fifo->count == 0 )
            empty = true;
    }
    else
        SMX_LOG_CH( out, error, (uintptr_t)ch, "channel is ready but is empty" );
    pthread_mutex_unlock( &fifo->fifo_mutex );

    if( empty )
    {
        pthread_mutex_lock( &ch->ch_mutex_r );
        smx_channel_change_read_state( ch, SMX_CHANNEL_PENDING );
        pthread_mutex_unlock( &ch->ch_mutex_r );
    }
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_d_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if( fifo->count > 0 ) {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        if( fifo->count == 1 ) {
            // last message, backup for later duplication
            if( fifo->backup != NULL ) // delete old backup
                smx_msg_destroy( fifo->backup, 1);
            fifo->backup = smx_msg_copy( msg );
        }
        fifo->count--;
        SMX_LOG_CH( out, info, (uintptr_t)ch, "read from fifo (new count: %d)",
                fifo->count );
    }
    else {
        if( fifo->backup != NULL ) {
            msg = smx_msg_copy( fifo->backup );
            SMX_LOG_CH( out, info, (uintptr_t)ch,
                    "fifo is empty, duplicate backup" );
        }
        else {
            SMX_LOG_CH( out, notice, (uintptr_t)ch,
                    "nothing to read, fifo and its backup is empty" );
        }
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
    return msg;
}

/*****************************************************************************/
void smx_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    bool full = false;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if(fifo->count < fifo->length)
    {
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;

        SMX_LOG_CH( in, info, (uintptr_t)ch, "write to fifo (new count: %d)",
                fifo->count );
        if( fifo->count == fifo->length )
            full = true;
    }
    else
        SMX_LOG_CH( in, error, (uintptr_t)ch,
                "channel is ready but has no space" );
    pthread_mutex_unlock( &fifo->fifo_mutex );

    if(full)
    {
       pthread_mutex_lock( &ch->ch_mutex_w );
       smx_channel_change_write_state( ch, SMX_CHANNEL_PENDING );
       pthread_mutex_unlock( &ch->ch_mutex_w );
    }
}

/*****************************************************************************/
int smx_d_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    bool overwrite = false;
    smx_msg_t* msg_tmp = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    msg_tmp = fifo->tail->msg;
    fifo->tail->msg = msg;
    if( fifo->count < fifo->length ) {
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        SMX_LOG_CH( in, info, (uintptr_t)ch, "write to fifo (new count: %d)",
                fifo->count );
    }
    else {
        overwrite = true;
        SMX_LOG_CH( in, notice, (uintptr_t)ch,
                "overwrite tail of fifo (new count: %d)", fifo->count );
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
    if( overwrite ) smx_msg_destroy( msg_tmp, true );
    return overwrite;
}

/*****************************************************************************/
smx_guard_t* smx_guard_create( int iats, int iatns, smx_channel_t* ch )
{
    struct itimerspec itval;
    SMX_LOG_CH( in, debug, (uintptr_t)ch, "create guard" );
    smx_guard_t* guard = malloc( sizeof( struct smx_guard_s ) );
    if( guard == NULL ) smx_out_of_memory();
    guard->iat.tv_sec = iats;
    guard->iat.tv_nsec = iatns;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = 1; // arm timer to immediately fire (1ns delay)
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    guard->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( guard->fd == -1 )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "failed to create guard timer \
                (timerfd_create returned %d", errno );
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "failed to arm guard timer \
                (timerfd_settime returned %d)", errno );
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
void smx_guard_write( smx_channel_t* ch )
{
    uint64_t expired;
    struct itimerspec itval;
    if( ch->guard == NULL ) return;
    if( -1 == read( ch->guard->fd, &expired, sizeof( uint64_t ) ) )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "falied to read guard timer \
                (read returned %d", errno );
    SMX_LOG_CH( in, debug, (uintptr_t)ch, "guard timer misses: %lu", expired );
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
}

/*****************************************************************************/
int smx_d_guard_write( smx_channel_t* ch, smx_msg_t* msg )
{
    struct itimerspec itval;
    if( ch->guard == NULL ) return 0;
    if( -1 == timerfd_gettime( ch->guard->fd, &itval ) )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "falied to read guard timer \
                (timerfd_gettime returned %d", errno );
    if( ( itval.it_value.tv_sec != 0 ) || ( itval.it_value.tv_nsec != 0 ) ) {
        SMX_LOG_CH( in, notice, (uintptr_t)ch,
                "rate_control: discard message '%p'", msg );
        smx_msg_destroy( msg, true );
        return -1;
    }
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
        SMX_LOG_CH( in, error, (uintptr_t)ch, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
    return 0;
}

/*****************************************************************************/
smx_msg_t* smx_msg_copy( smx_msg_t* msg )
{
    zlog_info( cat_msg, "copy message '%p'", msg );
    smx_msg_t* msg_copy = malloc( sizeof( struct smx_msg_s ) );
    if( msg_copy == NULL ) smx_out_of_memory();
    zlog_info( cat_msg, "create message '%p'", msg_copy );
    msg_copy->data = msg->copy( msg->data, msg->size );
    msg_copy->size = msg->size;
    msg_copy->copy = msg->copy;
    msg_copy->destroy = msg->destroy;
    return msg_copy;
}

/*****************************************************************************/
smx_msg_t* smx_msg_create( void* data, size_t size, void* copy( void*, size_t ),
        void destroy( void* ), void* unpack( void* ) )
{
    smx_msg_t* msg = malloc( sizeof( struct smx_msg_s ) );
    if( msg == NULL ) smx_out_of_memory();
    zlog_info( cat_msg, "create message '%p'", msg );
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
    void* data_copy = malloc( size );
    if( data_copy == NULL ) smx_out_of_memory();
    memcpy( data_copy, data, size );
    return data_copy;
}

/*****************************************************************************/
void smx_msg_data_destroy( void* data )
{
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
    if( msg == NULL ) return;
    zlog_info( cat_msg, "destroy message '%p'", msg );
    if( deep ) msg->destroy( msg->data );
    free( msg );
}

/*****************************************************************************/
void* smx_msg_unpack( smx_msg_t* msg )
{
    return msg->unpack( msg->data );
}

/*****************************************************************************/
smx_net_t* smx_net_create( unsigned int id, const char* name,
        const char* cat_name, void* sig )
{
    smx_net_t* net = malloc( sizeof( struct smx_net_s ) );
    if( net == NULL ) smx_out_of_memory();
    net->id = id;
    net->cat = zlog_get_category( cat_name );
    net->sig = sig;
    net->timer = NULL;
    zlog_debug( net->cat, "create net '%s(%d)'", name, id );
    return net;
}

/*****************************************************************************/
void smx_net_rn_destroy( net_smx_rn_t* cp )
{
    pthread_mutex_destroy( &cp->in.collector->col_mutex );
    pthread_cond_destroy( &cp->in.collector->col_cv );
    free( cp->in.collector );
}

/*****************************************************************************/
void smx_net_rn_init( net_smx_rn_t* cp )
{
    cp->in.collector = malloc( sizeof( struct smx_collector_s ) );
    if( cp->in.collector == NULL ) smx_out_of_memory();
    pthread_mutex_init( &cp->in.collector->col_mutex, NULL );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_PENDING;
}

/*****************************************************************************/
pthread_t smx_net_run( const char* name, void* box_impl( void* arg ), void* arg )
{
    pthread_t thread;
    smx_cat_th_t* cat = malloc(sizeof(struct smx_cat_th_s));
    if( cat == NULL ) smx_out_of_memory();
    cat->ptr = zlog_get_category( name );
    pthread_create( &thread, NULL, box_impl, arg );
    cat->id = thread;
    HASH_ADD_INT( cats_th, id, cat );
    zlog_debug( cat_main, "add zlog category for net %s", name );
    return thread;
}

/*****************************************************************************/
zlog_category_t* smx_get_category_th()
{
    pthread_t thread_id = pthread_self();
    smx_cat_th_t* cat;
    HASH_FIND_INT( cats_th, &thread_id, cat );
    if( cat == NULL )
    {
        zlog_warn( cat_main,
                "no zlog category found for current thread, using main cat" );
        return cat_main;
    }
    return cat->ptr;
}

/*****************************************************************************/
zlog_category_t* smx_get_category_ch_in( uintptr_t id )
{
    smx_cat_ch_t* cat;
    HASH_FIND_INT( cats_ch_in, &id, cat );
    return smx_get_category_ch_check( cat );
}

/*****************************************************************************/
zlog_category_t* smx_get_category_ch_out( uintptr_t id )
{
    smx_cat_ch_t* cat;
    HASH_FIND_INT( cats_ch_out, &id, cat );
    return smx_get_category_ch_check( cat );
}

/*****************************************************************************/
zlog_category_t* smx_get_category_ch_check( smx_cat_ch_t* cat )
{
    if( cat == NULL )
    {
        zlog_warn( cat_main, "no zlog category found for current channel, \
                using main channel cat" );
        return cat_ch;
    }
    return cat->ptr;
}

/*****************************************************************************/
int smx_net_update_state( void* h, smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out, int state )
{
    int i;
    int done_cnt_in = 0;
    int done_cnt_out = 0;
    int trigger_cnt = 0;
    int push_cnt = 0;
    // if state is forced by box implementation return forced state
    if( state != SMX_NET_RETURN ) return state;

    // check if a triggering input is still producing
    for( i=0; i<len_in; i++ ) {
        if( ( chs_in[i]->type == SMX_FIFO ) || ( chs_in[i]->type == SMX_D_FIFO ) ) {
            trigger_cnt++;
            if( ( chs_in[i]->state_r == SMX_CHANNEL_END )
                    && ( chs_in[i]->fifo->count == 0 ) )
                done_cnt_in++;
        }
    }
    // check if consumer is available
    for( i=0; i<len_out; i++ ) {
        push_cnt++;
        if( chs_out[i]->state_w == SMX_CHANNEL_END )
            done_cnt_out++;
    }
    // if all the triggering inputs are done, terminate the thread
    if( (len_in > 0) && (done_cnt_in >= trigger_cnt) )
    {
        SMX_LOG( h, debug, "all triggering producers have terminated" );
        return SMX_NET_END;
    }

    if( (len_out) > 0 && (done_cnt_out >= push_cnt) )
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
    SMX_LOG( h, notice, "send termination notice to neighbours" );
    for( i=0; i < len_in; i++ ) {
        SMX_LOG_CH( out, notice, (uintptr_t)chs_in[i], "mark as stale" );
        pthread_mutex_lock( &chs_in[i]->ch_mutex_w );
        smx_channel_change_write_state( chs_in[i], SMX_CHANNEL_END );
        pthread_mutex_unlock( &chs_in[i]->ch_mutex_w );
    }
    for( i=0; i < len_out; i++ ) {
        SMX_LOG_CH( in, notice, (uintptr_t)chs_out[i], "mark as stale" );
        pthread_mutex_lock( &chs_out[i]->ch_mutex_r );
        smx_channel_change_read_state( chs_out[i], SMX_CHANNEL_END );
        pthread_mutex_unlock( &chs_out[i]->ch_mutex_r );
        if( chs_out[i]->collector != NULL ) {
            SMX_LOG_CH( in, notice, (uintptr_t)chs_out[i],
                    "mark collector as stale" );
            pthread_mutex_lock( &chs_out[i]->collector->col_mutex );
            smx_channel_change_collector_state( chs_out[i], SMX_CHANNEL_END );
            pthread_mutex_unlock( &chs_out[i]->collector->col_mutex );
        }
    }
}

/*****************************************************************************/
void smx_out_of_memory()
{
    zlog_fatal( cat_main, "out of memory" );
}

/*****************************************************************************/
void smx_program_cleanup()
{
    smx_cat_th_t *cur_th, *tmp_th;
    smx_cat_ch_t *cur_ch, *tmp_ch;
    xmlCleanupParser();
    zlog_notice( cat_main, "end main thread" );
    zlog_fini();
    HASH_ITER( hh, cats_th, cur_th, tmp_th ) {
        HASH_DEL( cats_th, cur_th );
        free( cur_th );
    }
    HASH_ITER( hh, cats_ch_in, cur_ch, tmp_ch ) {
        HASH_DEL( cats_ch_in, cur_ch );
        free( cur_ch );
    }
    HASH_ITER( hh, cats_ch_out, cur_ch, tmp_ch ) {
        HASH_DEL( cats_ch_out, cur_ch );
        free( cur_ch );
    }
    exit( 0 );
}

/*****************************************************************************/
void smx_program_init()
{
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;

    /* required for thread safety */
    xmlInitParser();

    /*parse the file and get the DOM */
    doc = xmlParseFile(XML_PATH);

    if (doc == NULL)
    {
        printf("error: could not parse the app config file '%s'\n", XML_PATH);
        exit( 0 );
    }

    cur = xmlDocGetRootElement(doc);
    if(cur == NULL || xmlStrcmp(cur->name, (const xmlChar*)XML_APP))
    {
        printf("error: app config root node name is '%s' instead of '%s'\n",
                cur->name, XML_APP);
        exit( 0 );
    }

    cur = cur->xmlChildrenNode;
    while(cur != NULL)
    {
        if(!xmlStrcmp(cur->name, (const xmlChar*)XML_LOG))
            conf = xmlGetProp(cur, (const xmlChar*)XML_LOG_CONF);
        cur = cur->next;
    }

    if(conf == NULL)
    {
        printf("error: no log configuration found in app config\n");
        exit( 0 );
    }

    int rc = zlog_init((const char*)conf);

    if( rc ) {
        printf("error: zlog init failed with conf: '%s'\n", conf);
        exit( 0 );
    }

    cat_main = zlog_get_category( "main" );
    cat_msg = zlog_get_category( "msg" );
    cat_ch = zlog_get_category( "ch" );
    cat_net_tf = zlog_get_category( "net_smx_tf" );

    xmlFree(conf);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    zlog_notice( cat_main, "start thread main" );
}

/*****************************************************************************/
int smx_rn( void* h, void* state )
{
    (void)(state);
    bool has_msg = false;
    bool abort = false;
    int cur_count, i;
    int count_in = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.count;
    int count_out = ( ( net_smx_rn_t* )SMX_SIG( h ) )->out.count;
    smx_channel_t** chs_in = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.ports;
    smx_channel_t** chs_out = ( ( net_smx_rn_t* )SMX_SIG( h ) )->out.ports;
    smx_channel_t* ch = NULL;
    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    smx_collector_t* collector = ( ( net_smx_rn_t* )SMX_SIG( h ) )->in.collector;

    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG( h, debug, "waiting for message on collector" );
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    }
    if( collector->state == SMX_CHANNEL_END )
    {
        abort = true;
    }
    else if( collector->count > 0 )
    {
        collector->count--;
        has_msg = true;
    }
    else if( collector->state != SMX_CHANNEL_END )
    {
        SMX_LOG( h, debug, "collector state change %d -> %d", collector->state,
                SMX_CHANNEL_PENDING );
        collector->state = SMX_CHANNEL_PENDING;
    }
    cur_count = collector->count;
    pthread_mutex_unlock( &collector->col_mutex );

    if( abort )
        return SMX_NET_END;
    else if( has_msg )
    {
        for( i=0; i<count_in; i++ ) {
            if( smx_channel_ready_to_read( chs_in[i] ) ) {
                ch = chs_in[i];
                break;
            }
        }
        if( ch == NULL )
            SMX_LOG( h, error, "something went wrong: no msg ready in collector %s (count: %d)",
                    ch->name, collector->count );
        SMX_LOG( h, info, "read from collector %s (new count: %d)", ch->name,
                cur_count );
        msg = smx_channel_read( ch );
        if(msg != NULL)
        {
            for( i=0; i<count_out; i++ ) {
                msg_copy = smx_msg_copy( msg );
                smx_channel_write( chs_out[i], msg_copy );
            }
            smx_msg_destroy( msg, true );
        }
    }
    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_rn_init( void* h, void** state )
{
    (void)(h);
    (void)(state);
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* state )
{
    (void)(state);
}

/*****************************************************************************/
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out )
{
    net_smx_tf_t* tf = malloc( sizeof( struct net_smx_tf_s ) );
    if( tf == NULL ) smx_out_of_memory();
    tf->in = ch_in;
    tf->out = ch_out;
    tf->next = timer->ports;
    timer->ports = tf;
    timer->count++;
}

/*****************************************************************************/
smx_timer_t* smx_tf_create( int sec, int nsec )
{
    smx_timer_t* timer = malloc( sizeof( struct smx_timer_s ) );
    if( timer == NULL ) smx_out_of_memory();
    timer->itval.it_value.tv_sec = sec;
    timer->itval.it_value.tv_nsec = nsec;
    timer->itval.it_interval.tv_sec = sec;
    timer->itval.it_interval.tv_nsec = nsec;
    timer->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( timer->fd == -1 )
        zlog_error( cat_net_tf, "timerfd_create: %d", errno );
    timer->count = 0;
    timer->ports = NULL;
    return timer;
}

/*****************************************************************************/
void smx_tf_destroy( smx_timer_t* tt )
{
    net_smx_tf_t* tf = tt->ports;
    net_smx_tf_t* tf_tmp;
    while( tf != NULL ) {
        tf_tmp = tf;
        tf = tf->next;
        free( tf_tmp );
    }
    close( tt->fd );
    free( tt );
}

/*****************************************************************************/
void smx_tf_enable( smx_timer_t* timer )
{
    if( timer == NULL ) return;
    if( -1 == timerfd_settime( timer->fd, 0, &timer->itval, NULL ) )
        zlog_error( cat_net_tf, "timerfd_settime: %d", errno );
}

/*****************************************************************************/
int smx_tf_read_inputs( smx_msg_t** msg, smx_timer_t* tt,
        smx_channel_t** ch_in )
{
    int i, end_cnt = 0;
    zlog_debug( cat_net_tf, "tt_read" );
    for( i = 0; i < tt->count; i++ ) {
        msg[i] = smx_channel_read( ch_in[i] );
        if( ch_in[i]->state_r == SMX_CHANNEL_END ) end_cnt++;
    }
    return end_cnt;
}

/*****************************************************************************/
void smx_tf_wait( smx_timer_t* timer )
{
    uint64_t expired;
    struct pollfd pfd;
    int poll_res;
    if( timer == NULL ) return;
    pfd.fd = timer->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_res = poll( &pfd, 1, 0 ); // non-blocking poll to check for timer event
    if( -1 == poll_res )
        zlog_error( cat_net_tf, "timerfd poll: %d", errno );
    if( poll_res > 0 ) {
        zlog_error( cat_net_tf, "deadline missed" );
    }
    else if( -1 == read( timer->fd, &expired, sizeof( uint64_t ) ) )
        zlog_error( cat_net_tf, "timerfd read: %d", errno );
}

/*****************************************************************************/
void smx_tf_write_outputs( smx_msg_t** msg, smx_timer_t* tt,
        smx_channel_t** ch_in, smx_channel_t** ch_out )
{
    int i, res;
    zlog_debug( cat_net_tf, "tt_write" );
    for( i = 0; i < tt->count; i++ )
        if( msg[i] != NULL ) {
            res = smx_channel_write( ch_out[i], msg[i] );
            if( res )
                zlog_error( cat_net_tf, "consumer on channel '%s' missed its deadline",
                        ch_in[i]->name );
        }
        else
            zlog_error( cat_net_tf, "producer on channel '%s' missed its deadline",
                    ch_in[i]->name );

}

/*****************************************************************************/
void* start_routine_net( int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void* ), void* h, smx_channel_t** chs_in, int cnt_in,
        smx_channel_t** chs_out, int cnt_out )
{
    int state = SMX_NET_CONTINUE;
    SMX_LOG( h, notice, "init net" );
    void* net_state = NULL;
    if(init( h, &net_state ) == 0)
    {
        SMX_LOG( h, notice, "start net" );
        while( state == SMX_NET_CONTINUE )
        {
            SMX_LOG( h, info, "start net loop" );
            state = impl( h, net_state );
            state = smx_net_update_state( h, chs_in, cnt_in, chs_out, cnt_out,
                    state );
        }
    }
    else
        SMX_LOG( h, error, "initialisation of net failed" );
    smx_net_terminate( h, chs_in, cnt_in, chs_out, cnt_out );
    SMX_LOG( h, notice, "cleanup net" );
    cleanup( net_state );
    SMX_LOG( h, notice, "terminate net" );
    return NULL;
}

/*****************************************************************************/
void* start_routine_tf( void* h )
{
    smx_timer_t* tt = h;
    net_smx_tf_t* tf = tt->ports;
    smx_channel_t* ch_in[tt->count];
    smx_channel_t* ch_out[tt->count];
    smx_msg_t* msg[tt->count];
    zlog_notice( cat_net_tf, "init net" );
    int state = SMX_NET_CONTINUE;
    int tf_cnt = 0, end_cnt;
    while( tf != NULL ) {
        ch_in[tf_cnt] = tf->in;
        ch_out[tf_cnt] = tf->out;
        tf_cnt++;
        tf = tf->next;
    }
    zlog_notice( cat_net_tf, "start net" );
    smx_tf_enable( tt );
    while( state == SMX_NET_CONTINUE )
    {
        zlog_info( cat_net_tf, "start net loop" );
        end_cnt = smx_tf_read_inputs( msg, tt, ch_in );
        smx_tf_write_outputs( msg, tt, ch_in, ch_out );
        zlog_debug( cat_net_tf, "tt wait" );
        smx_tf_wait( tt );
        if( end_cnt == tt->count ) state = SMX_NET_END;
    }
    smx_net_terminate( h, ch_in, tt->count, ch_out, tt->count );
    zlog_notice( cat_net_tf, "terminate net" );
    return NULL;
}
