/**
 * @file     smxtest.h
 * @author   Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * A test environment that allows to test box implementation without having to
 * run them inside a streamix application.
 */

#ifndef SMXTEST_H
#define SMXTEST_H

#include <bson.h>
#include "smxutils.h"
#include "smxtypes.h"

#ifdef SMX_TESTING

/**
 * @def SMX_CHANNEL_READ()
 *
 * Read test input data from the configuration file.
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
    box_name ## _in_data_conversion( h, smx_read_test_data( h, "in", #ch_name ),\
            SMX_SIG_PORT_IDX( box_name, ch_name, in ) )

/**
 * @def SMX_CHANNEL_WRITE()
 *
 * Compare the produced data to the test ouput data from the configuration file.
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
    box_name ## _out_data_conversion( h, smx_read_test_data( h, "out", #ch_name ),\
            SMX_SIG_PORT_IDX( box_name, ch_name, out ), data )

/**
 * @def SMX_GET_READ_ERROR()
 *
 * This always returns SMX_CHANNEL_ERR_NO_TARGET
 */
#define SMX_GET_READ_ERROR( h, box_name, ch_name )\
    SMX_CHANNEL_ERR_NO_TARGET

/**
 * @def SMX_GET_WRITE_ERROR()
 *
 * This always returns SMX_CHANNEL_ERR_NO_TARGET
 */
#define SMX_GET_WRITE_ERROR( h, box_name, ch_name )\
    SMX_CHANNEL_ERR_NO_TARGET

/**
 * @def SMX_SET_READ_TIMEOUT()
 *
 * This macro performs a NOP
 */
#define SMX_SET_READ_TIMEOUT( h, box_name, ch_name )\
    asm( "nop" )

/**
 * @def SMX_SET_WRITE_TIMEOUT()
 *
 * This macro performs a NOP
 */
#define SMX_SET_WRITE_TIMEOUT( h, box_name, ch_name )\
    asm( "nop" )

#endif /* SMX_TESTING */

/**
 * Read the test data from the configuration file.
 *
 * @param h
 *  The net handler.
 * @param mode
 *  The port mode, either "in" or "out".
 * @param port_name
 *  The name of the port.
 * @return
 *  A pointer to the configuration value or NULL on failure.
 */
const bson_value_t* smx_read_test_data( smx_net_t* h, const char* mode,
        const char* port_name );

#endif /* SMXTEST_H */
