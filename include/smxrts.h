#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"
#include <stdlib.h>

#define FIFO_LEN 5

typedef struct smx_port_s
{
    void* ch;
    void* (*init)();
    void* (*copy)();
    void  (*free)();
} smx_port_t;

/**
 *
 */
typedef struct smx_channel_item_s smx_channel_item_t;
struct smx_channel_item_s
{
    void*   data;                   /**< pointer to the data */
    smx_channel_item_t* next;       /**< pointer to the next item */
    smx_channel_item_t* prev;       /**< pointer to the previous item */
};

/**
 * @brief Streamix channel structure
 *
 * The channel structure holds the data that is sent from one box to another
 */
typedef struct smx_channel_s
{
    smx_channel_item_t*  head;      /**< pointer to the heda of the FIFO */
    smx_channel_item_t*  tail;      /**< pointer to the tail of the FIFO */
    int     count;                  /**< counts occupied space */
    int     length;                 /**< siye of the FIFO */
    pthread_mutex_t channel_mutex;  /**< mutual exclusion */
    pthread_cond_t  channel_cv;     /**< conditional variable to trigger box */
} smx_channel_t;

typedef struct smx_blackboard_s
{
    void**  data;
    int     read;
    int     write;
    pthread_mutex_t mutex_read;  /**< mutual exclusion */
    pthread_mutex_t mutex_write;  /**< mutual exclusion */
} smx_blackboard_t;

/*****************************************************************************/
#define SMX_BOX_CREATE( box )\
    ( void* )malloc( sizeof( struct box_##box##_s ) )

/*****************************************************************************/
#define SMX_BOX_RUN( arg, box_name )\
    pthread_t th_ ## box_name = smx_box_run( box_ ## box_name, arg )

/**
 *
 */
pthread_t smx_box_run( void*( void* ), void* );

/*****************************************************************************/
#define SMX_BOX_WAIT_END( box_name )\
    pthread_join( th_ ## box_name, NULL )


/*****************************************************************************/
#define SMX_BOX_DESTROY( box )\
    free( box )

/*****************************************************************************/
#define SMX_CHANNEL_CREATE()\
    ( void* )smx_channel_create( FIFO_LEN )

/**
 *
 */
smx_channel_t* smx_channel_create( int );

/*****************************************************************************/
#define SMX_CHANNEL_DESTROY( ch )\
    smx_channel_destroy( ch )

/**
 *
 */
void smx_channel_destroy( smx_channel_t* );

/*****************************************************************************/
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( ( ( box_ ## box_name ## _t* )h )->port_ ## ch_name->ch )

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_IN( h, box, port ) provides a
 * convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @return void*            pointer to the data structure
 */
void* smx_channel_read( smx_channel_t* );

/*****************************************************************************/
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( ( ( box_ ## box_name ## _t* )h )->port_ ## ch_name->ch,\
            data )

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_OUT( h, box, port, data ) provides a
 * convenient interface to access the ports by name.
 *
 * @param smx_channel_t*    pointer to the channel
 * @param void*             pointer to the data structure
 */
void smx_channel_write( smx_channel_t*, void* );

/*****************************************************************************/
#define SMX_CHANNELS_CREATE( num )\
    smx_channels_create( num )

/**
 *
 */
smx_channel_t** smx_channels_create();

/*****************************************************************************/
#define SMX_CHANNELS_DESTROY( chs, num )\
    smx_channels_destroy( chs, num )

/**
 *
 */
void smx_channels_destroy( smx_channel_t**, int );

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
 *
 */
void smx_program_init();

/*****************************************************************************/
#define SMX_PROGRAM_CLEANUP()\
    smx_program_cleanup()

/**
 *
 */
void smx_program_cleanup();

#endif // HANDLER_H
