/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/devicetree.h>
#include <pmp.h>

#include "memory.h"

/*
 * ESP32-S31 SoC ROM region.
 *
 * The ESP32-S31 masked ROM lives at 0x2F800000 and contains libc and
 * other utility functions the application calls into. This region needs
 * to be accessible (R+X) from both kernel and user mode for proper
 * operation.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)

PMP_SOC_REGION_DEFINE(esp32s31_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + DT_REG_SIZE(SOC_ROM_NODE), PMP_R | PMP_X);

/*
 * ESP32-S31 IRAM text region.
 *
 * On ESP32-S31, IRAM and DRAM share the same 512KB physical memory space
 * (0x2F000000-0x2F080000). The split between code (IRAM) and data (DRAM)
 * is determined at link time. Only the IRAM text portion should be
 * executable.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32s31_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/* Flash-mapped read-only data (DROM).
 *
 * Const data and string literals live in a separate MMU window from
 * executable flash text (__rom_region). Without an explicit PMP entry,
 * user mode cannot read that window. Use _image_rodata_* so sections
 * after __rodata_region_end that still map into DROM are covered.
 */
extern char _image_rodata_start[];
extern char _image_rodata_end[];

PMP_SOC_REGION_DEFINE(esp32s31_flash_rodata, _image_rodata_start, _image_rodata_end, PMP_R);

/*
 * ESP32-S31 peripheral region.
 *
 * Memory-mapped I/O registers (UART, SPI, I2C, GPIO, PMU, LP peripherals,
 * cache and trace blocks) live in the 0x20000000-0x30000000 range. An
 * explicit PMP entry is needed because PMP_NO_LOCK_GLOBAL enforces U-mode
 * checks via MPRV and the catch-all entry alone is not sufficient for
 * reliable access.
 */
PMP_SOC_REGION_DEFINE(esp32s31_periph, (const void *)0x20000000, (const void *)0x30000000,
		      PMP_R | PMP_W);

/*
 * ESP32-S31 external RAM (PSRAM) region.
 *
 * The PSRAM cache window lives at 0x50000000-0x54000000. Code, read-only
 * data and the shared multi-heap can be placed in PSRAM, so the region
 * needs read, write and execute access. Without this entry the PSRAM
 * cannot be reached once Zephyr's PMP enforcement is active.
 */
PMP_SOC_REGION_DEFINE(esp32s31_extram, (const void *)EXTRAM_START,
		      (const void *)(EXTRAM_START + EXTRAM_SIZE), PMP_R | PMP_W | PMP_X);
