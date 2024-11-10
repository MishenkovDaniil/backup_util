#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "processing.h"
#include "../shared/config.h"

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

int get_latest_full_backup_basename (const char *backup_path, char *latest_basename) {
    assert (backup_path && latest_basename);

    DIR *backup_dir = opendir (backup_path);

    char buf[MAX_FILE_PATH] = "";

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