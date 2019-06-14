/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <pthread.h>
#include <stdlib.h>
#include <zlog.h>
#include "box_smx_profiler.h"
#include "box_smx_rn.h"
#include "box_smx_tf.h"
#include "smxch.h"
#include "smxlog.h"
#include "smxmsg.h"
#include "smxnet.h"
#include "smxprofiler.h"
#include "smxutils.h"

#ifndef SMXRTS_H
#define SMXRTS_H

#define SMX_TF_PRIO     3

// TYPEDEFS -------------------------------------------------------------------
typedef struct smx_rts_s smx_rts_t;

// ENUMS ----------------------------------------------------------------------

// STRUCTS --------------------------------------------------------------------

struct smx_rts_s
{
    smx_channel_t* chs[SMX_MAX_CHS];
    smx_net_t* nets[SMX_MAX_NETS];
    pthread_t ths[SMX_MAX_NETS];
    void* conf;
    int ch_cnt;
    int net_cnt;
};

// USER MACROS -----------------------------------------------------------------
/**
 *
 */
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( h, SMX_SIG_PORT( h, box_name, ch_name, in ) )

/**
 *
 */
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( h, SMX_SIG_PORT( h, box_name, ch_name, out ), data )

/**
 *
 */
#define SMX_LOG( h, level, format, ... )\
    SMX_LOG_NET( h, level, format, ##__VA_ARGS__ )

/**
 *
 */
#define SMX_MSG_CREATE( h, data, dsize, fcopy, ffree, funpack )\
    smx_msg_create( h, data, dsize, fcopy, ffree, funpack, 0 )

/**
 *
 */
#define SMX_MSG_COPY( h, msg )\
    smx_msg_copy( h, msg )

/**
 *
 */
#define SMX_MSG_DESTROY( h, msg )\
    smx_msg_destroy( h, msg, 1 )

/**
 *
 */
#define SMX_MSG_UNPACK( msg )\
    smx_msg_unpack( msg )

// RTS MACROS ------------------------------------------------------------------
#define SMX_CHANNEL_CREATE( id, len, type, name )\
    rts->chs[id] = smx_channel_create( &rts->ch_cnt, len, type, id, #name,\
            STRINGIFY( ch_ ## name ## _ ## id ) )

#define SMX_CHANNEL_DESTROY( id )\
    smx_channel_destroy( rts->chs[id] )

#define SMX_CONNECT( net_id, ch_id, box_name, ch_name, mode )\
    smx_connect( SMX_SIG_PORT_PTR( rts->nets[net_id], box_name, ch_name, mode ),\
            rts->chs[ch_id], net_id, rts->nets[net_id]->name, SMX_MODE_ ## mode,\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], mode ) )

#define SMX_CONNECT_ARR( net_id, ch_id, mode )\
    smx_connect( SMX_SIG_PORT_ARR_PTR( rts->nets[net_id], mode ),\
            rts->chs[ch_id], net_id, rts->nets[net_id]->name, SMX_MODE_ ## mode,\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], mode ) )

#define SMX_CONNECT_GUARD( id, iats, iatns )\
    smx_connect_guard( rts->chs[id],\
            smx_guard_create( iats, iatns, rts->chs[id] ) )

#define SMX_CONNECT_RN( net_id, ch_id )\
    smx_connect_rn( rts->chs[ch_id], rts->nets[net_id] )

#define SMX_CONNECT_TF( net_id, ch_in_id, ch_out_id, ch_name )\
    smx_connect_tf( rts->nets[net_id], rts->chs[ch_in_id], rts->chs[ch_out_id] )

#define SMX_NET_CREATE( id, net_name, box_name )\
    rts->nets[id] = smx_net_create( &rts->net_cnt, id, #net_name,\
            STRINGIFY( net_ ## net_name ## _ ## id ), &rts->conf )

#define SMX_NET_DESTROY( id )\
    smx_net_destroy( rts->nets[id] )

#define SMX_NET_DESTROY_PROFILER( id )\
    smx_net_destroy_profiler( rts->nets[id] )

#define SMX_NET_DESTROY_RN( id )\
    smx_net_destroy_rn( rts->nets[id] );\

#define SMX_NET_DESTROY_TF( id )\
    smx_net_destroy_tf( rts->nets[id] );\

#define SMX_NET_FINALIZE_PROFILER( net_id )\
    smx_net_finalize_profiler( rts->nets[net_id], rts->nets, rts->net_cnt )

#define SMX_NET_FINALIZE_TF( net_id )\
    smx_net_finalize_tf( rts->nets[net_id] )

#define SMX_NET_INIT( id, indegree, outdegree )\
    smx_net_init( rts->nets[id], indegree, outdegree )

#define SMX_NET_INIT_PROFILER( id )\
    smx_net_init_profiler( rts->nets[id] )

#define SMX_NET_INIT_RN( id )\
    smx_net_init_rn( rts->nets[id] )

#define SMX_NET_INIT_TF( id, sec, nsec )\
    smx_net_init_tf( rts->nets[id], sec, nsec )

#define SMX_NET_RUN( id, box_name, prio )\
    smx_net_run( rts->ths, id, start_routine_ ## box_name, rts->nets[id], prio )

#define SMX_NET_WAIT_END( id )\
    pthread_join( rts->ths[id], NULL )

#define SMX_PROGRAM_INIT_RUN() ;

#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup( rts )

#define SMX_PROGRAM_INIT()\
    smx_rts_t* rts = smx_program_init()

#define START_ROUTINE_NET( h, box_name )\
    start_routine_net( h, box_name, box_name ## _init, box_name ## _cleanup )

#define SMX_NET_EXTERN( box_name )\
    extern int box_name( void*, void* );\
    extern int box_name ## _init( void*, void** );\
    extern void box_name ## _cleanup( void*, void* )

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
 * @return a pointer to the RTS structure which holds the network information.
 */
smx_rts_t* smx_program_init();

/**
 * Initialize the profiler if enabled.
 *
 * @param rts a pointer to the RTS structure which holds the network information.
 */
void smx_program_init_run( smx_rts_t* rts );

#endif // SMXRTS_H
