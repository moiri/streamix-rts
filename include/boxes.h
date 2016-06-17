#ifndef BOXES_H
#define BOXES_H

/**
 * Simple example of a connection initiation
 */

typedef enum box_a_in_e { BOX_a_syn, BOX_a_ack } box_a_in_t;
typedef enum box_a_out_e { BOX_a_syn_ack } box_a_out_t;
void *box_impl_a( void* );

typedef enum box_b_in_e { BOX_b_syn_ack } box_b_in_t;
typedef enum box_b_out_e { BOX_b_syn, BOX_b_ack } box_b_out_t;
void *box_impl_b( void* );

#endif // BOXES_H
