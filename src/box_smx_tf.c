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
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "box_smx_tf.h"
#include "smxutils.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
void smx_connect_tf( smx_net_t* net, smx_channel_t* ch_in,
        smx_channel_t* ch_out )
{
    smx_timer_t* timer = SMX_SIG( net );
    if( net == NULL || timer == NULL )
    {
        SMX_LOG_MAIN( main, fatal,
                "unable to connect tf: timer not initialised" );
        return;
    }

    SMX_LOG_MAIN( ch, info, "connect '%s(%d)%s%s(%d)'", ch_in->name,
            ch_in->id, SMX_MODE_in, "smx_tf", net->id );
    SMX_LOG_MAIN( ch, info, "connect '%s(%d)%s%s(%d)'", ch_out->name,
            ch_out->id, SMX_MODE_out, "smx_tf", net->id );
    net_smx_tf_t* tf = smx_malloc( sizeof( struct net_smx_tf_s ) );
    if( tf == NULL )
        return;
    tf->in = ch_in;
    tf->out = ch_out;
    tf->next = timer->tfs;
    timer->tfs = tf;
    timer->count++;
}

/*****************************************************************************/
smx_timer_t* smx_net_create_tf( int sec, int nsec )
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
    timer->tfs = NULL;
    return timer;
}

/*****************************************************************************/
void smx_net_init_tf( smx_net_t* net )
{
    int i = 0;
    smx_timer_t* tt = SMX_SIG( net );
    net_smx_tf_t* tf = tt->tfs;
    net->in.ports = smx_malloc( sizeof( struct smx_channel_s ) * tt->count );
    net->in.count = tt->count;
    net->out.ports = smx_malloc( sizeof( struct smx_channel_s ) * tt->count );
    net->out.count = tt->count;
    for( i = 0; i < tt->count; i++ )
    {
        net->in.ports[i] = tf->in;
        net->out.ports[i] = tf->out;
        tf = tf->next;
    }
}

/*****************************************************************************/
void smx_net_destroy_tf( smx_net_t* tt )
{
    if( tt == NULL )
        return;

    smx_timer_t* sig = SMX_SIG( tt );
    net_smx_tf_t* tf = sig->tfs;
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
void smx_tf_enable( smx_net_t* h )
{
    smx_timer_t* timer = SMX_SIG( h );
    if( h == NULL || timer == NULL )
        return;

    if( -1 == timerfd_settime( timer->fd, 0, &timer->itval, NULL ) )
        SMX_LOG_NET( h, error, "timerfd_settime: %d", errno );
}

/*****************************************************************************/
void smx_tf_propagate_msgs( smx_net_t* h, int copy )
{
    int i;
    smx_timer_t* tt = SMX_SIG( h );
    smx_channel_t** ch_in = SMX_SIG_PORTS( h, in );
    smx_channel_t** ch_out = SMX_SIG_PORTS( h, out );
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
                pthread_mutex_lock( &ch_out[i]->ch_mutex );
                smx_channel_change_read_state( ch_out[i], SMX_CHANNEL_END );
                pthread_mutex_unlock( &ch_out[i]->ch_mutex );
            }
            if( ch_out[i]->sink->state == SMX_CHANNEL_END )
            {
                // propagate termination signal to producer
                pthread_mutex_lock( &ch_in[i]->ch_mutex );
                smx_channel_change_write_state( ch_in[i], SMX_CHANNEL_END );
                pthread_mutex_unlock( &ch_in[i]->ch_mutex );
            }
            continue;
        }

        // read msgs
        pthread_mutex_lock( &ch_in[i]->ch_mutex );
        if( copy )
            msg = smx_fifo_d_read( h, ch_in[i], ch_in[i]->fifo );
        else
            msg = smx_fifo_dd_read( h, ch_in[i], ch_in[i]->fifo );

        // notify producer that space is available
        smx_channel_change_write_state( ch_in[i], SMX_CHANNEL_READY );
        pthread_mutex_unlock( &ch_in[i]->ch_mutex );

        if( msg == NULL || ch_in[i]->fifo->copy )
        {
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
void smx_tf_wait( smx_net_t* h )
{
    uint64_t expired;
    struct pollfd pfd;
    int poll_res;
    smx_timer_t* timer = SMX_SIG( h );
    if( h == NULL || timer == NULL )
        return;

    pfd.fd = timer->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_res = poll( &pfd, 1, 0 ); // non-blocking poll to check for timer event
    if( -1 == poll_res )
        SMX_LOG_NET( h, error, "timerfd poll: %d", errno );
    else if( poll_res > 0 )
        SMX_LOG_NET( h, notice, "deadline missed" );

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
int smx_tf( void* h, void* state )
{
    net_smx_tf_state_t* tf_state = state;
    smx_tf_propagate_msgs( h, tf_state->do_copy );
    SMX_LOG_NET( h, debug, "wait for end of loop" );
    smx_tf_wait( h );
    return SMX_NET_RETURN;
}

/*****************************************************************************/
void smx_tf_cleanup( void* h, void* state )
{
    ( void )( h );
    if( state == NULL)
        return;

    free( state );
}

/*****************************************************************************/
int smx_tf_init( void* h, void** state )
{
    smx_timer_t* tt = SMX_SIG( h );
    xmlNodePtr cur = SMX_NET_GET_CONF( h );
    xmlChar* copy_str = NULL;
    net_smx_tf_state_t* tf_state = NULL;

    if( h == NULL || tt ==  NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to start tf: not initialised" );
        return -1;
    }

    tf_state = smx_malloc( sizeof( struct net_smx_tf_state_s ) );
    tf_state->do_copy = 0;

    if( cur != NULL )
    {
        copy_str = xmlGetProp(cur, (const xmlChar*)"copy");
        if( copy_str != NULL )
        {
            if( ( strcmp( ( const char* )copy_str, "on" ) == 0 )
                    || ( strcmp( ( const char* )copy_str, "1" ) == 0 ) )
                tf_state->do_copy = 1;
            else if( ( strcmp( ( const char* )copy_str, "off" ) == 0 )
                    || ( strcmp( ( const char* )copy_str, "0" ) == 0 ) )
                tf_state->do_copy = 0;
            else
            {
                SMX_LOG_NET( h, warn,
                        "value of property 'copy' is invalid (%s), using 'off'",
                        copy_str );
            }
            xmlFree( copy_str );
        }
        else
        {
            SMX_LOG_NET( h, notice, "property 'copy' is not set, using 'off'" );
        }
    }

    SMX_LOG_NET( h, notice, "start net" );
    smx_tf_enable( h );
    *state = tf_state;
    return 0;
}

/*****************************************************************************/
void* start_routine_smx_tf( void* h )
{
    return start_routine_net( h, smx_tf, smx_tf_init, smx_tf_cleanup );
}
