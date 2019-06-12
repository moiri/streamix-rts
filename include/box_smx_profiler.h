/**
 * Profiler box implementation for the runtime system library of Streamix. This
 * box implementation serves as a collector of all profiler ports of all nets.
 * It serves as a routing node that connects to all net instances trsnmits the
 * profiler data to a profiler backend (which is not part of the RTS)
 *
 * @file    box_smx_profiler.h
 * @author  Simon Maurer
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef BOX_SMX_PROFILER_H
#define BOX_SMX_PROFILER_H

typedef struct net_smx_profiler_s net_smx_profiler_t;/**< ::net_smx_profiler_s */

/**
 * The signature of the profiler port collecter
 */
struct net_smx_profiler_s
{
    struct {
        smx_collector_t* collector; /**< ::smx_collector_s */
    } in;                           /**< input channels */
    struct {
        smx_channel_t* port_profiler;
    } out;                          /**< output channels */
};

/**
 * Connect the profiler collector to all nets and the profiler backend
 *
 * @param profiler  a pointer to the profiler collector net
 * @param nets      a pointer to an array holding all nets to be connected to
 *                  the profiler
 * @param net_cnt   the number of nets
 */
void smx_connect_profiler( smx_net_t* profiler, smx_net_t** nets, int net_cnt );

/**
 * Destroy the profiler collector signature
 *
 * @param profiler  pointer to the profiler collector net
 */
void smx_net_profiler_destroy( smx_net_t* profiler );

/**
 * Initialize the profiler collector signature
 *
 * @param profiler  pointer to the profiler collector net
 */
void smx_net_profiler_init( smx_net_t* profiler );

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
 * Initialises the profiler collector. The state structure is allocated with
 * an integer which is used to remember the last port index from which
 * a message was read.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state structure
 * @return      0 on success, -1 on failure
 */
int smx_profiler_init( void* h, void** state );

/**
 * Cleanup the profiler collector by freeing the state structure.
 *
 * @param state pointer to the state structure
 */
void smx_profiler_cleanup( void* state );

#endif
