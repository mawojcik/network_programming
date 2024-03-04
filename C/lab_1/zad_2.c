#include <stdio.h>


void drukuj_alt(int * tablica, int liczba_elementow) {
    int *ptr = &tablica[0];
    printf("Numbers between 10 and 100\n");
    for (int i = 0; i < liczba_elementow; i ++) {
        if (*ptr > 10 && *ptr < 100) {
            printf("%d\n", *ptr);
        }
        ptr++;
    }
}

int main(void) {
    int size = 10;
    int liczby[size];
    int input = 0;
    int i;
    for(i = 0; i < size; i++) {
        scanf("%d", &input);
        if (input == 0) {
            drukuj_alt(liczby, i);
            return 0;
        } else {
            liczby[i] = input;
        }
    }
    drukuj_alt(liczby, i);
    return 0;
}
