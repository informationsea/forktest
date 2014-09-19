#include <stdio.h>
#include <unistd.h>

int main()
{
    char buf[1024*10];
    ssize_t bytes;

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bytes);
    }

    if (bytes < 0)
        perror("Fail");
    
    return 0;
}
