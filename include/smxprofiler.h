/**
 * Profiler definitions for the runtime system library of Streamix
 *
 * @file    smxprofiler.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef SMXPROFILER_H
#define SMXPROFILER_H

typedef enum smx_profiler_action_e smx_profiler_action_t;
typedef struct smx_mongo_msg_s smx_mongo_msg_t;

/**
 * The different actions a profiler can log.
 */
enum smx_profiler_action_e
{
    SMX_PROFILER_ACTION_START,          /**< start a net. */
    SMX_PROFILER_ACTION_CREATE,         /**< create a msg, channel, or net. */
    SMX_PROFILER_ACTION_DESTROY,        /**< destroy a msg, channel, or net. */
    SMX_PROFILER_ACTION_COPY,           /**< copy a message. */
    SMX_PROFILER_ACTION_READ,           /**< read from a channel. */
    SMX_PROFILER_ACTION_READ_COLLECTOR, /**< read from a collector. */
    SMX_PROFILER_ACTION_WRITE,          /**< write to a channel. */
    SMX_PROFILER_ACTION_WRITE_COLLECTOR,/**< write to a collector. */
    SMX_PROFILER_ACTION_OVERWRITE,      /**< overwrite a message in a channel. */
    SMX_PROFILER_ACTION_DISMISS,        /**< dismiss a message in a channel. */
    SMX_PROFILER_ACTION_DUPLICATE,      /**< duplicate a message ina channel. */
    SMX_PROFILER_ACTION_DL_MISS         /**< missed a deadline. */
};

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

#endif
