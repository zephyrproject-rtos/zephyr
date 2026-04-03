/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_EEPROM_EEPROM_SIMULATOR_NATIVE_H
#define DRIVERS_EEPROM_EEPROM_SIMULATOR_NATIVE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int eeprom_mock_init_native(bool eeprom_in_ram, char **mock_eeprom, unsigned int size,
			    int *eeprom_fd, const char *eeprom_file_path, unsigned int erase_value,
			    bool eeprom_erase_at_start);

void eeprom_mock_cleanup_native(bool eeprom_in_ram, int eeprom_fd, char *mock_eeprom,
				unsigned int size, const char *eeprom_file_path,
				bool eeprom_rm_at_exit);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_EEPROM_EEPROM_SIMULATOR_NATIVE_H */
