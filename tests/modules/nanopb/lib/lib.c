/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lib.h"

void lib_fill_message(SimpleMessage *msg)
{
	/* Reversed numbers */
	for (size_t i = 0; i < sizeof(msg->buffer); ++i) {
		msg->buffer[i] = sizeof(msg->buffer) - i;
	}
}
