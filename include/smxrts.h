/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include "pthread.h"
#include <stdlib.h>
#include <zlog.h>

#ifndef HANDLER_H
#define HANDLER_H

#define XML_PATH        "app.xml"
#define XML_APP         "app"
#define XML_LOG         "log"
#define XML_LOG_CONF    "conf"
#define XML_LOG_CAT     "cat"

// TYPEDEFS -------------------------------------------------------------------
typedef struct box_smx_rn_s box_smx_rn_t;             /**< ::box_smx_rn_s */
typedef struct box_smx_tf_s box_smx_tf_t;             /**< ::box_smx_tf_s */
typedef struct smx_channel_s smx_channel_t;           /**< ::smx_channel_s */
typedef struct smx_collector_s smx_collector_t;       /**< ::smx_collector_s */
typedef struct smx_fifo_s smx_fifo_t;                 /**< ::smx_fifo_s */
typedef struct smx_fifo_item_s smx_fifo_item_t;       /**< ::smx_fifo_item_s */
typedef struct smx_guard_s smx_guard_t;               /**< ::smx_guard_s */
typedef struct smx_msg_s smx_msg_t;                   /**< ::smx_msg_s */
typedef struct smx_timer_s smx_timer_t;               /**< ::smx_timer_s */
typedef enum smx_channel_type_e smx_channel_type_t;   /**< #smx_channel_type_e */
typedef enum smx_channel_state_e smx_channel_state_t; /**< #smx_channel_state_e */

// ENUMS ----------------------------------------------------------------------
/**
 * @brief Streamix channel (buffer) types
 */
enum smx_channel_type_e
{
    SMX_FIFO,           /**< a simple FIFO */
    SMX_FIFO_D,         /**< a FIFO with decoupled output */
    SMX_D_FIFO,         /**< a FIFO with decoupled input */
    SMX_D_FIFO_D,       /**< a FIFO with decoupled input and output */
};

/**
 * @brief Channel state
 *
 * This allows to indicate wheter a producer connected to the channel has
 * terminated and wheter data is available to read. The second point is
 * important in combination with copy synchronizers.
 */
enum smx_channel_state_e
{
    SMX_CHANNEL_READY,      /**< channel is ready to read from */
    SMX_CHANNEL_PENDING,    /**< channel is waiting for a signal */
    SMX_CHANNEL_END         /**< producer connected to channel has terminated */
};

/**
 * @brief Constants to indicate wheter a thread should terminate or continue
 */
enum smx_thread_state_e
{
    SMX_NET_RETURN = 0,     /**< decide automatically wheather to end or go on */
    SMX_NET_CONTINUE,       /**< continue to call the box implementation fct */
    SMX_NET_END             /**< end thread */
};

// STRUCTS --------------------------------------------------------------------
/**
 * @brief A generic Streamix channel
 */
struct smx_channel_s
{
    smx_channel_type_t  type;       /**< type of the channel */
    const char*         name;       /**< name of the channel */
    smx_fifo_t*         fifo;       /**< ::smx_fifo_s */
    smx_guard_t*        guard;      /**< ::smx_guard_s */
    smx_collector_t*    collector;  /**< ::smx_collector_s, collect signals */
    smx_channel_state_t state;      /**< state of the channel */
    pthread_mutex_t     ch_mutex;   /**< mutual exclusion */
    pthread_cond_t      ch_cv;      /**< conditional variable to trigger box */
};

/**
 * @brief Collect channel counts
 *
 * This is used to nondeterministically merge channels with a copy synchronyzer
 * that has multiple inputs.
 */
struct smx_collector_s
{
    pthread_mutex_t     col_mutex;  /**< mutual exclusion */
    pthread_cond_t      col_cv;     /**< conditional variable to trigger box */
    int                 count;      /**< collection of channel counts */
    smx_channel_state_t state;      /**< state of the channel */
};

/**
 * @brief Streamix fifo structure
 *
 * The fifo structure is blocking on write if all buffers are occupied and
 * blocking on read if all buffer spaces are empty. The blocking pattern
 * can be changed by decoupling either the input, the output or both.
 */
struct smx_fifo_s
{
    smx_fifo_item_t*  head;      /**< pointer to the heda of the FIFO */
    smx_fifo_item_t*  tail;      /**< pointer to the tail of the FIFO */
    smx_msg_t*        backup;    /**< ::smx_msg_s, msg space for decoupling */
    int     count;               /**< counts occupied space */
    int     length;              /**< size of the FIFO */
    pthread_mutex_t fifo_mutex;  /**< mutual exclusion */
    pthread_cond_t  fifo_cv;     /**< conditional variable to trigger box */
};

/**
 * @brief A single FIFO item of a circular double-linked-list
 */
struct smx_fifo_item_s
{
    smx_msg_t*       msg;        /**< ::smx_msg_s */
    smx_fifo_item_t* next;       /**< pointer to the next item */
    smx_fifo_item_t* prev;       /**< pointer to the previous item */
};

/**
 * @brief timed guard to limit communication rate
 */
struct smx_guard_s
{
    int             fd;     /**< file descriptor pointing to timer */
    struct timespec iat;    /**< minumum inter-arrival-time */
};

/**
 * @brief A Streamix port structure
 *
 * The structure contains handlers that can be used to manipulate data.
 * This handler is provided by the box implementation.
 */
struct smx_msg_s
{
    void* data;                     /**< pointer to the data */
    int   size;                     /**< size of the data */
    void* (*copy)( void*, size_t ); /**< pointer to a fct making a deep copy */
    void  (*destroy)( void* );      /**< pointer to a fct that frees data */
    void* (*unpack)( void* );       /**< pointer to a fct that unpacks data */
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
    box_smx_tf_t*       ports;      /**< list of temporal firewalls */
    int                 count;      /**< number of port pairs */
};

/**
 * @brief The signature of a copy synchronizer
 */
struct box_smx_rn_s
{
    struct {
        smx_channel_t** ports;      /**< an array of channel pointers */
        int count;                  /**< the number of input ports */
        smx_collector_t* collector; /**< ::smx_collector_s */
    } in;                           /**< input channels */
    struct {
        smx_channel_t** ports;      /**< an array of channel pointers */
        int count;                  /**< the number of output ports */
    } out;                          /**< output channels */
    smx_timer_t*        timer;      /**< timer structure for tt */
};

/**
 * @brief The signature of a temporal firewall
 */
struct box_smx_tf_s
{
    smx_channel_t*      in;         /**< input channel */
    smx_channel_t*      out;        /**< output channel */
    box_smx_tf_t*       next;       /**< pointer to the next element */
};


// MACROS ---------------------------------------------------------------------
#define SMX_CHANNEL_CREATE( len, type, name )\
    smx_channel_create( len, type, #name )

#define SMX_CHANNEL_DESTROY( ch )\
    smx_channel_destroy( ch )

#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( ( ( box_ ## box_name ## _t* )h )->in.port_ ## ch_name )

#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( ( ( box_ ## box_name ## _t* )h )->out.port_ ## ch_name,\
            data )

#define SMX_CONNECT( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.port_ ## ch_name\
        = ( smx_channel_t* )ch

#define SMX_CONNECT_ARR( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.ports[\
        ( ( box_ ## box_name ## _t* )box )->mode.count++] = ( smx_channel_t* )ch

#define SMX_CONNECT_GUARD( ch, iats, iatns )\
    ( ( smx_channel_t* )ch )->guard = smx_guard_create( iats, iatns )

#define SMX_CONNECT_RN( box, ch )\
    ( ( smx_channel_t* )ch )->collector\
        = ( ( box_smx_rn_t* )box )->in.collector;\

#define SMX_CONNECT_TF( timer, ch_in, ch_out )\
    smx_tf_connect( timer, ch_in, ch_out )

#define SMX_HAS_PRODUCER_TERMINATED( h, box_name, ch_name )\
    ( ( box_ ## box_name ## _t* )h )->in.port_ ## ch_name->state == SMX_CHANNEL_END

#define SMX_MSG_CREATE( data, dsize, fcopy, ffree, funpack )\
    smx_msg_create( data, dsize, fcopy, ffree, funpack )

#define SMX_MSG_DESTROY( msg )\
    smx_msg_destroy( msg, 1 )

#define SMX_MSG_UNPACK( msg )\
    smx_msg_unpack( msg )

#define SMX_NET_CREATE( box )\
    malloc( sizeof( struct box_##box##_s ) )

#define SMX_NET_DESTROY( box_name, box )\
    free( ( ( box_ ## box_name ## _t* )box )->in.ports );\
    free( ( ( box_ ## box_name ## _t* )box )->out.ports );\
    free( box )

#define SMX_NET_INIT( box_name, box, indegree, outdegree )\
    ( ( box_ ## box_name ## _t* )box )->in.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->in.ports\
        = malloc( sizeof( smx_channel_t* ) * indegree );\
    ( ( box_ ## box_name ## _t* )box )->out.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->out.ports\
        = malloc( sizeof( smx_channel_t* ) * outdegree );\
    ( ( box_ ## box_name ## _t* )box )->timer = NULL

#define SMX_NET_RUN( arg, box_name )\
    pthread_t th_ ## arg = smx_net_run( box_ ## box_name, arg )

#define SMX_NET_RN_DESTROY( box )\
    smx_net_rn_destroy( ( box_smx_rn_t* )box )

#define SMX_NET_RN_INIT( box )\
    smx_net_rn_init( ( box_smx_rn_t* )box )

#define SMX_NET_WAIT_END( box_name )\
    pthread_join( th_ ## box_name, NULL )

#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup()

#define SMX_PROGRAM_INIT( net_count )\
    smx_program_init()

#define SMX_TF_CREATE( sec, nsec )\
    smx_tf_create( sec, nsec )

#define SMX_TF_DESTROY( tt )\
    smx_tf_destroy( tt )

#define SMX_TF_RUN( timer )\
    pthread_t th_ ## timer = smx_net_run( start_routine_tf, timer )

#define SMX_TF_WAIT_END( timer )\
    pthread_join( th_ ## timer, NULL )

#define START_ROUTINE_NET( h, box_name )\
    start_routine_net( #box_name, box_name, box_name ## _init, box_name ## _cleanup, h,\
            ( ( box_ ## box_name ## _t* )h )->in.ports,\
            ( ( box_ ## box_name ## _t* )h )->in.count,\
            ( ( box_ ## box_name ## _t* )h )->out.ports,\
            ( ( box_ ## box_name ## _t* )h )->out.count )

#define SMX_NET_EXTERN( box_name )\
    extern int box_name( void*, void* );\
    extern int box_name ## _init( void*, void** );\
    extern void box_name ## _cleanup( void* )

// FUNCTIONS ------------------------------------------------------------------
/**
 * @brief Create Streamix channel
 *
 * @param len   length of a FIFO
 * @param type  type of the buffer
 * @param name  name of the channel
 * @return      pointer to the created channel
 */
smx_channel_t* smx_channel_create( int len, smx_channel_type_t type,
        const char* name );

/**
 * @brief Destroy Streamix channel structure
 *
 * @param ch    pointer to the channel to destroy
 */
void smx_channel_destroy( smx_channel_t* ch );

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_READ( h, box, port ) provides a
 * convenient interface to access the ports by name.
 *
 * @param ch    pointer to the channel
 * @return      pointer to a message structure ::smx_msg_s
 */
smx_msg_t* smx_channel_read( smx_channel_t* ch );

/**
 * @brief Returns the number of available messages in channel
 *
 * @param ch    pointer to the channel
 * @return      number of available messages in channel
 */
int smx_channel_ready_to_read( smx_channel_t* ch );

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_WRITE( h, box, port, data ) provides
 * a convenient interface to access the ports by name.
 *
 * @param ch    pointer to the channel
 * @param msg   pointer to the a message structure
 * @return      1 if message was overwritten, 0 otherwise
 */
int smx_channel_write( smx_channel_t* ch, smx_msg_t* msg );

/**
 * @brief Create Streamix FIFO channel
 *
 * @param length    length of the FIFO
 * @return          pointer to the created FIFO
 */
smx_fifo_t* smx_fifo_create( int length );

/**
 * @brief Destroy Streamix FIFO channel structure
 *
 * @param fifo  pointer to the channel to destroy
 */
void smx_fifo_destroy( smx_fifo_t* fifo );

/**
 * @brief read from a Streamix FIFO channel
 *
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO channel
 * @return      pointer to a message structure
 */
smx_msg_t* smx_fifo_read( smx_channel_t* ch, smx_fifo_t* fifo );

/**
 * @brief read from a Streamix FIFO_D channel
 *
 * Read from a channel that is decoupled at the output (the consumer is
 * decoupled at the input). This means that the msg at the head of the FIFO_D
 * will potentially be duplicated. However, the consumer is blocked on this
 * channel until a first message is available.
 *
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO_D channel
 * @return      pointer to a message structure
 */
smx_msg_t* smx_fifo_d_read( smx_channel_t* ch , smx_fifo_t* fifo );

/**
 * @brief write to a Streamix FIFO channel
 *
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO channel
 * @param msg   pointer to the data
 */
void smx_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg );

/**
 * @brief write to a Streamix D_FIFO channel
 *
 * Write to a channel that is decoupled at the input (the produced is decoupled
 * at the output). This means that the tail of the D_FIFO will potentially be
 * overwritten.
 *
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a D_FIFO channel
 * @param msg   pointer to the data
 * @return      1 if message was overwritten, 0 otherwise
 */
int smx_d_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg );

/**
 * @brief create timed guard structure and initialise timer
 *
 * @param iats  minimal inter-arrival time in seconds
 * @param iatns minimal inter-arrival time in nano seconds
 * @return      pointer to the created guard structure
 */
smx_guard_t* smx_guard_create( int iats, int iatns );

/**
 * @brief destroy the guard structure
 *
 * @param guard     pointer to the guard structure
 */
void smx_guard_destroy( smx_guard_t* guard );

/**
 * @brief imposes a rate-controld on write operations
 *
 * A producer is blocked until the minimum inter-arrival-time between two
 * consecutive messges has passed
 *
 * @param guard pointer to the guard structure
 */
void smx_guard_write( smx_guard_t* guard );

/**
 * @brief imposes a rate-control on decoupled write operations
 *
 * A message is discarded if it did not reach the specified minimal inter-
 * arrival time (messages are not buffered and delayed, it's only a very simple
 * implementation)
 *
 * @param guard pointer to the guard structure
 * @param msg   pointer to the message structure
 *
 * @return      -1 if message was discarded, 0 otherwise
 */
int smx_d_guard_write( smx_guard_t* guard, smx_msg_t* msg );

/**
 * @brief make a deep copy of a message
 *
 * @param msg   pointer to the message structure to copy
 * @return      pointer to the newly created message structure
 */
smx_msg_t* smx_msg_copy( smx_msg_t* msg );

/**
 * @brief Create a message structure
 *
 * Allows to create a message structure and attach handlers to modify the data
 * in the message structure. If defined, the init function handler is called
 * after the message structure is created.
 *
 * @param data              a pointer to the data to be added to the message
 * @param size              the size of the data
 * @param copy( data,size ) a pointer to a function perfroming a deep copy of
 *                          the data in the message structure. The function
 *                          takes a void pointer as an argument that points to
 *                          the data structure to copy and the size of the data
 *                          structure. The function must return a void pointer
 *                          to the copied data structure.
 * @param destroy( data )   a pointer to a function freeing the memory of the
 *                          data in the message structure. The function takes a
 *                          void pointer as an argument that points to the data
 *                          structure to free.
 * @param unpack( data )    a pointer to a function that unpacks the message
 *                          data. The function takes a void pointer as an
 *                          argument that points to the message payload and
 *                          returns a void pointer that points to the unpacked
 *                          message payload.
 * @return                  a pointer to the created message structure
 */
smx_msg_t* smx_msg_create( void* data, size_t size, void* copy( void*, size_t ),
        void destroy( void* ), void* unpack( void* ) );

/**
 * @brief Default copy function to perform a shallow copy of the message data
 *
 * @param data      a void pointer to the data structure
 * @param size      the size of the data
 * @return          a void pointer to the data
 */
void* smx_msg_data_copy( void* data, size_t size );

/**
 * @brief Default destroy function to destroy the data inside a message
 *
 * @param data  a void pointer to the data to be freed (shallow)
 */
void smx_msg_data_destroy( void* data );

/**
 * @brief Default unpack function for the message payload
 *
 * @param data  a void pointer to the message payload.
 * @return      a void pointer to the unpacked message payload.
 */
void* smx_msg_data_unpack( void* data );

/**
 * @brief Destroy a message structure
 *
 * Allows to destroy a message structure. If defined (see smx_msg_create()), the
 * destroy function handler is called before the message structure is freed.
 *
 * @param msg   a pointer to the message structure to be destroyed
 * @param deep  a flag to indicate whether the data shoudl be deleted as well
 *              if msg->destroy() is NULL this flag is ignored
 */
void smx_msg_destroy( smx_msg_t* msg, int deep );

/**
 * @brief Unpack the message payload
 *
 * @param msg   a pointer to the message structure to be destroyed
 * @return      a void pointer to the payload
 */
void* smx_msg_unpack( smx_msg_t* msg );

/**
 * @brief is executed once, at the beginning of a thread start up
 *
 * @param name  name of the box
 */
void smx_net_log_start( const char* name );

/**
 * @brief is executed once befor the thread of the box terminates
 *
 * @param name  name of the box
 */
void smx_net_log_terminate( const char* name );

/**
 * @brief Destroy copy sync structure
 *
 * @param cp    pointer to the cp sync structure
 */
void smx_net_rn_destroy( box_smx_rn_t* cp );

/**
 * @brief Initialize copy synchronizer structure
 *
 * @param cp    pointer to the copy sync structure
 */
void smx_net_rn_init( box_smx_rn_t* cp );

/**
 * @brief create pthred of box
 *
 * @param box_impl( arg )   function pointer to the box implementation
 * @param arg               pointer to the box handler
 * @return                  a pthread id
 */
pthread_t smx_net_run( void* box_impl( void* arg ), void* arg );

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
 * @param chs   a list of output channels
 * @param len   number of output channels
 * @param state state set by the box implementation. If set to SMX_NET_CONTINUE,
 *              the box will not terminate. If set to SMX_NET_END, the box
 *              will terminate. If set to SMX_NET_RETURN (or 0) this function
 *              will determine wheter a box terminates or not
 * @return      SMX_NET_CONTINUE if there is at least one triggering producer
 *              alive.
 *              SMX_BOX_TERINATE if all triggering prodicers are terminated.
 */
int smx_net_update_state( smx_channel_t** chs, int len, int state );

/**
 * @brief Send termination signal to all output channels
 *
 * @param chs       a list of output channels
 * @param length    number of output channels
 */
void smx_net_terminate( smx_channel_t** chs, int length );

/**
 * @brief Perfrom some cleanup tasks
 *
 * Close the log file
 */
void smx_program_cleanup();

/**
 * @brief Perfrom some initialisation tasks
 *
 * Initialize logs and log the beginning of the program
 */
void smx_program_init();

/**
 * @brief the box implementattion of a routing node (former known as copy sync)
 *
 * A copy synchronizer reads from any port where data is available and copies
 * it to every output. The read order is first come first serve with peaking
 * wheter data is available. The cp sync is only blocking on read if no input
 * channel has data available. The copied data is written to the output channel
 * in order how they appear in the list. Writing is blocking. All outputs must
 * be written before new input is accepted.
 *
 * @param handler   a pointer to the signature
 * @return          returns the state of the box
 */
int smx_rn( void* handler, void* state );
int smx_rn_init( void* handler, void** state );
void smx_rn_cleanup( void* state );

/**
 * @brief grow the list of temporal firewalls and connect channels
 *
 * @param timer     pointer to a timer structure
 * @param ch_in     input channel to the temporal firewall
 * @param ch_out    output channel from the temporal firewall
 */
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out );

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
void smx_tf_destroy( smx_timer_t* tt );

/**
 * @brief enable periodic tt timer
 *
 * @param timer pointer to a timer structure
 */
void smx_tf_enable( smx_timer_t* timer );

/**
 * @brief read all input channels of a temporal firewall
 *
 * @param msg   a pointer to an initilaized message array. Here, the input
 *              messages will be stored. The array must be initialized to a
 *              length that matches the number of input channels to the
 *              temporal firewall
 * @param tt    a pointer to the timer
 * @param ch_in a pointer to an array of input channels
 * @return      a counter indicating how many channels have terminated
 */
int smx_tf_read_inputs( smx_msg_t**, smx_timer_t*, smx_channel_t** );

/**
 * @brief blocking wait on timer
 *
 * Waits on the specified time interval. An error message is printed if the
 * deadline was missed.
 *
 * @param timer pointer to a timer structure
 */
void smx_tf_wait( smx_timer_t* timer );

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
 * @brief the start routine of a thread associated to a box
 *
 * @param name              name of the box
 * @param impl( arg )       pointer to the box implementation function
 * @param init( arg )       pointer to the box intitialisation function
 * @param cleanup( arg )    pointer to the box cleanup function
 * @param h                 pointer to the box signature
 * @param chs_in            list of input channels
 * @param cnt_in            count of input ports
 * @param chs_out           list of output channels
 * @param cnt_out           counter of output port
 * @return                  returns NULL
 */
void* start_routine_net( const char* name, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void* ), void* h,
        smx_channel_t** chs_in, int cnt_in, smx_channel_t** chs_out,
        int cnt_out );

/**
 * @brief start routine for a timer thread
 *
 * @param h pointer to a timer structure
 * @return  NULL
 */
void* start_routine_tf( void* h );


#endif // HANDLER_H
