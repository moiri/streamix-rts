/**
 * Profiler definitions for the runtime system library of Streamix
 *
 * @file    smxprofiler.c
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <string.h>
#include <time.h>
#include "smxprofiler.h"
#include "smxutils.h"

/**
 * [type, id_net, name_net, action]
 */
#define JSON_PROFILER_NET   "[%d,%d,\"%s\",%d]"

/**
 * [type, id_ch, id_net, name_ch, id_msg, action, count]
 */
#define JSON_PROFILER_CH    "[%d,%d,%d,\"%s\",%d,%d,%d]"

/**
 * [type, id_msg, name_net, action]
 */
#define JSON_PROFILER_MSG   "[%d,%d,\"%s\",%d]"

/*****************************************************************************/
void* smx_mongo_msg_copy( void* data, size_t size )
{
    smx_mongo_msg_t* mg = data;
    smx_mongo_msg_t* data_copy = smx_malloc( size );
    data_copy->ts = mg->ts;
    data_copy->j_data = smx_malloc( strlen( mg->j_data ) );
    strcpy( data_copy->j_data, mg->j_data );
    return data_copy;
}

/*****************************************************************************/
void* smx_mongo_msg_create( smx_net_t* net, smx_mongo_msg_t* data )
{
    return smx_msg_create( net, data, sizeof( struct smx_mongo_msg_s ),
            smx_mongo_msg_copy, smx_mongo_msg_destroy, NULL, 0 );
}

/*****************************************************************************/
void smx_mongo_msg_destroy( void* data )
{
    smx_mongo_msg_t* mg = data;
    if( mg->j_data != NULL )
        free( mg->j_data );
    free( data );
}

/*****************************************************************************/
void smx_profiler_log( smx_net_t* net, const char* format, ... )
{
    va_list arg_ptr;
    smx_mongo_msg_t* data = smx_malloc( sizeof( struct smx_mongo_msg_s ) );
    data->j_data = smx_malloc( 2000 );

    clock_gettime( CLOCK_REALTIME, &data->ts );

    va_start( arg_ptr, format );
    vsprintf( data->j_data, format, arg_ptr );
    va_end( arg_ptr );

    smx_msg_t* msg = smx_msg_create( net, data, sizeof( data ), NULL,
            smx_mongo_msg_destroy, NULL, 1 );
    smx_channel_write( net, net->profiler, msg );
}

/*****************************************************************************/
void smx_profiler_log_ch( smx_net_t* net, smx_channel_t* ch, smx_msg_t* msg,
        smx_profiler_action_t action, int val )
{
    if( ( net == NULL ) || ( net->profiler == NULL ) || ( ch == NULL )
            || ( msg != NULL && msg->is_profiler ) )
        return;
    if( action == SMX_PROFILER_ACTION_READ )
        ch->sink->count++;
    if( action == SMX_PROFILER_ACTION_WRITE )
        ch->source->count++;
    int msg_id = ( msg == NULL ) ? -1 : msg->id;
    smx_profiler_log( net, JSON_PROFILER_CH, SMX_PROFILER_TYPE_CH,
            ch->id, net->id, ch->name, msg_id, action, val );
}

/*****************************************************************************/
void smx_profiler_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_t action )
{
#ifndef SMX_PROFILER_MSG
    return;
#endif
    if( ( net == NULL ) || ( net->profiler == NULL ) || msg == NULL
            || msg->is_profiler )
        return;
    smx_profiler_log( net, JSON_PROFILER_MSG, SMX_PROFILER_TYPE_MSG, msg->id,
            net->name, action );
}

/*****************************************************************************/
void smx_profiler_log_net( smx_net_t* net, smx_profiler_action_t action )
{
    if( ( net == NULL ) || ( net->profiler == NULL ) )
        return;
    smx_profiler_log( net, JSON_PROFILER_NET, SMX_PROFILER_TYPE_NET, net->id,
            net->name, action );
}
