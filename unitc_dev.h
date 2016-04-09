/** unitc_dev.h
  * The purpose of using dev_uc is to allow for testing unitc with... unitc.
  * Definitions are to come from unitc.c, not the installed library.
  * Otherwise, name clashes occur.
  */

#ifndef UNITC_DEV_H
#define UNITC_DEV_H

#include <stdint.h>
#include <stdbool.h>

#define dev_UC_OPT_NONE UC_OPT_NONE

typedef uc_suite dev_uc_suite;

dev_uc_suite dev_uc_init(const uint_least8_t options, const char *name,
                         const char *comment);

void dev_uc_free(dev_uc_suite suite);

void dev_uc_check(dev_uc_suite suite, const bool cond, const char *comment);

void dev_uc_add_test(dev_uc_suite suite, void (*test_func)(dev_uc_suite suite),
                 const char *name, const char *comment);

void dev_uc_run_tests(dev_uc_suite suite);

bool dev_uc_all_tests_passed(dev_uc_suite suite);

void dev_uc_report_basic(dev_uc_suite suite);

void dev_uc_report_standard(dev_uc_suite suite);

#endif /* UNITC_DEV_H */

