/**
 * The runtime system library for Streamix
 *
 * @file    smxrts.c
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "smxrts.h"
#include "smxlog.h"

#define XML_APP         "app"
#define XML_LOG         "log"

/*****************************************************************************/
void smx_program_cleanup( smx_rts_t* rts )
{
    double elapsed_wall;
    xmlFreeDoc( rts->conf );
    xmlCleanupParser();
    clock_gettime( CLOCK_MONOTONIC, &rts->end_wall );
    elapsed_wall = ( rts->end_wall.tv_sec - rts->start_wall.tv_sec );
    elapsed_wall += ( rts->end_wall.tv_nsec - rts->start_wall.tv_nsec) / 1000000000.0;
    free( rts );
    SMX_LOG_MAIN( main, notice, "end main thread (wall time: %f)", elapsed_wall );
    smx_log_cleanup();
    exit( EXIT_SUCCESS );
}

/*****************************************************************************/
smx_rts_t* smx_program_init( const char* config )
{
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    xmlChar* conf = NULL;


    int rc = smx_log_init();

    if( rc ) {
        fprintf( stderr, "error: failed to initialise zlog, aborting\n" );
        exit( 0 );
    }

    /* required for thread safety */
    xmlInitParser();

    /*parse the file and get the DOM */
    doc = xmlParseFile( config );

    if( doc == NULL )
    {
        SMX_LOG_MAIN( main, error, "could not parse the app config file '%s'",
                config );
        exit( 0 );
    }

    cur = xmlDocGetRootElement( doc );
    if( cur == NULL || xmlStrcmp(cur->name, ( const xmlChar* )XML_APP ) )
    {
        SMX_LOG_MAIN( main, error,
                "app config root node name is '%s' instead of '%s'",
                cur->name, XML_APP );
        exit( 0 );
    }
    conf = xmlGetProp( cur, ( const xmlChar* )XML_LOG );

    if( conf == NULL )
        SMX_LOG_MAIN( main, warn, "no log configuration found in app config" );
    else
    {
        SMX_LOG_MAIN( main, notice,
                "switching to app specific zlog config file '%s'", conf );
        rc = zlog_reload( (const char*)conf );
        if( rc ) {
            SMX_LOG_MAIN( main, error, "zlog reload failed with conf: '%s'\n",
                    conf );
            exit( 0 );
        }
    }

    SMX_LOG_MAIN( main, notice, "using libxml2 version: %s",
            LIBXML_DOTTED_VERSION );

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
    rts->start_wall.tv_sec = 0;
    rts->start_wall.tv_nsec = 0;
    rts->end_wall.tv_sec = 0;
    rts->end_wall.tv_nsec = 0;
    clock_gettime( CLOCK_MONOTONIC, &rts->start_wall );

    SMX_LOG_MAIN( main, notice, "start thread main" );

    return rts;
}

/*****************************************************************************/
void smx_program_init_run( smx_rts_t* rts )
{
    pthread_barrier_init( &rts->init_done, NULL, rts->net_cnt );
}
