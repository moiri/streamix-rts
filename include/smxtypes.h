/**
 * @file    smxtypes.h
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Type definitions for the runtime system library of Streamix
 */

#include <stdbool.h>
#include <pthread.h>
#include <zlog.h>

#ifndef SMXTYPES_H
#define SMXTYPES_H

/**
 * The streamix channel error type. Refer to the error enumeration definition
 * for more details #smx_channel_err_e.
 */
typedef enum smx_channel_err_e smx_channel_err_t;
typedef enum smx_channel_state_e smx_channel_state_t; /**< #smx_channel_state_e */
typedef enum smx_channel_type_e smx_channel_type_t;   /**< #smx_channel_type_e */
/** #smx_config_error_e */
typedef enum smx_config_error_e smx_config_error_t;
/** #smx_profiler_action_e */
typedef enum smx_profiler_action_e smx_profiler_action_t;

typedef struct smx_channel_s smx_channel_t;           /**< ::smx_channel_s */
typedef struct smx_channel_end_s smx_channel_end_t;   /**< ::smx_channel_end_s */
typedef struct smx_collector_s smx_collector_t;       /**< ::smx_collector_s */
typedef struct smx_fifo_s smx_fifo_t;                 /**< ::smx_fifo_s */
typedef struct smx_fifo_item_s smx_fifo_item_t;       /**< ::smx_fifo_item_s */
typedef struct smx_guard_s smx_guard_t;               /**< ::smx_guard_s */
/**
 * The streamix message type.
 * Refer to the structure definition for more information ::smx_msg_s.
 */
typedef struct smx_msg_s smx_msg_t;
typedef struct smx_net_s smx_net_t;                   /**< ::smx_net_s */
typedef struct smx_net_sig_s smx_net_sig_t;           /**< ::smx_net_sig_s */

/**
 * The error state of a channel end
 */
enum smx_channel_err_e
{
    SMX_CHANNEL_ERR_NONE = 0,      /**< no error */
    SMX_CHANNEL_ERR_NO_DEFAULT,    /**< no default message for decoupled read */
    SMX_CHANNEL_ERR_NO_TARGET,     /**< connecting net has terminated */
    SMX_CHANNEL_ERR_DL_MISS,       /**< connecting net missed its deadline */
    SMX_CHANNEL_ERR_NO_DATA,       /**< unexpectedly, the channel has no data */
    SMX_CHANNEL_ERR_NO_SPACE,      /**< unexpectedly, the channel has no space */
    SMX_CHANNEL_ERR_FILTER,        /**< the msg type does not match the filter */
    SMX_CHANNEL_ERR_UNINITIALISED, /**< the channel was never initialised */
    SMX_CHANNEL_ERR_TIMEOUT,       /**< the channel operation timed out */
    SMX_CHANNEL_ERR_CV,            /**< the conditional variable lock failed */
};

/**
 * @brief Channel state
 *
 * This allows to indicate wheter a producer connected to the channel has
 * terminated and wheter data is available to read. The second point is
 * important in combination with copy synchronizers.
 */
enum smx_channel_state_e
{
    SMX_CHANNEL_UNINITIALISED, /**< decoupled channel was never written to */
    SMX_CHANNEL_PENDING,       /**< channel is waiting for a signal */
    SMX_CHANNEL_READY,         /**< channel is ready to read from */
    SMX_CHANNEL_END            /**< net connected to channel end has terminated */
};

/**
 * @brief Streamix channel (buffer) types
 */
enum smx_channel_type_e
{
    SMX_FIFO,           /**< a simple FIFO */
    SMX_FIFO_D,         /**< a FIFO with decoupled output */
    SMX_D_FIFO,         /**< a FIFO with decoupled input */
    SMX_D_FIFO_D        /**< a FIFO with decoupled input and output */
};

/**
 * The list of config read errors.
 */
enum smx_config_error_e
{
    SMX_CONFIG_ERROR_NO_ERROR,  /**< No error */
    SMX_CONFIG_ERROR_BAD_TYPE,  /**< The item exists but the type does not match */
    SMX_CONFIG_ERROR_NO_VALUE   /**< The item does not exist */
};

/**
 * The different actions a profiler can log.
 */
enum smx_profiler_action_e
{
    SMX_PROFILER_ACTION_START,          /**< start a net. */
    SMX_PROFILER_ACTION_CREATE,         /**< create a msg, channel, or net. */
    SMX_PROFILER_ACTION_DESTROY,        /**< destroy a msg, channel, or net. */
    SMX_PROFILER_ACTION_COPY,           /**< copy a message. */
    SMX_PROFILER_ACTION_READ,           /**< read from a channel. */
    SMX_PROFILER_ACTION_READ_COLLECTOR, /**< read from a collector. */
    SMX_PROFILER_ACTION_WRITE,          /**< write to a channel. */
    SMX_PROFILER_ACTION_WRITE_COLLECTOR,/**< write to a collector. */
    SMX_PROFILER_ACTION_OVERWRITE,      /**< overwrite a message in a channel. */
    SMX_PROFILER_ACTION_DISMISS,        /**< dismiss a message in a channel. */
    SMX_PROFILER_ACTION_DUPLICATE,      /**< duplicate a message in a channel. */
    SMX_PROFILER_ACTION_DL_MISS_SRC,    /**< rt producer missed a deadline. */
    SMX_PROFILER_ACTION_DL_MISS_SRC_CP, /**< rt producer missed a deadline, msg duplicated. */
    SMX_PROFILER_ACTION_TT_MISS_SRC,    /**< non-rt producer missed a tt interval. */
    SMX_PROFILER_ACTION_TT_MISS_SRC_CP, /**< non-rt producer missed a tt interval, msg duplicated. */
    SMX_PROFILER_ACTION_DL_MISS_SINK,   /**< rt consumer missed a deadline. */
    SMX_PROFILER_ACTION_TT_MISS_SINK    /**< non-rt consumer missed a tt interval. */
};

/**
 * Constants to indicate wheter a thread should terminate or continue.
 * Use one of these values to return from the main box implemenation funtion.
 */
enum smx_thread_state_e
{
    SMX_NET_RETURN = 0,     /**< decide automatically wheather to end or go on */
    SMX_NET_CONTINUE,       /**< continue to call the box implementation fct */
    SMX_NET_END             /**< end thread */
};

/**
 * @brief A generic Streamix channel
 */
struct smx_channel_s
{
    int                 id;         /**< the id of the channel */
    smx_channel_type_t  type;       /**< type of the channel */
    const char*         name;       /**< name of the channel */
    smx_fifo_t*         fifo;       /**< ::smx_fifo_s */
    smx_guard_t*        guard;      /**< ::smx_guard_s */
    smx_collector_t*    collector;  /**< ::smx_collector_s, collect signals */
    smx_channel_end_t*  sink;       /**< ::smx_channel_end_s */
    smx_channel_end_t*  source;     /**< ::smx_channel_end_s */
    zlog_category_t*    cat;        /**< zlog category of a channel end */
    pthread_mutex_t     ch_mutex;   /**< mutual exclusion */
};

/**
 * The end of a channel
 */
struct smx_channel_end_s
{
    smx_channel_state_t state;    /**< state of the channel end */
    smx_channel_err_t   err;      /**< error on the channel end */
    pthread_cond_t      ch_cv;    /**< conditional variable to trigger producer */
    unsigned long       count;    /**< access counter */
    smx_net_t*          net;      /**< pointer to the connecting net */
    struct {
        char**  items;
        int     count;
    } filter;     /**< All message types allowed on this channel */
    /** A pointer to the filter function. */
    bool ( *content_filter )( smx_msg_t* msg );
    struct timespec     timeout;    /**< channel-blocking timeout */
};

/**
 * @brief Collect channel counts
 *
 * This is used to nondeterministically merge channels with a copy synchronyzer
 * that has multiple inputs.
 */
struct smx_collector_s
{
    pthread_mutex_t     col_mutex;  /**< mutual exclusion */
    pthread_cond_t      col_cv;     /**< conditional variable to trigger box */
    int                 count;      /**< collection of channel counts */
    smx_channel_state_t state;      /**< state of the channel */
};

/**
 * @brief Streamix fifo structure
 *
 * The fifo structure is blocking on write if all buffers are occupied and
 * blocking on read if all buffer spaces are empty. The blocking pattern
 * can be changed by decoupling either the input, the output or both.
 */
struct smx_fifo_s
{
    smx_fifo_item_t*  head;      /**< pointer to the heda of the FIFO */
    smx_fifo_item_t*  tail;      /**< pointer to the tail of the FIFO */
    smx_msg_t*        backup;    /**< ::smx_msg_s, msg space for decoupling */
    int     overwrite;           /**< counts number of overwrite operations */
    int     copy;                /**< counts number of copy operations */
    int     count;               /**< counts occupied space */
    int     length;              /**< size of the FIFO */
};

/**
 * @brief A single FIFO item of a circular double-linked-list
 */
struct smx_fifo_item_s
{
    smx_msg_t*       msg;        /**< ::smx_msg_s */
    smx_fifo_item_t* next;       /**< pointer to the next item */
    smx_fifo_item_t* prev;       /**< pointer to the previous item */
};

/**
 * @brief timed guard to limit communication rate
 */
struct smx_guard_s
{
    int             fd;     /**< file descriptor pointing to timer */
    struct timespec iat;    /**< minumum inter-arrival-time */
};

/**
 * @brief A Streamix message structure
 *
 * The structure contains handlers that can be used to manipulate data.
 * These handlers are provided by the box implementation.
 */
struct smx_msg_s
{
    unsigned long id;               /**< the unique message id */
    char* type;                     /**< an optional string indicating the msg data type */
    bool prevent_backup;            /**< prevents msg backups from being created */
    void* data;                     /**< pointer to the data */
    int   size;                     /**< size of the data */
    void* (*copy)( void*, size_t ); /**< pointer to a fct making a deep copy */
    void  (*destroy)( void* );      /**< pointer to a fct that frees data */
    void* (*unpack)( void* );       /**< pointer to a fct that unpacks data */
};

/**
 * Common fields of a streamix net.
 */
struct smx_net_s
{
    bool                has_profiler; /**< is profiler enabled? */
    bool                has_type_filter; /**< is type filter enabled? */
    /** the thread priority of the net. 0 means ET, >0 means TT */
    int                 priority;
    unsigned int        id;           /**< a unique net id */
    unsigned long       count;        /**< loop counter */
    pthread_barrier_t*  init_done;    /**< pointer to the init sync barrier */
    zlog_category_t*    cat;          /**< the log category */
    smx_net_sig_t*      sig;          /**< the net port signature */
    /** port name on which to receive the dynamic configuration  */
    const char*         conf_port_name;
    void*               attr;         /**< custom attributes of special nets */
    void*               conf;         /**< pointer to the XML configuration */
    const char*         name;         /**< the name of the net */
    const char*         impl;         /**< the name of the box implementation */
    struct timespec     start_wall;   /**< start time of a net (after init) */
    struct timespec     end_wall;     /**< end time of a net (befoer cleanup) */
};

/**
 * The signature of a net
 */
struct smx_net_sig_s
{
    struct {
        int count;                  /**< the number of connected input ports */
        int len;                    /**< the number of input ports */
        smx_channel_t** ports;      /**< an array of channel pointers */
    } in;                           /**< input channels */
    struct {
        int count;                  /**< the number of connected output ports */
        int len;                    /**< the number of output ports */
        smx_channel_t** ports;      /**< an array of channel pointers */
    } out;                          /**< output channels */
};

#endif /* SMXTYPES_H */
