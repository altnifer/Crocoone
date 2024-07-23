/*
 * ascii_hid_table.c
 *
 *  Created on: Jul 19, 2024
 *      Author: nikit
 */
#include "adt.h"
#include "ascii_hid_table.h"

hashmap_t *ascii_hid_map;
static bool ascii_hid_map_init_flag = false;

void push_hid_key(char *key, uint32_t key_size, uint8_t modifier, uint8_t keycode);

void ascii_hid_map_init() {
	hashmap_init(&ascii_hid_map, 256);

	push_hid_key("a", 1, KEY_NONE, KEY_A);
	push_hid_key("b", 1, KEY_NONE, KEY_B);
	push_hid_key("c", 1, KEY_NONE, KEY_C);
	push_hid_key("d", 1, KEY_NONE, KEY_D);
	push_hid_key("e", 1, KEY_NONE, KEY_E);
	push_hid_key("f", 1, KEY_NONE, KEY_F);
	push_hid_key("g", 1, KEY_NONE, KEY_G);
	push_hid_key("h", 1, KEY_NONE, KEY_H);
	push_hid_key("i", 1, KEY_NONE, KEY_I);
	push_hid_key("j", 1, KEY_NONE, KEY_J);
	push_hid_key("k", 1, KEY_NONE, KEY_K);
	push_hid_key("l", 1, KEY_NONE, KEY_L);
	push_hid_key("m", 1, KEY_NONE, KEY_M);
	push_hid_key("n", 1, KEY_NONE, KEY_N);
	push_hid_key("o", 1, KEY_NONE, KEY_O);
	push_hid_key("p", 1, KEY_NONE, KEY_P);
	push_hid_key("q", 1, KEY_NONE, KEY_Q);
	push_hid_key("r", 1, KEY_NONE, KEY_R);
	push_hid_key("s", 1, KEY_NONE, KEY_S);
	push_hid_key("t", 1, KEY_NONE, KEY_T);
	push_hid_key("u", 1, KEY_NONE, KEY_U);
	push_hid_key("v", 1, KEY_NONE, KEY_V);
	push_hid_key("w", 1, KEY_NONE, KEY_W);
	push_hid_key("x", 1, KEY_NONE, KEY_X);
	push_hid_key("y", 1, KEY_NONE, KEY_Y);
	push_hid_key("z", 1, KEY_NONE, KEY_Z);

	push_hid_key("A", 1, KEY_MOD_LSHIFT, KEY_A);
	push_hid_key("B", 1, KEY_MOD_LSHIFT, KEY_B);
	push_hid_key("C", 1, KEY_MOD_LSHIFT, KEY_C);
	push_hid_key("D", 1, KEY_MOD_LSHIFT, KEY_D);
	push_hid_key("E", 1, KEY_MOD_LSHIFT, KEY_E);
	push_hid_key("F", 1, KEY_MOD_LSHIFT, KEY_F);
	push_hid_key("G", 1, KEY_MOD_LSHIFT, KEY_G);
	push_hid_key("H", 1, KEY_MOD_LSHIFT, KEY_H);
	push_hid_key("I", 1, KEY_MOD_LSHIFT, KEY_I);
	push_hid_key("J", 1, KEY_MOD_LSHIFT, KEY_J);
	push_hid_key("K", 1, KEY_MOD_LSHIFT, KEY_K);
	push_hid_key("L", 1, KEY_MOD_LSHIFT, KEY_L);
	push_hid_key("M", 1, KEY_MOD_LSHIFT, KEY_M);
	push_hid_key("N", 1, KEY_MOD_LSHIFT, KEY_N);
	push_hid_key("O", 1, KEY_MOD_LSHIFT, KEY_O);
	push_hid_key("P", 1, KEY_MOD_LSHIFT, KEY_P);
	push_hid_key("Q", 1, KEY_MOD_LSHIFT, KEY_Q);
	push_hid_key("R", 1, KEY_MOD_LSHIFT, KEY_R);
	push_hid_key("S", 1, KEY_MOD_LSHIFT, KEY_S);
	push_hid_key("T", 1, KEY_MOD_LSHIFT, KEY_T);
	push_hid_key("U", 1, KEY_MOD_LSHIFT, KEY_U);
	push_hid_key("V", 1, KEY_MOD_LSHIFT, KEY_V);
	push_hid_key("W", 1, KEY_MOD_LSHIFT, KEY_W);
	push_hid_key("X", 1, KEY_MOD_LSHIFT, KEY_X);
	push_hid_key("Y", 1, KEY_MOD_LSHIFT, KEY_Y);
	push_hid_key("Z", 1, KEY_MOD_LSHIFT, KEY_Z);

	push_hid_key("1", 1, KEY_NONE, KEY_1);
	push_hid_key("2", 1, KEY_NONE, KEY_2);
	push_hid_key("3", 1, KEY_NONE, KEY_3);
	push_hid_key("4", 1, KEY_NONE, KEY_4);
	push_hid_key("5", 1, KEY_NONE, KEY_5);
	push_hid_key("6", 1, KEY_NONE, KEY_6);
	push_hid_key("7", 1, KEY_NONE, KEY_7);
	push_hid_key("8", 1, KEY_NONE, KEY_8);
	push_hid_key("9", 1, KEY_NONE, KEY_9);
	push_hid_key("0", 1, KEY_NONE, KEY_0);

	push_hid_key("!", 1, KEY_MOD_LSHIFT, KEY_1);
	push_hid_key("@", 1, KEY_MOD_LSHIFT, KEY_2);
	push_hid_key("#", 1, KEY_MOD_LSHIFT, KEY_3);
	push_hid_key("$", 1, KEY_MOD_LSHIFT, KEY_4);
	push_hid_key("%", 1, KEY_MOD_LSHIFT, KEY_5);
	push_hid_key("^", 1, KEY_MOD_LSHIFT, KEY_6);
	push_hid_key("&", 1, KEY_MOD_LSHIFT, KEY_7);
	push_hid_key("*", 1, KEY_MOD_LSHIFT, KEY_8);
	push_hid_key("(", 1, KEY_MOD_LSHIFT, KEY_9);
	push_hid_key(")", 1, KEY_MOD_LSHIFT, KEY_0);

	push_hid_key("-", 1, KEY_NONE, KEY_MINUS);
	push_hid_key("=", 1, KEY_NONE, KEY_EQUAL);
	push_hid_key("[", 1, KEY_NONE, KEY_LEFTBRACE);
	push_hid_key("]", 1, KEY_NONE, KEY_RIGHTBRACE);
	push_hid_key("\\", 1, KEY_NONE, KEY_BACKSLASH);
	push_hid_key("#", 1, KEY_NONE, KEY_HASHTILDE);
	push_hid_key(";", 1, KEY_NONE, KEY_SEMICOLON);
	push_hid_key("'", 1, KEY_NONE, KEY_APOSTROPHE);
	push_hid_key("`", 1, KEY_NONE, KEY_GRAVE);
	push_hid_key(",", 1, KEY_NONE, KEY_COMMA);
	push_hid_key(".", 1, KEY_NONE, KEY_DOT);
	push_hid_key("/", 1, KEY_NONE, KEY_SLASH);

	push_hid_key("_", 1, KEY_MOD_LSHIFT, KEY_MINUS);
	push_hid_key("+", 1, KEY_MOD_LSHIFT, KEY_EQUAL);
	push_hid_key("{", 1, KEY_MOD_LSHIFT, KEY_LEFTBRACE);
	push_hid_key("}", 1, KEY_MOD_LSHIFT, KEY_RIGHTBRACE);
	push_hid_key("|", 1, KEY_MOD_LSHIFT, KEY_BACKSLASH);
	//push_hid_key("~", 1, KEY_MOD_LSHIFT, KEY_HASHTILDE);
	push_hid_key(":", 1, KEY_MOD_LSHIFT, KEY_SEMICOLON);
	push_hid_key("\"", 1, KEY_MOD_LSHIFT, KEY_APOSTROPHE);
	push_hid_key("~", 1, KEY_MOD_LSHIFT, KEY_GRAVE);
	push_hid_key("<", 1, KEY_MOD_LSHIFT, KEY_COMMA);
	push_hid_key(">", 1, KEY_MOD_LSHIFT, KEY_DOT);
	push_hid_key("?", 1, KEY_MOD_LSHIFT, KEY_SLASH);

	push_hid_key("F1", 2, KEY_NONE, KEY_F1);
	push_hid_key("F2", 2, KEY_NONE, KEY_F2);
	push_hid_key("F3", 2, KEY_NONE, KEY_F3);
	push_hid_key("F4", 2, KEY_NONE, KEY_F4);
	push_hid_key("F5", 2, KEY_NONE, KEY_F5);
	push_hid_key("F6", 2, KEY_NONE, KEY_F6);
	push_hid_key("F7", 2, KEY_NONE, KEY_F7);
	push_hid_key("F8", 2, KEY_NONE, KEY_F8);
	push_hid_key("F9", 2, KEY_NONE, KEY_F9);
	push_hid_key("F10", 3, KEY_NONE, KEY_F10);
	push_hid_key("F11", 3, KEY_NONE, KEY_F11);
	push_hid_key("F12", 3, KEY_NONE, KEY_F12);

	push_hid_key(" ", 1, KEY_NONE, KEY_SPACE);

	push_hid_key("ENTER", 5, KEY_NONE, KEY_ENTER);
	push_hid_key("SHIFT", 5, KEY_MOD_LSHIFT, KEY_NONE);
	push_hid_key("ALT", 3, KEY_MOD_LALT, KEY_NONE);
	push_hid_key("CTRL", 4, KEY_MOD_LCTRL, KEY_NONE);
	push_hid_key("GUI", 3, KEY_MOD_LGUI, KEY_NONE);

	ascii_hid_map_init_flag = true;
}

bool ascii_hid_map_state() {
	return ascii_hid_map_init_flag;
}

void push_hid_key(char *key, uint32_t key_size, uint8_t modifier, uint8_t keycode) {
	hid_key_t hid_key;
	hid_key.MODIFIER = modifier;
	hid_key.KEYCODE = keycode;
	hashmap_push(&ascii_hid_map, &hid_key, sizeof(hid_key_t), (uint8_t *)key, key_size);
}
