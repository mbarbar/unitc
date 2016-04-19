/* Implementation of functions declared in unitc.h */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include <sys/wait.h>

#include <glib.h>

#include "unitc.h"

#define ALLOC_STRING(src, dst, exec_on_failure)\
        do {\
                if ((src) != NULL) {\
                        (dst) = malloc(sizeof(char) * (strlen(src) + 1));\
                        if ((dst) == NULL) {\
                                { exec_on_failure }\
                        }\
        \
                        strcpy(dst, src);\
                } else {\
                        (dst) = NULL;\
                }\
        } while (0)

/** Call read or write and on failure execute exec_on_failure. rw is the name
  * of the function to call - read or write.
  */
#define TRY_RW(rw, fd, buf, count, exec_on_failure)\
        do {\
                if (rw((fd), (buf), (count)) == -1) {\
                        { exec_on_failure }\
                }\
        } while (0)

/** Indices to read/write from/to pipes. */
#define R 0
#define WR 1

static const char DEFAULT_SUITE_NAME[] = "Main";
static const char INDENTATION[] = "    ";

/** Representation of a call to uc_check. */
struct check {
        bool result;
        char *comment;
        /* Relative to the test this check is a part of. */
        unsigned int check_num;
};

struct test {
        char *name;
        char *comment;
        void (*test_func)(uc_suite);

        unsigned int num_succ;
        unsigned int num_checks;

        unsigned int test_num;

        /* List of struct checks in REVERSE order. */
        GList *checks;
};

struct uc_suite {
        char *name;
        char *comment;
        uint_least8_t options;

        unsigned int num_succ;
        unsigned int num_checks;
        unsigned int num_tests;

        /* List of struct tests in REVERSE order of addition. The final test
         * is for all checks made outside a test.
         */
        GList *tests;
        /* Entry in tests of the currently running test. When uc_run_tests is
         * not running, points to the final element of test.
         */
        GList *curr_test;
};

static void output_indent(const unsigned int level);

/** Outputs "Successful checks: succ/total." */
static void output_checks_fraction(const unsigned int succ,
                                   const unsigned int total,
                                   const unsigned int indent);

/** Test output common to all reports.
  *
  * [indent]Name
  * [indent]Comment
  * [indent][indent]output_checks_fraction(x, y);
  */
static void output_test_common(struct test *test, const unsigned int indent);

/** Output the failures associated with test, creating a "Check #x" for failed
  * checks without a comment.
  */
static void output_test_failures(struct test *test, const unsigned int indent);

/** Outputs the suites name, comment, total successful checks, successful
  * checks ("dangling checks"). This is common to all reports.
  *
  * [indent]Name
  * [indent]Comment
  * [indent]Total successful checks: x/y.
  * [indent][indent]output_checks_fraction(x, y);
  */
static void output_main_header(uc_suite);

/** Writes the results found in test to wr_fd. If a call to write fails,
  * abort() is called.
  *
  * Format:
  *  1. Write any non-null character if the checks list is not empty. If the
  *     checks list is empty, skip to #8.
  *  2. Write a bool for the result of the current check.
  *  3. Write a null character if no comment exists for the current check, and
  *     skip to #6. Otherwise write a non-null character.
  *  4. Write a size_t for the number of characters in the comment string of
  *     the current check (does not include the terminating null character).
  *  5. Write the comment string from the current check, including the
  *     terminating null character.
  *  6. Write a non-null character if the checks list still has elements.
  *  7. Repeat #2-#6 until the entire checks list has been written.
  *  8. Write a null character.
  */
static void write_test_results(const struct test *test, const int wr_fd);
/** Reads the results from r_fd and fills in the curr_test in the suite.
  * The checks list is populated by calling uc_check. Returns true if FULL
  * results were read, otherwise returns false.
  *
  * The format is as defined by write_test_results.
  */
static bool read_test_results(uc_suite, const int r_fd);

static void struct_check_free(void *);
static void struct_test_free(void *);

uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment) {
        uc_suite suite = malloc(sizeof(struct uc_suite));
        if (suite == NULL) return NULL;

        suite->options = options;
        suite->tests = NULL;
        suite->num_succ = 0;
        suite->num_checks = 0;
        suite->num_tests = 0;

        ALLOC_STRING(name, suite->name, { uc_free(suite); return NULL; });
        ALLOC_STRING(comment, suite->comment, { uc_free(suite); return NULL; });

        /* Add a test for all checks made outside a test. */
        uc_add_test(suite, NULL, NULL, NULL);
        suite->curr_test = g_list_last(suite->tests);

        return suite;
}

void uc_free(uc_suite suite) {
        if (suite == NULL) return;
        if (suite->name != NULL) free(suite->name);
        if (suite->comment != NULL) free(suite->comment);

        g_list_free_full(suite->tests, &struct_test_free);

        free(suite);
}

void uc_check(uc_suite suite, const bool cond, const char *comment) {
        if (suite == NULL) return;
        struct check *check;
        struct test *curr_test;

        curr_test = (struct test *)suite->curr_test->data;

        check = malloc(sizeof(struct check));
        if (check == NULL) {
                fprintf(stderr, "uc_check: failure to check: %s\n",
                        comment == NULL ? "no comment provided." : comment);
                return;
        }

        ++suite->num_checks;
        ++curr_test->num_checks;
        if (cond) {
                ++suite->num_succ;
                ++curr_test->num_succ;
        }

        ALLOC_STRING(comment, check->comment,
                     { fprintf(stderr,
                               "uc_check: failure to save comment: %s\n",
                               comment); });

        check->result = cond;
        check->check_num = curr_test->num_checks;

        curr_test->checks = g_list_prepend(curr_test->checks, check);
}

void uc_add_test(uc_suite suite, void (*test_func)(uc_suite suite),
                 const char *name, const char *comment) {
        struct test *test;
        test = malloc(sizeof(struct test));
        if (test == NULL) {
                fprintf(stderr, "uc_add_test: failure to add test: %s\n",
                        comment == NULL ? "no name provided." : name);
                return;
        }

        ALLOC_STRING(name, test->name,
                     { fprintf(stderr,
                               "uc_add_test: failure to save name: %s\n",
                               name); });
        ALLOC_STRING(comment, test->comment,
                     { fprintf(stderr,
                               "uc_add_test: failure to save comment: %s\n",
                               comment); });

        test->test_func = test_func;
        test->num_succ = 0;
        test->num_checks = 0;
        test->test_num = suite->num_tests;
        test->checks = NULL;

        ++suite->num_tests;
        suite->tests = g_list_prepend(suite->tests, test);
}

void uc_run_tests(uc_suite suite) {
        /* Guaranteed to have at least one element from uc_init. */
        for (GList *curr = g_list_last(suite->tests)->prev; curr != NULL;
             curr = curr->prev) {
                int ipc_pipe[2];
                struct test *test;
                pid_t pid;

                if (pipe(ipc_pipe) == -1) {
                        fputs("uc_run_tests: cannot create pipe,"
                              "not running test.\n", stderr);
                        continue;
                }

                suite->curr_test = curr;
                test = curr->data;

                pid = fork();
                if (pid == -1) {
                        fputs("uc_run_tests: cannot create process.\n", stderr);
                } else if (pid == 0) {
                        close(ipc_pipe[R]);
                        if (test->test_func != NULL) test->test_func(suite);

                        write_test_results(test, ipc_pipe[WR]);

                        uc_free(suite);
                        close(ipc_pipe[WR]);
                        exit(EXIT_SUCCESS);
                } else {
                        int wstatus;
                        if (waitpid(pid, &wstatus, 0) == -1) {
                                fputs("uc_run_tests: error creating process.\n",
                                      stderr);
                                continue;
                        } else if (WIFSIGNALED(wstatus)) {
                                /* The writing process called abort(). Don't
                                 * even try to read.
                                 */
                                fputs("uc_run_tests: test failed to run.\n",
                                      stderr);
                        } else {
                                if (!read_test_results(suite, ipc_pipe[R])) {
                                        /* Information may be incomplete.
                                         * Delete it all.
                                         */
                                        fputs("uc_run_tests: test failed to run"
                                              ".\n", stderr);
                                        g_list_free_full(test->checks,
                                                        &struct_check_free);
                                        test->checks = NULL;
                                        test->num_succ = 0;
                                        test->num_checks = 0;
                                }
                        }

                }

                /* child process which closes these will never reach here. */
                if (close(ipc_pipe[R]) == -1) {
                        fputs("uc_run_tests: cannot close read end of pipe.\n",
                              stderr);
                }

                if (close(ipc_pipe[WR]) == -1) {
                        fputs("uc_run_tests: cannot close write end of pipe.\n",
                              stderr);
                }
        }

        /* Reset curr_test to account for "dangling checks". */
        suite->curr_test = g_list_last(suite->tests);
}

void uc_add_hook(uc_suite suite, enum hook_type type, void (*hook)(void)) {
        return;
}

bool uc_all_tests_passed(uc_suite suite) {
        if (suite == NULL) return false;

        for (GList *curr = suite->tests; curr != NULL; curr = curr->next) {
                struct test *test = curr->data;
                if (test->num_succ != test->num_checks) return false;
        }

        return true;
}

void uc_report_basic(uc_suite suite) {
        if (suite == NULL) return;

        output_main_header(suite);

        /* Guaranteed to have at least one element from uc_init. */
        for (GList *curr = g_list_last(suite->tests)->prev; curr != NULL;
             curr = curr->prev) {
                output_test_common(curr->data, 1);
        }
}

void uc_report_standard(uc_suite suite) {
        GList *main_test;
        if (suite == NULL) return;

        main_test = g_list_last(suite->tests);

        output_main_header(suite);
        output_test_failures(main_test->data, 1);

        /* Guaranteed to have at least one element from uc_init. */
        for (GList *curr = g_list_last(suite->tests)->prev; curr != NULL;
             curr = curr->prev) {
                output_test_common(curr->data, 1);
                output_test_failures(curr->data, 2);
        }
}

void output_indent(const unsigned int level) {
        for (unsigned int i = 0; i < level; ++i) printf(INDENTATION);
}

void output_checks_fraction(const unsigned int succ, const unsigned int total,
                            const unsigned int indent) {
        output_indent(indent);
        printf("Successful checks: %u/%u.\n", succ, total);
}

void output_test_common(struct test *test, const unsigned int indent) {
        if (test == NULL) return;

        puts("");

        output_indent(indent);
        test->name != NULL ? puts(test->name) :
                             printf("Test #%u\n", test->test_num);

        if (test->comment != NULL) {
                output_indent(indent);
                puts(test->comment);
        }

        output_checks_fraction(test->num_succ, test->num_checks, indent + 1);
}

void output_test_failures(struct test *test, const unsigned int indent) {
        if (test == NULL) return;

        for (GList *curr = g_list_last(test->checks); curr != NULL;
             curr = curr->prev) {
                struct check *check;
                char *comment;

                check = curr->data;
                /* Print nothing for successful checks. */
                if (check->result) continue;

                comment = check->comment;

                output_indent(indent);
                if (comment != NULL) {
                        printf("Check failed: %s\n", comment);
                } else {
                        printf("Check failed: Check #%u.\n", check->check_num);
                }
        }
}

void output_main_header(uc_suite suite) {
        struct test *curr_test;

        if (suite == NULL) return;

        puts(suite->name != NULL ? suite->name : DEFAULT_SUITE_NAME);
        if (suite->comment != NULL) puts(suite->comment);

        printf("Total successful checks: %u/%u.\n", suite->num_succ,
               suite->num_checks);

        curr_test = suite->curr_test->data;
        output_checks_fraction(curr_test->num_succ, curr_test->num_checks, 1);
}

void write_test_results(const struct test *test, const int wr_fd) {
        static const char non_null = 'X';
        static const char null = '\0';

        /* Format is defined above prototype. */
        for (GList *curr = g_list_last(test->checks); curr != NULL;
             curr = curr->prev) {
                size_t comment_len;
                struct check *check;

                /* There are still checks to write. */
                TRY_RW(write, wr_fd, &non_null, sizeof(char), { abort(); });

                check = curr->data;
                TRY_RW(write, wr_fd, &check->result, sizeof(bool),
                       { abort(); });

                if (check->comment != NULL) {
                        /* Indicate a comment exists. */
                        TRY_RW(write, wr_fd, &non_null, sizeof(char),
                               { abort(); });
                        comment_len = strlen(check->comment);

                        TRY_RW(write, wr_fd, &comment_len, sizeof(size_t),
                               { abort(); });

                        TRY_RW(write, wr_fd, check->comment,
                               sizeof(char) * (comment_len + 1), { abort(); });
                } else TRY_RW(write, wr_fd, &null, sizeof(char), { abort(); });
        }

        /* No more checks to write. */
        TRY_RW(write, wr_fd, &null, sizeof(char), { abort(); });
}

bool read_test_results(uc_suite suite, const int r_fd) {
#define RET_FALSE { return false; }
        /* Determines whether to continue reading or not. */
        char check_char;

        /* Format is defined above write_test_results' protoype. */
        TRY_RW(read, r_fd, &check_char, sizeof(char), RET_FALSE);
        while (check_char != '\0') {
                bool result;
                char *comment, comment_check;

                TRY_RW(read, r_fd, &result, sizeof(bool), RET_FALSE);
                TRY_RW(read, r_fd, &comment_check, sizeof(char), RET_FALSE);

                if (comment_check == '\0') comment = NULL;
                else {
                        size_t comment_len;

                        TRY_RW(read, r_fd, &comment_len, sizeof(size_t),
                               RET_FALSE);

                        comment = malloc(sizeof(char) * (comment_len + 1));
                        if (comment != NULL) {
                                TRY_RW(read, r_fd, comment,
                                       sizeof(char) * (comment_len + 1),
                                       { free(comment); return false; });
                        } else {
                                char comment_char;
                                fprintf(stderr, "Cannot save a comment.");
                                /* Read the comment one char at a time
                                 * anyway to sync.
                                 */
                                TRY_RW(read, r_fd, &comment_char, sizeof(char),
                                       RET_FALSE);
                                while (comment_char != '\0') {
                                        TRY_RW(read, r_fd, &comment_char,
                                               sizeof(char), RET_FALSE);
                                }
                        }
                }

                uc_check(suite, result, comment);
                if (comment != NULL) free(comment);

                TRY_RW(read, r_fd, &check_char, sizeof(char), RET_FALSE);
        }

        return true;
#undef RET_FALSE
}

void struct_check_free(void *data) {
        struct check *check;
        if (data == NULL) return;

        check = data;

        if (check->comment != NULL) free(check->comment);

        free(check);
}

void struct_test_free(void *data) {
        struct test *test;
        if (data == NULL) return;

        test = data;

        if (test->name != NULL) free(test->name);
        if (test->comment != NULL) free(test->comment);
        g_list_free_full(test->checks, &struct_check_free);

        free(test);
}
