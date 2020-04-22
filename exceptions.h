#pragma once

enum exceptions {
    EX_ADD_DUPLICATE_NODE,
    EX_FILE_DUPLICATE_NODE,
    EX_FILE_OPEN_READ_ERROR,
    EX_FILE_OPEN_WRITE_ERROR,
    EX_SEM_TIMEOUT,
    EX_SEM_INVALID
};

static inline const char *exc2str(const exceptions &ex) {
    switch (ex) {
        case EX_ADD_DUPLICATE_NODE: return "EX_ADD_DUPLICATE_NODE";
        case EX_FILE_DUPLICATE_NODE: return "EX_FILE_DUPLICATE_NODE";
        case EX_FILE_OPEN_READ_ERROR: return "EX_FILE_OPEN_READ_ERROR";
        case EX_FILE_OPEN_WRITE_ERROR: return "EX_FILE_OPEN_WRITE_ERROR";
        case EX_SEM_TIMEOUT: return "EX_SEM_TIMEOUT";
        case EX_SEM_INVALID: return "EX_SEM_INVALID";
    }
    return "";
}
