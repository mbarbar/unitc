#!/bin/sh

TEST_BIN="./unitc_test"
VALGRIND_OUT=`valgrind --leak-check=full --show-leak-kinds=all\
              --trace-children=yes -v $TEST_BIN 2>&1`

echo "$VALGRIND_OUT" | egrep 'ERROR SUMMARY:.*[1-9]' > /dev/null
if [ $? -eq 0 ]; then
        echo "$0: errors found."
        echo -e "$VALGRIND_OUT" | less
else
        echo "$0: valgrind memcheck passed."
fi
