/**
 * Mongo DB client box implementation
 *
 * @file    box_smx_mongo.h
 * @author  Simon Maurer
 *
 * Example of a data-base strcuture for one application. Each application has
 * one database with a collection for each net.
 *
 * ui_document of ui collection
 * {
 *   "_id": 2,
 *   "id_trial": 24,
 *   "id_subject": 9,
 * }
 *
 * sig_document of #net collection
 * {
 *   "header": [["x", "y"], "state"],
 *   "net": "hanuele"
 * }
 *
 * item_document of #net collection
 * {
 *    "id_ui": 2,
 *    "ts": "2019-05-31T10:48:22",
 *    "data": {
 *      "1": [[0.238, 0.348], "fly"],
 *      "3": [[0.340, 0.423], "jump"],
 *      (...)
 *    }
 * }
 */

#include <bson.h>
#include <mongoc.h>
#include <time.h>
#include "smxch.h"

#ifndef SMX_MONGO_H
#define SMX_MONGO_H

typedef struct smx_mongo_state_s smx_mongo_state_t;
typedef struct smx_mongo_msg_s smx_mongo_msg_t;
typedef struct net_smx_mongo_s net_smx_mongo_t;

/**
 * The signature of _smx_mongo box
 */
struct net_smx_mongo_s
{
    struct {
        smx_channel_t*   port_ui;   /**< the id of the session manager item */
        smx_channel_t*   port_net; /**< the data item (see ::smx_mongo_msg_s)*/
        smx_channel_t**  ports;
        int              count;
    } in;
    struct {
        smx_channel_t**  ports;
        int              count;
    } out;
};

/**
 * The expected mongo db message structure
 */
struct smx_mongo_msg_s
{
    struct timespec ts; /**< the timestamp of the data item */
    char* j_data;       /**< a valid json string holding the data */
};

/**
 * The state variable for the smx_mongo box implemetation
 */
struct smx_mongo_state_s
{
    mongoc_uri_t* uri;                  /**< pointer to the db uri */
    mongoc_client_t* client;            /**< ipointer to the client instance */
    mongoc_database_t* database;        /**< pointer to the db instance */
    mongoc_collection_t* collection;    /**< pointer to the db collection */
    struct timespec last_ts;            /**< the timestamp of the last message */
    bson_t* doc;                        /**< pointer to the item_document */
    bson_t* data;                       /**< pointer to the embedded data document */
};

/**
 * @brief the box implementattion of a mongo db writer
 *
 * The data types of the messages passed to the signature are as follows:
 * - in net smx_mongo_msg_t*
 *    The Pointer to a mongo message which contains a json string data entry
 *    and a timestamp (::smx_mongo_msg_s).
 * - in ui to_be_seen
 *    The Pointer to a ui message which contains
 *
 * @param h     a pointer to the signature
 * @param state a pointer to the persitent state which was allocated in the
 *              init function
 * @return      returns the state number of the box
 */
int smx_mongo( void* h, void* state );

/**
 * Cleanup function of the 'smx_mongo' box implementation.
 *
 * @param state
 *  Pointer to the state structure.
 */
void smx_mongo_cleanup( void* state );

/**
 * Helper function to create a mongo msg.
 *
 * @param j_data    an allocated json string holding the data
 * @param sec       The timestamp in seconds
 * @param nsec      The nanosecond part of the timestamp
 */
smx_mongo_msg_t* smx_mongo_create_msg( char* j_data, time_t sec, long nsec );

/**
 * Stop appending elements to the embedded BSON document structure and commit
 * the BSON structure to the database. Cleanup the data.
 *
 * @param state a pointer to the persitent state which was allocated in the
 *              init function
 */
void smx_mongo_doc_finish( smx_mongo_state_t* state );

/**
 * Create and initialize a BSON document with an ui reference, a timestamp and
 * an embedded BSON document `data` which can be extended with data arriving as
 * messages to the net.
 *
 * @param state     a pointer to the persitent state which was allocated in the
 *                  init function.
 * @param msg_net   a pointer to the message received at the net port.
 * @param msg_ui    the reference id to the ui DB information.
 */
void smx_mongo_doc_init( smx_mongo_state_t* state, smx_mongo_msg_t* msg_net,
        int msg_ui );

/**
 * Init function of the 'smx_mongo' box implementation. This function reads
 * XML configurations and creates a mongo client and a mongo db collection.
 *
 * @param handler
 *  The box signature handler.
 * @param state
 *  Pointer to the state structure pointer.
 * @return
 *  0 on success, -1 on failure
 */
int smx_mongo_init( void* h, void** state );

#endif
