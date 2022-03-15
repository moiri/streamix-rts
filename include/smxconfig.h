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
 * Perform a value mapping defined in a key map.
 *
 * @param maps
 *  A pointer to the initialised key map list.
 * @param src_payload
 *  A pointer to the source payload.
 * @return
 *  0 on success, an error code on failure use smx_config_map_strerror().
 */
int smx_config_data_maps_apply( smx_config_data_maps_t* maps,
        bson_t* src_payload );

/**
 * Perform a basic value mapping. This only works for booelans, integers and
 * floating point values. To handle ather type mappings the extended mapping
 * version must be used.
 *
 * @param key_map
 *  A pointer to a key map item.
 * @param src_payload
 *  A pointer to the source payload.
 * @return
 *  0 on success, an error code on failure use smx_config_map_strerror().
 */
int smx_config_data_maps_apply_base( smx_config_data_map_t* key_map,
        bson_t* src_payload );

/**
 * Perform an extended value mapping.
 *
 * @param maps
 *  A pointer to the mapping list.
 * @param src_payload
 *  A pointer to the source payload.
 * @return
 *  0 on success, an error code on failure use smx_config_map_strerror().
 */
int smx_config_data_maps_apply_ext( smx_config_data_maps_t* maps,
        bson_t* src_payload );

/**
 * Recuresvly itereate through the target payload to reconstruct it with the
 * mapped values from the source payload.
 *
 * @param src_payload
 *  A pointer to the source payload.
 * @param payload
 *  A pointer to the new payload.
 * @param key
 *  A dot-separated key pointing to the current iteration parent.
 * @param maps
 *  A pointer to the mapping list.
 * @return
 *  0 on success, an error code on failure use smx_config_map_strerror().
 */
int smx_config_data_maps_apply_ext_iter( bson_iter_t* i_tgt,
        bson_t* src_payload, bson_t* payload, const char* key,
        smx_config_data_maps_t* maps );

/**
 * Free all maps allocated in a map array
 *
 * @param maps
 *  A pointer to the maps structure
 */
void smx_config_data_maps_cleanup( smx_config_data_maps_t* maps );

/**
 * Returns a pointer to the mapped payload.
 * Make sure to run smx_config_data_maps_apply() to update the payload.
 *
 * @param maps
 *  A pointer to the mapping list.
 * @return
 *  A pointer to the mapped payload.
 */
bson_t* smx_config_data_maps_get_mapped_payload( smx_config_data_maps_t* maps );

/**
 *
 */
smx_config_data_map_t* smx_config_data_maps_get_map_by_key(
        smx_config_data_maps_t* maps, const char* key );

/**
 * Initialise an input-output key map array.
 *
 * @param i_fields
 *  The document iterator of the map list definition.
 * @param data
 *  A pointer to the target document structure.
 * @param maps
 *  A pointer to an initialized maps structure.
 */
int smx_config_data_maps_init( bson_iter_t* i_fields, bson_t* data,
        smx_config_data_maps_t* maps );

int smx_config_data_maps_init_raw( bson_t* data,
        smx_config_data_maps_t* maps );

/**
 * Append a mapped value if defined in the mapping list.
 *
 * @param dot_key
 *  The dot-seperated key to the target value.
 * @param iter_key
 *  The key of the current target iterator.
 * @param src_payload
 *  A pointer to the source payload.
 * @param payload
 *  A pointer to the new payload.
 * @param maps
 *  A pointer to the mapping list.
 * return
 *  0 on success or -1 on failure.
 */
int smx_config_data_map_append_val( const char* dot_key,
        const char* iter_key, bson_t* src_payload, bson_t* payload,
        smx_config_data_maps_t* maps );

/**
 * Check whether the value at the source payload can be mapped to a boolean
 * value at the target payload.
 *
 * @param i_src
 *  The source payload iterator pointing to the field to be checked.
 * @return
 *  True if the value can be mapped, false otherwise.
 */
bool smx_config_data_map_can_write_bool( bson_iter_t* i_src );

/**
 * Check whether the value at the source payload can be mapped to a double
 * value at the target payload.
 *
 * @param i_src
 *  The source payload iterator pointing to the field to be checked.
 * @return
 *  True if the value can be mapped, false otherwise.
 */
bool smx_config_data_map_can_write_double( bson_iter_t* i_src );

/**
 * Check whether the value at the source payload can be mapped to a int32
 * value at the target payload.
 *
 * @param i_src
 *  The source payload iterator pointing to the field to be checked.
 * @return
 *  True if the value can be mapped, false otherwise.
 */
bool smx_config_data_map_can_write_int32( bson_iter_t* i_src );

/**
 * Check whether the value at the source payload can be mapped to a int64
 * value at the target payload.
 *
 * @param i_src
 *  The source payload iterator pointing to the field to be checked.
 * @return
 *  True if the value can be mapped, false otherwise.
 */
bool smx_config_data_map_can_write_int64( bson_iter_t* i_src );

/**
 * Get the value location in the output document. A key map value can either be
 * of type bool, int64, int32, or double.
 *
 * @param data
 *  The output document
 * @param map
 *  The target path to the output value (dot-notation).
 * @param child
 *  An output parameter to store the iterator
 * @return
 *  True if key exists fals otherwise
 */
bool smx_config_data_map_get_iter( bson_t* data, const char* map,
        bson_iter_t* child );

/**
 * Initialise an input output key mapping.
 *
 * @param payload
 *  A pointer to the output document.
 * @param i_map
 *  The document iterator for the subdocument in the configuration file
 *  pointing to a mapping.
 * @param is_extended
 *  An output parameter where the is_extended flag will be stored. This flag
 *  is set to true if one of the mapping uses extended functionality (e.g.
 *  mapping strings or explicit types).
 * @param map
 *  A pointer to an initialized mapping structure.
 * @return
 *  0 on success, an error code on failure use smx_config_map_strerror().
 */
int smx_config_data_map_init( bson_t* payload, bson_iter_t* i_map,
        bool* is_extended, smx_config_data_map_t* map );

/**
 * Initialise the key map target item if it is of type DOCUMENT.
 *
 * @param payload
 *  A pointer to the mapped payload document.
 * @param map
 *  A pointer to the current map item.
 * @param i_tgt
 *  The document iterator for the subdocument in the configuration file
 *  pointing to a mapping target.
 * @param is_extended
 *  An output parameter where the is_extended flag will be stored. This flag
 *  is set to true if one of the mapping uses extended functionality (e.g.
 *  mapping strings or explicit types).
 * @return
 *  0 on success, -1 on failure.
 */
int smx_config_data_map_init_tgt_doc( bson_t* payload,
        smx_config_data_map_t* map, bson_iter_t* i_tgt, bool* is_extended );

/**
 * Initialise the key map target item if it is of type UTF8.
 *
 * @param payload
 *  A pointer to the mapped payload document.
 * @param map
 *  A pointer to the current map item.
 * @param i_map
 *  The document iterator for the subdocument in the configuration file
 *  pointing to a mapping target.
 * @param is_extended
 *  An output parameter where the is_extended flag will be stored. This flag
 *  is set to true if one of the mapping uses extended functionality (e.g.
 *  mapping strings or explicit types).
 * @return
 *  0 on success, -1 on failure.
 */
int smx_config_data_map_init_tgt_utf8( bson_t* payload,
        smx_config_data_map_t* map, const char* tgt_path, bool* is_extended );

/**
 * Return a humanreadable error string, given a an error code.
 *
 * @param code
 *  The error code to translate.
 * @return
 *  The error string.
 */
const char* smx_config_data_map_strerror( int code );

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

/**
 * Get a boolean from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] val
 *  An output buffer to store the boolean value. This is only valid of the
 *  function returns successfully.
 * @return
 *  A negative error code or 0 on success.
 */
int smx_config_init_bool( bson_t* conf, const char* search, bool* val );

/**
 * Get a double from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] val
 *  An output buffer to store the double value. This is only valid of the
 *  function returns successfully.
 * @return
 *  A negative error code or 0 on success.
 */
int smx_config_init_double( bson_t* conf, const char* search, double* val );

/**
 * Get an int from the config file.
 *
 * @param[in] conf
 *  The pointer to th econfig file.
 * @param[in] search
 *  A dot-notation key like "a.b.c.d".
 * @param[out] val
 *  An output buffer to store the int value. This is only valid of the
 *  function returns successfully.
 * @return
 *  A negative error code or 0 on success.
 */
int smx_config_init_int( bson_t* conf, const char* search, int* val );

/**
 * Return a human-readable error message, give an error code.
 *
 * @param err
 *  The error code to transform.
 * @return
 *  A human-readable error message.
 */
const char* smx_config_strerror( smx_config_error_t err );

#endif /* SMXCONFIG_H */
