#ifndef APPFLID_KFILE_H
#define APPFLID_KFILE_H

#include <linux/fs.h>
int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) ;
struct file* file_open(const char* path, int flags, int rights);
void file_close(struct file* file);

#endif
