/*
 * btle.c
 *
 *  Created on: Nov 3, 2014
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
#include "btle.h"
#include "btle_msg.h"
#include "btle_driver.h"

// queue management
queue_t btle_queue;

void btle_init()
{
	// set portd bit 5 as output
	DDRD |= _BV(PD5);
	// set portd bit 4 as input
	DDRD &= ~_BV(PD4);

	btle_driver_init();
}

void btle_set_cts()
{
	//pd5 low
	PORTD &= ~_BV(PD5);
}

uint8_t btle_get_rts()
{
	// return logic high or low
	return (PIND & _BV(PD4));
}


void btle_tick()
{
	btle_driver_tick();

	// check to see if we have a new message
	if(btle_queue.count > 0) {

		queue_header_t *qh;
		qh = btle_queue.head;

		btle_msg_t *msg = (btle_msg_t *) QUEUE_DATA(qh);
		// TODO: Handle Messages
		// push out the lw-mesh radio

		// Dequeue the message
		queue_results_t  result = queue_remove(&btle_queue, (queue_header_t*) msg);

	}
}

queue_results_t btle_enqueue(btle_msg_t *msg)
{
	//queue_results_t result = queue_enqueue(&btle_queue, &msg, sizeof(btle_msg_t));
	queue_results_t result = queue_enqueue(&btle_queue, msg, sizeof(btle_msg_t));

	return result;
}



