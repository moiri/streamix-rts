/**
 * @file     smxconfig.h
 * @author   Simon Maurer
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
#include "smxtypes.h"

#ifndef SMXCONFIG_H
#define SMXCONFIG_H

/**
 * This is the same as smx_config_get_bool_err() however without the err output
 * parameter.
 */
bool smx_config_get_bool( bson_t* conf, const char* search );

/**
 * Get a boolean value from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] err
 *  A pointer to a config error flag.
 * @return
 *  The boolean value read from the file or false if value is not available.
 */
bool smx_config_get_bool_err( bson_t* conf, const char* search,
        smx_config_error_t* err );

/**
 * This is the same as smx_config_get_int_err() however without the err output
 * parameter.
 */
int smx_config_get_int( bson_t* conf, const char* search );

/**
 * Get an int value from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] err
 *  A pointer to a config error flag.
 * @return
 *  The int value read from the file or 0 if value is not available.
 */
int smx_config_get_int_err( bson_t* conf, const char* search,
        smx_config_error_t* err );

/**
 * This is the same as smx_config_get_double_err() however without the err
 * output parameter.
 */
double smx_config_get_double( bson_t* conf, const char* search );

/**
 * Get a double value from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] err
 *  A pointer to a config error flag.
 * @return
 *  The double value read from the file or 0 if value is not available.
 */
double smx_config_get_double_err( bson_t* conf, const char* search,
        smx_config_error_t* err );

/**
 * This is the same as smx_config_get_string_err() however without the err
 * output parameter.
 */
const char* smx_config_get_string( bson_t* conf, const char* search,
        unsigned int* len );

/**
 * Get a string value from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] len
 *  An optional autput buffer to store the string length.
 * @param[out] err
 *  A pointer to a config error flag.
 * @return
 *  The string value read from the file or NULL if value is not available.
 */
const char* smx_config_get_string_err( bson_t* conf, const char* search,
        unsigned int* len, smx_config_error_t* err );

#endif /* SMXCONFIG_H */
