# unitc

unitc is a simple to use C unit testing library.

## Documentation
unitc is documented with Doxygen. API documentation is available
[here](TODO).

## Installation
unitc has only been tested on Linux, but *should* work on POSIX systems.

### Dependencies
* make
* C compiler (only tested with gcc)
* pkg-config
* [GLib](https://developer.gnome.org/glib/) (also a runtime dependency)

### Instructions
1. Grab the source.
2. Build the shared object:
```
make build
```
3. Copy the shared object `libunitc.so` to some place in the library path.
4. Copy the header file `unitc.h` to some place in the include path.

## Testing
unitc is tested using itself and Valgrind's memcheck.

### Dependencies
As per **Installation** dependencies as well as

* unitc
* GNU Binutils and Core Utilities
* Valgrind (memcheck)
* less

### Instructions
Grab the source and
```
make test
```

This produces an executable of unitc's tests (`unitc_test` from `unitc_test.c`),
and runs unitc's unit tests and `unitc_memcheck.sh` (wrapper on memcheck).
Unit test results are output as a standard report. unitc_memcheck.sh
reports success or shows memcheck's output (in `less`) on failure.

## Example
The following tests an implementation of some C string functions:
```c
#include <stdlib.h>
#include <stdio.h>

#include <unitc.h>

size_t my_strlen(const char *s) {
        size_t len;
        for (len = 0; s[len] != '\0'; ++len);
        return len;
}

size_t my_strnlen(const char *s, size_t maxlen) {
        /* To demonstrate failure. */
        return my_strlen(s);
}

void test_my_strlen(uc_suite suite) {
        uc_check(suite, my_strlen("") == 0, "Check \"\".");
        uc_check(suite, my_strlen("ABC") == 3, "Check \"ABC\".");
        uc_check(suite, my_strlen("A") == 1, "Check \"A\".");
}

void test_my_strnlen(uc_suite suite) {
        uc_check(suite, my_strnlen("", 0) == 0, "Check \"\" with 0.");
        uc_check(suite, my_strnlen("ABC", 3) == 3, "Check \"ABC\" with 3.");
        /* Failure with no comment. */
        uc_check(suite, my_strnlen("ABC", 2) == 2, NULL);
        uc_check(suite, my_strnlen("ABCD", 9) == 4, "Check \"ABCD\" with 9.");
        uc_check(suite, my_strnlen("string", 3) == 3,
                 "Check \"string\" with 3.");
        uc_check(suite, my_strnlen("TEST!", 4) == 4, "Check \"TEST!\" with 4.");
}

int main(void) {
        uc_suite suite;
        suite = uc_init(UC_OPT_NONE, "my_str tests",
                        "Tests for my implementation of strlen and strnlen");
        if (suite == NULL) {
                fputs("Cannot create suite.\n", stderr);
                return EXIT_FAILURE;
        }

        uc_add_test(suite, test_my_strlen, "my_strlen tests", NULL);
        uc_add_test(suite, test_my_strnlen, "my_strnlen tests",
                    "Might see some failures...");
        uc_run_tests(suite);
        uc_report_standard(suite);
        uc_free(suite);

        return EXIT_SUCCESS;
}
```

Compiling with
```
gcc -Wall -Werror -lunitc -o example example.c
```
then running with
```
./example
```

produces the following output:
```
my_str tests
Tests for my implementation of strlen and strnlen
Total successful checks: 6/9.
    Successful checks: 0/0.

    my_strlen tests
        Successful checks: 3/3.

    my_strnlen tests
    Might see some failures...
        Successful checks: 3/6.
        Check failed: Check #3.
        Check failed: Check "string" with 3.
        Check failed: Check "TEST!" with 4.
```

A larger example would be unitc's own tests, stored in `unitc_test.c`.
