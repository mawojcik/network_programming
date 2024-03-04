#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool printable_str(const char * buf) {
    int i = 0;
    while(buf[i] != '\0') {
        if (buf[i] < 32 || buf[i] > 126) {
            return 0;
        }
        i++;
    }
    return 1;
}

int main(void) {
    char buf[] = "abcddd";

    printf("%d", printable_str(buf));
}