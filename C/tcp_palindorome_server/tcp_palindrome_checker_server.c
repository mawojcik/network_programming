// Przykład dydaktyczny: różne rodzaje pętli głównej serwera TCP.
//
// gcc -std=c99 -pedantic -Wall -pthread rot13_server.c -lrt -o rot13_server
//
// Opcja -pthread jest konieczna bo przykład korzysta z POSIX-owych wątków;
// -lrt dołącza bibliotekę, która w starszych wersjach glibc była niezbędna
// aby móc korzystać z zegarów wysokiej rozdzielczości.

// Zdefiniowanie dwóch poniższych makr sygnalizowałoby, że kod napisano
// używając funkcji z POSIX.1-2008 z rozszerzeniem X/Open System Interface.
//   #define _POSIX_C_SOURCE 200809L
//   #define _XOPEN_SOURCE 700
// Niestety, POSIX nie definiuje liczbowych id wątków ani ulepszonych wersji
// funkcji poll. W przykładzie obie te rzeczy były potrzebne, kod wykracza
// więc poza standard POSIX i używa funkcji specyficznych dla Linuksa. Stąd
// użycie makra _GNU_SOURCE, znaczącego mniej więcej "chcę mieć dostęp do
// wszystkiego, co oferuje mój system operacyjny".
//
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>

// Te dwa pliki nagłówkowe są specyficzne dla Linuksa z biblioteką glibc:
#include <sys/epoll.h>
#include <sys/syscall.h>

// Standardowa procedura tworząca nasłuchujące gniazdko TCP.

int listening_socket_tcp_ipv4(in_port_t port)
{
    int s;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in a = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = htonl(INADDR_ANY),
            .sin_port = htons(port)
    };

    if (bind(s, (struct sockaddr *) &a, sizeof(a)) == -1) {
        perror("bind");
        goto close_and_fail;
    }

    if (listen(s, 10) == -1) {
        perror("listen");
        goto close_and_fail;
    }

    return s;

    close_and_fail:
    close(s);
    return -1;
}

// Pomocnicze funkcje do drukowania na ekranie komunikatów uzupełnianych
// o znacznik czasu oraz identyfikatory procesu/wątku. Będą używane do
// raportowania przebiegu pozostałych operacji we-wy.

void log_printf(const char * fmt, ...)
{
    // bufor na przyrostowo budowany komunikat, len mówi ile już znaków ma
    char buf[1024];
    int len = 0;

    struct timespec date_unix;
    struct tm date_human;
    long int usec;
    if (clock_gettime(CLOCK_REALTIME, &date_unix) == -1) {
        perror("clock_gettime");
        return;
    }
    if (localtime_r(&date_unix.tv_sec, &date_human) == NULL) {
        perror("localtime_r");
        return;
    }
    usec = date_unix.tv_nsec / 1000;

    // getpid() i gettid() zawsze wykonują się bezbłędnie
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    len = snprintf(buf, sizeof(buf), "%02i:%02i:%02i.%06li PID=%ji TID=%ji ",
                   date_human.tm_hour, date_human.tm_min, date_human.tm_sec,
                   usec, (intmax_t) pid, (intmax_t) tid);
    if (len < 0) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    int i = vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
    va_end(ap);
    if (i < 0) {
        return;
    }
    len += i;

    // zamień \0 kończące łańcuch na \n i wyślij całość na stdout, czyli na
    // deskryptor nr 1; bez obsługi błędów aby nie komplikować przykładu
    buf[len] = '\n';
    write(1, buf, len + 1);
}

void log_perror(const char * msg)
{
    log_printf("%s: %s", msg, strerror(errno));
}

void log_error(const char * msg, int errnum)
{
    log_printf("%s: %s", msg, strerror(errnum));
}

// Pomocnicze funkcje wykonujące pojedynczą operację we-wy oraz wypisujące
// szczegółowe komunikaty o jej działaniu.

int accept_verbose(int srv_sock)
{
    struct sockaddr_in a;
    socklen_t a_len = sizeof(a);

    log_printf("calling accept() on descriptor %i", srv_sock);
    int rv = accept(srv_sock, (struct sockaddr *) &a, &a_len);
    if (rv == -1) {
        log_perror("accept");
    } else {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &a.sin_addr, buf, sizeof(buf)) == NULL) {
            log_perror("inet_ntop");
            strcpy(buf, "???");
        }
        log_printf("new client %s:%u, new descriptor %i",
                   buf, (unsigned int) ntohs(a.sin_port), rv);
    }
    return rv;
}

ssize_t read_verbose(int fd, void * buf, size_t nbytes)
{
    log_printf("calling read() on descriptor %i", fd);
    ssize_t rv = read(fd, buf, nbytes);
    if (rv == -1) {
        log_perror("read");
    } else if (rv == 0) {
        log_printf("connection closed by client on descriptor %i", fd);
        // Optionally, you can handle the connection closure here
        // For example, you might want to close the socket and clean up resources
    } else {
        log_printf("received %zi bytes on descriptor %i", rv, fd);
    }
    return rv;
}

ssize_t write_verbose(int fd, void * buf, size_t nbytes)
{
    signal(SIGPIPE, SIG_IGN);

    log_printf("calling write() on descriptor %i", fd);
    ssize_t rv = write(fd, buf, nbytes);
    if (rv == -1) {
        log_perror("write");
    } else if (rv < nbytes) {
        log_printf("partial write on %i, wrote only %zi of %zu bytes",
                   fd, rv, nbytes);
    } else {
        log_printf("wrote %zi bytes on descriptor %i", rv, fd);
    }
    return rv;
}

int close_verbose(int fd)
{
    log_printf("closing descriptor %i", fd);
    int rv = close(fd);
    if (rv == -1) {
        log_perror("close");
    }
    return rv;
}

// Procedury przetwarzające pojedynczą porcję danych przysłaną przez klienta.

int bufferValidator(char *buffer, int bufferLength) {
    int dataLength = bufferLength;
    
    if ((buffer[bufferLength-2] == 13 && buffer[bufferLength - 1] == 10)) {
        dataLength = dataLength - 2;
    } else {
        printf("NO CR LF at the end\n");
        return -1;
    }

    if (dataLength == 0) { // allow for empty input
        return 1;
    }

    //TODO: zmienić na gotową funkcę
    for (int i = 0; i < dataLength; i++) {
        if (((buffer[i] < 65 || buffer[i] > 90) && (buffer[i] < 97 || buffer[i] > 122)) && buffer[i] != 32) {
            printf("WRONG INPUT: %c\n", buffer[i]);
            return -1;
        }
    }

    // return ERROR when space on first or last char
    if (buffer[0] == ' ' || buffer[dataLength-1] == ' ') {
        return -1;
    }

    // return ERROR when two consecutive spaces found
    for(int i = 0; i < dataLength; i++) {
        if(buffer[i] == ' ' && buffer[i+1] == ' ') {
            return -1;
        }
    }
    return dataLength;
}

void correctInput(char *buffer, char *correctedInput, int inputLength) {
    // copy data from buffer to correctedInput without optional ending chars
    memcpy(correctedInput, buffer, inputLength);
}

void toLower(char *word) {
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
            printf("palindorme: %s\n", word);

        }
        word = strtok(NULL, " ");
    }
}


int isCompleteQuery(char *buffer, int inputLength) {
    for (int i = 1; i <= inputLength; i++) {
        if ((buffer[i] == 10 && buffer[i - 1] == 13)) {
            // return length of current query
            return i + 1;
            // return at least 2 bytes ('\r' and '\n')
        }
    }
    // no CR LF encountered
    return 0;
}

ssize_t read_is_palindrome_write(int sock, char *bufferFromAllBuffers)
{
    char outputMessage[12] = "";
    char * dataPtr;
    char query[1024] = "";
    int oneQueryLength = 0;
    char wholeBuf[2048] = "";
    memcpy(wholeBuf, bufferFromAllBuffers, strlen(bufferFromAllBuffers));

    ssize_t bytes_read = 0;
    int inputWithoutCRLFLength = 0;
    int queryToMoveLen = 0;
    do {
        int numberOfPalindromes = 0;
        int numberOfAllWords = 0;
        char correctedInput[1024] = "";

        // ---> if \r\n not found in buffer, read from socket
        for (int i = 0; i < strlen(wholeBuf); i++) {
            if ((wholeBuf[i] == 10 && wholeBuf[i - 1] == 13)) {
                //  /r/n found in previous query
                goto skip_reading;
            }
        }

        char readBuf[2048] = "";
        bytes_read = read_verbose(sock, readBuf, sizeof(readBuf));
        if (bytes_read < 0 || bytes_read > 1024) {
            return -1;
        }
        if (bytes_read == 0) {
            printf("Connection closed\n");
            return -1;
        }
        //wholeBuf = wholeBuf + readBuf
        sprintf(wholeBuf + strlen(wholeBuf), "%s", readBuf);


        skip_reading:

        oneQueryLength = isCompleteQuery(wholeBuf, strlen(wholeBuf));
        queryToMoveLen = oneQueryLength;
        //deleting first valid query from wholeBuf:
        if (oneQueryLength > 0) {
            //query - first found valid query
            //clear query
            strcpy(query, "");
            memcpy(query, wholeBuf, oneQueryLength);

            inputWithoutCRLFLength = bufferValidator(query, oneQueryLength);
            if(inputWithoutCRLFLength == -1) {
                snprintf(outputMessage, sizeof(outputMessage), "%s", "ERROR\r\n");
                printf("%s", "ERROR\r\n");
            } else {
                correctInput(query, correctedInput, inputWithoutCRLFLength);
                countWords(correctedInput, &numberOfPalindromes, &numberOfAllWords);
                snprintf(outputMessage, sizeof(outputMessage), "%d/%d\r\n", numberOfPalindromes, numberOfAllWords);
                printf("%d/%d\r\n", numberOfPalindromes, numberOfAllWords);
            }
            
            // send results:
            dataPtr = outputMessage;
            size_t data_len = strlen(outputMessage);
            while (data_len > 0) {
                ssize_t bytes_written = write_verbose(sock, dataPtr, data_len);
                if (bytes_written < 0) {
                    return -1;
                }
                dataPtr = dataPtr + bytes_written;
                data_len = data_len - bytes_written;
            }

            memmove(wholeBuf, wholeBuf+queryToMoveLen, strlen(wholeBuf) - queryToMoveLen); // nie powinienem usuwać z /r/n?????????????
            for (int i = strlen(wholeBuf) - queryToMoveLen; i < 30; i++) {
                wholeBuf[i] = '\0';
            }
            // clean bufferFromAllBuffers:
            for (int i = 0; i < 30; i++) {
                bufferFromAllBuffers[i] = '\0';
            }
            memcpy(bufferFromAllBuffers, wholeBuf, strlen(wholeBuf));

            oneQueryLength = isCompleteQuery(wholeBuf, strlen(wholeBuf));
        }
    } while (oneQueryLength != 0);
    
    return bytes_read;
}



int add_fd_to_epoll(int fd, int epoll_fd)
{
    log_printf("adding descriptor %i to epoll instance %i", fd, epoll_fd);
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;    // rodzaj interesujących nas zdarzeń
    ev.data.fd = fd;        // dodatkowe dane, jądro nie zwraca na nie uwagi
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rv == -1) {
        log_perror("epoll_ctl(ADD)");
    }
    return rv;
}

int remove_fd_from_epoll(int fd, int epoll_fd)
{
    log_printf("removing descriptor %i from epoll instance %i", fd, epoll_fd);
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (rv == -1) {
        log_perror("epoll_ctl(DEL)");
    }
    return rv;
}

#define MAX_EVENTS 8
#define MAX_CLIENTS_NUMBER 500
char allBuffers[MAX_CLIENTS_NUMBER][1024];

void epoll_loop(int srv_sock)
{
    // w dzisiejszych czasach wartość argumentu nie ma znaczenia
    int epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        log_perror("epoll_create");
        return;
    }
    log_printf("epoll instance created, descriptor %i", epoll_fd);

    if (add_fd_to_epoll(srv_sock, epoll_fd) == -1) {
        goto cleanup_epoll;
    }

    while (true) {
        log_printf("calling epoll()");
        struct epoll_event events[MAX_EVENTS];
        // timeout równy -1 oznacza czekanie w nieskończoność
        int num = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num == -1) {
            log_perror("epoll");
            break;
        }
        // epoll wypełniła num początkowych elementów tablicy events
        log_printf("number of events = %i", num);

        for (int i = 0; i < num; ++i) {
            // deskryptor został wcześniej zapisany jako dodatkowe dane
            int fd = events[i].data.fd;
            // typ zgłoszonego zdarzenia powinien zawierać "gotów do odczytu"
            if ((events[i].events & EPOLLIN) == 0) {
                // niby nigdy nie powinno się zdarzyć, ale...
                log_printf("descriptor %i isn't ready to read", fd);
                continue;
            }

            if (fd == srv_sock) {

                int s = accept_verbose(srv_sock);
                if (s == -1) {
                    goto cleanup_epoll;
                }
                if (add_fd_to_epoll(s, epoll_fd) == -1) {
                    goto cleanup_epoll;
                }

            } else {    // fd != srv_sock
                // wybierz odpowiedni bufer
                printf("DATA ON FD: ");
                for (int i = 0; i < strlen((allBuffers[fd])); i++) {
                    printf("%d ", allBuffers[fd][i]);
                }
                printf("\n");
                if (read_is_palindrome_write(fd, allBuffers[fd]) < 0) {
                    // druga strona zamknęła połączenie lub wystąpił błąd
                    remove_fd_from_epoll(fd, epoll_fd);
                    // clead used buffer
                    for (int j = 0; i < 1024; i++) {
                        allBuffers[fd][j] = '\0';
                    }
                    close_verbose(fd);
                }

            }
        }
    }

    cleanup_epoll:
    close_verbose(epoll_fd);

    // W tym miejscu należałoby zamknąć otwarte połączenia z klientami, ale
    // nie dysponujemy żadną listą ani zbiorem z numerami ich deskryptorów.
}

// Powyższa funkcja używała epoll w kontekście jednego wątku. W rzeczywistych
// serwerach, takich jak Apache czy nginx, liczba wątków zazwyczaj jest
// proporcjonalna do liczby rdzeni obliczeniowych w komputerze. Pojawiają
// się wtedy dodatkowe problemy związane z synchronizacją, ze sprawiedliwym
// podziałem pracy pomiędzy wątki, itd.
//
// I oczywiście w rzeczywistych serwerach obliczenia wykonywane na danych są
// znacznie bardziej skomplikowane. Niezbędne dane wejściowe prawdopodobnie
// przyjdą w wielu porcjach, wygenerowana odpowiedź może być zbyt duża aby
// dało się ją jednym wywołaniem write() odesłać bez blokowania, itd.
//
// Osobom na serio zainteresowanym tym, jak działają rzeczywiste serwery
// obsługujące setki/tysiące połączeń na sekundę polecam więc przestudiowanie
// ich kodu źródłowego.

// Do napisania została jeszcze tylko funkcja main, i to będzie wszystko.

//const struct {
//    const char * name;
//    void (*func_ptr)(int);
//} srv_modes[] = {
//        { "epoll", epoll_loop },
//};

int main(int argc, char * argv[])
{
    long int srv_port = 2020;
    int srv_sock;
//    void (*main_loop)(int);

    // Przetwórz argumenty podane w linii komend, ustaw na ich podstawie
    // wartości zmiennych srv_port i main_loop.

//    main_loop = NULL;
//    for (int i = 0; i < sizeof(srv_modes) / sizeof(srv_modes[0]); ++i) {
//        if (strcmp(argv[1], srv_modes[i].name) == 0) {
//            main_loop = srv_modes[i].func_ptr;
//            break;
//        }
//    }
//    if (main_loop == NULL) {
//        goto bad_args;
//    }

//    char * p;
//    errno = 0;
//    srv_port = strtol(argv[2], &p, 10);
//    if (errno != 0 || *p != '\0' || srv_port < 1024 || srv_port > 65535) {
//        goto bad_args;
//    }

    // Stwórz gniazdko i uruchom pętlę odbierającą przychodzące połączenia.

    if ((srv_sock = listening_socket_tcp_ipv4(srv_port)) == -1) {
        goto fail;
    }

    log_printf("starting main loop");
    epoll_loop(srv_sock);
    log_printf("main loop done");

    if (close(srv_sock) == -1) {
        log_perror("close");
        goto fail;
    }

    return 0;

//    bad_args:
//    fprintf(stderr, "Usage: %s mode port\n", argv[0]);
//    fprintf(stderr, "available server modes:\n");
//    for (int i = 0; i < sizeof(srv_modes) / sizeof(srv_modes[0]); ++i) {
//        fprintf(stderr, "    %s\n", srv_modes[i].name);
//    }
//    fprintf(stderr, "listening port number range: 1024-65535\n");
    fail:
    return 1;
}