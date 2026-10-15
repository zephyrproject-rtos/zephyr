/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/devicetree.h>
#include <pmp.h>

/*
 * ESP32-P4 SoC ROM region.
 *
 * The ESP32-P4 has a ROM at 0x4fc00000 containing libc and other utility
 * functions. This region needs to be accessible (R+X) from both kernel
 * and user mode for proper operation.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)

PMP_SOC_REGION_DEFINE(esp32p4_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + DT_REG_SIZE(SOC_ROM_NODE), PMP_R | PMP_X);

/* IRAM text, readable and executable from kernel and user mode. */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32p4_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/* Flash-mapped read-only data, past __rom_region_end, readable from
 * kernel and user mode.
 */
extern char _image_rodata_start[];
extern char _image_rodata_end[];

PMP_SOC_REGION_DEFINE(esp32p4_flash_rodata, _image_rodata_start, _image_rodata_end, PMP_R);
