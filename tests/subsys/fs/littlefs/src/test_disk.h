/*
 * Copyright (c) 2026 Tawonga Kondolo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TEST_DISK_H_
#define ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TEST_DISK_H_

#include <stdint.h>

#define LITTLEFS_TEST_DISK_NAME "LITTLEFS_TEST"

#if defined(CONFIG_DISK_DRIVER_RAM)

int littlefs_test_disk_register(void);
int littlefs_test_disk_unregister(void);
int littlefs_test_disk_reset(void);
uint16_t littlefs_test_disk_refcount(void);

#endif /* CONFIG_DISK_DRIVER_RAM */

#endif /* ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TEST_DISK_H_ */
