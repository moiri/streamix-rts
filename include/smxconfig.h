/**
 * @file     smxconfig.h
 * @author   Simon Maurer
 * @defgroup conf Configuration API
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Helper functions for parsing configuration files of the runtime systemr
 * library of Streamix.
 */

#include <bson.h>
#include <stdbool.h>

#ifndef SMXCONFIG_H
#define SMXCONFIG_H

/**
 * @addtogroup smx
 * @{
 * @addtogroup conf
 * @{
 */

/**
 * Get a boolean value from the config file.
 *
 * @param conf
 *  The pointer to th econfig file.
 * @param search
 *  A dot-notation key like "a.b.c.d".
 * @return
 *  The boolean value read from the file or false if value is not available.
 */
bool smx_config_get_bool( bson_t* conf, const char* search );

/**
 * Get an int value from the config file.
 *
 * @param conf
 *  The pointer to th econfig file.
 * @param search
 *  A dot-notation key like "a.b.c.d".
 * @return
 *  The int value read from the file or 0 if value is not available.
 */
int smx_config_get_int( bson_t* conf, const char* search );

/**
 * Get a double value from the config file.
 *
 * @param conf
 *  The pointer to th econfig file.
 * @param search
 *  A dot-notation key like "a.b.c.d".
 * @return
 *  The double value read from the file or 0 if value is not available.
 */
double smx_config_get_double( bson_t* conf, const char* search );

/**
 * Get a string value from the config file.
 *
 * @param conf
 *  The pointer to th econfig file.
 * @param search
 *  A dot-notation key like "a.b.c.d".
 * @param len
 *  An optional autput buffer to store the string length.
 * @return
 *  The string value read from the file or NULL if value is not available.
 */
const char* smx_config_get_string( bson_t* conf, const char* search,
        unsigned int* len );

/** @} */
/** @} */

#endif /* SMXCONFIG_H */
