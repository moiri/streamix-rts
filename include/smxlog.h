/**
 * @file    smxlog.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Log definitions for the runtime system library of Streamix
 */

#include <zlog.h>
#include <pthread.h>

#ifndef SMXLOG_H
#define SMXLOG_H

/** The custom zlog level "event" */
#define ZLOG_LEVEL_EVENT 50

/**
 * The macro to use the custom zlog level "event".
 */
#define zlog_event( cat, format, ... )\
     zlog( cat, __FILE__, sizeof( __FILE__ )-1, \
    __func__, sizeof( __func__ )-1, __LINE__, \
    ZLOG_LEVEL_EVENT, format, ## __VA_ARGS__ )

/**
 * The logger macro for all types of logs.
 */
#ifndef SMX_LOG_UNSAFE
#define SMX_LOG_INTERN SMX_LOG_LOCK
#else
#define SMX_LOG_INTERN SMX_LOG_FREE
#endif

/**
 * @def SMX_LOG_CH()
 *
 * The logger macro for channel-specific logs.
 */
#define SMX_LOG_CH( ch, level, format, ... )\
    SMX_LOG_INTERN( level, ch->cat, format,  ##__VA_ARGS__ )

/**
 * @def SMX_LOG_LOCK()
 *
 * The logger macro performing a mutex lock/unlock before logging to prevent
 * any issue of priority inversion.
 */
#define SMX_LOG_LOCK( level, cat, format, ... ) do {\
    pthread_mutex_lock( smx_get_mlog() );\
    zlog_ ## level( cat, format, ##__VA_ARGS__ );\
    pthread_mutex_unlock( smx_get_mlog() ); } while( 0 )

/**
 * @def SMX_LOG_FREE()
 *
 * The logger macro without locking.
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
 * @def SMX_LOG_NET()
 *
 * Refer top SMX_LOG() for more information.
 */
#define SMX_LOG_NET( net, level, format, ... )\
    SMX_LOG_INTERN( level, SMX_SIG_CAT( net ), format, ##__VA_ARGS__ )

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

#endif /* SMXLOG_H */
