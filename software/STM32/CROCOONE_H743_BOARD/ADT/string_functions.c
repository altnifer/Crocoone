/*
 * string_functions.c
 *
 *  Created on: Jul 18, 2024
 *      Author: nikit
 */
#include "string_functions.h"
#include <string.h>

int32_t contains_symbol(char *symbols, uint16_t symbols_len, char symbol) {
  if(symbols == NULL)
    return -1;
  char *curr_p = symbols;
  while(curr_p - symbols < symbols_len)
    if(*curr_p++ == symbol)
      return curr_p - symbols - 1;
  return -1;
}

int32_t contains_symbol_reversed(char *symbols, uint16_t symbols_len, char symbol) {
  if(symbols == NULL)
    return -1;
  char *curr_p = symbols + symbols_len - 1;
  while(curr_p - symbols >= 0)
    if(*curr_p-- == symbol)
      return curr_p - symbols + 1;
  return -1;
}

char *get_first_token(char *str, uint16_t str_len, char *separators, uint16_t sep_count, uint16_t *token_len_p) {
    if(str == NULL || separators == NULL)
        return 0;

    char *sp = str;
    char *token;
    uint16_t token_len = 0;
    while (contains_symbol(separators, sep_count, *sp) != -1)
        sp++;
    token = sp;

    while (contains_symbol(separators, sep_count, *sp) == -1 && sp - str < str_len) {
        token_len++;
        sp++;
    }

    *token_len_p = token_len;
    return token_len ? token : NULL;
}
