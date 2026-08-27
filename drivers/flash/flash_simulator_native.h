/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H
#define DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H

#ifdef __cplusplus
extern "C" {
#endif

int flash_mock_init_native(bool flash_in_ram, uint8_t **mock_flash, unsigned int size,
			   int *flash_fd, const char *flash_file_path,
			   unsigned int erase_value, bool flash_erase_at_start);

void flash_mock_cleanup_native(bool flash_in_ram, int flash_fd, uint8_t *mock_flash,
			       unsigned int size, const char *flash_file_path,
			       bool flash_rm_at_exit);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FLASH_FLASH_SIMULATOR_NATIVE_H */
