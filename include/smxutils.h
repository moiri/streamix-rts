/**
 * @file    smxutils.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Utility functions for the runtime system library of Streamix
 */

#include <stdlib.h>

#ifndef SMXUTILS_H
#define SMXUTILS_H

#define SMX_MAX( a , b ) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/**
 * ASCII definition of an input port
 */
#define SMX_MODE_in     "->"
/**
 * ASCII definition of an output port
 */
#define SMX_MODE_out    "<-"

/**
 * Macro to check whether a net signature is defined.
 */
#define SMX_NO_SIG( h )\
    ( SMX_SIG( h ) == NULL )

/**
 * Macro to check whether a port is defined.
 */
#define SMX_NO_SIG_PORT( h, box_name, port_name, mode )\
    SMX_SIG_PORT_LEN_NC( h, mode ) <= SMX_SIG_PORT_IDX( box_name, port_name, mode )

/**
 * Macro to get a net signature.
 */
#define SMX_SIG( h )\
    ( ( h == NULL ) ? NULL : SMX_SIG_NC( h ) )

/**
 * Macro to access a net signature without sanity checks.
 */
#define SMX_SIG_NC( h )\
    ( ( smx_net_sig_t* )( ( smx_net_t* )h )->sig )

/**
 * Macro to get the net logger category.
 */
#define SMX_SIG_CAT( h )\
    ( h == NULL ) ? zlog_get_category( "undef" ) : SMX_SIG_CAT_NC( h )

/**
 * Macro to get the net logger category without sanity checks.
 */
#define SMX_SIG_CAT_NC( h )\
    ( ( smx_net_t* )h )->cat

/**
 * Macro to get the port signature of a net.
 */
#define SMX_SIG_PORT( h, box_name, port_name, mode )\
    ( ( SMX_NO_SIG( h ) || SMX_NO_SIG_PORT( h, box_name, port_name, mode ) )\
        ? NULL : SMX_SIG_PORT_NC( h, box_name, port_name, mode ) )

/**
 * Macro to get the port array of a net.
 */
#define SMX_SIG_PORT_ARR( h, box_name, port_name, mode )\
    ( SMX_NO_SIG( h ) ? NULL : SMX_SIG_PORT_ARR_NC( h, mode ) )

/**
 * Macro to get the port array of a net without sanity checks.
 */
#define SMX_SIG_PORT_ARR_NC( h, mode )\
    SMX_SIG_PORTS_NC( h, mode )[SMX_SIG_PORT_COUNT_NC( h, mode )]

/**
 * Macro to get the port array pointer of a net.
 */
#define SMX_SIG_PORT_ARR_PTR( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_ARR_NC( h, mode ) )

/**
 * Macro to get a pointer to the port array length of a net.
 */
#define SMX_SIG_PORT_LEN( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_LEN_NC( h, mode ) )

/**
 * Macro to get a pointer to the port array length of a net without sanity
 * checks.
 */
#define SMX_SIG_PORT_LEN_NC( h, mode )\
    SMX_SIG_NC( h )->mode.len

/**
 * Macro to get a pointer to the port array count of a net.
 */
#define SMX_SIG_PORT_COUNT( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : &SMX_SIG_PORT_COUNT_NC( h, mode ) )

/**
 * Macro to get a pointer to the port array count of a net without sanity
 * checks.
 */
#define SMX_SIG_PORT_COUNT_NC( h, mode )\
    SMX_SIG_NC( h )->mode.count

/**
 * Macro to get the index of a port.
 */
#define SMX_SIG_PORT_IDX( box_name, port_name, mode )\
    SMX_PORT_IDX_ ## box_name ## _ ## mode ## _ ## port_name

/**
 * Macro to get the port signature of a net without sanity checks.
 */
#define SMX_SIG_PORT_NC( h, box_name, port_name, mode )\
    SMX_SIG_PORTS_NC( h, mode )[SMX_SIG_PORT_IDX( box_name, port_name, mode )]

/**
 * Macro to get a pointer to the port signature of a net.
 */
#define SMX_SIG_PORT_PTR( h, box_name, port_name, mode )\
    ( ( SMX_NO_SIG( h ) || SMX_NO_SIG_PORT( h, box_name, port_name, mode ) )\
        ? NULL : &SMX_SIG_PORT_NC( h, box_name, port_name, mode ) )

/**
 * Macro to get the port signature structure.
 */
#define SMX_SIG_PORTS( h, mode )\
    ( SMX_NO_SIG( h ) ? NULL : SMX_SIG_PORTS_NC( h, mode ) )

/**
 * Macro to get the port signature structure without sanity checks.
 */
#define SMX_SIG_PORTS_NC( h, mode )\
    ( ( smx_channel_t** )SMX_SIG_NC( h )->mode.ports )

/**
 * Helper macro to create a string out of an expression.
 */
#define STRINGIFY(x) #x

/**
 * Allocate space with malloc and log an error if malloc fails
 *
 * @param size  the memory size to allocate
 * @return      a void pointer to the allocated memory
 */
void* smx_malloc( size_t size );

#endif /* SMXUTILS_H */
