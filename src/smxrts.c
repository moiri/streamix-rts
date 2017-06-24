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
int smx_cp( void* handler )
{
    int i, count = ( ( box_smx_cp_t* )handler )->in.count;
    int end = 1;
    smx_channel_t** chs;
    smx_channel_t* ch = NULL;
    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    smx_collector_t* collector = ( ( box_smx_cp_t* )handler )->in.collector;

    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    if( collector->count > 0 ) {
        collector->count--;
        dzlog_debug("read from collector %p (new count: %d)", collector,
                collector->count );

        chs = ( ( box_smx_cp_t* )handler )->in.ports;
        for( i=0; i<count; i++ ) {
            if( chs[i]->state != SMX_CHANNEL_END ) end = 0;
            if( smx_channel_ready_to_read( chs[i] ) ) {
                ch = chs[i];
                break;
            }

        }
    }
    pthread_mutex_unlock( &collector->col_mutex );
    if( ch == NULL ) {
        collector->state = SMX_CHANNEL_PENDING;
        if( end )
            return SMX_BOX_TERMINATE;
        else
            return SMX_BOX_CONTINUE;
    }
    msg = smx_channel_read( ch );
    count = ( ( box_smx_cp_t* )handler )->out.count;
    chs = ( ( box_smx_cp_t* )handler )->out.ports;
    for( i=0; i<count; i++ ) {
        msg_copy = smx_msg_create( msg->init, msg->copy, msg->destroy );
        smx_channel_write( chs[i], msg_copy );
    }
    smx_msg_destroy( msg, ( msg->copy != NULL ) );
    return SMX_BOX_CONTINUE;
}

/*****************************************************************************/
void* box_smx_cp( void* handler )
{
    while( smx_cp( handler ) );
    smx_channels_terminate( ( ( box_smx_cp_t* )handler )->out.ports,
            ( ( box_smx_cp_t* )handler )->out.count );
    return NULL;
}

/*****************************************************************************/
void smx_box_cp_init( box_smx_cp_t* cp )
{
    cp->in.collector = malloc( sizeof( struct smx_collector_s ) );
    pthread_mutex_init( &cp->in.collector->col_mutex, NULL );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_PENDING;
}

/*****************************************************************************/
void smx_box_cp_destroy( box_smx_cp_t* cp )
{
    pthread_mutex_destroy( &cp->in.collector->col_mutex );
    pthread_cond_destroy( &cp->in.collector->col_cv );
    free( cp->in.collector );
}

/*****************************************************************************/
pthread_t smx_box_run( void* box_impl( void* ), void* arg )
{
    pthread_t thread;
    pthread_create( &thread, NULL, box_impl, arg );
    return thread;
}

/*****************************************************************************/
void smx_channels_terminate( smx_channel_t** chs, int len )
{
    int i;
    for( i=0; i<len; i++ )
        chs[i]->state = SMX_CHANNEL_END;
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
    ch->collector = NULL;
    pthread_mutex_init( &ch->ch_mutex, NULL );
    pthread_cond_init( &ch->ch_cv, NULL );
    ch->state = SMX_CHANNEL_PENDING;
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
    pthread_mutex_destroy( &ch->ch_mutex );
    pthread_cond_destroy( &ch->ch_cv );
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
    pthread_mutex_destroy( &fifo->fifo_mutex );
    pthread_cond_destroy( &fifo->fifo_cv );
    for( int i=0; i < fifo->length; i++ ) {
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    free( fifo );
}

/*****************************************************************************/
int smx_channel_ready_to_read( smx_channel_t* ch )
{
    switch( ch->type ) {
        case SMX_FIFO:
            return ch->ch_fifo->count;
        case SMX_BLACKBOARD:
            return 1;
    }
    return 0;
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( smx_channel_t* ch )
{
    smx_msg_t* msg;
    pthread_mutex_lock( &ch->ch_mutex );
    while( ch->state == SMX_CHANNEL_PENDING )
        pthread_cond_wait( &ch->ch_cv, &ch->ch_mutex );
    switch( ch->type ) {
        case SMX_FIFO:
            msg = smx_fifo_read( ch, ch->ch_fifo );
            break;
        case SMX_BLACKBOARD:
            msg = smx_blackboard_read( ch->ch_bb );
            break;
    }
    pthread_mutex_unlock( &ch->ch_mutex );
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if( fifo->count > 0 ) {
        // messages are available
        msg = fifo->head->msg;
        fifo->head = fifo->head->prev;
        fifo->count--;
        dzlog_debug("read from fifo %p (new count: %d)", fifo, fifo->count );
        if( ( fifo->count == 0 ) && ( ch->state != SMX_CHANNEL_END ) ) {
            // last message was read, wait for producer to terminate or produce
            ch->state = SMX_CHANNEL_PENDING;
        }
        pthread_cond_signal( &fifo->fifo_cv );
    }
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
    if( ch->collector != NULL ) {
        pthread_mutex_lock( &ch->collector->col_mutex );
        ch->collector->count++;
        ch->collector->state = SMX_CHANNEL_READY;
        dzlog_debug("write to collector %p (new count: %d)", ch->collector,
                ch->collector->count );
        pthread_cond_signal( &ch->collector->col_cv );
        pthread_mutex_unlock( &ch->collector->col_mutex );
    }
    pthread_mutex_lock( &ch->ch_mutex );
    ch->state = SMX_CHANNEL_READY;
    pthread_cond_signal( &ch->ch_cv );
    pthread_mutex_unlock( &ch->ch_mutex );
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
void smx_msg_destroy( smx_msg_t* msg, int deep )
{
    if( deep && ( msg->destroy != NULL ) )
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
