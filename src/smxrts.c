/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <time.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "smxrts.h"
#include "pthread.h"

/*****************************************************************************/
smx_channel_t* smx_channel_create( int len, smx_channel_type_t type,
        const char* name )
{
    smx_channel_t* ch = malloc( sizeof( struct smx_channel_s ) );
    ch->type = type;
    ch->fifo = smx_fifo_create( len );
    ch->collector = NULL;
    ch->guard = NULL;
    ch->name = name;
    pthread_mutex_init( &ch->ch_mutex_r, NULL );
    pthread_cond_init( &ch->ch_cv_r, NULL );
    pthread_mutex_init( &ch->ch_mutex_w, NULL );
    pthread_cond_init( &ch->ch_cv_w, NULL );
    ch->state_w = SMX_CHANNEL_READY;
    ch->state_r = SMX_CHANNEL_PENDING;
    if( ( type == SMX_FIFO_D ) || ( type == SMX_D_FIFO_D ) )
        ch->state_r = SMX_CHANNEL_READY; // do not block on decouped output
    return ch;
}

/*****************************************************************************/
void smx_channel_destroy( smx_channel_t* ch )
{
    dzlog_debug("destroy channel '%s' (msg count: %d)", ch->name, ch->fifo->count);
    pthread_mutex_destroy( &ch->ch_mutex_r );
    pthread_cond_destroy( &ch->ch_cv_r );
    pthread_mutex_destroy( &ch->ch_mutex_w );
    pthread_cond_destroy( &ch->ch_cv_w );
    smx_guard_destroy( ch->guard );
    smx_fifo_destroy( ch->fifo );
    free( ch );
}

/*****************************************************************************/
smx_msg_t* smx_channel_read( smx_channel_t* ch )
{
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &ch->ch_mutex_r );
    while( ch->state_r == SMX_CHANNEL_PENDING )
        pthread_cond_wait( &ch->ch_cv_r, &ch->ch_mutex_r );
    pthread_mutex_unlock( &ch->ch_mutex_r );
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            msg = smx_fifo_read( ch, ch->fifo );
            break;
        case SMX_FIFO_D:
        case SMX_D_FIFO_D:
            msg = smx_fifo_d_read( ch, ch->fifo );
            break;
        default:
            dzlog_error("undefined channel type '%d'", ch->type );
    }
    // notify producer that space is available
    pthread_mutex_lock( &ch->ch_mutex_w );
    if(ch->state_w != SMX_CHANNEL_END)
        ch->state_w = SMX_CHANNEL_READY;
    pthread_cond_signal( &ch->ch_cv_w );
    pthread_mutex_unlock( &ch->ch_mutex_w );
    return msg;
}

/*****************************************************************************/
int smx_channel_ready_to_read( smx_channel_t* ch )
{
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_D_FIFO:
            return ch->fifo->count;
        case SMX_D_FIFO_D:
        case SMX_FIFO_D:
            return 1;
        default:
            dzlog_error("undefined channel type '%d'", ch->type );
    }
    return 0;
}

/*****************************************************************************/
int smx_channel_write( smx_channel_t* ch, smx_msg_t* msg )
{
    int res = 0;
    pthread_mutex_lock( &ch->ch_mutex_w );
    while( ch->state_w == SMX_CHANNEL_PENDING )
        pthread_cond_wait( &ch->ch_cv_w, &ch->ch_mutex_w );
    pthread_mutex_unlock( &ch->ch_mutex_w );
    switch( ch->type ) {
        case SMX_FIFO:
        case SMX_FIFO_D:
            smx_guard_write( ch->guard );
            smx_fifo_write( ch, ch->fifo, msg );
            break;
        case SMX_D_FIFO:
        case SMX_D_FIFO_D:
            // discard message if miat is not reached
            if( smx_d_guard_write( ch->guard, msg ) < 0 ) return res;
            res = smx_d_fifo_write( ch, ch->fifo, msg );
            break;
        default:
            dzlog_error("undefined channel type '%d'", ch->type );
    }
    if( ch->collector != NULL ) {
        pthread_mutex_lock( &ch->collector->col_mutex );
        ch->collector->count++;
        ch->collector->state = SMX_CHANNEL_READY;
        dzlog_debug("write to collector %s (new count: %d)", ch->name,
                ch->collector->count );
        pthread_cond_signal( &ch->collector->col_cv );
        pthread_mutex_unlock( &ch->collector->col_mutex );
    }
    // notify consumer that messages are available
    pthread_mutex_lock( &ch->ch_mutex_r );
    if(ch->state_r != SMX_CHANNEL_END)
        ch->state_r = SMX_CHANNEL_READY;
    pthread_cond_signal( &ch->ch_cv_r );
    pthread_mutex_unlock( &ch->ch_mutex_r );
    return res;
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
    fifo->backup = NULL;
    pthread_mutex_init( &fifo->fifo_mutex, NULL );
    fifo->count = 0;
    fifo->length = length;
    return fifo;
}

/*****************************************************************************/
void smx_fifo_destroy( smx_fifo_t* fifo )
{
    pthread_mutex_destroy( &fifo->fifo_mutex );
    for( int i=0; i < fifo->length; i++ ) {
        if( fifo->head->msg != NULL ) smx_msg_destroy( fifo->head->msg, true );
        fifo->tail = fifo->head;
        fifo->head = fifo->head->next;
        free( fifo->tail );
    }
    if( fifo->backup != NULL ) smx_msg_destroy( fifo->backup, true );
    free( fifo );
}

/*****************************************************************************/
smx_msg_t* smx_fifo_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if( fifo->count > 0 ) {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        fifo->count--;

        dzlog_debug("read from fifo %s (new count: %d)", ch->name,
                fifo->count );
        if( ( fifo->count == 0 ) && ( ch->state_r != SMX_CHANNEL_END ) ) {
            // last message was read, wait for producer to terminate or produce
            pthread_mutex_lock( &ch->ch_mutex_r );
            ch->state_r = SMX_CHANNEL_PENDING;
            pthread_mutex_unlock( &ch->ch_mutex_r );
            dzlog_info("fifo '%s' is emty", ch->name);
        }
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
    return msg;
}

/*****************************************************************************/
smx_msg_t* smx_fifo_d_read( smx_channel_t* ch, smx_fifo_t* fifo )
{
    smx_msg_t* msg = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    if( fifo->count > 0 ) {
        // messages are available
        msg = fifo->head->msg;
        fifo->head->msg = NULL;
        fifo->head = fifo->head->prev;
        if( fifo->count == 1 ) {
            // last message, backup for later duplication
            if( fifo->backup != NULL ) // delete old backup
                smx_msg_destroy( fifo->backup, 1);
            fifo->backup = smx_msg_copy( msg );
        }
        fifo->count--;
        dzlog_debug("read from fifo %s (new count: %d)", ch->name,
                fifo->count );
    }
    else {
        if( fifo->backup != NULL ) {
            msg = smx_msg_copy( fifo->backup );
            dzlog_info("fifo %s is empty, duplicate backup", ch->name );
        }
        else {
            dzlog_notice("nothing to read, fifo %s and its backup is empty",
                    ch->name );
        }
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
    return msg;
}

/*****************************************************************************/
void smx_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    pthread_mutex_lock( &fifo->fifo_mutex );
    if(fifo->count < fifo->length)
    {
        fifo->tail->msg = msg;
        fifo->tail = fifo->tail->prev;
        fifo->count++;

        dzlog_debug("write to fifo %s (new count: %d)", ch->name, fifo->count );
        if( ( fifo->count == fifo->length ) && ( ch->state_w != SMX_CHANNEL_END ) ) {
            // fifo is full, wait for consumer to terminate or consume
            pthread_mutex_lock( &ch->ch_mutex_w );
            ch->state_w = SMX_CHANNEL_PENDING;
            pthread_mutex_unlock( &ch->ch_mutex_w );
            dzlog_info("fifo '%s' is full", ch->name);
        }
    }
    else
    {
        dzlog_info( "write to channel '%s' aborted, destroying message",
                ch->name );
        smx_msg_destroy( msg, true );
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
}

/*****************************************************************************/
int smx_d_fifo_write( smx_channel_t* ch, smx_fifo_t* fifo, smx_msg_t* msg )
{
    bool overwrite = false;
    smx_msg_t* msg_tmp = NULL;
    pthread_mutex_lock( &fifo->fifo_mutex );
    msg_tmp = fifo->tail->msg;
    fifo->tail->msg = msg;
    if( fifo->count < fifo->length ) {
        fifo->tail = fifo->tail->prev;
        fifo->count++;
        dzlog_debug("write to fifo %s (new count: %d)", ch->name, fifo->count );
    }
    else {
        overwrite = true;
        dzlog_info("overwrite tail of fifo %s (new count: %d)",
                ch->name, fifo->count );
    }
    pthread_mutex_unlock( &fifo->fifo_mutex );
    if( overwrite ) smx_msg_destroy( msg_tmp, true );
    return overwrite;
}

/*****************************************************************************/
smx_guard_t* smx_guard_create( int iats, int iatns )
{
    struct itimerspec itval;
    smx_guard_t* guard = malloc( sizeof( struct smx_guard_s ) );
    guard->iat.tv_sec = iats;
    guard->iat.tv_nsec = iatns;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = 1; // arm timer to immediately fire (1ns delay)
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    guard->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( guard->fd == -1 )
        dzlog_error( "timerfd_create: %d", errno );
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
        dzlog_error( "timerfd_settime: %d", errno );
    return guard;
}

/*****************************************************************************/
void smx_guard_destroy( smx_guard_t* guard )
{
    if( guard == NULL ) return;
    close( guard->fd );
    free( guard );
}

/*****************************************************************************/
void smx_guard_write( smx_guard_t* guard )
{
    uint64_t expired;
    struct itimerspec itval;
    if( guard == NULL ) return;
    if( -1 == read( guard->fd, &expired, sizeof( uint64_t ) ) )
        dzlog_error( "timerfd read: %d", errno );
    dzlog_notice( "timerfd read missed: %lu", expired );
    itval.it_value = guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
        dzlog_error( "timerfd_settime: %d", errno );
}

/*****************************************************************************/
int smx_d_guard_write( smx_guard_t* guard, smx_msg_t* msg )
{
    struct itimerspec itval;
    if( guard == NULL ) return 0;
    if( -1 == timerfd_gettime( guard->fd, &itval ) )
        dzlog_error( "timerfd_gettime: %d", errno );
    if( ( itval.it_value.tv_sec != 0 ) || ( itval.it_value.tv_nsec != 0 ) ) {
        smx_msg_destroy( msg, true );
        dzlog_info( "rate_control: discard message" );
        return -1;
    }
    itval.it_value = guard->iat;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if( -1 == timerfd_settime( guard->fd, 0, &itval, NULL ) )
        dzlog_error( "timerfd_settime: %d", errno );
    return 0;
}

/*****************************************************************************/
smx_msg_t* smx_msg_copy( smx_msg_t* msg )
{
    smx_msg_t* msg_copy = malloc( sizeof( struct smx_msg_s ) );
    msg_copy->data = msg->copy( msg->data, msg->size );
    msg_copy->size = msg->size;
    msg_copy->copy = msg->copy;
    msg_copy->destroy = msg->destroy;
    return msg_copy;
}

/*****************************************************************************/
smx_msg_t* smx_msg_create( void* data, size_t size, void* copy( void*, size_t ),
        void destroy( void* ), void* unpack( void* ) )
{
    smx_msg_t* msg = malloc( sizeof( struct smx_msg_s ) );
    msg->data = data;
    msg->size = size;
    if( copy == NULL ) msg->copy = smx_msg_data_copy;
    else msg->copy = copy;
    if( destroy == NULL ) msg->destroy = smx_msg_data_destroy;
    else msg->destroy = destroy;
    if( unpack == NULL ) msg->unpack = smx_msg_data_unpack;
    else msg->unpack = unpack;
    return msg;
}

/*****************************************************************************/
void* smx_msg_data_copy( void* data, size_t size )
{
    void* data_copy = malloc( size );
    memcpy( data_copy, data, size );
    return data_copy;
}

/*****************************************************************************/
void smx_msg_data_destroy( void* data )
{
    free( data );
}

/*****************************************************************************/
void* smx_msg_data_unpack( void* data )
{
    return data;
}

/*****************************************************************************/
void smx_msg_destroy( smx_msg_t* msg, int deep )
{
    if( msg == NULL ) return;
    if( deep ) msg->destroy( msg->data );
    free( msg );
}

/*****************************************************************************/
void* smx_msg_unpack( smx_msg_t* msg )
{
    return msg->unpack( msg->data );
}

/*****************************************************************************/
void smx_net_log_start( const char* name )
{
    dzlog_notice( "start net %s", name );
}

/*****************************************************************************/
void smx_net_log_terminate( const char* name )
{
    dzlog_notice( "terminate net %s", name );
}

/*****************************************************************************/
void smx_net_rn_destroy( box_smx_rn_t* cp )
{
    pthread_mutex_destroy( &cp->in.collector->col_mutex );
    pthread_cond_destroy( &cp->in.collector->col_cv );
    free( cp->in.collector );
}

/*****************************************************************************/
void smx_net_rn_init( box_smx_rn_t* cp )
{
    cp->in.collector = malloc( sizeof( struct smx_collector_s ) );
    pthread_mutex_init( &cp->in.collector->col_mutex, NULL );
    pthread_cond_init( &cp->in.collector->col_cv, NULL );
    cp->in.collector->count = 0;
    cp->in.collector->state = SMX_CHANNEL_PENDING;
}

/*****************************************************************************/
pthread_t smx_net_run( void* box_impl( void* arg ), void* arg )
{
    pthread_t thread;
    pthread_create( &thread, NULL, box_impl, arg );
    return thread;
}

/*****************************************************************************/
int smx_net_update_state( smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out, int state, const char* name )
{
    int i;
    int done_cnt_in = 0;
    int done_cnt_out = 0;
    int trigger_cnt = 0;
    int push_cnt = 0;
    // if state is forced by box implementation return forced state
    if( state != SMX_NET_RETURN ) return state;

    // check if a triggering input is still producing
    for( i=0; i<len_in; i++ ) {
        if( ( chs_in[i]->type == SMX_FIFO ) || ( chs_in[i]->type == SMX_D_FIFO ) ) {
            trigger_cnt++;
            if( ( chs_in[i]->state_r == SMX_CHANNEL_END )
                    && ( chs_in[i]->fifo->count == 0 ) )
                done_cnt_in++;
        }
    }
    // check if consumer is available
    for( i=0; i<len_out; i++ ) {
        push_cnt++;
        if( chs_out[i]->state_w == SMX_CHANNEL_END )
            done_cnt_out++;
    }
    // if all the triggering inputs are done, terminate the thread
    if( (len_in > 0) && (done_cnt_in >= trigger_cnt) )
    {
        dzlog_info( "all triggering producers of '%s' have terminated", name );
        return SMX_NET_END;
    }

    if( (len_out) > 0 && (done_cnt_out >= push_cnt) )
    {
        dzlog_info( "all consumers of '%s' have terminated", name );
        return SMX_NET_END;
    }
    return SMX_NET_CONTINUE;
}

/*****************************************************************************/
void smx_net_terminate( smx_channel_t** chs_in, int len_in,
        smx_channel_t** chs_out, int len_out )
{
    int i;
    for( i=0; i < len_in; i++ ) {
        pthread_mutex_lock( &chs_in[i]->ch_mutex_w );
        chs_in[i]->state_w = SMX_CHANNEL_END;
        pthread_cond_signal( &chs_in[i]->ch_cv_w );
        dzlog_notice( "mark channel '%s' as stale", chs_in[i]->name );
        pthread_mutex_unlock( &chs_in[i]->ch_mutex_w );
    }
    for( i=0; i < len_out; i++ ) {
        pthread_mutex_lock( &chs_out[i]->ch_mutex_r );
        chs_out[i]->state_r = SMX_CHANNEL_END;
        dzlog_notice( "mark channel '%s' as stale", chs_out[i]->name );
        pthread_cond_signal( &chs_out[i]->ch_cv_r );
        pthread_mutex_unlock( &chs_out[i]->ch_mutex_r );
        if( chs_out[i]->collector != NULL ) {
            pthread_mutex_lock( &chs_out[i]->collector->col_mutex );
            chs_out[i]->collector->state = SMX_CHANNEL_END;
            pthread_cond_signal( &chs_out[i]->collector->col_cv );
            pthread_mutex_unlock( &chs_out[i]->collector->col_mutex );
        }
    }
}

/*****************************************************************************/
void smx_program_cleanup()
{
    xmlCleanupParser();
    dzlog_notice("end thread main");
    zlog_fini();
    exit( 0 );
}

/*****************************************************************************/
void smx_program_init()
{
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;
    xmlChar* cat = NULL;

    /* required for thread safety */
    xmlInitParser();

    /*parse the file and get the DOM */
    doc = xmlParseFile(XML_PATH);

    if (doc == NULL)
    {
        printf("error: could not parse the app config file '%s'\n", XML_PATH);
        exit( 0 );
    }

    cur = xmlDocGetRootElement(doc);
    if(cur == NULL || xmlStrcmp(cur->name, (const xmlChar*)XML_APP))
    {
        printf("error: app config root node name is '%s' instead of '%s'\n",
                cur->name, XML_APP);
        exit( 0 );
    }

    cur = cur->xmlChildrenNode;
    while(cur != NULL)
    {
        if(!xmlStrcmp(cur->name, (const xmlChar*)XML_LOG))
        {
            conf = xmlGetProp(cur, (const xmlChar*)XML_LOG_CONF);
            cat = xmlGetProp(cur, (const xmlChar*)XML_LOG_CAT);
        }
        cur = cur->next;
    }

    if(conf == NULL)
    {
        printf("error: no log configuration found in app config\n");
        exit( 0 );
    }

    if(cat == NULL)
    {
        printf("error: no log cathegory found in app config\n");
        exit( 0 );
    }

    int rc = dzlog_init((const char*)conf, (const char*)cat);

    if( rc ) {
        printf("error: zlog init failed with conf: '%s' and cat: '%s'\n",
                conf, cat);
        exit( 0 );
    }

    xmlFree(conf);
    xmlFree(cat);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    dzlog_notice("start thread main");
}

/*****************************************************************************/
int smx_rn( void* handler, void* state )
{
    (void)(state);
    int i;
    int count_in = ( ( box_smx_rn_t* )handler )->in.count;
    int count_out = ( ( box_smx_rn_t* )handler )->out.count;
    smx_channel_t** chs_in = ( ( box_smx_rn_t* )handler )->in.ports;
    smx_channel_t** chs_out = ( ( box_smx_rn_t* )handler )->out.ports;
    smx_channel_t* ch = NULL;
    smx_msg_t* msg;
    smx_msg_t* msg_copy;
    smx_collector_t* collector = ( ( box_smx_rn_t* )handler )->in.collector;

    pthread_mutex_lock( &collector->col_mutex );
    while( collector->state == SMX_CHANNEL_PENDING )
        pthread_cond_wait( &collector->col_cv, &collector->col_mutex );
    if( collector->count > 0 ) {
        // there are messages available
        collector->count--;

        for( i=0; i<count_in; i++ ) {
            if( smx_channel_ready_to_read( chs_in[i] ) ) {
                ch = chs_in[i];
                break;
            }
        }
        if( ch == NULL )
            dzlog_error("something went wrong: no msg ready in collector %s (count: %d)",
                    ch->name, collector->count );
        dzlog_debug("read from collector %s (new count: %d)", ch->name,
                collector->count );
        pthread_mutex_unlock( &collector->col_mutex );
    }
    else {
        // no messages available
        collector->state = SMX_CHANNEL_PENDING;
        pthread_mutex_unlock( &collector->col_mutex );
        return smx_net_update_state( chs_in, count_in, chs_out, count_out,
                SMX_NET_RETURN, "smx_rn" );
    }
    msg = smx_channel_read( ch );
    for( i=0; i<count_out; i++ ) {
        msg_copy = smx_msg_copy( msg );
        smx_channel_write( chs_out[i], msg_copy );
    }
    smx_msg_destroy( msg, true );
    return SMX_NET_CONTINUE;
}

/*****************************************************************************/
int smx_rn_init( void* handler, void** state )
{
    (void)(handler);
    (void)(state);
    return 0;
}

/*****************************************************************************/
void smx_rn_cleanup( void* state )
{
    (void)(state);
}

/*****************************************************************************/
void smx_tf_connect( smx_timer_t* timer, smx_channel_t* ch_in,
        smx_channel_t* ch_out )
{
    box_smx_tf_t* tf = malloc( sizeof( struct box_smx_tf_s ) );
    tf->in = ch_in;
    tf->out = ch_out;
    tf->next = timer->ports;
    timer->ports = tf;
    timer->count++;
}

/*****************************************************************************/
smx_timer_t* smx_tf_create( int sec, int nsec )
{
    smx_timer_t* timer = malloc( sizeof( struct smx_timer_s ) );
    timer->itval.it_value.tv_sec = sec;
    timer->itval.it_value.tv_nsec = nsec;
    timer->itval.it_interval.tv_sec = sec;
    timer->itval.it_interval.tv_nsec = nsec;
    timer->fd = timerfd_create( CLOCK_MONOTONIC, 0 );
    if( timer->fd == -1 )
        dzlog_error( "timerfd_create: %d", errno );
    timer->count = 0;
    timer->ports = NULL;
    return timer;
}

/*****************************************************************************/
void smx_tf_destroy( smx_timer_t* tt )
{
    box_smx_tf_t* tf = tt->ports;
    box_smx_tf_t* tf_tmp;
    while( tf != NULL ) {
        tf_tmp = tf;
        tf = tf->next;
        free( tf_tmp );
    }
    close( tt->fd );
    free( tt );
}

/*****************************************************************************/
void smx_tf_enable( smx_timer_t* timer )
{
    if( timer == NULL ) return;
    if( -1 == timerfd_settime( timer->fd, 0, &timer->itval, NULL ) )
        dzlog_error( "timerfd_settime: %d", errno );
}

/*****************************************************************************/
int smx_tf_read_inputs( smx_msg_t** msg, smx_timer_t* tt,
        smx_channel_t** ch_in )
{
    int i, end_cnt = 0;
    dzlog_debug( "tt_read" );
    for( i = 0; i < tt->count; i++ ) {
        msg[i] = smx_channel_read( ch_in[i] );
        if( ch_in[i]->state_r == SMX_CHANNEL_END ) end_cnt++;
    }
    return end_cnt;
}

/*****************************************************************************/
void smx_tf_wait( smx_timer_t* timer )
{
    uint64_t expired;
    struct pollfd pfd;
    int poll_res;
    if( timer == NULL ) return;
    pfd.fd = timer->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll_res = poll( &pfd, 1, 0 );
    if( -1 == poll_res )
        dzlog_error( "timerfd poll: %d", errno );
    if( poll_res > 0 ) {
        dzlog_error( "deadline missed" );
    }
    else if( -1 == read( timer->fd, &expired, sizeof( uint64_t ) ) )
        dzlog_error( "timerfd read: %d", errno );
}

/*****************************************************************************/
void smx_tf_write_outputs( smx_msg_t** msg, smx_timer_t* tt,
        smx_channel_t** ch_in, smx_channel_t** ch_out )
{
    int i, res;
    dzlog_debug( "tt_write" );
    for( i = 0; i < tt->count; i++ )
        if( msg[i] != NULL ) {
            res = smx_channel_write( ch_out[i], msg[i] );
            if( res )
                dzlog_error( "consumer on channel '%s' missed its deadline",
                        ch_in[i]->name );
        }
        else
            dzlog_error( "producer on channel '%s' missed its deadline",
                    ch_in[i]->name );

}

/*****************************************************************************/
void* start_routine_net( const char* name, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void* ), void* h,
        smx_channel_t** chs_in, int cnt_in, smx_channel_t** chs_out,
        int cnt_out )
{
    int state = SMX_NET_CONTINUE;
    dzlog_notice( "init net %s", name );
    void* net_state = NULL;
    if(init( h, &net_state ) == 0)
    {
        smx_net_log_start( name );
        while( state == SMX_NET_CONTINUE )
        {
            state = impl( h, net_state );
            state = smx_net_update_state( chs_in, cnt_in, chs_out, cnt_out,
                    state, name );
        }
    }
    else
        dzlog_error( "initialisation of net %s failed", name );
    smx_net_terminate( chs_in, cnt_in, chs_out, cnt_out );
    dzlog_notice( "cleanup net %s", name );
    cleanup( net_state );
    smx_net_log_terminate( name );
    return NULL;
}

/*****************************************************************************/
void* start_routine_tf( void* h )
{
    smx_timer_t* tt = h;
    box_smx_tf_t* tf = tt->ports;
    smx_channel_t* ch_in[tt->count];
    smx_channel_t* ch_out[tt->count];
    smx_msg_t* msg[tt->count];
    int state = SMX_NET_CONTINUE;
    int tf_cnt = 0, end_cnt;
    while( tf != NULL ) {
        ch_in[tf_cnt] = tf->in;
        ch_out[tf_cnt] = tf->out;
        tf_cnt++;
        tf = tf->next;
    }
    smx_net_log_start( "tf" );
    smx_tf_enable( tt );
    while( state == SMX_NET_CONTINUE )
    {
        end_cnt = smx_tf_read_inputs( msg, tt, ch_in );
        smx_tf_write_outputs( msg, tt, ch_in, ch_out );
        dzlog_debug( "tt_start_routine: tt_wait" );
        smx_tf_wait( tt );
        if( end_cnt == tt->count ) state = SMX_NET_END;
    }
    smx_net_terminate( ch_in, tt->count, ch_out, tt->count );
    smx_net_log_terminate( "tf" );
    return NULL;
}
