#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Wrong number of arguments!");
        return 0;
    }

    int fd_input = open(argv[1], O_RDONLY);
    int fd_output = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);

    if (fd_input == -1 || fd_output == -1) {
        printf("Error numbers: input: %d, output: %d", fd_input, fd_output);
        return 0;
    }

    int read_status;
    char buf[10];

    while ((read_status = read(fd_input, buf, 10))> 0) {
        if (write(fd_output, buf, read_status) == -1) {
            printf("Problem with writing\n");
            return 0;
        }
    }

    if (close(fd_input) == -1) {
        printf("Problem with closing input");
    }
    if (close(fd_output) == -1) {
        printf("Problem with closing output");
    }
}
