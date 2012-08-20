/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <iniparser.h>
#include <std/c_path.h>

#include "../../modules/csync_owncloud.c"
#include <cmocka.h>

#include "config_test.h"

struct oc_credentials {
    const char *url;
    const char *user;
    const char *pwd;
    char *oc_server;

} _credentials;


static bool load_oc_config( const char *config ) {
    dictionary *dict;
    const char* val;
    bool re = true;
    char *deflt;
    char b;
    deflt = &b;


    dict = iniparser_load( config );

    if( ! dict ) {
        printf("Could not load config %s\n", config);
        return false;
    }

    val = iniparser_getstring(dict, "global:host", deflt);
    if( val ) {
        _credentials.url = c_strdup( val );
    } else {
        re = false;
    }

    val = iniparser_getstring(dict, "global:user", deflt);
    if( re && val ) {
        _credentials.user = c_strdup( val );
    } else {
        re = false;
    }

    val = iniparser_getstring(dict, "global:pwd", deflt);
    if( re && val ) {
        _credentials.pwd = c_strdup( val );
    } else {
        re = false;
    }

    if( re ) {
        asprintf(&(_credentials.oc_server),
                 "owncloud://%s:%s@%s/files/webdav.php",
                 _credentials.user, _credentials.pwd,
                 _credentials.url );
    }

    return re;
}


// A test case that does nothing and succeeds.
static void null_test_success(void **state) {
    (void) state;
}


static void connect_test_success(void **state) {

    int re = 0;
    char buf[255];

    (void) state;
    strcpy(buf, TEST_CONFIG_DIR);
    strcat(buf, "test.cfg");

    assert_true( load_oc_config( buf ));

    re = dav_connect( _credentials.oc_server );

    assert_int_equal( re, 0 );
    assert_int_equal( _connected, 1 );
    assert_int_equal( dav_session.time_delta_sum, 0);
    assert_int_equal( dav_session.time_delta_cnt, 0);
}

static void fetch_a_context(void **state) {
    struct listdir_context  *fetchCtx = NULL;
    char *curi = _cleanPath(_credentials.oc_server);
    int rc = 0;
    unsigned int i;

    (void) state;

    fetchCtx = c_malloc( sizeof( struct listdir_context ));
    fetchCtx->target = curi;
    fetchCtx->include_target = 1;
    
    rc = fetch_resource_list( curi, NE_DEPTH_ONE, fetchCtx );
    assert_int_equal( rc, 0 );
    printf("Results: %d\n", fetchCtx->result_count);
    
    fetchCtx->currResource = fetchCtx->list;
    for( i = 0; i < fetchCtx->result_count; i++ ) {
	assert_true( fetchCtx->currResource != NULL );
	assert_true( fetchCtx->currResource->uri != NULL );
	assert_true( fetchCtx->currResource->name != NULL );
	
	printf( "   %s -> %s\n", fetchCtx->currResource->uri, fetchCtx->currResource->name );
	fetchCtx->currResource = fetchCtx->currResource->next;
    } 
}

static int test_mkdir(const char *dir) {
    char path[255];

    strcpy( path, _credentials.oc_server );
    strcat( path, "/");
    strcat( path, dir );

    return owncloud_mkdir( path, 775 );
}

static void setup_toplevel_dir( void **state ) {
    char basepath[255];

    strcpy( basepath, "tXXXXXX");
    assert_int_equal( c_tmpname(basepath), 0 );
    printf("Using top testing dir %s\n", basepath);
    assert_int_equal( test_mkdir( basepath ), 0 );
    *state = (void*) c_strdup(basepath);
}

static void teardown_toplevel_dir( void **state ) {
    (void) state;
}

static void stat_local_file( csync_stat_t *sb, const char *file )
{
    const _TCHAR *mpath = NULL;
    mpath = c_multibyte(file);
    assert_int_not_equal(_tstat(mpath, sb), -1);
    c_free_multibyte(mpath);
}

#define BUFSIZE 4096
static size_t upload_a_file( void **state, const char *src_name, const char *durl ) {

    char buffer[BUFSIZE+1];
    int size;
    char path[256];
    char src_path[256];
    int fh;
    csync_vio_method_handle_t *handle;
    size_t written;
    size_t overall_size = 0;
    csync_stat_t sb;

    /* Create the target path */
    strcpy( path, _credentials.oc_server );
    strcat( path, "/");
    strcat( path, (const char*) *state );
    strcat( path, "/");
    strcat( path, durl );

    handle = owncloud_creat( path, 0644 );
    assert_int_not_equal( handle, NULL );

    strcpy(src_path, TESTFILES_DIR);
    strcat(src_path, src_name);
    fh = open(src_path, O_RDONLY);
    assert_int_not_equal( fh, -1 );

    while( (size = read(fh, buffer, BUFSIZE) )>0 ) {
        buffer[size] = '\0';
        written = owncloud_write( handle, buffer, size );
        assert_int_equal( size, written );
        overall_size += written;
    }
    assert_int_equal( owncloud_close(handle), 0);

    /* stat the local file */
    stat_local_file( &sb, src_path );

    assert_int_equal( overall_size, sb.st_size );

    close(fh);

    return overall_size;
}

static void test_upload_files( void **state ) {
    const char *bpath = (char*) (*state);

    upload_a_file( state, "test.txt", "t1/test.txt");
    upload_a_file( state, "red_is_the_rose.jpg", "t1/red is the rose.jpg");

    printf("Base path: %s\n", bpath);

}

static void download_a_file( const char* local, void **state, const char *durl)
{
    char buffer[BUFSIZE+1];
    char path[256];
    char src_path[256];
    int  did;
    _TCHAR tlocal[256];

    csync_vio_method_handle_t *handle;
    ssize_t count;
    ssize_t overall_size = 0;
    csync_stat_t sb;

    /* Create the target path */
    strcpy( path, _credentials.oc_server );
    strcat( path, "/");
    strcat( path, (const char*) *state );
    strcat( path, "/");
    strcat( path, durl );

    strcpy( tlocal, "/tmp/");
    strcat( tlocal, local );
    did = _topen(tlocal, O_RDWR|O_CREAT, 0644);
    assert_true( did > -1 );

    handle = owncloud_open( path, O_RDONLY, 0644 );
    assert_int_not_equal( handle, NULL );

    while( (count = owncloud_read(handle, buffer, BUFSIZE)) > 0 ) {
        write( did, buffer, count );
        overall_size += count;
    }
    assert_int_equal( owncloud_close(handle), 0 );
    close(did);

    strcpy(src_path, TESTFILES_DIR);
    strcat(src_path, local);
    stat_local_file( &sb, src_path );

    /* assert the download size, it has to be the same. */
    assert_true( overall_size == sb.st_size );

}

static void test_download_files( void **state ) {
    const char *bpath = (char*) (*state);

    printf("Base path: %s\n", bpath);

    download_a_file( "test.txt", state, "t1/test.txt");
    download_a_file( "red_is_the_rose.jpg", state, "t1/red is the rose.jpg");
}

static void test_setup_dirs(void **state) {
    const char *basepath = *state;
    char path[255];

    strcpy( path, basepath );
    strcat( path, "/t1" );
    assert_int_equal( test_mkdir( path ), 0 );

    strcpy( path, basepath );
    strcat( path, "/t2");
    assert_int_equal( test_mkdir( path ), 0 );
    strcat( path, "/Übergröße");
    assert_int_equal( test_mkdir( path ), 0 );
}


int main(void) {
    const UnitTest tests[] = {
        unit_test(null_test_success),
        unit_test(connect_test_success),
        unit_test(fetch_a_context),
        unit_test_setup_teardown(test_setup_dirs, setup_toplevel_dir, teardown_toplevel_dir),
        unit_test(test_upload_files),
        unit_test(test_download_files),
    };

    return run_tests(tests);
}
