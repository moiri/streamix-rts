/**
 * Temporal firewall box implementation for the runtime system library of
 * Streamix
 *
 * @file    box_smx_tf.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef BOX_SMX_TF_H
#define BOX_SMX_TF_H

typedef struct net_smx_tf_s net_smx_tf_t;             /**< ::net_smx_tf_s */
typedef struct net_smx_tf_state_s net_smx_tf_state_t; /**< ::net_smx_tf_state_s */
typedef struct smx_timer_s smx_timer_t;               /**< ::smx_timer_s */

/**
 * @brief The signature of a temporal firewall
 */
struct net_smx_tf_s
{
    smx_channel_t*      in;         /**< input channel */
    smx_channel_t*      out;        /**< output channel */
    net_smx_tf_t*       next;       /**< pointer to the next element */
};

/**
 * @brief A Streamix timer structure
 *
 * A timer collects alle temporal firewalls of the same rate
 */
struct smx_timer_s
{
    int                 fd;         /**< timer file descriptor */
    struct itimerspec   itval;      /**< iteration specifiaction */
    net_smx_tf_t*       tfs;        /**< list of temporal firewalls */
    int                 count;      /**< number of port pairs */
};

/**
 * The persistent state to be passed to each iteration.
 */
struct net_smx_tf_state_s
{
    int do_copy;    /**< config argument to indicate whether msgs are copied */
};

/**
 * @brief grow the list of temporal firewalls and connect channels
 *
 * @param net       pointer to the timer net handler
 * @param ch_in     input channel to the temporal firewall
 * @param ch_out    output channel from the temporal firewall
 */
void smx_connect_tf( smx_net_t* net, smx_channel_t* ch_in,
        smx_channel_t* ch_out );

/**
 * @brief create a periodic timer structure
 *
 * @param sec   time interval in seconds
 * @param nsec  time interval in nano seconds
 * @return      pointer to the created timer structure
 */
smx_timer_t* smx_net_create_tf( int sec, int nsec);

/**
 * @brief destroy a timer structure and the list of temporal firewalls inside
 *
 * @param net    pointer to the temporal firewall
 */
void smx_net_destroy_tf( smx_net_t* net );

/**
 * Allocate net ports and assign connected tf ports to the net ports
 *
 * @param net   pointer to the temporal firewall
 */
void smx_net_finalize_tf( smx_net_t* net );

/**
 * @brief init a timer structure and the list of temporal firewalls inside
 *
 * @param net   pointer to the temporal firewall
 * @param sec   time interval in seconds
 * @param nsec  time interval in nano seconds
 */
void smx_net_init_tf( smx_net_t* net, int sec, int nsec );

/**
 * @brief enable periodic tt timer
 *
 * @param h     the net handler
 */
void smx_tf_enable( smx_net_t* h );

/**
 * Read all input channels of a temporal firewall and propagate the messages to
 * the corresponding outputs of the temporal firewall.
 *
 * @param h     pointer to the net handler
 * @param copy   1 if messages ought to be duplicated, 0 otherwise
 */
void smx_tf_propagate_msgs( smx_net_t* h, int copy );

/**
 * @brief blocking wait on timer
 *
 * Waits on the specified time interval. An error message is printed if the
 * deadline was missed.
 *
 * @param h     the net handler
 */
void smx_tf_wait( smx_net_t* h );

/**
 * @brief the box implementattion of the temporal firewall
 *
 * A temporal firewall peridically reads form producers and writes to consumers.
 * All inputs and outputs are decoupled in order to prevent blocking.
 *
 * @param h     a pointer to the signature
 * @param state a pointer to the persistent state structure
 * @return      returns the progress state of the box
 */
int smx_tf( void* h, void* state );

/**
 * Initialises the temporal firewall.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state structure
 * @return      0 on success, -1 on failure
 */
int smx_tf_init( void* h, void** state );

/**
 * Cleanup the temporal firewall by freeing the state structure.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state structure
 */
void smx_tf_cleanup( void* h, void* state );

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
void* start_routine_smx_tf( void* h );

#endif
