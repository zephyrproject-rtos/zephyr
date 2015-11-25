/* uart.c - UART based network driver */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <stdint.h>

#include <console/uart_pipe.h>

#include "dev/slip.h"

void slip_arch_writeb(unsigned char c)
{
	uint8_t buf[1] = { c };

	uart_pipe_send(&buf[0], 1);
}

void slip_arch_init(unsigned long ubr)
{
}
