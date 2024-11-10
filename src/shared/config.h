#ifndef CONFIG_H
#define CONFIG_H

#define MAX_FILE_PATH 1024

static const int ERROR_CODE = -1;
static const __uint64_t MAX_PATH_LEN = 1024;
enum  Mode {
    None,
    Full,
    Incremental 
};

#endif /*CONFIG_H*/