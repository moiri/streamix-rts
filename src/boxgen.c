#include "boxgen.h"
#include "boximpl.h"
#include "smxrts.h"
#include <zlog.h>

void* box_a( void* handler )
{
    dzlog_info("start thread a");
    box_a_t* box = SMX_BOX_CREATE( a );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[0], syn );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[1], ack );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[2], syn_ack );
    a( box );
    SMX_BOX_DESTROY( box );
    dzlog_info("end thread a");
    return NULL;
}

void* box_b( void* handler )
{
    dzlog_info("start thread b");
    box_b_t* box = SMX_BOX_CREATE( b );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[0], syn );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[1], ack );
    SMX_CONNECT( box, ( ( smx_channel_t** )handler )[2], syn_ack );
    b( box );
    SMX_BOX_DESTROY( box );
    dzlog_info("end thread b");
    return NULL;
}
