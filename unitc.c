/* Implementation of functions declared in unitc.h */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "unitc.h"

struct uc_suite {
        int stub;
};

uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment) {
        return NULL;
}

void uc_free(uc_suite suite) {
}

void uc_check(uc_suite suite, const bool cond, const char *comment) {
}

void uc_report_basic(uc_suite suite) {
}

