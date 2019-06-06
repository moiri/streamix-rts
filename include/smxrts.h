/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <pthread.h>
#include <stdlib.h>
#include <zlog.h>
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
#define SMX_MSG_CREATE( data, dsize, fcopy, ffree, funpack )\
    smx_msg_create( NULL, data, dsize, fcopy, ffree, funpack, 0 )

/**
 *
 */
#define SMX_MSG_COPY( msg )\
    smx_msg_copy( NULL, msg )

/**
 *
 */
#define SMX_MSG_DESTROY( msg )\
    smx_msg_destroy( NULL, msg, 1 )

/**
 *
 */
#define SMX_MSG_UNPACK( msg )\
    smx_msg_unpack( msg )

/**
 *
 */
#define SMX_SET_PROFILER_PORT( h, box_name, ch_name )\
    ( ( smx_net_t* ) h )->profiler = SMX_SIG_PORT( h, box_name, ch_name, out )

// RTS MACROS ------------------------------------------------------------------
#define SMX_CHANNEL_CREATE( id, len, type, name )\
    smx_channel_create( rts->chs, &rts->ch_cnt, len, type, id, #name,\
            STRINGIFY( ch_ ## name ## _ ## id ) )

#define SMX_CHANNEL_DESTROY( id )\
    smx_channel_destroy( rts->chs[id] )

#define SMX_CONNECT( net_id, ch_id, net_name, box_name, ch_name, mode )\
    smx_connect( SMX_SIG_PORT_PTR( rts->nets[net_id], box_name, ch_name, mode ),\
            rts->chs[ch_id] )

#define SMX_CONNECT_ARR( net_id, ch_id, net_name, box_name, ch_name, mode )\
    smx_connect_arr( SMX_SIG_PORTS( rts->nets[net_id], mode ),\
            SMX_SIG_PORT_COUNT( rts->nets[net_id], mode ),\
            rts->chs[ch_id], net_id, ch_id, #net_name, #ch_name,\
            SMX_MODE_ ## mode )

#define SMX_CONNECT_GUARD( id, iats, iatns )\
    smx_connect_guard( rts->chs[id],\
            smx_guard_create( iats, iatns, rts->chs[id] ) )

#define SMX_CONNECT_RN( net_id, ch_id )\
    smx_connect_rn( rts->chs[ch_id], rts->nets[net_id] )

#define SMX_CONNECT_TF( timer_id, ch_in_id, ch_out_id, ch_name )\
    smx_tf_connect( SMX_SIG( rts->nets[timer_id] ), rts->chs[ch_in_id],\
            rts->chs[ch_out_id], timer_id )

#define SMX_NET_CREATE( id, net_name, box_name )\
    smx_net_create( rts->nets, &rts->net_cnt, id, #net_name,\
            STRINGIFY( net_ ## net_name ## _ ## id ),\
            smx_malloc( sizeof( struct net_ ## box_name ## _s ) ), &rts->conf )

#define SMX_NET_DESTROY( id, box_name )\
    smx_net_destroy(\
            SMX_SIG_PORTS( rts->nets[id], in ),\
            SMX_SIG_PORTS( rts->nets[id], out ),\
            SMX_SIG( rts->nets[id] ),\
            rts->nets[id] )

#define SMX_NET_INIT( id, box_name, indegree, outdegree )\
    smx_net_init(\
            SMX_SIG_PORT_COUNT( rts->nets[id], in ),\
            SMX_SIG_PORTS_PTR( rts->nets[id], in ), indegree,\
            SMX_SIG_PORT_COUNT( rts->nets[id], out ),\
            SMX_SIG_PORTS_PTR( rts->nets[id], out ), outdegree )

#define SMX_NET_RN_DESTROY( id )\
    smx_net_rn_destroy( ( SMX_SIG( rts->nets[id] ) ) )

#define SMX_NET_RN_INIT( id )\
    smx_net_rn_init( SMX_SIG( rts->nets[id] ) )

#define SMX_NET_RUN( id, net_name, box_name, prio )\
    smx_net_run( rts->ths, id, box_ ## box_name, rts->nets[id], prio )

#define SMX_NET_WAIT_END( id )\
    pthread_join( rts->ths[id], NULL )

#define SMX_PROGRAM_INIT_RUN() ;

#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup( rts )

#define SMX_PROGRAM_INIT()\
    smx_rts_t* rts = smx_program_init()

#define SMX_TF_CREATE( id, sec, nsec )\
    smx_net_create( rts->nets, &rts->net_cnt, id, STRINGIFY( smx_tf ),\
            STRINGIFY( net_nsmx_tf ## _ ## id ), smx_tf_create( sec, nsec ),\
            &rts->conf )

#define SMX_TF_DESTROY( id )\
    smx_tf_destroy( rts->nets[id] );\

#define SMX_TF_RUN( id )\
    smx_net_run( rts->ths, id, start_routine_tf, rts->nets[id], SMX_TF_PRIO )

#define SMX_TF_WAIT_END( id )\
    pthread_join( rts->ths[id], NULL )

#define START_ROUTINE_NET( h, net_name, box_name )\
    start_routine_net( box_name, box_name ## _init, box_name ## _cleanup, h,\
            SMX_SIG_PORTS( h, in ),\
            SMX_SIG_PORT_COUNT( h, in ),\
            SMX_SIG_PORTS( h, out ),\
            SMX_SIG_PORT_COUNT( h, out ) )

#define SMX_NET_EXTERN( box_name )\
    extern int box_name( void*, void* );\
    extern int box_name ## _init( void*, void** );\
    extern void box_name ## _cleanup( void* )

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
