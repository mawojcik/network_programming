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

void koniec(void) {
    if (close(serwer) == -1) {
        perror("close error");
        exit(1);
    }
}

int isPalindrome(char word[]) {
    int leftIndex = 0;
    int rightIndex = strlen(word) - 1;
    if (rightIndex == 0) {
        return 1;
    }

    while (leftIndex < rightIndex) {
        if (word[leftIndex] != word[rightIndex]) {
            return 0;
        }
        leftIndex++;
        rightIndex--;
    }
    return 1;
}

int countWords(char *buffer, int *numberOfPalindomes, int *numberOfAllWords) {
    if (buffer[0] == ' ') {
        return -1;
    }
    buffer[strlen(buffer)-1] = ' ';

    char *token = strtok(buffer, " ");

    while (token != NULL) {
        (*numberOfAllWords)++;

        if (isPalindrome(token)) {
            (*numberOfPalindomes)++;
        }
        token = strtok(NULL, " ");
    }
    return 1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong number of arguments!");
        exit(1);
    }
    // Creating socket
    serwer = socket(AF_INET, SOCK_DGRAM, 0);

    // checking if creating failed
    if (serwer == -1) {
        perror("socket error");
        exit(1);
    }

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

    // Ustalenie adresu lokalnego końca gniazdka
    if (bind(serwer, (struct sockaddr *)&adres, sizeof(adres)) == -1) {
        perror("bind error");
        exit(1);
    }

    struct sockaddr_in klient;

    while (1) {
        int numberOfPalindomes = 0;
        int numberOfAllWords = 0;
        char buffer[1024] = "";
        char message[100] = "";


        unsigned int len = sizeof(klient);

        int receive_status = recvfrom(serwer, (char *)buffer, 1024, 0, (struct sockaddr *)&klient, &len);

        if (receive_status == -1) {
            perror("recivefrom error");
            exit(1);
        }

        if(countWords(buffer, &numberOfPalindomes, &numberOfAllWords) == -1) {
            sprintf(message, "%s", "ERROR\n");
        } else {
            sprintf(message, "%d / %d\n", numberOfPalindomes, numberOfAllWords);
        }

        int send_status = sendto(serwer, message, strlen(message), 0, (struct sockaddr *)&klient, len);
        
        if (send_status != strlen(message)) {
            perror("sendto error");
            exit(1);
        }
    }
}
