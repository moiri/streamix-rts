/**
 * Temporal firewall box implementation for the runtime system library of
 * Streamix
 *
 * @file    box_smx_tf.h
 * @author  Simon Maurer
 */

#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <poll.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "box_smx_tf.h"
#include "smxutils.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out, int id )
{
    if( timer == NULL )
    {
        SMX_LOG_MAIN( main, fatal,
                "unable to connect tf: timer not initialised" );
        return;
    }

    SMX_LOG_MAIN( ch, info, "connect '%s(%d)%s%s(%d)'", ch_in->name,
            ch_in->id, SMX_MODE_in, "smx_tf", id );
    SMX_LOG_MAIN( ch, info, "connect '%s(%d)%s%s(%d)'", ch_out->name,
            ch_out->id, SMX_MODE_out, "smx_tf", id );
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
        SMX_LOG_MAIN( main, error, "timerfd_create: %d", errno );
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
        SMX_LOG_NET( h, error, "timerfd_settime: %d", errno );
}

/*****************************************************************************/
void smx_tf_propagate_msgs( void* h, smx_timer_t* tt, smx_channel_t** ch_in,
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
            msg = smx_fifo_d_read( h, ch_in[i], ch_in[i]->fifo );
        else
            msg = smx_fifo_dd_read( h, ch_in[i], ch_in[i]->fifo );

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
            smx_channel_write( h, ch_out[i], msg );
            if( ch_out[i]->fifo->overwrite )
                zlog_error( ch_out[i]->cat, "missed deadline to consume" );
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
        SMX_LOG_NET( h, error, "timerfd poll: %d", errno );
    else if( poll_res > 0 )
        SMX_LOG_NET( h, error, "deadline missed" );

    if( -1 == read( timer->fd, &expired, sizeof( uint64_t ) ) )
        SMX_LOG_NET( h, error, "timerfd read: %d", errno );
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
        SMX_LOG_MAIN( main, fatal, "unable to start tf: not initialised" );
        return NULL;
    }

    SMX_LOG_NET( h, notice, "init net" );
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
            SMX_LOG_NET( h, error, "invalid tf configuartion, no property 'copy'" );
        else
        {
            copy = atoi( ( const char* )copy_str );
            xmlFree( copy_str );
        }
    }

    SMX_LOG_NET( h, notice, "start net" );
    smx_tf_enable( h, tt );
    while( state == SMX_NET_CONTINUE )
    {
        SMX_LOG_NET( h, info, "start net loop" );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_START );
        smx_tf_propagate_msgs( h, tt, ch_in, ch_out, copy );
        SMX_LOG_NET( h, debug, "wait for end of loop" );
        smx_tf_wait( h, tt );
        state = smx_net_update_state( h, ch_in, tt->count, ch_out, tt->count,
                SMX_NET_RETURN );
    }
    smx_net_terminate( h, ch_in, tt->count, ch_out, tt->count );
    SMX_LOG_NET( h, notice, "terminate net" );
    return NULL;
}
