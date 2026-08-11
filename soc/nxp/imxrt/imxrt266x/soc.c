/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SoC bring-up for the i.MX RT266x (single Cortex-M85): the access-control, TCM,
 * cache and clock steps every board needs before drivers run. Peripheral gating
 * and clock roots belong to the nxp,imx-ccm-rev3 driver, driven from devicetree.
 */

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/fatal.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>

#include "soc.h"
#include "soc_clock.h"

#include <fsl_common.h>
#include <fsl_powercon.h>

/*
 * The ROM leaves SCB->VTOR at 0 and SystemInit() only relocates it for a RAM
 * vector table, so an XIP image has to point VTOR at this one itself.
 */
extern char _vector_table[];

FUNC_NORETURN void soc_early_init_failed(enum soc_early_init_step step)
{
	/* No console yet, so the step identifier is the whole diagnostic. */
	k_fatal_halt((unsigned int)step);
	CODE_UNREACHABLE;
}

/*
 * Bring up the last-level cache, which serves the cached aliases at 0x68000000
 * (XSPI0) and 0x88000000 (XSPI1); reads through them return zero until it is
 * initialized. Idempotent: re-running on a live LLC would disturb valid tags.
 */
static void soc_llc_init(void)
{
	LLC_Type *const llc = CMPT__LLC;

	if ((llc->CCUCTCR & (LLC_CCUCTCR_LOOKUPEN_MASK | LLC_CCUCTCR_FILLEN_MASK)) != 0U) {
		return;
	}

	/*
	 * Writing the way-valid register is required even though 0xFF is its reset
	 * value: without the write the tag invalidate misses ways.
	 */
	llc->CCUCMWVR = 0xFFU;

	/* Initialize the tag array (ARRAYID 0), then the data array (ARRAYID 1). */
	llc->CCUCMCR = LLC_CCUCMCR_ARRAYID(0U) | LLC_CCUCMCR_MNTOP(0U);
	SOC_POLL_UNTIL((llc->CCUCMAR & LLC_CCUCMAR_MNTOPACTV_MASK) == 0U, SOC_STEP_LLC_TAG_INIT);

	llc->CCUCMCR = LLC_CCUCMCR_ARRAYID(1U) | LLC_CCUCMCR_MNTOP(0U);
	SOC_POLL_UNTIL((llc->CCUCMAR & LLC_CCUCMAR_MNTOPACTV_MASK) == 0U, SOC_STEP_LLC_DATA_INIT);

	llc->CCUCTCR = LLC_CCUCTCR_LOOKUPEN(1U);
	llc->CCUCTCR = LLC_CCUCTCR_LOOKUPEN(1U) | LLC_CCUCTCR_FILLEN(1U);

	llc->CCUUEDR = LLC_CCUUEDR_PROTERRDETEN(1U) | LLC_CCUUEDR_MEMERRDETEN(1U);
	llc->CCUCAOR = LLC_CCUCAOR_WRALLOCPARTIALEN(1U);
}

/*
 * Release the sleep hold, which comes out of reset set. It pins the CMC state
 * machine at its current step, so a WFI never completes: only a debugger halt
 * brings the core back. The SDK clears it from POWER_SetPolicy(); nothing else
 * in this port programs POWERCON, so it happens here.
 */
static void soc_release_sleep_hold(void)
{
	POWERCON_DisableSleepHold(SYSCON__POWERCON_CMC0_CTRL);
}

void soc_reset_hook(void)
{
	/*
	 * TCM strap, LLC and VTOR all come before z_prep_c(), which copies the
	 * ITCM-resident clock bring-up code and its DTCM data into TCM and
	 * initializes .data and .bss. A board placing those behind an LLC-cached
	 * alias needs the LLC up first -- reads there return zero until it is --
	 * so it is brought up here even though the in-tree board uses the direct
	 * XSPI1 window. The SDK does this from BOARD_EarlyInit() inside
	 * SystemInit(); Zephyr has no such hook, so the ordering is explicit.
	 */
	SYSCON__POWERCON_SOC_CTRL->GPR_COLD[0] |= 1U;
	soc_llc_init();
	SCB->VTOR = (uint32_t)_vector_table;

	/* Only sets SCB->CCR LOB/TRD on this SoC; the L1 caches stay off. */
	SystemInit();
}

void soc_early_init_hook(void)
{
	/* Access control first: every step after it is a peripheral access. */
	soc_trdc_setup();

	soc_release_sleep_hold();

	/*
	 * Caches before the clock bring-up, whose switching window saves and
	 * restores whatever cache state it finds. SystemInit() leaves both off.
	 * Without the i-cache the CPU refetches every instruction over XSPI0 --
	 * measured 16x on a register-only loop, 2.6M vs 41.6M iterations/s at 1 GHz.
	 * The d-cache caches the XSPI1 PSRAM the board runs its data from; DMA
	 * buffers there need sys_cache_data_* maintenance.
	 */
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/*
	 * Clocks last: this reconfigures the PLLs, which parks both XSPI
	 * controllers -- including the flash this code runs from -- so everything
	 * it depends on has to be working already.
	 */
	soc_clock_init();
}

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
#include "xspi_nor_boot.h"

/*
 * How far the core image sits past the container header, which is how the ROM
 * locates it. The linker already placed both -- CONFIG_ROM_START_OFFSET is the
 * image and CONFIG_IMAGE_CONTAINER_OFFSET the container -- so deriving the
 * distance here is what keeps it from drifting out of step with either.
 */
#define CONTAINER_TO_IMAGE_OFFSET (CONFIG_ROM_START_OFFSET - CONFIG_IMAGE_CONTAINER_OFFSET)

/*
 * AHAB boot container, KEEPt by boot_header.ld at CONFIG_IMAGE_CONTAINER_OFFSET.
 * Without it the ROM finds a valid flash configuration block but no bootable
 * image, so it never initializes the PSRAM or jumps in. In the image array entry
 * the offset is measured from the container header, the size covers the core
 * image only, and load_addr/entry are both the flash vector table -- the ROM
 * boots from the table's SP and reset PC, it does not branch to the handler.
 */
/* clang-format off */
const __imx_boot_container_section boot_container_t boot_header = {
	.header = {
		.version = CONTAINER_VER,
		.length = CONTAINER_SIZE,
		.tag = CONTAINER_HEADER_TAG,
		.flags = CONTAINER_FLAGS,
		.sw_ver = CONTAINER_SW_VER,
		.fuse_ver = CONTAINER_FUSE_VER,
		.num_images = CONTAINER_NUM_IMG,
		.signature_block_offset = CONTAINER_SIGNATURE_BLOCK_OFFSET,
		.cert_ver = CONTAINER_CERT_VER,
		.reserved1 = 0,
	},
	.image_array = {
		{
			.offset = (uint32_t)CONTAINER_TO_IMAGE_OFFSET,
			.size = (uint32_t)((uintptr_t)_flash_used -
					   CONFIG_ROM_START_OFFSET),
			.load_addr = (uint32_t)(uintptr_t)_vector_start,
			.load_addr_high = 0,
			.entry = (uint32_t)(uintptr_t)_vector_start,
			.entry_high = 0,
			.flags = IMG_FLAGS,
			.metadata = 0,
			.hash = {0},
			.iv = {0},
		},
	},
	.signature_block = {
		.version = SIGNATURE_BLOCK_VER,
		.length = SIGNATURE_BLOCK_SIZE,
		.tag = SIGNATURE_BLOCK_TAG,
		.cert_offset = 0,
		.srk_table_array_offset = 0,
		.signature_offset = 0,
		.blob_offset = 0,
		.key_identifier = 0,
	},
};
/* clang-format on */
#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER */
