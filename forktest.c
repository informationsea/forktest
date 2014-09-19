#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

typedef struct child_process_tag
{
    int fd_stdin;
    int fd_stdout;
    int fd_stderr;
    pid_t pid;
} child_process_t;

#define SPWAN_PIPE_STDIN  1
#define SPWAN_PIPE_STDOUT 2
#define SPWAN_PIPE_STDERR 4

child_process_t spwan(const char *file, char *argv[], int pipe_flags, bool usepath);

int main(void)
{
    char *echo_args[] = {"echo", "hello", "world", "!!", NULL};
    char *cat_args[] = {"cat", NULL};
    char *ls_args[] = {"ls", "-lha", NULL};
    char *simple_echo_args[] = {"simple-echo", NULL};
    const char *writestr = "TEST STRING\n";
    char buf[1024*10];
    char *buf_pointer;
    child_process_t process;
    int status;
    bool should_write = false;
    int i, j;

    // no pipe
    process = spwan("echo", echo_args, 0, true);
    if (process.pid < 0) goto onerror;
    waitpid(process.pid, &status, 0);

    // stdin only
    process = spwan("cat", cat_args, SPWAN_PIPE_STDIN, true);
    if (process.pid < 0) goto onerror;
    write(process.fd_stdin, writestr, strlen(writestr));
    close(process.fd_stdin);
    waitpid(process.pid, &status, 0);

    // stdout only
    process = spwan("ls", ls_args, SPWAN_PIPE_STDOUT, true);
    if (process.pid < 0) goto onerror;
    buf_pointer = buf;
    bzero(buf, sizeof(buf));
    while (1) {
        ssize_t bytes = read(process.fd_stdout, buf_pointer, sizeof(buf));
        if (bytes > 0) {
            buf_pointer += bytes;
        } else {
            if (bytes < 0) {
                perror("Failed to read pipe");
            }
            break;
        }
    }
    waitpid(process.pid, &status, 0);
    printf("RESULT START\n%s\nEND\n", buf);

    // stdin/stdout
    process = spwan("./simple-echo/simple-echo", simple_echo_args, SPWAN_PIPE_STDOUT|SPWAN_PIPE_STDIN, true);
    if (process.pid < 0) goto onerror;
    buf_pointer = buf;
    bzero(buf, sizeof(buf));
    should_write = true;
    i = 0;
    while (1) {
        if (should_write) {
            if (i >= 4) {
                close(process.fd_stdin);
                break;
            } else {
                write(process.fd_stdin, writestr, strlen(writestr));
                i += 1;
            }
        }
        
        ssize_t bytes = read(process.fd_stdout, buf_pointer, sizeof(buf) - (buf_pointer - buf));
        if (bytes > 0) {
            //fprintf(stderr, "READING[%s]\n", buf_pointer);
            for (j = 0; j < bytes; j++) {
                if (buf_pointer[j] == '\n') {
                    printf("ONE LINE %zd: [%s]\n", bytes, buf);
                    buf_pointer = buf;
                    bzero(buf, sizeof(buf));
                    should_write = true;
                    goto continue2;
                }
            }
            buf_pointer += bytes;
        } else {
            if (bytes < 0) {
                perror("Failed to read pipe");
            }
            break;
        }
      continue2:
        continue;
    }
    waitpid(process.pid, &status, 0);
    
    
    return status;

  onerror:
    perror("Failed to start process");
    return 2;
}

child_process_t spwan(const char *file, char *argv[], int pipe_flags, bool usepath)
{
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
    child_process_t process;
    bzero(&process, sizeof(process));

    if (pipe_flags & SPWAN_PIPE_STDIN) {
        pipe(stdin_pipe);
    }
    if (pipe_flags & SPWAN_PIPE_STDOUT) {
        pipe(stdout_pipe);
    }
    if (pipe_flags & SPWAN_PIPE_STDERR) {
        pipe(stderr_pipe);
    }
    
    process.pid = fork();
    if (process.pid < 0)
        return process;

    if (process.pid > 0) {
        if (pipe_flags & SPWAN_PIPE_STDIN) {
            process.fd_stdin = stdin_pipe[1];
            close(stdin_pipe[0]);
        }
        
        if (pipe_flags & SPWAN_PIPE_STDOUT) {
            process.fd_stdout = stdout_pipe[0];
            close(stdout_pipe[1]);
        }
        
        if (pipe_flags & SPWAN_PIPE_STDERR) {
            process.fd_stderr = stderr_pipe[0];
            close(stderr_pipe[1]);
        }
        
        return process;
    }

    if (pipe_flags & SPWAN_PIPE_STDIN) {
        close(STDIN_FILENO);
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[1]);
    }

    if (pipe_flags & SPWAN_PIPE_STDOUT) {
        close(STDOUT_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[0]);
    }

    if (pipe_flags & SPWAN_PIPE_STDERR) {
        close(STDERR_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stderr_pipe[0]);
    }

    if (usepath)
        execvp(file, argv);
    execv(file, argv);
    return process; // never run
}
