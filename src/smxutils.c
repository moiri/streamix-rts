/**
 * utility functions for the runtime system library of Streamix
 *
 * @file    smxutils.c
 * @author  Simon Maurer
 */

#include <errno.h>
#include <string.h>
#include "smxutils.h"
#include "smxlog.h"

/*****************************************************************************/
void* smx_malloc( size_t size )
{
    void* mem = malloc( size );
    if( mem == NULL )
        SMX_LOG_MAIN( main, fatal, "unable to allocate memory: %s",
                strerror( errno ) );
    return mem;
}
