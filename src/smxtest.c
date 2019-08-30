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
        const char* port_name )
{
    char search_str[1000];
    static int idx = 0;
    bson_iter_t iter;
    bson_iter_t child;
    sprintf( search_str, "test_data.%s.%s.%d", mode, port_name,
            idx );
    if( bson_iter_init( &iter, h->conf ) && bson_iter_find_descendant( &iter,
                search_str, &child ) )
    {
        SMX_LOG_NET( h, debug, "reading test data from config node '%s'", search_str );
        idx++;
        return bson_iter_value( &child );
    }
    else
    {
        SMX_LOG_NET( h, debug, "failed to read test data from config node '%s'", search_str );
    }

    return NULL;
}
