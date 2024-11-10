#include "backup.h"
#include "config.h"
#include <assert.h>
#include <string.h>
#include <dirent.h>


void construct (struct Backup_args *backup_args, const char *work_fname, const char *backup_fname, const int mode) {
    assert (backup_args && work_fname && backup_fname);

    backup_args->work_fname_ = work_fname;
    backup_args->backup_fname_ = backup_fname;
    backup_args->mode_ = mode;
}

int check_read_permission (const char *filename, struct stat *fs) {
    assert (filename && fs);
    // printf ("[%s]\n", filename);

    int stat_status = stat (filename, fs);
    if (stat_status == -1) {
        perror ("Getting file stats error"); //add advices 
        return ERROR_CODE;
    }
    
    // printf ("[%s] -- %d\n", filename, (fs->st_mode & S_IRUSR) > 0);

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
    // printf ("[%s]\n", argv[0]);
    // printf ("[%s]\n", argv[1]);
    // printf ("[%s]\n", argv[2]);
    // printf ("[%s]\n", argv[3]);
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
    status |= stat (backup_fname, &backup_st);

    if (status == -1) {
        perror ("Error: stat() failed");
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

    char real_backup_fname[1024] ="";
    char time[20] = "";

    if (strlen (backup_args->backup_fname_) + 20 > 1024) {
        fprintf (stderr, "gg\n");
        return ERROR_CODE;
    }

    get_cur_date (time);

    sprintf (real_backup_fname, "%s/%s", backup_args->backup_fname_, time);
    mkdir (real_backup_fname, 0x777);

    cp_to_backup (real_backup_fname, backup_args->work_fname_, backup_args->mode_);

    return 0;
}

int cp_to_backup (const char *backup_fname, const char * work_fname, const int mode) {
    return 0;
}
int get_time (char *time) {
    return 0;
}