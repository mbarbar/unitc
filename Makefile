# unitc Makefile

CC     = gcc
CFLAGS = -std=c99 -Wall -Werror

DOC_CONF = doxygen_conf

HEADERS = unitc.h

TEST_OBJ = unitc.o unitc_test.o
TEST_OUT = unitc_test

OUTS = $(TEST_OUT) $(DOC_OUT)

.PHONY: test doc clean

test: $(TEST_OUT)
	./$(TEST_OUT)

doc: *.c *.h $(DOC_CONF)
	doxygen $(DOC_CONF)

$(TEST_OUT): $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -r -f *.o $(OUTS) doc

