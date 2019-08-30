/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Helper functions for parsing configuration files of the runtime systemr
 * library of Streamix.
 */

#include <stdarg.h>
#include "smxconfig.h"

bool smx_config_get_bool( bson_t* conf, const char* search )
{
    bson_iter_t iter;
    bson_iter_t child;
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search, &child ) && BSON_ITER_HOLDS_BOOL( &iter ) )
    {
        return bson_iter_bool( &child );
    }
    return false;
}

int smx_config_get_int( bson_t* conf, const char* search )
{
    bson_iter_t iter;
    bson_iter_t child;
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search, &child ) && BSON_ITER_HOLDS_INT32( &iter ) )
    {
        return bson_iter_int32( &child );
    }
    return 0;
}

double smx_config_get_double( bson_t* conf, const char* search )
{
    bson_iter_t iter;
    bson_iter_t child;
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search, &child ) && BSON_ITER_HOLDS_DOUBLE( &iter ) )
    {
        return bson_iter_double( &child );
    }
    return 0;
}

const char* smx_config_get_string( bson_t* conf, const char* search,
        unsigned int* len )
{
    bson_iter_t iter;
    bson_iter_t child;
    if( bson_iter_init( &iter, conf ) && bson_iter_find_descendant( &iter,
                search, &child ) && BSON_ITER_HOLDS_UTF8( &iter ) )
    {
        return bson_iter_utf8( &child, len );
    }
    return NULL;
}
