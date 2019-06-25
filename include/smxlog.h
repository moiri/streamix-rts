/**
 * Log definitions for the runtime system library of Streamix
 *
 * @file    smxlog.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <zlog.h>
#include <pthread.h>

#ifndef SMXLOG_H
#define SMXLOG_H

#ifndef SMX_LOG_UNSAFE
#define SMX_LOG_INTERN SMX_LOG_LOCK
#else
#define SMX_LOG_INTERN SMX_LOG_FREE
#endif

/**
 *
 */
#define SMX_LOG_LOCK( level, cat, format, ... ) do {\
    pthread_mutex_lock( smx_get_mlog() );\
    zlog_ ## level( cat, format, ##__VA_ARGS__ );\
    pthread_mutex_unlock( smx_get_mlog() ); } while( 0 )

#define SMX_LOG_FREE( level, cat, format, ... )\
    zlog_ ## level( cat, format, ##__VA_ARGS__ )

/**
 *
 */
#define SMX_LOG_MAIN( cat, level, format, ... )\
    SMX_LOG_INTERN( level, smx_get_zcat_ ## cat(), format,  ##__VA_ARGS__ )

/**
 *
 */
int smx_log_init( const char* conf );

/**
 *
 */
void smx_log_cleanup();

/**
 *
 */
pthread_mutex_t* smx_get_mlog();

/**
 *
 */
zlog_category_t* smx_get_zcat_ch();

/**
 *
 */
zlog_category_t* smx_get_zcat_main();

/**
 *
 */
zlog_category_t* smx_get_zcat_msg();

/**
 *
 */
zlog_category_t* smx_get_zcat_net();

#endif
