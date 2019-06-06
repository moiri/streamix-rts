/**
 * Net definitions for the runtime system library of Streamix
 *
 * @file    smxnet.h
 * @author  Simon Maurer
 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "smxnet.h"
#include "smxutils.h"
#include "smxprofiler.h"

/*****************************************************************************/
smx_msg_t* smx_net_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx )
{
    bool has_msg = false;
    int cur_count, i, ch_count, ready_cnt;
    smx_msg_t* msg = NULL;
    smx_channel_t* ch = NULL;
    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_NET( h, debug, "waiting for message on collector" );
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    }
    if( collector->count > 0 )
    {
        collector->count--;
        has_msg = true;
    }
    else
    {
        SMX_LOG_NET( h, debug, "collector state change %d -> %d", collector->state,
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
            SMX_LOG_NET( h, error,
                    "something went wrong: no msg ready in collector (count: %d)",
                    collector->count );
            return NULL;
        }
        SMX_LOG_NET( h, info, "read from collector (new count: %d)", cur_count );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_READ_COLLECTOR );
        msg = smx_channel_read( h, ch );
    }
    return msg;
}

/*****************************************************************************/
int smx_net_create( smx_net_t** nets, int* net_cnt, unsigned int id,
        const char* name, const char* cat_name, void* sig, void** conf )
{
    nets[id] = NULL;
    // sig is allocated in the macro, hence, the NULL check is done here
    if( sig == NULL )
        return -1;

    if( id >= SMX_MAX_NETS )
    {
        SMX_LOG_MAIN( main, fatal, "net count exeeds maximum %d", id );
        return -1;
    }

    xmlNodePtr cur = NULL;
    smx_net_t* net = smx_malloc( sizeof( struct smx_net_s ) );
    if( net == NULL )
        return -1;

    net->id = id;
    net->cat = zlog_get_category( cat_name );
    net->sig = sig;
    net->conf = NULL;
    net->profiler = NULL;
    net->name = name;

    cur = xmlDocGetRootElement( (xmlDocPtr)*conf );
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

    nets[id] = net;
    (*net_cnt)++;
    SMX_LOG_MAIN( net, info, "create net instance %s(%d)", name, id );
    return 0;
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
int smx_net_run( pthread_t* ths, int idx, void* box_impl( void* arg ), void* h,
        int prio )
{
    xmlChar* profiler_port = NULL;
    smx_net_t* net = h;
    pthread_attr_t sched_attr;
    struct sched_param fifo_param;
    pthread_t thread;
    int min_fifo, max_fifo;

    if( idx >= SMX_MAX_NETS )
    {
        SMX_LOG_MAIN( main, fatal, "thread count exeeds maximum %d", idx );
        return -1;
    }

    if( net != NULL && net->conf != NULL )
    {
        profiler_port = xmlGetProp( net->conf, ( const xmlChar* )"profiler" );
        if( profiler_port != NULL )
            net->profiler = smx_get_channel_by_name( net->out.ports,
                    net->out.count, ( const char* )profiler_port );
    }

    pthread_attr_init(&sched_attr);
    if( prio > 0 )
    {
        min_fifo = sched_get_priority_min( SCHED_FIFO );
        max_fifo = sched_get_priority_max( SCHED_FIFO );
        fifo_param.sched_priority = min_fifo;
        if( fifo_param.sched_priority > max_fifo )
        {
            SMX_LOG_NET( h, warn, "cannot use therad priority of %d, falling back\
                    to the max priority %d", fifo_param.sched_priority,
                    max_fifo );
            fifo_param.sched_priority = max_fifo;
        }
        pthread_attr_setinheritsched(&sched_attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&sched_attr, SCHED_FIFO);
        pthread_attr_setschedparam(&sched_attr, &fifo_param);
        SMX_LOG_NET( h, debug, "creating RT thread of priority %d",
                fifo_param.sched_priority );
    }
    if( ( errno = pthread_create( &thread, &sched_attr, box_impl, h ) ) != 0 )
    {
        SMX_LOG_NET( h, error, "failed to create a new thread: %s",
                strerror( errno ) );
        return -1;
    }
    ths[idx] = thread;
    return 0;
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
        SMX_LOG_MAIN( main, fatal, "net channels not initialised" );
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
        SMX_LOG_NET( h, debug, "all triggering producers have terminated" );
        return SMX_NET_END;
    }

    // if all the outputs are done, terminate the thread
    if( (len_out) > 0 && (done_cnt_out >= len_out) )
    {
        SMX_LOG_NET( h, debug, "all consumers have terminated" );
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

    SMX_LOG_NET( h, notice, "send termination notice to neighbours" );
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
void* start_routine_net( int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void* ), void* h, smx_channel_t** chs_in, int* cnt_in,
        smx_channel_t** chs_out, int* cnt_out )
{
    if( h == NULL || chs_in ==  NULL || chs_out == NULL || cnt_in == NULL
            || cnt_out == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to start net: not initialised" );
        return NULL;
    }

    int state = SMX_NET_CONTINUE;
    SMX_LOG_NET( h, notice, "init net" );
    void* net_state = NULL;
    if(init( h, &net_state ) == 0)
    {
        SMX_LOG_NET( h, notice, "start net" );
        while( state == SMX_NET_CONTINUE )
        {
            SMX_LOG_NET( h, info, "start net loop" );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_START );
            state = impl( h, net_state );
            state = smx_net_update_state( h, chs_in, *cnt_in, chs_out, *cnt_out,
                    state );
        }
    }
    else
        SMX_LOG_NET( h, error, "initialisation of net failed" );
    smx_net_terminate( h, chs_in, *cnt_in, chs_out, *cnt_out );
    SMX_LOG_NET( h, notice, "cleanup net" );
    cleanup( net_state );
    SMX_LOG_NET( h, notice, "terminate net" );
    return NULL;
}
