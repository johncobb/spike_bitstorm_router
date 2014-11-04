/*
 * main.c
 *
 *  Created on: Nov 3, 2014
 *      Author: jcobb
 */

#define F_CPU		800000

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "util/clock.h"
#include "util/log.h"
#include "util/config.h"
#include "queue/queue.h"
#include "btle/btle.h"
#include "usart/usart_btle.h"



static const char _tag[] PROGMEM = "main: ";
volatile char term_in = 0;

// timeout helper
volatile clock_time_t future = 0;
bool timeout();
void set_timer(clock_time_t timeout);


void enqueue_msg();


void terminal_in_cb(uint8_t c)
{
	term_in = c;
	LOG("input=%c\r\n", c);
}




void main()
{
	debug_init(terminal_in_cb);
	init_btle_usart(terminal_in_cb);

	clock_init();

	/*
	 * load configuration
	 */
	LOG("config_init...\r\n");
	config_init();
	btle_init();
	sei();


	while(true){
		if(timeout()){
			btle_tick();
			// DO SOMETHING
			set_timer(1000);
		}
	}
}

void set_timer(clock_time_t timeout)
{
	future = clock_time() + timeout;
}

// timeout routine to demonstrate clock_time
// being kept by pwm isr interrupt
bool timeout()
{
	bool timeout = false;

	if(clock_time() >= future)
	{
		set_timer(1000);
		timeout = true;

	}

	return timeout;
}



