#include "smxrts.h"
#include "pthread.h"
#include <stdlib.h>
#include <zlog.h>

/*****************************************************************************/
smx_blackboard_t* smx_blackboard_create()
{
    smx_blackboard_t* bb = malloc( sizeof( struct smx_blackboard_s ) );
    bb->read = 0;
    bb->write = 1;
    bb->msgs = malloc( sizeof( smx_msg_t* ) * 2 );
    bb->msgs[bb->write] = NULL;
    bb->msgs[bb->read] = NULL;
    return bb;
}

/*****************************************************************************/
void smx_blackboard_destroy( smx_blackboard_t* bb )
{
    free( bb->msgs );
    free( bb );
}

/*****************************************************************************/
smx_msg_t* smx_blackboard_read( smx_blackboard_t* bb )
{
    smx_msg_t* msg;
    pthread_mutex_lock( &bb->mutex_read );
    msg = bb->msgs[bb->read];
    dzlog_debug("read from blackboard %p (position: %d)", bb, bb->read );
    pthread_mutex_unlock( &bb->mutex_read );
    return msg;
}

/*****************************************************************************/
void smx_blackboard_write( smx_blackboard_t* bb, smx_msg_t* msg )
{
    int read;
    pthread_mutex_lock( &bb->mutex_write );
    bb->msgs[bb->write] = msg;
    read = bb->read;
    pthread_mutex_lock( &bb->mutex_read );
    bb->read = bb->write;
    pthread_mutex_unlock( &bb->mutex_read );
    bb->write = read;
    dzlog_debug("write to blackboard %p (position: %d)", bb, bb->write );
    pthread_mutex_unlock( &bb->mutex_write );
}

/*****************************************************************************/
void* box_smx_cp( void* handler )
{
    int i, count;
    smx_channel_t** ports;
    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    ports = ( ( box_smx_cp_t* )handler )->in.ports;
    msg = smx_fifo_read( ports[0]->ch_fifo );
    count = ( ( box_smx_cp_t* )handler )->out.count;
    ports = ( ( box_smx_cp_t* )handler )->out.ports;
    for( i=0; i<count; i++ ) {
        msg_copy = SMX_MSG_CREATE( msg->init, msg->copy, msg->destroy );
        msg_copy->data = msg->copy( msg->data );
        smx_fifo_write( ports[i]->ch_fifo, msg_copy );
    }
    return NULL;
}

/*****************************************************************************/
pthread_t smx_box_run( void* box_impl( void* ), void* arg )
{
    pthread_t thread;
    pthread_create( &thread, NULL, box_impl, arg );
    return thread;
}

/*****************************************************************************/
smx_channel_t* smx_channel_create( int len, smx_channel_type_t type )
{
    smx_channel_t* ch = malloc( sizeof( struct smx_channel_s ) );
    ch->type = type;
    switch( type ){
        case SMX_FIFO:
            ch->ch_fifo = smx_fifo_create( len );
            break;
        case SMX_BLACKBOARD:
            ch->ch_bb = smx_blackboard_create();
            break;
    }
    return ch;
}

/*****************************************************************************/
smx_fifo_t* smx_fifo_create( int length )
{
    smx_fifo_item_t* last_item = NULL;
    smx_fifo_t* fifo = malloc( sizeof( struct smx_fifo_s ) );
    for( int i=0; i < length; i++ ) {
        fifo->head = malloc( sizeof( struct smx_fifo_item_s ) );
        fifo->head->msg = NULL;
        fifo->head->prev = last_item;
        if( last_item == NULL )
            fifo->tail = fifo->head;
        else
            last_item->next = fifo->head;
        last_item = fifo->head;
    }
    fifo->head->next = fifo->tail;
    fifo->tail->prev = fifo->head;
    fifo->tail = fifo->head;
    pthread_mutex_init( &fifo->fifo_mutex, NULL );
    pthread_cond_init( &fifo->fifo_cv, NULL );
    fifo->count = 0;
    fifo->length = length;
    return fifo;
}

/*****************************************************************************/
void smx_channel_destroy( smx_channel_t* ch )
{
    switch( ch->type ) {
        case SMX_FIFO:
            smx_fifo_destroy( ch->ch_fifo );
            break;
        case SMX_BLACKBOARD:
            smx_blackboard_destroy( ch->ch_bb );
            break;
    }
    free( ch );
}

/*****************************************************************************/
void smx_fifo_destroy( smx_fifo_t* fifo )
{
    pthread_mutex_destroy( &( (smx_fifo_t* )fifo )->fifo_mutex );
    pthread_cond_destroy( &( ( smx_fifo_t* )fifo )->fifo_cv );
    for( int i=0; i < fifo->length; i++ ) {
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    free( fifo );
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( smx_channel_t* ch )
{
    smx_msg_t* msg;
    switch( ch->type ) {
        case SMX_FIFO:
            msg = smx_fifo_read( ch->ch_fifo );
            break;
        case SMX_BLACKBOARD:
            msg = smx_blackboard_read( ch->ch_bb );
            break;
    }
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( smx_fifo_t* fifo )
{
    smx_msg_t* msg;
    pthread_mutex_lock( &fifo->fifo_mutex );
    while( fifo->count == 0 )
        pthread_cond_wait( &fifo->fifo_cv, &fifo->fifo_mutex );
    msg = fifo->head->msg;
    fifo->head = fifo->head->prev;
    fifo->count--;
    dzlog_debug("read from fifo %p (new count: %d)", fifo, fifo->count );
    pthread_cond_signal( &fifo->fifo_cv );
    pthread_mutex_unlock( &fifo->fifo_mutex );
    return msg;
}

/*****************************************************************************/
void smx_channel_write( smx_channel_t* ch, smx_msg_t* msg )
{
    switch( ch->type ) {
        case SMX_FIFO:
            smx_fifo_write( ch->ch_fifo, msg );
            break;
        case SMX_BLACKBOARD:
            smx_blackboard_write( ch->ch_bb, msg );
            break;
    }
}

/*****************************************************************************/
void smx_fifo_write( smx_fifo_t* fifo, smx_msg_t* msg )
{
    pthread_mutex_lock( &fifo->fifo_mutex );
    while( fifo->count == fifo->length )
        pthread_cond_wait( &fifo->fifo_cv, &fifo->fifo_mutex );
    fifo->tail->msg = msg;
    fifo->tail = fifo->tail->prev;
    fifo->count++;
    dzlog_debug("write to fifo %p (new count: %d)", fifo, fifo->count );
    pthread_cond_signal( &fifo->fifo_cv );
    pthread_mutex_unlock( &fifo->fifo_mutex );
}

/*****************************************************************************/
smx_msg_t* smx_msg_create( void* ( *init )(), void* ( *copy )( void* ),
        void ( *destroy )( void* ) )
{
    smx_msg_t* msg = malloc( sizeof( struct smx_msg_s ) );
    msg->init = init;
    msg->copy = copy;
    msg->destroy = destroy;
    if( msg->init != NULL )
        msg->data = msg->init();
    return msg;
}

/*****************************************************************************/
void smx_msg_destroy( smx_msg_t* msg )
{
    if( msg->destroy != NULL )
        msg->destroy( msg->data );
    free( msg );
}

/*****************************************************************************/
void smx_program_init()
{
    int rc = dzlog_init("test_default.conf", "my_cat");

    if( rc ) {
        printf("init failed\n");
        pthread_exit( NULL );
    }

    dzlog_info("start thread main");
}

/*****************************************************************************/
void smx_program_cleanup()
{
    dzlog_info("end thread main");
    zlog_fini();
    pthread_exit( NULL );
}
