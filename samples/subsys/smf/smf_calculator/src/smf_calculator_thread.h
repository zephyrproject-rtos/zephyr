/*
 * Copyright (c) 2024 Glenn Andrews
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMF_CALCULATOR_THREAD_H_
#define SMF_CALCULATOR_THREAD_H_

#include <zephyr/kernel.h>

#define SMF_THREAD_STACK_SIZE 1024
#define SMF_THREAD_PRIORITY   7

#define CALCULATOR_MAX_DIGITS 15

/* Add one byte for '-' sign, one for trailing '\0' */
#define CALCULATOR_STRING_LENGTH (CALCULATOR_MAX_DIGITS + 2)

enum calculator_events {
	DIGIT_0,
	DIGIT_1_9,
	DECIMAL_POINT,
	OPERATOR,
	EQUALS,
	CANCEL_ENTRY,
	CANCEL_BUTTON,
};

struct calculator_event {
	enum calculator_events event_id;
	char operand;
};

/* event queue to post messages to */
extern struct smf_event_queue smf_thread_event_queue;

int post_calculator_event(struct calculator_event *event, k_timeout_t timeout);

#endif /* SMF_CALCULATOR_THREAD_H_ */
