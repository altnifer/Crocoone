/*
 * ESP32_AT_cmd.c
 *
 *  Created on: 30 июл. 2023 г.
 *      Author: nikit
 */
#include "ESP32_WiFi.h"

#define PKTS 60
#define MAXWIFICH 14

const char* FOLDER_NAME = "handshakes";

uint8_t rxBuff[AT_UART_Buff_size] = {};

char ** scaned_AP = NULL;
int AP_count = 0;

int pkts[MAXWIFICH][PKTS] = {};
int maxPkts[MAXWIFICH] = {};
int currentCh = 0;
int deauthPkts[MAXWIFICH][PKTS] = {};
int maxDeauthPkts[MAXWIFICH] = {};

bool PktAnalyzerParsing = 0;

uint32_t recieved_pkt_size = 0;

int target = -1;


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &ESP32_AT_UART_Port)
	{
		if (PktAnalyzerParsing) {

			for (int i = 0; i < PKTS - 1; i++) {
				pkts[currentCh][i] = pkts[currentCh][i+1];
			}
			for (int i = 0; i < PKTS - 1; i++) {
				deauthPkts[currentCh][i] = deauthPkts[currentCh][i+1];
			}
			sscanf((char *)rxBuff, "%d%d", &(pkts[currentCh][PKTS-1]), &(deauthPkts[currentCh][PKTS-1]));
			if (pkts[currentCh][PKTS-1] > maxPkts[currentCh]) maxPkts[currentCh] = pkts[currentCh][PKTS-1];
			if (deauthPkts[currentCh][PKTS-1] > maxDeauthPkts[currentCh]) maxDeauthPkts[currentCh] = deauthPkts[currentCh][PKTS-1];
			memset(rxBuff, 0, AT_UART_Buff_size);

		}
		recieved_pkt_size = Size;

		HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
	}
}

at_error_t ESP32_WriteRecieve_AT(char * cmd, int cmd_len) {
	memset(rxBuff, 0, AT_UART_Buff_size);
	HAL_UART_Transmit(&ESP32_AT_UART_Port, (uint8_t *)cmd, cmd_len, 100);
	HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
	uint32_t current_time = HAL_GetTick();
	while (HAL_GetTick() - current_time < 4000)
		if (strstr((char *)rxBuff, "AT/")) {
			uint32_t current_time = HAL_GetTick();        //delay before sending next command
			while (HAL_GetTick() - current_time < 250);   //needed when sending commands one after another
			int ans = *(strstr((char *)rxBuff, "AT/") + 3);
			if (ans < AT_OK && ans > AT_WRONG_TYPE) {
				return WRONG_AT_ANS;
			}
			return (at_error_t)ans;
		}
	return UART_ERROR;
}

at_error_t ESP32_RecieveDATA_OnSD(data_type_t data_type) {
	FATFS SDFatFs;
	FIL MyFile;
	uint32_t byteswritten;

	const uint32_t MAX_WAIT_TIME = 1500;

	char cmd[30];
	int res;
	char file_name[255] = {}; //255 - maximum LFN file name

	uint32_t DATA_size = 0;
	uint32_t recieved_DATA_size = 0;
	uint32_t time = 0;

	eeprom_settings_t eeprom_settings;
	if (!eeprom_read_settings(&eeprom_settings))
		return EEPROM_ERROR;

	if (data_type == PCAP) {
		sprintf(file_name, "%s/pcap_%lu.pcap", FOLDER_NAME, eeprom_settings.pcap_counter);
		sprintf(cmd, GETPCAP);
		eeprom_settings.pcap_counter++;
	} else if (data_type == HCCAPX) {
		sprintf(file_name, "%s/hccapx_%lu.hccapx", FOLDER_NAME, eeprom_settings.hccapx_counter);
		sprintf(cmd, GETHCCAPX);
		eeprom_settings.hccapx_counter++;
	} else if (data_type == HC22000){
		sprintf(file_name, "%s/ch22000_%lu.hc22000", FOLDER_NAME, eeprom_settings.hc22000_hnd_counter);
		sprintf(cmd, GETHC22000);
		eeprom_settings.hc22000_hnd_counter++;
	}

	if (f_mount(&SDFatFs, (TCHAR const*)SDPath, 1) != FR_OK) {
		return SD_ERROR;
	}

	res = f_mkdir((TCHAR const*)FOLDER_NAME);
	if (res != FR_OK && res != FR_EXIST) {
		f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
		return SD_MK_DIR_ERROR;
	}

	res = f_open(&MyFile, file_name, FA_CREATE_NEW | FA_WRITE);
	if (res != FR_OK && res != FR_EXIST) {
		f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
		return SD_MK_FILE_ERROR;
	}

	res = ESP32_WriteRecieve_AT(cmd, strlen(cmd));
	if (res != AT_OK) {
		f_close(&MyFile);
		f_unlink(file_name);
		f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
		return res;
	}
	if (!sscanf(strstr((char*)rxBuff, "size") + 5, "%lu", &DATA_size) || DATA_size == 0) {
		f_close(&MyFile);
		f_unlink(file_name);
		f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
		return NO_DATA;
	}

	time = HAL_GetTick();
	memset(rxBuff, 0, AT_UART_Buff_size);
	recieved_pkt_size = 0;
	HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
	while (1) {
		if (recieved_DATA_size >= DATA_size) break;
		if (HAL_GetTick() - time > MAX_WAIT_TIME) {
			f_close(&MyFile);
			f_unlink(file_name);
			f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
			return TIMEOUT;
		}
		if (recieved_pkt_size) {
			recieved_DATA_size += recieved_pkt_size;
			time = HAL_GetTick();
			res = f_write(&MyFile, rxBuff, recieved_pkt_size, (void*)&byteswritten);
			if (res != FR_OK) {
				f_close(&MyFile);
				f_unlink(file_name);
				f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
				return SD_FILE_WRITE_ERROR;
			}
			recieved_pkt_size = 0;
			memset(rxBuff, 0, AT_UART_Buff_size);
			HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
		}
	}

	if (!eeprom_write_settings(&eeprom_settings))
		return EEPROM_ERROR;

	f_close(&MyFile);
	f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
	return AT_OK;
}


void ESP32_AT_UART_Init() {
	HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
}

void ESP32_start_handshake_attack(const uint8_t timeout_t, const handshake_method_t method_t) {

	int res;
	char str[30];

	int current_set = 0;
	const int FILE_FORMAT_COUNT = 3;
	bool download = 0;
	data_type_t data_type;
	char* file_names[FILE_FORMAT_COUNT];
	for (uint8_t i = 0; i < FILE_FORMAT_COUNT; i++) {
		file_names[i] = (char*)malloc(11*sizeof(char));
	}
	strncpy(file_names[0], " *.pcap", 11);
	strncpy(file_names[1], " *.hccapx", 11);
	strncpy(file_names[2], " *.hc22000", 11);

	ClearAllButtons();

	fillRect(0, 13, 160, 128 - 13, MAIN_BG_COLOR);
	drawIcon("ATTACK STATUS", 1, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("prepair...") * 7) / 2, 34, "prepair...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("start capture...") * 7) / 2, 46, "start capture...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(str, "%s%d/%d/%d", HSTART, target + 1, timeout_t, (int)method_t);
	res = ESP32_WriteRecieve_AT(str, strlen(str));
	if (res != AT_OK) {
		sprintf(str, "AT code %d", res);
		fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 46, str, Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		while (1) if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return; //exit
	}

	fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("handshake...") * 7) / 2, 34, "handshake...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	uint32_t time = HAL_GetTick();
	uint8_t darw_timeout = 0;
	while (darw_timeout < timeout_t) { //waiting for handshake capturing finish
		if (HAL_GetTick() - time > 1000 && darw_timeout != timeout_t) {
			darw_timeout++;
			sprintf(str, "timeout: %ds/%ds", darw_timeout, timeout_t);
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 46, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			time = HAL_GetTick();
		}
	}

	drawIcon("ATTACK STATUS", 0, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("DONE") * 7) / 2, 34, "DONE", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	drawIcon("DOWNLOAD DATA", 1, 11, 63, 149, 121, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	while (1) {
		if (GetButtonState(BUTTON_DOWN) == SINGLE_CLICK) {
			current_set = (current_set + 1) % FILE_FORMAT_COUNT;
		}

		if (GetButtonState(BUTTON_UP) == SINGLE_CLICK) {
			current_set = (current_set - 1) % FILE_FORMAT_COUNT;
			if (current_set < 0) current_set = FILE_FORMAT_COUNT;
		}

		if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK && (*file_names[current_set] != 'v' || *file_names[current_set] != 'x')) {
			data_type = (data_type_t)current_set;
			download = 1;
		}

		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;

		drawMenu(file_names, FILE_FORMAT_COUNT, 13, 81, 127, 81 + 3 * 12, current_set, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

		if (!download || *file_names[current_set] == 'v') continue;


		drawIcon("ATTACK STATUS", 1, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("get data...") * 7) / 2, 34, "get data...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

		res = ESP32_RecieveDATA_OnSD(data_type);
		drawIcon("ATTACK STATUS", 0, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("DONE") * 7) / 2, 34, "DONE", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		if (res == NO_DATA) {
			*file_names[current_set] = 'x';
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("no hashes captured") * 7) / 2, 46, "no hashes captured", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		} else if (res != AT_OK) {
			sprintf(str, "parse code %d", res);
			fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 46, str, Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
			//f_mount(&SDFatFs, (TCHAR const*)SDPath, 0);
			while (1) {
				if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return; //exit
			}
		} else {
			*file_names[current_set] = 'v';
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("data download") * 7) / 2, 46, "data download", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}

		download = 0;
	}
	memset(rxBuff, 0, AT_UART_Buff_size);
}

void ESP32_start_deauth_attack(const int timeout_t) {

	int res;
	char str[30];

	ClearAllButtons();

	fillRect(0, 13, 160, 128 - 13, MAIN_BG_COLOR);
	drawIcon("ATTACK STATUS", 1, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("prepair...") * 7) / 2, 34, "prepair...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	if ((timeout_t != -1 && (timeout_t < MIN_TIMEOUT_SEC || timeout_t > MAX_DEAUTH_TIMEOUT_SEC)) || target == -1) return;

	sprintf(str, "%s%d/%d", DASTART, target + 1, timeout_t);
	res = ESP32_WriteRecieve_AT(str, strlen(str));
	if (res != AT_OK) {
		fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("start attack") * 7) / 2, 46, "start attack", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		while (1) {
			if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return; //exit
		}
	}

	fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("deauth...") * 7) / 2, 34, "deauth...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	uint32_t time = HAL_GetTick();
	uint8_t darw_timeout = 0;

	if (timeout_t == -1) {
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("button3 to stop") * 7) / 2, 46, "button3 to stop", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		fillRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
		ST7735_WriteString((ST7735_HEIGHT - strlen("STOP") * 7)/2, 117, "STOP", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		while (1) {
			if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK) break;
		}
		fillRoundRect(49, 114, 62, 14, 4, MAIN_BG_COLOR);
		drawRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
		ST7735_WriteString((ST7735_HEIGHT - strlen("STOP") * 7)/2, 117, "STOP", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	} else {
		while (darw_timeout < timeout_t && timeout_t != -1) {
			if (HAL_GetTick() - time > 1000 && darw_timeout != timeout_t) {
				darw_timeout++;
				sprintf(str, "timeout: %ds/%ds", darw_timeout, timeout_t);
				ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 46, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
				time = HAL_GetTick();
			}
		}
	}

	sprintf(str, "%s", STOPTASK);
	res = ESP32_WriteRecieve_AT(str, strlen(str));
	if (res != AT_OK) {
		fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("stop attack") * 7) / 2, 46, "stop attack", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
	} else {
		drawIcon("ATTACK STATUS", 0, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("DONE") * 7) / 2, 34, "DONE", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	while (1) {
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return;
	}
}

void ESP32_start_beacon_attack(const int timeout_t) {

	int res;
	char str[30];

	ClearAllButtons();

	fillRect(0, 13, 160, 128 - 13, MAIN_BG_COLOR);
	drawIcon("ATTACK STATUS", 1, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("prepair...") * 7) / 2, 34, "prepair...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	if (timeout_t != -1 && (timeout_t < MIN_TIMEOUT_SEC || timeout_t > MAX_DEAUTH_TIMEOUT_SEC)) return;

	sprintf(str, "%s%d", BASTART, timeout_t);
	res = ESP32_WriteRecieve_AT(str, strlen(str));
	if (res != AT_OK) {
		fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("start attack") * 7) / 2, 46, "start attack", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		while (1) {
			if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return; //exit
		}
	}

	fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("beacon...") * 7) / 2, 34, "beacon...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	uint32_t time = HAL_GetTick();
	uint8_t darw_timeout = 0;

	if (timeout_t == -1) {
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("button3 to stop") * 7) / 2, 46, "button3 to stop", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		fillRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
		ST7735_WriteString((ST7735_HEIGHT - strlen("STOP") * 7)/2, 117, "STOP", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		while (1) {
			if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK) break;
		}
		fillRoundRect(49, 114, 62, 14, 4, MAIN_BG_COLOR);
		drawRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
		ST7735_WriteString((ST7735_HEIGHT - strlen("STOP") * 7)/2, 117, "STOP", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	} else {
		while (darw_timeout < timeout_t && timeout_t != -1) {
			if (HAL_GetTick() - time > 1000 && darw_timeout != timeout_t) {
				darw_timeout++;
				sprintf(str, "timeout: %ds/%ds", darw_timeout, timeout_t);
				ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 46, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
				time = HAL_GetTick();
			}
		}
	}

	sprintf(str, "%s", STOPTASK);
	res = ESP32_WriteRecieve_AT(str, strlen(str));
	if (res != AT_OK) {
		fillRect(13, 34, 134, 24, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("ERROR") * 7) / 2, 34, "ERROR", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("stop attack") * 7) / 2, 46, "stop attack", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
	} else {
		drawIcon("ATTACK STATUS", 0, 11, 17, 149, 60, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("DONE") * 7) / 2, 34, "DONE", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	while (1) {
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return;
	}
}

void ESP32_attacks_loop() {
	const static int ATTACK_COUNT = 3;
	const char* ATTACK_NAMES[] = {"<deauth>", "<beacon>", "<handshake>"};
	const char* METHODS[] = {"<passive>", "<deauth>"};
	const int ATTACK_MAX_SET_COUNT[] = {1, 1, 2};

	int current_menu = 0; //current menu: 0 - attack; 1 - attack settings; 2 - start
	static int current_attack = 0; //0 - DEAUTN; 1 - BEACON; 2 - HANDSHAKE
	static int attack_curent_set[] = {0, 0, 0};

	static int handshake_timeout = 10;
	static int timeout = -1;
	static int method = 0;
	static char* attack_settings[2] = {};
	attack_settings[0] = (char*)malloc(20*sizeof(char));
	attack_settings[1] = (char*)malloc(20*sizeof(char));
	bool start_attack = 0;

	bool update_attack = 1; //draw update
	bool update_attack_menu = 1;
	bool update_settings = 1;
	bool update_settings_menu = 1;
	bool update_start_button = 1;


	fillScreen(MAIN_BG_COLOR);
	drawHeader("Wi-Fi attacks", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);

	ClearAllButtons();
	eButtonEvent button_right, button_left, button_confirm, button_up, button_down;
	uint32_t hold_timer = 0;

	while (1) {

		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_confirm = GetButtonState(BUTTON_CONFIRM);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);

		if (button_right == SINGLE_CLICK && current_menu == 0) { //attack chose menu
			current_attack = (current_attack + 1) % ATTACK_COUNT;
			update_attack = 1;
			update_settings = 1;
			update_start_button = 1;
		}

		if (button_left == SINGLE_CLICK && current_menu == 0) {
			current_attack = (current_attack - 1) % ATTACK_COUNT;
			if (current_attack < 0) current_attack = ATTACK_COUNT - 1;
			update_attack = 1;
			update_settings = 1;
			update_start_button = 1;
		}

		if (button_right == SINGLE_CLICK  && current_menu == 1) { //attack settings menu
			attack_curent_set[current_attack] = (attack_curent_set[current_attack] + 1) % ATTACK_MAX_SET_COUNT[current_attack];
			update_settings = 1;
		}

		if (button_left == SINGLE_CLICK && current_menu == 1) {
			attack_curent_set[current_attack] = (attack_curent_set[current_attack] - 1) % ATTACK_MAX_SET_COUNT[current_attack];
			if (attack_curent_set[current_attack] < 0) attack_curent_set[current_attack] = ATTACK_MAX_SET_COUNT[current_attack] - 1;
			update_settings = 1;
		}

		if (button_down == SINGLE_CLICK) {
			if (target == -1 && current_attack != 1) {
				current_menu = (current_menu + 1) % 2;
			} else {
				current_menu = (current_menu + 1) % 3;
			}
			update_attack = 1;
			update_attack_menu = 1;
			update_settings = 1;
			update_settings_menu = 1;
			update_start_button = 1;
		}

		if (button_up == SINGLE_CLICK) {
			if (target == -1 && current_attack != 1) {
				current_menu = (current_menu - 1) % 2;
				if (current_menu < 0) current_menu = 1;
			} else {
				current_menu = (current_menu - 1) % 3;
				if (current_menu < 0) current_menu = 2;
			}
			update_attack = 1;
			update_attack_menu = 1;
			update_settings = 1;
			update_settings_menu = 1;
			update_start_button = 1;
		}

		if (button_confirm == SINGLE_CLICK && current_menu == 2) start_attack = 1;

		if ((button_right == SINGLE_CLICK || (button_right == HOLD && HAL_GetTick() - hold_timer > HOLD_FAST_DELLAY)) && current_menu == 1 && attack_curent_set[current_attack] == 0) {
			if (current_attack != 2) {
				if (timeout == -1)
					timeout = 10;
				else
					timeout += 10;

				if (timeout > MAX_DEAUTH_TIMEOUT_SEC)
					timeout = -1;
			} else {
				handshake_timeout = (handshake_timeout + 10) % MAX_HANDSHAKE_TIMEOUT_SEC;
				if (handshake_timeout < 10) handshake_timeout = 10;
			}
			update_settings = 1;

			hold_timer = HAL_GetTick();
		}

		if ((button_left == SINGLE_CLICK || (button_left == HOLD && HAL_GetTick() - hold_timer > HOLD_FAST_DELLAY)) && current_menu == 1 && attack_curent_set[current_attack] == 0) {
			if (current_attack != 2) {
				if (timeout == -1)
					timeout = MAX_DEAUTH_TIMEOUT_SEC;
				else
					timeout -= 10;

				if (timeout < MIN_TIMEOUT_SEC)
					timeout = -1;
			} else {
				handshake_timeout = (handshake_timeout - 10) % MAX_HANDSHAKE_TIMEOUT_SEC;
				if (handshake_timeout < 10) handshake_timeout = MAX_HANDSHAKE_TIMEOUT_SEC;
			}
			update_settings = 1;

			hold_timer = HAL_GetTick();
		}

		if ((button_right == SINGLE_CLICK) && current_menu == 1 && attack_curent_set[current_attack] == 1) {
			method = (method + 1) % 2;
			update_settings = 1;
		}

		if ((button_left == SINGLE_CLICK) && current_menu == 1 && attack_curent_set[current_attack] == 1) {
			method = (method - 1) % 2;
			if (method < 0) method = 1;
			update_settings = 1;
		}

		if (button_right == DOUBLE_CLICK) break;

		if (update_attack_menu) {
			drawIcon("ATTACK", current_menu == 0 ? 1 : 0, 11, 17, 149, 48, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			update_attack_menu = 0;
		}

		if (update_settings_menu) {
			drawIcon("ATTACK SETTINGS", current_menu == 1 ? 1 : 0, 11, 51, 149, 108, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			update_settings_menu = 0;
		}

		if (update_attack) {
			fillRect(13, 34, ST7735_HEIGHT - 26, 10, MAIN_BG_COLOR);
			ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(ATTACK_NAMES[current_attack]) * 7) / 2, 34,
					ATTACK_NAMES[current_attack],
					Font_7x10,
					current_menu == 0 ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
					current_menu == 0 ? MAIN_TXT_COLOR : MAIN_BG_COLOR);
			update_attack = 0;
		}

		if (update_settings) {
			fillRect(13, 69, 134, 12*3, MAIN_BG_COLOR);
			if (target == -1 && current_attack != 1) {
				ST7735_WriteString(13, 69, "NO TARGET SELECTED", Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
				drawFastHLine(12, 80, 149 - 13, MAIN_TXT_COLOR);
			} else if (current_attack != 1) {
				char str[18 + 1] = {};
				sprintf(str, "target");
				strncpy(str + strlen("target"), scaned_AP[target], 18 - strlen("target"));
				ST7735_WriteString(13, 69, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
				drawFastHLine(12, 80, 149 - 13, MAIN_TXT_COLOR);
			}

			if (current_attack == 2) {
				sprintf(attack_settings[0], "timeout <%03ds>", handshake_timeout);
			} else if (timeout != -1) {
				sprintf(attack_settings[0], "timeout <%03ds>", timeout);
			} else {
				sprintf(attack_settings[0], "timeout <none>");
			}
			sprintf(attack_settings[1], "method %7s", METHODS[method]);

			if (current_attack == 1)
				drawMenu(attack_settings, ATTACK_MAX_SET_COUNT[current_attack], 13, 69, 127, 69 + 12*2, current_menu == 1 ? attack_curent_set[current_attack] : -1, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			else
				drawMenu(attack_settings, ATTACK_MAX_SET_COUNT[current_attack], 13, 82, 127, 82 + 12*2, current_menu == 1 ? attack_curent_set[current_attack] : -1, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

			update_settings = 0;
		}

		if (update_start_button){
			fillRoundRect(49, 114, 62, 14, 4, MAIN_BG_COLOR);
			if ((target != -1 || current_attack == 1) && current_menu != 2) {
				drawRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
				ST7735_WriteString((ST7735_HEIGHT - strlen("START") * 7)/2, 117, "START", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			} else if (current_menu == 2) {
				fillRoundRect(49, 114, 62, 14, 4, MAIN_TXT_COLOR);
				ST7735_WriteString((ST7735_HEIGHT - strlen("START") * 7)/2, 117, "START", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			}
			update_start_button = 0;
		}


		if (start_attack) { //0 - DEAUTN; 1 - BEACON; 2 - HANDSHAKE
			if (current_attack == 0) {
				ESP32_start_deauth_attack(timeout);
			} else if (current_attack == 1) {
				ESP32_start_beacon_attack(timeout);
			} else if (current_attack == 2) {
				ESP32_start_handshake_attack(handshake_timeout, method + 1);
			}
			ClearAllButtons();
			fillRect(0, 13, 160, 128 - 13, MAIN_BG_COLOR);
			start_attack = 0;
			update_attack = 1;
			update_attack_menu = 1;
			update_settings = 1;
			update_settings_menu = 1;
			update_start_button = 1;
		}
	}
	ClearAllButtons();
}

bool ESP32_ScanWiFi() {
	if (ESP32_WriteRecieve_AT("AT+WIFI_SCAN", strlen("AT+WIFI_SCAN")) != AT_OK) return 0;

	for (int i = 0; i < AP_count; i++)
		free(*(scaned_AP + i));
	free(scaned_AP);
	scaned_AP = NULL;

	if (!sscanf(strstr((char *)rxBuff, "AT/") + 5, "%d", &AP_count)) return 0;

	scaned_AP = (char**) malloc(AP_count * sizeof(char*));

	const char* ssid_search_p = (char *)rxBuff;
	char SSID_len;
	for (int i = 0; i < AP_count; i++) {
		SSID_len = 0;
		ssid_search_p = strstr(ssid_search_p, "_SSID:") + strlen("_SSID:");// + 1;
		const char* ssid_search_p_copy = ssid_search_p;
		for (int j = 0; j < 20; j++)
			if (*(ssid_search_p + j) != '\n')
				SSID_len++;
			else
				break;
		*(scaned_AP + i) = (char*) malloc((SSID_len + 1) * sizeof(char));
		*(*(scaned_AP + i) + SSID_len) = '\0';
		strncpy(*(scaned_AP + i), ssid_search_p_copy, SSID_len);
	}

	return 1;
}

void ESP32_scan_loop() {
	static int submenu = 1;
	static int choose_set = 0;
	static int max_set = 0;
	static int previous_target = -1;

	ClearAllButtons();
	eButtonEvent button_right, button_confirm, button_up, button_down;
	uint32_t hold_timer = 0;

	fillScreen(MAIN_BG_COLOR);
	ST7735_WriteString(48, 0, "Scan", Font_16x26, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	drawRect(0, 29, 160, 99, MAIN_TXT_COLOR);
	drawRect(0, 29, 160, 74, MAIN_TXT_COLOR);

	while (1) {
		/// buttons handling ///
		button_right = GetButtonState(BUTTON_RIGHT);
		button_confirm = GetButtonState(BUTTON_CONFIRM);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);

		if (button_down == DOUBLE_CLICK || button_up == DOUBLE_CLICK) {
			submenu = (submenu + 1) % 2;
		}

		if (button_down == SINGLE_CLICK || (button_down == HOLD && HAL_GetTick() - hold_timer > HOLD_MENU_DELLAY)) {
			choose_set = (choose_set + 1) % max_set;

			hold_timer = HAL_GetTick();
		}

		if (button_up == SINGLE_CLICK || (button_up == HOLD && HAL_GetTick() - hold_timer > HOLD_MENU_DELLAY)) {
			choose_set = (choose_set - 1) % max_set;
			if (choose_set < 0) choose_set = max_set;

			hold_timer = HAL_GetTick();
		}

		if (button_confirm == SINGLE_CLICK) {
			if (submenu) {
				fillRect(1, 30, 158, 72, MAIN_BG_COLOR);
				ST7735_WriteString(40, 60, "Scan start...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
				ESP32_ScanWiFi();
				max_set = AP_count;
				ST7735_WriteString(40, 60, "Scan start...", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
			} else {
				if (choose_set != target) {
					scaned_AP[choose_set][0] = '>';
					target = choose_set;
					if (previous_target != -1) {
						scaned_AP[previous_target][0] = ' ';
					}
					previous_target = choose_set;
				} else {
					scaned_AP[choose_set][0] = ' ';
				}
			}
		}

		if (button_right == DOUBLE_CLICK) break;
		////////////////////////
		if (!AP_count)
			ST7735_WriteString(43, 60, "No AP scanned", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		else
			drawMenu(scaned_AP, max_set, 1, 30, 159, 104, (!submenu) ? (choose_set) : (-1), Font_7x10, 	MAIN_TXT_COLOR, MAIN_BG_COLOR);

		if (submenu)
			ST7735_WriteString(1, 104, "Rescan", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		else
			ST7735_WriteString(1, 104, "Rescan", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}
	ClearAllButtons();
}

void ESP32_pkt_analyzer_loop() {
	const int X_POS = 26;
	const int Y_POS = 20;
	const int GRAPH_HEIGHT = 80;
	const int GRAPH_SCALE = 2;

	uint32_t screenUpdateCounter = 0;
	static int ch;
	ch = currentCh + 1;
	char str[25] = {};

	ClearAllButtons();

	fillScreen(MAIN_BG_COLOR);
	sprintf(str, "%s%02d%c", "<channel_", ch, '>');
	ST7735_WriteString(50, 120, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	ST7735_WriteString(0, 1, "c:00 p:000 m:000", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(19*5, 1, "d:00 m:00", Font_5x7, MAIN_ERROR_COLOR, MAIN_BG_COLOR);

	drawRect(X_POS, Y_POS, 122, 82, MAIN_TXT_COLOR);
	for (int i = 0; i <= 120; i += 30) {
		drawFastVLine(X_POS + i + 1, Y_POS + 82, 3, MAIN_TXT_COLOR);
		if (i == 120) {
			sprintf(str, "%s", "now");
		} else {
			sprintf(str, "%d%c", (i - 120) / 2, 's');
		}
		ST7735_WriteString(X_POS + i + 1 - 5 * strlen(str) / 2, Y_POS + 86, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	drawFastHLine(X_POS - 3, Y_POS + 1, 3, MAIN_TXT_COLOR);
	drawFastHLine(X_POS - 3, Y_POS + 1 + GRAPH_HEIGHT / 2, 3, MAIN_TXT_COLOR);
	drawFastHLine(X_POS - 3, Y_POS + GRAPH_HEIGHT, 3, MAIN_TXT_COLOR);
	sprintf(str, "%d", GRAPH_SCALE);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + GRAPH_HEIGHT - 14, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + GRAPH_HEIGHT - 6, "p/s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(str, "%d", GRAPH_HEIGHT * GRAPH_SCALE);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + 1, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + 1 + 8, "p/s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(str, "%d", GRAPH_HEIGHT * GRAPH_SCALE / 2);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + GRAPH_HEIGHT / 2 - 7, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(X_POS - 4 - 3 * 5, Y_POS + GRAPH_HEIGHT / 2, "p/s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	sprintf(str, "%s%d", "AT+PKT_ANALYZER_START->", currentCh + 1);
	ESP32_WriteRecieve_AT(str, strlen(str));
	PktAnalyzerParsing = 1;

	while (1) {
		if (GetButtonState(BUTTON_RIGHT) == SINGLE_CLICK) {
			ch = (ch + 1) % (MAXWIFICH + 1);
			ST7735_WriteString(50, 120, "<", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			ST7735_WriteString(50 + 11*5, 120, ">",Font_5x7, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		}
		if (GetButtonState(BUTTON_LEFT) == SINGLE_CLICK) {
			ch = (ch - 1) % (MAXWIFICH + 1);
			if (ch < 1) ch = MAXWIFICH;
			ST7735_WriteString(50, 120, "<", Font_5x7, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			ST7735_WriteString(50 + 11*5, 120, ">", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
		if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK) {
			currentCh = ch - 1;
			PktAnalyzerParsing = 0;
			sprintf(str, "%s%d", "AT+PKT_ANALYZER_START->", currentCh + 1);
			ESP32_WriteRecieve_AT(str, strlen(str));
			memset(rxBuff, 0, AT_UART_Buff_size);
			PktAnalyzerParsing = 1;
			HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
			fillRect(X_POS + 1, Y_POS + 1, 120, 80, MAIN_BG_COLOR);
		}

		if (HAL_GetTick() - screenUpdateCounter > 1000) {
			screenUpdateCounter = HAL_GetTick();
			int drawPackets = 0;
			int drawDeauthPackets = 0;
			for (int i = 0; i < PKTS; i++) {
				if (pkts[currentCh][i] < 1) continue;

				if (pkts[currentCh][i] > 0 && pkts[currentCh][i] <= GRAPH_SCALE)
					drawPackets = GRAPH_SCALE;
				else
					drawPackets = pkts[currentCh][i] / GRAPH_SCALE;
				if (drawPackets > GRAPH_HEIGHT) drawPackets = GRAPH_HEIGHT;

				if (deauthPkts[currentCh][i] > 0 && deauthPkts[currentCh][i] <= GRAPH_SCALE)
					drawDeauthPackets = GRAPH_SCALE;
				else
					drawDeauthPackets = deauthPkts[currentCh][i] / GRAPH_SCALE;
				if (drawPackets > GRAPH_HEIGHT) drawPackets = GRAPH_HEIGHT;

				drawFastVLine(X_POS + 1 + i*2, Y_POS + 1, GRAPH_HEIGHT, MAIN_BG_COLOR);
				drawFastVLine(X_POS + 1 + i*2 + 1, Y_POS + 1, GRAPH_HEIGHT, MAIN_BG_COLOR);

				drawFastVLine(X_POS + 1 + i*2, Y_POS + 1 + GRAPH_HEIGHT - drawPackets, drawPackets, MAIN_TXT_COLOR);
				drawFastVLine(X_POS + 1 + i*2 + 1, Y_POS + 1 + GRAPH_HEIGHT - drawPackets, drawPackets, MAIN_TXT_COLOR);

				if (drawDeauthPackets > 0) {
					drawFastVLine(X_POS + 1 + i*2, Y_POS + 1 + GRAPH_HEIGHT - drawDeauthPackets, drawDeauthPackets, MAIN_ERROR_COLOR);
					drawFastVLine(X_POS + 1 + i*2 + 1, Y_POS + 1 + GRAPH_HEIGHT - drawDeauthPackets, drawDeauthPackets, MAIN_ERROR_COLOR);
				}
			}

			sprintf(str, "%s%02d", "channel_", ch);
			ST7735_WriteString(50 + 5, 120, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);

			sprintf(str, "c:%02d p:%03d m:%03d", currentCh + 1, pkts[currentCh][PKTS-1], maxPkts[currentCh]);
			ST7735_WriteString(0, 1, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);

			sprintf(str, "d:%02d m:%02d", deauthPkts[currentCh][PKTS-1], maxDeauthPkts[currentCh]);
			ST7735_WriteString(19*5, 1, str, Font_5x7, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		}

		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) {
			ESP32_WriteRecieve_AT("AT+STOP_CURRENT_TASK", strlen("AT+STOP_CURRENT_TASK"));
			PktAnalyzerParsing = 0;
			break;
		}
	}
	ClearAllButtons();
}

void ESP32_start_pmkid_attack() {
	uint8_t retval = 0;
	bool error = 0;

	bool mount_sd = 0;
	bool create_file = 0;

	uint16_t PMKID_count = 0;
	char str[30] = {};

	char file_name[255] = {}; //255 - maximum LFN file name
	FATFS *fs;
	FIL *my_file;
	uint32_t byteswritten;

	fs = malloc(sizeof(FATFS));
	my_file = malloc(sizeof(FIL));

	eeprom_settings_t eeprom_settings;
	eeprom_read_settings(&eeprom_settings);

	ClearAllButtons();

	fillRect(0, 13, 160, 128 - 13, MAIN_BG_COLOR);
	drawIcon("ATTACK STATUS", 1, 11, 17, 149, 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("prepare...") * 7) / 2, 34, "prepare...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	FATFS_LinkDriver(&SD_Driver, SDPath);
	retval = f_mount(fs, (TCHAR const*)SDPath, 1);

	if (retval == FR_OK) {
		mount_sd = 1;
		retval = f_mkdir(FOLDER_NAME);
	} else {
		error = 1;
		sprintf(str, "SD mount error %d", retval);
	}

	if ((retval == FR_OK || retval == FR_EXIST) && !error) {
		sprintf(file_name, "%s/pmkid_scan_%lu.hc22000", FOLDER_NAME, eeprom_settings.hc22000_pmk_counter);
		retval = f_open(my_file, file_name, FA_CREATE_NEW | FA_WRITE);
	} else if (!error) {
		error = 1;
		sprintf(str, "SD mkdir error %d", retval);
	}

	if ((retval == FR_OK ) && !error) { // || retval == FR_EXIST
		create_file = 1;
		retval = ESP32_WriteRecieve_AT("AT+PMKID_ATTACK", strlen("AT+PMKID_ATTACK"));
	} else if (!error) {
		error = 1;
		sprintf(str, "SD file error %d", retval);
	}

	if (retval == AT_OK && !error) {
		memset(rxBuff, 0, AT_UART_Buff_size);
		recieved_pkt_size = 0;
		HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen("attack run...") * 7) / 2, 34, "attack run...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(15, 58, "current AP:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(str, "PMKID captured: %d", PMKID_count);
		ST7735_WriteString(15, 46, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	} else if (!error) {
		error = 1;
		sprintf(str, "ESP AT error %d", retval);
	}

	if (error)
		ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 34, str, Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);

	while (!error) {
		if (recieved_pkt_size) {
			if (strstr((const char *)rxBuff, (const char *)"WPA*01*")) {
				retval = f_write(my_file, rxBuff, recieved_pkt_size, (void*)&byteswritten);
				if (retval != FR_OK) {
					error = 1;
					sprintf(str, "SD write error %d", retval);
					ST7735_WriteString(5 + (ST7735_HEIGHT - 11 - strlen(str) * 7) / 2, 34, str, Font_7x10, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
					break;
				}
				PMKID_count++;
				sprintf(str, "PMKID captured: %d", PMKID_count);
				ST7735_WriteString(15, 46, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			} else {
				memset(str, 0, 30);
				strncpy(str, (char *)rxBuff, 17);
				str[strlen((char *)rxBuff)-1] = 0;
				fillRect(15, 70, 17*7, 10, MAIN_BG_COLOR);
				ST7735_WriteString(15, 70, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			}
			recieved_pkt_size = 0;
			memset(rxBuff, 0, AT_UART_Buff_size);
			HAL_UARTEx_ReceiveToIdle_IT(&ESP32_AT_UART_Port, rxBuff, AT_UART_Buff_size);
		}
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;
	}

	ESP32_WriteRecieve_AT(STOPTASK, strlen(STOPTASK));
	if (create_file) f_close(my_file);
	if (create_file && error) f_unlink(file_name);
	if (mount_sd) f_mount(0, (TCHAR const*)SDPath, 1);
	FATFS_UnLinkDriver(SDPath);

	if (!error) {
		eeprom_settings.hc22000_pmk_counter++;
		eeprom_write_settings(&eeprom_settings);
	}

	free(fs);
	free(my_file);
	memset(rxBuff, 0, AT_UART_Buff_size);

	while (error) {
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;
	}
	ClearAllButtons();
}
