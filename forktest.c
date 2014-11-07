#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spawn.h"


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
    process = spawn_basic("echo", echo_args, true);
    if (process.pid < 0) goto onerror;
    waitpid(process.pid, &status, 0);

    // connect stdout to file
    //process = spawn("echo", echo_args, 0, true);
    

    // stdin only
    process = spawn_pipe("cat", cat_args, SPAWN_PIPE_STDIN, true);
    if (process.pid < 0) goto onerror;
    write(process.fd_stdin, writestr, strlen(writestr));
    close(process.fd_stdin);
    waitpid(process.pid, &status, 0);

    // stdout only
    process = spawn_pipe("ls", ls_args, SPAWN_PIPE_STDOUT, true);
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

    // redirect stdout
    {
        int fd = open("./test-redirect.txt", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        process = spawn("ls", ls_args, SPAWN_REDIRECT_STDOUT, true, 0, fd, 0);
        waitpid(process.pid, &status, 0);
        close(fd);
    }

    // stdin/stdout
    process = spawn_pipe("./simple-echo/simple-echo", simple_echo_args, SPAWN_PIPE_STDOUT|SPAWN_PIPE_STDIN, true);
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

