#include "smxrts.h"
#include "pthread.h"
#include <stdlib.h>
#include <zlog.h>

pthread_t smx_box_run( void* box_impl( void* ), void* arg )
{
    pthread_t thread;
    pthread_create( &thread, NULL, box_impl, arg );
    return thread;
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

void* smx_channel_read( smx_channel_t* ch )
{
    void* data;
    pthread_mutex_lock( &ch->channel_mutex );
    while( ch->ready == 0 )
        pthread_cond_wait( &ch->channel_cv, &ch->channel_mutex );
    ch->ready = 0;
    data = ch->data;
    pthread_mutex_unlock( &ch->channel_mutex );
    return data;
}

void smx_channel_write( smx_channel_t* ch, void* data )
{
    pthread_mutex_lock( &ch->channel_mutex );
    ch->ready = 1;
    ch->data = data;
    pthread_cond_signal( &ch->channel_cv );
    pthread_mutex_unlock( &ch->channel_mutex );
}

smx_channel_t** smx_channels_create( int count )
{
    int i;
    smx_channel_t** channels = malloc( sizeof( smx_channel_t* ) * count );
    for( i = 0; i < count; i++ )
        channels[i] = SMX_CHANNEL_CREATE();
    return channels;
}

void smx_channels_destroy( smx_channel_t** channels, int count )
{
    int i;
    for( i = 0; i < count; i++ )
        SMX_CHANNEL_DESTROY( channels[i] );
    free( channels );
}

void smx_program_init()
{
    int rc = dzlog_init("test_default.conf", "my_cat");

    if( rc ) {
        printf("init failed\n");
        pthread_exit( NULL );
    }

    dzlog_info("start thread main");
}

void smx_program_cleanup()
{
    dzlog_info("end thread main");
    zlog_fini();
    pthread_exit( NULL );
}
