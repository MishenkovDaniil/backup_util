#include <stdio.h>

#include "../shared/config.h"
#include "restore.h"

int main (const int argc, const char **argv) {
    struct Restore_args restore_args;
    int status = 0;

    status = parse_args (argc, argv, &restore_args);
    if (status == ERROR_CODE) {
        return ERROR_CODE;
    }

    status = restore (&restore_args);

    return status;
}