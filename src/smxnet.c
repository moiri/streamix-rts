/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Net definitions for the runtime system library of Streamix
 */
#define _GNU_SOURCE

#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "smxch.h"
#include "smxconfig.h"
#include "smxnet.h"
#include "smxmsg.h"
#include "smxprofiler.h"
#include "smxutils.h"

/*****************************************************************************/
smx_msg_t* smx_net_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx )
{
    int i, ch_count, ready_cnt;
    smx_msg_t* msg = NULL;
    smx_channel_t* ch = NULL;
    int rc = 0;

    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING && rc == 0 )
    {
        smx_profiler_log_ch( h, in[0], NULL,
                SMX_PROFILER_ACTION_CH_READ_COLLECTOR_BLOCK,
                collector->count );
        SMX_LOG_NET( h, debug, "waiting for message on collector" );
        rc = pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    }
    pthread_mutex_unlock( &collector->col_mutex );

    if( collector->count > 0 )
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
        msg = smx_channel_read( h, ch );
    }
    else if( collector->state != SMX_CHANNEL_END )
    {
        SMX_LOG_NET( h, warn, "collector is ready but count is 0, aborting" );
        return NULL;
    }
    return msg;
}

/*****************************************************************************/
smx_net_t* smx_net_create( unsigned int id, const char* name,
        const char* impl, const char* cat_name, smx_rts_t* rts, int prio )
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
    net->last_count_wall.tv_sec = 0;
    net->last_count_wall.tv_nsec = 0;
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

    net->rts = rts;
    net->state = NULL;
    net->shared_state = NULL;
    net->id = id;
    net->priority = prio;
    net->cat = zlog_get_category( cat_name );
    net->name = ( name == NULL ) ? NULL : strdup( name );
    net->impl = ( impl == NULL ) ? NULL : strdup( impl );
    net->attr = NULL;
    net->conf = NULL;
    net->static_conf = NULL;
    net->dyn_conf = NULL;
    smx_net_get_json_doc( net, rts->conf, name, impl, id );
    net->has_profiler = smx_net_get_boolean_prop( rts->conf, name, impl, id,
            "profiler" );
    net->has_type_filter = smx_net_get_boolean_prop( rts->conf, name, impl, id,
            "type_filter" );
    net->is_disabled = smx_net_get_boolean_prop( rts->conf, name, impl, id,
            "is_disabled" );
    net->conf_port_name = smx_net_get_string_prop( rts->conf, name, impl, id,
            "dyn_conf_port" );
    net->conf_port_timeout = smx_net_get_int_prop( rts->conf, name, impl, id,
            "dyn_conf_timeout" );
    net->expected_rate = smx_net_get_int_prop( rts->conf, name, impl, id,
            "expected_rate" );
    net->shared_state_key = smx_net_get_string_prop( rts->conf, name, impl, id,
            "shared_state_key" );

    rts->net_cnt++;
    SMX_LOG_MAIN( net, info, "create net instance %s(%d)", name, id );
    return net;
}

/*****************************************************************************/
void smx_net_destroy( smx_net_t* h )
{
    if( h != NULL )
    {
        if( h->name != NULL )
            free( h->name );
        if( h->impl != NULL )
            free( h->impl );
        if( h->static_conf != NULL )
        {
            bson_destroy( h->static_conf );
        }
        if( h->dyn_conf != NULL )
        {
            bson_destroy( h->dyn_conf );
        }
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
bool smx_net_get_boolean_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop )
{
    bson_iter_t iter;
    bson_iter_t child;
    char search_str[1000];
    const char* nets = "_nets";
    sprintf( search_str, "%s.%s.%s.%d.%s", nets, impl, name, id, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_BOOL( &child ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s.%s.%s._default.%s", nets, impl, name, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_BOOL( &child ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s.%s._default.%s", nets, impl, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_BOOL( &child ) )
    {
        return bson_iter_bool( &child );
    }
    sprintf( search_str, "%s._default.%s", nets, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_BOOL( &child ) )
    {
        return bson_iter_bool( &child );
    }

    return false;
}

/*****************************************************************************/
int smx_net_get_int_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop )
{
    bson_iter_t iter;
    bson_iter_t child;
    char search_str[1000];
    const char* nets = "_nets";
    sprintf( search_str, "%s.%s.%s.%d.%s", nets, impl, name, id, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_INT32( &child ) )
    {
        return bson_iter_int32( &child );
    }
    sprintf( search_str, "%s.%s.%s._default.%s", nets, impl, name, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_INT32( &child ) )
    {
        return bson_iter_int32( &child );
    }
    sprintf( search_str, "%s.%s._default.%s", nets, impl, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_INT32( &child ) )
    {
        return bson_iter_int32( &child );
    }
    sprintf( search_str, "%s._default.%s", nets, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_INT32( &child ) )
    {
        return bson_iter_int32( &child );
    }

    return false;
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
        h->static_conf = bson_new_from_data( nets, len );
        h->conf = h->static_conf;
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

        h->static_conf = bson_new();
        rc = bson_json_reader_read( reader, h->static_conf, &error );
        h->conf = h->static_conf;
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
const char* smx_net_get_string_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop )
{
    bson_iter_t iter;
    bson_iter_t child;
    char search_str[1000];
    const char* nets = "_nets";
    sprintf( search_str, "%s.%s.%s.%d.%s", nets, impl, name, id, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_UTF8( &child ) )
    {
        return bson_iter_utf8( &child, NULL );
    }
    sprintf( search_str, "%s.%s.%s._default.%s", nets, impl, name, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_UTF8( &child ) )
    {
        return bson_iter_utf8( &child, NULL );
    }
    sprintf( search_str, "%s.%s._default.%s", nets, impl, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_UTF8( &child ) )
    {
        return bson_iter_utf8( &child, NULL );
    }
    sprintf( search_str, "%s._default.%s", nets, prop );
    if( bson_iter_init( &iter, conf )
            && bson_iter_find_descendant( &iter, search_str, &child )
            && BSON_ITER_HOLDS_UTF8( &child ) )
    {
        return bson_iter_utf8( &child, NULL );
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
    h->sig->in.ports = smx_malloc( sizeof( smx_channel_t* ) * h->sig->in.len );
    for( i = 0; i < h->sig->in.len; i++ )
        h->sig->in.ports[i] = NULL;

    h->sig->out.len = outdegree;
    h->sig->out.ports = smx_malloc( sizeof( smx_channel_t* ) * h->sig->out.len );
    for( i = 0; i < h->sig->out.len; i++ )
        h->sig->out.ports[i] = NULL;
}

/*****************************************************************************/
void smx_net_report_rate_warning( smx_net_t* h )
{
    struct timespec now;
    int sec;
    int nsec;
    double rate;
    const double delta = 0.2;

    clock_gettime( CLOCK_MONOTONIC, &now );
    sec = now.tv_sec - h->last_count_wall.tv_sec;
    nsec = now.tv_nsec - h->last_count_wall.tv_nsec;
    rate = sec + ( nsec  / 1000000000.0 );
    if( rate > ( 1 + delta ) || rate < ( 1 - delta ) )
    {
        SMX_LOG_NET( h, warn,
                "net rate at %.0f%% of the expected rate (%d)",
                1 / rate * 100, h->expected_rate );
    }
    h->last_count_wall.tv_nsec = now.tv_nsec;
    h->last_count_wall.tv_sec = now.tv_sec;
}

/*****************************************************************************/
int smx_net_run( pthread_t* ths, int idx, void* box_impl( void* arg ), void* h )
{
    smx_net_t* net = h;
    pthread_attr_t sched_attr;
    struct sched_param fifo_param;
    pthread_t thread;
    int min_fifo, max_fifo;
    char id_str[16];

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
    sprintf( id_str, "smx_net_%d", net->id );
    pthread_setname_np( thread, id_str );
    ths[idx] = thread;
    return 0;
}

/*****************************************************************************/
void* smx_net_start_routine( smx_net_t* h, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void*, void* ) )
{
    return smx_net_start_routine_with_shared_state( h, impl, init,
            cleanup, NULL, NULL, NULL );
}

/*****************************************************************************/
void* smx_net_start_routine_with_shared_state( smx_net_t* h,
        int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void*, void* ), int init_shared( void*, void** ),
        void cleanup_shared( void* ), const char* shared_state_key )
{
    double elapsed_wall;
    int state = SMX_NET_CONTINUE;
    int rc;
    int i;
    smx_channel_t* conf_port;
    smx_channel_err_t c_err;
    smx_msg_t* msg;
    bson_error_t b_err;

    if( h == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to start net: not initialised" );
        return NULL;
    }

    if( h->is_disabled )
    {
        SMX_LOG_NET( h, notice, "net was disabled through configuration" );
        pthread_barrier_wait( &h->rts->pre_init_done );
        pthread_barrier_wait( &h->rts->init_done );
        goto smx_terminate_net;
    }

    if( h->has_profiler )
        SMX_LOG_NET( h, notice, "profiler enabled" );

    if( h->shared_state_key == NULL )
    {
        h->shared_state_key = shared_state_key;
    }
    if( init_shared != NULL && cleanup_shared != NULL
            && h->shared_state_key != NULL )
    {
        SMX_LOG_NET( h, notice, "pre init net" );
        if( h->static_conf == NULL )
        {
            SMX_LOG_NET( h, error, "no static net configuration available" );
            pthread_barrier_wait( &h->rts->pre_init_done );
            pthread_barrier_wait( &h->rts->init_done );
            goto smx_terminate_net;
        }

        pthread_mutex_lock( &h->rts->net_mutex );
        for( i = 0; i < h->rts->shared_state_cnt; i++ )
        {
            if( strcmp( h->rts->shared_state[i]->key, h->shared_state_key ) == 0 )
            {
                h->shared_state = h->rts->shared_state[i]->state;
                SMX_LOG_NET( h, notice, "using already allocated shared state"
                        " with key '%s'", h->shared_state_key );
                break;
            }
        }
        if( h->shared_state == NULL )
        {
            h->rts->shared_state[h->rts->shared_state_cnt] = smx_malloc(
                    sizeof( struct smx_rts_shared_state_s ) );
            h->rts->shared_state[h->rts->shared_state_cnt]->key =
                h->shared_state_key;
            h->rts->shared_state[h->rts->shared_state_cnt]->cleanup =
                cleanup_shared;
            h->rts->shared_state[h->rts->shared_state_cnt]->state = NULL;
            rc = init_shared( h, &h->shared_state );
            h->rts->shared_state[h->rts->shared_state_cnt]->state =
                h->shared_state;
            SMX_LOG_NET( h, notice, "shared state allocated with key '%s' at"
                    " position '%d'", h->shared_state_key,
                    h->rts->shared_state_cnt );
            h->rts->shared_state_cnt++;
            if( rc < 0 )
            {
                SMX_LOG_NET( h, error, "pre initialisation of net failed" );
                pthread_barrier_wait( &h->rts->pre_init_done );
                pthread_barrier_wait( &h->rts->init_done );
                goto smx_terminate_net;
            }
        }
        pthread_mutex_unlock( &h->rts->net_mutex );
        SMX_LOG_NET( h, notice, "pre init done" );
    }

    pthread_barrier_wait( &h->rts->pre_init_done );

    if( h->conf_port_name != NULL )
    {
        conf_port = smx_get_channel_by_name( h->sig->in.ports,
                h->sig->in.count, h->conf_port_name );
        if( conf_port == NULL )
        {
            SMX_LOG_NET( h, error,
                    "dynamic configuration port '%s' does not exist",
                    h->conf_port_name );
            pthread_barrier_wait( &h->rts->init_done );
            goto smx_terminate_net;
        }
        SMX_LOG_NET( h, notice, "awaiting dynamic configuration..." );
        smx_channel_set_filter( h, conf_port, 1, "json" );
        smx_set_read_timeout( conf_port, h->conf_port_timeout / 1000,
                ( h->conf_port_timeout % 1000 ) * 1000000 );
        msg = smx_channel_read( h, conf_port );
        if( msg == NULL )
        {
            c_err = smx_get_read_error( conf_port );
            if( c_err == SMX_CHANNEL_ERR_TIMEOUT )
            {
                SMX_LOG_NET( h, warn, "read operation dynamic configuration"
                        " port '%s' timed out", h->conf_port_name );
            }
            SMX_LOG_NET( h, error, "failed to read dynamic configuration" );
            pthread_barrier_wait( &h->rts->init_done );
            goto smx_terminate_net;
        }
        else
        {
            h->dyn_conf = bson_new_from_json( msg->data, msg->size, &b_err );
            if( h->dyn_conf == NULL )
            {
                SMX_LOG( h, error, "unable to parse dynamic configuration" );
                pthread_barrier_wait( &h->rts->init_done );
                goto smx_terminate_net;
            }
            SMX_LOG( h, debug, "received dynamic configuration: %s",
                    ( char* )msg->data );
            h->conf = h->dyn_conf;
            SMX_LOG( h, notice, "dynamic configuration received and"
                    " successfully parsed" );
        }
        SMX_MSG_DESTROY( h, msg );
    }

    if( h->conf == NULL )
    {
        SMX_LOG_NET( h, error, "no net configuration available" );
        pthread_barrier_wait( &h->rts->init_done );
        goto smx_terminate_net;
    }

    SMX_LOG_NET( h, notice, "init net" );
    rc = init( h, &h->state );
    if( rc < 0 )
    {
        SMX_LOG_NET( h, error, "initialisation of net failed" );
        pthread_barrier_wait( &h->rts->init_done );
        goto smx_terminate_net;
    }

    SMX_LOG_NET( h, notice, "init done" );
    pthread_barrier_wait( &h->rts->init_done );

    clock_gettime( CLOCK_MONOTONIC, &h->start_wall );
    h->last_count_wall.tv_nsec = h->start_wall.tv_nsec;
    h->last_count_wall.tv_sec = h->start_wall.tv_sec;
    SMX_LOG_NET( h, notice, "start net" );
    while( state == SMX_NET_CONTINUE )
    {
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_NET_START );
        h->count++;
        SMX_LOG_NET( h, info, "start net loop %ld", h->count );
        if( ( h->expected_rate > 0 )
                && ( ( h->count % h->expected_rate ) == 0 ) )
        {
            smx_net_report_rate_warning( h );
        }
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_NET_START_IMPL );
        state = impl( h, h->state );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_NET_END_IMPL );
        state = smx_net_update_state( h, state );
        smx_profiler_log_net( h, SMX_PROFILER_ACTION_NET_END );
    }

smx_terminate_net:
    clock_gettime( CLOCK_MONOTONIC, &h->end_wall );
    smx_net_terminate( h );
    SMX_LOG_NET( h, notice, "cleanup net" );
    cleanup( h, h->state );
    elapsed_wall = ( h->end_wall.tv_sec - h->start_wall.tv_sec );
    elapsed_wall += ( h->end_wall.tv_nsec - h->start_wall.tv_nsec) / 1000000000.0;
    SMX_LOG_NET( h, notice, "terminate net (loop count: %ld, loop rate: %d, wall time: %f)",
            h->count, (int)(h->count/elapsed_wall), elapsed_wall );
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

/*****************************************************************************/
void smx_net_wait_end( pthread_t th )
{
    pthread_join( th, NULL );
}
