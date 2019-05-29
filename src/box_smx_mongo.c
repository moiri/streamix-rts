/**
 * Mongo DB client box implementation
 *
 * @file    box_smx_mongo.h
 * @author  Simon Maurer
 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "box_smx_mongo.h"
#include "smxlog.h"
#include "smxutils.h"

#define SMX_MONGO_APP   "smx_mongo"

/*****************************************************************************/
int _smx_mongo( void* h, void* state )
{
    return smx_mongo( h, state );
}

/*****************************************************************************/
void _smx_mongo_cleanup( void* state )
{
    smx_mongo_cleanup( state );
}

/*****************************************************************************/
int _smx_mongo_init( void* h, void** state )
{
    return smx_mongo_init( h, state );
}

/*****************************************************************************/
int smx_mongo( void* h, void* state )
{
    ( void )( h );
    ( void )( state );
    return SMX_NET_RETURN;
}

/*****************************************************************************/
void smx_mongo_cleanup( void* state )
{
    smx_mongo_state_t* ms = state;
    mongoc_collection_destroy( ms->collection );
    mongoc_database_destroy( ms->database );
    mongoc_client_destroy( ms->client );
    mongoc_uri_destroy( ms->uri );
    mongoc_cleanup();
}

/*****************************************************************************/
int smx_mongo_init( void* h, void** state )
{
    smx_mongo_state_t* ms;
    bson_error_t error;
    ( void )( h );

    xmlNodePtr cur = SMX_NET_GET_CONF( h );
    xmlChar* uri_str = NULL;
    xmlChar* db_name = NULL;

    if( cur == NULL )
    {
        SMX_LOG_NET( h, error, "invalid box configuartion" );
        return -1;
    }

    uri_str = xmlGetProp(cur, ( const xmlChar* )"uri");
    if( uri_str == NULL )
    {
        SMX_LOG_NET( h, error, "invalid box configuration, no property 'uri'" );
        return -1;
    }

    db_name = xmlGetProp(cur, (const xmlChar*)"db");
    if( db_name == NULL )
    {
        SMX_LOG_NET( h, error, "invalid box configuration, no property 'db'" );
        return -1;
    }

    *state = smx_malloc( sizeof( struct smx_mongo_state_s ) );
    ms = *state;

    mongoc_init();

    ms->uri = mongoc_uri_new_with_error( ( const char* )uri_str, &error );
    if( !ms->uri )
    {
        SMX_LOG_MAIN( main, error, "failed to parse URI '%s': %s", uri_str,
                error.message );
        xmlFree( uri_str );
        return -1;
    }

    ms->client = mongoc_client_new_from_uri( ms->uri );
    if( !ms->client )
    {
        SMX_LOG_MAIN( main, error, "failed to create a mongoDB client" );
        return -1;
    }

    if( !mongoc_client_set_appname( ms->client, SMX_NET_GET_NAME( h ) ) )
        SMX_LOG_MAIN( main, warn, "failed to set mongoDB app name '%s'",
                SMX_NET_GET_NAME( h ) );

    ms->database = mongoc_client_get_database( ms->client,
            ( const char* )db_name );
    if( !ms->database )
    {
        SMX_LOG_MAIN( main, error, "failed to create database '%s'", db_name );
        return -1;
    }

    ms->collection = mongoc_client_get_collection( ms->client,
            ( const char* )db_name, SMX_NET_GET_NAME( h ) );
    if( !ms->collection )
    {
        SMX_LOG_MAIN( main, error, "failed to create collection '%s'",
                SMX_NET_GET_NAME( h ) );
        return -1;
    }

    xmlFree( uri_str );
    xmlFree( db_name );

    return 0;
}
