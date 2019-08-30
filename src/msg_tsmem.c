/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * This file holds the message manipulation functions for a time-stamped data
 * messages.
 */

#include <string.h>
#include "msg_tsmem.h"
#include "smxmsg.h"
#include "smxutils.h"

/*****************************************************************************/
void* smx_data_tsmem_copy( void* data, size_t size )
{
    smx_data_tsmem_t* mg = data;
    smx_data_tsmem_t* data_copy = smx_malloc( size );
    data_copy->ts = mg->ts;
    data_copy->size = mg->size;
    data_copy->data = smx_malloc( mg->size + 1 );
    memcpy( data_copy->data, mg->data, mg->size );
    return data_copy;
}

/*****************************************************************************/
void smx_data_tsmem_destroy( void* data )
{
    smx_data_tsmem_t* mg = data;
    if( mg->data != NULL )
        free( mg->data );
    free( data );
}

/*****************************************************************************/
void* smx_data_tsmem_unpack( void* data )
{
    return data;
}

/*****************************************************************************/
smx_msg_t* smx_msg_tsmem_create( smx_net_t* net, struct timespec ts, void* data,
        size_t size )
{
    smx_data_tsmem_t* msg_data = smx_malloc( sizeof( struct smx_data_tsmem_s ) );
    msg_data->ts = ts;
    msg_data->data = data;
    msg_data->size = size;
    return smx_msg_create( net, msg_data, sizeof( struct smx_data_tsmem_s ),
            smx_data_tsmem_copy, smx_data_tsmem_destroy, smx_data_tsmem_unpack );
}

/*****************************************************************************/
smx_msg_t* smx_msg_tsmem_create_ts( smx_net_t* net, void* data, size_t size )
{
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts );
    return smx_msg_tsmem_create( net, ts, data, size );
}

/*****************************************************************************/
smx_msg_t* smx_msg_tsstr_create( smx_net_t* net, struct timespec ts, char* data )
{
    return smx_msg_tsmem_create( net, ts, data, strlen( data ) + 1 );
}

/*****************************************************************************/
smx_msg_t* smx_msg_tsstr_create_ts( smx_net_t* net, char* data )
{
    return smx_msg_tsmem_create_ts( net, data, strlen( data ) + 1 );
}
