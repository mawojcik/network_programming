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
//@TODO:
// add checking for numbers - do not accept numbers
// change this function to return input data length and -1 when buffer is invalid
int bufferValidator(char *buffer, int bufferLength) {
    int inputLength = bufferLength;

    // deleting carriage return and line feed when found
    for (int i = 0; i < bufferLength; i++) {
        if (buffer[i] == 10 || buffer[i] == 13) {
        }
    }
    printf("inputLength: %d\n", inputLength);

    // return ERROR when space on first or last char
    if (buffer[0] == ' ' || buffer[inputLength-1] == ' ') {
        return 0;
    }

    // return ERROR when two consecutive spaces found
    for(int i = 0; i < inputLength; i++) {
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
            .sin_port = htons(2020)
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

        int bufferLength = recvfrom(serwer, (char *)buffer, 1024, 0, (struct sockaddr *)&klient, &len);

//        printf("%d\n", receive_status);

        if (bufferLength == -1) {
            perror("recivefrom error");
            exit(1);
        }


        if(bufferValidator(buffer, bufferLength) == 0) {
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
