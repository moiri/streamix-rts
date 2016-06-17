#include "boxes.h"
#include "handler.h"
#include <stdio.h>
#include <pthread.h>

enum COM_STATE { SYN, SYN_ACK, ACK, DONE };

void *box_impl_a( void *handler )
{
    int state = SYN;
    int val = 33;
    void *data;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                data = smx_channel_in( handler, 0 );
                printf("in SYN: %d\n", *( int* )data );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                smx_channel_out( handler, 0, ( void* )&val );
                state = ACK;
                break;
            case ACK:
                data = smx_channel_in( handler, 1 );
                printf("in ACK: %d\n", *( int* )data );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
    pthread_exit( NULL );
}

void *box_impl_b( void *handler )
{
    int state = SYN;
    int syn = 22;
    int ack = 44;
    void *data;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                smx_channel_out( handler, 0, ( void* )&syn );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                data = smx_channel_in( handler, 0 );
                printf("in SYN_ACK: %d\n", *( int* )data );
                state = ACK;
                break;
            case ACK:
                smx_channel_out( handler, 1, ( void* )&ack );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
    pthread_exit( NULL );
}
