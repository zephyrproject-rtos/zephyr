/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief RAM Console definitions
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RAM Console Header FLAG String
 */
#define RAM_CONSOLE_HEAD_STR	"RAM_CONSOLE"
/**
 * @brief RAM Console Header size in bytes
 */
#define RAM_CONSOLE_HEAD_SIZE	64

/**
 * @brief RAM Console Header struct definition
 */
struct ram_console_header {
	/**
	 * RAM Console header flag string which is RAM_CONSOLE_HEAD_STR by
	 * default.
	 */
	char flag_string[12];
	/**
	 * The start address of RAM Console buffer which is used to save
	 * console strings.
	 */
	char *buf_addr;
	/**
	 * The size of RAM Console buffer which is used to save console
	 * strings.
	 */
	size_t buf_size;
	/**
	 * The absolute position of console cursor, which is start from zero
	 * and always increasing, and back to zero when it is overflow.
	 */
	uint32_t pos;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_RAM_CONSOLE_H_ */
