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

int bufferValidator(char *buffer, int bufferLength) {
    // checking if only letters and spaces are in input - also not accepting \n \r \t
    for (int i = 0; i < bufferLength; i++) {
        if (((buffer[i] < 65 || buffer[i] > 90) && (buffer[i] < 97 || buffer[i] > 122)) && buffer[i] != 32) {
            printf("WRONG INPUT: %c\n", buffer[i]);
            return -1;
        }
    }

    if (bufferLength == 0) { // allow for empty input
        return 1;
    }

    // return ERROR when space on first or last char
    if (buffer[0] == ' ' || buffer[bufferLength-1] == ' ') {
        return -1;
    }

    // return ERROR when two consecutive spaces found
    for(int i = 0; i < bufferLength; i++) {
        if(buffer[i] == ' ' && buffer[i+1] == ' ') {
            return -1;
        }
    }
    return bufferLength;
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

void correctInput(char *buffer, char *correctedInput, int inputLength) {
    // copy data from buffer to correctedInput without optional ending chars
    memcpy(correctedInput, buffer, inputLength);
}

void countWords(char *buffer, int *numberOfPalindomes, int *numberOfAllWords) {
    // new var, only with correct input
    char *word = strtok(buffer, " ");
    while (word != NULL) {
        printf("word: %s, word len: %lu\n", word, strlen(word));

        toLower(word);
        (*numberOfAllWords)++;

        if (isPalindrome(word)) {
            (*numberOfPalindomes)++;
            printf("palindorme: %s\n", word);

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

    // bind socket to address and port
    if (bind(serwer, (struct sockaddr *)&adres, sizeof(adres)) == -1) {
        perror("bind error");
        exit(1);
    }

    struct sockaddr_in klient;

    while (1) {
        int numberOfPalindomes = 0;
        int numberOfAllWords = 0;
        char buffer[1025] = "";
        char outputMessage[100] = "";
        char correctedInput[1024] = "";


        unsigned int len = sizeof(klient);

        int bufferLength = recvfrom(serwer, (char *)buffer, 1025, 0, (struct sockaddr *)&klient, &len);

        if (bufferLength == -1) {
            perror("recivefrom error");
            exit(1);
        }
        if (bufferLength <= 1024) {
            int inputLength = bufferValidator(buffer, bufferLength);
            if(inputLength == -1) {
                sprintf(outputMessage, "%s", "ERROR");
            } else {
                correctInput(buffer, correctedInput, inputLength);
                countWords(correctedInput, &numberOfPalindomes, &numberOfAllWords);
                sprintf(outputMessage, "%d/%d", numberOfPalindomes, numberOfAllWords);
            }
        } else {
            sprintf(outputMessage, "%s", "ERROR");
        }

        int send_status = sendto(serwer, outputMessage, strlen(outputMessage), 0, (struct sockaddr *)&klient, len);

        if (send_status != strlen(outputMessage)) {
            perror("sendto error");
            exit(1);
        }
    }
}
