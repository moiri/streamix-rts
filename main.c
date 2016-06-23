/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "boxgen.h"
#include "smxrts.h"
#include <zlog.h>

int main( void )
{
    int rc = dzlog_init("test_default.conf", "my_cat");

    if( rc ) {
        printf("init failed\n");
        pthread_exit( NULL );
    }

    dzlog_info("start thread main");

    smx_channel_t** channels = SMX_CHANNELS_CREATE( 3 );
    channels[0] = SMX_CHANNEL_CREATE();
    channels[1] = SMX_CHANNEL_CREATE();
    channels[2] = SMX_CHANNEL_CREATE();

    SMX_BOX_INIT( a, channels );
    SMX_BOX_INIT( b, channels );
    SMX_BOX_CLEANUP( a );
    SMX_BOX_CLEANUP( b );

    SMX_CHANNEL_DESTROY( channels[0] );
    SMX_CHANNEL_DESTROY( channels[1] );
    SMX_CHANNEL_DESTROY( channels[2] );
    SMX_CHANNELS_DESTROY( channels );
    dzlog_info("end thread main");
    zlog_fini();
    pthread_exit( NULL );
}
