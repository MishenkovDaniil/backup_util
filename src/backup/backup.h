#ifndef BACKUP_H
#define BACKUP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"

struct Backup_args {
    char *work_fname_;
    char *backup_fname_;
    int mode_;
};

void construct (struct Backup_args *backup_args, const char *work_fname, const char *backup_fname, const int mode);

int check_read_permission (const char *filename, struct stat *fs);
int check_read_permission_recursive (const char *filename); /// same as check_read_permission() if reg file, for dir check dir permissions and for all what in dir
int check_read_permission_recursive_ (const char *filename, int *read_status);
int check_read_permission_dir (const char *filename, int *read_status);

int parse_args (const int argc, const char **argv, struct Backup_args *backup_args);
int parse_mode (const char *input_mode, int *mode);

int check_input (const char *work_fname, const char *backup_fname);
int create_backup (struct Backup_args *backup_args);

int cp_to_backup (const char *backup_fname, const char * work_fname, const int mode);
int get_time (char *time);


#endif /*BACKUP_H*/

