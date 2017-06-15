#include "smxrts.h"
#include "pthread.h"
#include <stdlib.h>
#include <zlog.h>

smx_blackboard_t* smx_blackboard_create()
{
    smx_blackboard_t* bb = malloc( sizeof( struct smx_blackboard_s ) );
    bb->read = 0;
    bb->write = 1;
    bb->data = malloc( sizeof( void* ) * 2 );
    bb->data[bb->write] = NULL;
    bb->data[bb->read] = NULL;
    return bb;
}

void* smx_blackboard_read( smx_blackboard_t* bb )
{
    void* data;
    pthread_mutex_lock( &bb->mutex_read );
    data = bb->data[bb->read];
    dzlog_debug("read from blackboard %p (position: %d)", bb, bb->read );
    pthread_mutex_unlock( &bb->mutex_read );
    return data;
}

void smx_blackboard_write( smx_blackboard_t* bb, void* data )
{
    int read;
    pthread_mutex_lock( &bb->mutex_write );
    bb->data[bb->write] = data;
    read = bb->read;
    pthread_mutex_lock( &bb->mutex_read );
    bb->read = bb->write;
    pthread_mutex_unlock( &bb->mutex_read );
    bb->write = read;
    dzlog_debug("write to blackboard %p (position: %d)", bb, bb->write );
    pthread_mutex_unlock( &bb->mutex_write );
}

pthread_t smx_box_run( void* box_impl( void* ), void* arg )
{
    pthread_t thread;
    pthread_create( &thread, NULL, box_impl, arg );
    return thread;
}

smx_channel_t* smx_channel_create( int length )
{
    smx_channel_item_t* last_item = NULL;
    smx_channel_t* channel = malloc( sizeof( struct smx_channel_s ) );
    for( int i=0; i < length; i++ ) {
        channel->head = malloc( sizeof( struct smx_channel_item_s ) );
        channel->head->data = NULL;
        channel->head->prev = last_item;
        if( last_item == NULL )
            channel->tail = channel->head;
        else
            last_item->next = channel->head;
        last_item = channel->head;
    }
    channel->head->next = channel->tail;
    channel->tail->prev = channel->head;
    channel->tail = channel->head;
    pthread_mutex_init( &channel->channel_mutex, NULL );
    pthread_cond_init( &channel->channel_cv, NULL );
    channel->count = 0;
    channel->length = length;
    return channel;
}

void smx_channel_destroy( smx_channel_t* channel )
{
    pthread_mutex_destroy( &( (smx_channel_t* )channel )->channel_mutex );
    pthread_cond_destroy( &( ( smx_channel_t* )channel )->channel_cv );
    for( int i=0; i < channel->length; i++ ) {
        channel->tail = channel->head;
        channel->head = channel->head->next;
        free( channel->tail );
    }
    free( channel );
}

void* smx_channel_read( smx_channel_t* ch )
{
    void* data;
    pthread_mutex_lock( &ch->channel_mutex );
    while( ch->count == 0 )
        pthread_cond_wait( &ch->channel_cv, &ch->channel_mutex );
    data = ch->head->data;
    ch->head = ch->head->prev;
    ch->count--;
    dzlog_debug("read from channel %p (new count: %d)", ch, ch->count );
    pthread_cond_signal( &ch->channel_cv );
    pthread_mutex_unlock( &ch->channel_mutex );
    return data;
}

void smx_channel_write( smx_channel_t* ch, void* data )
{
    pthread_mutex_lock( &ch->channel_mutex );
    while( ch->count == ch->length )
        pthread_cond_wait( &ch->channel_cv, &ch->channel_mutex );
    ch->tail->data = data;
    ch->tail = ch->tail->prev;
    ch->count++;
    dzlog_debug("write to channel %p (new count: %d)", ch, ch->count );
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
