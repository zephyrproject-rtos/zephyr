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

#define MAX_LINE_LEN 1024
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
 *  @param avail nano_fifo queue keeping available input slots
 *  @param lines nano_fifo queue of entered lines which to be processed
 *         in the application code.
 *
 *  @return None
 */
void uart_register_input(struct nano_fifo *avail, struct nano_fifo *lines);

void uart_console_isr(void *unused);

#ifdef __cplusplus
}
#endif

#endif /* _UART_CONSOLE__H_ */
