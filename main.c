/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxes.h"
#include <stdlib.h>
#include <stdio.h>

int main( void )
{
    int syn = 22;
    int ack = 44;
    strmx* ha = malloc( sizeof( strmx* ) );
    ha->box = 1;
    ha->value = malloc( sizeof( void* )*3 );
    ha->value[0] = ( void* )&syn;
    ha->value[2] = ( void* )&ack;
    box_impl_a( ha );
    printf("out ACK_SYN: %d\n", *( int* )ha->value[1] );
    return 0;
}
