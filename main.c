/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxgen.h"
#include "smxrts.h"
#include <pthread.h>



int main( void )
{
    pthread_t thread_a;
    pthread_t thread_b;
    smx_box_t* a = smx_box_create( 3 );
    smx_box_t* b = smx_box_create( 3 );
    smx_channel_t* syn = smx_channel_create();
    smx_channel_t* ack = smx_channel_create();
    smx_channel_t* syn_ack = smx_channel_create();
    SMX_CONNECT( syn, a );
    SMX_CONNECT( syn, b );
    SMX_CONNECT( ack, a );
    SMX_CONNECT( ack, b );
    SMX_CONNECT( syn_ack, a );
    SMX_CONNECT( syn_ack, b );

    a->th_id = pthread_create( &thread_a, NULL, box_impl_a, ( void* )a );
    b->th_id = pthread_create( &thread_b, NULL, box_impl_b, ( void* )b );

    pthread_exit( NULL );
}
