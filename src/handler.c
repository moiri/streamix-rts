#include "handler.h"

void* smx_channel_read( void* h, int idx )
{
    void* data;
    smx_channel_t* ch = ( ( smx_box_t* )h )->in[ idx ];
    pthread_mutex_lock( &ch->channel_mutex );
    while( ch->ready == 0 )
        pthread_cond_wait( &ch->channel_cv, &ch->channel_mutex );
    ch->ready = 0;
    data = ch->data;
    pthread_mutex_unlock( &ch->channel_mutex );
    return data;
}

void smx_channel_write( void* h, int idx, void* data )
{
    smx_channel_t* ch = ( ( smx_box_t* )h )->out[ idx ];
    pthread_mutex_lock( &ch->channel_mutex );
    ch->ready = 1;
    ch->data = data;
    pthread_cond_signal( &ch->channel_cv );
    pthread_mutex_unlock( &ch->channel_mutex );
}
