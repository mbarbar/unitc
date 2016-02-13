/** \file unitc.h
  * \brief Function prototypes, macros, and typedefs for unitc.
  */

#include <stdint.h>
#include <stdbool.h>

/** @name Options
  */
/**@{*/
#define UC_OPT_NONE (0) /**< No options set. */
/**@}*/

/** A uc_suite carries specified options, successes/failures, and comments of
  * a test suite.
  */
typedef struct uc_suite *uc_suite;


/** Create a test suite with the specified options.
  *
  * @param options Logical OR of values prefixed with UC_OPT.
  * @param name    Name of the suite - to appear as the title of reports.
  *                Defaults to 'Main' if NULL.
  * @param comment A description of the suite - to appear as a complement
  *                to name. Can be omitted by passing NULL.
  *
  * @return A uc_suite with options specified by options or NULL on error.
  */
uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment);

/** Free a test suite. Using suite after this call results in undefined
  * behaviour. Does nothing if suite is NULL.
  *
  * @param suite Test suite to free.
  */
void uc_free(uc_suite suite);

/** Check whether an expression evaluates as expected. Does nothing if suite is
  * NULL.
  *
  * @param suite   Test suite in which the check belongs to.
  * @param cond    The condition to check - a check is deemed successful
  *                when cond evaluates to true.
  * @param comment Information about what is being checked. No other
  *                information about a check is available in (applicable)
  *                reports. Can be omitted by passing NULL.
  */
void uc_check(uc_suite suite, const bool cond, const char *comment);

/** Add a test to suite to be executed when run_test is called on the same
  * suite.
  *
  * @param suite   Test suite to add test to.
  * @param test    Test to execute. A test is a collection of uc_checks.
  *                test takes in a uc_suite which is the instance used in
  *                calling uc_check within test. suite will be passed to it.
  * @param name    Name of the test - to appear in reports. Defaults to
  *                "Test #" where # depends on it's position in the queue if
  *                NULL.
  * @param comment A description of the test - to appear in reports. Can be
  *                omitted by passing NULL.
  */
void uc_add_test(uc_suite suite, const void (*test)(uc_suite suite),
                 const char *name, const char *comment);

/** Run all tests added by uc_add_test (in order they were added in).
  *
  * @param suite Test suite to run tests for.
  */
void uc_run_tests(uc_suite suite);

/** Outputs a report showing suite's title, comment, and the number of
  * successful checks vs. failed checks as a fraction. Outputs nothing if
  * suite is NULL.
  *
  * Example:
  * Suite name
  * Some information about the suite.
  * Successful checks: 5/20.
  *
  * @param suite Test suite to generate report from.
  */
void uc_report_basic(uc_suite suite);

/** Outputs a report showing suite's title, comment, the number of successful
  * checks vs. failed checks as a fraction, and all comments of failed checks.
  * Outputs nothing if suite is NULL.
  * Example (the 'Check #8.' is auto generated when no comment is provided):
  * Suite name
  * Some information about the suite.
  * Successful checks: 17/20.
  * Check failed: A comment
  * Check failed: Another comment.
  * Check failed: Check #8.
  *
  * @param suite Test suite to generate report from.
  */
void uc_report_standard(uc_suite suite);
