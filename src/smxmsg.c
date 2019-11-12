/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Message definitions for the runtime system library of Streamix
 */

#include <string.h>
#include "smxlog.h"
#include "smxmsg.h"
#include "smxnet.h"
#include "smxprofiler.h"
#include "smxutils.h"

/*****************************************************************************/
smx_msg_t* smx_msg_copy( void* h, smx_msg_t* msg )
{
    if( msg == NULL )
        return NULL;

    SMX_LOG_MAIN( msg, info, "copy message '%lu' in net '%s(%d)'", msg->id,
            SMX_NET_GET_NAME( h ), SMX_NET_GET_ID( h ) );
    smx_profiler_log_msg( h, msg, SMX_PROFILER_ACTION_COPY );
    smx_msg_t* copy = smx_msg_create( h, msg->copy( msg->data, msg->size ),
            msg->size, msg->copy, msg->destroy, msg->unpack );
    if( msg->type != NULL )
        smx_msg_set_type( copy, msg->type );
    if( msg->prevent_backup )
        smx_msg_prevent_backup( copy );
    return copy;
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
    msg->type = NULL;
    msg->data = data;
    msg->size = size;
    msg->prevent_backup = false;
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
    if( msg->type != NULL )
        free( msg->type );
    free( msg );
}

/*****************************************************************************/
int smx_msg_filter( smx_msg_t* msg, int count, ... )
{
    int i;
    va_list arg_ptr;
    const char* arg;

    va_start( arg_ptr, count );

    for( i = 0; i < count; i++ )
    {
        arg = va_arg( arg_ptr, const char* );
        if( ( msg->type == arg )
                || ( ( msg->type != NULL ) && strcmp( msg->type, arg ) == 0 ) )
        {
            break;
        }
    }

    va_end( arg_ptr );

    return (i < count) ? i : -1;
}

/*****************************************************************************/
void smx_msg_prevent_backup( smx_msg_t* msg )
{
    msg->prevent_backup = true;
}

/*****************************************************************************/
void* smx_msg_unpack( smx_msg_t* msg )
{
    return msg->unpack( msg->data );
}

/*****************************************************************************/
int smx_msg_set_type( smx_msg_t* msg, const char* type )
{
    if( msg->type != NULL )
        free( msg->type );
    msg->type = strdup( type );
    return 0;
}
