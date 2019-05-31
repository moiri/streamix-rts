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
void smx_profiler_log( smx_net_t* net, smx_profiler_type_t type, ... )
{
    if( net->profile == NULL )
        return;

    va_list arg_ptr;
    struct timespec now;
    const char* profile;
    smx_mongo_msg_t* data;
    char* buf = smx_malloc( 2000 );

    switch( type )
    {
        case SMX_PROFILER_TYPE_CH:
            profile = JSON_PROFILER_CH;
            break;
        case SMX_PROFILER_TYPE_MSG:
            profile = JSON_PROFILER_MSG;
            break;
        case SMX_PROFILER_TYPE_NET:
            profile = JSON_PROFILER_NET;
            break;
    }

    data = smx_malloc( sizeof( struct smx_mongo_msg_s ) );

    clock_gettime( CLOCK_REALTIME, &now );
    data->ts.tv_sec = now.tv_sec;
    data->ts.tv_nsec = now.tv_nsec;

    va_start( arg_ptr, type );
    sprintf( buf, profile, type, arg_ptr );
    va_end( arg_ptr );
    data->j_data = buf;

    smx_msg_t* msg = smx_msg_create( data, sizeof( data ), NULL,
            smx_profiler_destroy_msg, NULL );
    smx_channel_write( net, net->profile, msg );
}

/*****************************************************************************/
void smx_profile_log_ch( smx_net_t* net, smx_channel_t* ch,
        smx_profiler_action_t action, int val )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_CH, ch->id, net->name,
            ch->name, action, val );
}

/*****************************************************************************/
void smx_profile_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_t action )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_MSG, msg->id, net->name,
            action );
}

/*****************************************************************************/
void smx_profile_log_net( smx_net_t* net, smx_profiler_action_t action )
{
    smx_profiler_log( net, SMX_PROFILER_TYPE_NET, net->id, net->name,
            action );
}
