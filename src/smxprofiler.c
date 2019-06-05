/**
 * Profiler definitions for the runtime system library of Streamix
 *
 * @file    smxprofiler.h
 * @author  Simon Maurer
 */

#include <time.h>
#include "box_smx_mongo.h"
#include "smxlog.h"
#include "smxprofiler.h"
#include "smxutils.h"
#include "zlog.h"

/**
 * [type, id_net, name_net, action]
 */
#define JSON_PROFILER_NET   "[%d,%d,\"%s\",%d]"

/**
 * [type, id_ch, name_net, name_ch, action, count]
 */
#define JSON_PROFILER_CH    "[%d,%d,\"%s\",\"%s\",%d,%d]"

/**
 * [type, id_msg, name_net, action]
 */
#define JSON_PROFILER_MSG   "[%d,%d,\"%s\",%s]"

/*****************************************************************************/
void smx_profiler_destroy_msg( void* data )
{
    free( ( ( smx_mongo_msg_t* )data )->j_data );
    free( data );
}

/*****************************************************************************/
void smx_profiler_log( smx_net_t* net, smx_profiler_type_t type,
        const char* format, ... )
{
    if( ( net == NULL ) || ( net->profile == NULL ) )
        return;

    va_list arg_ptr;
    smx_mongo_msg_t* data = smx_malloc( sizeof( struct smx_mongo_msg_s ) );
    data->j_data = smx_malloc( 2000 );

    clock_gettime( CLOCK_REALTIME, &data->ts );

    va_start( arg_ptr, format );
    sprintf( data->j_data, format, type, arg_ptr );
    va_end( arg_ptr );

    smx_msg_t* msg = smx_msg_create( net, data, sizeof( data ), NULL,
            smx_profiler_destroy_msg, NULL, 1 );
    smx_channel_write( net, net->profile, msg );
}

/*****************************************************************************/
void smx_profile_log_ch( smx_net_t* net, smx_channel_t* ch,
        smx_profiler_action_t action, int val )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_CH, JSON_PROFILER_CH,
            ch->id, net->name, ch->name, action, val );
}

/*****************************************************************************/
void smx_profile_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_t action )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_MSG, JSON_PROFILER_MSG, msg->id,
            net->name, action );
}

/*****************************************************************************/
void smx_profile_log_net( smx_net_t* net, smx_profiler_action_t action )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_NET, JSON_PROFILER_NET, net->id,
            net->name, action );
}
