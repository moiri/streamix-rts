/**
 * Message definitions for the runtime system library of Streamix
 *
 * @file    smxmsg.c
 * @author  Simon Maurer
 */

#include <string.h>
#include "smxmsg.h"
#include "smxutils.h"
#include "smxlog.h"
#include "smxprofiler.h"

/*****************************************************************************/
smx_msg_t* smx_msg_copy( void* h, smx_msg_t* msg )
{
    if( msg == NULL )
        return NULL;

    SMX_LOG_MAIN( msg, info, "copy message '%lu' in net '%s(%d)'", msg->id,
            SMX_NET_GET_NAME( h ), SMX_NET_GET_ID( h ) );
    smx_profiler_log_msg( h, msg, SMX_PROFILER_ACTION_COPY );
    return smx_msg_create( h, msg->copy( msg->data, msg->size ),
            msg->size, msg->copy, msg->destroy, msg->unpack );
}

/*****************************************************************************/
smx_msg_t* smx_msg_create( void* h, void* data, size_t size,
        void* copy( void*, size_t ), void destroy( void* ),
        void* unpack( void* ) )
{
    static unsigned long msg_count = 0;
    smx_msg_t* msg = smx_malloc( sizeof( struct smx_msg_s ) );
    if( msg == NULL )
        return NULL;

    msg->id = msg_count++;
    SMX_LOG_MAIN( msg, info, "create message '%lu' in '%s(%d)'", msg->id,
            SMX_NET_GET_NAME( h ), SMX_NET_GET_ID( h ) );
    smx_profiler_log_msg( h, msg, SMX_PROFILER_ACTION_CREATE );
    msg->data = data;
    msg->size = size;
    if( copy == NULL ) msg->copy = smx_msg_data_copy;
    else msg->copy = copy;
    if( destroy == NULL ) msg->destroy = smx_msg_data_destroy;
    else msg->destroy = destroy;
    if( unpack == NULL ) msg->unpack = smx_msg_data_unpack;
    else msg->unpack = unpack;
    return msg;
}

/*****************************************************************************/
void* smx_msg_data_copy( void* data, size_t size )
{
    void* data_copy = smx_malloc( size );
    if( data_copy == NULL ) 
        return NULL;

    memcpy( data_copy, data, size );
    return data_copy;
}

/*****************************************************************************/
void smx_msg_data_destroy( void* data )
{
    if( data == NULL )
        return;

    free( data );
}

/*****************************************************************************/
void* smx_msg_data_unpack( void* data )
{
    return data;
}

/*****************************************************************************/
void smx_msg_destroy( void* h, smx_msg_t* msg, int deep )
{
    if( msg == NULL )
        return;

    SMX_LOG_MAIN( msg, info, "destroy message '%lu' in '%s(%d)'", msg->id,
            SMX_NET_GET_NAME( h ), SMX_NET_GET_ID( h ) );
    smx_profiler_log_msg( h, msg, SMX_PROFILER_ACTION_DESTROY );
    if( deep )
        msg->destroy( msg->data );
    free( msg );
}

/*****************************************************************************/
void* smx_msg_unpack( smx_msg_t* msg )
{
    return msg->unpack( msg->data );
}
