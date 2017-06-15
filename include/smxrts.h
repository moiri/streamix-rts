#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"
#include <stdlib.h>

#define FIFO_LEN 5

// TYPEDEFS -------------------------------------------------------------------
typedef struct smx_fifo_item_s smx_fifo_item_t;
typedef struct smx_fifo_s smx_fifo_t;
typedef struct smx_blackboard_s smx_blackboard_t;
typedef struct smx_channel_s smx_channel_t;
typedef struct smx_port_s smx_port_t;
typedef enum smx_channel_type_e smx_channel_type_t;

// ENUMS ----------------------------------------------------------------------
/**
 * @brief Streamix channel (buffer) types
 */
enum smx_channel_type_e
{
    SMX_FIFO,           /**< ::smx_fifo_t */
    SMX_BLACKBOARD      /**< ::smx_blackboard_t */
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
    void**  data;                 /**< array of data */
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
    int     length;              /**< siye of the FIFO */
    pthread_mutex_t fifo_mutex;  /**< mutual exclusion */
    pthread_cond_t  fifo_cv;     /**< conditional variable to trigger box */
};

/**
 * @brief A single FIFO item of a circular double-linked-list
 */
struct smx_fifo_item_s
{
    void*   data;                /**< pointer to the data */
    smx_fifo_item_t* next;       /**< pointer to the next item */
    smx_fifo_item_t* prev;       /**< pointer to the previous item */
};

/**
 * @brief A Streamix port structure
 *
 * The structure contains handlers that can be used to manipulate data.
 * This handler is provided by the box implementation.
 */
struct smx_port_s
{
    smx_channel_t* ch;  /**< ::smx_channel_s */
    void* (*init)();    /**< pointer to a function that initialiyes data */
    void* (*copy)();    /**< pointer to a function that makes a deep copy */
    void  (*free)();    /**< pointer to a function that frees data */
};

// FUNCTIONS ------------------------------------------------------------------
/*****************************************************************************/
#define SMX_BOX_CREATE( box )\
    ( void* )malloc( sizeof( struct box_##box##_s ) )

/*****************************************************************************/
#define SMX_BOX_RUN( arg, box_name )\
    pthread_t th_ ## box_name = smx_box_run( box_ ## box_name, arg )

/**
 * @brief create pthred of box
 *
 * @param box_impl:     function pointer to the box implementation
 * @param arg:          pointer to the box handler
 * @return              pointer to a pthread
 */
pthread_t smx_box_run( void*( void* ), void* );

/*****************************************************************************/
#define SMX_BOX_WAIT_END( box_name )\
    pthread_join( th_ ## box_name, NULL )

/*****************************************************************************/
#define SMX_BOX_DESTROY( box )\
    free( box )

/*****************************************************************************/
#define SMX_CHANNEL_CREATE( len, type )\
    smx_channel_create( len, type )

/**
 * @brief Create Streamix channel
 *
 * @param len:      length of a FIFO
 * @param type:     type of the buffer, #smx_channel_type_e
 * @return          pointer to the created channel
 */
smx_channel_t* smx_channel_create( int, smx_channel_type_t );

/**
 * @brief Create Streamix FIFO channel
 *
 * @param len:      length of the FIFO
 * @return          pointer to the created FIFO
 */
smx_fifo_t* smx_fifo_create( int );

/**
 * @brief Create Streamix blackboard channel
 *
 * @return          pointer to the created blackboard
 */
smx_blackboard_t* smx_blackboard_create();

/*****************************************************************************/
#define SMX_CHANNEL_DESTROY( ch )\
    smx_channel_destroy( ch )

/**
 * @brief Destroy Streamix channel structure
 *
 * @param ch    pointer to the channel to destroy
 */
void smx_channel_destroy( smx_channel_t* );

/**
 * @brief Destroy Streamix FIFO channel structure
 *
 * @param fifo  pointer to the channel to destroy
 */
void smx_fifo_destroy( smx_fifo_t* );

/**
 * @brief Destroy Streamix blackboard channel structure
 *
 * @param bb    pointer to the channel to destroy
 */
void smx_blackboard_destroy( smx_blackboard_t* );

/*****************************************************************************/
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( ( ( box_ ## box_name ## _t* )h )->port_ ## ch_name )

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_READ( h, box, port ) provides a
 * convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @return void*            pointer to the data structure
 */
void* smx_channel_read( smx_port_t* );

/**
 * @brief read from a Streamix FIFO channel
 *
 * @param fifo  pointer to a FIFO channel
 * @param data  pointer to the data
 */
void* smx_fifo_read( smx_fifo_t* );

/**
 * @brief read from a Streamix blackboard channel
 *
 * @param fifo  pointer to a blackboard channel
 * @param data  pointer to the data
 */
void* smx_blackboard_read( smx_blackboard_t* );

/*****************************************************************************/
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( ( ( box_ ## box_name ## _t* )h )->port_ ## ch_name,\
            data )

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_WRITE( h, box, port, data ) provides
 * a convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @param void*             pointer to the data structure
 */
void smx_channel_write( smx_port_t*, void* );

/**
 * @brief write to a Streamix FIFO channel
 *
 * @param fifo  pointer to a FIFO channel
 * @param data  pointer to the data
 */
void smx_fifo_write( smx_fifo_t*, void* );

/**
 * @brief write to a Streamix blackboard channel
 *
 * @param fifo  pointer to a blackboard channel
 * @param data  pointer to the data
 */
void smx_blackboard_write( smx_blackboard_t*, void* );

/*****************************************************************************/
#define SMX_CONNECT( box, ch_v, box_name, ch_name )\
    ( ( box_ ## box_name ## _t* )box )->port_ ## ch_name->ch =\
            ( smx_channel_t* )ch_v

/*****************************************************************************/
#define SMX_PORT_CREATE( box, box_name, ch_name )\
    ( ( box_ ## box_name ## _t* )box )->port_ ## ch_name =\
            malloc( sizeof( struct smx_port_s ) )

/*****************************************************************************/
#define SMX_PORT_DESTROY( box, box_name, ch_name )\
    free( ( ( box_ ## box_name ## _t* )box )->port_ ## ch_name )

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
