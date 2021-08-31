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
#include <bson.h>

#ifndef SMXTYPES_H
#define SMXTYPES_H

#define SMX_CONFIG_MAX_MAP_ITEMS      1000
#define SMX_MSG_RAW_TYPE 0
#define SMX_MSG_RAW_TYPE_STR "raw"
#define SMX_MSG_INT_TYPE 1
#define SMX_MSG_INT_TYPE_STR "int"
#define SMX_MSG_DOUBLE_TYPE 2
#define SMX_MSG_DOUBLE_TYPE_STR "double"
#define SMX_MSG_BOOL_TYPE 3
#define SMX_MSG_BOOL_TYPE_STR "bool"
#define SMX_MSG_STRING_TYPE 4
#define SMX_MSG_STRING_TYPE_STR "string"
#define SMX_MSG_JSON_TYPE 5
#define SMX_MSG_JSON_TYPE_STR "json"

/**
 * The number of maximal allowed nets in one streamix application.
 */
#define SMX_MAX_NETS 1000

/**
 * The number of maximal allowed channel in one streamix application.
 */
#define SMX_MAX_CHS 10000

/**
 * The streamix channel error type. Refer to the error enumeration definition
 * for more details #smx_channel_err_e.
 */
typedef enum smx_channel_err_e smx_channel_err_t;
typedef enum smx_channel_state_e smx_channel_state_t; /**< #smx_channel_state_e */
typedef enum smx_channel_type_e smx_channel_type_t;   /**< #smx_channel_type_e */
/** #smx_config_error_e */
typedef enum smx_config_error_e smx_config_error_t;
/** #smx_config_map_error_e */
typedef enum smx_config_map_error_e smx_config_map_error_t;
/** #smx_profiler_action_e */
typedef enum smx_profiler_action_ch_e smx_profiler_action_ch_t;
typedef enum smx_profiler_action_msg_e smx_profiler_action_msg_t;
typedef enum smx_profiler_action_net_e smx_profiler_action_net_t;

typedef struct smx_rts_s smx_rts_t; /**< ::smx_rts_s */
typedef struct smx_rts_shared_state_s smx_rts_shared_state_t; /**< ::smx_rts_shared_state_s */
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
/** ::smx_msg_tsmem_data_map_s */
typedef struct smx_config_data_map_s smx_config_data_map_t;
/** ::smx_msg_tsmem_data_maps_s */
typedef struct smx_config_data_maps_s smx_config_data_maps_t;

/**
 * The error state of a channel end
 */
enum smx_channel_err_e
{
    SMX_CHANNEL_ERR_NONE = 0,      /**< no error */
    SMX_CHANNEL_ERR_NO_DEFAULT = -99,    /**< no default message for decoupled read */
    SMX_CHANNEL_ERR_NO_TARGET = -98,     /**< connecting net has terminated */
    SMX_CHANNEL_ERR_DL_MISS = -97,       /**< connecting net missed its deadline */
    SMX_CHANNEL_ERR_NO_DATA = -96,       /**< unexpectedly, the channel has no data */
    SMX_CHANNEL_ERR_NO_SPACE = -95,      /**< unexpectedly, the channel has no space */
    SMX_CHANNEL_ERR_FILTER = -94,        /**< the msg type does not match the filter */
    SMX_CHANNEL_ERR_UNINITIALISED = -93, /**< the channel was never initialised */
    SMX_CHANNEL_ERR_TIMEOUT = -92,       /**< the channel operation timed out */
    SMX_CHANNEL_ERR_CV = -91,            /**< the conditional variable lock failed */
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
    SMX_CONFIG_ERROR_NO_ERROR = 0,  /**< No error */
    SMX_CONFIG_ERROR_BAD_TYPE = -199,  /**< The item exists but the type does not match */
    SMX_CONFIG_ERROR_NO_VALUE = -198   /**< The item does not exist */
};

/**
 * The list of config read errors.
 */
enum smx_config_map_error_e
{
    SMX_CONFIG_MAP_ERROR_NO_ERROR = 0,  /**< No error */
    SMX_CONFIG_MAP_ERROR_BAD_ROOT_TYPE = -299,
    SMX_CONFIG_MAP_ERROR_BAD_MAP_TYPE = -298,
    SMX_CONFIG_MAP_ERROR_MISSING_SRC_KEY = -297,
    SMX_CONFIG_MAP_ERROR_MISSING_SRC_DEF = -296,
    SMX_CONFIG_MAP_ERROR_MISSING_TGT_KEY = -295,
    SMX_CONFIG_MAP_ERROR_MISSING_TGT_DEF = -294,
    SMX_CONFIG_MAP_ERROR_MAP_COUNT_EXCEEDED = -293,
    SMX_CONFIG_MAP_ERROR_NO_MAP_ITEM = -292,
    SMX_CONFIG_MAP_ERROR_BAD_TYPE_OPTION = -291,
};

/**
 * The different actions a profiler can log.
 */
enum smx_profiler_action_ch_e
{
    SMX_PROFILER_ACTION_CH_READ,           /**< read from a channel. */
    SMX_PROFILER_ACTION_CH_READ_BLOCK,     /**< blocking at read from a channel. */
    SMX_PROFILER_ACTION_CH_READ_COLLECTOR, /**< read from a collector. */
    SMX_PROFILER_ACTION_CH_READ_COLLECTOR_BLOCK,      /**< blocking at collector read. */
    SMX_PROFILER_ACTION_CH_WRITE,          /**< write to a channel. */
    SMX_PROFILER_ACTION_CH_WRITE_BLOCK,    /**< blocking at write to a channel. */
    SMX_PROFILER_ACTION_CH_WRITE_COLLECTOR,/**< write to a collector. */
    SMX_PROFILER_ACTION_CH_OVERWRITE,      /**< overwrite a message in a channel. */
    SMX_PROFILER_ACTION_CH_DISMISS,        /**< dismiss a message in a channel. */
    SMX_PROFILER_ACTION_CH_DUPLICATE,      /**< duplicate a message in a channel. */
    SMX_PROFILER_ACTION_CH_DL_MISS_SRC,    /**< rt producer missed a deadline. */
    SMX_PROFILER_ACTION_CH_DL_MISS_SRC_CP, /**< rt producer missed a deadline, msg duplicated. */
    SMX_PROFILER_ACTION_CH_TT_MISS_SRC,    /**< non-rt producer missed a tt interval. */
    SMX_PROFILER_ACTION_CH_TT_MISS_SRC_CP, /**< non-rt producer missed a tt interval, msg duplicated. */
    SMX_PROFILER_ACTION_CH_DL_MISS_SINK,   /**< rt consumer missed a deadline. */
    SMX_PROFILER_ACTION_CH_TT_MISS_SINK    /**< non-rt consumer missed a tt interval. */
};

/**
 * The different actions a profiler can log.
 */
enum smx_profiler_action_msg_e
{
    SMX_PROFILER_ACTION_MSG_CREATE,         /**< create a msg. */
    SMX_PROFILER_ACTION_MSG_DESTROY,        /**< destroy a msg. */
    SMX_PROFILER_ACTION_MSG_COPY_START,     /**< copy a message. */
    SMX_PROFILER_ACTION_MSG_COPY_END        /**< copy a message. */
};

/**
 * The different actions a profiler can log.
 */
enum smx_profiler_action_net_e
{
    SMX_PROFILER_ACTION_NET_START,      /**< start a net loop. */
    SMX_PROFILER_ACTION_NET_START_IMPL, /**< start a net implementation. */
    SMX_PROFILER_ACTION_NET_END_IMPL,   /**< end a net implementation. */
    SMX_PROFILER_ACTION_NET_END         /**< end a net loop. */
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
    char*               name;       /**< name of the channel */
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
    bool ( *content_filter )( smx_net_t* net, smx_msg_t* msg );
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
    int                 ch_count;   /**< number of connected channels */
    smx_channel_state_t state;      /**< state of the channel */
};

/**
 * This structure defines an input key mapping
 */
struct smx_config_data_map_s
{
    const char* src_path;    /**< The source value location (use dot-notation) */
    const char* tgt_path;    /**< The target value location (use dot-notation) */
    bson_iter_t tgt_iter;    /**< The target value location iterator  */
    bson_t* src_payload;
    bson_type_t type;
    union {
        double v_double;
        int64_t v_int64;
        int32_t v_int32;
        bool v_bool;
    } fallback;
};

/**
 *
 */
struct smx_config_data_maps_s
{
    smx_config_data_map_t items[SMX_CONFIG_MAX_MAP_ITEMS];
    int count;
    bool is_extended;
    bson_t* tgt_payload;
    bson_t mapped_payload;
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
    unsigned long long id;          /**< the unique message id */
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
    /** The expected loop rate per second. */
    int                 expected_rate;
    zlog_category_t*    cat;          /**< the log category */
    smx_net_sig_t*      sig;          /**< the net port signature */
    /** port name on which to receive the dynamic configuration  */
    const char*         conf_port_name;
    /** read timeout on dynamic conf port in milliseconds */
    int                 conf_port_timeout;
    void*               attr;         /**< custom attributes of special nets */
    void*               conf;         /**< pointer to the net configuration */
    bson_t*             dyn_conf;     /**< pointer to the dynamic configuration */
    bson_t*             static_conf;  /**< pointer to the static configuration */
    char*               name;         /**< the name of the net */
    char*               impl;         /**< the name of the box implementation */
    void*               state;
    void*               shared_state;
    const char*         shared_state_key;
    smx_rts_t*          rts;
    struct timespec     last_count_wall;   /**< start time of a net (after init) */
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

/**
 * A shared state item
 */
struct smx_rts_shared_state_s
{
    /**
     * The name of the shared state item. This is used to reference the item.
     */
    const char* key;
    /**
     * The actual state.
     */
    void* state;
    /**
     * The cleanup function to free the shared state item.
     */
    void ( *cleanup )( void* );
};

/**
 * The main RTS structure holding information about the streamix network.
 */
struct smx_rts_s
{
    int ch_cnt;                     /**< the number of channels of the system */
    int net_cnt;                    /**< the number of nets of the system */
    int shared_state_cnt;
    pthread_barrier_t pre_init_done;/**< the barrier for syncing pre initialisation */
    pthread_barrier_t init_done;    /**< the barrier for syncing initialisation */
    void* conf;                     /**< the application configuration */
    pthread_t ths[SMX_MAX_NETS];    /**< the array holding all thread ids */
    smx_channel_t* chs[SMX_MAX_CHS];/**< the array holding all channel pointers */
    smx_net_t* nets[SMX_MAX_NETS];  /**< the array holdaing all net pointers */
    struct timespec start_wall;     /**< the walltime of the application start */
    struct timespec end_wall;       /**< the walltime of the application end. */
    smx_rts_shared_state_t* shared_state[SMX_MAX_NETS];
    pthread_mutex_t net_mutex;      /**< mutual exclusion */
};

#endif /* SMXTYPES_H */
