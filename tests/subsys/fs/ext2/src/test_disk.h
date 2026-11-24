/*
 * Copyright (c) 2026 Tawonga Kondolo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_FS_EXT2_TEST_DISK_H_
#define ZEPHYR_TESTS_SUBSYS_FS_EXT2_TEST_DISK_H_

#include <stdint.h>

#define EXT2_TEST_DISK_NAME "EXT2_TEST"

#if defined(CONFIG_DISK_DRIVER_RAM)

int ext2_test_disk_register(void);
int ext2_test_disk_unregister(void);
int ext2_test_disk_reset(void);
uint16_t ext2_test_disk_refcount(void);

#endif /* CONFIG_DISK_DRIVER_RAM */

#endif /* ZEPHYR_TESTS_SUBSYS_FS_EXT2_TEST_DISK_H_ */
