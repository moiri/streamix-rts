#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"
#include <stdlib.h>

// TYPEDEFS -------------------------------------------------------------------
typedef struct smx_fifo_item_s smx_fifo_item_t;
typedef struct smx_fifo_s smx_fifo_t;
typedef struct smx_blackboard_s smx_blackboard_t;
typedef struct smx_channel_s smx_channel_t;
typedef struct smx_msg_s smx_msg_t;
typedef struct box_smx_cp_s box_smx_cp_t;
typedef struct smx_collector_s smx_collector_t;
typedef enum smx_channel_type_e smx_channel_type_t;
typedef enum smx_channel_state_e smx_channel_state_t;

// ENUMS ----------------------------------------------------------------------
/**
 * @brief Streamix channel (buffer) types
 */
enum smx_channel_type_e
{
    SMX_FIFO,           /**< ::smx_fifo_t */
    SMX_BLACKBOARD      /**< ::smx_blackboard_t */
};

enum smx_channel_state_e
{
    SMX_CHANNEL_READY,      /**< */
    SMX_CHANNEL_PENDING,    /**< */
    SMX_CHANNEL_END         /**< */
};

enum smx_thread_state_e
{
    SMX_BOX_TERMINATE,
    SMX_BOX_CONTINUE
};

// STRUCTS --------------------------------------------------------------------
/**
 * @brief Streamix blackboard structure
 *
 * The balckboard structure allows to always write and read without blocking
 * On each write, the spaces are switches such that a read operation is always
 * performed on the newest data set
 */
struct smx_blackboard_s
{
    smx_msg_t**  msgs;            /**< array of message pointers, ::smx_msg_s */
    int     read;                 /**< read index to data array */
    int     write;                /**< write index to data array */
    pthread_mutex_t mutex_read;   /**< mutual exclusion for reding*/
    pthread_mutex_t mutex_write;  /**< mutual exclusion for writing*/
};

/**
 * @brief A generic Streamix channel
 */
struct smx_channel_s
{
    smx_channel_type_t  type;           /**< #smx_channel_type_e */
    union {
        smx_fifo_t*         ch_fifo;    /**< ::smx_fifo_s */
        smx_blackboard_t*   ch_bb;      /**< ::smx_blackboard_s */
    };
    smx_collector_t*    collector;
    smx_channel_state_t state;
    pthread_mutex_t     ch_mutex;  /**< mutual exclusion */
    pthread_cond_t      ch_cv;     /**< conditional variable to trigger box */
};

/**
 *
 */
struct smx_collector_s
{
    pthread_mutex_t     col_mutex;  /**< mutual exclusion */
    pthread_cond_t      col_cv;     /**< conditional variable to trigger box */
    int                 count;
    int                 end_count;
    smx_channel_state_t state;
};

/**
 * @brief Streamix fifo structure
 *
 * The fifo structure is blocking on write if all buffers are occupied and
 * blocking on read if all buffer spaces are empty
 */
struct smx_fifo_s
{
    smx_fifo_item_t*  head;      /**< pointer to the heda of the FIFO */
    smx_fifo_item_t*  tail;      /**< pointer to the tail of the FIFO */
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
 * @brief A Streamix port structure
 *
 * The structure contains handlers that can be used to manipulate data.
 * This handler is provided by the box implementation.
 */
struct smx_msg_s
{
    void* data;                 /**< pointer to the data */
    void* (*init)();            /**< pointer to a fct that initialiyes data */
    void* (*copy)( void* );     /**< pointer to a fct that makes a deep copy */
    void  (*destroy)( void* );  /**< pointer to a fct that frees data */
};

/**
 *
 */
struct box_smx_cp_s
{
    struct {
        smx_channel_t** ports;
        int count;
        smx_collector_t* collector;
    } in;
    struct {
        smx_channel_t** ports;
        int count;
    } out;
};

// FUNCTIONS BOX --------------------------------------------------------------
int smx_cp( void* );
void* box_smx_cp( void* );
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
#define SMX_BOX_INIT( box_name, box, indegree, outdegree )\
    ( ( box_ ## box_name ## _t* )box )->in.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->in.ports\
        = malloc( sizeof( smx_channel_t* ) * indegree );\
    ( ( box_ ## box_name ## _t* )box )->out.count = 0;\
    ( ( box_ ## box_name ## _t* )box )->out.ports\
        = malloc( sizeof( smx_channel_t* ) * outdegree )

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
#define SMX_BOX_WAIT_END( box_name )\
    pthread_join( th_ ## box_name, NULL )

/*****************************************************************************/
#define SMX_BOX_DESTROY( box_name, box )\
    free( ( ( box_ ## box_name ## _t* )box )->in.ports );\
    free( ( ( box_ ## box_name ## _t* )box )->out.ports );\
    free( box )

/*****************************************************************************/
#define SMX_BOX_CP_DESTROY( box )\
    smx_box_cp_destroy( ( box_smx_cp_t* )box )

/**
 * @brief Destroy copy sync structure
 *
 * @param box_smx_cp_t*     pointer to the cp sync structure
 */
void smx_box_cp_destroy( box_smx_cp_t* );

// FUNCTIONS CHANNEL-----------------------------------------------------------
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

/**
 * @brief Create Streamix FIFO channel
 *
 * @param int           length of the FIFO
 * @return smx_fifo_t*  pointer to the created FIFO
 */
smx_fifo_t* smx_fifo_create( int );

/**
 * @brief Create Streamix blackboard channel
 *
 * @return smx_blackboart_t*    pointer to the created blackboard
 */
smx_blackboard_t* smx_blackboard_create();

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
 * @brief Destroy Streamix FIFO channel structure
 *
 * @param smx_fifo_t*   pointer to the channel to destroy
 */
void smx_fifo_destroy( smx_fifo_t* );

/**
 * @brief Destroy Streamix blackboard channel structure
 *
 * @param smx_balckboart_t* pointer to the channel to destroy
 */
void smx_blackboard_destroy( smx_blackboard_t* );

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

/**
 * @brief Returns the number of available messages in channel
 *
 * @param smx_channel_t*    pointer to the channel
 * @return int              number of available messages in channel
 */
int smx_channel_ready_to_read( smx_channel_t* );

/**
 * @brief read from a Streamix FIFO channel
 *
 * @param smx_channel_t*    pointer to channel struct of the FIFO
 * @param smx_fifo_t*       pointer to a FIFO channel
 * @return smx_msg_t*       pointer to the data
 */
smx_msg_t* smx_fifo_read( smx_channel_t*, smx_fifo_t* );

/**
 * @brief read from a Streamix blackboard channel
 *
 * @param smx_blackboard_t* pointer to a blackboard channel
 * @return smx_msg_t*       pointer to the data
 */
smx_msg_t* smx_blackboard_read( smx_blackboard_t* );

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

/**
 * @brief write to a Streamix FIFO channel
 *
 * @param smx_fifo_t*   pointer to a FIFO channel
 * @param smx_msg_t*    pointer to the data
 */
void smx_fifo_write( smx_fifo_t*, smx_msg_t* );

/**
 * @brief write to a Streamix blackboard channel
 *
 * @param smx_blackboart_t* pointer to a blackboard channel
 * @param smx_msg_t*        pointer to the data
 */
void smx_blackboard_write( smx_blackboard_t*, smx_msg_t* );

/*****************************************************************************/
#define SMX_CHANNELS_TERMINATE( h, box_name )\
    smx_channels_terminate( ( ( box_ ## box_name ## _t* )h )->out.ports,\
            ( ( box_ ## box_name ## _t* )h )->out.count )

/**
 *
 */
void smx_channels_terminate( smx_channel_t**, int );

/*****************************************************************************/
#define SMX_CONNECT( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.port_ ## ch_name\
        = ( smx_channel_t* )ch

/*****************************************************************************/
#define SMX_CONNECT_ARR( box, ch, box_name, ch_name, mode )\
    ( ( box_ ## box_name ## _t* )box )->mode.ports[\
        ( ( box_ ## box_name ## _t* )box )->mode.count++] = ( smx_channel_t* )ch

/*****************************************************************************/
#define SMX_CONNECT_CP( box, ch )\
    ( ( smx_channel_t* )ch )->collector\
        = ( ( box_smx_cp_t* )box )->in.collector;\

/*****************************************************************************/
#define SMX_MSG_CREATE( f_init, f_copy, f_destroy )\
    smx_msg_create( f_init, f_copy, f_destroy )

/**
 * @brief Create a message structure
 *
 * Allows to create a message structure and attach handlers to modify the data
 * in the message structure. If defined, the init function handler is called
 * after the message structure is created.
 *
 * @param void*( void )     a pointer to a function that initializes the data
 *                          in the message structure. The function must return a
 *                          void pointer to the initilaiyed data structure.
 * @param void*( void* )    a pointer to a function perfroming a deep copy of
 *                          the data in the message structure. The function
 *                          takes a void pointer as an argument that points to
 *                          the data structure to copy. The function must return
 *                          a void pointer to the copied data structure.
 * @param void( void* )     a pointer to a function freeing the memory of the
 *                          data in the message structure. The function takes a
 *                          void pointer as an argument that points to the data
 *                          structure to free.
 * @return smx_msg_t*       a pointer to the created message structure
 */
smx_msg_t* smx_msg_create( void*( void ), void*( void* ), void( void* ) );

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
    smx_program_init();

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
