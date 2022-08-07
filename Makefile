CC=gcc
CFLAGS=-fsanitize=address -Wvla -Wall -Werror -g -std=gnu11 -lasan

objdump_x2017: objdump_x2017.c
	$(CC) $(CFLAGS) $^ -o $@

tests:
	echo "tests"

run_tests:
	echo "run_tests"

clean:
	rm objdump_x2017
	rm vm_x2017
