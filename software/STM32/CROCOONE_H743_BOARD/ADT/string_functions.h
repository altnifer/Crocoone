/*
 * string_functions.h
 *
 *  Created on: Jul 18, 2024
 *      Author: nikit
 */

#ifndef STRING_FUNCTIONS_H_
#define STRING_FUNCTIONS_H_

#include "stdbool.h"
#include "stdint.h"

int32_t contains_symbol(char *symbols, uint16_t symbols_len, char symbol);
int32_t contains_symbol_reversed(char *symbols, uint16_t symbols_len, char symbol);
char *get_first_token(char *str, uint16_t str_len, char *separators, uint16_t sep_count, uint16_t *token_len_p);

#endif /* STRING_FUNCTIONS_H_ */
