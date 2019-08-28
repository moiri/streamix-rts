/**
 * Net definitions for the runtime system library of Streamix
 *
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "smxch.h"
#include "smxconfig.h"
#include "smxnet.h"
#include "smxprofiler.h"
#include "smxutils.h"

/*****************************************************************************/
int smx_net_collector_check_avaliable( void* h, smx_collector_t* collector )
{
    int cur_count;
    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
    {
        SMX_LOG_NET( h, debug, "waiting for message on collector" );
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    }
    cur_count = collector->count;
    if( collector->count > 0 )
    {
        collector->count--;
    }
    else
    {
        SMX_LOG_NET( h, debug, "collector state change %d -> %d", collector->state,
                SMX_CHANNEL_PENDING );
        collector->state = SMX_CHANNEL_PENDING;
    }
    pthread_mutex_unlock( &collector->col_mutex );
    return cur_count;
}

/*****************************************************************************/
smx_msg_t* smx_net_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx )
{
    int cur_count, i, ch_count, ready_cnt;
    smx_msg_t* msg = NULL;
    smx_channel_t* ch = NULL;

    cur_count = smx_net_collector_check_avaliable( h, collector );

    if( cur_count > 0 )
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
                    cur_count );
            return NULL;
        }
        SMX_LOG_NET( h, info, "read from collector (new count: %d)",
                cur_count - 1 );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_READ_COLLECTOR );
        msg = smx_channel_read( h, ch );
    }
    return msg;
}

/*****************************************************************************/
smx_net_t* smx_net_create( int* net_cnt, unsigned int id, const char* name,
        const char* impl, const char* cat_name, void* conf,
        pthread_barrier_t* init_done, int prio )
{
    if( id >= SMX_MAX_NETS )
    {
        SMX_LOG_MAIN( main, fatal, "net count exeeds maximum %d", id );
        return NULL;
    }

    smx_net_t* net = smx_malloc( sizeof( struct smx_net_s ) );
    if( net == NULL )
        return NULL;

    net->sig = smx_malloc( sizeof( struct smx_net_sig_s ) );
    if( net->sig == NULL )
    {
        free( net );
        return NULL;
    }
    net->start_wall.tv_sec = 0;
    net->start_wall.tv_nsec = 0;
    net->end_wall.tv_sec = 0;
    net->end_wall.tv_nsec = 0;
    net->count = 0;
    net->sig->in.ports = NULL;
    net->sig->in.count = 0;
    net->sig->in.len = 0;
    net->sig->out.ports = NULL;
    net->sig->out.count = 0;
    net->sig->out.len = 0;

    net->id = id;
    net->priority = prio;
    net->init_done = init_done;
    net->cat = zlog_get_category( cat_name );
    net->name = name;
    net->impl = impl;
    net->attr = NULL;
    net->conf = bson_new();
    net->has_profiler = smx_net_has_profiler( conf, name, impl, id );
    smx_net_get_json_doc( net, conf, name, impl, id );

    (*net_cnt)++;
    SMX_LOG_MAIN( net, info, "create net instance %s(%d)", name, id );
    return net;
}

/*****************************************************************************/
void smx_net_destroy( smx_net_t* h )
{
    if( h != NULL )
    {
        bson_free( h->conf );
        if( h->sig != NULL )
        {
            if( h->sig->in.ports != NULL )
                free( h->sig->in.ports );
            if( h->sig->out.ports != NULL )
                free( h->sig->out.ports );
            free( h->sig );
        }
        free( h );
    }
}

/*****************************************************************************/
int smx_net_get_json_doc( smx_net_t* h, bson_t* conf, const char* name,
        const char* impl, unsigned int id )
{
    const char* nets = "_nets";
    const char* config = "config";
    char search_str[1000];
    int rc;
    sprintf( search_str, "%s.%s.%s.%d.%s", nets, impl, name, id, config );
    rc = smx_net_get_json_doc_item( h, conf, search_str );
    if(rc < 0)
    {
        sprintf( search_str, "%s.%s.%s._default.%s", nets, impl, name, config );
        rc = smx_net_get_json_doc_item( h, conf, search_str );
    }
    if(rc < 0)
    {
        sprintf( search_str, "%s.%s._default.%s", nets, impl, config );
        rc = smx_net_get_json_doc_item( h, conf, search_str );
    }
    if(rc < 0)
    {
        sprintf( search_str, "%s._default.%s", nets, config );
        rc = smx_net_get_json_doc_item( h, conf, search_str );
    }

    return rc;
}

/*****************************************************************************/
int smx_net_get_json_doc_item( smx_net_t* h, bson_t* conf,
        const char* search_str )
{
    int rc;
    uint32_t len;
    const uint8_t* nets;
    const char* config;
    bson_iter_t iter;
    bson_iter_t child;
    bson_json_reader_t *reader;
    bson_error_t error;
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_DOCUMENT( &child ) )
    {
        bson_iter_document( &child, &len, &nets );
        bson_init_static( h->conf, nets, len );
        SMX_LOG_NET( h, notice, "load configuration '%s'", search_str );
        return 0;
    }
    else if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_UTF8( &child ) )
    {
        config = bson_iter_utf8( &child, NULL );
        reader = bson_json_reader_new_from_file( config, &error );
        if( reader == NULL )
        {
            SMX_LOG_NET( h, error, "failed to load net config file '%s': %s",
                    config, error.message );
            return -1;
        }

        rc = bson_json_reader_read( reader, h->conf, &error );
        if( rc < 0 )
        {
            SMX_LOG_NET( h, error,
                    "failed to parse net config file '%s': %s",
                    config, error.message );
            return -1;
        }

        bson_json_reader_destroy( reader );
        SMX_LOG_NET( h, notice,
                "load config file '%s' of configuration item '%s'",
                config, search_str );
        return 0;
    }
    SMX_LOG_NET( h, debug, "no configuration loaded from '%s'", search_str );
    return -1;
}

/*****************************************************************************/
bool smx_net_has_profiler( bson_t* conf, const char* name, const char* impl,
        unsigned int id )
{
    bson_iter_t iter;
    bson_iter_t child;
    char search_str[1000];
    const char* nets = "_nets";
    const char* profiler = "profiler";
    sprintf( search_str, "%s.%s.%s.%d.%s", nets, impl, name, id, profiler );
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_BOOL( &iter ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s.%s.%s._default.%s", nets, impl, name, profiler );
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_DOCUMENT( &iter ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s.%s._default.%s", nets, impl, profiler );
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_DOCUMENT( &iter ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s._default.%s", nets, profiler );
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) && BSON_ITER_HOLDS_DOCUMENT( &iter ) )
    {
        return bson_iter_bool( &child );
    }

    return false;
}

/*****************************************************************************/
void smx_net_init( smx_net_t* h, int indegree, int outdegree )
{
    int i;
    if( h == NULL || h->sig == NULL )
        return;

    h->sig->in.len = indegree;
    h->sig->in.ports = smx_malloc( sizeof( smx_channel_t* ) * indegree );
    for( i = 0; i < indegree; i++ )
        h->sig->in.ports[i] = NULL;

    h->sig->out.len = outdegree;
    h->sig->out.ports = smx_malloc( sizeof( smx_channel_t* ) * outdegree );
    for( i = 0; i < indegree; i++ )
        h->sig->in.ports[i] = NULL;
}

/*****************************************************************************/
int smx_net_run( pthread_t* ths, int idx, void* box_impl( void* arg ), void* h )
{
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

    pthread_attr_init( &sched_attr );
    if( net->priority > 0 )
    {
        min_fifo = sched_get_priority_min( SCHED_FIFO );
        max_fifo = sched_get_priority_max( SCHED_FIFO );
        fifo_param.sched_priority = min_fifo + net->priority;
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
void* smx_net_start_routine( smx_net_t* h, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void*, void* ) )
{
    double elapsed_wall;
    int init_res;
    int state = SMX_NET_CONTINUE;
    void* net_state = NULL;
    /* xmlChar* profiler = NULL; */

    if( h == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to start net: not initialised" );
        return NULL;
    }

    SMX_LOG_NET( h, notice, "init net" );

    if( h->has_profiler )
        SMX_LOG_NET( h, notice, "profiler enabled" );

    init_res = init( h, &net_state );
    pthread_barrier_wait( h->init_done );

    clock_gettime( CLOCK_MONOTONIC, &h->start_wall );
    if( init_res == 0)
    {
        SMX_LOG_NET( h, notice, "start net" );
        while( state == SMX_NET_CONTINUE )
        {
            SMX_LOG_NET( h, info, "start net loop" );
            h->count++;
            smx_profiler_log_net( h, SMX_PROFILER_ACTION_START );
            state = impl( h, net_state );
            state = smx_net_update_state( h, state );
        }
    }
    else
        SMX_LOG_NET( h, error, "initialisation of net failed" );
    clock_gettime( CLOCK_MONOTONIC, &h->end_wall );
    smx_net_terminate( h );
    SMX_LOG_NET( h, notice, "cleanup net" );
    cleanup( h, net_state );
    elapsed_wall = ( h->end_wall.tv_sec - h->start_wall.tv_sec );
    elapsed_wall += ( h->end_wall.tv_nsec - h->start_wall.tv_nsec) / 1000000000.0;
    SMX_LOG_NET( h, notice, "terminate net (loop count: %ld, loop rate: %d, wall time: %f)",
            h->count, (int)(h->count/elapsed_wall), elapsed_wall );
    /* xmlFree( profiler ); */
    return NULL;
}

/*****************************************************************************/
void smx_net_terminate( smx_net_t* h )
{
    if( h->sig == NULL || h->sig->in.ports == NULL || h->sig->out.ports == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "net channels not initialised" );
        return;
    }

    int i;
    int len_in = h->sig->in.count;
    int len_out = h->sig->out.count;
    smx_channel_t** chs_in = h->sig->in.ports;
    smx_channel_t** chs_out = h->sig->out.ports;

    SMX_LOG_NET( h, notice, "send termination notice to neighbours" );
    for( i=0; i < len_in; i++ ) {
        if( chs_in[i] == NULL ) continue;
        smx_channel_terminate_sink( chs_in[i] );
    }
    for( i=0; i < len_out; i++ ) {
        if( chs_out[i] == NULL ) continue;
        smx_channel_terminate_source( chs_out[i] );
        smx_collector_terminate( chs_out[i] );
    }
}

/*****************************************************************************/
int smx_net_update_state( smx_net_t* h, int state )
{
    if( h->sig == NULL || h->sig->in.ports == NULL || h->sig->out.ports == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "net channels not initialised" );
        return SMX_NET_END;
    }

    int i;
    int done_cnt_in = 0;
    int done_cnt_out = 0;
    int trigger_cnt = 0;
    int len_in = h->sig->in.count;
    int len_out = h->sig->out.count;
    smx_channel_t** chs_in = h->sig->in.ports;
    smx_channel_t** chs_out = h->sig->out.ports;

    // if state is forced by box implementation return forced state
    if( state != SMX_NET_RETURN ) return state;

    // check if a triggering input is still producing
    for( i=0; i<len_in; i++ )
    {
        if( chs_in[i] == NULL ) continue;
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
        if( chs_out[i] == NULL ) continue;
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
