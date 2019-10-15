/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Channel and FIFO definitions for the runtime system library of Streamix
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "smxch.h"
#include "smxmsg.h"
#include "smxutils.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
void smx_channel_change_collector_state( smx_channel_t* ch,
        smx_channel_state_t state )
{
    if(ch->collector->state != state
            && ch->collector->state != SMX_CHANNEL_END )
    {
        SMX_LOG_CH( ch, debug, "collector state change %d -> %d",
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
        SMX_LOG_CH( ch, debug, "read state change %d -> %d",
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
        SMX_LOG_CH( ch, debug, "write state change %d -> %d",
                ch->sink->state, state );
        ch->sink->state = state;
        pthread_cond_signal( &ch->sink->ch_cv );
    }
}

/*****************************************************************************/
smx_channel_t* smx_channel_create( int* ch_cnt, int len,
        smx_channel_type_t type, int id, const char* name,
        const char* cat_name )
{
    pthread_mutexattr_t mutexattr_prioinherit;
    if( id >= SMX_MAX_CHS )
    {
        SMX_LOG_MAIN( main, fatal, "channel count exeeds maximum %d", id );
        return NULL;
    }
    smx_channel_t* ch = smx_malloc( sizeof( struct smx_channel_s ) );
    if( ch == NULL )
        return NULL;

    SMX_LOG_MAIN( ch, info, "create channel '%s(%d)' of length %d", name, id,
            len );
    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );

    pthread_mutex_init( &ch->ch_mutex, &mutexattr_prioinherit );
    ch->id = id;
    ch->type = type;
    ch->fifo = smx_fifo_create( len );
    ch->collector = NULL;
    ch->guard = NULL;
    ch->name = name;
    ch->cat = zlog_get_category( cat_name );
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
    {
        smx_channel_destroy( ch );
        return NULL;
    }
    (*ch_cnt)++;
    return ch;
}

/*****************************************************************************/
smx_channel_end_t* smx_channel_create_end()
{
    smx_channel_end_t* end = smx_malloc( sizeof( struct smx_channel_end_s ) );
    if( end == NULL )
        return NULL;

    end->count = 0;
    end->err = SMX_CHANNEL_ERR_NONE;
    end->net = NULL;
    end->filter.items = NULL;
    end->filter.count = 0;
    end->content_filter = NULL;
    pthread_cond_init( &end->ch_cv, NULL );
    return end;
}

/*****************************************************************************/
void smx_channel_destroy( smx_channel_t* ch )
{
    if( ch == NULL )
        return;
    SMX_LOG_MAIN( ch, debug, "destroy channel '%s(%d)' (msg count: %d)",
            ch->name, ch->id, ch->fifo->count );
    smx_guard_destroy( ch->guard );
    smx_fifo_destroy( ch->fifo );
    smx_channel_destroy_end( ch->sink );
    smx_channel_destroy_end( ch->source );
    pthread_mutex_destroy( &ch->ch_mutex );
    free( ch );
}

/*****************************************************************************/
void smx_channel_destroy_end( smx_channel_end_t* end )
{
    if( end == NULL )
        return;
    int i;
    if( end->filter.items != NULL )
    {
        for( i = 0; i < end->filter.count; i++ )
            free( end->filter.items[i] );
        free( end->filter.items );
    }
    pthread_cond_destroy( &end->ch_cv );
    free( end );
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( void* h, smx_channel_t* ch )
{
    smx_msg_t* msg = NULL;
    if( ch == NULL )
        return NULL;

    if( ch->source ==  NULL )
    {
        SMX_LOG_MAIN( main, fatal, "channel not initialised" );
        return NULL;
    }

    pthread_mutex_lock( &ch->ch_mutex);
    while( ch->source->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_CH( ch, debug, "waiting for message" );
        pthread_cond_wait( &ch->source->ch_cv, &ch->ch_mutex );
    }
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            msg = smx_fifo_read( h, ch, ch->fifo );
            break;
        case SMX_FIFO_D:
        case SMX_D_FIFO_D:
            msg = smx_fifo_d_read( h, ch, ch->fifo );
            break;
        default:
            pthread_mutex_unlock( &ch->ch_mutex );
            SMX_LOG_CH( ch, error, "undefined channel type '%d'",
                    ch->type );
            ch->source->err = SMX_CHANNEL_ERR_UNINITIALISED;
            return NULL;
    }
    // notify producer that space is available
    smx_channel_change_write_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->ch_mutex );
    return msg;
}

/*****************************************************************************/
int smx_channel_ready_to_read( smx_channel_t* ch )
{
    if( ch == NULL )
        return -1;

    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            return ch->fifo->count;
        case SMX_D_FIFO_D:
        case SMX_FIFO_D:
            return 1;
        default:
            SMX_LOG_CH( ch, error, "undefined channel type '%d'",
                    ch->type );
            return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_channel_ready_to_write( smx_channel_t* ch )
{
    if( ch == NULL )
        return -1;

    switch( ch->type ) {
        case SMX_D_FIFO:
        case SMX_D_FIFO_D:
            return 1;
        case SMX_FIFO_D:
        case SMX_FIFO:
            return ch->fifo->length - ch->fifo->count;
        default:
            SMX_LOG_CH( ch, error, "undefined channel type '%d'",
                    ch->type );
            return -1;
    }
    return 0;
}

/*****************************************************************************/
bool smx_channel_set_content_filter( smx_channel_t* ch,
        bool filter( smx_msg_t* ) )
{
    if( ch == NULL || ch->sink == NULL || filter == NULL )
        return false;
    SMX_LOG_CH( ch, notice, "adding message content filter" );
    ch->sink->content_filter = filter;
    return true;
}

/*****************************************************************************/
bool smx_channel_set_filter( smx_net_t* h, smx_channel_t* ch, int count, ... )
{
    int i;
    va_list arg_ptr;
    const char* arg;

    if( !h->has_type_filter )
        return false;

    SMX_LOG_CH( ch, notice, "adding message type filter" );
    ch->sink->filter.items = smx_malloc( sizeof( char* ) * count );
    ch->sink->filter.count = count;

    va_start( arg_ptr, count );

    for( i = 0; i < count; i++ )
    {
        arg = va_arg( arg_ptr, char* );
        ch->sink->filter.items[i] = arg ? strdup( arg ) : NULL;
        SMX_LOG_CH( ch, notice, "allow message type '%s'", arg );
    }

    va_end( arg_ptr );

    return true;
}

/*****************************************************************************/
void smx_channel_terminate_sink( smx_channel_t* ch )
{
    zlog_debug( ch->cat, "mark as stale" );
    pthread_mutex_lock( &ch->ch_mutex );
    smx_channel_change_write_state( ch, SMX_CHANNEL_END );
    pthread_mutex_unlock( &ch->ch_mutex );
}

/*****************************************************************************/
void smx_channel_terminate_source( smx_channel_t* ch )
{
    zlog_debug( ch->cat, "mark as stale" );
    pthread_mutex_lock( &ch->ch_mutex );
    smx_channel_change_read_state( ch, SMX_CHANNEL_END );
    pthread_mutex_unlock( &ch->ch_mutex );
}

/*****************************************************************************/
int smx_channel_write( void* h, smx_channel_t* ch, smx_msg_t* msg )
{
    bool abort = false;
    int new_count;
    int i;
    const char* filter;
    bool pass = false;
    if( ch == NULL )
    {
        // channel is open, dismiss message silently.
        smx_msg_destroy( h, msg, true );
        return 0;
    }

    if( ch->sink == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "channel not initialised" );
        return -1;
    }

    if( msg == NULL )
    {
        SMX_LOG_MAIN( main, warn, "write aborted: message is NULL" );
        ch->sink->err = SMX_CHANNEL_ERR_NO_DATA;
        return -1;
    }

    if( ch->sink->content_filter != NULL && !ch->sink->content_filter( msg ) )
    {
        SMX_LOG_CH( ch, debug, "msg content filter failed, dismissing msg" );
        smx_msg_destroy( h, msg, true );
        return 0;
    }

    if( ch->sink->filter.items != NULL )
    {
        for( i = 0; i < ch->sink->filter.count; i++ )
        {
            filter = ch->sink->filter.items[i];
            if( ( msg->type == NULL && filter == NULL )
                    || ( ( msg->type != NULL ) && ( filter != NULL )
                        && strcmp( msg->type, filter ) == 0 ) )
            {
                pass = true;
                break;
            }
        }

        if( !pass )
        {
            ch->sink->err = SMX_CHANNEL_ERR_FILTER;
            SMX_LOG_CH( ch, error, "write aborted: msg type '%s' did not pass"
                    " filter, msg dismissed (%ld)",
                    msg->type ? msg->type : "unknonw", msg->id );
            smx_msg_destroy( h, msg, true );
            return -1;
        }
    }

    pthread_mutex_lock( &ch->ch_mutex );
    while( ch->sink->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_CH( ch, debug, "waiting for free space" );
        pthread_cond_wait( &ch->sink->ch_cv, &ch->ch_mutex );
    }
    if( ch->sink->state == SMX_CHANNEL_END ) abort = true;
    if( abort )
    {
        pthread_mutex_unlock( &ch->ch_mutex );
        SMX_LOG_CH( ch, notice, "write aborted: consumer has termninated" );
        smx_msg_destroy( h, msg, true );
        ch->sink->err = SMX_CHANNEL_ERR_NO_TARGET;
        return -1;
    }
    switch( ch->type )
    {
        case SMX_FIFO:
        case SMX_FIFO_D:
            if( ch->guard != NULL )
                smx_guard_write( h, ch );
            if( smx_fifo_write( h, ch, ch->fifo, msg ) < 0 )
            {
                SMX_LOG_CH( ch, error, "write to fifo failed" );
                smx_msg_destroy( h, msg, true );
            }
            break;
        case SMX_D_FIFO:
        case SMX_D_FIFO_D:
            if( ch->guard != NULL )
                // discard message if miat is not reached
                if( smx_d_guard_write( h, ch, msg ) )
                {
                    pthread_mutex_unlock( &ch->ch_mutex );
                    return 0;
                }
            smx_d_fifo_write( h, ch, ch->fifo, msg );
            break;
        default:
            pthread_mutex_unlock( &ch->ch_mutex );
            SMX_LOG_CH( ch, error, "undefined channel type '%d'", ch->type );
            ch->sink->err = SMX_CHANNEL_ERR_UNINITIALISED;
            return -1;
    }
    if( ch->collector != NULL && ch->fifo->overwrite == 0 )
    {
        pthread_mutex_lock( &ch->collector->col_mutex );
        ch->collector->count++;
        new_count = ch->collector->count;
        smx_channel_change_collector_state( ch, SMX_CHANNEL_READY );
        pthread_mutex_unlock( &ch->collector->col_mutex );
        SMX_LOG_CH( ch, info, "write to collector (new count: %d)",
                new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_WRITE_COLLECTOR,
                new_count );
    }
    // notify consumer that messages are available
    smx_channel_change_read_state( ch, SMX_CHANNEL_READY );
    pthread_mutex_unlock( &ch->ch_mutex );
    return 0;
}

/*****************************************************************************/
smx_collector_t* smx_collector_create()
{
    pthread_mutexattr_t mutexattr_prioinherit;
    smx_collector_t* collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( collector == NULL )
        return NULL;

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &collector->col_mutex, &mutexattr_prioinherit );
    pthread_cond_init( &collector->col_cv, NULL );
    collector->count = 0;
    collector->state = SMX_CHANNEL_PENDING;
    return collector;
}

/*****************************************************************************/
void smx_collector_destroy( smx_collector_t* collector )
{
    if( collector == NULL )
        return;

    pthread_mutex_destroy( &collector->col_mutex );
    pthread_cond_destroy( &collector->col_cv );
    free( collector );
}

/*****************************************************************************/
void smx_collector_terminate( smx_channel_t* ch )
{
    if( ch->collector == NULL )
        return;

    zlog_debug( ch->cat, "mark collector as stale" );
    pthread_mutex_lock( &ch->collector->col_mutex );
    smx_channel_change_collector_state( ch, SMX_CHANNEL_END );
    pthread_mutex_unlock( &ch->collector->col_mutex );
}

/*****************************************************************************/
void smx_connect( smx_channel_t** dest, smx_channel_t* src, int net_id,
        const char* net_name, const char* mode, int* count )
{
    const char* elem;
    if( dest ==  NULL || src == NULL )
    {
        elem = ( dest == NULL ) ? "dest" : "src";
        SMX_LOG_MAIN( main, fatal,
                "unable to connect channels: not initialised, %s", elem );
        return;
    }
    SMX_LOG_MAIN( ch, info, "connect '%s(%d)%s%s(%d)'", src->name, src->id,
            mode, net_name, net_id );
    *dest = src;
    ( *count )++;
}

/*****************************************************************************/
void smx_connect_guard( smx_channel_t* ch, smx_guard_t* guard )
{
    const char* elem;
    if( ch == NULL || guard == NULL )
    {
        elem = ( ch == NULL ) ? "channel" : "guard";
        SMX_LOG_MAIN( main, fatal,
                "unable to connect guard: not initialised %s", elem );
        return;
    }
    ch->guard = guard;
}

/*****************************************************************************/
void smx_connect_in( smx_channel_t** dest, smx_channel_t* src, smx_net_t* net,
        const char* mode, int* count )
{
    src->source->net = net;
    smx_connect( dest, src, net->id, net->name, mode, count );
}

/*****************************************************************************/
void smx_connect_out( smx_channel_t** dest, smx_channel_t* src, smx_net_t* net,
        const char* mode, int* count )
{
    src->sink->net = net;
    smx_connect( dest, src, net->id, net->name, mode, count );
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

    for( int i=0; i < fifo->length; i++ ) {
        if( fifo->head == NULL )
            continue;

        if( fifo->head->msg != NULL )
            smx_msg_destroy( NULL, fifo->head->msg, true );
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    if( fifo->backup != NULL )
        smx_msg_destroy( NULL, fifo->backup, true );
    free( fifo );
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( void* h, smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    int new_count;
    if( ch == NULL || fifo == NULL )
        return NULL;

    ch->source->err = SMX_CHANNEL_ERR_NONE;

    if( fifo->count > 0 )
    {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;
        if( fifo->count == 0 )
        {
            smx_channel_change_read_state( ch, SMX_CHANNEL_PENDING );
        }
        new_count = fifo->count;

        SMX_LOG_CH( ch, info, "read from fifo (new count: %d)", new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_READ, new_count );
    }
    else if( ch->source->state != SMX_CHANNEL_END )
    {
        new_count = fifo->count;
        SMX_LOG_CH( ch, error, "channel is ready but is empty (%d/%d)",
                new_count, fifo->length );
        ch->source->err = SMX_CHANNEL_ERR_NO_DATA;
        return NULL;
    }
    else
        ch->source->err = SMX_CHANNEL_ERR_NO_TARGET;

    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_d_read( void* h, smx_channel_t* ch, smx_fifo_t* fifo )
{
    int new_count;
    smx_msg_t* msg = NULL;
    smx_msg_t* old_backup = NULL;
    if( ch == NULL || fifo == NULL )
        return NULL;

    ch->source->err = SMX_CHANNEL_ERR_NONE;

    if( fifo->count > 0 )
    {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        if( fifo->count == 1 )
        {
            // last message, backup for later duplication
            if( fifo->backup != NULL ) // delete old backup
                old_backup = fifo->backup;
            fifo->backup = smx_msg_copy( h, msg );
        }
        fifo->count--;
        fifo->copy = 0;
        new_count = fifo->count;

        smx_msg_destroy( h, old_backup, true );
        SMX_LOG_CH( ch, info, "read from fifo_d (new count: %d)", new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_READ, new_count );
    }
    else
    {
        if( fifo->backup != NULL )
        {
            msg = smx_msg_copy( h, fifo->backup );
            fifo->copy++;

            SMX_LOG_CH( ch, info, "fifo_d is empty, duplicate backup" );
            smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_DUPLICATE, 0 );
        }
        else
        {
            SMX_LOG_CH( ch, notice,
                    "nothing to read, fifo and its backup is empty" );
            ch->source->err = SMX_CHANNEL_ERR_NO_DEFAULT;
        }
    }
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_dd_read( void* h, smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    int new_count;
    if( ch == NULL || fifo == NULL )
        return NULL;

    ch->source->err = SMX_CHANNEL_ERR_NONE;

    if( fifo->count > 0 )
    {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;
        new_count = fifo->count;

        SMX_LOG_CH( ch, info, "read from fifo_dd (new count: %d)", new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_READ, new_count );
    }
    else if( ch->source->state != SMX_CHANNEL_END )
        ch->source->err = SMX_CHANNEL_ERR_DL_MISS;
    else
        ch->source->err = SMX_CHANNEL_ERR_NO_TARGET;
    return msg;
}

/*****************************************************************************/
int smx_fifo_write( void* h, smx_channel_t* ch, smx_fifo_t* fifo,
        smx_msg_t* msg )
{
    int new_count;
    if( ch == NULL || fifo == NULL || msg == NULL )
        return -1;

    if(fifo->count < fifo->length)
    {
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        if( fifo->count == fifo->length )
        {
            smx_channel_change_write_state( ch, SMX_CHANNEL_PENDING );
        }
        new_count = fifo->count;

        SMX_LOG_CH( ch, info, "write to fifo (new count: %d)", new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_WRITE, new_count );
    }
    else
    {
        new_count = fifo->count;
        SMX_LOG_CH( ch, warn, "channel is ready but has no space (%d/%d)",
                new_count, fifo->length );
        ch->sink->err = SMX_CHANNEL_ERR_NO_SPACE;
        return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_d_fifo_write( void* h, smx_channel_t* ch, smx_fifo_t* fifo,
        smx_msg_t* msg )
{
    int new_count;
    smx_msg_t* msg_tmp = NULL;
    if( ch == NULL || fifo == NULL || msg == NULL )
        return -1;

    SMX_LOG_CH( ch, debug, "prepare to write to fifo_d" );
    if( fifo->count < fifo->length )
    {
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        new_count = fifo->count;
        fifo->overwrite = 0;

        SMX_LOG_CH( ch, info, "write to fifo_d (new count: %d)", new_count );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_WRITE, new_count );
    }
    else
    {
        msg_tmp = fifo->tail->msg;
        fifo->tail->msg = msg;
        fifo->overwrite++;

        smx_msg_destroy( h, msg_tmp, true );
        SMX_LOG_CH( ch, notice, "overwrite tail of fifo" );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_OVERWRITE,
                fifo->length );
    }
    return 0;
}

/*****************************************************************************/
smx_channel_t* smx_get_channel_by_name( smx_channel_t** ports, int count,
        const char* name )
{
    int i;
    for( i = 0; i < count; i++ )
        if( 0 == strcmp( ports[i]->name, name ) )
            return ports[i];
    return NULL;
}

/*****************************************************************************/
smx_channel_err_t smx_get_read_error( smx_channel_t* ch )
{
    if( ch == NULL || ch->source == NULL || ch->fifo == NULL )
        return SMX_CHANNEL_ERR_UNINITIALISED;

    return ch->source->err;
}

/*****************************************************************************/
smx_channel_err_t smx_get_write_error( smx_channel_t* ch )
{
    if( ch == NULL || ch->sink == NULL || ch->fifo == NULL )
        return SMX_CHANNEL_ERR_UNINITIALISED;

    return ch->sink->err;
}

/*****************************************************************************/
smx_guard_t* smx_guard_create( int iats, int iatns, smx_channel_t* ch )
{
    struct itimerspec itval;
    SMX_LOG_CH( ch, debug, "create guard" );
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
        SMX_LOG_CH( ch, error, "failed to create guard timer \
                (timerfd_create returned %d", errno );
        free( guard );
        return NULL;
    }
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
    {
        SMX_LOG_CH( ch, error, "failed to arm guard timer \
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
int smx_guard_write( void* h, smx_channel_t* ch )
{
    uint64_t expired;
    struct itimerspec itval;
    (void)(h);
    if( ch == NULL || ch->guard == NULL )
        return -1;

    if( -1 == read( ch->guard->fd, &expired, sizeof( uint64_t ) ) )
    {
        SMX_LOG_CH( ch, error, "falied to read guard timer \
                (read returned %d", errno );
        return -1;
    }
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
    {
        SMX_LOG_CH( ch, error, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
        return -1;
    }
    return 0;
}

/*****************************************************************************/
int smx_d_guard_write( void* h, smx_channel_t* ch, smx_msg_t* msg )
{
    struct itimerspec itval;
    if( ch == NULL || ch->guard == NULL )
        return -1;

    if( -1 == timerfd_gettime( ch->guard->fd, &itval ) )
    {
        SMX_LOG_CH( ch, error, "falied to read guard timer \
                (timerfd_gettime returned %d", errno );
        return -1;
    }
    if( ( itval.it_value.tv_sec != 0 ) || ( itval.it_value.tv_nsec != 0 ) ) {
        SMX_LOG_CH( ch, notice, "rate_control: discard message '%lu'",
                msg->id );
        smx_profiler_log_ch( h, ch, msg, SMX_PROFILER_ACTION_DISMISS,
                ch->fifo->count );
        smx_msg_destroy( h, msg, true );
        return 1;
    }
    itval.it_value = ch->guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( ch->guard->fd, 0, &itval, NULL ) )
    {
        SMX_LOG_CH( ch, error, "failed to re-arm guard timer \
                (timerfd_settime retuned %d", errno );
        return -1;
    }
    return 0;
}
