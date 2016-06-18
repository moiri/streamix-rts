#ifndef BOXGEN_H
#define BOXGEN_H

/**
 * Simple example of a connection initiation
 */

typedef enum box_a_e { PORT_a_syn, PORT_a_ack, PORT_a_syn_ack } box_a_t;
void *box_impl_a( void* );

typedef enum box_b_e { PORT_b_syn_ack, PORT_b_syn, PORT_b_ack } box_b_t;
void *box_impl_b( void* );

#endif // BOXGEN_H
