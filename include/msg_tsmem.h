/**
 * This file holds the message manipulation functions for a time-stamped data
 * messages.
 *
 * @file    msg_tsmem.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <time.h>
#include "smxnet.h"

#ifndef MSG_TSMEM_H
#define MSG_TSMEM_H

/** ::smx_data_tsmem_s */
typedef struct smx_data_tsmem_s smx_data_tsmem_t;

/**
 * A message data structure that allows to store a time-stamped data chunk.
 */
struct smx_data_tsmem_s
{
    struct timespec ts; /**< the timestamp of the data item */
    void* data;         /**< the chunk of data */
    size_t size;        /**< the size of the data chunk */
};

/**
 * The custom copy function to perform a copy of the tsmem message data.
 * Here, the data element of the structure ::smx_data_tsmem_s is copied with
 * memcpy.
 *
 * @param data
 *  A void pointer to the data structure.
 * @param size
 *  The size of the data.
 * @return
 *  A void pointer to the copied data.
 */
void* smx_data_tsmem_copy( void* data, size_t size );

/**
 * The custom destroy function to destroy the data inside a tsmem message.
 * Here, the data element of the structure ::smx_data_tsmem_s is freed.
 *
 * @param data
 *  A void pointer to the data to be freed.
 */
void smx_data_tsmem_destroy( void* data );

/**
 * The custom unpack function for the tsmem message payload.
 * This function simply returns the payload.
 *
 * @param data
 *  A void pointer to the message payload.
 * @return
 *  A void pointer to the unpacked message payload.
 */
void* smx_data_tsmem_unpack( void* data );

/**
 * This is a helper function to easily create a time-stamped message containing
 * a data chunk.
 *
 * @param net
 *  The net handler
 * @param ts
 *  The timestamp.
 * @param data
 *  The allocated data chunk.
 * @param size
 *  The size of the allocated data chunk.
 */
void* smx_msg_tsmem_create( smx_net_t* net, struct timespec ts, void* data,
        size_t size );

/**
 * This is a helper function to easily create a time-stamped message containing
 * a data chunk where the timestamp is created from the current time.
 *
 * @param net
 *  The net handler
 * @param data
 *  The allocated data chunk.
 * @param size
 *  The size of the allocated data chunk.
 */
void* smx_msg_tsmem_create_ts( smx_net_t* net, void* data, size_t size );

/**
 * This is a helper function to easily create a time-stamped message containing
 * a data string.
 *
 * @param net
 *  The net handler
 * @param ts
 *  The timestamp.
 * @param data
 *  The allocated data string.
 */
void* smx_msg_tsstr_create( smx_net_t* net, struct timespec ts, char* data );

/**
 * This is a helper function to easily create a time-stamped message containing
 * a data string where the timestamp is created from the current time.
 *
 * @param net
 *  The net handler
 * @param data
 *  The allocated data string.
 */
void* smx_msg_tsstr_create_ts( smx_net_t* net, char* data );

#endif
