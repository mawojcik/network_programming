#include <stdio.h>


void drukuj(int tablica[], int liczba_elementow) {
    printf("Numbers between 10 and 100\n");
    for (int i = 0; i < liczba_elementow; i ++) {
        if (tablica[i] > 10 && tablica[i] < 100) {
            printf("%d\n", tablica[i]);
        }
    }
}

int main() {
    int size = 10;
    int liczby[size];
    int input = 0;
    int i;
    for(i = 0; i < size; i++) {
        scanf("%d", &input);
        if (input == 0) {
            drukuj(liczby, i);
            return 0;
        } else {
            liczby[i] = input;
        }
    }
    drukuj(liczby, i);
    return 0;
}
