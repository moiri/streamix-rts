#include "handler.h"
#include <stdlib.h>

smx_box_t* smx_box_create( int port_count )
{
    smx_box_t* box = malloc( sizeof( struct smx_box_s ) );

    box->ports = malloc( sizeof( smx_channel_t* ) * port_count );
    return box;
}

void smx_box_destroy( void* box )
{
    free( ( ( smx_box_t* )box )->ports );
    free( box );
}

smx_channel_t* smx_channel_create( void )
{
    smx_channel_t* channel = malloc( sizeof( struct smx_channel_s ) );
    pthread_mutex_init( &channel->channel_mutex, NULL );
    pthread_cond_init( &channel->channel_cv, NULL );
    channel->data = NULL;
    channel->ready = 0;
    return channel;
}

void smx_channel_destroy( void* channel )
{
    pthread_mutex_destroy( &( (smx_channel_t* )channel )->channel_mutex );
    pthread_cond_destroy( &( ( smx_channel_t* )channel )->channel_cv );
    free( channel );
}

void* smx_channel_read( void* h, int idx )
{
    void* data;
    smx_channel_t* ch = ( ( smx_box_t* )h )->ports[ idx ];
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
    smx_channel_t* ch = ( ( smx_box_t* )h )->ports[ idx ];
    pthread_mutex_lock( &ch->channel_mutex );
    ch->ready = 1;
    ch->data = data;
    pthread_cond_signal( &ch->channel_cv );
    pthread_mutex_unlock( &ch->channel_mutex );
}
