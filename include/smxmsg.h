/**
 * Message definitions for the runtime system library of Streamix
 *
 * @file    smxmsg.h
 * @author  Simon Maurer
 */

#include <stdlib.h>

#ifndef SMXMSG_H
#define SMXMSG_H

typedef struct smx_msg_s smx_msg_t;                   /**< ::smx_msg_s */

/**
 * @brief A Streamix port structure
 *
 * The structure contains handlers that can be used to manipulate data.
 * This handler is provided by the box implementation.
 */
struct smx_msg_s
{
    unsigned long id;               /**< the unique message id */
    void* data;                     /**< pointer to the data */
    int   size;                     /**< size of the data */
    void* (*copy)( void*, size_t ); /**< pointer to a fct making a deep copy */
    void  (*destroy)( void* );      /**< pointer to a fct that frees data */
    void* (*unpack)( void* );       /**< pointer to a fct that unpacks data */
    int is_profiler;                /**< 1 if the message was created by the
                                      profiler, 0 otherwise */
};

/**
 * @brief make a deep copy of a message
 *
 * @param h     pointer to the net handler
 * @param msg   pointer to the message structure to copy
 * @return      pointer to the newly created message structure
 */
smx_msg_t* smx_msg_copy( void* h, smx_msg_t* msg );

/**
 * @brief Create a message structure
 *
 * Allows to create a message structure and attach handlers to modify the data
 * in the message structure. If defined, the init function handler is called
 * after the message structure is created.
 *
 * @param h                 pointer to the net handler
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
 * @param is_profiler       1 if the message was created by the profiler,
 *                          0 otherwise
 * @return                  a pointer to the created message structure
 */
smx_msg_t* smx_msg_create( void* h, void* data, size_t size,
        void* copy( void*, size_t ), void destroy( void* ),
        void* unpack( void* ), int is_profiler );

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
 * @param h     pointer to the net handler
 * @param msg   a pointer to the message structure to be destroyed
 * @param deep  a flag to indicate whether the data shoudl be deleted as well
 *              if msg->destroy() is NULL this flag is ignored
 */
void smx_msg_destroy( void* h, smx_msg_t* msg, int deep );

/**
 * @brief Unpack the message payload
 *
 * @param msg   a pointer to the message structure to be destroyed
 * @return      a void pointer to the payload
 */
void* smx_msg_unpack( smx_msg_t* msg );

#endif
