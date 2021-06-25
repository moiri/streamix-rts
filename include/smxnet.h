/**
 * @file     smxnet.h
 * @author   Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Net definitions for the runtime system library of Streamix
 */

#include <bson.h>
#include <stdbool.h>
#include "smxtypes.h"
#include "smxlog.h"

#ifndef SMXNET_H
#define SMXNET_H

/**
 * @def SMX_LOG()
 *
 * This macro allows to log events to the log file.
 *
 * @param h
 *  The pointer to the net handler.
 * @param level
 *  Note that this parameter is not a string but the literal name of the box
 *  (without quotation marks). Use one of the following levels:
 *   - `fatal`:   everything went wrong
 *   - `error`:   an error occured and the net has to terminate
 *   - `warn`:    The net can continue to execute but the result might be faulty
 *   - `notice`:  A useful information to report that occurs on initialisation,
 *                cleanup, or rarely during execution.
 *   - `info`:    A useful information to report that occurs during execution.
 *   - `debug`:   Debug information.
 * @param format
 *  The [printf](https://linux.die.net/man/3/printf) format string.
 * @param ...
 *  The required arguments to be replaced in the printf format string.
 *
 */
#define SMX_LOG( h, level, format, ... )\
    SMX_LOG_NET( h, level, format, ##__VA_ARGS__ )

/**
 * @def SMX_NET_GET_CONF()
 *
 * Get the net configuration structure.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  The net configuration structure of type `bson_t`.
 */
#define SMX_NET_GET_CONF( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->conf )

/**
 * @def SMX_NET_GET_COUNT()
 *
 * Get the current net loop count.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  The current net loop count or -1 on failure.
 */
#define SMX_NET_GET_COUNT( h )\
    ( ( h == NULL ) ? -1 : ( ( smx_net_t* )h )->count )

/**
 * @def SMX_NET_GET_ID()
 *
 * Get the net id.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  The net id of type `unsigned int`.
 */
#define SMX_NET_GET_ID( h ) ( ( h == NULL ) ? -1 : ( ( smx_net_t* )h )->id )

/**
 * @def SMX_NET_GET_NAME()
 *
 * Get the net name.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  A pointer to the net name of type `const char*`.
 */
#define SMX_NET_GET_NAME( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->name )

/**
 * @def SMX_NET_GET_IMPL()
 *
 * Get the box implementation name.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  A pointer to the box implementation name of type `const char*`.
 */
#define SMX_NET_GET_IMPL( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->impl )

/**
 * @def SMX_NET_GET_PRIO()
 *
 * Get the net thread priority.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  The net thread priority or -1 on failure.
 */
#define SMX_NET_GET_PRIO( h )\
    ( ( h == NULL ) ? -1 : ( ( smx_net_t* )h )->priority )

/**
 * @def SMX_NET_GET_STATE()
 *
 * Get the allocated state of the net instance.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  A pointer to the state or NULL.
 */
#define SMX_NET_GET_STATE( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->state )

/**
 * @def SMX_NET_GET_SHARED_STATE()
 *
 * Get the allocated shared state of the net instance.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  A pointer to the shared state or NULL.
 */
#define SMX_NET_GET_SHARED_STATE( h )\
    ( ( h == NULL ) ? NULL : ( ( smx_net_t* )h )->shared_state )

/**
 * @def SMX_NET_IS_FIRST_RUN()
 *
 * Check if the current net run is the first run.
 *
 * @param h
 *  The pointer to the net handler.
 * @return
 *  1 if net is executing for the first time, 0 otherwise
 */
#define SMX_NET_IS_FIRST_RUN( h )\
    ( ( h == NULL ) ? 0 : ( ( ( smx_net_t* )h )->count == 1 ? 1 : 0 ) )


/**
 * Read from a collector of a net.
 *
 * @param h         pointer to the net handler
 * @param collector pointer to the net collector structure
 * @param in        pointer to the input port array
 * @param count_in  number of input ports
 * @param last_idx  pointer to the state variable storing the last port index
 * @return          the message that was read or NULL if no message was read
 */
smx_msg_t* smx_net_collector_read( void* h, smx_collector_t* collector,
        smx_channel_t** in, int count_in, int* last_idx );

/**
 * Create a new net instance. This includes
 *  - creating a zlog category
 *  - assigning the net-specifix XML configuartion
 *  - assigning the net signature
 *
 * @param id        a unique net identifier
 * @param name      the name of the net
 * @param impl      the name of the box implementation
 * @param cat_name  the name of the zlog category
 * @param rts       a pointer to the main rts structure
 * @param prio      the RT thread priority (0 means no rt thread)
 * @return          a pointer to the ctreated net or NULL
 */
smx_net_t* smx_net_create( unsigned int id, const char* name,
        const char* impl, const char* cat_name, smx_rts_t* rts, int prio );

/**
 * Destroy a net
 *
 * @param h         pointer to the net handler
 */
void smx_net_destroy( smx_net_t* h );

/**
 * Get a boolean property configuration setting for the current net.
 *
 * The function hiearchically searches for a confic that is specific for
 *  1. this net id
 *  2. this net name
 *  3. the box implementation of this net
 *  4. all nets
 *
 *  If a hit is found, the function returns te config and does not continue
 *  searching.
 *
 * @param conf
 *  The input buffer of the app configuration
 * @param name
 *  The name of the net
 * @param impl
 *  The box implemntation name
 * @param id
 *  The id of the net
 * @param prop
 *  The name of the property.
 *
 * @return
 *  the boolean property
 */
bool smx_net_get_boolean_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop );

/**
 * Get a int property configuration setting for the current net.
 *
 * The function hiearchically searches for a confic that is specific for
 *  1. this net id
 *  2. this net name
 *  3. the box implementation of this net
 *  4. all nets
 *
 *  If a hit is found, the function returns te config and does not continue
 *  searching.
 *
 * @param conf
 *  The input buffer of the app configuration
 * @param name
 *  The name of the net
 * @param impl
 *  The box implemntation name
 * @param id
 *  The id of the net
 * @param prop
 *  The name of the property.
 *
 * @return
 *  the boolean property
 */
int smx_net_get_int_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop );

/**
 * Get the appropriate json configuration for the current net.
 *
 * The function hiearchically searches for a confic that is specific for
 *  1. this net id
 *  2. this net name
 *  3. the box implementation of this net
 *  4. all nets
 *
 *  If a hit is found, the function returns te config and does not continue
 *  searching.
 *
 * @param h
 *  pointer to the net handler
 * @param conf
 *  The input buffer of the app configuration
 * @param name
 *  The name of the net
 * @param impl
 *  The box implemntation name
 * @param id
 *  The id of the net
 *
 * @return
 *  0 on success, -1 if nothing was found.
 */
int smx_net_get_json_doc( smx_net_t* h, bson_t* conf, const char* name,
        const char* impl, unsigned int id );

/**
 * Get the json configuration for a given search string.
 *
 * @param h
 *  pointer to the net handler
 * @param conf
 *  The input buffer of the app configuration
 * @param search_str
 *  The hierachical search string
 * @return
 *  0 on success, -1 if nothing was found.
 */
int smx_net_get_json_doc_item( smx_net_t* h, bson_t* conf,
        const char* search_str );

/**
 * Get a string property configuration setting for the current net.
 *
 * The function hiearchically searches for a confic that is specific for
 *  1. this net id
 *  2. this net name
 *  3. the box implementation of this net
 *  4. all nets
 *
 *  If a hit is found, the function returns te config and does not continue
 *  searching.
 *
 * @param conf
 *  The input buffer of the app configuration
 * @param name
 *  The name of the net
 * @param impl
 *  The box implemntation name
 * @param id
 *  The id of the net
 * @param prop
 *  The name of the property.
 *
 * @return
 *  the string property
 */
const char* smx_net_get_string_prop( bson_t* conf, const char* name, const char* impl,
        unsigned int id, const char* prop );

/**
 * Initialise a net
 *
 * @param h            pointer to the net handler
 * @param indegree     number of input ports
 * @param outdegree    number of output ports
 */
void smx_net_init( smx_net_t* h, int indegree, int outdegree );

/**
 * Logs a warning if the net rate is lower or higher that the expected net rate
 * by 20%.
 *
 * @param h
 *  A pointer to the net handler.
 */
void smx_net_report_rate_warning( smx_net_t* h );

/**
 * @brief create pthred of net
 *
 * @param ths               the target array to store the thread id
 * @param idx               the index of where to store the thread id in the
 *                          target array
 * @param box_impl( arg )   function pointer to the box implementation
 * @param h                 pointer to the net handler
 * @return                  0 on success, -1 on failure
 */
int smx_net_run( pthread_t* ths, int idx, void* box_impl( void* arg ), void* h );

/**
 * @brief the start routine of a thread associated to a box
 *
 * @param h                 pointer to the net handler
 * @param impl( arg )       pointer to the net implementation function
 * @param init( arg )       pointer to the net intitialisation function
 * @param cleanup( arg )    pointer to the net cleanup function
 * @return                  returns NULL
 */
void* smx_net_start_routine( smx_net_t* h, int impl( void*, void* ),
        int init( void*, void** ), void cleanup( void*, void* ) );


/**
 * The same as smx_net_start_routine() but this includes handlers to initialize
 * and cleanup shared state.
 *
 * @param h
 *  A pointer to the net handler
 * @param impl( arg )
 *  A pointer to the net implementation function
 * @param init( arg )
 *  A pointer to the net intitialisation function
 * @param cleanup( arg )
 *  A pointer to the net cleanup function
 * @param init_shared( arg )
 *  A pointer to the shared state intitialisation function
 * @param cleanup_shared( arg )
 *  A pointer to the shared state cleanup function
 * @param shared_state_key
 *  The key under which the shared state will be stored.
 * @return
 *  returns NULL
 */
void* smx_net_start_routine_with_shared_state( smx_net_t* h,
        int impl( void*, void* ), int init( void*, void** ),
        void cleanup( void*, void* ), int init_shared( void*, void** ),
        void cleanup_shared( void* ), const char* shared_state_key );

/**
 * @brief Set all channel states to end and send termination signal to all
 * output channels.
 *
 * @param h         pointer to the net handler
 */
void smx_net_terminate( smx_net_t* h );

/**
 * @brief Update the state of the box
 *
 * Update the state of the box to indicate wheter computaion needs to scontinue
 * or terminate. The state can either be forced by the box implementation (see
 * \p state) or depends on the state of the triggering producers.
 * Note that non-triggering producers may still be alive but the thread will
 * still terminate if all triggering producers are terminated. This is to
 * prevent a while(1) type of behaviour because no blocking will occur to slow
 * the thread execution.
 *
 * @param h         pointer to the net handler
 * @param state     state set by the box implementation. If set to
 *                  SMX_NET_CONTINUE, the box will not terminate. If set to
 *                  SMX_NET_END, the box will terminate. If set to
 *                  SMX_NET_RETURN (or 0) this function will determine wheter
 *                  a box terminates or not
 * @return          SMX_NET_CONTINUE if there is at least one triggeringr
 *                  producer alive. SMX_BOX_TERINATE if all triggering
 *                  prodicers are terminated.
 */
int smx_net_update_state( smx_net_t* h, int state );

/**
 * Wait for all nets to terminate by joining the net threads.
 *
 * @param th
 *  The thread id
 */
void smx_net_wait_end( pthread_t th );

#endif /* SMXNET_H */
