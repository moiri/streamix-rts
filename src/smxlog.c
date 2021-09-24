/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Log definitions for the runtime system library of Streamix
 */

#include <string.h>
#include <unistd.h>
#include "smxlog.h"

zlog_category_t* smx_zcat_ch;
zlog_category_t* smx_zcat_net;
zlog_category_t* smx_zcat_main;
zlog_category_t* smx_zcat_msg;
pthread_mutex_t mlog;

/*****************************************************************************/
pthread_mutex_t* smx_get_mlog() { return &mlog; }

/*****************************************************************************/
zlog_category_t* smx_get_zcat_ch() { return smx_zcat_ch; }

/*****************************************************************************/
zlog_category_t* smx_get_zcat_main() { return smx_zcat_main; }

/*****************************************************************************/
zlog_category_t* smx_get_zcat_msg() { return smx_zcat_msg; }

/*****************************************************************************/
zlog_category_t* smx_get_zcat_net() { return smx_zcat_net; }

/*****************************************************************************/
void smx_log_cleanup()
{
    zlog_fini();
}

/*****************************************************************************/
int smx_log_init( const char* log_conf )
{
    pthread_mutexattr_t mutexattr_prioinherit;

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &mlog, &mutexattr_prioinherit );

    int rc = zlog_init( log_conf );

    if( rc < 0 ) {
        fprintf( stderr,
                "error: failed to init zlog with configuration file: '%s'\n",
                log_conf );
        return -1;
    }

    smx_zcat_main = zlog_get_category( "main" );
    smx_zcat_msg = zlog_get_category( "msg" );
    smx_zcat_ch = zlog_get_category( "ch" );
    smx_zcat_net = zlog_get_category( "net" );

    return 0;
}

/*****************************************************************************/
int smx_log_mask( smx_log_buffer_t* log, zlog_category_t* cat,
        const char* format, ... )
{
    char new[1000];
    va_list argptr;
    int ret = 0;

    if( log == NULL )
        return 0;

    va_start( argptr, format );
    vsprintf( new, format, argptr );
    va_end( argptr );

    if( strcmp( new, log->last ) == 0 )
    {
        if( log->count == 2 ) {
            SMX_LOG_RAW( notice, cat, "accumulating identical messages" );
        }
        ret = -1;
        log->count++;
    }
    else if( log->count > 0 )
    {
        SMX_LOG_RAW( notice, log->cat, "accumulated %llu identical messages",
                log->count );
    }

    log->cat = cat;
    strcpy( log->last, new );

    return ret;
}
