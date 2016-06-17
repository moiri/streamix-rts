#ifndef HANDLER_H
#define HANDLER_H

#include "pthread.h"

#define SMX_CHANNEL_IN( h, box, port ) smx_channel_in( h, BOX_##box##_##port )
#define SMX_CHANNEL_OUT( h, box, port, data ) smx_channel_out( h, BOX_##box##_##port, data )

typedef struct smx_channel_s
{
    void*   data;
    int     ready;
    pthread_mutex_t channel_mutex;
    pthread_cond_t  channel_cv;
} smx_channel_t;

typedef struct smx_box_s {
    int             th_id;
    smx_channel_t** in;
    smx_channel_t** out;
} smx_box_t;

void* smx_channel_in( void*, int );
void smx_channel_out( void*, int, void* );

#endif // HANDLER_H
