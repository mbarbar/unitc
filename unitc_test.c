/** \file unitc_test.c
  * \brief Tests for the unitc library.
  */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "unitc.h"

/* http://www.jera.com/techinfo/jtns/jtn002.html */
#include "minunit.h"

/** Location of (manually created) files containing expected output. */
#define TEST_DIR "test_resources/"

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

int main(void) {
        char *minunit_result;

        minunit_result = minunit_tests();
        if (minunit_result == NULL) puts("minunit tests passed.");
        else printf("minunit tests failed: %s\n", minunit_result);

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
