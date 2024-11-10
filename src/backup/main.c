#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backup.h"
#include "config.h"

int main(const int argc, const char *argv[])
{
    struct Backup_args backup_args;

    parse_args (argc, argv, &backup_args);
    // printf ("%s\n", backup_args.work_fname_);
    // printf ("%s\n", backup_args.backup_fname_);
    // printf ("%d\n", backup_args.mode_);
    int status = create_backup (&backup_args);

    return status;
}

