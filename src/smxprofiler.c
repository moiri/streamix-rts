/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Profiler definitions for the runtime system library of Streamix
 */

#include <string.h>
#include <time.h>
#include "smxprofiler.h"
#include "smxutils.h"
#include "lttng_tp.h"

#define tracepoint_ch(action)\
    tracepoint(smx_lttng, action, ch->id, net->id, ch->name, msg_id, val)

/*****************************************************************************/
void smx_profiler_log_ch( smx_net_t* net, smx_channel_t* ch, smx_msg_t* msg,
        smx_profiler_action_ch_t action, int val )
{
    if( net == NULL || !net->has_profiler || ch == NULL )
        return;
    int msg_id = ( msg == NULL ) ? -1 : msg->id;
    switch(action)
    {
        case SMX_PROFILER_ACTION_CH_READ:
            ch->sink->count++;
            tracepoint_ch(ch_read);
            break;
        case SMX_PROFILER_ACTION_CH_READ_COLLECTOR:
            tracepoint_ch(ch_read_collector);
            break;
        case SMX_PROFILER_ACTION_CH_WRITE:
            ch->source->count++;
            tracepoint_ch(ch_write);
            break;
        case SMX_PROFILER_ACTION_CH_WRITE_COLLECTOR:
            tracepoint_ch(ch_write_collector);
            break;
        case SMX_PROFILER_ACTION_CH_OVERWRITE:
            tracepoint_ch(ch_overwrite);
            break;
        case SMX_PROFILER_ACTION_CH_DISMISS:
            tracepoint_ch(ch_dismiss);
            break;
        case SMX_PROFILER_ACTION_CH_DUPLICATE:
            tracepoint_ch(ch_duplicate);
            break;
        case SMX_PROFILER_ACTION_CH_DL_MISS_SRC:
            tracepoint_ch(ch_dl_miss_src);
            break;
        case SMX_PROFILER_ACTION_CH_DL_MISS_SRC_CP:
            tracepoint_ch(ch_dl_miss_src_cp);
            break;
        case SMX_PROFILER_ACTION_CH_DL_MISS_SINK:
            tracepoint_ch(ch_dl_miss_sink);
            break;
        case SMX_PROFILER_ACTION_CH_TT_MISS_SRC:
            tracepoint_ch(ch_tt_miss_src);
            break;
        case SMX_PROFILER_ACTION_CH_TT_MISS_SRC_CP:
            tracepoint_ch(ch_tt_miss_src_cp);
            break;
        case SMX_PROFILER_ACTION_CH_TT_MISS_SINK:
            tracepoint_ch(ch_tt_miss_sink);
            break;
    }
}

#define tracepoint_msg(action)\
    tracepoint(smx_lttng, action, msg->id, net->name)

/*****************************************************************************/
void smx_profiler_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_msg_t action )
{
#ifndef SMX_PROFILER_MSG
    return;
#endif
    if( net == NULL || !net->has_profiler || msg == NULL )
        return;
    switch(action)
    {
        case SMX_PROFILER_ACTION_MSG_CREATE:
            tracepoint_msg(msg_create);
            break;
        case SMX_PROFILER_ACTION_MSG_COPY_START:
            tracepoint_msg(msg_copy_start);
            break;
        case SMX_PROFILER_ACTION_MSG_COPY_END:
            tracepoint_msg(msg_copy_end);
            break;
        case SMX_PROFILER_ACTION_MSG_DESTROY:
            tracepoint_msg(msg_destroy);
            break;
    }
}

#define tracepoint_net(action)\
    tracepoint(smx_lttng, action, net->id, net->name)

/*****************************************************************************/
void smx_profiler_log_net( smx_net_t* net, smx_profiler_action_net_t action )
{
    if( net == NULL || !net->has_profiler )
        return;
    switch(action)
    {
        case SMX_PROFILER_ACTION_NET_START:
            tracepoint_net(net_start);
            break;
        case SMX_PROFILER_ACTION_NET_START_IMPL:
            tracepoint_net(net_start_impl);
            break;
        case SMX_PROFILER_ACTION_NET_END_IMPL:
            tracepoint_net(net_end_impl);
            break;
        case SMX_PROFILER_ACTION_NET_END:
            tracepoint_net(net_end);
            break;
    }
}
