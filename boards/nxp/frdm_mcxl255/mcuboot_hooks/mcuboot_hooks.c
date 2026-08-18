/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MCUboot hook example for FRDM-MCXL255 - MBC0 flash access control management.
 *
 * This module demonstrates the BOOT_GO_HOOKS and MCUBOOT_ACTION_HOOKS interfaces
 * to configure MBC0 (TRDC) flash memory block permissions based on the MCUboot
 * partition layout defined in the device tree.
 * Partition offsets and sizes are read from the DT nodes boot_partition,
 * slot0_partition, and slot1_partition at compile time via DT_REG_ADDR() and
 * DT_REG_SIZE().
 *
 * Permission policy:
 *   ACP slot 0 - RW  : used for writable regions (primary slot before boot, secondary slot)
 *   ACP slot 4 - RX  : used for executable regions (MCUboot, primary slot before jump)
 *
 * Lifecycle:
 *   boot_go_hook()
 *       Sets up ACP slots 0 and 4, then assigns:
 *           MCUboot   -> RX (slot 4)
 *           primary   -> RW (slot 0)  [writable for potential update]
 *           secondary -> RW (slot 0)
 *
 *   mcuboot_status_change(MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND)
 *       Switches the primary slot to RX just before MCUboot jumps to the app,
 *       covering both the update and no-update boot paths.
 *
 */

/*
 * Build-time guard: this hook implementation relies on the assumption that
 * MCUboot never needs to write swap-state metadata (magic, swap type, copy
 * done, image OK) into the primary slot after an image has been installed.
 * That assumption holds only for overwrite-only mode and plain direct-XIP
 * (without revert).
 *
 * In every swap mode (scratch, move, offset) and in direct-XIP-with-revert
 * mode, MCUboot writes the trailer / magic into the primary slot AFTER copying
 * the image, so switching the primary slot to RX at copy-completion time would
 * block that write when trying to set the image as accepted.
 *
 * To extend this ACL handling to also cover these modes would require reserving
 * one extra 8kB block at the end of the image that would be set as RW.
 * Or alternatively the MCUboot API that takes care of accepting an image
 * (modifying the image OK status) would have to adjust the ACL setup to allow for
 * the write.
 *
 */
#if defined(CONFIG_BOOT_SWAP_USING_SCRATCH) || \
    defined(CONFIG_BOOT_SWAP_USING_MOVE)    || \
    defined(CONFIG_BOOT_SWAP_USING_OFFSET)  || \
    defined(CONFIG_BOOT_DIRECT_XIP_REVERT)
#error "mcuboot_hooks.c: incompatible MCUboot mode selected"
#endif

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "bootutil/bootutil.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil/boot_hooks.h"
#include "bootutil/mcuboot_status.h"

/* NXP device register header - includes PERI_TRDC.h for MCXL255. */
#include "fsl_device_registers.h"

LOG_MODULE_DECLARE(mcuboot, CONFIG_MCUBOOT_LOG_LEVEL);

/*
 * MBC0 flash block size for MCXL255.
 * The internal flash is organised in 8 KB erase/protect blocks.
 */
#define HOOKS_FLASH_BLOCK_SIZE  (0x2000U)  /* 8 KB */

/*
 * MBC0 MEM0 block count: 8 BLK_CFG_W words x 8 blocks/word = 64 blocks.
 * At 8 KB per block this covers the full 512 KB internal flash.
 */
#define HOOKS_MEM0_BLOCK_COUNT  (64U)

/*
 * Access control policy (ACP) slot assignments:
 *   slot 0 -> RW  (read + write, no execute)
 *   slot 4 -> RX  (read + execute, no write)
 *
 * GLBAC register bit layout (1 = allow):
 *   bit 0  NUX  non-secure user execute
 *   bit 1  NUW  non-secure user write
 *   bit 2  NUR  non-secure user read
 *   bit 4  NPX  non-secure privileged execute
 *   bit 5  NPW  non-secure privileged write
 *   bit 6  NPR  non-secure privileged read
 *   bit 8  SUX  secure user execute
 *   bit 9  SUW  secure user write
 *   bit 10 SUR  secure user read
 *   bit 12 SPX  secure privileged execute
 *   bit 13 SPW  secure privileged write
 *   bit 14 SPR  secure privileged read
 *
 * RW = NUW|NUR|NPW|NPR|SUW|SUR|SPW|SPR = 0x6666
 * RX = NUX|NUR|NPX|NPR|SUX|SUR|SPX|SPR = 0x5555
 */
#define HOOKS_ACP_RW     (0U)
#define HOOKS_ACP_RX     (4U)

/* GLBAC value that grants read+write (no execute) to all privilege levels. */
#define HOOKS_GLBAC_RW_VAL  (0x6666U)
/* GLBAC value that grants read+execute (no write) to all privilege levels. */
#define HOOKS_GLBAC_RX_VAL  (0x5555U)

/*
 * Block configuration word layout (MBC_DOMx_MEMy_BLK_CFG_W):
 *   Each 32-bit word encodes 8 consecutive blocks at 4 bits each:
 *     bits [3:0]   block 0: NSE | MBACSEL[2:0]
 *     bits [7:4]   block 1: NSE | MBACSEL[2:0]
 *     ...
 *     bits [31:28] block 7: NSE | MBACSEL[2:0]
 *   NSE=1 enables non-secure access via the selected GLBAC slot.
 */
#define HOOKS_BLOCKS_PER_CFG_WORD  (8U)
#define HOOKS_BITS_PER_BLOCK       (4U)
#define HOOKS_MBACSEL_MASK         (0x7U)
#define HOOKS_NSE_BIT              (0x8U)

/*
 * Partition base offsets and sizes sourced from the device tree at
 * compile time.
 */
#define BOOT_PARTITION_OFFSET  \
	((uint32_t)DT_REG_ADDR(DT_NODELABEL(boot_partition)))
#define BOOT_PARTITION_SIZE    \
	((uint32_t)DT_REG_SIZE(DT_NODELABEL(boot_partition)))

#define SLOT0_PARTITION_OFFSET \
	((uint32_t)DT_REG_ADDR(DT_NODELABEL(slot0_partition)))
#define SLOT0_PARTITION_SIZE   \
	((uint32_t)DT_REG_SIZE(DT_NODELABEL(slot0_partition)))

#define SLOT1_PARTITION_OFFSET \
	((uint32_t)DT_REG_ADDR(DT_NODELABEL(slot1_partition)))
#define SLOT1_PARTITION_SIZE   \
	((uint32_t)DT_REG_SIZE(DT_NODELABEL(slot1_partition)))

/* --------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------
 */

static void mbc0_glikey_write_enable(void)
{
	/* Enable MBC register written with GLIKEY index 15 */
	GLIKEY0->CTRL_0 = 0x00060000U;
	GLIKEY0->CTRL_0 = 0x0002000FU;
	GLIKEY0->CTRL_0 = 0x0001000FU;
	GLIKEY0->CTRL_1 = 0x00290000U;
	GLIKEY0->CTRL_0 = 0x0002000FU;
	GLIKEY0->CTRL_1 = 0x00280000U;
	GLIKEY0->CTRL_0 = 0x0000000FU;
}

static void mbc0_glikey_write_disable(void)
{
	GLIKEY0->CTRL_0 = 0x0002000FU;
}

/*
 * mbc0_configure_acp_slot - program one of the eight GLBAC policy registers.
 *
 * @slot : ACP slot index (0-7)
 * @val  : GLBAC register value encoding the desired permissions
 */
static void mbc0_configure_acp_slot(uint8_t slot, uint32_t val)
{
	MBC0->MBC_INDEX[0].MBC_MEMN_GLBAC[slot] = val;
}

/*
 * mbc0_set_mem0_block - set the ACP slot for a single block in MEM0.
 *
 * MEM0 BLK_CFG_W covers blocks 0-63.  Blocks outside this range are silently
 * skipped.
 *
 * @block    : absolute block index (0-based within MBC0)
 * @acp_slot : ACP policy slot to assign (0-7)
 */
static void mbc0_set_mem0_block(uint32_t block, uint8_t acp_slot)
{
	uint32_t word;
	uint32_t bit_off;
	uint32_t field_mask;
	uint32_t reg_val;
	uint32_t field_val;

	if (block >= HOOKS_MEM0_BLOCK_COUNT) {
		return;
	}

	field_val = ((uint32_t)acp_slot & HOOKS_MBACSEL_MASK) | HOOKS_NSE_BIT;
	word = block / HOOKS_BLOCKS_PER_CFG_WORD;
	bit_off = (block % HOOKS_BLOCKS_PER_CFG_WORD) * HOOKS_BITS_PER_BLOCK;
	field_mask = 0xFU << bit_off;

	reg_val = MBC0->MBC_INDEX[0].MBC_DOM0_MEM0_BLK_CFG_W[word];
	reg_val &= ~field_mask;
	reg_val |= (field_val << bit_off) & field_mask;
	MBC0->MBC_INDEX[0].MBC_DOM0_MEM0_BLK_CFG_W[word] = reg_val;
}

/*
 * mbc0_set_partition_blocks - apply an ACP slot to every block that covers the
 * given flash partition.
 *
 * @offset     : partition start offset within flash (bytes)
 * @size       : partition size (bytes)
 * @acp_slot   : ACP policy slot to assign (0-7)
 */
static void mbc0_set_partition_blocks(uint32_t offset, uint32_t size,
				      uint8_t acp_slot)
{
	uint32_t first_blk = offset / HOOKS_FLASH_BLOCK_SIZE;
	uint32_t num_blks  = (size + HOOKS_FLASH_BLOCK_SIZE - 1U) / HOOKS_FLASH_BLOCK_SIZE;

	for (uint32_t i = 0U; i < num_blks; i++) {
		mbc0_set_mem0_block(first_blk + i, acp_slot);
	}
}

/*
 * mbc0_switch_primary_to_rx - change the primary-slot blocks to RX
 * (read + execute, no write) so the image can be executed but not modified.
 */
static void mbc0_switch_primary_to_rx(void)
{
	LOG_DBG("mcuboot_hooks: switching primary slot to RX");
	mbc0_set_partition_blocks(SLOT0_PARTITION_OFFSET, SLOT0_PARTITION_SIZE, HOOKS_ACP_RX);
}

/* --------------------------------------------------------------------------
 * MCUboot hooks
 * --------------------------------------------------------------------------
 */

/*
 * boot_go_hook - called by MCUboot before image selection begins.
 *
 * Configures MBC0 ACP slots and assigns flash-partition block permissions:
 *   - ACP slot 0: RW  (read + write)
 *   - ACP slot 4: RX  (read + execute)
 *   - MCUboot partition  -> RX
 *   - Primary slot       -> RW  (may need to be written during update)
 *   - Secondary slot     -> RW  (always writable for incoming updates)
 *
 * Returns FIH_BOOT_HOOK_REGULAR to let MCUboot continue normally.
 */
fih_ret boot_go_hook(struct boot_rsp *rsp)
{
	ARG_UNUSED(rsp);

	LOG_DBG("%s: configuring MBC0 flash permissions", __func__);

	mbc0_glikey_write_enable();

	LOG_DBG("%s: flash block size = %u bytes", __func__,
		(unsigned int)HOOKS_FLASH_BLOCK_SIZE);

	/* ACP slot 0: RW - full read/write access, no execute. */
	mbc0_configure_acp_slot(HOOKS_ACP_RW, HOOKS_GLBAC_RW_VAL);
	LOG_DBG("%s: ACP slot 0 (RW) = 0x%04x", __func__, HOOKS_GLBAC_RW_VAL);

	/* ACP slot 4: RX - read and execute, no write. */
	mbc0_configure_acp_slot(HOOKS_ACP_RX, HOOKS_GLBAC_RX_VAL);
	LOG_DBG("%s: ACP slot 4 (RX) = 0x%04x", __func__, HOOKS_GLBAC_RX_VAL);

	/* MCUboot partition: RX - MCUboot code must not be overwritten. */
	mbc0_set_partition_blocks(BOOT_PARTITION_OFFSET, BOOT_PARTITION_SIZE,
				  HOOKS_ACP_RX);
	LOG_DBG("%s: MCUboot  0x%05x +%u KB -> RX", __func__,
		(unsigned int)BOOT_PARTITION_OFFSET,
		(unsigned int)(BOOT_PARTITION_SIZE / 1024U));

	/* Primary slot: RW - writable so an incoming image can be copied here. */
	mbc0_set_partition_blocks(SLOT0_PARTITION_OFFSET, SLOT0_PARTITION_SIZE,
				  HOOKS_ACP_RW);
	LOG_DBG("%s: primary  0x%05x +%u KB -> RW", __func__,
		(unsigned int)SLOT0_PARTITION_OFFSET,
		(unsigned int)(SLOT0_PARTITION_SIZE / 1024U));

	/* Secondary slot: always RW - this is where new images are uploaded. */
	mbc0_set_partition_blocks(SLOT1_PARTITION_OFFSET, SLOT1_PARTITION_SIZE,
				  HOOKS_ACP_RW);
	LOG_DBG("%s: secondary 0x%05x +%u KB -> RW", __func__,
		(unsigned int)SLOT1_PARTITION_OFFSET,
		(unsigned int)(SLOT1_PARTITION_SIZE / 1024U));

	LOG_DBG("%s: MBC0 permissions configured", __func__);

	/* Let MCUboot continue with normal image selection. */
	FIH_RET(FIH_BOOT_HOOK_REGULAR);
}

/*
 * mcuboot_status_change - action hook called when MCUboot's internal status
 * changes.
 *
 * On MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND MCUboot has validated the primary
 * image and is about to jump to it.  Switch the primary slot to RX here so
 * the image cannot be modified after the permission change but before the
 * actual jump.  This covers both the normal boot path (no update) and the
 * post-update path.
 */
void mcuboot_status_change(mcuboot_status_type_t status)
{
	if (status == MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND) {
		LOG_DBG("%s: BOOTABLE_IMAGE_FOUND - switching primary slot to RX",
			__func__);
		mbc0_switch_primary_to_rx();
		mbc0_glikey_write_disable();
	}
}
