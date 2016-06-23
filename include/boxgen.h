#ifndef BOXGEN_H
#define BOXGEN_H

#include "smxrts.h"

/**
 * Simple example of a connection initiation
 */

typedef struct box_impl_a_s {
    smx_channel_t*  port_syn;
    smx_channel_t*  port_ack;
    smx_channel_t*  port_syn_ack;
} box_impl_a_t;
typedef struct box_impl_b_s {
    smx_channel_t*  port_ack;
    smx_channel_t*  port_syn_ack;
    smx_channel_t*  port_syn;
} box_impl_b_t;

void *box_impl_a( void* );
void *box_impl_b( void* );

#endif // BOXGEN_H
