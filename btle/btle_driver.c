/*
 * btle_driver.c

 *
 *  Created on: Nov 4, 2014
 *      Author: jcobb
 */

#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "../util/defines.h"
#include "../queue/queue.h"
#include "../usart/usart_btle.h"
#include "btle_driver.h"
#include "btle.h"
#include "btle_msg.h"

char HEX_DIGITS[] = "0123456789abcdef";

#define BTLE_MAX_CHARS		128

char btle_lines[BTLE_MAX_CHARS+1];
int btle_index = 0;
uint8_t new_line = 0;

char btle_line_buffer[BTLE_MAX_CHARS+1];
int btle_line_index = 0;

static void init_buffer();
static void init_lines();
static bool handle_data();
static uint8_t parse_data(char *token, char **out);

static uint8_t btle_parse_nybble(char c);
static btle_msg_t btle_handle_le_packet(char * buffer);

void btle_driver_init()
{
	queue_init(&btle_queue, MSG_QUEUE_SIZE);
	init_buffer();
	init_lines();
}

void btle_driver_tick()
{
	if(btle_usart_data_available()) {
		if(handle_data()) {

			char *ptr = NULL;
			// handle the new line
			// TODO: review warning
			if(parse_data(BTLE_TKSTART, &ptr) == BTLE_TKFOUND) {
				// TODO: handle the message
				btle_msg_t msg = btle_handle_le_packet(ptr);

				if(msg.mac != 0) {
					queue_results_t result = btle_enqueue(&msg);
				}
				init_lines();
			}
		}
	}
}

static void init_buffer()
{
	btle_line_index = 0;
	memset(btle_line_buffer, '\0', sizeof(btle_line_buffer));
}

static void init_lines()
{
	memset(btle_lines, '\0', sizeof(btle_lines));
}

// check to see if we have a new line
bool handle_data()
{

	char c = btle_usart_data_read();

	// ignore null terminated strings
	if(c == '\0') return false;
	// prevent buffer overrun
	if(btle_line_index >= BTLE_MAX_CHARS) return false;

	// store character in btle_line_buffer
	btle_line_buffer[btle_line_index] = c;
	btle_line_index++;

	// check for end of line
	if(c == BTLE_TKEND) {
		// copy new message into buffer
		strcpy(btle_lines, btle_line_buffer);
		init_buffer();
		return true;
	}

	return false;
}

static uint8_t parse_data(char *token, char **out)
{
	uint8_t *ptr = NULL;
	// TODO: review warning
	if((ptr == strstr(btle_lines, token)))
	{
		if(out != NULL) *out = ptr;
		return BTLE_TKFOUND;
	}
	else
		return BTLE_TKNOTFOUND;
}


btle_msg_t btle_handle_le_packet(char * buffer)
{
	btle_msg_t btle_msg;

	memset(&btle_msg, '0', sizeof(btle_msg_t));

	//           1111111111222222222
	// 01234567890123456789012345678
	// |||||||||||||||||||||||||||||
	// *00078072CCB3 C3 5994 63BC 24

	uint8_t * num;
	uint8_t msb, lsb, ck, ckx;
	uint8_t rssi;
	uint16_t batt, temp;
	uint64_t mac;
	int i;

	// Validate checksum in bytes 27-28
	// Just an XOR of bytes 0-26
	msb = btle_parse_nybble(buffer[27]);	lsb = btle_parse_nybble(buffer[28]);
	ck = (msb << 4) | lsb;
	ckx = 0;
	for (i=0;i<=26;i++)
		ckx ^= buffer[i];
	if (ck != ckx)
	{
		//AT_debugString("BAD CK ");
		//AT_writeHex(ck);
		//AT_debugString(" ");
		//AT_writeHex(ckx);
		//AT_debugString("\n");
		return btle_msg;
	}

	// MAC address - incoming 48bits
	//
	num = (uint8_t *)&mac;
	num[7] = 0;
	num[6] = 0;
	msb = btle_parse_nybble(buffer[1]);	lsb = btle_parse_nybble(buffer[2]);
	num[5] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[3]);	lsb = btle_parse_nybble(buffer[4]);
	num[4] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[5]);	lsb = btle_parse_nybble(buffer[6]);
	num[3] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[7]);	lsb = btle_parse_nybble(buffer[8]);
	num[2] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[9]);	lsb = btle_parse_nybble(buffer[10]);
	num[1] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[11]);lsb = btle_parse_nybble(buffer[12]);
	num[0] = (msb << 4) | lsb;

	// RSSI
	//
	msb = btle_parse_nybble(buffer[14]);	lsb = btle_parse_nybble(buffer[15]);
	rssi = (msb << 4) | lsb;


	// Temperature
	//
	num = (uint8_t *)&temp;
	msb = btle_parse_nybble(buffer[17]);	lsb = btle_parse_nybble(buffer[18]);
	num[0] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[19]);	lsb = btle_parse_nybble(buffer[20]);
	num[1] = (msb << 4) | lsb;

	// Battery
	//
	num = (uint8_t *)&batt;
	msb = btle_parse_nybble(buffer[22]);	lsb = btle_parse_nybble(buffer[23]);
	num[0] = (msb << 4) | lsb;
	msb = btle_parse_nybble(buffer[24]);	lsb = btle_parse_nybble(buffer[25]);
	num[1] = (msb << 4) | lsb;


	btle_msg.rssi = rssi;
	btle_msg.mac = mac;
	btle_msg.batt = batt;
	btle_msg.temp = temp;

	return btle_msg;

}

uint8_t btle_parse_nybble(char c)
{
	if (c >= 'A' && c <= 'F')
		c = c | 0x20;
	for (uint8_t i=0; i<16; i++)
	{
		if (HEX_DIGITS[i] == c)
			return i;
	}
	return 0x80;
}


