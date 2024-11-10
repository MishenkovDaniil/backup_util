#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "backup.h"
#include "../shared/config.h"

void construct (struct Backup_args *backup_args, const char *work_fname, const char *backup_fname, const int mode) {
    assert (backup_args && work_fname && backup_fname);

    backup_args->work_fname_ = work_fname;
    backup_args->backup_fname_ = backup_fname;
    backup_args->mode_ = mode;
}


int check_read_permission (const char *filename, struct stat *fs) {
    assert (filename && fs);

    int stat_status = stat (filename, fs);
    if (stat_status == -1) {
        fprintf (stderr, "Getting file %s stats error.\n", filename); 
        perror (""); //add advices 
        return ERROR_CODE;
    }

    return (fs->st_mode & S_IRUSR) > 0;
}

int check_read_permission_recursive_ (const char *filename, int *read_status) {
    assert (filename && read_status);
  
    struct stat st;

    *read_status |= check_read_permission (filename, &st);

    if (S_ISDIR (st.st_mode)) {
        check_read_permission_dir (filename, read_status);
    }   
}

int check_read_permission_dir (const char *dirname, int *read_status) {
    DIR *d = opendir (dirname);
    
    static char buf[1024] = "";
    size_t cur_len  = 0;
    static int is_start = 1;

    if (!d) {
        fprintf (stderr, "Error: cannot open dir %s", dirname);
        perror ("");
        return ERROR_CODE;
    }

    struct dirent *dirp;
    if (is_start) {
        sprintf (buf, "%s", dirname); ///buf = "work"
        is_start = 0;
    }
    cur_len = strlen (buf);

    while (dirp = readdir(d)) {
        if ((!strcmp (dirp->d_name, ".")) ||
            (!strcmp (dirp->d_name, "..")))
            continue;
                                                            /// buf = "work/dir1/.../dir<n>"
        sprintf (&buf[cur_len], "/%s", dirp->d_name);       /// add /<dir_name> or /<file_name> to existing path in buf

        struct stat st;
        *read_status |= check_read_permission (buf, &st);   /// get read status for curr name

        if (S_ISDIR (st.st_mode)) {                         /// if dir:
            check_read_permission_dir (buf, read_status);   /// get read status for all files in dir 
        }   
        buf[cur_len] = '\0';                                /// restore buf = "work/dir1/.../dir<n>" for future reading
    }    

    closedir (d);

    return 0;
}

int check_read_permission_recursive (const char *filename) {
    int read_status = 0;
    check_read_permission_recursive_ (filename, &read_status);
    return read_status;
}


int parse_args (const int argc, const char **argv, struct Backup_args *backup_args) {
    assert (argv && backup_args);
    // handle null values

    if (argc != 4) {
        fprintf (stderr, "Error: wrong input format...\n");
        return ERROR_CODE; 
    }
    int mode = -1;
    if (parse_mode (argv[1], &mode) == ERROR_CODE) {
        return ERROR_CODE;
    }
    construct (backup_args, argv[2], argv[3], mode);
    return 0;
}

int parse_mode (const char *input_mode, int *mode) {
    assert (input_mode && mode);
    
    if (!strcmp(input_mode, "full")) {
        *mode = Full;
    } else if (!strcmp(input_mode, "incremental")) {
        *mode = Incremental;
    } else  {
        fprintf (stderr, "Error: wrong backup mode...\n");
        return ERROR_CODE;
    }

    return 0;
}


int check_input (const char *work_fname, const char *backup_fname) {
    assert (work_fname && backup_fname);
    
    struct stat work_st;    
    struct stat backup_st;

    int status = 0;

    status = stat (work_fname, &work_st);
    if (status == -1) {
        fprintf (stderr, "Error: stat() failed, filename - %s.", work_fname);
        perror ("");
        return ERROR_CODE;
    }
    status |= stat (backup_fname, &backup_st);
    if (status == -1) {
        fprintf (stderr, "Error: stat() failed, filename - %s.", backup_fname);
        perror ("");
        return ERROR_CODE;
    }

    if (!(S_ISREG(work_st.st_mode) || S_ISDIR(work_st.st_mode))) {
        fprintf (stderr, "Error: source must be regular file or directory.\n");
        return ERROR_CODE;
    } else if (!S_ISDIR(backup_st.st_mode)) {
        fprintf (stderr, "Error: backup must be a directory.\n");
    }

    return 0;
}


int create_backup (struct Backup_args *backup_args) {
    assert (backup_args && backup_args->work_fname_ && backup_args->backup_fname_);
    
    if (check_input (backup_args->work_fname_, backup_args->backup_fname_) == ERROR_CODE) {
        return ERROR_CODE;
    }

    if (check_read_permission_recursive (backup_args->work_fname_) == ERROR_CODE) {
        fprintf (stderr, "Error:some files don't have permissions for reading, cannot copy them.\n");
        return ERROR_CODE;
    }

    char real_backup_fname[1024] = "";
    char time[20] = "";
    char *work_basename = basename (backup_args->work_fname_);

    if (strlen (backup_args->backup_fname_) + strlen(work_basename) + 19 + 3 > 1024) { /// 3 string lens (strlen(time) == 19 always) + 2 '/' + '\0'
        fprintf (stderr, "gg\n");
        return ERROR_CODE;
    }

    char latest[20] = "";
    if (backup_args->mode_ == Incremental) {
        get_latest_full_backup_basename (backup_args->backup_fname_, latest);
    }

    get_cur_date (time);
    printf ("%s\n", work_basename);

    sprintf (real_backup_fname, "%s/%s", backup_args->backup_fname_, time);
    mkdir (real_backup_fname, S_IRWXU | S_IRWXG | S_IRWXO);
    create_backup_info_file (real_backup_fname, backup_args->mode_, time);
    sprintf (&real_backup_fname[strlen(real_backup_fname)], "/%s", work_basename);

    if (strlen(latest)) {
        char latest_fname[1024] = "";
        sprintf (latest_fname, "%s/%s/%s", backup_args->backup_fname_, latest, work_basename);
        create_incremental_backup (real_backup_fname, backup_args->work_fname_, latest_fname);
    } else {
        cp_to_backup (real_backup_fname, backup_args->work_fname_, backup_args->mode_);
    }

    return 0;
}

int create_backup_info_file (const char *backup_path, const int mode, const char *time) {
    char buf[1024] ="";
    sprintf (buf, "%s/%s", backup_path, backup_info_fname);

    int fd = open (buf, O_CREAT | O_WRONLY, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd == - 1) {
        perror ("Error:cannot create backup info file.\n");
        return ERROR_CODE;
    }

    sprintf (buf, "mode = %d.\ntime = %s.\n", mode, time);

    write (fd, buf, strlen(buf));

    close (fd);
    return 0;
}

int cp_to_backup (const char *backup_fname, const char * work_fname, const int mode) {
    int cp_status  = 0;
    
    cp_to_backup_recursive (backup_fname, work_fname, mode, &cp_status);

    return cp_status;
}

int create_incremental_backup (const char *backup_fname, const char *work_fname, const char *latest_fname){
    int cp_status  = 0;
    
    cp_to_backup_incremental (backup_fname, work_fname, latest_fname, &cp_status);

    return cp_status;
}

void cp_to_backup_incremental (const char *backup_fname, const char *work_fname, const char *latest_fname, int *cp_status) {
    struct stat st;
    int status = stat (work_fname, &st);
    if (S_ISDIR (st.st_mode)) {
        // if (is_modified(work_fname, latest_fname)) {
            mkdir (backup_fname, S_IRWXU | S_IRWXG | S_IRWXO);
            creat_n_cp_dir_incr (work_fname, backup_fname, latest_fname, cp_status);
        // }
    } else {
        if (is_modified(work_fname, latest_fname)) {
            creat_n_cp_file (work_fname, backup_fname);
        }
    }
}

void cp_to_backup_recursive (const char *backup_fname, const char *work_fname, const int mode, int *cp_status) {
    struct stat st;
    int status = stat (work_fname, &st);

    if (S_ISDIR (st.st_mode)) {
        mkdir (backup_fname, S_IRWXU | S_IRWXG | S_IRWXO);
        creat_n_cp_dir (work_fname, backup_fname, mode, cp_status);
    } else {
        creat_n_cp_file (work_fname, backup_fname);
    }
}

int is_modified (const char *work_path, const char *backup_path) {
    struct stat work_st;
    struct stat backup_st;

    stat (work_path, &work_st);
    int status = stat (backup_path, &backup_st);
    if (status == -1 && errno == ENOENT) {
        return 1;
    } else if (status == -1){
        fprintf (stderr, "error %s, %s\n", work_path, backup_path); 
        perror ("");
        return 1;
    }

    return (work_st.st_mtime > backup_st.st_mtime);
}

int creat_n_cp_file (const char *src_path, const char *dst_path) {
    static char buf[8096];
    
    struct stat src_st;
    int status = stat (src_path, &src_st);
    __uint64_t size = src_st.st_size;

    int src_fd = open (src_path, O_RDONLY);
    int dst_fd = open (dst_path, O_CREAT | O_WRONLY, src_st.st_mode | S_IWUSR); /// S_IWUSR to guarantee writing opportunity

    if (size > 8096) {
        char *whole_buf = (char *)malloc (sizeof(char) * size);
        assert (whole_buf);

        read(src_fd, whole_buf, size);
        write (dst_fd, whole_buf, size);

        free (whole_buf);
    } else {
        read(src_fd, buf, size);
        write (dst_fd, buf, size);
    }

    close (src_fd);
    close (dst_fd);

    return 0;
}

int creat_n_cp_dir (const char *work_dirname, const char *backup_dirname, const int mode, int *cp_status) {
    DIR *work_d = opendir (work_dirname);
    DIR *backup_d = opendir (backup_dirname);

    static char work_buf[1024] = "";
    static char backup_buf[1024] = "";
    
    __uint64_t work_cur_len  = 0;
    __uint64_t backup_cur_len  = 0;
    static int is_start = 1;

    if (!work_d) {
        fprintf (stderr, "Error: cannot open dir %s", work_d);
        perror ("");
        return ERROR_CODE;
    }
    if (!backup_d) {
        fprintf (stderr, "Error: cannot open dir %s", backup_d);
        perror ("");
        return ERROR_CODE;
    }

    struct dirent *work_dirp;
    struct dirent *backup_dirp;

    if (is_start) {
        sprintf (work_buf, "%s", work_dirname); 
        sprintf (backup_buf, "%s", backup_dirname); 
        is_start = 0;
    }

    work_cur_len = strlen (work_buf);
    backup_cur_len = strlen (backup_buf);

    while (work_dirp = readdir(work_d)) {
        if ((!strcmp (work_dirp->d_name, ".")) ||
            (!strcmp (work_dirp->d_name, "..")))
            continue;

        sprintf (&work_buf[work_cur_len], "/%s", work_dirp->d_name);       
        sprintf (&backup_buf[backup_cur_len], "/%s", work_dirp->d_name);       
        
        struct stat st;
        int status = stat (work_buf, &st); // 
        
        if (S_ISDIR(st.st_mode)) {
            mkdir (backup_buf, st.st_mode | S_IWUSR); //
            creat_n_cp_dir (work_buf, backup_buf, mode, cp_status);
        } else {
            *cp_status |= creat_n_cp_file (work_buf, backup_buf);  
        }

        work_buf[work_cur_len] = '\0';                               
        backup_buf[backup_cur_len] = '\0';                               
    }    

    closedir (backup_d);
    closedir (work_d);

    return 0;
}

int creat_n_cp_dir_incr (const char *work_dirname, const char *backup_dirname, const char *latest_dirname, int *cp_status) {
    DIR *work_d = opendir (work_dirname);
    DIR *backup_d = opendir (backup_dirname);

    static char work_buf[1024] = "";
    static char backup_buf[1024] = "";
    static char latest_buf[1024] = "";
    
    __uint64_t work_cur_len  = 0;
    __uint64_t backup_cur_len  = 0;
    __uint64_t latest_cur_len  = 0;
    static int is_start = 1;

    if (!work_d) {
        fprintf (stderr, "Error: cannot open dir %s", work_d);
        perror ("");
        return ERROR_CODE;
    }
    if (!backup_d) {
        fprintf (stderr, "Error: cannot open dir %s", backup_d);
        perror ("");
        return ERROR_CODE;
    }

    struct dirent *work_dirp;

    if (is_start) {
        sprintf (work_buf, "%s", work_dirname); 
        sprintf (backup_buf, "%s", backup_dirname); 
        sprintf (latest_buf, "%s", latest_dirname); 
        is_start = 0;
    }

    work_cur_len    = strlen (work_buf);
    backup_cur_len  = strlen (backup_buf);
    latest_cur_len  = strlen (latest_buf);

    while ((work_dirp = readdir(work_d))) {
        if ((!strcmp (work_dirp->d_name, ".")) ||
            (!strcmp (work_dirp->d_name, "..")))
            continue;

        sprintf (&work_buf[work_cur_len], "/%s", work_dirp->d_name);       
        sprintf (&backup_buf[backup_cur_len], "/%s", work_dirp->d_name);       
        sprintf (&latest_buf[latest_cur_len], "/%s", work_dirp->d_name);       
        
        struct stat st;
        int status = stat (work_buf, &st); // 
        
        if (S_ISDIR(st.st_mode)) {
            mkdir (backup_buf, st.st_mode | S_IWUSR); //
            creat_n_cp_dir_incr (work_buf, backup_buf, latest_buf, cp_status);
        } else {
            if (!is_modified(work_buf, latest_buf)) 
                continue;
            *cp_status |= creat_n_cp_file (work_buf, backup_buf);  
        }

        work_buf[work_cur_len] = '\0';                               
        backup_buf[backup_cur_len] = '\0';                               
        latest_buf[latest_cur_len] = '\0';                               
    }    

    closedir (backup_d);
    closedir (work_d);

    return 0;
}

int get_latest_full_backup_basename (const char *backup_path, char *latest_basename) {
    assert (backup_path && latest_basename);

    DIR *backup_dir = opendir (backup_path);

    char buf[1024] = "";

    sprintf (buf, "%s", backup_path);
    __uint64_t len = strlen (buf);

    struct dirent *backup_dirp;
    int is_start = 1;
    char best_time[20] = "";

    while (backup_dirp = readdir(backup_dir)) {
        if ((!strcmp (backup_dirp->d_name, ".")) ||
            (!strcmp (backup_dirp->d_name, "..")))
            continue;
        sprintf (&buf[len], "/%s/%s", backup_dirp->d_name, backup_info_fname);
        if (get_backup_mode (buf) == Full) {
            if (is_start) {
                init_backup_time (buf, best_time);
                is_start = 0;
            } else {
                char temp[20] = "";
                init_backup_time (buf, temp);
                if (cmp_time (best_time, temp) < 0) {
                    strcpy (best_time, temp);
                }
            }
        }
    }
    strcpy (latest_basename, best_time);
    closedir (backup_dir);
    return 0;
}

int get_backup_mode (const char *info_file_path) {
    assert (info_file_path);

    FILE *file = fopen (info_file_path, "r");
    int mode = None;
    fscanf (file, "mode = %d.", &mode);
    fclose (file);
    
    return mode;
}

int init_backup_time (const char *info_file_path, char *time) {
    assert (info_file_path && time);

    FILE *file = fopen (info_file_path, "r");
    
    fscanf (file, "%*[^\n]\n%*[^=]= %[^.]", time);
    fclose (file);

    return 0;
}

int cmp_time (char *time1, char *time2) {
    static int year1 = 0;
    static int year2 = 0;

    static int mon1 = 0;
    static int mon2 = 0;

    static int day1 = 0;
    static int day2 = 0;

    static int hour1 = 0;
    static int hour2 = 0;

    static int min1 = 0;
    static int min2 = 0;

    static int sec1 = 0;
    static int sec2 = 0;

    sscanf (time1, "%d-%d-%d_%d-%d-%d", &year1, &mon1, &day1, &hour1, &min1, &sec1);
    sscanf (time2, "%d-%d-%d_%d-%d-%d", &year2, &mon2, &day2, &hour2, &min2, &sec2);

    __uint64_t date1 =  (day1) + (mon1) * 100 +  (year1) * 10000;
    __uint64_t date2 =  (day2) + (mon2) * 100 +  (year2) * 10000;

    __uint64_t clock1 =  (sec1) + (min1) * 100 +  (hour1) * 10000;
    __uint64_t clock2 =  (sec2) + (min2) * 100 +  (hour2) * 10000;

    if (date1 == date2) {
        return clock1 - clock2;
    }
    return date1 - date2;
}

int get_cur_date (char *time_str) {
    assert (time_str);

    char year   [5] = "";
    char month  [3] = "";
    char day    [3] = "";
    char hour   [3] = "";
    char minute [3] = "";
    char second [3] = "";
    
    time_t result = time(NULL);
   
    if (result == (time_t)-1) {
        return ERROR_CODE;
    }
   
    struct tm *date = localtime(&result);

    itoa(date->tm_year + 1900, year);
    itoa(date->tm_mon, month);
    itoa(date->tm_mday, day);
    itoa(date->tm_hour, hour);
    itoa(date->tm_min, minute);
    itoa(date->tm_sec, second);
    
    sprintf (time_str, "%s-%s-%s_%s-%s-%s",year, month, day, hour, minute, second); ///YYYY-MM-DD_HH-MM-SS time format

    return 0;
}

void itoa (int num, char *str) {
    assert(str);

    int sign = 0;

    if ((sign = num) < 0)  {
        num = -num;          
    } 

    int idx = 0;
    for (; num > 0; ++idx, num /= 10) {
        str[idx] = '0' + num % 10;
    }

    if (sign < 0) {
        str[idx++] = '-';
    }

    str[idx] = '\0';

    reverse(str);
}

void reverse (char *str) {
    for (int i = 0, j = strlen(str) - 1; i < j; ++i, --j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}