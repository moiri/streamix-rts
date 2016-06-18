/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxgen.h"
#include "smxrts.h"
#include <pthread.h>
#include <zlog.h>
#include <unistd.h>

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

    pthread_create( &thread_a, NULL, box_impl_a, ( void* )a );
    pthread_create( &thread_b, NULL, box_impl_b, ( void* )b );

    pthread_join( thread_a, NULL );
    pthread_join( thread_b, NULL );

    smx_box_destroy( a );
    smx_box_destroy( b );
    smx_channel_destroy( syn );
    smx_channel_destroy( ack );
    smx_channel_destroy( syn_ack );
    dzlog_info("end thread main");
    zlog_fini();
    pthread_exit( NULL );
}
