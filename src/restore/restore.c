#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "restore.h"
#include "../shared/config.h"

void construct (struct Restore_args *restore_args, const char *work_fname, const char *backup_fname) {
    assert (restore_args && work_fname && backup_fname);

    restore_args->work_fname_ = work_fname;
    restore_args->backup_fname_ = backup_fname;
}

int parse_args (const int argc, const char **argv, struct Restore_args *restore_args) {
    assert (argv && restore_args);
    // handle null values

    if (argc != 3) {
        fprintf (stderr, "Error: wrong input format...\n");
        return ERROR_CODE; 
    }
    
    construct (restore_args, argv[1], argv[2]);
    return 0;
}

int check_write_permission (const char *filename, struct stat *fs) {
    assert (filename && fs);

    int stat_status = stat (filename, fs);
    if (stat_status == -1) {
        fprintf (stderr, "Getting file %s stats error.\n", filename); 
        perror (""); //add advices 
        return ERROR_CODE;
    }

    return (fs->st_mode & S_IWUSR) > 0;
}

int restore (const struct Restore_args *restore_args) {
    assert (restore_args);

    char backup_base_dirname[1024] = "";
    char *work_basename = basename (restore_args->work_fname_);

    if (strlen(restore_args->backup_fname_) + strlen(work_basename) + 2 > 1024) {
        fprintf (stderr, "gg\n");
        return ERROR_CODE;
    }

    sprintf (backup_base_dirname, "%s/%s", restore_args->backup_fname_, work_basename);

    restore_backup (backup_base_dirname, restore_args->work_fname_);
    return 0;
}

int restore_backup (const char *backup_fname, const char *work_fname) {
    assert (backup_fname && work_fname);
    
    int restore_status  = 0;
    // printf ("backup_fname = %s\n", backup_fname);
    // printf ("work_fname = %s\n", work_fname);
    
    restore_recursive (backup_fname, work_fname, &restore_status);

    return restore_status;
}

void restore_recursive (const char *backup_fname, const char *work_fname, int *restore_status) {
    assert (backup_fname && work_fname && restore_status);

    struct stat st;
    int status = stat (backup_fname, &st);

    if (S_ISDIR (st.st_mode)) {
        // printf ("go dir\n");
        // mkdir (work_fname, S_IRWXU | S_IRWXG | S_IRWXO);
        restore_dir (work_fname, backup_fname, restore_status);
    } else {
        // printf ("go file\n");
        *restore_status = restore_file (work_fname, backup_fname);
    }
}

int restore_file (const char *src_path, const char *dst_path) { /// src - backup file, dst - work file
    static char buf[8096];
    
    struct stat src_st;
    int status = stat (src_path, &src_st);
    __uint64_t size = src_st.st_size;

    int src_fd = open (src_path, O_RDONLY);
    if (src_fd == -1) {
        fprintf ("Error: open() failed with file %s.\n", src_path);
        perror ("");
        return ERROR_CODE;
    }

    int dst_fd = open (dst_path, O_CREAT | O_WRONLY, src_st.st_mode | S_IWUSR); /// S_IWUSR to guarantee writing opportunity
    if (dst_fd == -1) {
        if (errno == EACCES) {
            printf ("No write permissions for file %s.\n", dst_path);
            close (src_fd);
            return 0;
        } else {
            fprintf ("Error: open() failed with file %s.\n", src_path);
            perror ("");
            close (src_fd);
            return ERROR_CODE;
        }
    }
    
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

int restore_dir (const char *work_dirname, const char *backup_dirname, int *restore_status) {
    DIR *work_d = opendir (work_dirname);
    DIR *backup_d = opendir (backup_dirname);
    // printf ("work_dirname = %s\n", work_dirname);
    // printf ("backup_dirname = %s\n", backup_dirname);

    static char work_buf[1024] = "";
    static char backup_buf[1024] = "";
    
    __uint64_t work_cur_len  = 0;
    __uint64_t backup_cur_len  = 0;
    static int is_start = 1;

    if (!work_d) {
        if (errno == ENOENT) {
            struct stat st;
            int status = stat (backup_dirname, &st);
            mkdir (work_dirname, st.st_mode | S_IWUSR);
        } else {
            fprintf (stderr, "Error: cannot open dir %s", work_dirname);
            perror ("");
            return ERROR_CODE;
        }
    }
    if (!backup_d) {
        fprintf (stderr, "Error: cannot open dir %s", backup_dirname);
        perror ("");
        return ERROR_CODE;
    }
    {
        struct stat st; 
        if (check_write_permission (work_dirname, &st) == 0) {
            printf ("No write permissions for directory %s.\n", work_dirname);
            return 0;
        }
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

    while (backup_dirp = readdir(backup_d)) {
        if ((!strcmp (backup_dirp->d_name, ".")) ||
            (!strcmp (backup_dirp->d_name, "..")))
            continue;

        sprintf (&work_buf[work_cur_len], "/%s", backup_dirp->d_name);       
        sprintf (&backup_buf[backup_cur_len], "/%s", backup_dirp->d_name);       
 
        // printf ("work_buf = %s\n", work_buf);
        // printf ("backup_buf = %s\n", backup_buf);
 
        struct stat st;
        int status = stat (backup_buf, &st); // 
    
        if (S_ISDIR(st.st_mode)) {
            // mkdir (backup_buf, st.st_mode | S_IWUSR); //
            restore_dir (work_buf, backup_buf, restore_status);
        } else {
            *restore_status |= restore_file (backup_buf, work_buf);  
        }

        work_buf[work_cur_len] = '\0';                               
        backup_buf[backup_cur_len] = '\0';                               
    }    

    closedir (backup_d);
    closedir (work_d);

    return 0;
}

