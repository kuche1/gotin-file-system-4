
GCC=gcc -Werror -Wextra -Wall -pedantic
DEBUG=-DGFS_DEBUG

TEST_DISK_SIZE=1G

all: run_test

.PHONY: run_test
run_test: test test_disk_1 test_disk_2
	./test

.PHONY: clear
clear: clear_test clear_test.o clear_gfs.o clear_test_disk_1 clear_test_disk_2

test: test.o gfs.o
	${GCC} -o test test.o gfs.o
.PHONY: clear_test
clear_test:
	rm test || true

test.o: test.c gfs.h
	${GCC} -c test.c
.PHONY: clear_test.o
clear_test.o:
	rm test.o || true

gfs.o: gfs.c gfs.h
	${GCC} ${DEBUG} -c gfs.c
.PHONY: clear_gfs.o
clear_gfs.o:
	rm gfs.o || true

test_disk_1:
	dd if=/dev/random of=test_disk_1 bs=${TEST_DISK_SIZE} count=1
.PHONY: clear_test_disk_1
clear_test_disk_1:
	rm test_disk_1 || true

test_disk_2: 
	dd if=/dev/random of=test_disk_2 bs=${TEST_DISK_SIZE} count=1
.PHONY: clear_test_disk_2
clear_test_disk_2:
	rm test_disk_2 || true
