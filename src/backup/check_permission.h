#ifndef CHECK_PERMISSION_H
#define CHECK_PERMISSION_H

#include <sys/stat.h>

#include "backup.h"

int check_read_permission (const char *filename, struct stat *fs);
int check_read_permission_recursive (const char *filename); /// same as check_read_permission() if reg file, for dir check dir permissions and for all what in dir
int check_read_permission_recursive_ (const char *filename, int *read_status);
int check_read_permission_dir (const char *filename, int *read_status);

#endif /*CHECK_PERMISSION_H*/