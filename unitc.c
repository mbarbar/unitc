/* Implementation of functions declared in unitc.h */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

static const char *DEFAULT_SUITE_NAME = "Main";

/** Representation of a call to uc_check. */
struct check {
        bool result;
        char *comment;
        unsigned int check_num;
};

struct uc_suite {
        char *name;
        char *comment;
        uint_least8_t options;
        /* List of struct checks in REVERSE order. */
        GList *checks;
        unsigned int num_successes;
        unsigned int num_checks;
};

/** Outputs "Succesful checks: x/y." appropriate to suite. */
static void output_checks_fraction(uc_suite suite);

uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment) {
        uc_suite suite = malloc(sizeof(struct uc_suite));
        if (suite == NULL) return NULL;

        suite->options = options;
        suite->checks = NULL;
        suite->num_successes = 0;
        suite->num_checks = 0;

        ALLOC_STRING(name, suite->name, { uc_free(suite); return NULL; });
        ALLOC_STRING(comment, suite->comment, { uc_free(suite); return NULL; });

        return suite;
}

void uc_free(uc_suite suite) {
        if (suite == NULL) return;
        if (suite->name != NULL) free(suite->name);
        if (suite->comment != NULL) free(suite->comment);

        if (suite->checks != NULL) {
                GList *curr, *to_free;
                curr = g_list_first(suite->checks);

                while (curr != NULL) {
                        struct check *curr_data = curr->data;

                        if (curr_data->comment != NULL) {
                                free(curr_data->comment);
                        }

                        to_free = curr->data;
                        curr = curr->next;
                        free(to_free);
                }

                g_list_free(suite->checks);
        }

        free(suite);
}

void uc_check(uc_suite suite, const bool cond, const char *comment) {
        if (suite == NULL) return;
        struct check *check;

        check = malloc(sizeof(struct check));
        if (check == NULL) {
                fprintf(stderr, "uc_check: failure to check: %s\n",
                        comment == NULL ? "no comment provided." : comment);
                return;
        }

        if (cond) ++suite->num_successes;
        ++suite->num_checks;

        check->result = cond;
        check->check_num = suite->num_checks;

        ALLOC_STRING(comment, check->comment,
                     { fprintf(stderr,
                               "uc_check: failure to save comment: %s\n",
                               comment); });

        suite->checks = g_list_prepend(suite->checks, check);
}

void uc_add_test(uc_suite suite, const void (*test)(uc_suite suite),
                 const char *name, const char *comment) {
}

void uc_run_tests(uc_suite suite) {
}

void uc_report_basic(uc_suite suite) {
        if (suite == NULL) return;

        puts(suite->name != NULL ? suite->name : DEFAULT_SUITE_NAME);
        if (suite->comment != NULL) puts(suite->comment);

        output_checks_fraction(suite);
}

void uc_report_standard(uc_suite suite) {
        if (suite == NULL) return;

        puts(suite->name != NULL ? suite->name : DEFAULT_SUITE_NAME);
        if (suite->comment != NULL) puts(suite->comment);

        output_checks_fraction(suite);

        /* Traverse backwards to maintain order checks were done. */
        for (GList *curr = g_list_last(suite->checks); curr != NULL;
             curr = curr->prev) {
                struct check *curr_data;
                char *comment, *str_fmt;

                curr_data = curr->data;

                /* Nothing to print. */
                if (curr_data->result) continue;

                comment = (char *)curr_data->comment;

                str_fmt = comment != NULL ? "Check failed: %s\n"
                                            : "Check failed: Check #%u.\n";

                if (comment != NULL) printf(str_fmt, comment);
                else printf(str_fmt, curr_data->check_num);
        }
}

void output_checks_fraction(uc_suite suite) {
        if (suite == NULL) return;
        printf("Successful checks: %d/%d.\n", suite->num_successes,
               suite->num_checks);
}
