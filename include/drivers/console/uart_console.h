/* uart_console.h - uart console driver */

/*
 * Copyright (c) 2011, 2014 Wind River Systems, Inc.
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

#ifndef _UART_CONSOLE__H_
#define _UART_CONSOLE__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nanokernel.h>

#define MAX_LINE_LEN 256
struct uart_console_input {
	int _unused;
	char line[MAX_LINE_LEN];
};

/** @brief Register uart input processing
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  @param avail k_fifo queue keeping available input slots
 *  @param lines k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion callback for tab completion of entered commands
 *
 *  @return N/A
 */
void uart_register_input(struct k_fifo *avail, struct nano_fifo *lines,
			 uint8_t (*completion)(char *str, uint8_t len));

/*
 * Allows having debug hooks in the console driver for handling incoming
 * control characters, and letting other ones through.
 */
#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
#define UART_CONSOLE_DEBUG_HOOK_HANDLED 1
#define UART_CONSOLE_OUT_DEBUG_HOOK_SIG(x) int(x)(char c)
typedef UART_CONSOLE_OUT_DEBUG_HOOK_SIG(uart_console_out_debug_hook_t);

void uart_console_out_debug_hook_install(
				uart_console_out_debug_hook_t *hook);

typedef int (*uart_console_in_debug_hook_t) (uint8_t);

void uart_console_in_debug_hook_install(uart_console_in_debug_hook_t hook);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _UART_CONSOLE__H_ */
