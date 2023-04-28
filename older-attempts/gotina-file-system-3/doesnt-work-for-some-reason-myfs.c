
//#define FUSE_USE_VERSION 29
#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int myfs_getattr(const char *path, struct stat *st)
{
  printf("===================================================== 1");
  // This is a very minimal example, you should open devfile, read and parse your file system structures in it and fill in st fields accordingly. 
  if (!strcmp(path, "/")) {
    // it's the root directory (just an example, you probably have more directories)
    st->st_mode = S_IFDIR | 0755; // access rights and directory type
    st->st_nlink = 2;             // number of hard links, for directories this is at least 2
    st->st_size = 4096;           // file size
  } else {
    st->st_mode = S_IFREG | 0644; // access rights and regular file type
    st->st_nlink = 2;             // number of hard links
    st->st_size = 4096;           // file size
  }
  // user and group. we use the user's id who is executing the FUSE driver
  st->st_uid = getuid();
  st->st_gid = getgid();
  return 0;
}

int myfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  filler(buffer, ".", NULL, 0);       // current directory reference
  filler(buffer, "..", NULL, 0);      // parent directory reference
  filler(buffer, "abc.txt", NULL, 0); // any filename at path in your image
  return 0;
}

static struct fuse_operations myfs_ops = {
    .getattr = myfs_getattr,
    .readdir = myfs_readdir,
};
 
char *devfile = NULL; // device or image file where the FS resides
 
int main(int argc, char **argv)
{
  int i;
 
  // get the device or image filename from arguments
  for (i = 1; i < argc && argv[i][0] == '-'; i++); // skips any options on the command line (should not be needed, just a precaution)
  if (i < argc) {
    devfile = realpath(argv[i], NULL); // convert to absolute path; returns NULL for non-existstent files
    memcpy(&argv[i], &argv[i+1], (argc-i) * sizeof(argv[0]));
    argc--;
  }

  // leave the rest to FUSE
  return fuse_main(argc, argv, &myfs_ops, NULL);
}
