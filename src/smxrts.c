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
    int i;
    double elapsed_wall;
    for( i = 0; i < rts->shared_state_cnt; i++ )
    {
        rts->shared_state[i]->cleanup( rts->shared_state[i]->state );
        free( rts->shared_state[i] );
    }
    pthread_mutex_destroy( &rts->net_mutex );
    bson_destroy( rts->conf );
    if( rts->args != NULL )
    {
        bson_destroy( rts->args );
    }
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
smx_rts_t* smx_program_init( const char* app_conf, const char* log_conf,
        const char** app_conf_maps, int app_conf_map_count,
        const char* arg_file, const char* arg_str )
{
    int i, rc;
    bson_t tgt, payload, mapping;
    bson_iter_t i_map, i_maps;
    pthread_mutexattr_t mutexattr_prioinherit;
    smx_config_data_maps_t maps;
    char* name = NULL;

    rc = smx_log_init( log_conf );
    if( rc < 0 ) {
        fprintf( stderr, "error: failed to initialise zlog, aborting\n" );
        exit( 0 );
    }

    SMX_LOG_MAIN( main, notice, "using log configuration file '%s'", log_conf );

    bson_init( &tgt );
    rc = smx_program_init_conf( app_conf, &tgt, &name );
    if( rc < 0 )
    {
        goto error;
    }
    SMX_LOG_MAIN( main, notice,
            "=============== Initializing app '%s' ===============", name );

    for( i = 0; i < app_conf_map_count; i++ )
    {
        bson_init( &mapping );
        rc = smx_program_init_maps( app_conf_maps[i], &mapping, &i_maps,
                &payload );
        if( rc < 0 )
        {
            bson_destroy( &mapping );
            goto error;
        }
        if( bson_iter_recurse( &i_maps, &i_map ) )
        {
            rc = smx_config_data_maps_init( &i_map, &tgt, &maps );
            if( rc < 0 )
            {
                SMX_LOG_MAIN( main, fatal, "failed to init config map: %s",
                        smx_config_data_map_strerror( rc ) );
                goto error_map;
            }
            rc = smx_config_data_maps_apply( &maps, &payload );
            if( rc < 0 )
            {
                SMX_LOG_MAIN( main, fatal, "failed to apply config map: %s",
                        smx_config_data_map_strerror( rc ) );
                bson_destroy( &maps.mapped_payload );
                goto error_map;
            }
            bson_destroy( &tgt );
            bson_copy_to( &maps.mapped_payload, &tgt );
            bson_destroy( &maps.mapped_payload );
        }
        else
        {
            SMX_LOG_MAIN( main, fatal, "failed to iterate maps array" );
            goto error_map;
        }
        bson_destroy( &mapping );
        bson_destroy( &payload );
    }

    smx_rts_t* rts = smx_malloc( sizeof( struct smx_rts_s ) );
    if( rts == NULL )
    {
        SMX_LOG_MAIN( main, fatal, "cannot create RTS structure" );
        goto error;
    }

    rts->shared_state_cnt = 0;
    rts->ch_cnt = 0;
    rts->net_cnt = 0;
    rts->start_wall.tv_sec = 0;
    rts->start_wall.tv_nsec = 0;
    rts->end_wall.tv_sec = 0;
    rts->end_wall.tv_nsec = 0;
    rts->conf = bson_copy( &tgt );
    rts->args = NULL;

    rc = smx_program_init_args( arg_str, arg_file, name, rts );
    if( rc < 0 )
    {
        bson_destroy( rts->conf );
        free( rts );
        goto error;
    }

    pthread_mutexattr_init( &mutexattr_prioinherit );
    pthread_mutexattr_setprotocol( &mutexattr_prioinherit,
            PTHREAD_PRIO_INHERIT );
    pthread_mutex_init( &rts->net_mutex, &mutexattr_prioinherit );
    clock_gettime( CLOCK_MONOTONIC, &rts->start_wall );

    SMX_LOG_MAIN( main, notice, "using libsmxrts version: %s",
            LIBSMXRTS_VERSION );
    SMX_LOG_MAIN( main, notice, "using libbson version: %s",
            bson_get_version() );
    SMX_LOG_MAIN( main, notice, "using libzlog version: %s", LIBZLOG_VERSION );
    SMX_LOG_MAIN( main, notice, "using libpthread stubs version: %s",
            LIBPTHREAD_STUBS_VERSION );

    SMX_LOG_MAIN( main, notice, "start thread main" );

    bson_destroy( &tgt );
    if( name != NULL )
    {
        free( name );
    }

    return rts;

error_map:
    bson_destroy( &payload );
    bson_destroy( &mapping );
error:
    if( name != NULL )
    {
        free( name );
    }
    bson_destroy( &tgt );
    smx_log_cleanup();
    exit( 0 );
}

/*****************************************************************************/
int smx_program_init_bson_file( const char* path, bson_t* doc )
{
    bson_json_reader_t *reader;
    bson_error_t error;
    int rc;

    reader = bson_json_reader_new_from_file( path, &error );
    if( reader == NULL )
    {
        SMX_LOG_MAIN( main, error, "failed to open file '%s': %s", path,
                error.message );
        return -1;
    }

    rc = bson_json_reader_read( reader, doc, &error );
    if( rc < 0 )
    {
        SMX_LOG_MAIN( main, error, "could not parse the file '%s': %s", path,
                error.message );
        bson_json_reader_destroy( reader );
        return -1;
    }

    bson_json_reader_destroy( reader );

    return 0;
}

/*****************************************************************************/
int smx_program_init_args( const char* arg_str, const char* arg_file,
        const char* name, smx_rts_t* rts )
{
    int rc;
    bson_error_t error;
    const char* key;

    if( arg_str != NULL )
    {
        rts->args = bson_new_from_json( ( const uint8_t* )arg_str, -1, &error );
        if( rts->args == NULL )
        {
            SMX_LOG_MAIN( main, error, "failed to parse argument '%s': %s",
                    arg_str, error.message );
            return -1;
        }
    }
    else if( arg_file != NULL )
    {
        rts->args = bson_new();
        rc = smx_program_init_bson_file( arg_file, rts->args );
        if( rc < 0 )
        {
            bson_destroy( rts->args );
            rts->args = NULL;
            SMX_LOG_MAIN( main, error, "failed to parse argument file '%s'",
                    arg_file );
            return -1;
        }
    }

    if( rts->args == NULL )
    {
        rts->args = bson_new();
    }

    key = "experiment_id";
    if( !bson_has_field( rts->args, key ) )
    {
        BSON_APPEND_UTF8( rts->args, key, name );
        SMX_LOG_MAIN( main, warn, "no experiment ID defined in arguments"
                ", falling back to '%s'", name );
    }

    return 0;
}

/*****************************************************************************/
int smx_program_init_conf( const char* conf, bson_t* doc, char** name )
{
    int rc;
    bson_iter_t iter;

    rc = smx_program_init_bson_file( conf, doc );
    if( rc < 0 )
    {
        return -1;
    }

    if( !( bson_iter_init_find( &iter, doc, "_nets" )
            && BSON_ITER_HOLDS_DOCUMENT( &iter ) ) )
    {
        SMX_LOG_MAIN( main, error,
                "missing mandatory key '_nets' in app config" );
        return -1;
    }

    if( bson_iter_init_find( &iter, doc, "_name" ) )
    {
        *name = bson_iter_dup_utf8( &iter, NULL );
        SMX_LOG_MAIN( main, notice, "successfully parsed config file '%s'",
                conf );
    }
    else
    {
        SMX_LOG_MAIN( main, error,
                "missing mandatory key '_name' in app config" );
        return -1;
    }

    return 0;
}

/*****************************************************************************/
int smx_program_init_maps( const char* path, bson_t* doc, bson_iter_t* i_maps,
        bson_t* payload )
{
    int rc;
    bson_iter_t iter;
    const uint8_t* data;
    uint32_t len;

    rc = smx_program_init_bson_file( path, doc );
    if( rc < 0 )
    {
        goto error;
    }

    if( !bson_iter_init_find( i_maps, doc, "maps" )
            && BSON_ITER_HOLDS_ARRAY( &iter ) )
    {
        SMX_LOG_MAIN( main, error,
                "missing mandatory key 'maps' in app config map" );
        goto error;
    }

    if( bson_iter_init_find( &iter, doc, "payload" )
            && BSON_ITER_HOLDS_DOCUMENT( &iter ) )
    {
        bson_iter_document( &iter, &len, &data );
        if( !bson_init_static( payload, data, len ) )
        {
            SMX_LOG_MAIN( main, error,
                    "failed to init payload document: %s", strerror( errno ) );
            goto error;
        }
    }
    else
    {
        SMX_LOG_MAIN( main, error,
                "missing mandatory key 'payload' in app config map" );
        goto error;
    }

    SMX_LOG_MAIN( main, notice, "successfully parsed config map file '%s'",
            path );

    return 0;

error:
    return -1;
}

/*****************************************************************************/
void smx_program_init_run( smx_rts_t* rts )
{
    SMX_LOG_MAIN( main, notice, "waiting for all %d nets to finish"
            " initialisation", rts->net_cnt );
    if( pthread_barrier_init( &rts->pre_init_done, NULL, rts->net_cnt ) != 0 )
    {
        SMX_LOG_MAIN( main, error, "barrier pre initialisation failed" );
    }
    if( pthread_barrier_init( &rts->init_done, NULL, rts->net_cnt ) != 0 )
    {
        SMX_LOG_MAIN( main, error, "barrier initialisation failed" );
    }
}

/******************************************************************************/
const char* smx_rts_get_version()
{
    return LIBSMXRTS_VERSION;
}
