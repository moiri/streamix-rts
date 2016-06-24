/**
 * Firts stepts to play with the interface between a box and its implementation
 */

#include "smxrts.h"

int main( void )
{
    SMX_PROGRAM_INIT();

    void* ch_0_0 = SMX_CHANNEL_CREATE();
    void* ch_1_0 = SMX_CHANNEL_CREATE();
    void* ch_2_0 = SMX_CHANNEL_CREATE();

    void* box_0_0 = SMX_BOX_CREATE( a );
    SMX_CONNECT( box_0_0, ch_0_0, a, syn );
    SMX_CONNECT( box_0_0, ch_1_0, a, ack );
    SMX_CONNECT( box_0_0, ch_2_0, a, syn_ack );

    void* box_1_0 = SMX_BOX_CREATE( b );
    SMX_CONNECT( box_1_0, ch_0_0, b, syn );
    SMX_CONNECT( box_1_0, ch_1_0, b, ack );
    SMX_CONNECT( box_1_0, ch_2_0, b, syn_ack );

    SMX_BOX_RUN( a, box_0_0 );
    SMX_BOX_RUN( b, box_1_0 );

    SMX_BOX_WAIT_END( a );
    SMX_BOX_DESTROY( box_0_0 );

    SMX_BOX_WAIT_END( b );
    SMX_BOX_DESTROY( box_1_0 );

    SMX_CHANNEL_DESTROY( ch_0_0 );
    SMX_CHANNEL_DESTROY( ch_1_0 );
    SMX_CHANNEL_DESTROY( ch_2_0 );

    SMX_PROGRAM_CLEANUP();
}
