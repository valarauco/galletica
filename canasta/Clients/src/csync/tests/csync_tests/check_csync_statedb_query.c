/* Last Change: 2008-05-14 00:59:10 */
#define _GNU_SOURCE /* asprintf */
#include <string.h>

#include "support.h"

#define CSYNC_TEST 1
#include "csync_statedb.c"

CSYNC *csync;
const char *testdb = (char *) "/tmp/check_csync1/test.db";
const char *testtmpdb = (char *) "/tmp/check_csync1/test.db.ctmp";

static void setup(void) {
  fail_if(system("rm -rf /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("rm -rf /tmp/check_csync2") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync2") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync") < 0, "Setup failed");
  fail_if(csync_create(&csync, "/tmp/check_csync1", "/tmp/check_csync2") < 0, "Setup failed");
  csync_set_config_dir(csync, "/tmp/check_csync/");
  fail_if(csync_init(csync) < 0, NULL, "Setup failed");
}

static void setup_db(void) {
  char *stmt = NULL;

  fail_if(system("rm -rf /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("rm -rf /tmp/check_csync2") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync2") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync") < 0, "Setup failed");
  fail_if(csync_create(&csync, "/tmp/check_csync1", "/tmp/check_csync2") < 0, "Setup failed");
  csync_set_config_dir(csync, "/tmp/check_csync/");
  fail_if(csync_init(csync) < 0, NULL, "Setup failed");
  fail_unless(csync_statedb_create_tables(csync) == 0, "Setup failed");

  stmt = sqlite3_mprintf("INSERT INTO metadata"
    "(phash, pathlen, path, inode, uid, gid, mode, modtime) VALUES"
    "(%lu, %d, '%q', %d, %d, %d, %d, %lu);",
    42,
    42,
    "It's a rainy day",
    23,
    42,
    42,
    42,
    42);

  fail_if(csync_statedb_insert(csync, stmt) < 0, NULL);
  sqlite3_free(stmt);
}

static void teardown(void) {
  fail_if(csync_destroy(csync) < 0, "Teardown failed");
  fail_if(system("rm -rf /tmp/check_csync") < 0, "Teardown failed");
  fail_if(system("rm -rf /tmp/check_csync1") < 0, "Teardown failed");
  fail_if(system("rm -rf /tmp/check_csync2") < 0, "Teardown failed");
}


START_TEST (check_csync_statedb_query_statement)
{
  c_strlist_t *result = NULL;
  result = csync_statedb_query(csync, "");
  fail_unless(result == NULL, NULL);
  c_strlist_destroy(result);

  result = csync_statedb_query(csync, "SELECT;");
  fail_unless(result == NULL, NULL);
  c_strlist_destroy(result);
}
END_TEST

START_TEST (check_csync_statedb_create_error)
{
  c_strlist_t *result = NULL;
  result = csync_statedb_query(csync, "CREATE TABLE test(phash INTEGER, text VARCHAR(10));");
  fail_if(result == NULL, NULL);
  c_strlist_destroy(result);

  result = csync_statedb_query(csync, "CREATE TABLE test(phash INTEGER, text VARCHAR(10));");
  fail_unless(result == NULL, NULL);
  c_strlist_destroy(result);
}
END_TEST

START_TEST (check_csync_statedb_insert_statement)
{
  c_strlist_t *result = NULL;
  result = csync_statedb_query(csync, "CREATE TABLE test(phash INTEGER, text VARCHAR(10));");
  fail_if(result == NULL, NULL);
  c_strlist_destroy(result);
  fail_unless(csync_statedb_insert(csync, "INSERT;") == 0, NULL);
  fail_unless(csync_statedb_insert(csync, "INSERT") == 0, NULL);
  fail_unless(csync_statedb_insert(csync, "") == 0, NULL);
}
END_TEST

START_TEST (check_csync_statedb_query_create_and_insert_table)
{
  c_strlist_t *result = NULL;
  result = csync_statedb_query(csync, "CREATE TABLE test(phash INTEGER, text VARCHAR(10));");
  c_strlist_destroy(result);
  fail_unless(csync_statedb_insert(csync, "INSERT INTO test (phash, text) VALUES (42, 'hello');"), NULL);
  result = csync_statedb_query(csync, "SELECT * FROM test;");
  fail_unless(result->count == 2, NULL);
  fail_unless(strcmp(result->vector[0], "42") == 0, NULL);
  fail_unless(strcmp(result->vector[1], "hello") == 0, NULL);
  c_strlist_destroy(result);
}
END_TEST

START_TEST (check_csync_statedb_is_empty)
{
  c_strlist_t *result = NULL;

  /* we have an empty db */
  fail_unless(_csync_statedb_is_empty(csync) == 1, NULL);

  /* add a table and an entry */
  result = csync_statedb_query(csync, "CREATE TABLE metadata(phash INTEGER, text VARCHAR(10));");
  c_strlist_destroy(result);
  fail_unless(csync_statedb_insert(csync, "INSERT INTO metadata (phash, text) VALUES (42, 'hello');"), NULL);

  fail_unless(_csync_statedb_is_empty(csync) == 0, NULL);
}
END_TEST

START_TEST (check_csync_statedb_create_tables)
{
  char *stmt = NULL;

  fail_unless(csync_statedb_create_tables(csync) == 0, NULL);

  stmt = sqlite3_mprintf("INSERT INTO metadata_temp"
    "(phash, pathlen, path, inode, uid, gid, mode, modtime) VALUES"
    "(%lu, %d, '%q', %d, %d, %d, %d, %lu);",
    42,
    42,
    "It's a rainy day",
    42,
    42,
    42,
    42,
    42);

  fail_if(csync_statedb_insert(csync, stmt) < 0, NULL);
  sqlite3_free(stmt);
}
END_TEST

START_TEST (check_csync_statedb_drop_tables)
{
  fail_unless(csync_statedb_drop_tables(csync) == 0, NULL);
  fail_unless(csync_statedb_create_tables(csync) == 0, NULL);
  fail_unless(csync_statedb_drop_tables(csync) == 0, NULL);
}
END_TEST

START_TEST (check_csync_statedb_insert_metadata)
{
  int i = 0;
  csync_file_stat_t *st;

  fail_unless(csync_statedb_create_tables(csync) == 0, NULL);

  for(i = 0; i < 100; i++) {
    st = c_malloc(sizeof(csync_file_stat_t));
    st->phash = i;

    fail_unless(c_rbtree_insert(csync->local.tree, (void *) st) == 0, NULL);
  }

  fail_unless(csync_statedb_insert_metadata(csync) == 0, NULL);
}
END_TEST

START_TEST (check_csync_statedb_write)
{
  int i = 0;
  csync_file_stat_t *st;

  for(i = 0; i < 100; i++) {
    st = c_malloc(sizeof(csync_file_stat_t));
    st->phash = i;

    fail_unless(c_rbtree_insert(csync->local.tree, (void *) st) == 0, NULL);
  }

  fail_unless(csync_statedb_write(csync) == 0, NULL);
}
END_TEST

START_TEST (check_csync_statedb_get_stat_by_hash)
{
  csync_file_stat_t *tmp = NULL;

  tmp = csync_statedb_get_stat_by_hash(csync, (uint64_t) 42);
  fail_if(tmp == NULL, NULL);

  fail_unless(tmp->phash == 42, NULL);
  fail_unless(tmp->inode == 23, NULL);

  SAFE_FREE(tmp);
}
END_TEST

START_TEST (check_csync_statedb_get_stat_by_hash_not_found)
{
  csync_file_stat_t *tmp = NULL;

  tmp = csync_statedb_get_stat_by_hash(csync, (uint64_t) 666);
  fail_unless(tmp == NULL, NULL);
}
END_TEST

START_TEST (check_csync_statedb_get_stat_by_inode)
{
  csync_file_stat_t *tmp = NULL;

  tmp = csync_statedb_get_stat_by_inode(csync, (ino_t) 23);
  fail_if(tmp == NULL, NULL);

  fail_unless(tmp->phash == 42, NULL);
  fail_unless(tmp->inode == 23, NULL);

  SAFE_FREE(tmp);
}
END_TEST

START_TEST (check_csync_statedb_get_stat_by_inode_not_found)
{
  csync_file_stat_t *tmp = NULL;

  tmp = csync_statedb_get_stat_by_inode(csync, (ino_t) 666);
  fail_unless(tmp == NULL, NULL);
}
END_TEST

static Suite *make_csync_suite(void) {
  Suite *s = suite_create("csync_statedb");

  create_case_fixture(s, "check_csync_statedb_query_statement", check_csync_statedb_query_statement, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_create_error", check_csync_statedb_create_error, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_insert_statement", check_csync_statedb_insert_statement, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_query_create_and_insert_table", check_csync_statedb_query_create_and_insert_table, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_is_empty", check_csync_statedb_is_empty, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_create_tables", check_csync_statedb_create_tables, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_drop_tables", check_csync_statedb_drop_tables, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_insert_metadata", check_csync_statedb_insert_metadata, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_write", check_csync_statedb_write, setup, teardown);
  create_case_fixture(s, "check_csync_statedb_get_stat_by_hash", check_csync_statedb_get_stat_by_hash, setup_db, teardown);
  create_case_fixture(s, "check_csync_statedb_get_stat_by_hash_not_found", check_csync_statedb_get_stat_by_hash_not_found, setup_db, teardown);
  create_case_fixture(s, "check_csync_statedb_get_stat_by_inode", check_csync_statedb_get_stat_by_inode, setup_db, teardown);
  create_case_fixture(s, "check_csync_statedb_get_stat_by_inode_not_found", check_csync_statedb_get_stat_by_inode_not_found, setup_db, teardown);

  return s;
}

int main(int argc, char **argv) {
  Suite *s = NULL;
  SRunner *sr = NULL;
  struct argument_s arguments;
  int nf;

  ZERO_STRUCT(arguments);

  cmdline_parse(argc, argv, &arguments);

  s = make_csync_suite();

  sr = srunner_create(s);
  if (arguments.nofork) {
    srunner_set_fork_status(sr, CK_NOFORK);
  }
  srunner_run_all(sr, CK_VERBOSE);
  nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

