#ifndef PROCESSING_H
#define PROCESSING_H

#include "backup.h"

int parse_args (const int argc, const char **argv, struct Backup_args *backup_args);
int parse_mode (const char *input_mode, int *mode);

int check_input (const char *work_fname, const char *backup_fname);

int is_modified (const char *work_path, const char *backup_path);

int get_latest_full_backup_basename (const char *backup_path, char *latest_basename);

int init_backup_time (const char *info_file_path, char *time);
int get_backup_mode (const char *info_file_path);
int cmp_time (char *time1, char *time2);

int get_cur_date (char *time);
void itoa (int num, char *str);
void reverse (char *str);

#endif /*PROCESSING_H*/