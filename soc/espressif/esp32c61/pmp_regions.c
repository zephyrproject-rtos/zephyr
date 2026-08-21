/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/devicetree.h>
#include <pmp.h>

/* The soc ROM holds libc and other utility functions and must stay
 * readable and executable from both kernel and user mode.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)

PMP_SOC_REGION_DEFINE(esp32c61_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + DT_REG_SIZE(SOC_ROM_NODE), PMP_R | PMP_X);

/* IRAM and DRAM share the same physical SRAM and the code/data split
 * is decided at link time, so only the linked IRAM text range may be
 * executable.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32c61_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/* The peripheral bus needs an explicit PMP entry: PMP_NO_LOCK_GLOBAL
 * enforces machine-mode checks through MPRV and the catch-all entry
 * alone is not sufficient for reliable register access.
 */
PMP_SOC_REGION_DEFINE(esp32c61_periph, (const void *)0x60000000, (const void *)0x60100000,
		      PMP_R | PMP_W);
