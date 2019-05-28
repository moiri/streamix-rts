/**
 * Log definitions for the runtime system library of Streamix
 *
 * @file    smxlog.h
 * @author  Simon Maurer
 */

#include <zlog.h>
#include "smxlog.h"

zlog_category_t* smx_zcat_ch;
zlog_category_t* smx_zcat_net;
zlog_category_t* smx_zcat_main;
zlog_category_t* smx_zcat_msg;
pthread_mutex_t mlog;

int smx_init_log( const char* conf )
{
    pthread_mutexattr_t mutexattr_prioinherit;

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &mlog, &mutexattr_prioinherit );

    int rc = zlog_init( conf );

    if( rc ) {
        return -1;
    }

    smx_zcat_main = zlog_get_category( "main" );
    smx_zcat_msg = zlog_get_category( "msg" );
    smx_zcat_ch = zlog_get_category( "ch" );
    smx_zcat_net = zlog_get_category( "net" );

    return 0;
}

pthread_mutex_t* smx_get_mlog() { return &mlog; }
zlog_category_t* smx_get_zcat_ch() { return smx_zcat_ch; }
zlog_category_t* smx_get_zcat_net() { return smx_zcat_net; }
zlog_category_t* smx_get_zcat_main() { return smx_zcat_main; }
zlog_category_t* smx_get_zcat_msg() { return smx_zcat_msg; }
