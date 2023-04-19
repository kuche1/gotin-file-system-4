
all: test

build:
	#gcc -I/usr/include/fuse -lfuse myfs.c -o myfs
	gcc -I/usr/include/fuse3 -lfuse3 myfs.c -o myfs

test:
	./myfs -d -f -s myfsimage.bin somedir
	# -d debug
	# -s use single thread
	# -f keep it in the foreground, so that using `printf` works properly
	# call the image `myfsimage.bin`
	# `somedir` is where we want to mount the FS
	#
	# pressing ctrl+c will unmount
	# If you start FUSE in the background (without -f), then you can unmount with 
	# 	`fusermount -u somedir`

.PHONY: test2
test2:
	# gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
	gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
	./hello -d -s -f somedir
