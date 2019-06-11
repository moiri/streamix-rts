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

/*****************************************************************************/
void smx_connect_profiler( smx_net_t* profiler, smx_net_t** nets, int net_cnt )
{

    int i;
    if( profiler == NULL )
    {
        SMX_LOG_MAIN( main, fatal,
                "unable to connect profiler: not initialised" );
        return;
    }
    net_smx_profiler_t* sig = profiler->sig;

    for( i = 0; i < net_cnt; i++ )
    {
        if( profiler == nets[i] ) continue;
        smx_channel_create( sig->in.ports, &sig->in.port_cnt, 1, SMX_FIFO, -1,
                "smx_profiler", "ch_smx_profiler" );
        nets[i]->profiler = sig->in.ports[sig->in.port_cnt - 1];
        nets[i]->profiler->collector = sig->in.collector;
    }
}

/*****************************************************************************/
void smx_net_profiler_destroy( net_smx_profiler_t* profiler )
{
    int i = 0;
    if( profiler == NULL )
        return;

    pthread_mutex_destroy( &profiler->in.collector->col_mutex );
    pthread_cond_destroy( &profiler->in.collector->col_cv );
    free( profiler->in.collector );

    if( profiler->in.ports != NULL )
    {
        for( i = 0; i<profiler->in.port_cnt; i++ )
            smx_channel_destroy( profiler->in.ports[i] );
        free( profiler->in.ports );
    }
}

/*****************************************************************************/
void smx_net_profiler_init( net_smx_profiler_t* profiler )
{
    profiler->in.collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( profiler->in.collector == NULL )
        return;

    pthread_mutex_init( &profiler->in.collector->col_mutex, NULL );
    pthread_cond_init( &profiler->in.collector->col_cv, NULL );
    profiler->in.collector->count = 0;
    profiler->in.collector->state = SMX_CHANNEL_UNINITIALISED;
    profiler->in.port_cnt = 0;
    profiler->in.ports = NULL;
}

/*****************************************************************************/
int smx_profiler( void* h, void* state )
{
    int* last_idx = ( int* )state;
    smx_msg_t* msg;
    smx_net_t* net = h;
    net_smx_profiler_t* profiler = SMX_SIG( h );

    if( profiler == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to run smx_rn: not initialised" );
        return SMX_NET_END;
    }

    msg = smx_net_collector_read( h, profiler->in.collector, net->in.ports,
            net->in.count, last_idx );
    smx_channel_write( h, profiler->out.profiler, msg );

    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_profiler_init( void* h, void** state )
{
    (void)(h);
    *state = smx_malloc( sizeof( int ) );
    if( *state == NULL )
        return -1;

    *( int* )( *state ) = -1;
    return 0;
}

/*****************************************************************************/
void smx_prfiler_cleanup( void* state )
{
    if( state != NULL )
        free( state );
}
