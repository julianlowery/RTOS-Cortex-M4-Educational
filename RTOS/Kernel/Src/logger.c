/*
 * logger.c
 *
 *  Created on: Dec 30, 2025
 *      Author: julianlowery
 */

// TODO: This design is useful for demos, but not useful for debug.
//		 Should also find a way to dump the buffer if we hardfault

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <stm32f4xx.h>

#define CAP 600
static char log_buf[CAP + 1];
static unsigned int buf_index = 0;

bool log_msg(char *str) {

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	unsigned int str_len = strlen(str);

	if ((buf_index + str_len) > CAP) {
		return false;
	}

	// TODO: just use memcpy!
	for (int str_index = 0; str_index < str_len; str_index++, buf_index++) {
		log_buf[buf_index] = str[str_index];
	}

	__enable_irq();
	__set_PRIMASK(primask);
	return true;
}

void log_print() {
	log_buf[buf_index] = '\0';
	fputs(log_buf, stdout);
	buf_index = 0;
}
