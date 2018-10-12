/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_TELNET_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_TELNET_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>

/** @brief Register telnet input processing
 *
 *  Input processing is started when string is received in telnet server.
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
void telnet_register_input(struct k_fifo *avail, struct k_fifo *lines,
			   u8_t (*completion)(char *str, u8_t len));

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_TELNET_CONSOLE_H_ */
