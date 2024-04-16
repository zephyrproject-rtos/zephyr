/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define DATA_SIZE_K(n)     (n * 1024u)
#define RSDP_SIGNATURE     ((uint64_t)0x2052545020445352)
#define EBDA_ADD           (0x040e)
#define BIOS_RODATA_ADD    (0xe0000)
#define BIOS_EXT_DATA_LOW  (0x80000UL)
#define BIOS_EXT_DATA_HIGH (0x100000UL)

static uintptr_t bios_search_rsdp_buff(uintptr_t search_phy_add, uint32_t search_length)
{
	uint64_t *search_buff;

	z_phys_map((uint8_t **)&search_buff, search_phy_add, search_length, 0);
	if (!search_buff) {
		return 0;
	}

	for (int i = 0; i < search_length / 8u; i++) {
		if (search_buff[i] == RSDP_SIGNATURE) {
			z_phys_unmap((uint8_t *)search_buff, search_length);
			return (search_phy_add + (i * 8u));
		}
	}
	z_phys_unmap((uint8_t *)search_buff, search_length);

	return 0;
}

void *bios_acpi_rsdp_get(void)
{
	uint8_t *bios_ext_data, *zero_page_base;
	uintptr_t search_phy_add, rsdp_phy_add;

	z_phys_map(&zero_page_base, 0, DATA_SIZE_K(4u), 0);
	bios_ext_data = EBDA_ADD + zero_page_base;
	search_phy_add = (uintptr_t)((*(uint16_t *)bios_ext_data) << 4u);
	z_phys_unmap(zero_page_base, DATA_SIZE_K(4u));

	if ((search_phy_add >= BIOS_EXT_DATA_LOW) && (search_phy_add < BIOS_EXT_DATA_HIGH)) {
		rsdp_phy_add = bios_search_rsdp_buff(search_phy_add, DATA_SIZE_K(1u));
		if (rsdp_phy_add) {
			return (void *)rsdp_phy_add;
		}
	}
	return (void *)bios_search_rsdp_buff(BIOS_RODATA_ADD, DATA_SIZE_K(128u));
}
