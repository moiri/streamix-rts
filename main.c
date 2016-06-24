/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "smxrts.h"

int main( void )
{
    SMX_PROGRAM_INIT();

    void* channels = SMX_CHANNELS_CREATE( 3 );

    SMX_BOX_INIT( a, channels );
    SMX_BOX_INIT( b, channels );
    SMX_BOX_CLEANUP( a );
    SMX_BOX_CLEANUP( b );

    SMX_CHANNELS_DESTROY( channels, 3 );

    SMX_PROGRAM_CLEANUP();
}
