#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"
#include <stdlib.h>

// TYPEDEFS -------------------------------------------------------------------
typedef struct smx_channel_s smx_channel_t;
typedef struct smx_collector_s smx_collector_t;
typedef struct smx_fifo_item_s smx_fifo_item_t;
typedef struct smx_fifo_s smx_fifo_t;
typedef struct smx_guard_s smx_guard_t;
typedef struct smx_msg_s smx_msg_t;
typedef struct smx_timer_s smx_timer_t;
typedef struct box_smx_cp_s box_smx_cp_t;
typedef enum smx_channel_type_e smx_channel_type_t;
typedef enum smx_channel_state_e smx_channel_state_t;

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
    SMX_BOX_RETURN,
    SMX_BOX_CONTINUE,       /**< continue to call the box implementation fct */
    SMX_BOX_TERMINATE       /**< end thread */
};

// STRUCTS --------------------------------------------------------------------
/**
 * @brief A generic Streamix channel
 */
struct smx_channel_s
{
    smx_channel_type_t  type;       /**< #smx_channel_type_e */
    const char*         name;       /**< name of the channel */
    smx_fifo_t*         fifo;       /**< ::smx_fifo_s */
    smx_guard_t*        guard;      /**< ::smx_guard_s */
    smx_collector_t*    collector;  /**< ::smx_collector_s, collect signals */
    smx_channel_state_t state;      /**< ::smx_channel_state_e */
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
    smx_channel_state_t state;      /**< ::smx_channel_state_e */
};

/**
 * @brief Streamix fifo structure
 *
 * The fifo structure is blocking on write if all buffers are occupied and
 * blocking on read if all buffer spaces are empty. The blocking pattern
 * can be changed by decoupling either the input, the output or both.
 * See ::smx_channel_type_e
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
    int             fd;     /**> file descriptor pointing to timer */
    struct timespec iat;    /**> minumum inter-arrival-time */
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
};

/**
 *
 */
struct smx_timer_s
{
    int                 fd;         /**< timer file descriptor */
    pthread_mutex_t     mutex;      /**< mutual exclusion */
    struct itimerspec   itval;      /**< iteration specifiaction */
};

/**
 * @brief The signature of a copy synchronizer
 */
struct box_smx_cp_s
{
    struct {
        smx_channel_t** ports;      /**< an array of channel pointers */
        int count;                  /**< the number of input ports */
        smx_collector_t* collector; /**< ::smx_collector_s */
    } in;
    struct {
        smx_channel_t** ports;      /**< an array of channel pointers */
        int count;                  /**< the number of output ports */
    } out;
    smx_timer_t*    timer;          /**< timer structure for tt*/
};

// FUNCTIONS BOX --------------------------------------------------------------
/*****************************************************************************/
/*****************************************************************************/
#define SMX_BOX_CP_DESTROY( box )\
    smx_box_cp_destroy( ( box_smx_cp_t* )box )

/**
 * @brief Destroy copy sync structure
 *
 * @param box_smx_cp_t*     pointer to the cp sync structure
 */
void smx_box_cp_destroy( box_smx_cp_t* );

/*****************************************************************************/
#define smx_cp( h ) smx_box_cp_impl( h )
/**
 * @brief the box implementattion of a copy synchronizer
 *
 * A copy synchronizer reads from any port where data is available and copies
 * it to every output. The read order is first come first serve with peaking
 * wheter data is available. The cp sync is only blocking on read if no input
 * channel has data available. The copied data is written to the output channel
 * in order how they appear in the list. Writing is blocking. All outputs must
 * be written before new input is accepted.
 *
 * @param void*     a pointer to the signature ::box_smx_cp_s
 * @return int      ::smx_thread_state_e
 */
int smx_box_cp_impl( void* );

/*****************************************************************************/
#define SMX_BOX_CP_INIT( box )\
    smx_box_cp_init( ( box_smx_cp_t* )box )

/**
 * @brief Initialize copy synchronizer structure
 *
 * @param box_smx_cp_t*     pointer to the copy sync structure
 */
void smx_box_cp_init( box_smx_cp_t* );

/*****************************************************************************/
#define SMX_BOX_CREATE( box )\
    malloc( sizeof( struct box_##box##_s ) )

/*****************************************************************************/
#define SMX_BOX_DESTROY( box_name, box )\
    free( ( ( box_ ## box_name ## _t* )box )->in.ports );\
    free( ( ( box_ ## box_name ## _t* )box )->out.ports );\
    free( box )

/*****************************************************************************/
#define SMX_BOX_ENABLE( h, box_name )\
    smx_box_start( #box_name );\
    smx_tt_enable( ( ( box_ ## box_name ## _t* )h )->timer )

/*****************************************************************************/
#define SMX_BOX_INIT( box_name, box, indegree, outdegree )\
    ( ( box_ ## box_name ## _t* )box )->in.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->in.ports\
        = malloc( sizeof( smx_channel_t* ) * indegree );\
    ( ( box_ ## box_name ## _t* )box )->out.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->out.ports\
        = malloc( sizeof( smx_channel_t* ) * outdegree );\
    ( ( box_ ## box_name ## _t* )box )->timer = NULL

/*****************************************************************************/
#define SMX_BOX_RUN( arg, box_name )\
    pthread_t th_ ## arg = smx_box_run( box_ ## box_name, arg )

/**
 * @brief create pthred of box
 *
 * @param void*( void* ):   function pointer to the box implementation
 * @param void*:            pointer to the box handler
 * @return pthread_t:       a pthreadi id
 */
pthread_t smx_box_run( void*( void* ), void* );

/*****************************************************************************/
#define SMX_BOX_UPDATE_STATE( h, box_name, state )\
    smx_box_update_state( ( ( box_ ## box_name ## _t* )h )->in.ports,\
            ( ( box_ ## box_name ## _t* )h )->in.count, state )

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
 * @param smx_channel_t**   a list of output channels
 * @param int               number of output channels
 * @param int               state set by the box implementation. If set to
 *                          SMX_BOX_CONTINUE, the box will not terminate. If
 *                          set to SMX_BOX_TERMINATE, the box will terminate.
 *                          If set to SMX_BOX_RETURN (or 0) this function will
 *                          determine wheter a box terminates or not
 * @param int               SMX_BOX_CONTINUE if there is at least one
 *                          triggering producer alive.
 *                          SMX_BOX_TERINATE if all triggering prodicers are
 *                          terminated.
 */
int smx_box_update_state( smx_channel_t**, int, int );

/*****************************************************************************/
#define SMX_BOX_TERMINATE( h, box_name )\
    smx_channels_terminate( ( ( box_ ## box_name ## _t* )h )->out.ports,\
            ( ( box_ ## box_name ## _t* )h )->out.count );\
    smx_box_terminate( #box_name )

/**
 *
 */
void smx_box_start( const char* );

/**
 *
 */
void smx_box_terminate( const char* );

/*****************************************************************************/
#define SMX_BOX_WAIT( h, box_name )\
    smx_tt_wait( ( ( box_ ## box_name ## _t* )h )->timer )

/**
 * @brief blocking wait on timer
 *
 * @param smx_timer_t*  pointer to a timer structure
 */
void smx_tt_wait( smx_timer_t* );

/*****************************************************************************/
#define SMX_BOX_WAIT_END( box_name )\
    pthread_join( th_ ## box_name, NULL )

/**
 * @brief enable periodic tt timer
 *
 * @param smx_timer_t*  pointer to a timer structure
 */
void smx_tt_enable( smx_timer_t* );

// FUNCTIONS CHANNEL-----------------------------------------------------------
/*****************************************************************************/
#define SMX_HAS_PRODUCER_TERMINATED( h, box_name, ch_name )\
    ( ( box_ ## box_name ## _t* )h )->in.port_ ## ch_name->state == SMX_CHANNEL_END

/**
 * @brief Send termination signal to all output channels
 *
 * @param smx_channel_t**   a list of output channels
 * @param int               number of output channels
 */
void smx_channels_terminate( smx_channel_t**, int );

/*****************************************************************************/
#define SMX_CHANNEL_CREATE( len, type )\
    smx_channel_create( len, type )

/**
 * @brief Create Streamix channel
 *
 * @param int                   length of a FIFO
 * @param smx_channel_type_t    type of the buffer, #smx_channel_type_e
 * @return smx_channel_t*       pointer to the created channel
 */
smx_channel_t* smx_channel_create( int, smx_channel_type_t );

/*****************************************************************************/
#define SMX_CHANNEL_DESTROY( ch )\
    smx_channel_destroy( ch )

/**
 * @brief Destroy Streamix channel structure
 *
 * @param smx_channel_t*    pointer to the channel to destroy
 */
void smx_channel_destroy( smx_channel_t* );

/**
 * @brief Returns the number of available messages in channel
 *
 * @param smx_channel_t*    pointer to the channel
 * @return int              number of available messages in channel
 */
int smx_channel_ready_to_read( smx_channel_t* );

/*****************************************************************************/
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( ( ( box_ ## box_name ## _t* )h )->in.port_ ## ch_name )

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_READ( h, box, port ) provides a
 * convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @return smx_msg_t*       pointer to the data structure
 */
smx_msg_t* smx_channel_read( smx_channel_t* );

/*****************************************************************************/
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( ( ( box_ ## box_name ## _t* )h )->out.port_ ## ch_name,\
            data )

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_WRITE( h, box, port, data ) provides
 * a convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @param smx_msg_t*        pointer to the data structure
 */
void smx_channel_write( smx_channel_t*, smx_msg_t* );

/*****************************************************************************/
#define SMX_CONNECT( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.port_ ## ch_name\
        = ( smx_channel_t* )ch;\
    ( ( box_ ## box_name ## _t* )box )->mode.port_ ## ch_name->name = #ch_name

/*****************************************************************************/
#define SMX_CONNECT_ARR( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.ports[\
        ( ( box_ ## box_name ## _t* )box )->mode.count++] = ( smx_channel_t* )ch

/*****************************************************************************/
#define SMX_CONNECT_CP( box, ch )\
    ( ( smx_channel_t* )ch )->collector\
        = ( ( box_smx_cp_t* )box )->in.collector;\

/*****************************************************************************/
#define SMX_CONNECT_GUARD( ch, iats, iatns )\
    ( ( smx_channel_t* )ch )->guard = smx_guard_create( iats, iatns )

/*****************************************************************************/
#define SMX_CONNECT_TT( box, box_name, sec, nsec )\
    ( ( box_ ## box_name ## _t* )box )->timer = smx_tt_create( sec, nsec )

/**
 * @brief create a periodic timer structure
 *
 * @param int           seconds
 * @param int           nano seconds
 * @return smx_timer_t* pointer to the created timer structure
 */
smx_timer_t* smx_tt_create( int, int );

// FUNCTIONS FIFO--------------------------------------------------------------
/**
 * @brief Create Streamix FIFO channel
 *
 * @param int           length of the FIFO
 * @return smx_fifo_t*  pointer to the created FIFO
 */
smx_fifo_t* smx_fifo_create( int );

/**
 * @brief Destroy Streamix FIFO channel structure
 *
 * @param smx_fifo_t*   pointer to the channel to destroy
 */
void smx_fifo_destroy( smx_fifo_t* );

/**
 * @brief read from a Streamix FIFO channel
 *
 * @param smx_channel_t*    pointer to channel struct of the FIFO
 * @param smx_fifo_t*       pointer to a FIFO channel
 * @return smx_msg_t*       pointer to the data
 */
smx_msg_t* smx_fifo_read( smx_channel_t*, smx_fifo_t* );

/**
 * @brief read from a Streamix FIFO_D channel
 *
 * Read from a channel that is decoupled at the output (the consumer is
 * decoupled at the input). This means that the msg at the head of the FIFO_D
 * will potentially be duplicated. However, the consumer is blocked on this
 * channel until a first message is available.
 *
 * @param smx_channel_t*    pointer to channel struct of the FIFO
 * @param smx_fifo_t*       pointer to a FIFO_D channel
 * @return smx_msg_t*       pointer to the data
 */
smx_msg_t* smx_fifo_d_read( smx_channel_t*, smx_fifo_t* );

/**
 * @brief write to a Streamix FIFO channel
 *
 * @param smx_channel_t*    pointer to channel struct of the FIFO
 * @param smx_fifo_t*       pointer to a FIFO channel
 * @param smx_msg_t*        pointer to the data
 */
void smx_fifo_write( smx_channel_t*, smx_fifo_t*, smx_msg_t* );

/**
 * @brief write to a Streamix D_FIFO channel
 *
 * Write to a channel that is decoupled at the input (the produced is decoupled
 * at the output). This means that the tail of the D_FIFO will potentially be
 * overwritten.
 *
 * @param smx_channel_t*    pointer to channel struct of the FIFO
 * @param smx_fifo_t*       pointer to a D_FIFO channel
 * @param smx_msg_t*        pointer to the data
 */
void smx_d_fifo_write( smx_channel_t*, smx_fifo_t*, smx_msg_t* );

/**
 * @brief create timed guard structure and initialise timer
 *
 * @param int           miart in seconds
 * @param int           miart in nano seconds
 * @return smx_guart_t* pointer to the created guard structure
 */
smx_guard_t* smx_guard_create( int, int );

/**
 * @brief imposes a rate-controld on write operations
 *
 * A producer is blocked until the minimum inter-arrival-time between two
 * consecutive messges has passed
 *
 * @param smx_guart_t*  pointer to the guard structure
 */
void smx_guard_write( smx_guard_t* );

/**
 * @brief imposes a rate-control on decoupled write operations
 *
 * A message is discarded if it did not reach the specified minimal inter-
 * arrival time (messages are not buffered and delayed, it's only a very simple
 * implementation)
 *
 * @param smx_guart_t*  pointer to the guard structure
 * @param smx_guart_t*  pointer to the message structure
 *
 * @return int          -1 if message was discarded, 0 otherwise
 */
int smx_d_guard_write( smx_guard_t*, smx_msg_t* );

// FUNCTIONS MSGS--------------------------------------------------------------
/**
 * @brief Create a message structure
 *
 * Allows to create a message structure and attach handlers to modify the data
 * in the message structure. If defined, the init function handler is called
 * after the message structure is created.
 *
 * @param void*             a pointer to the data to be added to the message
 * @param size_t            the size of the data
 * @param void*( void*,     a pointer to a function perfroming a deep copy of
 *              size_t )    the data in the message structure. The function
 *                          takes a void pointer as an argument that points to
 *                          the data structure to copy and the size of the data
 *                          structure. The function must return a void pointer
 *                          to the copied data structure.
 * @param void( void* )     a pointer to a function freeing the memory of the
 *                          data in the message structure. The function takes a
 *                          void pointer as an argument that points to the data
 *                          structure to free.
 * @return smx_msg_t*       a pointer to the created message structure
 */
smx_msg_t* smx_msg_create( void*, size_t, void*( void*, size_t ),
        void( void* ) );

/**
 * @brief make a deep copy of a message
 *
 * @param smx_msg_t*    pointer to the message structure to copy
 * @return smx_msg_t*   pointer to the newly created message structure
 */
smx_msg_t* smx_msg_copy( smx_msg_t* );

/**
 * @brief Default copy function to perform a shallow copy of the message data
 *
 * @param void*     a void pointer to the data structure
 * @param size_t    the size of the data
 * @return void*    a void pointer to the data
 */
void* smx_msg_data_copy( void*, size_t );

/**
 * @brief Default destroy function to destroy the data inside a message
 *
 * @param void*     a void pointer to the data to be freed (shallow)
 */
void smx_msg_data_destroy( void* );

/*****************************************************************************/
#define SMX_MSG_DESTROY( msg )\
    smx_msg_destroy( msg, 1 )
/**
 * @brief Destroy a message structure
 *
 * Allows to destroy a message structure. If defined (see smx_msg_create()), the
 * destroy function handler is called before the message structure is freed.
 *
 * @param smx_msg_t*    a pointer to the message structure to be destroyed
 * @param int           a flag to indicate whether the data shoudl be deleted as
 *                      well if msg->destroy() is NULL this flag is ignored
 */
void smx_msg_destroy( smx_msg_t*, int );

// FUNCTIONS PROGRAM ----------------------------------------------------------
/*****************************************************************************/
#define SMX_PROGRAM_INIT()\
    smx_program_init()

/**
 * @brief Perfrom some initialisation tasks
 *
 * Initialize logs and log the beginning of the program
 */
void smx_program_init();

/*****************************************************************************/
#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup()

/**
 * @brief Perfrom some cleanup tasks
 *
 * Close the log file
 */
void smx_program_cleanup();


#endif // HANDLER_H
