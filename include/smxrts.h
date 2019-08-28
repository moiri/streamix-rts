/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * @defgroup smx Complete Streamix API
 */

#include <pthread.h>
#include <stdlib.h>
#include <zlog.h>
#include "box_smx_rn.h"
#include "box_smx_tf.h"
#include "msg_tsmem.h"
#include "smxch.h"
#include "smxconfig.h"
#include "smxlog.h"
#include "smxmsg.h"
#include "smxnet.h"
#include "smxprofiler.h"
#include "smxtypes.h"
#include "smxutils.h"

#ifndef SMXRTS_H
#define SMXRTS_H

// TYPEDEFS -------------------------------------------------------------------

typedef struct smx_rts_s smx_rts_t; /**< ::smx_rts_s */

// STRUCTS --------------------------------------------------------------------

/**
 * The main RTS structure holding information about the streamix network.
 */
struct smx_rts_s
{
    int ch_cnt;                     /**< the number of channels of the system */
    int net_cnt;                    /**< the number of nets of the system */
    pthread_barrier_t init_done;    /**< the barrier for syncing initialisation */
    void* conf;                     /**< the application configuration */
    pthread_t ths[SMX_MAX_NETS];    /**< the array holding all thread ids */
    smx_channel_t* chs[SMX_MAX_CHS];/**< the array holding all channel pointers */
    smx_net_t* nets[SMX_MAX_NETS];  /**< the array holdaing all net pointers */
    struct timespec start_wall;     /**< the walltime of the application start */
    struct timespec end_wall;       /**< the walltime of the application end. */
};

// RTS MACROS ------------------------------------------------------------------

/**
 * Macro to create a streamix channel.
 */
#define SMX_CHANNEL_CREATE( id, len, type, name )\
    rts->chs[id] = smx_channel_create( &rts->ch_cnt, len, type, id, #name,\
            STRINGIFY( ch_ ## name ## _ ## id ) )

/**
 * Macro to destroy a streamix channel.
 */
#define SMX_CHANNEL_DESTROY( id )\
    smx_channel_destroy( rts->chs[id] )

/**
 * Macro to connect a streamix channel and a net by name mapping.
 */
#define SMX_CONNECT( net_id, ch_id, box_name, ch_name, mode )\
    smx_connect_ ## mode(\
            SMX_SIG_PORT_PTR( rts->nets[net_id], box_name, ch_name, mode ),\
            rts->chs[ch_id], rts->nets[net_id], SMX_MODE_ ## mode,\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], mode ) )

/**
 * Macro to connect a streamix channel and a net by id mapping.
 */
#define SMX_CONNECT_ARR( net_id, ch_id, mode )\
    smx_connect_ ## mode( SMX_SIG_PORT_ARR_PTR( rts->nets[net_id], mode ),\
            rts->chs[ch_id], rts->nets[net_id], SMX_MODE_ ## mode,\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], mode ) )

/**
 * Macro to connect a rate control guard to a streamix channel.
 */
#define SMX_CONNECT_GUARD( id, iats, iatns )\
    smx_connect_guard( rts->chs[id],\
            smx_guard_create( iats, iatns, rts->chs[id] ) )

/**
 * Macro to connect a collector to a routing node net.
 */
#define SMX_CONNECT_RN( net_id, ch_id )\
    smx_connect_rn( rts->chs[ch_id], rts->nets[net_id] )

/**
 * Macro to interconnect a temporal firewall with streamix channels.
 */
#define SMX_CONNECT_TF( net_id, ch_in_id, ch_out_id, ch_name )\
    smx_connect_tf( rts->nets[net_id], rts->chs[ch_in_id], rts->chs[ch_out_id] )

/**
 * Macro to create a streamix net.
 */
#define SMX_NET_CREATE( id, net_name, box_name, prio )\
    rts->nets[id] = smx_net_create( &rts->net_cnt, id, #net_name, #box_name,\
            STRINGIFY( net_ ## net_name ## _ ## id ), rts->conf,\
            &rts->init_done, prio )

/**
 * Macro to destroy a streamix net.
 */
#define SMX_NET_DESTROY( id )\
    smx_net_destroy( rts->nets[id] )

/**
 * Macro to destroy a additional structures created by a routing node.
 */
#define SMX_NET_DESTROY_RN( id )\
    smx_net_destroy_rn( rts->nets[id] );\

/**
 * Macro to destroy a additional structures created by a temporal firewall.
 */
#define SMX_NET_DESTROY_TF( id )\
    smx_net_destroy_tf( rts->nets[id] );\

/**
 * Macro to interconnect temporal fierwalls with neighbouring nets.
 */
#define SMX_NET_FINALIZE_TF( net_id )\
    smx_net_finalize_tf( rts->nets[net_id] )

/**
 * Allocate the necessary space for a net structure.
 */
#define SMX_NET_INIT( id, indegree, outdegree )\
    smx_net_init( rts->nets[id], indegree, outdegree )

/**
 * Allocate the necessary space for a routing node structure.
 */
#define SMX_NET_INIT_RN( id )\
    smx_net_init_rn( rts->nets[id] )

/**
 * Allocate the necessary space for a temporal firewall structure.
 */
#define SMX_NET_INIT_TF( id, sec, nsec )\
    smx_net_init_tf( rts->nets[id], sec, nsec )

/**
 * Macro to create and execute a thread associated to a net.
 */
#define SMX_NET_RUN( id, box_name )\
    smx_net_run( rts->ths, id, start_routine_ ## box_name, rts->nets[id] )

/**
 * Macro to wait for all threads to reach this point.
 */
#define SMX_NET_WAIT_END( id )\
    pthread_join( rts->ths[id], NULL )

/**
 * Macro to wait for initialisation of all nets to complete before running the
 * application.
 */
#define SMX_PROGRAM_INIT_RUN()\
    smx_program_init_run( rts )

/**
 * The start routing to be passed to the pthread.
 */
#define START_ROUTINE_NET( h, box_name )\
    smx_net_start_routine( h, box_name, box_name ## _init, box_name ## _cleanup )

// FUNCTIONS ------------------------------------------------------------------

/**
 * @brief Perfrom some cleanup tasks
 *
 * Close the log file
 *
 * @param rts   a pointer to the RTS structure
 */
void smx_program_cleanup( smx_rts_t* rts );

/**
 * Initialize the rts structure, read the configuration files, and initialize
 * the log.
 *
 * @param config    the path of the config file to be loaded
 * @return a pointer to the RTS structure which holds the network information.
 */
smx_rts_t* smx_program_init( const char* config );

/**
 * Initialize the synchronisation barrier to make sure all nets finish
 * intialisation befor staring the main loop.
 *
 * @param rts a pointer to the RTS structure which holds the network information.
 */
void smx_program_init_run( smx_rts_t* rts );

#endif // SMXRTS_H
