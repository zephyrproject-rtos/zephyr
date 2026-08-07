/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/devicetree.h>
#include <pmp.h>

/*
 * ESP32-C6 SoC ROM region.
 *
 * The ESP32-C6 has a ROM at 0x40000000 containing libc and other utility
 * functions. This region needs to be accessible (R+X) from both kernel
 * and user mode for proper operation.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)

PMP_SOC_REGION_DEFINE(esp32c6_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + DT_REG_SIZE(SOC_ROM_NODE), PMP_R | PMP_X);

/*
 * ESP32-C6 IRAM text region.
 *
 * On ESP32-C6, IRAM and DRAM share the same 512KB physical memory space
 * (0x40800000-0x40880000). The split between code (IRAM) and data (DRAM)
 * is determined at link time. Only the IRAM text portion should be
 * executable to maintain security - making the entire region executable
 * would allow code execution from the data area.
 *
 * The linker symbols _iram_text_start and _iram_text_end define the
 * actual IRAM text boundaries.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32c6_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);
