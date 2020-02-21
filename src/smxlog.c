/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Log definitions for the runtime system library of Streamix
 */

#include <unistd.h>
#include "smxlog.h"

#define ZLOG_DEFAULT    "/etc/smx/default.zlog"

zlog_category_t* smx_zcat_ch;
zlog_category_t* smx_zcat_net;
zlog_category_t* smx_zcat_main;
zlog_category_t* smx_zcat_msg;
pthread_mutex_t mlog;

/*****************************************************************************/
int smx_log_init()
{
    if( access( ZLOG_DEFAULT, F_OK ) == -1 )
    {
        fprintf( stderr,
                "error: cannot open default zlog configuration file: '%s'\n",
                ZLOG_DEFAULT );
        return -1;
    }
    pthread_mutexattr_t mutexattr_prioinherit;

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &mlog, &mutexattr_prioinherit );

    int rc = zlog_init( ZLOG_DEFAULT );

    if( rc ) {
        return -1;
    }

    smx_zcat_main = zlog_get_category( "main" );
    smx_zcat_msg = zlog_get_category( "msg" );
    smx_zcat_ch = zlog_get_category( "ch" );
    smx_zcat_net = zlog_get_category( "net" );

    return 0;
}

/*****************************************************************************/
void smx_log_cleanup()
{
    zlog_fini();
}

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
