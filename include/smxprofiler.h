/**
 * Profiler definitions for the runtime system library of Streamix
 *
 * @file    smxprofiler.h
 * @author  Simon Maurer
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef SMXPROFILER_H
#define SMXPROFILER_H

typedef enum smx_profiler_type_e smx_profiler_type_t;
typedef enum smx_profiler_action_e smx_profiler_action_t;

enum smx_profiler_type_e
{
    SMX_PROFILER_TYPE_CH,
    SMX_PROFILER_TYPE_MSG,
    SMX_PROFILER_TYPE_NET
};

enum smx_profiler_action_e
{
    SMX_PROFILER_ACTION_START,
    SMX_PROFILER_ACTION_CREATE,
    SMX_PROFILER_ACTION_DESTROY,
    SMX_PROFILER_ACTION_COPY,
    SMX_PROFILER_ACTION_READ,
    SMX_PROFILER_ACTION_READ_COLLECTOR,
    SMX_PROFILER_ACTION_WRITE,
    SMX_PROFILER_ACTION_WRITE_COLLECTOR,
    SMX_PROFILER_ACTION_OVERWRITE,
    SMX_PROFILER_ACTION_DISMISS,
    SMX_PROFILER_ACTION_DUPLICATE,
    SMX_PROFILER_ACTION_DL_MISS
};

/**
 *
 */
void smx_profiler_destroy_msg( void* data );

/**
 *
 */
void smx_profiler_log( smx_net_t* net, smx_profiler_type_t type, ... );

/**
 *
 */
void smx_profiler_log_ch( smx_net_t* net, smx_channel_t* ch,
        smx_profiler_action_t action, int val );

/**
 *
 */
void smx_profiler_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_t action );

/**
 *
 */
void smx_profiler_log_net( smx_net_t* net, smx_profiler_action_t action );

#endif
