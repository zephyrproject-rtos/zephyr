/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

extern uint32_t central_notify_receive(uint32_t count);

int main(void)
{
	(void)central_notify_receive(0U);
	return 0;
}
