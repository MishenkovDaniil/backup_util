#include <stdio.h>

#include "backup.h"
#include "processing.h"
#include "../shared/config.h"

int main(const int argc, const char *argv[])
{
    struct Backup_args backup_args;
    int status = 0;

    status = parse_args (argc, argv, &backup_args);
    if (status == ERROR_CODE) {
        return status;
    }

    status = create_backup (&backup_args);
    return status;
}

