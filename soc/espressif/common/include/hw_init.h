/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ESPRESSIF_COMMON_HW_INIT_H_
#define _SOC_ESPRESSIF_COMMON_HW_INIT_H_

struct rom_segments {
	unsigned int irom_map_addr;     /* Mapped address (VMA) for IROM region */
	unsigned int irom_flash_offset; /* Flash offset (LMA) for IROM region */
	unsigned int irom_size;         /* Size of IROM region */
	unsigned int drom_map_addr;     /* Mapped address (VMA) for DROM region */
	unsigned int drom_flash_offset; /* Flash offset (LMA) for DROM region */
	unsigned int drom_size;         /* Size of DROM region */
};

void map_rom_segments(int core, struct rom_segments *map);

int hardware_init(void);

#endif /* _SOC_ESPRESSIF_COMMON_HW_INIT_H_ */
