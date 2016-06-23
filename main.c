/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxgen.h"
#include "smxrts.h"
#include <pthread.h>
#include <zlog.h>
#include <unistd.h>
#include <stdlib.h>

int main( void )
{
    int rc = dzlog_init("test_default.conf", "my_cat");

    if( rc ) {
        printf("init failed\n");
        pthread_exit( NULL );
    }

    dzlog_info("start thread main");

    pthread_t thread_a;
    pthread_t thread_b;
    void* box1 = SMX_BOX_CREATE( a );
    void* box2 = SMX_BOX_CREATE( b );
    smx_channel_t* ch1 = SMX_CHANNEL_CREATE();
    smx_channel_t* ch2 = SMX_CHANNEL_CREATE();
    smx_channel_t* ch3 = SMX_CHANNEL_CREATE();
    SMX_CONNECT( box1, ch1, a, syn );
    SMX_CONNECT( box1, ch2, a, ack );
    SMX_CONNECT( box1, ch3, a, syn_ack );
    SMX_CONNECT( box2, ch1, b, syn );
    SMX_CONNECT( box2, ch2, b, ack );
    SMX_CONNECT( box2, ch3, b, syn_ack );

    pthread_create( &thread_a, NULL, box_impl_a, box1 );
    pthread_create( &thread_b, NULL, box_impl_b, box2 );

    pthread_join( thread_a, NULL );
    pthread_join( thread_b, NULL );

    SMX_BOX_DESTROY( box1 );
    SMX_BOX_DESTROY( box2 );
    SMX_CHANNEL_DESTROY( ch1 );
    SMX_CHANNEL_DESTROY( ch2 );
    SMX_CHANNEL_DESTROY( ch3 );
    dzlog_info("end thread main");
    zlog_fini();
    pthread_exit( NULL );
}
