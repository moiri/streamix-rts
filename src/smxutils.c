/**
 * utility functions for the runtime system library of Streamix
 *
 * @file    smxutils.c
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <string.h>
#include "smxlog.h"
#include "smxutils.h"

/*****************************************************************************/
void* smx_malloc( size_t size )
{
    void* mem = malloc( size );
    if( mem == NULL )
        SMX_LOG_MAIN( main, fatal, "unable to allocate memory: %s",
                strerror( errno ) );
    return mem;
}
