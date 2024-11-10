#include <assert.h>
#include <dirent.h>
#include <stdio.h>

#include "check_permission.h"
#include "../shared/config.h"

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
    
    static char buf[MAX_FILE_PATH] = "";
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