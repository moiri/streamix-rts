/**
 * @file     smxrts.h
 * @author   Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * The runtime system library for Streamix
 */

#include <pthread.h>
#include <stdlib.h>
#include <zlog.h>
#include "box_smx_rn.h"
#include "box_smx_tf.h"
#include "smxch.h"
#include "smxconfig.h"
#include "smxlog.h"
#include "smxmsg.h"
#include "smxnet.h"
#include "smxprofiler.h"
#include "smxtest.h"
#include "smxtypes.h"
#include "smxutils.h"

#ifndef SMXRTS_H
#define SMXRTS_H

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
 * Macro to accomodate for open ports.
 */
#define SMX_CONNECT_OPEN( net_id, box_name, mode )\
    smx_connect_open(\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], SMX_MODE_LOW_ ## mode ),\
            SMX_ ## mode ## DEGREE_ ## box_name\
    )

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
    rts->nets[id] = smx_net_create( id, #net_name, #box_name,\
            STRINGIFY( net_ ## net_name ## _ ## id ), rts, prio )

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
#define SMX_NET_INIT( id, indegree, outdegree, box_name )\
    smx_net_init( rts->nets[id],\
            indegree + SMX_INDEGREE_ ## box_name,\
            outdegree + SMX_OUTDEGREE_ ## box_name )

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
    smx_net_wait_end( rts->ths[id] )

/**
 * Macro to wait for cleanup of all nets to complete before running the
 * post cleanup functions.
 */
#define SMX_PROGRAM_INIT_CLEANUP()\
    smx_program_init_cleanup( rts )

/**
 * Macro to wait for initialisation of all nets to complete before running the
 * application.
 */
#define SMX_PROGRAM_INIT_RUN()\
    smx_program_init_run( rts )

/**
 * The start routine to be passed to the pthread.
 */
#define START_ROUTINE_NET( h, box_name )\
    smx_net_start_routine( h, box_name, box_name ## _init, box_name ## _cleanup )

/**
 * The start routine to be passed to the pthread. Using shared state
 */
#define START_ROUTINE_NET_WITH_SHARED_STATE( h, box_name, key )\
    smx_net_start_routine_with_shared_state( h, box_name, box_name ## _init,\
            box_name ## _cleanup, box_name ## _init_shared,\
            box_name ## _cleanup_shared, key )

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
 * @param app_conf
 *  The path of the application config file to be loaded.
 * @param log_conf
 *  The path of the log config file to be loaded.
 * @param app_conf_maps
 *  The path list of the config map files to be loaded.
 * @param app_conf_map_count
 *  The number of config map files.
 * @return
 *  A pointer to the RTS structure which holds the network information.
 */
smx_rts_t* smx_program_init( const char* app_conf, const char* log_conf,
        const char** app_conf_maps, int app_conf_map_count,
        const char* arg_file, const char* args );

/**
 * Read and parse custom arguments passed to the streamix app as JSON file or
 * JSON string. If a JSON string is passed, the JSON file will be ignored.
 *
 * @param arg_str
 *  A JSON string. If this is set, `arg_file` will be ignored.
 * @param arg_file
 *  A path to a JSON file. This is only considered if `arg_str` is NULL.
 * @param rts
 *  A pointer to the RTS structure where the args will be stored.
 */
int smx_program_init_args( const char* arg_str, const char* arg_file,
        smx_rts_t* rts );

/**
 * Read a BSON file
 *
 * @param path
 *  The path to the BSON file.
 * @param doc
 *  An initialised bson document where the BSON data will be stored.
 * @return
 *  0 on success, -1 on failure
 */
int smx_program_init_bson_file( const char* path, bson_t* doc );

/**
 * Read and parse an app configuration file.
 *
 * @param conf
 *  The path to the configuration file.
 * @param doc
 *  An initialised bson document where the configuration data will be stored.
 * @param name
 *  A pointer to a location where the name pointer of the app will be stored.
 * @return
 *  0 on success, -1 on failure
 */
int smx_program_init_conf( const char* conf, bson_t* doc, const char** name );

/**
 * Read and parse an app configuration map file.
 *
 * @param path
 *  The path to the configuration map file.
 * @param doc
 *  An initialised bson document where the root mapping data will be stored.
 * @param i_maps
 *  An output parameter where the map list iterator will be stored.
 * @param payload
 *  An uninitialised bson document wehre the payload to be mapped will be
 *  stored.
 * @return
 *  0 on success, -1 on failure
 */
int smx_program_init_maps( const char* path, bson_t* doc, bson_iter_t* i_maps,
        bson_t* payload );

/**
 * Initialize the synchronisation barrier to make sure all nets finish
 * intialisation befor staring the main loop.
 *
 * @param rts
 *  A pointer to the RTS structure which holds the network information.
 */
void smx_program_init_run( smx_rts_t* rts );

/**
 * Get the current version of the library.
 *
 * @return
 *  A version number string.
 */
const char* smx_rts_get_version();

#endif /* SMXRTS_H */
