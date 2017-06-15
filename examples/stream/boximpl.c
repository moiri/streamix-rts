#include "boximpl.h"
#include "boxgen.h"
#include "smxrts.h"
#include <stdlib.h>
#include <zlog.h>
#include <unistd.h>

enum com_state_e { SYN, SYN_ACK, ACK, DONE };

void* a( void* handler )
{
    char* data_x;
    char ch;
    while(read(STDIN_FILENO, &ch, 1) > 0)
    {
        data_x = malloc( sizeof( char ) );
        *data_x = ch;
        SMX_CHANNEL_WRITE( handler, A, x, ( void* )data_x );
    }
    return NULL;
}

void* b( void* handler )
{
    char* data = NULL;
    while(1) {
        data = ( char* )SMX_CHANNEL_READ( handler, B, x );
        dzlog_info( "b, received data_x: %c", *data );
        free( data );
        sleep(1);
    }
    return NULL;
}
