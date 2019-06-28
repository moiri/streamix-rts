/**
 * Profiler box implementation for the runtime system library of Streamix. This
 * box implementation serves as a collector of all profiler ports of all nets.
 * It serves as a routing node that connects to all net instances trsnmits the
 * profiler data to a profiler backend (which is not part of the RTS)
 *
 * @file    box_smx_profiler.c
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdbool.h>
#include "box_smx_profiler.h"
#include "smxutils.h"
#include "smxnet.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
void smx_net_destroy_profiler( smx_net_t* profiler )
{
    int i = 0;
    if( profiler == NULL || profiler->sig == NULL )
        return;

    smx_collector_destroy( profiler->attr );

    if( profiler->sig->in.ports != NULL )
    {
        for( i = 0; i<profiler->sig->in.len; i++ )
            smx_channel_destroy( profiler->sig->in.ports[i] );
    }
}

/*****************************************************************************/
void smx_net_finalize_profiler( smx_net_t* profiler, smx_net_t** nets,
        int net_cnt )
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

    free( profiler->sig->in.ports );
    profiler->sig->in.len = net_cnt - 2;
    profiler->sig->in.ports =
        smx_malloc( sizeof( struct smx_channel_s ) * profiler->sig->in.len );
    if( profiler->sig->in.ports == NULL )
        return;

    SMX_LOG_MAIN( ch, info, "connecting profiler channels" );
    for( i = 0; i < net_cnt; i++ )
    {
        is_profiler = 0;
        if( profiler == nets[i] ) continue;
        for( j = 0; j < nets[i]->sig->in.len; j++ )
            if( profiler->sig->out.ports[0] == nets[i]->sig->in.ports[j] )
            {
                is_profiler = 1;
                break;
            }
        if( is_profiler ) continue;

        id = profiler->sig->in.count;
        sprintf( cat_name, "ch_%s_i%d", name, id );
        profiler->sig->in.ports[id] = smx_channel_create(
                &profiler->sig->in.count, net_cnt, SMX_FIFO, id, name,
                cat_name );
        nets[i]->profiler = profiler->sig->in.ports[id];
        if( nets[i]->profiler != NULL )
            nets[i]->profiler->collector = profiler->attr;

    }

    if( profiler->sig->in.count != profiler->sig->in.len )
        SMX_LOG_MAIN( ch, warn,
                "profiler input port missmatch: expexted %d, got %d",
                profiler->sig->in.len, profiler->sig->in.count );
}

/*****************************************************************************/
void smx_net_init_profiler( smx_net_t* profiler )
{
    profiler->attr = smx_collector_create();
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
    smx_net_t* net = h;

    if( net == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to run smx_rn: not initialised" );
        return SMX_NET_END;
    }

    msg = smx_net_profiler_read( h, net->attr, net->sig->in.ports,
            net->sig->in.len );
    if( msg != NULL )
    {
        if( smx_channel_ready_to_write(net->sig->out.ports[0]) == 0 )
            SMX_LOG_NET( h, warn, "profiler queue is full" );
        smx_channel_write( h, net->sig->out.ports[0], msg );
    }

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

/*****************************************************************************/
void* start_routine_smx_profiler( void* h )
{
    return smx_net_start_routine( h, smx_profiler, smx_profiler_init,
            smx_profiler_cleanup );
}
