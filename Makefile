
GCC_EXECUTABLE=gcc
GCC_GENERIC_FLAGS=-Werror -Wextra -Wall -pedantic
GCC_RUNTIME_FLAGS=-Warray-bounds -Wformat-overflow -Wnonnull -Wstringop-overflow -Wuninitialized
GCC_PARANOID_FLAGS=#-Wconversion
GCC=gcc -Werror -Wextra -Wall -pedantic \

GCC=${GCC_EXECUTABLE} ${GCC_GENERIC_FLAGS} ${GCC_RUNTIME_FLAGS} ${GCC_PARANOID_FLAGS}

DEBUG=-DGFS_DEBUG

#RNG_SOURCE=/dev/zero
RNG_SOURCE=/dev/random

TEST_DISK_SIZE=1G

all: run_test

.PHONY: run_test
run_test: test test_disk_1 test_disk_2
	./test

.PHONY: clear
clear: clear_test clear_test.o clear_gfs.o clear_test_disk_1 clear_test_disk_2

# testing

test: test.o gfs.o helpers.o
	${GCC} -o test test.o gfs.o helpers.o
.PHONY: clear_test
clear_test:
	rm test || true

test.o: test.c gfs.h
	${GCC} -c test.c
.PHONY: clear_test.o
clear_test.o:
	rm test.o || true

# library

gfs.o: Makefile gfs.c gfs.h gfs_block.c gfs_block.h gfs_file.c gfs_file.h gfs_disk.c gfs_disk.h gfs_folder.c gfs_folder.h
	${GCC} ${DEBUG} -c gfs.c
.PHONY: clear_gfs.o
clear_gfs.o:
	rm gfs.o || true

helpers.o: helpers.c helpers.h
	${GCC} -c helpers.c
.PHONY: clear_helpers.o
	rm helpers.o || true

# test disks

test_disk_1:
	dd if=${RNG_SOURCE} of=test_disk_1 bs=${TEST_DISK_SIZE} count=1
.PHONY: clear_test_disk_1
clear_test_disk_1:
	rm test_disk_1 || true

test_disk_2: 
	dd if=${RNG_SOURCE} of=test_disk_2 bs=${TEST_DISK_SIZE} count=1
.PHONY: clear_test_disk_2
clear_test_disk_2:
	rm test_disk_2 || true
