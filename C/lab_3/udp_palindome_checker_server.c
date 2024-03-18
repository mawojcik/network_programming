#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

int serwer;

// Funkcja własnej obsługi sygnału
void koniec(void) {
    // Zamknięcie połączenia
    if (close(serwer) == -1) {
        perror("close error");
        exit(1);
    }
}

int isPalindome(char word[], int len) {
    int leftIndex = 0;
    int rightIndex = len;
    while (leftIndex < rightIndex) {
        if (word[leftIndex] != word[rightIndex]) {
            return 0;
        }
        leftIndex++;
        rightIndex--;
    }
    return 1;
}

int bufferValidator(char *buffer) {
    if (isspace(buffer[0])) {
        printf("BEGGINING WITH SPACE");
    }
    return 0;
}
int palindomesCounter(char words[], int* numberOfAllWords) {
    char word[65507] = "";

}

int main(int argc, char *argv[]) {
    // Sprawdzenie czy podano argumenty
    if (argc != 2) {
        printf("Wrong number of arguments!");
        exit(1);
    }

    // Tworzenie gniazdka
    serwer = socket(AF_INET, SOCK_DGRAM, 0);

    // Sprawdzenie czy gniazdko zostało stworzone poprawnie
    if (serwer == -1) {
        perror("socket error");
        exit(1);
    }

    // Rejestracja funkcji wywołanych przez exit()
    if (atexit(koniec) != 0) {
        perror("atexit error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in adres = {
            .sin_family = AF_INET,
//            .sin_addr.s_addr = htonl(INADDR_ANY),
            .sin_addr.s_addr = inet_addr("127.0.0.1"),
            .sin_port = htons(atoi(argv[1]))
    };

    // Ustalenie ardesu lokalnego końca gniazdka
    if (bind(serwer, (struct sockaddr *)&adres, sizeof(adres)) == -1) {
        perror("bind error");
        exit(1);
    }

    char buffer[65507] = "";
    char message[] = "Hello, world\n";
    struct sockaddr_in klient;

    // Pętla czekająca na połączenia
    while (1) {
        unsigned int len = sizeof(klient);

        // Odebranie wiadomości z gniazdka
        int odczyt = recvfrom(serwer, (char *)buffer, 65507, 0, (struct sockaddr *)&klient, &len);

//        printf("%s", buffer);
        int numberOfAllWords = 0;
        int numberOfPalindomes = palindomesCounter(buffer, &numberOfAllWords);

        printf("%d", numberOfPalindomes);

        bufferValidator(buffer);
        printf("%s", buffer);







        // Sprawdzenie błędów funkcji recvfrom
        if (odczyt == -1) {
            perror("recivefrom error");
            exit(1);
        }

        // Wysłanie wiadomości do gniazdka
        int zapis = sendto(serwer, message, strlen(message), 0, (struct sockaddr *)&klient, len);

        // Sprawdzenie błędów funkcji sendto
        if (zapis != strlen(message)) {
            perror("sendto error");
            exit(1);
        }
    }
}
//int main(int argc, char *argv[]) {
//    char input[30];
//    scanf("%s", input);
//    printf("%d", isPalindome(input, 3));
//}
