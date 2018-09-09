/* rtt_console.h - segger rtt console driver */

/*
 * Copyright (c) 2018 qianfan Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RTT_CONSOLE__H_
#define _RTT_CONSOLE__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>

/** @brief Register segger rtt input processing
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  @param avail k_fifo queue keeping available input slots
 *  @param lines k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion doesn't support now
 *
 *  @return N/A
 */
void rtt_register_input(struct k_fifo *avail, struct k_fifo *lines,
                        u8_t (*completion)(char *str, u8_t len));

#ifdef __cplusplus
}
#endif

#endif /* _RTT_CONSOLE__H_ */