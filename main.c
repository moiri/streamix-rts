/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxes.h"
#include "handler.h"
#include <stdlib.h>
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
    ha->in[PORT_a_syn] = syn;
    ha->in[PORT_a_ack] = ack;
    ha->out = malloc( sizeof( smx_channel_t* ) );
    ha->out[PORT_a_syn_ack] = syn_ack;
    ha->th_id = pthread_create( &thread_a, NULL, box_impl_a, ( void* )ha );

    hb->in = malloc( sizeof( void* ) );
    hb->in[PORT_b_syn_ack] = syn_ack;
    hb->out = malloc( sizeof( void* ) * 2 );
    hb->out[PORT_b_syn] = syn;
    hb->out[PORT_b_ack] = ack;
    hb->th_id = pthread_create( &thread_b, NULL, box_impl_b, ( void* )hb );

    pthread_exit( NULL );
    return 0;
}
