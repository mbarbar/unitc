/* Implementation of functions declared in unitc.h */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "unitc.h"

static const char *DEFAULT_SUITE_NAME = "Main";

/** Representation of a call to uc_check. */
struct check {
        bool result;
        char *comment;
};

struct uc_suite {
        char *name;
        char *comment;
        uint_least8_t options;
        /* List of struct checks in REVERSE order. */
        GList *checks;
};

uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment) {
        uc_suite suite = malloc(sizeof(struct uc_suite));
        if (suite == NULL) return NULL;

        suite->options = options;
        suite->checks = NULL;

        if (name != NULL) {
                suite->name = malloc(sizeof(char) * (strlen(name) + 1));
                if (suite->name == NULL) {
                        uc_free(suite);
                        return NULL;
                }

                strcpy(suite->name, name);
        } else {
                suite->name = NULL;
        }

        if (comment != NULL) {
                suite->comment = malloc(sizeof(char) * (strlen(comment) + 1));
                if (suite->comment == NULL) {
                        uc_free(suite);
                        return NULL;
                }

                strcpy(suite->comment, comment);
        } else {
                suite->comment = NULL;
        }

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

        check->result = cond;

        if (comment != NULL) {
                check->comment = malloc(sizeof(char) * (strlen(comment) + 1));
                if (check->comment == NULL) {
                        fprintf(stderr,
                                "uc_check: failure to save comment: %s\n",
                                comment);
                } else {
                        strcpy(check->comment, comment);
                }
        } else {
                check->comment = NULL;
        }

        suite->checks = g_list_prepend(suite->checks, check);
}

void uc_add_test(uc_suite suite, const void (*test)(uc_suite suite),
                 const char *name, const char *comment) {
}

void uc_run_tests(uc_suite suite) {
}

void uc_report_basic(uc_suite suite) {
        if (suite == NULL) return;
        unsigned int successes, total;

        successes = total = 0;

        puts(suite->name != NULL ? suite->name : DEFAULT_SUITE_NAME);
        if (suite->comment != NULL) puts(suite->comment);

        if (suite->checks != NULL) {
                /* Order doesn't matter. */
                GList *curr = g_list_first(suite->checks);

                while (curr != NULL) {
                        struct check *curr_data = curr->data;
                        if (curr_data->result) ++successes;
                        ++total;

                        curr = curr->next;
                }
        }

        printf("Successful checks: %d/%d.\n", successes, total);
}

void uc_report_standard(uc_suite suite) {
        if (suite == NULL) return;
        unsigned int successes, total;
        /* Comments of failed checks. Includes NULL (i.e. no comment). */
        GList *to_print;

        successes = total = 0;
        to_print = NULL;

        puts(suite->name != NULL ? suite->name : DEFAULT_SUITE_NAME);
        if (suite->comment != NULL) puts(suite->comment);

        /* Traverse backwards to maintain order checks were done. */
        for (GList *curr = g_list_last(suite->checks); curr != NULL;
             curr = curr->prev) {
                struct check *curr_data = curr->data;

                if (curr_data->result) ++successes;
                else to_print = g_list_prepend(to_print, curr_data->comment);

                ++total;
        }

        printf("Successful checks: %d/%d.\n", successes, total);

        /* Order matters again. */
        for (GList *curr = g_list_last(to_print); curr != NULL;
             curr = curr->prev) {
                printf("Check failed: %s\n",
                       curr->data != NULL ? (char *)curr->data : "");
        }

        g_list_free(to_print);
}
