#include <unistd.h>
#include <cerrno>
#include <cstring>

void myPerror(const char* msg) {
    const char* error_str;

    switch (errno) {
        case EPIPE:
            error_str = "Broken pipe\n";
            break;
        case EINVAL:
            error_str = "Invalid argument\n";
            break;
        case ENOMEM:
            error_str = "Out of memory\n";
            break;
        case EAGAIN:
            error_str = "Try again\n";
            break;
        case EBADF:
            error_str = "Bad file descriptor\n";
            break;
        case ECHILD:
            error_str = "No child processes\n";
            break;
        case EINTR:
            error_str = "Interrupted system call\n";
            break;
        case EIO:
            error_str = "Input/output error\n";
            break;
        default:
            error_str = "Unknown error\n";
            break;
    }
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, ": ", 2);
    write(STDERR_FILENO, error_str, strlen(error_str));
}
