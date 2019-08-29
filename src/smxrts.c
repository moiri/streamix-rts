/**
 * The runtime system library for Streamix
 *
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "smxrts.h"

#define LIBZLOG_VERSION "1.2.14"
#define LIBPTHREAD_STUBS_VERSION "0.4-1"

/*****************************************************************************/
void smx_program_cleanup( smx_rts_t* rts )
{
    double elapsed_wall;
    bson_destroy( rts->conf );
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
    bson_json_reader_t *reader;
    bson_error_t error;
    bson_t* doc = bson_new();
    bson_iter_t iter;
    uint32_t len;
    const char* conf;

    int rc = smx_log_init();

    if( rc ) {
        fprintf( stderr, "error: failed to initialise zlog, aborting\n" );
        exit( 0 );
    }

    reader = bson_json_reader_new_from_file( config, &error );
    if( reader == NULL )
    {
        SMX_LOG_MAIN( main, error, "failed to open '%s': %s", config,
                error.message );
        exit( 0 );
    }

    rc = bson_json_reader_read( reader, doc, &error );
    if( rc < 0 )
    {
        SMX_LOG_MAIN( main, error,
                "could not parse the app config file '%s': %s",
                config, error.message );
        exit( 0 );
    }

    bson_json_reader_destroy( reader );
    if( bson_iter_init_find( &iter, doc, "_log" ) )
    {
        conf = bson_iter_utf8( &iter, &len );
        rc = zlog_reload( conf );
        if( rc ) {
            SMX_LOG_MAIN( main, error, "zlog reload failed with conf: '%s'\n",
                    conf );
            exit( 0 );
        }
        else
            SMX_LOG_MAIN( main, notice,
                    "switched to app specific zlog config file '%s'", conf );
    }
    else
        SMX_LOG_MAIN( main, warn,
                "no log configuration found in app config, missing mandatory ket '_log'" );

    if( !( bson_iter_init_find( &iter, doc, "_nets" )
            && BSON_ITER_HOLDS_DOCUMENT( &iter ) ) )
    {
        SMX_LOG_MAIN( main, error, "missing mandatory key '_nets' in app config" );
        exit( 0 );
    }

    if( bson_iter_init_find( &iter, doc, "_name" ) )
        SMX_LOG_MAIN( main, notice,
                "=============== Initializing app '%s' ===============",
                bson_iter_utf8( &iter, &len ) );
    else
    {
        SMX_LOG_MAIN( main, error, "missing mandatory key '_name' in app config" );
        exit( 0 );
    }

    smx_rts_t* rts = smx_malloc( sizeof( struct smx_rts_s ) );
    if( rts == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "cannot create RTS structure" );
        exit( 0 );
    }

    rts->ch_cnt = 0;
    rts->net_cnt = 0;
    rts->start_wall.tv_sec = 0;
    rts->start_wall.tv_nsec = 0;
    rts->end_wall.tv_sec = 0;
    rts->end_wall.tv_nsec = 0;
    rts->conf = doc;
    clock_gettime( CLOCK_MONOTONIC, &rts->start_wall );

    SMX_LOG_MAIN( main, info, "using libbson version: %s", bson_get_version() );
    SMX_LOG_MAIN( main, info, "using libzlog version: %s", LIBZLOG_VERSION );
    SMX_LOG_MAIN( main, info, "using libphtread stubs version: %s", LIBPTHREAD_STUBS_VERSION );

    SMX_LOG_MAIN( main, notice, "start thread main" );

    return rts;
}

/*****************************************************************************/
void smx_program_init_run( smx_rts_t* rts )
{
    pthread_barrier_init( &rts->init_done, NULL, rts->net_cnt );
}
