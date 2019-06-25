/**
 * utility functions for the runtime system library of Streamix
 *
 * @file    smxutils.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>

#ifndef SMXUTILS_H
#define SMXUTILS_H

#define SMX_MODE_in     "->"
#define SMX_MODE_out    "<-"

#define SMX_NO_SIG( h )\
    ( SMX_SIG( h ) == NULL )

#define SMX_NO_SIG_PORT( h, box_name, port_name, mode )\
    SMX_SIG_PORT_LEN_NC( h, mode ) <= SMX_SIG_PORT_IDX( box_name, port_name, mode )

#define SMX_SIG( h )\
    ( ( h == NULL ) ? NULL : SMX_SIG_NC( h ) )

#define SMX_SIG_NC( h )\
    ( ( smx_net_sig_t* )( ( smx_net_t* )h )->sig )

#define SMX_SIG_CAT( h )\
    ( h == NULL ) ? zlog_get_category( "undef" ) : SMX_SIG_CAT_NC( h )

#define SMX_SIG_CAT_NC( h )\
    ( ( smx_net_t* )h )->cat

#define SMX_SIG_PORT( h, box_name, port_name, mode )\
    ( ( SMX_NO_SIG( h ) || SMX_NO_SIG_PORT( h, box_name, port_name, mode ) )\
        ? NULL : SMX_SIG_PORT_NC( h, box_name, port_name, mode ) )

#define SMX_SIG_PORT_ARR( h, box_name, port_name, mode )\
    ( SMX_NO_SIG( h ) ? NULL : SMX_SIG_PORT_ARR_NC( h, mode ) )

#define SMX_SIG_PORT_ARR_NC( h, mode )\
    SMX_SIG_PORTS_NC( h, mode )[SMX_SIG_PORT_COUNT_NC( h, mode )]

#define SMX_SIG_PORT_ARR_PTR( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_ARR_NC( h, mode ) )

#define SMX_SIG_PORT_LEN( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_LEN_NC( h, mode ) )

#define SMX_SIG_PORT_LEN_NC( h, mode )\
    SMX_SIG_NC( h )->mode.len

#define SMX_SIG_PORT_COUNT( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_COUNT_NC( h, mode ) )

#define SMX_SIG_PORT_COUNT_NC( h, mode )\
    SMX_SIG_NC( h )->mode.count

#define SMX_SIG_PORT_IDX( box_name, port_name, mode )\
    SMX_PORT_IDX_ ## box_name ## _ ## mode ## _ ## port_name

#define SMX_SIG_PORT_NC( h, box_name, port_name, mode )\
    SMX_SIG_PORTS_NC( h, mode )[SMX_SIG_PORT_IDX( box_name, port_name, mode )]

#define SMX_SIG_PORT_PTR( h, box_name, port_name, mode )\
    ( ( SMX_NO_SIG( h ) || SMX_NO_SIG_PORT( h, box_name, port_name, mode ) )\
        ? NULL : &SMX_SIG_PORT_NC( h, box_name, port_name, mode ) )

#define SMX_SIG_PORTS( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : SMX_SIG_PORTS_NC( h, mode ) )

#define SMX_SIG_PORTS_NC( h, mode )\
    ( ( smx_channel_t** )SMX_SIG_NC( h )->mode.ports )

#define STRINGIFY(x) #x

/**
 * Allocate space with malloc and log an error if malloc fails
 *
 * @param size  the memory size to allocate
 * @return      a void pointer to the allocated memory
 */
void* smx_malloc( size_t size );

#endif
