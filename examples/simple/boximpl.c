#include "boximpl.h"
#include "boxgen.h"
#include "smxrts.h"
#include <stdlib.h>
#include <zlog.h>

enum com_state_e { SYN, SYN_ACK, ACK, DONE };

void* a( void* handler )
{
    char* data_x = malloc( sizeof( char ) );
    char* data_y = malloc( sizeof( char ) );
    *data_x = 'x';
    *data_y = 'y';
    SMX_CHANNEL_WRITE( handler, A, x, ( void* )data_x );
    SMX_CHANNEL_WRITE( handler, A, y, ( void* )data_y );
    return NULL;
}

void* b( void* handler )
{
    char* data = NULL;
    data = ( char* )SMX_CHANNEL_READ( handler, B, y );
    dzlog_info( "b, received data_y: %c", *data );
    free( data );
    data = ( char* )SMX_CHANNEL_READ( handler, B, x );
    dzlog_info( "b, received data_x: %c", *data );
    free( data );
    return NULL;
}
