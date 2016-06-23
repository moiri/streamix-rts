#include "boximpl.h"
#include "smxrts.h"
#include <stdlib.h>
#include <zlog.h>

enum com_state_e { SYN, SYN_ACK, ACK, DONE };

void a( void* handler )
{
    int state = SYN;
    int* data = NULL;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                data = ( int* )SMX_CHANNEL_READ( handler, a, syn );
                dzlog_info( "a, received data: %d", *data );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                *data -= 3;
                SMX_CHANNEL_WRITE( handler, a, syn_ack, ( void* )data );
                state = ACK;
                break;
            case ACK:
                data = ( int* )SMX_CHANNEL_READ( handler, a, ack );
                dzlog_info( "a, received data: %d", *data );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
    free( data );
}

void b( void* handler )
{
    int state = SYN;
    int* data = malloc( sizeof( int ) );
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                *data = 42;
                SMX_CHANNEL_WRITE( handler, b, syn, ( void* )data );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                data = ( int* )SMX_CHANNEL_READ( handler, b, syn_ack );
                dzlog_info( "b, received data: %d", *data );
                state = ACK;
                break;
            case ACK:
                *data += 5;
                SMX_CHANNEL_WRITE( handler, b, ack, ( void* )data );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
}
