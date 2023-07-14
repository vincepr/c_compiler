#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

// runs the command and pipes output into a string 
// ! (no real error-catching)
// - int inc_sterr =0 -> (errors are in place for our purpose)
char* runCmdToStr(char** cmd, int inc_stderr) {
    int stdout_fds[2];
    pipe(stdout_fds);

    int stderr_fds[2];
    if (!inc_stderr) {
        pipe(stderr_fds);
    }

    const pid_t pid = fork();
    if (!pid) {
        close(stdout_fds[0]);
        dup2(stdout_fds[1], 1);
        if (inc_stderr) {
            dup2(stdout_fds[1], 2);
        }

        close(stdout_fds[1]);

        if (!inc_stderr) {
            close(stderr_fds[0]);
            dup2(stderr_fds[1], 2);
            close(stderr_fds[1]);
        }

        execvp(*cmd, cmd);
        exit(0);
    }

    close(stdout_fds[1]);

    const int buf_size = 4096;
    char* out = malloc(buf_size);
    int out_size = buf_size;
    int i = 0;
    do {
        const ssize_t r = read(stdout_fds[0], &out[i], buf_size);
        if (r > 0) {
            i += r;
        }

        if (out_size - i <= 4096) {
            out_size *= 2;
            out = realloc(out, out_size);
        }
    } while (errno == EAGAIN || errno == EINTR);

    close(stdout_fds[0]);

    if (!inc_stderr) {
        close(stderr_fds[1]);
        do {
            const ssize_t r = read(stderr_fds[0], &out[i], buf_size);

            if (r > 0) {
                i += r;
            }

            if (out_size - i <= 4096) {
                out_size *= 2;
                out = realloc(out, out_size);
            }

        } while (errno == EAGAIN || errno == EINTR);

        close(stderr_fds[0]);
    }

    int r, status;
    do {
        r = waitpid(pid, &status, 0);
    } while (r == -1 && errno == EINTR);

    out[i] = 0;

    return out;
}

