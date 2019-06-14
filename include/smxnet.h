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

#define SMX_MAX_NETS 1000

typedef struct smx_net_s smx_net_t;                   /**< ::smx_net_s */
typedef struct smx_net_sig_s smx_net_sig_t;           /**< ::smx_net_sig_s */

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
    pthread_barrier_t*  init_done;  /**< pointer to the init sync barrier */
    zlog_category_t*    cat;        /**< the log category */
    smx_channel_t*      profiler;   /**< a pointer to the profiler channel */
    smx_net_sig_t*      sig;        /**< the net port signature */
    void*               attr;       /**< custom attributes of special nets */
    void*               conf;       /**< pointer to the XML configuration */
    const char*         name;       /**< the name of the net */
};

/**
 * The signature of a net
 */
struct smx_net_sig_s
{
    struct {
        int count;                  /**< the number of connected input ports */
        int len;                    /**< the number of input ports */
        smx_channel_t** ports;      /**< an array of channel pointers */
    } in;                           /**< input channels */
    struct {
        int count;                  /**< the number of connected output ports */
        int len;                    /**< the number of output ports */
        smx_channel_t** ports;      /**< an array of channel pointers */
    } out;                          /**< output channels */
};

#define SMX_LOG_NET( net, level, format, ... )\
    SMX_LOG_INTERN( level, SMX_SIG_CAT( net ), format, ##__VA_ARGS__ )

/**
 *
 */
#define SMX_NET_GET_ID( h ) ( h == NULL ) ? -1 : ( ( smx_net_t* )h )->id

/**
 *
 */
#define SMX_NET_GET_CONF( h ) ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->conf

/**
 *
 */
#define SMX_NET_GET_NAME( h ) ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->name

/**
 * Check whether messages are available on the collector and block until a
 * message is made available or a producer terminates.
 *
 * @param h         pointer to the net handler
 * @param collector pointer to the net collector structure
 * @return          the number of currently available messages in the collector
 */
int smx_net_collector_check_avaliable( void* h, smx_collector_t* collector );

/**
 * Read from a collector of a net.
 *
 * @param h         pointer to the net handler
 * @param collector pointer to the net collector structure
 * @param in        pointer to the input port array
 * @param count_in  number of input ports
 * @param last_idx  pointer to the state variable storing the last port index
 * @return          the message that was read or NULL if no message was read
 */
smx_msg_t* smx_net_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx );

/**
 * Create a new net instance. This includes
 *  - creating a zlog category
 *  - assigning the net-specifix XML configuartion
 *  - assigning the net signature
 *
 * @param net_cnt   pointer to the net counter (is increased by one after net
 *                  creation)
 * @param id        a unique net identifier
 * @param name      the name of the net
 * @param cat_name  the name of the zlog category
 * @param conf      a pointer to the net configuration structure
 * @param init_done a pointer to the init sync barrier
 * @return          a pointer to the ctreated net or NULL
 */
smx_net_t* smx_net_create( int* net_cnt, unsigned int id, const char* name,
        const char* cat_name, void** conf, pthread_barrier_t* init_done );

/**
 * Destroy a net
 *
 * @param h         pointer to the net handler
 */
void smx_net_destroy( smx_net_t* h );

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
void smx_net_init( smx_net_t* net, int indegree, int outdegree );

/**
 * @brief create pthred of net
 *
 * @param ths               the target array to store the thread id
 * @param idx               the index of where to store the thread id in the
 *                          target array
 * @param box_impl( arg )   function pointer to the box implementation
 * @param h                 pointer to the net handler
 * @param prio              the RT thread priority (0 means no rt thread)
 * @return                  0 on success, -1 on failure
 */
int smx_net_run( pthread_t* ths, int idx, void* box_impl( void* arg ), void* h,
        int prio );

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
 * @param state     state set by the box implementation. If set to
 *                  SMX_NET_CONTINUE, the box will not terminate. If set to
 *                  SMX_NET_END, the box will terminate. If set to
 *                  SMX_NET_RETURN (or 0) this function will determine wheter
 *                  a box terminates or not
 * @return          SMX_NET_CONTINUE if there is at least one triggeringr
 *                  producer alive. SMX_BOX_TERINATE if all triggering
 *                  prodicers are terminated.
 */
int smx_net_update_state( smx_net_t* h, int state );

/**
 * @brief Set all channel states to end and send termination signal to all
 * output channels.
 *
 * @param h         pointer to the net handler
 */
void smx_net_terminate( smx_net_t* h );

/**
 * @brief the start routine of a thread associated to a box
 *
 * @param h                 pointer to the net handler
 * @param impl( arg )       pointer to the net implementation function
 * @param init( arg )       pointer to the net intitialisation function
 * @param cleanup( arg )    pointer to the net cleanup function
 * @return                  returns NULL
 */
void* start_routine_net( smx_net_t* h, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void*, void* ) );

#endif
