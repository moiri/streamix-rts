/**
 * Temporal firewall box implementation for the runtime system library of
 * Streamix
 *
 * @file    box_smx_tf.h
 * @author  Simon Maurer
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef BOX_SMX_TF_H
#define BOX_SMX_TF_H

typedef struct net_smx_tf_s net_smx_tf_t;             /**< ::net_smx_tf_s */
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
    net_smx_tf_t*       ports;      /**< list of temporal firewalls */
    int                 count;      /**< number of port pairs */
};

/**
 * @brief grow the list of temporal firewalls and connect channels
 *
 * @param timer     pointer to a timer structure
 * @param ch_in     input channel to the temporal firewall
 * @param ch_out    output channel from the temporal firewall
 * @param id        the id of the timer
 */
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out, int id );

/**
 * @brief create a periodic timer structure
 *
 * @param sec   time interval in seconds
 * @param nsec  time interval in nano seconds
 * @return      pointer to the created timer structure
 */
smx_timer_t* smx_tf_create( int sec, int nsec);

/**
 * @brief destroy a timer structure and the list of temporal firewalls inside
 *
 * @param tt    pointer to the temporal firewall
 */
void smx_tf_destroy( smx_net_t* tt );

/**
 * @brief enable periodic tt timer
 *
 * @param h     the net handler
 * @param timer pointer to a timer structure
 */
void smx_tf_enable( void* h, smx_timer_t* timer );

/**
 * Read all input channels of a temporal firewall and propagate the messages to
 * the corresponding outputs of the temporal firewall.
 *
 * @param tt     a pointer to the timer
 * @param ch_in  a pointer to an array of input channels
 * @param ch_out a pointer to an array of output channels
 * @param copy   1 if messages ought to be duplicated, 0 otherwise
 */
void smx_tf_propagate_msgs( smx_timer_t* tt, smx_channel_t** ch_in,
        smx_channel_t** ch_out, int copy );

/**
 * @brief blocking wait on timer
 *
 * Waits on the specified time interval. An error message is printed if the
 * deadline was missed.
 *
 * @param h     the net handler
 * @param timer pointer to a timer structure
 */
void smx_tf_wait( void* h, smx_timer_t* timer );

/**
 * @brief write to all output channels of a temporal firewall
 *
 * @param msg       a pointer to the message array. The array has a length that
 *                  matches the number of output channels of the temporal
 *                  firewall
 * @param tt        a pointer to the timer
 * @param ch_in     a pointer to an array of input channels
 * @param ch_out    a pointer to an array of output channels
 */
void smx_tf_write_outputs( smx_msg_t**, smx_timer_t*, smx_channel_t**,
        smx_channel_t** );

/**
 * @brief start routine for a timer thread
 *
 * @param h pointer to a timer structure
 * @return  NULL
 */
void* start_routine_tf( void* h );

#endif
