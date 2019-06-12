/**
 * Profiler box implementation for the runtime system library of Streamix. This
 * box implementation serves as a collector of all profiler ports of all nets.
 * It serves as a routing node that connects to all net instances trsnmits the
 * profiler data to a profiler backend (which is not part of the RTS)
 *
 * @file    box_smx_profiler.c
 * @author  Simon Maurer
 */

#include <stdbool.h>
#include "box_smx_profiler.h"
#include "smxutils.h"
#include "smxnet.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
void smx_connect_profiler( smx_net_t* profiler, smx_net_t** nets, int net_cnt )
{

    int i, j, id, is_profiler;
    char cat_name[15];
    const char* name = "profiler";
    if( profiler == NULL )
    {
        SMX_LOG_MAIN( main, fatal,
                "unable to connect profiler: not initialised" );
        return;
    }
    net_smx_profiler_t* sig = profiler->sig;

    profiler->in.ports = smx_malloc( sizeof( struct smx_channel_s ) * net_cnt - 2 );
    if( profiler->in.ports == NULL )
        return;

    SMX_LOG_MAIN( ch, info, "connecting profiler channels" );
    for( i = 0; i < net_cnt; i++ )
    {
        is_profiler = 0;
        if( profiler == nets[i] ) continue;
        for( j = 0; j < nets[i]->in.count; j++ )
            if( sig->out.port_profiler == nets[i]->in.ports[j] )
            {
                is_profiler = 1;
                break;
            }
        if( is_profiler ) continue;

        id = profiler->in.count;
        sprintf( cat_name, "ch_%s_i%d", name, id );
        smx_channel_create( profiler->in.ports, &profiler->in.count, 1,
                SMX_FIFO, id, name, cat_name );
        nets[i]->profiler = profiler->in.ports[id];
        nets[i]->profiler->collector = sig->in.collector;
    }
}

/*****************************************************************************/
void smx_net_profiler_destroy( smx_net_t* profiler )
{
    int i = 0;
    if( profiler == NULL )
        return;

    net_smx_profiler_t* sig = profiler->sig;

    pthread_mutex_destroy( &sig->in.collector->col_mutex );
    pthread_cond_destroy( &sig->in.collector->col_cv );
    free( sig->in.collector );

    if( profiler->in.ports != NULL )
    {
        for( i = 0; i<profiler->in.count; i++ )
            smx_channel_destroy( profiler->in.ports[i] );
    }
}

/*****************************************************************************/
void smx_net_profiler_init( smx_net_t* profiler )
{
    net_smx_profiler_t* sig = profiler->sig;
    sig->in.collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( sig->in.collector == NULL )
        return;

    pthread_mutex_init( &sig->in.collector->col_mutex, NULL );
    pthread_cond_init( &sig->in.collector->col_cv, NULL );
    sig->in.collector->count = 0;
    sig->in.collector->state = SMX_CHANNEL_PENDING;
    profiler->in.count = 0;
    profiler->in.ports = NULL;
}

/*****************************************************************************/
void smx_net_profiler_peak_ts( smx_channel_t* ch, struct timespec* ts )
{
    smx_mongo_msg_t* mg_msg = NULL;
    mg_msg = ch->fifo->head->msg->data;
    (*ts).tv_sec = mg_msg->ts.tv_sec;
    (*ts).tv_nsec = mg_msg->ts.tv_nsec;
}

/*****************************************************************************/
smx_msg_t* smx_net_profiler_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in )
{
    int cur_count, i;
    smx_msg_t* msg = NULL;
    smx_channel_t* ch = NULL;
    struct timespec ts;
    struct timespec ts_oldest;
    ts_oldest.tv_sec = 0;
    ts_oldest.tv_nsec = 0;

    cur_count = smx_net_collector_check_avaliable( h, collector );

    if( cur_count > 0 )
    {
        for( i = 0; i < count_in; i++ )
        {
            if( smx_channel_ready_to_read( in[i] ) > 0 )
            {
                smx_net_profiler_peak_ts( in[i], &ts );
                if( ( ts_oldest.tv_sec == 0 && ts_oldest.tv_nsec == 0 )
                        || ( ts.tv_sec < ts_oldest.tv_sec
                            || ( ts.tv_sec == ts_oldest.tv_sec
                                && ts.tv_nsec < ts_oldest.tv_nsec ) ) )
                {
                    ts_oldest.tv_sec = ts.tv_sec;
                    ts_oldest.tv_nsec = ts.tv_nsec;
                    ch = in[i];
                }
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
        msg = smx_channel_read( h, ch );
    }
    return msg;
}

/*****************************************************************************/
int smx_profiler( void* h, void* state )
{
    ( void )( state );
    smx_msg_t* msg;
    net_smx_profiler_t* profiler = SMX_SIG( h );
    smx_net_t* net = h;

    if( profiler == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to run smx_rn: not initialised" );
        return SMX_NET_END;
    }

    msg = smx_net_profiler_read( h, profiler->in.collector, net->in.ports,
            net->in.count );
    if( msg != NULL )
        smx_channel_write( h, profiler->out.port_profiler, msg );

    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_profiler_init( void* h, void** state )
{
    ( void )( h );
    ( void )( state );
    return 0;
}

/*****************************************************************************/
void smx_profiler_cleanup( void* h, void* state )
{
    ( void )( h );
    ( void )( state );
}
