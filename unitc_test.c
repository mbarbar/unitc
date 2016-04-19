/** unitc_test.c
  * Unit tests for the unitc library.
  */

#define _XOPEN_SOURCE 500

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unitc.h>

#include "unitc_dev.h"

/** Location of (manually created) files containing expected output. */
#define TEST_DIR "test_resources/"
#define TMP_FILE_TEMPLATE TEST_DIR "XXXXXX"

/** Set up for redirecting stdout to path (opens path to fd). */
#define STDOUT_REDIR_SET_UP(path, fd)\
        do {\
                if ((fd = open(path, O_WRONLY | O_TRUNC)) == -1) {\
                        fputs("Failed to open temporary file.", stderr);\
                };\
                fflush(stdout);\
                if (dup2(fd, STDOUT_FILENO) == -1) {\
                        close(fd);\
                        fputs("dup2 failed.", stderr);\
                }\
        } while (0)

/** Closes fd and redirects stdout to orig_stdout. */
#define STDOUT_REDIR_TEAR_DOWN(fd, orig_stdout)\
        do {\
                if (close(fd) == -1) {\
                        fputs("Failed to close temporary file.", stderr);\
                };\
                fflush(stdout);\
                if (dup2(orig_stdout, STDOUT_FILENO) == -1) {\
                        fputs("dup2 failed (to revert stdout).", stderr);\
                }\
        } while (0)

/** Check files at path_a and path_b are equal.
  *
  * @param path_a,path_b Paths to files to compare.
  *
  * @return true if files at path_a and path_b contain the same contents, false
  *         otherwise. Returns false if the files at the specified paths cannot
  *         be opened.
  */
static bool files_eq(char *path_a, char *path_b);

static bool test_uc_init(void);

static void test_files_eq(uc_suite);
static void test_uc_all_tests_passed(uc_suite);
static void test_uc_report_basic(uc_suite);
static void test_uc_report_basic_with_tests(uc_suite);
static void test_uc_report_standard(uc_suite);
static void test_uc_report_standard_with_tests(uc_suite);
static void test_isolation(uc_suite);
static void test_before_hook(uc_suite suite);

int main(void) {
        uc_suite main_suite;

        if (!test_uc_init()) {
                fputs("uc_init is failing. Aborting.", stderr);
                return EXIT_FAILURE;
        }
        /* From here, uc_init (dev) is assumed to be working. */

        main_suite = uc_init(UC_OPT_NONE, "unitc tests", NULL);
        uc_add_test(main_suite, &test_files_eq, "files_eq tests",
                    "This is an internal testing function.");
        uc_add_test(main_suite, &test_uc_all_tests_passed,
                    "uc_all_tests_passed tests", NULL);
        uc_add_test(main_suite, &test_uc_report_basic, "uc_report_basic tests",
                    "For suites with dangling checks only.");
        uc_add_test(main_suite, &test_uc_report_basic_with_tests,
                    "uc_report_basic tests",
                    "For suites with tests.");
        uc_add_test(main_suite, &test_uc_report_standard,
                    "uc_report_standard tests",
                    "For suites with dangling checks only.");
        uc_add_test(main_suite, &test_uc_report_standard_with_tests,
                    "uc_report_standard tests",
                    "For suites with tests.");
        uc_add_test(main_suite, &test_isolation,
                    "Isolation tests",
                    "By using the same static int in separate tests.");
        uc_add_test(main_suite, &test_before_hook,
                    "BEFORE hook tests",
                    "Shows that the before hooks for EACH test and IN that "
                    "test's address space.");
        uc_run_tests(main_suite);

        uc_report_standard(main_suite);
        uc_free(main_suite);

        return EXIT_SUCCESS;
}

static bool files_eq(char *path_a, char *path_b) {
        FILE *a_file, *b_file;
        int a_char, b_char;

        a_file = fopen(path_a, "rb");
        if (a_file == NULL) return false;

        b_file = fopen(path_b, "rb");
        if (b_file == NULL) {
                fclose(a_file);
                return false;
        }

        a_char = fgetc(a_file);
        b_char = fgetc(b_file);
        while (a_char != EOF && b_char != EOF) {
                if (a_char != b_char) break;

                a_char = fgetc(a_file);
                b_char = fgetc(b_file);
        }

        fclose(a_file);
        fclose(b_file);

        return a_char == b_char;
}

static void test_files_eq(uc_suite suite) {
        uc_check(suite, files_eq(TEST_DIR "files_eq_equal_a",
                                 TEST_DIR "files_eq_equal_b"),
                 "Check unique equal files are equal");

        uc_check(suite, !files_eq(TEST_DIR "files_eq_diff_a",
                                  TEST_DIR "files_eq_diff_b"),
                 "Check unique unequal files are not equal");

        uc_check(suite, files_eq(TEST_DIR "files_eq_empty_a",
                                 TEST_DIR "files_eq_empty_b"),
                 "Check unique empty files are equal");

        uc_check(suite, files_eq(TEST_DIR "files_eq_diff_a",
                                 TEST_DIR "files_eq_diff_a"),
                 "Check file is equal to itself");
}

static bool test_uc_init(void) {
        dev_uc_suite suite_a, suite_b;

        suite_a = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        if (suite_a == NULL) return false;
        dev_uc_free(suite_a);

        suite_a = NULL;
        suite_a = dev_uc_init(dev_UC_OPT_NONE, "Main", "Test suite.");
        if (suite_a == NULL) return false;
        dev_uc_free(suite_a);

        suite_b = dev_uc_init(dev_UC_OPT_NONE, "Name", NULL);
        if (suite_b == NULL) return false;
        dev_uc_free(suite_b);

        return true;
}

static void succ_test(uc_suite suite) {
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, NULL);
}

static void unsucc_test(dev_uc_suite suite) {
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, false, NULL);
        dev_uc_check(suite, true, NULL);
}

static void test_uc_all_tests_passed(uc_suite suite) {
        dev_uc_suite sut_suite;

        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Suite name", NULL);

        uc_check(suite, dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: no tests or checks.");

        dev_uc_check(sut_suite, true, NULL);

        uc_check(suite, dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Successful dangling check only.");

        dev_uc_check(sut_suite, false, NULL);

        uc_check(suite, !dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Un/successful dangling checks.");

        dev_uc_check(sut_suite, false, NULL);
        dev_uc_check(sut_suite, true, NULL);

        uc_check(suite, !dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: >1 Un/successful dangling checks.");

        dev_uc_add_test(sut_suite, &succ_test, NULL, NULL);
        dev_uc_run_tests(sut_suite);

        uc_check(suite, !dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Dangling checks and successful test.");

        dev_uc_free(sut_suite);

        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_add_test(sut_suite, &succ_test, "A", "a");
        dev_uc_add_test(sut_suite, &unsucc_test, "B", "b");
        dev_uc_run_tests(sut_suite);

        uc_check(suite, !dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Un/successful tests.");

        dev_uc_check(sut_suite, true, NULL);

        uc_check(suite, !dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Un/successful tests + dangling check.");

        dev_uc_free(sut_suite);

        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_add_test(sut_suite, &succ_test, "X", "a");
        dev_uc_add_test(sut_suite, &succ_test, "Y", "b");
        dev_uc_add_test(sut_suite, &succ_test, "Z", "c");
        dev_uc_run_tests(sut_suite);

        uc_check(suite, dev_uc_all_tests_passed(sut_suite),
                 "uc_all_tests_passed: Successful tests only.");

        dev_uc_free(sut_suite);
}

static void test_uc_report_basic(uc_suite suite) {
        dev_uc_suite sut_suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) {
                fputs("Failed to create temporary file.", stderr);
        }

        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                fputs("Failed to close temporary file.", stderr);
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Suite 1",
                                "Suite 1's comment.");
        dev_uc_check(sut_suite, true, "True.");
        dev_uc_check(sut_suite, false, "False.");
        dev_uc_report_basic(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite, files_eq(tmp_file_path, TEST_DIR "uc_report_basic_a"),
                 "Check basic report a.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Second suite", NULL);
        dev_uc_check(sut_suite, true, "True!");
        dev_uc_report_basic(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite, files_eq(tmp_file_path, TEST_DIR "uc_report_basic_b"),
                 "Check basic report b.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        for (int i = 0; i < 5; ++i) dev_uc_check(sut_suite, false, "False!");
        dev_uc_report_basic(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite, files_eq(tmp_file_path, TEST_DIR "uc_report_basic_c"),
                 "Check basic report c.");

        if (remove(tmp_file_path) == -1) {
                fputs("Could not remove temporary file", stderr);
        }

        if (close(orig_stdout) == -1) {
                fputs("Could not close dup'd stdout fd", stderr);
        }
}

void basic_d_test_1(dev_uc_suite suite) {
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, "...");
        dev_uc_check(suite, false, "---");
}

void basic_d_test_2(dev_uc_suite suite) {
        dev_uc_check(suite, false, NULL);
}

void basic_e_test_1(dev_uc_suite suite) {
        dev_uc_check(suite, true, NULL);
        dev_uc_check(suite, true, "True.");
}

void basic_e_test_2(dev_uc_suite suite) {
        dev_uc_check(suite, false, NULL);
}

static void test_uc_report_basic_with_tests(uc_suite suite) {
        dev_uc_suite sut_suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) {
                fputs("Failed to create temporary file.", stderr);
        }

        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                fputs("Failed to close temporary file.", stderr);
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "A suite", NULL);
        dev_uc_add_test(sut_suite, &basic_d_test_1, NULL, NULL);
        dev_uc_add_test(sut_suite, &basic_d_test_2, "Test name", "A comment");
        dev_uc_run_tests(sut_suite);
        dev_uc_report_basic(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite, files_eq(tmp_file_path, TEST_DIR "uc_report_basic_d"),
                 "Check basic report d.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL,
                                "Comment about the suite.");
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_check(sut_suite, false, NULL);
        dev_uc_add_test(sut_suite, basic_e_test_1, "1st test", NULL);
        dev_uc_add_test(sut_suite, basic_e_test_2, "2nd test", "A comment...");
        dev_uc_run_tests(sut_suite);
        dev_uc_check(sut_suite, false, NULL);
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_report_basic(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite, files_eq(tmp_file_path, TEST_DIR "uc_report_basic_e"),
                 "Check basic report e.");

        if (remove(tmp_file_path) == -1) {
                fputs("Could not remove temporary file", stderr);
        }

        if (close(orig_stdout) == -1) {
                fputs("Could not close dup'd stdout fd", stderr);
        }
}

static void test_uc_report_standard(uc_suite suite) {
        dev_uc_suite sut_suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) {
                fputs("Failed to create temporary file.", stderr);
        }

        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                fputs("Failed to close temporary file.", stderr);
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_check(sut_suite, false, "False.");
        dev_uc_report_standard(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite,
                 files_eq(tmp_file_path, TEST_DIR "uc_report_standard_a"),
                 "Check standard report a.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Suite b", "This is suite b");
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_check(sut_suite, true, "");
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_report_standard(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite,
                 files_eq(tmp_file_path, TEST_DIR "uc_report_standard_b"),
                 "Check standard report b.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Suite c", NULL);
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_check(sut_suite, false, "1st failure");
        dev_uc_check(sut_suite, false, "2nd failure");
        dev_uc_check(sut_suite, false, "3rd failure");
        dev_uc_check(sut_suite, true, "True.");
        dev_uc_check(sut_suite, false, NULL);
        dev_uc_report_standard(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite,
                 files_eq(tmp_file_path, TEST_DIR "uc_report_standard_c"),
                 "Check standard report c.");

        if (remove(tmp_file_path) == -1) {
                fputs("Could not remove temporary file", stderr);
        }

        if (close(orig_stdout) == -1) {
                fputs("Could not close dup'd stdout fd", stderr);
        }
}

void standard_d_test_1(dev_uc_suite suite) {
        dev_uc_check(suite, false, NULL);
}

void standard_d_test_2(dev_uc_suite suite) {
        dev_uc_check(suite, false, NULL);
}

void standard_e_test_1(dev_uc_suite suite) {
        dev_uc_check(suite, false, "Failure!");
        dev_uc_check(suite, false, "Failure!!");
}

void standard_e_test_2(dev_uc_suite suite) {
        dev_uc_check(suite, false, "Hmm...");
}

static void test_uc_report_standard_with_tests(uc_suite suite) {
        dev_uc_suite sut_suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) {
                fputs("Failed to create temporary file.", stderr);
        }

        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                fputs("Failed to close temporary file.", stderr);
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_add_test(sut_suite, &standard_d_test_1, "A test...",
                        "This is a test...");
        dev_uc_check(sut_suite, false, NULL);
        dev_uc_add_test(sut_suite, &standard_d_test_2, NULL, NULL);
        dev_uc_run_tests(sut_suite);
        dev_uc_report_standard(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite,
                 files_eq(tmp_file_path, TEST_DIR "uc_report_standard_d"),
                 "Check standard report d.");

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        sut_suite = dev_uc_init(dev_UC_OPT_NONE, "Suite!", "Comment!");
        dev_uc_check(sut_suite, true, NULL);
        dev_uc_add_test(sut_suite, standard_e_test_1, "Test!", "Test comment!");
        dev_uc_add_test(sut_suite, standard_e_test_2, "Another test!",
                    "Another test comment!");
        dev_uc_run_tests(sut_suite);
        dev_uc_report_standard(sut_suite);
        dev_uc_free(sut_suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        uc_check(suite,
                 files_eq(tmp_file_path, TEST_DIR "uc_report_standard_e"),
                 "Check standard report e.");

        if (remove(tmp_file_path) == -1) {
                fputs("Could not remove temporary file", stderr);
        }

        if (close(orig_stdout) == -1) {
                fputs("Could not close dup'd stdout fd", stderr);
        }
}

static void incr_static(dev_uc_suite suite) {
        static int x = 0;
        ++x;

        dev_uc_check(suite, x == 1, "Should increment once from 0 each time.");
}

static void test_isolation(uc_suite suite) {
        dev_uc_suite sut_suite;

        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_add_test(sut_suite, &incr_static, NULL, NULL);
        dev_uc_add_test(sut_suite, &incr_static, NULL, NULL);
        dev_uc_add_test(sut_suite, &incr_static, NULL, NULL);
        dev_uc_add_test(sut_suite, &incr_static, NULL, NULL);
        dev_uc_add_test(sut_suite, &incr_static, NULL, NULL);
        dev_uc_run_tests(sut_suite);

        uc_check(suite, dev_uc_all_tests_passed(sut_suite),
                 "Check each test ran in a separate address space.");

        dev_uc_free(sut_suite);
}

static int before_hook_int = 0;

static void incr_before_hook_int() {
        /* To check hooks are run in the separate address space,
         * rather than running once before forking anything.
         */
        static int x = 0;
        x -= 3;

        /* If we find out we're not running in a separate address space (i.e.
         * the test's address space), return early causing
         * check_before_hook_int to fail since the decrements below will not
         * run.
         */
        if (x != -3) return;

        ++before_hook_int;
        ++before_hook_int;
}

static void check_before_hook_int(dev_uc_suite suite) {
        dev_uc_check(suite, before_hook_int == 2, NULL);
}

static void test_before_hook(uc_suite suite) {
        dev_uc_suite sut_suite;

        sut_suite = dev_uc_init(dev_UC_OPT_NONE, NULL, NULL);
        dev_uc_add_hook(sut_suite, dev_BEFORE, &incr_before_hook_int);
        /* Need to run multiple times to accurately check the hook ran
         * for each test, and that it ran in the test's address space.
         */
        dev_uc_add_test(sut_suite, &check_before_hook_int, NULL, NULL);
        dev_uc_add_test(sut_suite, &check_before_hook_int, NULL, NULL);
        dev_uc_add_test(sut_suite, &check_before_hook_int, NULL, NULL);
        dev_uc_add_test(sut_suite, &check_before_hook_int, NULL, NULL);
        dev_uc_add_test(sut_suite, &check_before_hook_int, NULL, NULL);
        dev_uc_run_tests(sut_suite);

        uc_check(suite, dev_uc_all_tests_passed(sut_suite),
                 "Check each hook ran before each test, in the test's address "
                 "space.");

        dev_uc_free(sut_suite);
}

