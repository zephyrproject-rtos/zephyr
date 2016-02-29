/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>

#include <device.h>
#include <uart.h>
#include <zephyr.h>

static const char *banner1 = "Send any character to the UART device\n";
static const char *banner2 = "Character read:\n";

#ifdef CONFIG_BOARD_QUARK_SE_DEVBOARD
#define UART_DEVICE "UART_1"
#elif CONFIG_BOARD_QUARK_D2000_CRB
#define UART_DEVICE "UART_0"
#else
/* For any other board not specified above, we use UART_0 by default. */
#define UART_DEVICE "UART_0"
#endif

static void write_string(struct device *dev, const char *str, int len)
{
	for (int i = 0; i < len; i++)
		uart_poll_out(dev, str[i]);
}

void main(void)
{
	struct device *dev = device_get_binding(UART_DEVICE);
	unsigned char data;

	write_string(dev, banner1, strlen(banner1));

	/* Poll in the character */
	while (uart_poll_in(dev, &data) == -1)
		;

	write_string(dev, banner2, strlen(banner2));
	write_string(dev, &data, 1);
	write_string(dev, "\n", 1);
}
