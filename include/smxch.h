/**
 * @file     smxch.h
 * @author   Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Channel and FIFO definitions for the runtime system library of Streamix
 */

#include "smxtypes.h"

#ifndef SMXCH_H
#define SMXCH_H

/**
 * @def SMX_CHANNEL_SET_BACKUP()
 *
 * Store a backup message to a channel. This can be used to ansure that ar
 * decoupled input port always provides a valid message.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param msg
 *  A pointer to a message to back up.
 * @return
 *  0 on success, -1 on failure
 */
#define SMX_CHANNEL_SET_BACKUP( h, box_name, ch_name, msg )\
    smx_channel_set_backup( SMX_SIG_PORT( h, box_name, ch_name, in ), msg )

/**
 * @def SMX_CHANNEL_READ()
 *
 * Read from a streamix channel by accessing a net input port.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @return
 *  A pointer to a message type ::smx_msg_t or NULL if something went
 *  wrong. Use the macro SMX_GET_READ_ERROR() to find out the cause of an error.
 */
#define SMX_CHANNEL_READ( h, box_name, ch_name )\
    smx_channel_read( h, SMX_SIG_PORT( h, box_name, ch_name, in ) )

/**
 * @def SMX_CHANNEL_WRITE()
 *
 * Write to a streamix channel by accessing a net output port.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the output port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param data
 *  A pointer to an allocated message of type ::smx_msg_t. Use the macro
 *  SMX_MSG_CREATE() to create a new message if required.
 * @return
 *  0 on success, -1 on failure. Use the macro SMX_GET_WRITE_ERROR() to find
 *  out the cause of an error.
 */
#define SMX_CHANNEL_WRITE( h, box_name, ch_name, data )\
    smx_channel_write( h, SMX_SIG_PORT( h, box_name, ch_name, out ), data )

#ifndef SMX_TESTING

/**
 * @def SMX_GET_READ_ERROR()
 *
 * Get the error code of a channel read operation. Use this macro if
 * SMX_CHANNEL_READ() failed.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @return
 *  The error code of the operation. Refer to #smx_channel_err_e for a
 *  description of the error codes.
 */
#define SMX_GET_READ_ERROR( h, box_name, ch_name )\
    smx_get_read_error( SMX_SIG_PORT( h, box_name, ch_name, in ) )

/**
 * @def SMX_GET_WRITE_ERROR()
 *
 * Get the error code of a channel write operation. Use this macro if
 * SMX_CHANNEL_WRITE() failed.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @return
 *  The error code of the operation. Refer to #smx_channel_err_e for a
 *  description of the error codes.
 */
#define SMX_GET_WRITE_ERROR( h, box_name, ch_name )\
    smx_get_write_error( SMX_SIG_PORT( h, box_name, ch_name, out ) )

/**
 * @def SMX_SET_READ_TIMEOUT()
 *
 * Set a timeout on the channel source
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param sec
 *  The number of seconds to wait
 * @param nsec
 *  The number of nanoseconds to wait
 * @return
 *  The error code of the operation. Refer to #smx_channel_err_e for a
 *  description of the error codes.
 */
#define SMX_SET_READ_TIMEOUT( h, box_name, ch_name, sec, nsec )\
    smx_set_read_timeout( SMX_SIG_PORT( h, box_name, ch_name, in ), sec, nsec )

/**
 * @def SMX_SET_ERITE_TIMEOUT()
 *
 * Set a timeout on the channel sink
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the input port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param sec
 *  The number of seconds to wait
 * @param nsec
 *  The number of nanoseconds to wait
 * @return
 *  The error code of the operation. Refer to #smx_channel_err_e for a
 *  description of the error codes.
 */
#define SMX_SET_WRITE_TIMEOUT( h, box_name, ch_name, sec, nsec )\
    smx_set_write_timeout( SMX_SIG_PORT( h, box_name, ch_name, out ), sec, nsec )


#endif /* SMX_TESTING */

/**
 * @def SMX_CHANNEL_SET_CONTENT_FILTER()
 *
 * Set a message content filter on a channel. The filter is a function that
 * operates on the message content. The function receives the message as
 * parameter and must return either true if the filter passes or false if the
 * filter fails.
 *
 * If the filter failes, the macro SMX_CHANNEL_WRITE() silently dismisses the
 * message and returns 0. A content filter fail does not count as error.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the output port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param filter
 *  A pointer to the filter function. The filter function must return a booloan
 *  and takes a pointer to the message to be checked as parameter.
 * @return
 *  true on success or false on failure.
 */
#define SMX_CHANNEL_SET_CONTENT_FILTER( h, box_name, ch_name, filter )\
    smx_channel_set_content_filter( SMX_SIG_PORT( h, box_name, ch_name, in ),\
            filter )

/**
 * @def SMX_CHANNEL_SET_TYPE_FILTER()
 *
 * Set a message type filter on a channel filter. A channel filter allows to
 * whitelist message types. If the filter is set, only messages of the
 * specified types are allowed to be written to a channel. One filter is an
 * arbitrary string or NULL to allow messages with undefined message type.
 * If a message type does not match any whitelisted types, an error is logged
 * and the message is dismissed.
 *
 * If the filter failes, the macro SMX_CHANNEL_WRITE() returns -1 and sets the
 * error SMX_CHANNEL_ERR_FILTER.
 *
 * @param h
 *  The pointer to the net handler.
 * @param box_name
 *  The name of the box. Note that this is not a string but the literal name of
 *  the box (without quotation marks).
 * @param ch_name
 *  The name of the output port. Note that this is not a string but the literal
 *  name of the port (without quotation marks).
 * @param count
 *  The number of filter arguments passed to the function
 * @param ...
 *  Any number of string arguments. If the message type matches any of these
 *  the filter check passed. NULL is a valid argument.
 * @return
 *  true on success or false on failure.
 */
#define SMX_CHANNEL_SET_TYPE_FILTER( h, box_name, ch_name, count, ... )\
    smx_channel_set_filter( h, SMX_SIG_PORT( h, box_name, ch_name, in ),\
            count, ##__VA_ARGS__ )

/**
 * Change the state of a channel collector. The state is only changed if the
 * current state is differnt than the new state and than the end state.
 *
 * @param ch    pointer to the channel
 * @param state the new state
 */
void smx_channel_change_collector_state( smx_channel_t* ch,
        smx_channel_state_t state );

/**
 * Change the read state of a channel. The state is only changed if the
 * current state is differnt than the new state and than the end state.
 *
 * @param ch    pointer to the channel
 * @param state the new state
 */
void smx_channel_change_read_state( smx_channel_t* ch,
        smx_channel_state_t state );

/**
 * Change the write state of a channel. The state is only changed if the
 * current state is differnt than the new state and than the end state.
 *
 * @param ch    pointer to the channel
 * @param state the new state
 */
void smx_channel_change_write_state( smx_channel_t* ch,
        smx_channel_state_t state );

/**
 * @brief Create Streamix channel
 *
 * @param ch_cnt    pointer to the channel counter (is increased by one after
 *                  channel creation)
 * @param len       length of a FIFO
 * @param type      type of the buffer
 * @param id        unique identifier of the channel
 * @param name      name of the channel
 * @param cat_name  name of the channel zlog category
 * @return          a pointer to the created channel or NULL
 */
smx_channel_t* smx_channel_create( int* ch_cnt, int len,
        smx_channel_type_t type, int id, const char* name,
        const char* cat_name );

/**
 * Create a channel end.
 *
 * @return a pointer to a ne channel end or NULL if something went wrong
 */
smx_channel_end_t* smx_channel_create_end();

/**
 * @brief Destroy Streamix channel structure
 *
 * @param ch    pointer to the channel to destroy
 */
void smx_channel_destroy( smx_channel_t* ch );

/**
 * @brief Destroy Streamix channel end structure
 *
 * @param end   pointer to the channel end to destroy
 */
void smx_channel_destroy_end( smx_channel_end_t* end );

/**
 * @brief Read the data from an input port
 *
 * Allows to access the channel and read data. The channel is protected by
 * mutual exclusion. The macro SMX_CHANNEL_READ() provides a convenient
 * interface to access the ports by name.
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to the channel
 * @return      pointer to a message structure ::smx_msg_s or NULL if something
 *              went wrong.
 */
smx_msg_t* smx_channel_read( void* h, smx_channel_t* ch );

/**
 * @brief Returns the number of available messages in channel
 *
 * @param ch    pointer to the channel
 * @return      number of available messages in channel or -1 on failure
 */
int smx_channel_ready_to_read( smx_channel_t* ch );

/**
 * @brief Returns the number of available space in a channel
 *
 * @param ch    pointer to the channel
 * @return      number of available space in a channel or -1 on failure
 */
int smx_channel_ready_to_write( smx_channel_t* ch );

/**
 * Set a backup message to a decouple input port. This allows to read from a
 * decoupled port without ever having received a message.
 *
 * @param ch
 *  A pointer to the channel.
 * @return
 *  0 on success, -1 on failure.
 */
int smx_channel_set_backup( smx_channel_t* ch, smx_msg_t* msg );

/**
 * Set a channel filter to only allow messages of a certain content to be
 * written to this channel.
 *
 * @param ch
 *  pointer to the channel
 * @param filter
 *  a pointer to a function returning a booloan and taking the message to be
 *  filtered as argument.
 * @return
 *  true on success or false on failure.
 */
bool smx_channel_set_content_filter( smx_channel_t* ch,
        bool filter( smx_net_t*, smx_msg_t* ) );

/**
 * Set the channel filter to only allow messages of a certain type to be
 * written to this channel.
 *
 * @param h
 *  pointer to the net handler.
 * @param ch
 *  pointer to the channel
 * @param count
 *  The number of filter arguments passed to the function
 * @param ...
 *  Any number of string arguments. If the message type matches any of these
 *  the filter check passed. NULL is a valid argument.
 * @return
 *  true on success or false on failure.
 */
bool smx_channel_set_filter( smx_net_t* h, smx_channel_t* ch, int count, ... );

/**
 * Return a human-readable error message, give an error code.
 *
 * @param err
 *  The error code to transform.
 * @return
 *  A human-readable error message.
 */
const char* smx_channel_strerror( smx_channel_err_t err );

/**
 * Send the termination signal to a channel sink
 *
 * @param ch    pointer to the channel
 */
void smx_channel_terminate_sink( smx_channel_t* ch );

/**
 * Send the termination signal to a channel source
 *
 * @param ch    pointer to the channel
 */
void smx_channel_terminate_source( smx_channel_t* ch );

/**
 * @brief Write data to an output port
 *
 * Allows to access the channel and write data. The channel ist protected by
 * mutual exclusion. The macro SMX_CHANNEL_WRITE( h, net, port, data ) provides
 * a convenient interface to access the ports by name.
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to the channel
 * @param msg   pointer to the a message structure
 * @return      0 on success, -1 otherwise
 */
int smx_channel_write( void* h, smx_channel_t* ch, smx_msg_t* msg );

/**
 * Create a collector structure and initialize it.
 *
 * @return a pointer to the created collector strcuture or NULL.
 */
smx_collector_t* smx_collector_create();

/**
 * Destroy and deinit a collector structure.
 *
 * @param collector a pointer to the collector structure to be destroyed.
 */
void smx_collector_destroy( smx_collector_t* collector );

/**
 * Send the termination signal to the collector
 *
 * @param ch    pointer to the channel
 *
 */
void smx_collector_terminate( smx_channel_t* ch );

/**
 * Connect a channel to a net by name matching.
 *
 * @param dest        a pointer to the destination
 * @param src         a pointer to the source
 * @param net_id      the id of the net
 * @param net_name    the name of the net
 * @param mode        the direction of the connection
 * @param count       pointer to th econnected port counter
 */
void smx_connect( smx_channel_t** dest, smx_channel_t* src, int net_id,
        const char* net_name, const char* mode, int* count );

/**
 * Connect a guard to a channel
 *
 * @param ch    the target channel
 * @param guard the guard to be connected
 */
void smx_connect_guard( smx_channel_t* ch, smx_guard_t* guard );

/**
 * Connect a channel to an input of a net.
 *
 * @param dest        a pointer to the destination
 * @param src         a pointer to the source
 * @param net         a pointer to the net
 * @param mode        the direction of the connection
 * @param count       pointer to th econnected port counter
 */
void smx_connect_in( smx_channel_t** dest, smx_channel_t* src, smx_net_t* net,
        const char* mode, int* count );

/**
 * Increment the port counter to take into account any non-declared open port.
 * This helps to avoid missaligned connections if an open port was ommitted in
 * the STreamix decalration.
 *
 * @param count
 *  A pointer to the current port counter.
 * @param static_count
 *  The number of static ports.
 */
void smx_connect_open( int* count, int static_count );

/**
 * Connect a channel to an output of a net.
 *
 * @param dest        a pointer to the destination
 * @param src         a pointer to the source
 * @param net         a pointer to the net
 * @param mode        the direction of the connection
 * @param count       pointer to th econnected port counter
 */
void smx_connect_out( smx_channel_t** dest, smx_channel_t* src, smx_net_t* net,
        const char* mode, int* count );

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
 * @param h     pointer to the net handler
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO channel
 * @return      pointer to a message structure
 */
smx_msg_t* smx_fifo_read( void* h, smx_channel_t* ch, smx_fifo_t* fifo );

/**
 * @brief read from a Streamix FIFO_D channel
 *
 * Read from a channel that is decoupled at the output (the consumer is
 * decoupled at the input). This means that the msg at the head of the FIFO_D
 * will potentially be duplicated.
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO_D channel
 * @return      pointer to a message structure
 */
smx_msg_t* smx_fifo_d_read( void* h, smx_channel_t* ch , smx_fifo_t* fifo );

/**
 * @brief read from a Streamix FIFO_DD channel
 *
 * Read from a channel that is decoupled at the output and connected to a
 * temporal firewall. The read is non-blocking but no duplication of messages
 * is done. If no message is available NULL is returned.
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO_D channel
 * @return      pointer to a message structure
 */
smx_msg_t* smx_fifo_dd_read( void* h, smx_channel_t* ch, smx_fifo_t* fifo );

/**
 * @brief write to a Streamix FIFO channel
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a FIFO channel
 * @param msg   pointer to the data
 * @return      0 on success, 1 otherwise
 */
int smx_fifo_write( void* h, smx_channel_t* ch, smx_fifo_t* fifo,
        smx_msg_t* msg );

/**
 * @brief write to a Streamix D_FIFO channel
 *
 * Write to a channel that is decoupled at the input (the produced is decoupled
 * at the output). This means that the tail of the D_FIFO will potentially be
 * overwritten.
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to channel struct of the FIFO
 * @param fifo  pointer to a D_FIFO channel
 * @param msg   pointer to the data
 * @return      0 on success, 1 otherwise
 */
int smx_d_fifo_write( void* h, smx_channel_t* ch, smx_fifo_t* fifo,
        smx_msg_t* msg );

/**
 * Given a port name return a pointer to the port.
 *
 * @param ports     an array of ports to be searched
 * @param count     the number of ports to search
 * @param name      the name to search for
 * @return          the pointer to a port on success, NULL otherwise
 */
smx_channel_t* smx_get_channel_by_name( smx_channel_t** ports, int count,
        const char* name );

/**
 * Get the read error on a channel.
 *
 * @param ch
 *  Pointer to the channel
 * @return
 *  The error value indicationg the problem
 */
smx_channel_err_t smx_get_read_error( smx_channel_t* ch );

/**
 * Get the write error on a channel.
 *
 * @param ch
 *  Pointer to the channel
 * @return
 *  The error value indicationg the problem
 */
smx_channel_err_t smx_get_write_error( smx_channel_t* ch );

/**
 * @brief create timed guard structure and initialise timer
 *
 * @param iats  minimal inter-arrival time in seconds
 * @param iatns minimal inter-arrival time in nano seconds
 * @param ch    pointer to the channel
 * @return      pointer to the created guard structure
 */
smx_guard_t* smx_guard_create( int iats, int iatns, smx_channel_t* ch );

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
 * @param h     pointer to the net handler
 * @param ch pointer to the channel structure
 * @return      0 on success, 1 otherwise
 */
int smx_guard_write( void* h, smx_channel_t* ch );

/**
 * @brief imposes a rate-control on decoupled write operations
 *
 * A message is discarded if it did not reach the specified minimal inter-
 * arrival time (messages are not buffered and delayed, it's only a very simple
 * implementation)
 *
 * @param h     pointer to the net handler
 * @param ch    pointer to the channel structure
 * @param msg   pointer to the message structure
 *
 * @return      -1 if message was discarded, 0 otherwise
 */
int smx_d_guard_write( void* h, smx_channel_t* ch, smx_msg_t* msg );

/**
 * Set the channel read timeout.
 *
 * @param end
 *  Pointer to the channel end
 * @param sec
 *  The second part of the timer
 * @param nsec
 *  The nanosecond part of the timer
 * @return
 *  The error value indicationg the problem
 */
int smx_set_read_timeout( smx_channel_t* ch, long sec, long nsec );

/**
 * Set the channel write timeout.
 *
 * @param end
 *  Pointer to the channel end
 * @param sec
 *  The second part of the timer
 * @param nsec
 *  The nanosecond part of the timer
 * @return
 *  The error value indicationg the problem
 */
int smx_set_write_timeout( smx_channel_t* ch, long sec, long nsec );

#endif /* SMXCH_H */
