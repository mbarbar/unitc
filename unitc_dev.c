/** unitc_dev.c
  * Implementation of unitc_dev.c.
  * The definitions of uc_* are to come from unitc.c not the installed library.
  */

#include <stdint.h>
#include <stdbool.h>

#include "unitc.h"
#include "unitc_dev.h"

dev_uc_suite dev_uc_init(const uint_least8_t options, const char *name,
                         const char *comment) {
        return (dev_uc_suite)uc_init(options, name, comment);
}

void dev_uc_free(dev_uc_suite suite) {
        uc_free((struct uc_suite *)suite);
}

void dev_uc_check(dev_uc_suite suite, const bool cond, const char *comment) {
        uc_check((struct uc_suite *)suite, cond, comment);
}

void dev_uc_add_test(dev_uc_suite suite, void (*test_func)(dev_uc_suite suite),
                    const char *name, const char *comment) {
        uc_add_test((struct uc_suite *)suite,
                    (void (*)(uc_suite suite))test_func, name, comment);
}

void dev_uc_run_tests(dev_uc_suite suite) {
        uc_run_tests((struct uc_suite *)suite);
}

void dev_uc_add_hook(dev_uc_suite suite, enum dev_hook_type type,
                     void (*hook)(void)) {
        uc_add_hook((struct uc_suite *)suite, (enum hook_type)type, hook);
}

bool dev_uc_all_tests_passed(dev_uc_suite suite) {
        return uc_all_tests_passed((struct uc_suite *)suite);
}

void dev_uc_report_basic(dev_uc_suite suite) {
        uc_report_basic((struct uc_suite *)suite);
}

void dev_uc_report_standard(dev_uc_suite suite) {
        uc_report_standard((struct uc_suite *)suite);
}
