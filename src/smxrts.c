/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "smxrts.h"
#include "smxlog.h"

#define XML_PATH        "app.xml"
#define XML_APP         "app"
#define XML_LOG         "log"
#define XML_PROFILE     "profile"

#define JSON_LOG_NET "{\"%s\":{\"name\":\"%s\",\"id\":%d}}"
#define JSON_LOG_CH "{\"%s\":{\"name\":\"%s\",\"id\":%d,\"len\":%d}}"
#define JSON_LOG_CONNECT "{\"connect\":{\"ch\":{\"name\":\"%s\",\"id\":%d,\"mode\":\"%s\"},\"net\":{\"name\":\"%s\",\"id\":%d}}}"
#define JSON_LOG_ACCESS "{\"%s\":{\"tgt\":\"%s\",\"count\":%d}}"
#define JSON_LOG_ACTION "{\"%s\":{\"tgt\":\"%s\"}}"
#define JSON_LOG_MSG "{\"%s\":{\"id\":%lu}}"

/*****************************************************************************/
void smx_program_cleanup( smx_rts_t* rts )
{
    xmlFreeDoc( rts->conf );
    xmlCleanupParser();
    free( rts );
    SMX_LOG_MAIN( main, notice, "end main thread" );
    smx_log_cleanup();
    exit( EXIT_SUCCESS );
}

/*****************************************************************************/
smx_rts_t* smx_program_init()
{
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;

    /* required for thread safety */
    xmlInitParser();

    /*parse the file and get the DOM */
    doc = xmlParseFile( XML_PATH );

    if( doc == NULL )
    {
        printf( "error: could not parse the app config file '%s'\n", XML_PATH );
        exit( 0 );
    }

    cur = xmlDocGetRootElement( doc );
    if( cur == NULL || xmlStrcmp(cur->name, ( const xmlChar* )XML_APP ) )
    {
        printf( "error: app config root node name is '%s' instead of '%s'\n",
                cur->name, XML_APP );
        exit( 0 );
    }
    conf = xmlGetProp( cur, ( const xmlChar* )XML_LOG );

    if( conf == NULL )
    {
        printf( "error: no log configuration found in app config\n" );
        exit( 0 );
    }

    int rc = smx_log_init( (const char*)conf );

    if( rc ) {
        printf( "error: zlog init failed with conf: '%s'\n", conf );
        exit( 0 );
    }

    xmlFree( conf );

    smx_rts_t* rts = smx_malloc( sizeof( struct smx_rts_s ) );
    if( rts == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "cannot create RTS structure" );
        exit( 0 );
    }

    rts->conf = doc;
    rts->ch_cnt = 0;
    rts->net_cnt = 0;

    SMX_LOG_MAIN( main, notice, "start thread main" );

    return rts;
}

/*****************************************************************************/
void smx_program_init_run( smx_rts_t* rts )
{
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;
    int rn_id = rts->net_cnt;
    int profile_id = rn_id + 1;
    int new_ch_id = rts->ch_cnt;
    int i, ch_id;

    cur = xmlDocGetRootElement( rts->conf );
    conf = xmlGetProp( cur, ( const xmlChar* )XML_PROFILE );
    if( conf == NULL || xmlStrcmp(conf, ( const xmlChar* )"1" ) )
        return;

    SMX_LOG_MAIN( main, notice, "intitialising profiler" );

    for( i = 0; i < rn_id; i++ )
        SMX_CHANNEL_CREATE( i + new_ch_id, 1, SMX_FIFO, profile );

    SMX_NET_CREATE( rn_id, smx_profile_rn, smx_rn );
    SMX_NET_INIT( rn_id, smx_rn, rn_id, 1 );
    SMX_NET_RN_INIT( rn_id );

    for( i = 0; i < rn_id; i++ )
    {
        ch_id = i + new_ch_id;
        rts->nets[i]->profile = rts->chs[ch_id];
        SMX_CONNECT_ARR( rn_id, ch_id, smx_profile_rn, smx_rn, profile, in );
        SMX_CONNECT_RN( rn_id, ch_id );
    }

    new_ch_id = rts->ch_cnt;
    SMX_CHANNEL_CREATE( new_ch_id, 1, SMX_FIFO, net );
    SMX_NET_CREATE( profile_id, smx_profile, smx_mongo );
    SMX_NET_INIT( profile_id, smx_mongo, 1, 0 );
    SMX_CONNECT_ARR( rn_id, new_ch_id, smx_profile_rn, smx_rn, net, out );
    SMX_CONNECT_ARR( profile_id, new_ch_id, smx_profile, smx_mongo, net, in );
    SMX_CONNECT( profile_id, new_ch_id, smx_profile, smx_mongo, net, in );
}
