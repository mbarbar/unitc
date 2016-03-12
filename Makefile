# unitc Makefile

CC     = gcc
CFLAGS = -std=c99 -Wall -Werror -O3 `pkg-config --cflags --libs glib-2.0`

DOC_CONF = doxygen_conf

HEADERS = unitc.h

BUILD_OBJ = unitc.o
BUILD_OUT = libunitc.so

TEST_OBJ = dev_uc.o unitc_test.o
TEST_OUT = unitc_test

OUTS = $(TEST_OUT) $(DOC_OUT) $(BUILD_OUT)

.PHONY: build test doc clean

build:
	$(CC) $(CFLAGS) -fPIC -c -o unitc.o unitc.c
	$(CC) $(CFLAGS) -shared -o $(BUILD_OUT) $(BUILD_OBJ)

test: $(TEST_OUT)
	./$(TEST_OUT)

doc: *.c *.h $(DOC_CONF)
	doxygen $(DOC_CONF)

$(TEST_OUT): $(TEST_OBJ)
	$(CC) $(CFLAGS) -lunitc -o $@ $^

dev_uc.o: unitc.o unitc_dev.o
	ld -r unitc.o unitc_dev.o -o dev_uc.o; \
	tmp_file=`mktemp`; \
	nm --defined-only unitc.o | cut -d' ' -f3 > $$tmp_file; \
	objcopy --localize-symbols=$$tmp_file dev_uc.o; \
	rm -f $$tmp_file

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -g -c -o $@ $<

clean:
	rm -r -f *.o $(OUTS) doc

