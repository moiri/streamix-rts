/**
 * Log definitions for the runtime system library of Streamix
 *
 * @file    smxlog.h
 * @author  Simon Maurer
 */

#include "smxch.h"

#ifndef SMXLOG_H
#define SMXLOG_H

typedef struct net_smx_log_s net_smx_log_t;             /**< ::net_smx_log_s */

/**
 * @brief The signature of a logger
 */
struct net_smx_log_s
{
    struct {
        smx_channel_t** ports;      /**< an array of channel pointers */
        int count;                  /**< the number of input ports */
        smx_collector_t* collector; /**< ::smx_collector_s */
    } in;                           /**< input channels */
};

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
#define SMX_LOG_MAIN( cat, level, format, ... )\
    SMX_LOG_LOCK( level, smx_get_zcat_ ## cat(), format,  ##__VA_ARGS__ );\

/**
 *
 */
int smx_init_log( const char* conf );
pthread_mutex_t* smx_get_mlog();
zlog_category_t* smx_get_zcat_ch();
zlog_category_t* smx_get_zcat_net();
zlog_category_t* smx_get_zcat_main();
zlog_category_t* smx_get_zcat_msg();

#endif
