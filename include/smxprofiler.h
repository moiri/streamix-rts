/**
 * @file    smxprofiler.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Profiler definitions for the runtime system library of Streamix
 */

#include "smxtypes.h"

#ifndef SMXPROFILER_H
#define SMXPROFILER_H

/**
 * The function to log profiler messages related to a channel.
 *
 * @param net       a pointer to the net handler which logs the event.
 * @param ch        a pointer to the channel which logs the event.
 * @param msg       a pointer to the message invloved in the log event.
 * @param action    the channel action.
 * @param val       the current message count held in the channel.
 */
void smx_profiler_log_ch( smx_net_t* net, smx_channel_t* ch, smx_msg_t* msg,
        smx_profiler_action_t action, int val );

/**
 * The function to log profiler messages related to a smx message.
 *
 * @param net       a pointer to the net handler which logs the event.
 * @param msg       a pointer to the message which logs the event.
 * @param action    the message action.
 */
void smx_profiler_log_msg( smx_net_t* net, smx_msg_t* msg,
        smx_profiler_action_t action );

/**
 * The function to log profiler messages related to a smx net.
 *
 * @param net       a pointer to the net handler which logs the event.
 * @param action    the net action.
 */
void smx_profiler_log_net( smx_net_t* net, smx_profiler_action_t action );

#endif /* SMXPROFILER_H */
