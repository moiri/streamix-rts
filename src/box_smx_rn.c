/**
 * Routing node box implementation for the runtime system library of Streamix
 *
 * @file    box_smx_rn.h
 * @author  Simon Maurer
 */

#include <stdbool.h>
#include "box_smx_rn.h"
#include "smxutils.h"
#include "smxnet.h"
#include "smxlog.h"

/*****************************************************************************/
void smx_connect_rn( smx_channel_t* ch, smx_net_t* rn )
{
    const char* elem;
    if( ch == NULL || rn == NULL )
    {
        elem = ( ch == NULL ) ? "channel" : "rn";
        SMX_LOG_MAIN( main, fatal,
                "unable to connect routing node: not initialised %s", elem );
        return;
    }
    ch->collector = ( ( net_smx_rn_t* )SMX_SIG( rn ) )->in.collector;
}

/*****************************************************************************/
void smx_net_rn_destroy( net_smx_rn_t* cp )
{
    if( cp == NULL )
        return;

    pthread_mutex_destroy( &cp->in.collector->col_mutex );
    pthread_cond_destroy( &cp->in.collector->col_cv );
    free( cp->in.collector );
}

/*****************************************************************************/
void smx_net_rn_init( net_smx_rn_t* cp )
{
    pthread_mutexattr_t mutexattr_prioinherit;
    cp->in.collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( cp->in.collector == NULL )
        return;

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &cp->in.collector->col_mutex, &mutexattr_prioinherit );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_PENDING;
}

/*****************************************************************************/
int smx_rn( void* h, void* state )
{
    int* last_idx = ( int* )state;
    int i;
    net_smx_rn_t* rn = SMX_SIG( h );
    if( rn == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "unable to run smx_rn: not initialised" );
        return SMX_NET_END;
    }

    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    smx_net_t* net = h;
    int count_in = net->in.count;
    int count_out = net->out.count;
    smx_channel_t** chs_in = net->in.ports;
    smx_channel_t** chs_out = net->out.ports;
    smx_collector_t* collector = rn->in.collector;

    msg = smx_net_collector_read( h, collector, chs_in, count_in, last_idx );
    if(msg != NULL)
    {
        for( i=0; i<count_out; i++ ) {
            if( i == count_out - 1 )
                smx_channel_write( h, chs_out[i], msg );
            else
            {
                msg_copy = smx_msg_copy( h, msg );
                smx_channel_write( h, chs_out[i], msg_copy );
            }
        }
    }
    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_rn_init( void* h, void** state )
{
    ( void )( h );
    *state = smx_malloc( sizeof( int ) );
    if( *state == NULL )
        return -1;

    *( int* )( *state ) = -1;
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* h, void* state )
{
    ( void )( h );
    if( state != NULL )
        free( state );
}
