/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * A test environment that allows to test box implementation without having to
 * run them inside a streamix application.
 */

#include "smxtest.h"
#include "smxlog.h"

/*****************************************************************************/
const bson_value_t* smx_read_test_data( smx_net_t* h, const char* mode,
        const char* port_name, smx_channel_end_t* end )
{
    char search_str[1000];
    bson_iter_t iter;
    bson_iter_t child;
    if( end == NULL )
    {
        SMX_LOG_NET( h, error, "failed to read test data from %sput port '%s':"
                " channel end not connected", mode, port_name );
        return NULL;
    }
    sprintf( search_str, "test_data.%s.%s.%lu", mode, port_name,
            end->count );
    end->count++;
    if( bson_iter_init( &iter, h->conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) )
    {
        SMX_LOG_NET( h, debug, "reading test data from config node '%s'",
                search_str );
        return bson_iter_value( &child );
    }
    else
    {
        SMX_LOG_NET( h, debug, "failed to read test data from config node '%s'",
                search_str );
    }

    return NULL;
}

/*****************************************************************************/
int smx_read_test_value( smx_net_t* h, const char* mode,
        const char* port_name, smx_channel_end_t* end, bson_value_t* data )
{
    char search_str[1000];
    bson_iter_t iter;
    bson_iter_t child;
    const bson_value_t* data_iter;
    if( end == NULL )
    {
        SMX_LOG_NET( h, error, "failed to read test data from %sput port '%s':"
                " channel end not connected", mode, port_name );
        return -1;
    }
    sprintf( search_str, "test_data.%s.%s.%lu", mode, port_name,
            end->count );
    end->count++;
    if( bson_iter_init( &iter, h->conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) )
    {
        SMX_LOG_NET( h, debug, "reading test data from config node '%s'",
                search_str );
        data_iter = bson_iter_value( &child );
        bson_value_copy( data_iter, data );

        return 0;
    }
    else
    {
        SMX_LOG_NET( h, debug, "failed to read test data from config node '%s'",
                search_str );
    }

    return -1;
}
