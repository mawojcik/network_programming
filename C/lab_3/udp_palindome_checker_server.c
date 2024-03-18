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

int bufferValidator(char *buffer) {
    int bufferLen = strlen(buffer) - 2;
    if (buffer[0] == ' ' || buffer[bufferLen] == ' ') {
        return 0;
    }

    for(int i = 0; i < bufferLen; i++) {
        if(buffer[i] == ' ' && buffer[i+1] == ' ') {
            return 0;
        }
    }
    return 1;
}
void toLower(char * word) {
    for(int i = 0; i < strlen(word); i++) {
        word[i] = tolower(word[i]);
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

void countWords(char *buffer, int *numberOfPalindomes, int *numberOfAllWords) {
    buffer[strlen(buffer)-1] = ' ';
    char *word = strtok(buffer, " ");

    while (word != NULL) {
        toLower(word);
        (*numberOfAllWords)++;

        if (isPalindrome(word)) {
            (*numberOfPalindomes)++;
        }
        word = strtok(NULL, " ");
    }
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


        if(bufferValidator(buffer) == 0) {
            sprintf(message, "%s", "ERROR\n");
        } else {
            countWords(buffer, &numberOfPalindomes, &numberOfAllWords);
            sprintf(message, "%d / %d\n", numberOfPalindomes, numberOfAllWords);
        }

        int send_status = sendto(serwer, message, strlen(message), 0, (struct sockaddr *)&klient, len);
        
        if (send_status != strlen(message)) {
            perror("sendto error");
            exit(1);
        }
    }
}
