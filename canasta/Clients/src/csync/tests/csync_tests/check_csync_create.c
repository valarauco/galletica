#include <string.h>

#include "support.h"

#include "csync_private.h"

CSYNC *csync;

START_TEST (check_csync_destroy_null)
{
  fail_unless(csync_destroy(NULL) < 0, NULL);
}
END_TEST

START_TEST (check_csync_create)
{
  fail_unless(csync_create(&csync, "/tmp/csync1", "/tmp/csync2") == 0, NULL);

  fail_unless(csync->options.max_depth == MAX_DEPTH, NULL);
  fail_unless(csync->options.max_time_difference == MAX_TIME_DIFFERENCE, NULL);
  fail_unless(strcmp(csync->options.config_dir, CSYNC_CONF_DIR) > 0, NULL);

  fail_unless(csync_destroy(csync) == 0, NULL);
}
END_TEST

static Suite *make_csync_suite(void) {
  Suite *s = suite_create("csync");

  create_case(s, "check_csync_destroy_null", check_csync_destroy_null);
  create_case(s, "check_csync_create", check_csync_create);

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

