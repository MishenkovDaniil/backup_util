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
#include "check_permission.h"
#include "processing.h"
#include "../shared/config.h"

void construct (struct Backup_args *backup_args, const char *work_fname, const char *backup_fname, const int mode) {
    assert (backup_args && work_fname && backup_fname);

    backup_args->work_fname_ = work_fname;
    backup_args->backup_fname_ = backup_fname;
    backup_args->mode_ = mode;
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

    char real_backup_fname[MAX_FILE_PATH] = "";
    char time[20] = "";
    char *work_basename = basename (backup_args->work_fname_);

    if (strlen (backup_args->backup_fname_) + strlen(work_basename) + 19 + 3 > MAX_FILE_PATH) { /// 3 string lens (strlen(time) == 19 always) + 2 '/' + '\0'
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
        char latest_fname[MAX_FILE_PATH] = "";
        sprintf (latest_fname, "%s/%s/%s", backup_args->backup_fname_, latest, work_basename);
        create_incremental_backup (real_backup_fname, backup_args->work_fname_, latest_fname);
    } else {
        cp_to_backup (real_backup_fname, backup_args->work_fname_, backup_args->mode_);
    }

    return 0;
}

int create_backup_info_file (const char *backup_path, const int mode, const char *time) {
    char buf[MAX_FILE_PATH] ="";
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

    static char work_buf[MAX_FILE_PATH] = "";
    static char backup_buf[MAX_FILE_PATH] = "";
    
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

    static char work_buf[MAX_FILE_PATH] = "";
    static char backup_buf[MAX_FILE_PATH] = "";
    static char latest_buf[MAX_FILE_PATH] = "";
    
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
