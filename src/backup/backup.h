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

int check_read_permission (const char *filename, struct stat *fs);
int check_read_permission_recursive (const char *filename); /// same as check_read_permission() if reg file, for dir check dir permissions and for all what in dir
int check_read_permission_recursive_ (const char *filename, int *read_status);
int check_read_permission_dir (const char *filename, int *read_status);

int parse_args (const int argc, const char **argv, struct Backup_args *backup_args);
int parse_mode (const char *input_mode, int *mode);

int check_input (const char *work_fname, const char *backup_fname);
int create_backup (struct Backup_args *backup_args);

int create_incremental_backup (const char *backup_fname, const char * work_fname, const char *latest);

int cp_to_backup (const char *backup_fname, const char * work_fname, const int mode);
void cp_to_backup_recursive (const char *backup_fname, const char * work_fname, const int mode, int *cp_status);
int creat_n_cp_dir (const char *work_dirname, const char *backup_dirname, const int mode, int *cp_status);
int creat_n_cp_file (const char *src_path, const char *dst_path);
int create_backup_info_file (const char *backup_path, const int mode, const char *time);

int cmp_time (char *time1, char *time2);
int init_backup_time (const char *info_file_path, char *time);
int get_backup_mode (const char *info_file_path);

int get_cur_date (char *time);
void itoa(int num, char *str);
void reverse(char *str);

#endif /*BACKUP_H*/

