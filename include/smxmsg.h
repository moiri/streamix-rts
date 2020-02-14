/**
 * @file     smxmsg.h
 * @author   Simon Maurer
 * @defgroup msg Message API
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Message definitions for the runtime system library of Streamix
 */

#include <stdlib.h>
#include "smxtypes.h"

#ifndef SMXMSG_H
#define SMXMSG_H

/**
 * @def SMX_MSG_COPY()
 *
 * Make a deep copy of a message. For details refer to smx_msg_copy().
 */
#define SMX_MSG_COPY( h, msg )\
    smx_msg_copy( h, msg )

/**
 * @def SMX_MSG_CREATE()
 *
 * Create a message structure. For details refer to smx_msg_create().
 */
#define SMX_MSG_CREATE( h, data, dsize, fcopy, ffree, funpack )\
    smx_msg_create( h, data, dsize, fcopy, ffree, funpack )

/**
 * @def SMX_MSG_DESTROY()
 *
 * Destroy a message structure. For details refer to smx_msg_destroy().
 */
#define SMX_MSG_DESTROY( h, msg )\
    smx_msg_destroy( h, msg, 1 )

/**
 * @def SMX_MSG_FILTER()
 *
 * Checks wether any of the provided filter match with the message type.
 * For more details refer to smx_msg_filer().
 */
#define SMX_MSG_FILTER( msg, count, ... )\
    smx_msg_filter( msg, count, ##__VA_ARGS__ )

/**
 * @def SMX_MSG_UNPACK()
 *
 * Unpack the message payload. For details refer to smx_msg_unpack().
 */
#define SMX_MSG_UNPACK( msg )\
    smx_msg_unpack( msg )

/**
 * @def SMX_MSG_SET_TYPE()
 *
 * Set the type of the message payload. The type can be an arbitrary string.
 * For details refer to smx_msg_set_type().
 */
#define SMX_MSG_SET_TYPE( msg, type )\
    smx_msg_set_type( msg, type )

/**
 * @def SMX_MSG_PREVENT_BACKUP()
 *
 * Prevent a message from creating backups in decoupled channels.
 */
#define SMX_MSG_PREVENT_BACKUP( msg )\
    smx_msg_prevent_backup( msg )

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
 * @param h
 *  pointer to the net handler
 * @param data
 *  a pointer to the data to be added to the message
 * @param size
 *  the size of the data
 * @param copy
 *  a pointer to a function perfroming a deep copy of the data in the message
 *  structure.
 *  If NULL is passed the default function smx_msg_data_copy() will be used.
 *  Refer to smx_msg_data_copy() for information on function arguments and
 *  return value.
 * @param destroy
 *  a pointer to a function freeing the memory of the data in the message
 *  structure.
 *  If NULL is passed the default function smx_msg_data_destroy() will be used.
 *  Refer to smx_msg_data_destroy() for information on function arguments and
 *  return value.
 * @param unpack
 *  a pointer to a function that unpacks the message data.
 *  If NULL is passed the default function smx_msg_data_unpack() will be used.
 *  Refer to smx_msg_data_unpack() for information on function arguments and
 *  return value.
 * @return
 *  a pointer to the created message structure
 */
smx_msg_t* smx_msg_create( void* h, void* data, size_t size,
        void* (*copy)( void* data, size_t size ), void (*destroy)( void* data ),
        void* (*unpack)( void* data ) );

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
 * Checks wether the message type matches any of the strings passed as
 * arguments.
 *
 * @param msg
 *  The message to be checked
 * @param count
 *  The number of filter arguments passed to the function
 * @param ...
 *  Any number of string arguments. If the message type matches any of these
 *  the filter check passed. NULL is a valid argument.
 * @return
 *  The index of the mathcing filer on success or -1 on failure.
 */
int smx_msg_filter( smx_msg_t* msg, int count, ... );

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
 * Prevents a message from being copied to the backup space in a decoupled
 * channel.
 *
 * @param msg
 *  A pointer to the message structure.
 */
void smx_msg_prevent_backup( smx_msg_t* msg );

/**
 * @brief Unpack the message payload
 *
 * @param msg   a pointer to the message structure to be destroyed
 * @return      a void pointer to the payload
 */
void* smx_msg_unpack( smx_msg_t* msg );

/**
 * Set the type of the message payload. The type can be an arbitrary string.
 *
 * @param msg
 *  A pointer to the message where the type will be set.
 * @param type
 *  An arbitrary string definig the type. This function will allocate the
 *  string in memory.
 */
int smx_msg_set_type( smx_msg_t* msg, const char* type );

#endif /* SMXMSG_H */
