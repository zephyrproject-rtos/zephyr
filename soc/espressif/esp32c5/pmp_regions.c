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
 * The ESP32-C5 has a 320KB ROM at 0x40000000 containing libc and other utility
 * functions. This region needs to be accessible (R+X) from both kernel and
 * user mode for proper operation.
 *
 * The size is rounded up to the next power of two (512KB) so the region
 * encodes as a single NAPOT entry instead of a two-slot TOR range. The 192KB
 * past the ROM is unmapped and denied by the locked PMA entries, so
 * over-covering is harmless and frees a PMP slot for user-mode partitions.
 */
#define SOC_ROM_NODE DT_NODELABEL(soc_rom)
#define SOC_ROM_NAPOT_SIZE ((uintptr_t)NHPOT(DT_REG_SIZE(SOC_ROM_NODE)))

PMP_SOC_REGION_DEFINE(esp32c5_soc_rom, DT_REG_ADDR(SOC_ROM_NODE),
		      DT_REG_ADDR(SOC_ROM_NODE) + SOC_ROM_NAPOT_SIZE, PMP_R | PMP_X);

/*
 * ESP32-C5 IRAM text region.
 *
 * IRAM and DRAM share one physical window (0x40800000-0x40860000); the
 * code/data split is decided at link time. Only the IRAM text needs a global
 * read+execute PMP entry so kernel code executes from RAM. Kernel data
 * accesses are covered by the kernel-mode dynamic catch-all entry under MPRV,
 * so a blanket read+write SRAM entry is not needed. Covering only the text
 * also keeps user-mode data access falling through to the per-thread
 * memory-domain partitions, so partition permissions (for example read-only
 * or no-access regions) are actually enforced instead of being shadowed by a
 * global read+write+execute grant over all of SRAM.
 */
extern char _iram_text_start[];
extern char _iram_text_end[];

PMP_SOC_REGION_DEFINE(esp32c5_iram_text, _iram_text_start, _iram_text_end, PMP_R | PMP_X);

/*
 * Flash-mapped read-only data (DROM).
 *
 * Const data and string literals map into the same unified flash cache
 * window as the executable text, but past __rom_region_end, so the
 * read-only region entry does not cover them. User-mode loads of const
 * data are permission checked against PMP, so this window must be
 * explicitly allowed as read-only. Use _image_rodata_* so sections
 * after __rodata_region_end that still map into DROM are covered. The
 * bounds are kept exact: the flash cache window also hosts the PSRAM
 * data mapping, so a wider read-only cover would deny kernel stores to
 * external RAM.
 *
 * The CPU subsystem (CLINT-style timer and software interrupt) and the
 * peripheral bus are not listed here: SoC regions are granted to both
 * kernel and user mode, and user threads must not reach those windows.
 * Kernel accesses to them are covered by the kernel-mode catch-all entry.
 */
extern char _image_rodata_start[];
extern char _image_rodata_end[];

PMP_SOC_REGION_DEFINE(esp32c5_flash_rodata, _image_rodata_start, _image_rodata_end, PMP_R);
