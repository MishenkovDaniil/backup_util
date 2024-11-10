#ifndef BACKUP_H
#define BACKUP_H

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../shared/config.h"

static const char *backup_info_fname = "backup_info.txt";
struct Backup_args {
    char *work_fname_;
    char *backup_fname_;
    int mode_;
};

void construct (struct Backup_args *backup_args, const char *work_fname, const char *backup_fname, const int mode);

int create_backup (struct Backup_args *backup_args);
int create_backup_info_file (const char *backup_path, const int mode, const char *time);

int cp_to_backup (const char *backup_fname, const char * work_fname, const int mode);
void cp_to_backup_recursive (const char *backup_fname, const char * work_fname, const int mode, int *cp_status);
int creat_n_cp_dir (const char *work_dirname, const char *backup_dirname, const int mode, int *cp_status);

int create_incremental_backup (const char *backup_fname, const char * work_fname, const char *latest);
void cp_to_backup_incremental (const char *backup_fname, const char *work_fname, const char *latest_fname, int *cp_status);
int creat_n_cp_dir_incr (const char *work_dirname, const char *backup_dirname, const char *latest_dirname, int *cp_status);

int creat_n_cp_file (const char *src_path, const char *dst_path);




#endif /*BACKUP_H*/

