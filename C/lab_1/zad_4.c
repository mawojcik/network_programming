#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool printable_str(const char * buf) {
    while(*buf != '\0') {
        if (*buf < 32 || *buf > 126) {
            return 0;
        }
        buf++;
    }
    return 1;
}

int main(void) {
    char buf[] = "abcd£dd";

    printf("%d", printable_str(buf));
}
