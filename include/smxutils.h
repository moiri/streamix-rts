/**
 * utility functions for the runtime system library of Streamix
 *
 * @file    smxutils.h
 * @author  Simon Maurer
 */

#include <stdlib.h>

#ifndef SMXUTILS_H
#define SMXUTILS_H

#define SMX_MODE_in     "->"
#define SMX_MODE_out    "<-"

#define SMX_SIG( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->sig )

#define SMX_SIG_CAT( h )\
    ( h == NULL ) ? zlog_get_category( "undef" ) : ( ( smx_net_t* )h )->cat

#define SMX_SIG_PORT( h, box_name, port_name, mode )\
    ( SMX_SIG( h ) == NULL ) ? NULL : ( ( net_ ## box_name ## _t* )SMX_SIG( h ) )\
            ->mode.port_ ## port_name

#define SMX_SIG_PORT_PTR( h, box_name, port_name, mode )\
    ( SMX_SIG( h ) == NULL ) ? NULL : &( ( net_ ## box_name ## _t* )SMX_SIG( h ) )\
            ->mode.port_ ## port_name

#define SMX_SIG_PORT_COUNT( h, mode )\
    ( h == NULL ) ? NULL : &( ( smx_net_t* )h )->mode.count

#define SMX_SIG_PORTS( h, mode )\
    ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->mode.ports

#define SMX_SIG_PORTS_PTR( h, mode )\
    ( h == NULL ) ? NULL : &( ( smx_net_t* )h )->mode.ports

#define STRINGIFY(x) #x

/**
 * Allocate space with malloc and log an error if malloc fails
 *
 * @param size  the memory size to allocate
 * @return      a void pointer to the allocated memory
 */
void* smx_malloc( size_t size );

#endif
