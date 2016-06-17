#include "boxes.h"
#include "handler.h"
#include <stdio.h>
#include <stdlib.h>

enum COM_STATE { SYN, SYN_ACK, ACK, DONE };

void* box_impl_a( void* handler )
{
    int state = SYN;
    int* val = malloc( sizeof(int) );
    *val = 33;
    void *data;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                data = SMX_CHANNEL_IN( handler, a, syn );
                printf("in SYN: %d\n", *( int* )data );
                free( data );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                SMX_CHANNEL_OUT( handler, a, syn_ack, ( void* )val );
                state = ACK;
                break;
            case ACK:
                data = SMX_CHANNEL_IN( handler, a, ack );
                printf("in ACK: %d\n", *( int* )data );
                free( data );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
    pthread_exit( NULL );
}

void* box_impl_b( void* handler )
{
    int state = SYN;
    int* syn = malloc( sizeof(int) );
    *syn = 22;
    int* ack = malloc( sizeof(int) );
    *ack = 44;
    void *data;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                SMX_CHANNEL_OUT( handler, b, syn, ( void* )syn );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                data = SMX_CHANNEL_IN( handler, b, syn_ack );
                printf("in SYN_ACK: %d\n", *( int* )data );
                free( data );
                state = ACK;
                break;
            case ACK:
                SMX_CHANNEL_OUT( handler, b, ack, ( void* )ack );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
    pthread_exit( NULL );
}
