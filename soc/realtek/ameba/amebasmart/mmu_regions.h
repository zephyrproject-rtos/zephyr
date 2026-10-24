/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Private API: soc_prep_hook patches runtime sizes in the static MMU table. */

#ifndef ZEPHYR_SOC_REALTEK_AMEBA_AMEBASMART_MMU_REGIONS_H_
#define ZEPHYR_SOC_REALTEK_AMEBA_AMEBASMART_MMU_REGIONS_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>

/* CA32 NS DRAM from the zephyr,sram node; must match TF-A NS_DRAM0_BASE/SIZE
 * (0x60300000, 5 MB).
 */
#define AMEBASMART_DRAM0_BASE  DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define AMEBASMART_DRAM0_SIZE  DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#define AMEBASMART_DRAM_END    (AMEBASMART_DRAM0_BASE + AMEBASMART_DRAM0_SIZE)

void amebasmart_mmu_set_psram_image2_size(size_t size);

void amebasmart_mmu_set_dram_beyond_size(uintptr_t image_ram_end);

#endif /* ZEPHYR_SOC_REALTEK_AMEBA_AMEBASMART_MMU_REGIONS_H_ */
