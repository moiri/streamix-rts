/**
 * Routing node box implementation for the runtime system library of Streamix
 *
 * @file    box_smx_rn.h
 * @author  Simon Maurer
 */

#include "smxch.h"
#include "smxnet.h"

#ifndef BOX_SMX_RN_H
#define BOX_SMX_RN_H

typedef struct net_smx_rn_s net_smx_rn_t;             /**< ::net_smx_rn_s */

/**
 * @brief The signature of a copy synchronizer
 */
struct net_smx_rn_s
{
    struct {
        smx_collector_t* collector; /**< ::smx_collector_s */
    } in;                           /**< input channels */
    struct {
    } out;                          /**< output channels */
};

/**
 * Connect a routing node to a channel
 *
 * @param ch    the target channel
 * @param guard the rn to be connected
 */
void smx_connect_rn( smx_channel_t* ch, smx_net_t* rn );

/**
 * @brief Destroy copy sync structure
 *
 * @param cp    pointer to the cp sync structure
 */
void smx_net_rn_destroy( net_smx_rn_t* cp );

/**
 * @brief Initialize copy synchronizer structure
 *
 * @param cp    pointer to the copy sync structure
 */
void smx_net_rn_init( net_smx_rn_t* cp );

/**
 * @brief the box implementattion of a routing node (former known as copy sync)
 *
 * A routing node reads from any port where data is available and copies
 * it to every output. The read order is first come first serve with peaking
 * wheter data is available. The cp sync is only blocking on read if no input
 * channel has data available. The copied data is written to the output channel
 * in order how they appear in the list. Writing is blocking. All outputs must
 * be written before new input is accepted.
 *
 * In order to provide fairness the routing node remembers the last port index
 * from which a message was read. The next time the rn is executed it will
 * search for available messages starting from the last port index +1. This
 * means that a routing node is not pure.
 *
 * @param handler   a pointer to the signature
 * @return          returns the state of the box
 */
int smx_rn( void* h, void* state );

/**
 * Initialises the routing node. The state is allocated with an integer which
 * is used to remember the last port index from which a message was read.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state variable
 * @return      0 on success, -1 on failure
 */
int smx_rn_init( void* h, void** state );

/**
 * Cleanup the routing node by freeing the state variable.
 *
 * @param h     pointer to the net handler
 * @param state pointer to the state variable
 */
void smx_rn_cleanup( void* h, void* state );

#endif
