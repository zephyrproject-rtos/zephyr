/* ram_console.h - RAM console driver */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAM_CONSOLE_HEAD_STR	"RAM_CONSOLE"
#define RAM_CONSOLE_HEAD_SIZE	64

struct ram_console_header {
	char flag_string[12];
	char *buf_addr;
	size_t buf_size;
	uint32_t pos;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_ */
