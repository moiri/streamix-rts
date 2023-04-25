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
#define SMX_SET_READ_TIMEOUT( h, box_name, ch_name, sec, nsec )\
    asm( "nop" )

/**
 * @def SMX_SET_WRITE_TIMEOUT()
 *
 * This macro performs a NOP
 */
#define SMX_SET_WRITE_TIMEOUT( h, box_name, ch_name, sec, nsec )\
    asm( "nop" )

#endif /* SMX_TESTING */

/**
 * Read the test data from the configuration file.
 * @deprecated
 *
 * @param h
 *  The net handler.
 * @param mode
 *  The port mode, either "in" or "out".
 * @param port_name
 *  The name of the port.
 * @param end
 *  A pointer to the channel end which is connected to the port.
 * @return
 *  A pointer to the configuration value or NULL on failure.
 */
const bson_value_t* smx_read_test_data( smx_net_t* h, const char* mode,
        const char* port_name, smx_channel_end_t* end );

/**
 * Read the current test value from the configuration file.
 *
 * @param h
 *  The net handler.
 * @param mode
 *  The port mode, either "in" or "out".
 * @param port_name
 *  The name of the port.
 * @param end
 *  A pointer to the channel end which is connected to the port.
 * @param data
 *  An ouptut param where the data will be stored. This has to be freed after
 *  usage.
 * @return
 *  0 on success and -1 on failure.
 */
int smx_read_test_value( smx_net_t* h, const char* mode,
        const char* port_name, smx_channel_end_t* end, bson_value_t* data );
#endif /* SMXTEST_H */
