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

/**
 *
 */
#define SMX_LOG_FREE( level, cat, format, ... )\
    zlog_ ## level( cat, format, ##__VA_ARGS__ )

/**
 * @def SMX_LOG_MAIN( cat, level, format, ... )
 * A macro to log to a globally defined category `cat` on log level `level`.
 */
#define SMX_LOG_MAIN( cat, level, format, ... )\
    SMX_LOG_INTERN( level, smx_get_zcat_ ## cat(), format,  ##__VA_ARGS__ )

/**
 * Define mutex protection and main categories for zlog. Further, initialise
 * zlog with the default configuration file.
 */
int smx_log_init();

/**
 * Cleanup zlog
 */
void smx_log_cleanup();

/**
 * Get the global zlog mutex handler
 *
 * @return a pointer to the mutex variable
 */
pthread_mutex_t* smx_get_mlog();

/**
 * Get the global zlog channel category
 *
 * @return a pointer to the zlog category
 */
zlog_category_t* smx_get_zcat_ch();

/**
 * Get the global zlog main category
 *
 * @return a pointer to the zlog category
 */
zlog_category_t* smx_get_zcat_main();

/**
 * Get the global zlog mssage category
 *
 * @return a pointer to the zlog category
 */
zlog_category_t* smx_get_zcat_msg();

/**
 * Get the global zlog net category
 *
 * @return a pointer to the zlog category
 */
zlog_category_t* smx_get_zcat_net();

#endif
