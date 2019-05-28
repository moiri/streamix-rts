/**
 * Net definitions for the runtime system library of Streamix
 *
 * @file    smxnet.h
 * @author  Simon Maurer
 */

#include "smxch.h"
#include "smxlog.h"

#ifndef SMXNET_H
#define SMXNET_H

typedef struct smx_net_s smx_net_t;                   /**< ::smx_net_s */

/**
 * @brief Constants to indicate wheter a thread should terminate or continue
 */
enum smx_thread_state_e
{
    SMX_NET_RETURN = 0,     /**< decide automatically wheather to end or go on */
    SMX_NET_CONTINUE,       /**< continue to call the box implementation fct */
    SMX_NET_END             /**< end thread */
};

/**
 * Common fields of a streamix net.
 */
struct smx_net_s
{
    unsigned int        id;         /**< a unique net id */
    zlog_category_t*    cat;        /**< the log category */
    void*               sig;        /**< the net port signature */
    void*               conf;       /**< pointer to the XML configuration */
};

#define SMX_LOG_NET( net, level, format, ... )\
    SMX_LOG_LOCK( level, SMX_SIG_CAT( net ), format, ##__VA_ARGS__ )

/**
 *
 */
#define SMX_NET_GET_ID( h ) ( h == NULL ) ? -1 : ( ( smx_net_t* )h )->id;

/**
 *
 */
#define SMX_NET_GET_CONF( h ) ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->conf;

/**
 * Create a new net instance. This includes
 *  - creating a zlog category
 *  - assigning the net-specifix XML configuartion
 *  - assigning the net signature
 *
 * @param id        a unique net identifier
 * @param cat_name  the name of the zlog category
 * @param sig       a pointer to the net signature
 */
smx_net_t* smx_net_create( unsigned int id, const char* name,
        const char* cat_name, void* sig, void** conf );

/**
 * Destroy a net
 *
 * @param in    a pointer to the input port structure
 * @param out   a pointer to the output port structure
 * @param sig   a pointer to the net signature
 * @param net   a pointer to the net handler
 */
void smx_net_destroy( void* in, void* out, void* sig, void* h );

/**
 * Initialise a net
 *
 * @param in_cnt        pointer to the input counter
 * @param in_ports      pointer to the input ports array
 * @param in_degree     number of input ports
 * @param out_cnt       pointer to the output counter
 * @param out_ports     pointer to the output ports array
 * @param out_degree    number of output ports
 */
void smx_net_init( int* in_cnt, smx_channel_t*** in_ports, int in_degree,
        int* out_cnt, smx_channel_t*** out_ports, int out_degree );

/**
 * @brief create pthred of net
 *
 * @param box_impl( arg )   function pointer to the box implementation
 * @param h                 pointer to the net handler
 * @param prio              the RT thread priority (0 means no rt thread)
 * @return                  a pthread id
 */
pthread_t smx_net_run( void* box_impl( void* arg ), void* h, int prio );

/**
 * @brief Update the state of the box
 *
 * Update the state of the box to indicate wheter computaion needs to scontinue
 * or terminate. The state can either be forced by the box implementation (see
 * \p state) or depends on the state of the triggering producers.
 * Note that non-triggering producers may still be alive but the thread will
 * still terminate if all triggering producers are terminated. This is to
 * prevent a while(1) type of behaviour because no blocking will occur to slow
 * the thread execution.
 *
 * @param h         pointer to the net handler
 * @param chs_in    a list of input channels
 * @param len_in    number of input channels
 * @param chs_out   a list of output channels
 * @param len_out   number of output channels
 * @param state     state set by the box implementation. If set to
 *                  SMX_NET_CONTINUE, the box will not terminate. If set to
 *                  SMX_NET_END, the box will terminate. If set to
 *                  SMX_NET_RETURN (or 0) this function will determine wheter
 *                  a box terminates or not
 * @return          SMX_NET_CONTINUE if there is at least one triggeringr
 *                  producer alive. SMX_BOX_TERINATE if all triggering
 *                  prodicers are terminated.
 */
int smx_net_update_state( void* h, smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out, int state );

/**
 * @brief Set all channel states to end and send termination signal to all
 * output channels.
 *
 * @param h         pointer to the net handler
 * @param chs_in    a list of input channels
 * @param len_in    number of input channels
 * @param chs_out   a list of output channels
 * @param len_out   number of output channels
 */
void smx_net_terminate( void* h, smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out );

/**
 * @brief the start routine of a thread associated to a box
 *
 * @param impl( arg )       pointer to the net implementation function
 * @param init( arg )       pointer to the net intitialisation function
 * @param cleanup( arg )    pointer to the net cleanup function
 * @param h                 pointer to the net handler
 * @param chs_in            list of input channels
 * @param cnt_in            pointer to count of input ports
 * @param chs_out           list of output channels
 * @param cnt_out           pointer to counter of output port
 * @return                  returns NULL
 */
void* start_routine_net( int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void* ), void* h, smx_channel_t** chs_in, int* cnt_in,
        smx_channel_t** chs_out, int* cnt_out );

#endif
