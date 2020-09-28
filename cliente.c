#include "utils.h"
#include "forca.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MIN_ARGC 3
#define BUFSZ 1024

struct user_guess fill_user_guess(const char guess);

void usage(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (addrparse(argv[1], argv[2], &storage) != 0)
    {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (connect(s, addr, sizeof(storage)) != 0)
    {
        logexit("connext)");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Connected to %s", addrstr);

    char message[MAX_MESSAGE_SIZE];
    size_t received = recv(s, message, MAX_MESSAGE_SIZE, 0);
    if (received > MAX_MESSAGE_SIZE)
    {
        logexit("received");
    }

    int word_size = strtocfg(message);
    char word[MAX_WORD_SIZE];
    memset(word, 0, MAX_WORD_SIZE);
    generate_blank_word(word, word_size);

    bool already_tried[52];
    memset(already_tried, false, 52);

    while (true)
    {
        char guess;
        bool invalid_guess = false;
        do
        {
            if (invalid_guess)
            {
                printf("\nYou have already tried \"%c\"", guess);
            }
            printf("\nProgress: %s", word);
            printf("\nGuess a letter: ");
            scanf(" %c", &guess);
            invalid_guess = already_tried[guess - 'A'];
        } while (invalid_guess);
        already_tried[guess - 'A'] = true;

        struct user_guess user_guess = fill_user_guess(guess);
        memset(message, 0, MAX_MESSAGE_SIZE);
        ugtostr(user_guess, message);
        size_t count = send(s, message, strlen(message), 0);
        printf("Sent %d bytes.", (int)count);
        if (count != strlen(message))
        {
            logexit("send");
        }

        memset(message, 0, MAX_MESSAGE_SIZE);
        count = recv(s, message, MAX_MESSAGE_SIZE, 0);
        if (count == -1)
        {
            logexit("receive");
        }
        printf("\nReceived %u bytes", (int)count);
        if (message[0] - '0' == GUESS_RESPONSE)
        {
            struct guess_response response = strtogr(message);
            fill_guess(guess, response.positions, response.occurrences, word);
            printf("\n--------------");
        }
        else if (message[0] - '0' == EOG)
        {
            fill_final_guess(guess, word);
            printf("\nCongratulations!! The answer was: \"%s\"\n", word);
            break;
        }
    }

    close(s);
    exit(EXIT_SUCCESS);
}

struct user_guess fill_user_guess(const char guess)
{
    struct user_guess user_guess;
    user_guess.type = GUESS;
    user_guess.letter = guess;
    return user_guess;
}