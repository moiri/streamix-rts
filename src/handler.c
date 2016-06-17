#include "handler.h"

void* smx_channel_in( void* h, int idx )
{
    smx_channel_t* ch = ( ( smx_box_t* )h )->in[ idx ];
    pthread_mutex_lock( &ch->ready_mutex );
    while( ch->ready == 0 )
        pthread_cond_wait( &ch->ready_cv, &ch->ready_mutex );
    ch->ready = 0;
    pthread_mutex_unlock( &ch->ready_mutex );
    return ch->data;
}

void smx_channel_out( void* h, int idx, void* data )
{
    smx_channel_t* ch = ( ( smx_box_t* )h )->out[ idx ];
    ch->data = data;
    pthread_mutex_lock( &ch->ready_mutex );
    ch->ready = 1;
    pthread_cond_signal( &ch->ready_cv );
    pthread_mutex_unlock( &ch->ready_mutex );
}
