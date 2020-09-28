
#include "forca.h"

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

char* generate_blank_word(char* blank_word, const size_t length) {
    memset(blank_word, 0, MAX_WORD_SIZE);
    for (int i = 0; i < length; i++) {
        blank_word[i] = WILDCARD_CHAR;
    }
    return blank_word;
}

void fill_guess(const char guess, const int *positions, const int occurrences, char *word) {
    for (size_t i = 0; i < occurrences; i++)
    {       
        word[positions[i]] = guess;
    }
}

struct guess_response check_occurrences(const char letter, const char* word) {
    int matches = 0;
    struct guess_response response;
    size_t word_size = strlen(word);
    for (size_t i = 0; i < word_size; i++)
    {       
        if (word[i] == letter) {
            response.positions[matches] = i;
            matches += 1;
        }
    }
    response.occurrences = matches;
    response.type = GUESS_RESPONSE;
    return response;
}

void grtostr(const struct guess_response response, char* str) {
    str[0] = GUESS_RESPONSE + '0';
    str[1] = response.occurrences + '0';
    for (size_t i = 0; i < response.occurrences; i++)
    {
        str[i + 2] = response.positions[i] + '0'; 
    }
}

struct user_guess strtoug(const char* str) {
    struct user_guess guess;
    guess.letter = str[1];
    return guess;
}

void ugtostr(const struct user_guess guess, char* str) {
    str[0] = GUESS + '0';
    str[1] = guess.letter;
}

void cfgtostr(const int word_size, char* str) {
    str[0] = START + '0';
    str[1] = word_size + '0';
}

void eogtostr(char* str) {
    str[0] = EOG + '0';
}

int strtocfg(const char *str) {
    return str[1] - '0';
}

struct guess_response strtogr(const char *str) {
    struct guess_response response;
    response.occurrences = str[1] - '0';
    for (size_t i = 0; i < response.occurrences; i++){
        response.positions[i] = str[2 + i] - '0';
    }
    return response;
}

void fill_final_guess(const char guess, char *word) {
    for (size_t i = 0; i < strlen(word); i++)
    {
        if (word[i] == WILDCARD_CHAR) {
            word[i] = guess;
        }
    }
    
}