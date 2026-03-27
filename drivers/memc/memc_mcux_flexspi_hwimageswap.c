/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/memc/hwimageswap.h>

#include <fsl_flexspi.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

LOG_MODULE_DECLARE(memc_flexspi, CONFIG_MEMC_LOG_LEVEL);

#define ALIGNED(x, y) ((x) % (y) == 0)

/*
 * Get image layout and remapping info from the DT and sanity check
 * them at build time.
 */

/* partition labeled image_1 being remapped to partition image_0 */
#define REMAP_IMAGE_DST    DT_NODE_BY_FIXED_PARTITION_LABEL(image_0)
#define REMAP_IMAGE_SRC    DT_NODE_BY_FIXED_PARTITION_LABEL(image_1)

#if DT_CHOSEN(zephyr_code_partition) != REMAP_IMAGE_DST
#error "Address base used for linking and remap start address must be the same"
#endif


/* Get the FlexSPI node that contains the selected partition */
#define FLEXSPI_DT    DT_NODELABEL(flexspi)

#if !DT_NODE_HAS_COMPAT(FLEXSPI_DT, nxp_imx_flexspi)
#error "Failed to resolve flexspi compatible node as a parent of image0 node"
#endif


/* Get image layout */
#define REMAP_IMAGE_DST_OFFSET    DT_PROP_BY_IDX(REMAP_IMAGE_DST, reg, 0)
#define REMAP_IMAGE_DST_SIZE      DT_PROP_BY_IDX(REMAP_IMAGE_DST, reg, 1)

#define REMAP_IMAGE_SRC_OFFSET    DT_PROP_BY_IDX(REMAP_IMAGE_SRC, reg, 0)
#define REMAP_IMAGE_SRC_SIZE      DT_PROP_BY_IDX(REMAP_IMAGE_SRC, reg, 1)

#if     REMAP_IMAGE_SRC_SIZE   != REMAP_IMAGE_DST_SIZE   || \
	REMAP_IMAGE_SRC_OFFSET <= REMAP_IMAGE_DST_OFFSET
#error "Images must be of the same size and SRC image must be placed after the DST one"
#endif


/* Get remapping alignment */
#define REMAP_ALIGNMENT    DT_PROP(FLEXSPI_DT, nxp_remap_alignment)

#if REMAP_ALIGNMENT <= 0 || !ALIGNED(REMAP_ALIGNMENT, 1024)
#error "Invalid remap alignment value"
#endif

#if defined(CONFIG_BOOTLOADER_MCUBOOT) && (CONFIG_ROM_START_OFFSET != REMAP_ALIGNMENT)
#error "MCUboot header must be padded to the same size as FlexSPI address remapping alignment"
#endif

#if     !ALIGNED(REMAP_IMAGE_DST_OFFSET, REMAP_ALIGNMENT) || \
	!ALIGNED(REMAP_IMAGE_DST_SIZE,   REMAP_ALIGNMENT) || \
	!ALIGNED(REMAP_IMAGE_SRC_OFFSET, REMAP_ALIGNMENT) || \
	!ALIGNED(REMAP_IMAGE_SRC_SIZE,   REMAP_ALIGNMENT)
#error "Invalid image offset/size alignment"
#endif


uint8_t boot_hwimageswap_get_active_slot(void)
{
	FLEXSPI_Type *flexspi = (FLEXSPI_Type *) DT_REG_ADDR(FLEXSPI_DT);

	if (flexspi->HADDRSTART & FLEXSPI_HADDRSTART_REMAPEN_MASK) {
		/* remap is active -> slot1 is active */
		return 1;
	}

	/* remap is NOT active -> slot0 is active */
	return 0;
}

#if defined(CONFIG_BOOT_HW_IMAGE_SWAP)

/* This is called from the bootloader to setup address remapping before
 * accessing the image selected for boot.
 */

int boot_hwimageswap_setup(uint32_t image_offset, uint32_t *offset_fix)
{

/* FLEXSPI address remapping uses so called *overlay* mechanism.
 * Once active a lower part of flash defined by offset and size is mapped
 * on top of a higher part of flash. This allows to execute from image that is
 * stored in the lower part of flash and linked for the upper part of flash.
 * This can be used together with MCUboot's DIRECT_XIP mode and be able
 * to swap images without any flash wear as in case of other modes like SWAP
 * or MOVE.
 *
 * There is one challenge in this overlay mode - it is not possible
 * to access the upper part of flash using a simple bus read (AHB read)
 * via memcpy call or pointer dereference. It is only possible to access it using
 * FLEXSPI IP read command.
 * In situations like computing a hash of downloaded image that was stored in the upper
 * overlayed flash block while remapping is active the IP read must be used. Using a bus
 * read would return data from a wrong image slot. This must be considered when accessing
 * data when remapping is active. Currently FLEXSPI read is done using the IP command so
 * this guarantees data are read as if remapping is not active.
 *
 * To make things more safe the image header and trailer are not included in the remapped area.
 * This guarantees that correct data will be read from these areas even when AHB read is used.
 *
 */

	if (!offset_fix) {
		return -EINVAL;
	}

	FLEXSPI_Type *flexspi = (FLEXSPI_Type *) DT_REG_ADDR(FLEXSPI_DT);

	uint32_t slot0_offset = REMAP_IMAGE_DST_OFFSET;
	uint32_t slot0_size   = REMAP_IMAGE_DST_SIZE;
	uint32_t slot1_offset = REMAP_IMAGE_SRC_OFFSET;
	uint32_t slot1_size   = REMAP_IMAGE_SRC_SIZE;

	uint32_t sector_size = DT_PROP_OR(DT_GPARENT(REMAP_IMAGE_DST),
						erase_block_size, REMAP_ALIGNMENT);

	uint32_t flash_base  = DT_REG_ADDR_BY_IDX(FLEXSPI_DT, 1);
	uint32_t flash_base_ns = flash_base & ~(1 << 28);

	LOG_DBG("FlexSPI HW_IMAGE_SWAP info:");
	LOG_DBG("  boot image offset 0x%x", image_offset);
	LOG_DBG("  sector_size 0x%x remap_alignment 0x%x", sector_size, REMAP_ALIGNMENT);
	LOG_DBG("  slot0 at 0x%x of size 0x%x", slot0_offset, slot0_size);
	LOG_DBG("  slot1 at 0x%x of size 0x%x", slot1_offset, slot1_size);

	/* Remapping is done only for image in slot1 otherwise no action */
	if (image_offset != slot1_offset) {
		return 0;
	}

	/* remapping is set between MCUboot header and image trailer - those areas are left out */
	flexspi_addr_map_config_t cfg = {
		.addrStart  = flash_base_ns + slot0_offset + REMAP_ALIGNMENT,
		.addrEnd    = flash_base_ns + slot0_offset + slot0_size - sector_size,
		.addrOffset = slot1_offset - slot0_offset,
		.remapEnable = true
	};

	LOG_DBG("  HADDRSTART 0x%x HADDREND 0x%x HADDROFFSET 0x%0x",
		flexspi->HADDRSTART, flexspi->HADDREND, flexspi->HADDROFFSET);
	LOG_DBG("  peek slot0 0x%x 0x%x",
		*((uint32_t *)cfg.addrStart), *((uint32_t *)(cfg.addrStart + 4)));

	FLEXSPI_SetAddressMapping(flexspi, &cfg);

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) cfg.addrStart, cfg.addrEnd - cfg.addrStart);
#endif
	__DSB();
	__ISB();

	LOG_DBG("  HADDRSTART 0x%x HADDREND 0x%x HADDROFFSET 0x%0x",
		flexspi->HADDRSTART, flexspi->HADDREND, flexspi->HADDROFFSET);
	LOG_DBG("  peek slot0 0x%x 0x%x",
		*((uint32_t *)cfg.addrStart), *((uint32_t *)(cfg.addrStart + 4)));

	/* No offset adjustment as the MCUboot header is not included in the remapped region */
	*offset_fix = 0;

	return HWIMAGESWAP_MCUX_FLEXSPI_OVERLAY;
}
#endif
