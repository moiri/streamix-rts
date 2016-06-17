#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"

#define SMX_CHANNEL_READ( h, box, port ) smx_channel_read( h, PORT_##box##_##port )
#define SMX_CHANNEL_WRITE( h, box, port, data ) smx_channel_write( h, PORT_##box##_##port, data )

/**
 * @brief Streamix channel structure
 *
 * The channel structure holds the data that is sent from one box to another
 */
typedef struct smx_channel_s
{
    void*   data;                   /**< pointer to the data */
    int     ready;                  /**< flag to indicate if data is ready */
    pthread_mutex_t channel_mutex;  /**< mutual exclusion */
    pthread_cond_t  channel_cv;     /**< conditional variable to trigger box */
} smx_channel_t;

/**
 * @brief Streamix box structure
 *
 * The box structure provides an interface to access channels connected to the
 * box via named ports
 */
typedef struct smx_box_s {
    int             th_id;  /**< id of the pthread */
    smx_channel_t** in;     /**< input ports */
    smx_channel_t** out;    /**< output ports */
} smx_box_t;

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_IN( h, box, port ) provides a
 * convenient interface to access the ports by name.
 *
 * @param void*     pointer to the box interface structure
 * @param int       index of the port to access
 * @return void*    pointer to the data structure
 */
void* smx_channel_read( void*, int );

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_OUT( h, box, port, data ) provides a
 * convenient interface to access the ports by name.
 *
 * @param void*     pointer to the box interface structure
 * @param int       index of the port to access
 * @param void*     pointer to the data structure
 */
void smx_channel_write( void*, int, void* );

#endif // HANDLER_H
