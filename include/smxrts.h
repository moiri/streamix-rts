/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <pthread.h>
#include <stdlib.h>
#include <zlog.h>
#include "smxch.h"
#include "smxmsg.h"
#include "smxnet.h"
#include "smxutils.h"
#include "box_smx_rn.h"
#include "box_smx_tf.h"

#ifndef SMXRTS_H
#define SMXRTS_H

#define SMX_TF_PRIO     3

// TYPEDEFS -------------------------------------------------------------------
typedef struct smx_rts_s smx_rts_t;

// ENUMS ----------------------------------------------------------------------

// STRUCTS --------------------------------------------------------------------

struct smx_rts_s
{
    void* conf;
    smx_channel_t** chs;
    smx_net_t** nets;
    pthread_t* ths;
};

// USER MACROS -----------------------------------------------------------------
/**
 *
 */
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( SMX_SIG_PORT( h, box_name, ch_name, in ) )

/**
 *
 */
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( SMX_SIG_PORT( h, box_name, ch_name, out ), data )

/**
 *
 */
#define SMX_LOG( h, level, format, ... )\
    SMX_LOG_NET( h, level, format, ##__VA_ARGS__ )

/**
 *
 */
#define SMX_MSG_CREATE( data, dsize, fcopy, ffree, funpack )\
    smx_msg_create( data, dsize, fcopy, ffree, funpack )

/**
 *
 */
#define SMX_MSG_COPY( msg )\
    smx_msg_copy( msg )

/**
 *
 */
#define SMX_MSG_DESTROY( msg )\
    smx_msg_destroy( msg, 1 )

/**
 *
 */
#define SMX_MSG_UNPACK( msg )\
    smx_msg_unpack( msg )

// RTS MACROS ------------------------------------------------------------------
#define SMX_CHANNEL_CREATE( id, len, type, name )\
    smx_channel_t* ch_ ## id = smx_channel_create( len, type, id, #name );\
    smx_cat_add_channel( ch_ ## id, STRINGIFY( ch_ ## name ## _ ## id ) )

#define SMX_CHANNEL_DESTROY( id )\
    smx_channel_destroy( ch_ ## id )

#define SMX_CONNECT( net_id, ch_id, net_name, box_name, ch_name, mode )\
    smx_connect( SMX_SIG_PORT_PTR( net_ ## net_id, box_name, ch_name, mode ),\
            ch_ ## ch_id )

#define SMX_CONNECT_ARR( net_id, ch_id, net_name, box_name, ch_name, mode )\
    smx_connect_arr( SMX_SIG_PORTS( net_ ## net_id, box_name, mode ),\
            SMX_SIG_PORT_COUNT( net_ ## net_id, box_name, mode ),\
            ch_ ## ch_id, net_id, ch_id, #net_name, #ch_name, SMX_MODE_ ## mode )

#define SMX_CONNECT_GUARD( id, iats, iatns )\
    smx_connect_guard( ch_ ## id, smx_guard_create( iats, iatns, ch_ ## id ) )

#define SMX_CONNECT_RN( net_id, ch_id )\
    smx_connect_rn( ch_ ## ch_id, net_ ## net_id )

#define SMX_CONNECT_TF( timer_id, ch_in_id, ch_out_id, ch_name )\
    smx_tf_connect( SMX_SIG( timer_ ## timer_id ), ch_ ## ch_in_id,\
            ch_ ## ch_out_id, timer_id )

#define SMX_NET_CREATE( id, net_name, box_name )\
    smx_net_t* net_ ## id = smx_net_create( id, #net_name,\
            STRINGIFY( net_ ## net_name ## _ ## id ),\
            smx_malloc( sizeof( struct net_ ## box_name ## _s ) ), &conf )

#define SMX_NET_DESTROY( id, box_name )\
    smx_net_destroy(\
            SMX_SIG_PORTS( net_ ## id, box_name, in ),\
            SMX_SIG_PORTS( net_ ## id, box_name, out ),\
            SMX_SIG( net_ ## id ),\
            net_ ## id )

#define SMX_NET_INIT( id, box_name, indegree, outdegree )\
    smx_net_init(\
            SMX_SIG_PORT_COUNT( net_ ## id, box_name, in ),\
            SMX_SIG_PORTS_PTR( net_ ## id, box_name, in ), indegree,\
            SMX_SIG_PORT_COUNT( net_ ## id, box_name, out ),\
            SMX_SIG_PORTS_PTR( net_ ## id, box_name, out ), outdegree )

#define SMX_NET_RN_DESTROY( id )\
    smx_net_rn_destroy( ( SMX_SIG( net_ ## id ) ) )

#define SMX_NET_RN_INIT( id )\
    smx_net_rn_init( SMX_SIG( net_ ## id ) )

#define SMX_NET_RUN( id, net_name, box_name, prio )\
    pthread_t th_net_ ## id = smx_net_run( box_ ## box_name, net_ ## id, prio )

#define SMX_NET_WAIT_END( id )\
    pthread_join( th_net_ ## id, NULL )

#define SMX_PROGRAM_INIT_RUN() ;

#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup( &conf )

#define SMX_PROGRAM_INIT()\
    void* conf = NULL;\
    smx_program_init( &conf )

#define SMX_TF_CREATE( id, sec, nsec )\
    smx_net_t* timer_ ## id = smx_net_create( id, STRINGIFY( smx_tf ),\
            STRINGIFY( net_nsmx_tf ## _ ## id ), smx_tf_create( sec, nsec ),\
            &conf )

#define SMX_TF_DESTROY( id )\
    smx_tf_destroy( timer_ ## id );\

#define SMX_TF_RUN( id )\
    pthread_t th_timer_ ## id = smx_net_run( start_routine_tf, timer_ ## id,\
            SMX_TF_PRIO )

#define SMX_TF_WAIT_END( id )\
    pthread_join( th_timer_ ## id, NULL )

#define START_ROUTINE_NET( h, net_name, box_name )\
    start_routine_net( box_name, box_name ## _init, box_name ## _cleanup, h,\
            SMX_SIG_PORTS( h, box_name, in ),\
            SMX_SIG_PORT_COUNT( h, box_name, in ),\
            SMX_SIG_PORTS( h, box_name, out ),\
            SMX_SIG_PORT_COUNT( h, box_name, out ) )

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
 * @param conf   a pointer to the configurcation structure
 */
void smx_program_cleanup( void** doc );

/**
 * @brief Perfrom some initialisation tasks
 *
 * Initialize logs and log the beginning of the program
 *
 * @param conf   a pointer to the configurcation structure
 */
void smx_program_init( void** doc );


#endif // SMXRTS_H
