/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Routing node box implementation for the runtime system library of Streamix
 */

#include <stdbool.h>
#include "box_smx_rn.h"
#include "smxutils.h"
#include "smxch.h"
#include "smxlog.h"
#include "smxnet.h"
#include "smxmsg.h"

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
    ch->collector = rn->attr;
}

/*****************************************************************************/
void smx_net_destroy_rn( smx_net_t* rn )
{
    if( rn == NULL )
        return;
    smx_collector_destroy( rn->attr );
}

/*****************************************************************************/
void smx_net_init_rn( smx_net_t* rn )
{
    if( rn == NULL )
    {
        SMX_LOG_MAIN( main, fatal,
                "unable to init routing node: not initialised" );
        return;
    }
    rn->attr = smx_collector_create();
}

/*****************************************************************************/
int smx_rn( void* h, void* state )
{
    int* last_idx = ( int* )state;
    int i;
    smx_net_t* net = h;

    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    int count_in = net->sig->in.len;
    int count_out = net->sig->out.len;
    smx_channel_t** chs_in = net->sig->in.ports;
    smx_channel_t** chs_out = net->sig->out.ports;
    smx_collector_t* collector = net->attr;

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

    *( int* )( *state ) = 0;
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* h, void* state )
{
    ( void )( h );
    if( state != NULL )
        free( state );
}

/*****************************************************************************/
void* start_routine_smx_rn( void* h )
{
    return smx_net_start_routine( h, smx_rn, smx_rn_init, smx_rn_cleanup );
}
