#ifndef BOXGEN_H
#define BOXGEN_H

/**
 * Simple example of a connection initiation
 */

typedef struct box_a_s {
    void*  port_syn;
    void*  port_ack;
    void*  port_syn_ack;
} box_a_t;
typedef struct box_b_s {
    void*  port_ack;
    void*  port_syn_ack;
    void*  port_syn;
} box_b_t;

void *box_a( void* );
void *box_b( void* );

#endif // BOXGEN_H
