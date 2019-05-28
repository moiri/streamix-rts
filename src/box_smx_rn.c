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
smx_msg_t* smx_collector_read( void* h, smx_collector_t* collector,
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
        msg = smx_channel_read( ch );
    }
    return msg;
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
    cp->in.collector = smx_malloc( sizeof( struct smx_collector_s ) );
    if( cp->in.collector == NULL )
        return;

    pthread_mutex_init( &cp->in.collector->col_mutex, NULL );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_UNINITIALISED;
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
    int count_in = rn->in.count;
    int count_out = rn->out.count;
    smx_channel_t** chs_in = rn->in.ports;
    smx_channel_t** chs_out = rn->out.ports;
    smx_collector_t* collector = rn->in.collector;

    msg = smx_collector_read( h, collector, chs_in, count_in, last_idx );
    if(msg != NULL)
    {
        for( i=0; i<count_out; i++ ) {
            msg_copy = smx_msg_copy( msg );
            smx_channel_write( chs_out[i], msg_copy );
        }
        smx_msg_destroy( msg, true );
    }
    return SMX_NET_RETURN;
}

/*****************************************************************************/
int smx_rn_init( void* h, void** state )
{
    (void)(h);
    *state = smx_malloc( sizeof( int ) );
    if( *state == NULL )
        return -1;

    *( int* )( *state ) = -1;
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* state )
{
    if( state != NULL )
        free( state );
}
