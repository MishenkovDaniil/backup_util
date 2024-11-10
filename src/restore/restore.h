#ifndef RESTORE_H
#define RESTORE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../shared/config.h"

struct Restore_args {
    char *work_fname_;
    char *backup_fname_;
};

void construct (struct Restore_args *restore_args, const char *work_fname, const char *backup_fname);

int parse_args (const int argc, const char **argv, struct Restore_args *restore_args);

int check_write_permission (const char *filename, struct stat *fs);

int restore (const struct Restore_args *restore_args);
int restore_backup (const char *backup_fname, const char * work_fname);
void restore_recursive (const char *backup_fname, const char * work_fname, int *restore_status);
int restore_dir (const char *work_dirname, const char *backup_dirname, int *restore_status);
int restore_file (const char *src_path, const char *dst_path); /// src - backup file, dst - work file


#endif /*RESTORE_H*/