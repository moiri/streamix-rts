/**
 * Profiler box implementation for the runtime system library of Streamix. This
 * box implementation serves as a collector of all profiler ports of all nets.
 * It serves as a routing node that connects to all net instances trsnmits the
 * profiler data to a profiler backend (which is not part of the RTS)
 *
 * @file    box_smx_profiler.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef BOX_SMX_PROFILER_H
#define BOX_SMX_PROFILER_H

/**
 * The signature of the profiler port collecter
 */
enum net_smx_profiler_out_e
{
    SMX_PORT_IDX_smx_profiler_out_profiler
};

/**
 * Destroy the profiler collector signature
 *
 * @param profiler  pointer to the profiler collector net
 */
void smx_net_destroy_profiler( smx_net_t* profiler );

/**
 * Connect the profiler collector to all nets and the profiler backend
 *
 * @param profiler  a pointer to the profiler collector net
 * @param nets      a pointer to an array holding all nets to be connected to
 *                  the profiler
 * @param net_cnt   the number of nets
 */
void smx_net_finalize_profiler( smx_net_t* profiler, smx_net_t** nets,
        int net_cnt );

/**
 * Initialize the profiler collector signature
 *
 * @param profiler  pointer to the profiler collector net
 */
void smx_net_init_profiler( smx_net_t* profiler );

/**
 * Read the timestamp from available messaes in the profiler collector.
 *
 * @param ch    the channel to peak at
 * @param ts    a pointer to a time structrue which will hold the timestamp of
 *              the message
 */
void smx_net_profiler_peak_ts( smx_channel_t* ch, struct timespec* ts );

/**
 * Read from a collector of a net.
 *
 * @param h         pointer to the net handler
 * @param collector pointer to the net collector structure
 * @param in        pointer to the input port array
 * @param count_in  number of input ports
 * @return          the message that was read or NULL if no message was read
 */
smx_msg_t* smx_net_profiler_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in );

/**
 * @brief the box implementattion of the profiler collector
 *
 * A profiling collector reads from any port where data is available and writes
 * it to its single output. The read order is oldest message first with
 * peaking wheter data is available. The profiler collector is only blocking on
 * read if no input channel has data available. Writing is blocking.
 *
 * @param h     a pointer to the signature
 * @param state a pointer to the persistent state structure
 * @return      returns the progress state of the box
 */
int smx_profiler( void* h, void* state );

/**
 * Initialises the profiler collector.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state structure
 * @return      0 on success, -1 on failure
 */
int smx_profiler_init( void* h, void** state );

/**
 * Cleanup the profiler collector by freeing the state structure.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state structure
 */
void smx_profiler_cleanup( void* h, void* state );

/**
 * This function is predefined and must not be changed. It will be passed to the
 * net thread upon creation and will be executed as soon as the thread is
 * started. This function calls a macro which is define in the RTS and handles
 * the initialisation, the main loop of the net and the cleanup.
 *
 * @param h
 *  A pointer to the net handler.
 * @return
 *  This function always returns NULL.
 */
void* start_routine_smx_profiler( void* h );

#endif
