/**
 * @author  Simon Maurer
 * @license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * The runtime system library for Streamix
 */

#include "smxrts.h"

#define LIBZLOG_VERSION "1.2.14"
#define LIBPTHREAD_STUBS_VERSION "0.4-1"
#ifndef LIBSMXRTS_VERSION
#define LIBSMXRTS_VERSION "undefined"
#endif

/*****************************************************************************/
void smx_program_cleanup( smx_rts_t* rts )
{
    double elapsed_wall;
    bson_destroy( rts->conf );
    pthread_barrier_destroy( &rts->init_done );
    clock_gettime( CLOCK_MONOTONIC, &rts->end_wall );
    elapsed_wall = ( rts->end_wall.tv_sec - rts->start_wall.tv_sec );
    elapsed_wall += ( rts->end_wall.tv_nsec - rts->start_wall.tv_nsec) / 1000000000.0;
    free( rts );
    SMX_LOG_MAIN( main, notice, "end main thread (wall time: %f)", elapsed_wall );
    smx_log_cleanup();
    exit( EXIT_SUCCESS );
}

/*****************************************************************************/
smx_rts_t* smx_program_init( const char* app_conf, const char* log_conf )
{
    bson_json_reader_t *reader;
    bson_error_t error;
    bson_t* doc = bson_new();
    bson_iter_t iter;
    uint32_t len;

    int rc = smx_log_init( log_conf );

    if( rc ) {
        fprintf( stderr, "error: failed to initialise zlog, aborting\n" );
        exit( 0 );
    }

    SMX_LOG_MAIN( main, notice, "using log configuration file '%s'", log_conf );

    reader = bson_json_reader_new_from_file( app_conf, &error );
    if( reader == NULL )
    {
        SMX_LOG_MAIN( main, error, "failed to open '%s': %s", app_conf,
                error.message );
        exit( 0 );
    }

    rc = bson_json_reader_read( reader, doc, &error );
    if( rc < 0 )
    {
        SMX_LOG_MAIN( main, error,
                "could not parse the app app_conf file '%s': %s",
                app_conf, error.message );
        exit( 0 );
    }

    bson_json_reader_destroy( reader );

    if( !( bson_iter_init_find( &iter, doc, "_nets" )
            && BSON_ITER_HOLDS_DOCUMENT( &iter ) ) )
    {
        SMX_LOG_MAIN( main, error, "missing mandatory key '_nets' in app config" );
        exit( 0 );
    }

    if( bson_iter_init_find( &iter, doc, "_name" ) )
    {
        SMX_LOG_MAIN( main, notice,
                "=============== Initializing app '%s' ===============",
                bson_iter_utf8( &iter, &len ) );
        SMX_LOG_MAIN( main, notice, "successfully parsed config file '%s'",
                app_conf );
    }
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

    SMX_LOG_MAIN( main, notice, "using libsmxrtl version: %s",
            LIBSMXRTS_VERSION );
    SMX_LOG_MAIN( main, notice, "using libbson version: %s",
            bson_get_version() );
    SMX_LOG_MAIN( main, notice, "using libzlog version: %s", LIBZLOG_VERSION );
    SMX_LOG_MAIN( main, notice, "using libpthread stubs version: %s",
            LIBPTHREAD_STUBS_VERSION );

    SMX_LOG_MAIN( main, notice, "start thread main" );

    return rts;
}

/*****************************************************************************/
void smx_program_init_run( smx_rts_t* rts )
{
    SMX_LOG_MAIN( main, notice, "waiting for all nets to finish"
            " initialisation" );
    pthread_barrier_init( &rts->init_done, NULL, rts->net_cnt );
    SMX_LOG_MAIN( main, notice, " initialisation on all nets done" );
}

/******************************************************************************/
const char* smx_rts_get_version()
{
    return LIBSMXRTS_VERSION;
}
