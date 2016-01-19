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
  * @param comment A description of the suite - to appear as a complement
  *                to name. Can be omitted by passing NULL.
  *
  * @return A uc_suite with options specified by options or NULL on error.
  */
uc_suite uc_init(const uint_least8_t options, const char *name,
                 const char *comment);

/** Free a test suite. Using suite after this call results in undefined
  * behaviour.
  *
  * @param suite Test suite to free.
  */
void uc_free(uc_suite suite);

/** Check whether an expression evaluates as expected.
  *
  * @param suite   Test suite in which the check belongs to.
  * @param cond    The condition to check - a check is deemed successful
  *                when cond evaluates to true.
  * @param comment Information about what is being checked. No other
  *                information about a check is available in (applicable)
  *                reports
  */
void uc_check(uc_suite suite, const bool cond, const char *comment);

/** Outputs a report showing suite's title, comment, and the number of
  * successful checks vs. failed checks as a fraction.
  *
  * @param suite Test suite to generate report from.
  */
void uc_report_basic(uc_suite suite);

