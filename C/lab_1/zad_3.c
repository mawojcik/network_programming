#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool printable_buf(const void * buf, int len) {
    char *buf_ptr = (char *) buf;
    for (int i = 0; i < len; i++) {
        if (buf_ptr[i] < 32 || buf_ptr[i] > 126) {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    char buf[] = "abcddd";
    int len = strlen(buf);

    printf("%d", printable_buf(buf, len));
    return 0;
}
