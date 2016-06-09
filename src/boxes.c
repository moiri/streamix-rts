#include "boxes.h"
#include <stdio.h>

enum COM_STATE { SYN, SYN_ACK, ACK, DONE };

void box_impl_a( strmx* handler ) {
    int state = SYN;
    int val = 33;
    void* data;
    while( state != DONE ) {
        switch( state ) {
            case SYN:
                data = STRMX_IN( handler, 0 );
                printf("in SYN: %d\n", *( int* )data );
                state = SYN_ACK;
                break;
            case SYN_ACK:
                STRMX_OUT( handler, 1, ( void* )&val );
                state = ACK;
                break;
            case ACK:
                data = STRMX_IN( handler, 2 );
                printf("in ACK: %d\n", *( int* )data );
                state = DONE;
                break;
            default:
                state = DONE;
        }
    }
}
