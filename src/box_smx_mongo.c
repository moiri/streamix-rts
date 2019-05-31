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
#include "smxnet.h"
#include "smxutils.h"

#define SMX_MONGO_APP   "smx_mongo"

/*****************************************************************************/
int smx_mongo( void* h, void* state )
{
    char snum[7];
    int id_ui;
    smx_msg_t* msg_net;
    smx_msg_t* msg_ui;
    smx_mongo_msg_t* mg_msg;
    smx_mongo_state_t* mg_state = state;
    net_smx_mongo_t* net = SMX_SIG( h );

    msg_net = smx_channel_read( h, net->in.port_net );
    mg_msg = msg_net->data;

    if( mg_msg->ts.tv_sec != mg_state->last_ts.tv_sec )
    {
        if( mg_state->doc != NULL )
            smx_mongo_doc_finish( state );

        msg_ui = smx_channel_read( h, net->in.port_ui );
        id_ui = ( msg_ui == NULL ) ? 0 : *( int* )msg_ui->data;
        smx_mongo_doc_init( state, mg_msg, id_ui );
        smx_msg_destroy( msg_ui, 1 );
    }
    sprintf( snum, "%ld", mg_msg->ts.tv_nsec/1000 );
    BSON_APPEND_ARRAY( mg_state->data, snum,
            bson_new_from_json( ( const uint8_t * )mg_msg->j_data, -1, NULL ) );

    mg_state->last_ts = mg_msg->ts;

    smx_msg_destroy( msg_net, 1 );

    return SMX_NET_RETURN;
}

/*****************************************************************************/
void smx_mongo_cleanup( void* state )
{
    smx_mongo_state_t* ms = state;
    if( ms->doc != NULL )
        smx_mongo_doc_finish( state );
    mongoc_collection_destroy( ms->collection );
    mongoc_database_destroy( ms->database );
    mongoc_client_destroy( ms->client );
    mongoc_uri_destroy( ms->uri );
    mongoc_cleanup();
}

/*****************************************************************************/
void smx_mongo_doc_finish( smx_mongo_state_t* state )
{
    bson_append_document_end( state->doc, state->data );
    mongoc_collection_insert_one( state->collection, state->doc,
            NULL, NULL, NULL );
    bson_destroy( state->doc );
    bson_free( state->data );
    state->doc = NULL;
    state->data = NULL;
}

/*****************************************************************************/
void smx_mongo_doc_init( smx_mongo_state_t* state, smx_mongo_msg_t* msg_net,
        int msg_ui )
{
    state->doc = smx_malloc( sizeof( bson_t ) );
    state->data = smx_malloc( sizeof( bson_t ) );
    bson_init( state->doc );
    BSON_APPEND_INT32( state->doc, "ui", msg_ui );
    BSON_APPEND_TIME_T( state->doc, "ts", msg_net->ts.tv_sec );
    BSON_APPEND_DOCUMENT_BEGIN( state->doc, "data", state->data );
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
    ms->data = NULL;
    ms->doc = NULL;

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
