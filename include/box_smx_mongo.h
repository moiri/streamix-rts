/**
 * Mongo DB client box implementation
 *
 * @file    box_smx_mongo.h
 * @author  Simon Maurer
 */

#include <mongoc.h>
#include "smxch.h"
#include "smxnet.h"

#ifndef SMX_MONGO_H
#define SMX_MONGO_H

typedef struct smx_mongo_state_s smx_mongo_state_t;
typedef struct net__smx_mongo_s net__smx_mongo_t;

struct net__smx_mongo_s
{
    struct {
        smx_channel_t*   port_profile;
        smx_channel_t**  ports;
        int              count;
    } in;
    struct {
        smx_channel_t**  ports;
        int              count;
    } out;
};

struct smx_mongo_state_s
{
    mongoc_uri_t* uri;
    mongoc_client_t* client;
    mongoc_database_t* database;
    mongoc_collection_t* collection;
};

/**
 *
 */
int _smx_mongo( void* h, void* state );

/**
 *
 */
void _smx_mongo_cleanup( void* state );

/**
 *
 */
int _smx_mongo_init( void* h, void** state );

/**
 *
 */
int smx_mongo( void* h, void* state );

/**
 *
 */
void smx_mongo_cleanup( void* state );

/**
 *
 */
int smx_mongo_init( void* h, void** state );

#endif

