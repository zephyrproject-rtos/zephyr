/*
 * Copyright (c) 2015 Intel Corporation.
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

/**
 * @file Header file for NSIM UART driver
 */

#ifndef _DRIVERS_UART_NSIM_H_
#define _DRIVERS_UART_NSIM_H_

#include <drivers/uart.h>

void nsim_uart_port_init(struct device *, const struct uart_init_info * const);

#endif /* _DRIVERS_UART_NSIM_H_ */
