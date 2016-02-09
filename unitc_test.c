/** \file unitc_test.c
  * \brief Tests for the unitc library.
  */

#define _XOPEN_SOURCE 500

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "unitc.h"

/* http://www.jera.com/techinfo/jtns/jtn002.html */
#include "minunit.h"

/** Location of (manually created) files containing expected output. */
#define TEST_DIR "test_resources/"
#define TMP_FILE_TEMPLATE TEST_DIR "XXXXXX"

/** Set up for redirecting stdout to path (opens path to fd). Returns on
  * failure. (For minunit).
  */
#define STDOUT_REDIR_SET_UP(path, fd) \
        do { \
                if ((fd = open(path, O_WRONLY | O_TRUNC)) == -1) { \
                        return "Failed to open temporary file."; \
                }; \
                fflush(stdout); \
                if (dup2(fd, STDOUT_FILENO) == -1) { \
                        close(fd); \
                        return "dup2 failed."; \
                } \
        } while (0)

/** Closes fd and redirects stdout to orig_stdout. */
#define STDOUT_REDIR_TEAR_DOWN(fd, orig_stdout) \
        do { \
                if (close(fd) == -1) { \
                        return "Failed to close temporary file."; \
                }; \
                fflush(stdout); \
                if (dup2(orig_stdout, STDOUT_FILENO) == -1) { \
                        close(fd); \
                        return "dup2 failed (to revert stdout)."; \
                } \
        } while (0)

/** For minunit. */
int tests_run = 0;

/** Check files at path_a and path_b are equal.
  *
  * @param path_a,path_b Paths to files to compare.
  *
  * @return true if files at path_a and path_b contain the same contents, false
  *         otherwise. Returns false if the files at the specified paths cannot
  *         be opened.
  */
static bool files_eq(char *path_a, char *path_b);

/** List of 'mu_run_test's. */
static char *minunit_tests(void);

/** Tests for function files_eq. */
static char *test_files_eq(void);
/** Tests for uc_init (just that it never fails before running more tests). */
static char *test_uc_init(void);
/** Tests for uc_check and uc_report_basic. */
static char *test_uc_check_and_uc_report_basic(void);
/** Tests for and uc_report_standard. */
static char *test_uc_report_standard(void);

int main(void) {
        char *minunit_result;

        minunit_result = minunit_tests();
        if (minunit_result == NULL) fputs("minunit tests passed.", stderr);
        else fprintf(stderr, "minunit tests failed: %s\n", minunit_result);

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

static char *minunit_tests(void) {
        mu_run_test(test_files_eq);
        mu_run_test(test_uc_init);
        mu_run_test(test_uc_check_and_uc_report_basic);
        mu_run_test(test_uc_report_standard);

        return NULL;
}

static char *test_files_eq(void) {
        mu_assert("Check unique equal files are equal",
                  files_eq(TEST_DIR "files_eq_equal_a",
                           TEST_DIR "files_eq_equal_b"));

        mu_assert("Check unique unequal files are not equal",
                  !files_eq(TEST_DIR "files_eq_diff_a",
                            TEST_DIR "files_eq_diff_b"));

        mu_assert("Check unique empty files are equal",
                  files_eq(TEST_DIR "files_eq_empty_a",
                           TEST_DIR "files_eq_empty_b"));

        mu_assert("Check file is equal to itself",
                  files_eq(TEST_DIR "files_eq_diff_a",
                           TEST_DIR "files_eq_diff_a"));

        return NULL;
}

static char *test_uc_init(void) {
        uc_suite suite_a, suite_b;

        suite_a = uc_init(UC_OPT_NONE, NULL, NULL);
        mu_assert("Check suite_a was allocated", suite_a != NULL);
        uc_free(suite_a);

        suite_a = NULL;
        suite_a = uc_init(UC_OPT_NONE, "Main", "Test suite.");
        mu_assert("Check suite_a was allocated again", suite_a != NULL);
        uc_free(suite_a);

        suite_b = uc_init(UC_OPT_NONE, "Name", NULL);
        mu_assert("Check suite_b was allocated", suite_b != NULL);
        uc_free(suite_b);

        return NULL;
}

static char *test_uc_check_and_uc_report_basic(void) {
        uc_suite suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) return "Failed to create temporary file.";
        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                return "Failed to close temporary file.";
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, "Suite 1", "Suite 1's comment.");
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        uc_check(suite, true, "True.");
        uc_check(suite, false, "False.");
        uc_report_basic(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check basic report a.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_basic_a"));

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, "Second suite", NULL);
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        uc_check(suite, true, "True!");
        uc_report_basic(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check basic report b.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_basic_b"));

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, NULL, NULL);
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        for (int i = 0; i < 5; ++i) uc_check(suite, false, "False!");
        uc_report_basic(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check basic report c.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_basic_c"));

        if (remove(tmp_file_path) == -1) {
                return "Could not remove temporary file";
        }

        return NULL;
}

static char *test_uc_report_standard(void) {
        uc_suite suite;
        char tmp_file_path[strlen(TMP_FILE_TEMPLATE) + 1];
        int tmp_file_fd, orig_stdout;

        strncpy(tmp_file_path, TMP_FILE_TEMPLATE,
                strlen(TMP_FILE_TEMPLATE) + 1);

        orig_stdout = dup(STDOUT_FILENO);
        tmp_file_fd = mkstemp(tmp_file_path);
        if (tmp_file_fd == -1) return "Failed to create temporary file.";
        /* Since the STDOUT_REDIR_SET_UP opens. */
        if (close(tmp_file_fd) == -1) {
                return "Failed to close temporary file.";
        };

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, NULL, NULL);
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        uc_check(suite, false, "False.");
        uc_report_standard(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check standard report a.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_standard_a"));

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, "Suite b", "This is suite b");
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        uc_check(suite, true, NULL);
        uc_check(suite, true, "");
        uc_check(suite, true, NULL);
        uc_report_standard(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check standard report b.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_standard_b"));

        STDOUT_REDIR_SET_UP(tmp_file_path, tmp_file_fd);
        suite = uc_init(UC_OPT_NONE, "Suite c", NULL);
        if (suite == NULL) {
                close(tmp_file_fd);
                return "uc_init failed.";
        }
        uc_check(suite, true, NULL);
        uc_check(suite, false, "1st failure");
        uc_check(suite, false, "2nd failure");
        uc_check(suite, false, "3rd failure");
        uc_check(suite, true, "True.");
        uc_check(suite, false, "4th failure");
        uc_report_standard(suite);
        uc_free(suite);
        STDOUT_REDIR_TEAR_DOWN(tmp_file_fd, orig_stdout);

        mu_assert("Check standard report c.",
                  files_eq(tmp_file_path, TEST_DIR "uc_report_standard_c"));

        if (remove(tmp_file_path) == -1) {
                return "Could not remove temporary file";
        }

        return NULL;

}
