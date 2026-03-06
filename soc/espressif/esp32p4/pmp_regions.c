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

/*
 * ESP32-P4 IRAM text region.
 *
 * On ESP32-P4, IRAM and DRAM share the same 768KB physical memory space
 * (0x4ff00000-0x4ffc0000). The split between code (IRAM) and data (DRAM)
 * is determined at link time. Only the IRAM text portion should be
 * executable.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32p4_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/*
 * ESP32-P4 peripheral region.
 *
 * HP peripherals at 0x50000000-0x50200000 and LP peripherals at 0x50100000+.
 * An explicit PMP entry is needed because PMP_NO_LOCK_GLOBAL enforces
 * U-mode checks via MPRV.
 */
PMP_SOC_REGION_DEFINE(esp32p4_periph, (const void *)0x50000000, (const void *)0x50200000,
		      PMP_R | PMP_W);
