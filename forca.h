#pragma once

#include <sys/types.h>
#define START ((unsigned char) 1)
#define GUESS ((unsigned char) 2)
#define GUESS_RESPONSE ((unsigned char) 3)
#define EOG ((unsigned char) 4)
#define MAX_WORD_SIZE 256
#define MAX_MESSAGE_SIZE 258
#define WILDCARD_CHAR '*'

struct guess_response {
    unsigned char type;
    int occurrences;
    int positions[MAX_WORD_SIZE];
};

struct user_guess {
    unsigned char type;
    char letter;
};

void fill_guess(const char guess, const int *positions, const int occurrences, char *word);
void fill_final_guess(const char guess, char *word);
char* generate_blank_word(char* blank_word, const size_t length);
struct guess_response check_occurrences(const char letter, const char* word);

void grtostr(const struct guess_response response, char* str);
void ugtostr(const struct user_guess guess, char* str);
void cfgtostr(const int word_size, char* str);
void eogtostr(char* str);

struct user_guess strtoug(const char* str);
int strtocfg(const char* str);
struct guess_response strtogr(const char *str);
