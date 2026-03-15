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
 * ESP32-C5 SoC ROM region.
 *
 * The ESP32-C5 has a ROM at 0x40000000 containing libc and other utility
 * functions. This region needs to be accessible (R+X) from both kernel
 * and user mode for proper operation.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)

PMP_SOC_REGION_DEFINE(esp32c5_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + DT_REG_SIZE(SOC_ROM_NODE), PMP_R | PMP_X);

/*
 * ESP32-C5 IRAM text region.
 *
 * On ESP32-C5, IRAM and DRAM share the same 384KB physical memory space
 * (0x40800000-0x40860000). The split between code (IRAM) and data (DRAM)
 * is determined at link time. Only the IRAM text portion should be
 * executable.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32c5_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/*
 * ESP32-C5 peripheral region.
 *
 * Memory-mapped I/O registers (UART, SPI, I2C, GPIO, PMU, etc.) live
 * in the 0x60000000-0x60100000 range. An explicit PMP entry is needed
 * because PMP_NO_LOCK_GLOBAL enforces U-mode checks via MPRV and the
 * catch-all entry alone is not sufficient for reliable access.
 */
PMP_SOC_REGION_DEFINE(esp32c5_periph, (const void *)0x60000000, (const void *)0x60100000,
		      PMP_R | PMP_W);
