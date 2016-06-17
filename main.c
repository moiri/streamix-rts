/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxes.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int main( void )
{
    pthread_t thread_a;
    smx_box_t* ha = malloc( sizeof( struct smx_box_s ) );
    pthread_t thread_b;
    smx_box_t* hb = malloc( sizeof( struct smx_box_s ) );
    smx_channel_t* syn = malloc( sizeof( struct smx_channel_s ) );
    smx_channel_t* ack = malloc( sizeof( struct smx_channel_s ) );
    smx_channel_t* syn_ack = malloc( sizeof( struct smx_channel_s ) );

    ha->in = malloc( sizeof( smx_channel_t* ) * 2 );
    ha->in[0] = syn;
    ha->in[1] = syn_ack;
    ha->out = malloc( sizeof( smx_channel_t* ) );
    ha->out[0] = ack;
    ha->th_id = pthread_create( &thread_a, NULL, box_impl_a, ( void* )ha );

    hb->in = malloc( sizeof( void* ) );
    hb->in[0] = ack;
    hb->out = malloc( sizeof( void* ) * 2 );
    hb->out[0] = syn;
    hb->out[1] = syn_ack;
    hb->th_id = pthread_create( &thread_b, NULL, box_impl_b, ( void* )hb );

    pthread_exit( NULL );
    return 0;
}
