/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP ROM Bootloader DFU Backend Implementation
 *
 * This file implements the DFU Boot API for NXP MCXW series devices that use
 * the ROM bootloader for OTA (Over-The-Air) firmware updates.
 *
 * Architecture Overview:
 * ----------------------
 * The NXP ROM boot uses a two-slot staging model:
 *   - slot0 (active):  Contains the running application in plain binary format
 *   - slot1 (staging): Holds new firmware packaged in SB3.1 secure container format
 *
 * Update Flow:
 * ------------
 * 1. New firmware (SB3.1 format) is written to slot1
 * 2. OTA configuration is programmed into IFR0 OTACFG page
 * 3. On next boot, ROM bootloader reads OTACFG and processes the SB3.1 container
 * 4. ROM extracts and installs the firmware to the destination encoded in SB3.1
 * 5. ROM updates the status field in OTACFG and boots the new firmware
 *
 * Key Characteristics:
 * --------------------
 * - Updates are permanent once applied (no swap/revert mechanism)
 * - Confirmation is implicit after successful ROM update
 * - Supports both internal and external SPI flash for slot1
 * - SB3.1 containers can target different cores or memory regions
 *
 * Hybrid Mode (CONFIG_NXPBOOT_HYBRID_MCUBOOT):
 * --------------------------------------------
 * When enabled, the system can handle both NXP SB3 and MCUboot image formats.
 * The image type is auto-detected based on the magic number in slot1:
 *   - SB3 images (magic 0x33766273): Use ROM bootloader OTA mechanism
 *   - MCUboot images (magic 0x96f3b83d): Use standard MCUboot swap mechanism
 *
 * This allows the ROM bootloader to upgrade MCUboot itself or update cores
 * that MCUboot cannot access directly.
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/dfu/dfu_boot.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
#include <zephyr/dfu/mcuboot.h>
#include "bootutil/bootutil_public.h"
#include "mcuboot_priv.h"
#endif

LOG_MODULE_REGISTER(dfu_boot_nxp, CONFIG_IMG_MANAGER_LOG_LEVEL);

/* ==========================================================================
 * Constants and Magic Numbers
 * ==========================================================================
 */

/** SB3.1 image magic number ("sbv3" in little-endian) */
#define SB3_IMAGE_MAGIC 0x33766273U

/** SB3.1 block sizes determine the hash algorithm used */
#define SB3_BLOCK_SIZE_SHA256 292
#define SB3_BLOCK_SIZE_SHA384 308

/** Slot indices for the NXP ROM boot staging model */
#define SB3_ACTIVE_SLOT  0
#define SB3_STAGING_SLOT 1

/* ==========================================================================
 * OTA Configuration Constants
 *
 * These magic values are defined by the NXP ROM bootloader specification
 * and are used to communicate update requests and status via the IFR0
 * OTACFG page.
 * ==========================================================================
 */

/** Written to update_available field to trigger an OTA update on next boot */
#define OTA_UPDATE_AVAILABLE_MAGIC 0x746f4278U

/** Written to dump_location field when SB3 file is on external SPI flash */
#define OTA_EXTERNAL_FLASH_MAGIC 0x74784578U

/** Status codes written by ROM bootloader after update attempt */
#define OTA_UPDATE_SUCCESS    0x5ac3c35aU
#define OTA_UPDATE_FAIL_SB3   0x4412d283U
#define OTA_UPDATE_FAIL_ERASE 0x2d61e1acU

/* ==========================================================================
 * Devicetree Configuration
 * ==========================================================================
 */

/*
 * Slot Partition Definitions
 */
#define SLOT0_PARTITION slot0_partition
#define SLOT1_PARTITION slot1_partition

#define SLOT1_PARTITION_NODE DT_NODELABEL(slot1_partition)
#define SLOT1_FLASH_NODE     DT_GPARENT(SLOT1_PARTITION_NODE)

/** Check if slot1 resides on external SPI flash */
#define SLOT1_ON_EXTERNAL_FLASH DT_ON_BUS(SLOT1_FLASH_NODE, spi)

#if SLOT1_ON_EXTERNAL_FLASH
#define SLOT1_SPI_BUS_NODE DT_BUS(SLOT1_FLASH_NODE)
#define SLOT1_SPI_MAX_FREQ DT_PROP(SLOT1_FLASH_NODE, spi_max_frequency)
#endif

/*
 * IFR0 OTACFG Page Configuration
 *
 * The IFR0 (Information Flash Region 0) contains a dedicated sector for
 * OTA configuration. The ROM bootloader reads this sector on every boot
 * to check for pending updates.
 */
#define IFR0_FLASH_NODE      DT_CHOSEN(zephyr_flash_controller)
#define IFR0_BASE            DT_REG_ADDR(DT_NODELABEL(ifr0))
#define IFR_SECTOR_SIZE      DT_PROP(DT_NODELABEL(ifr0), erase_block_size)
#define IFR_SECTOR_OFFSET(n) ((n) * IFR_SECTOR_SIZE)
#define IFR_SECT_OTA_CFG     DT_PROP(DT_NODELABEL(ifr0), ota_cfg_sector)
#define IFR_OTA_CFG_ADDR     (IFR0_BASE + IFR_SECTOR_OFFSET(IFR_SECT_OTA_CFG))

/* ==========================================================================
 * Hybrid Mode Configuration (MCUboot + NXP Boot ROM)
 * ==========================================================================
 */

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT

#ifdef CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512
#define IMAGE_TLV_SHA IMAGE_TLV_SHA512
#define IMAGE_SHA_LEN 64
#else
#define IMAGE_TLV_SHA IMAGE_TLV_SHA256
#define IMAGE_SHA_LEN 32
#endif

#define ACTIVE_SLOT_FLASH_AREA_ID DT_PARTITION_ID(DT_CHOSEN(zephyr_code_partition))

BUILD_ASSERT(sizeof(struct image_header) == IMAGE_HEADER_SIZE,
	     "struct image_header not required size");

/** Image index values for multi-image MCUboot configurations */
enum IMAGE_INDEXES {
	IMAGE_INDEX_INVALID = -1,
	IMAGE_INDEX_0,
	IMAGE_INDEX_1,
	IMAGE_INDEX_2,
	IMAGE_INDEX_3,
	IMAGE_INDEX_4,
	IMAGE_INDEX_5,
	IMAGE_INDEX_6,
	IMAGE_INDEX_7,
};

/** Image format types for hybrid mode detection */
enum IMAGE_TYPE {
	IMAGE_TYPE_INVALID = -1,
	IMAGE_TYPE_SB3,
	IMAGE_TYPE_MCUBOOT,
};

#endif /* CONFIG_NXPBOOT_HYBRID_MCUBOOT */

/* ==========================================================================
 * Data Structures
 * ==========================================================================
 */

/**
 * @brief OTA Configuration structure
 *
 * This structure maps directly to the IFR0 OTACFG page layout as defined
 * by the NXP ROM bootloader specification. All fields must maintain their
 * exact offsets for proper ROM bootloader operation.
 *
 * @note This structure is written to flash and read by the ROM bootloader,
 *       so it must remain packed and match the hardware specification exactly.
 */
__packed struct nxp_ota_config {
	uint32_t update_available; /**< 0x00: OTA_UPDATE_AVAILABLE_MAGIC if update pending */
	uint32_t reserved1;        /**< 0x04: Reserved for future use */
	uint32_t reserved2;        /**< 0x08: Reserved for future use */
	uint32_t reserved3;        /**< 0x0C: Reserved for future use */
	uint32_t dump_location;    /**< 0x10: OTA_EXTERNAL_FLASH_MAGIC or 0 for internal */
	uint32_t baud_rate;        /**< 0x14: LPSPI baud rate for external flash access */
	uint32_t dump_address;     /**< 0x18: Physical start address of SB3 file */
	uint32_t file_size;        /**< 0x1C: Size of SB3 file in bytes */
	uint32_t update_status;    /**< 0x20: Status code written by ROM after update */
	uint32_t reserved4;        /**< 0x24: Reserved for future use */
	uint32_t reserved5;        /**< 0x28: Reserved for future use */
	uint32_t reserved6;        /**< 0x2C: Reserved for future use */
	uint8_t unlock_key[16];    /**< 0x30: Feature unlock key (ROM requirement) */
};

BUILD_ASSERT(sizeof(struct nxp_ota_config) == 64,
	     "nxp_ota_config size mismatch with IFR0 OTACFG layout");

/**
 * @brief SB3.1 Image Header structure
 *
 * This structure represents the header of an NXP Secure Binary v3.1 container.
 * The header contains metadata about the container including version info,
 * block configuration, and firmware version.
 *
 * @note The block_size field determines the hash algorithm:
 *       - 292 bytes: SHA-256
 *       - 308 bytes: SHA-384
 */
__packed struct sb3_image_header {
	uint32_t magic;             /**< Must be SB3_IMAGE_MAGIC (0x33766273) */
	uint16_t sb3_version_minor; /**< SB3 format minor version (expected: 1) */
	uint16_t sb3_version_major; /**< SB3 format major version (expected: 3) */
	uint32_t flags;             /**< Container flags */
	uint32_t block_count;       /**< Number of cipher blocks */
	uint32_t block_size;        /**< Size of each block (determines hash algorithm) */
	uint32_t timestamp_low;     /**< Build timestamp (low 32 bits) */
	uint32_t timestamp_high;    /**< Build timestamp (high 32 bits) */
	uint32_t fw_version;        /**< Firmware version (encoding: major.minor.revision) */
	uint32_t total_length;      /**< Total length of the container */
	uint32_t image_type;        /**< Type identifier for the contained image */
	uint32_t cert_offset;       /**< Offset to certificate block */
	uint8_t description[16];    /**< Human-readable description string */
};

BUILD_ASSERT(sizeof(struct sb3_image_header) == 60,
	     "sb3_image_header size mismatch with SB3.1 specification");

/* ==========================================================================
 * Static Data
 * ==========================================================================
 */

/**
 * @brief OTA Feature Unlock Key
 *
 * This key is required by the NXP ROM bootloader to authorize OTA updates.
 * The value is fixed by the ROM specification and cannot be customized.
 */
static const uint8_t ota_unlock_key[16] = {0x61, 0x63, 0x74, 0x69, 0x76, 0x61, 0x74, 0x65,
					   0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x78, 0x4d};

/* ==========================================================================
 * IFR0 OTACFG Page Access Functions
 *
 * These functions provide low-level access to the OTA configuration stored
 * in the IFR0 flash region. The OTACFG page is the communication channel
 * between the application and the ROM bootloader.
 * ==========================================================================
 */

/**
 * @brief Get the IFR0 flash device
 *
 * @return Pointer to flash device, or NULL if not ready
 */
static const struct device *get_ifr0_flash_device(void)
{
	const struct device *flash_dev = DEVICE_DT_GET(IFR0_FLASH_NODE);

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("IFR0 flash device not ready");
		return NULL;
	}

	return flash_dev;
}

/**
 * @brief Erase the OTACFG page
 *
 * Erases the entire OTACFG sector in IFR0. This clears any pending update
 * request and resets the update status.
 *
 * @return 0 on success, negative error code on failure
 */
static int erase_otacfg_page(void)
{
	const struct device *flash_dev;
	int rc;

	flash_dev = get_ifr0_flash_device();
	if (flash_dev == NULL) {
		return -ENODEV;
	}

	rc = flash_erase(flash_dev, IFR_OTA_CFG_ADDR, IFR_SECTOR_SIZE);
	if (rc != 0) {
		LOG_ERR("Failed to erase OTACFG page: %d", rc);
		return -EIO;
	}

	LOG_DBG("OTACFG page erased");
	return 0;
}

/**
 * @brief Write OTA configuration to IFR0 OTACFG page
 *
 * Programs the OTA configuration structure to the OTACFG page. The sector
 * is erased before writing to ensure clean programming.
 *
 * @param cfg Pointer to OTA configuration structure to write
 * @return 0 on success, negative error code on failure
 */
static int write_otacfg_page(const struct nxp_ota_config *cfg)
{
	const struct device *flash_dev;
	int rc;

	flash_dev = get_ifr0_flash_device();
	if (flash_dev == NULL) {
		return -ENODEV;
	}

	/* Erase sector before programming (required for flash write) */
	rc = flash_erase(flash_dev, IFR_OTA_CFG_ADDR, IFR_SECTOR_SIZE);
	if (rc != 0) {
		LOG_ERR("Failed to erase OTACFG page: %d", rc);
		return -EIO;
	}

	rc = flash_write(flash_dev, IFR_OTA_CFG_ADDR, cfg, sizeof(*cfg));
	if (rc != 0) {
		LOG_ERR("Failed to write OTACFG page: %d", rc);
		return -EIO;
	}

	LOG_DBG("OTACFG page programmed successfully");
	return 0;
}

/**
 * @brief Read OTA configuration from IFR0 OTACFG page
 *
 * @param cfg Pointer to OTA configuration structure to fill
 * @return 0 on success, negative error code on failure
 */
static int read_otacfg_page(struct nxp_ota_config *cfg)
{
	const struct device *flash_dev;
	int rc;

	flash_dev = get_ifr0_flash_device();
	if (flash_dev == NULL) {
		return -ENODEV;
	}

	rc = flash_read(flash_dev, IFR_OTA_CFG_ADDR, cfg, sizeof(*cfg));
	if (rc != 0) {
		LOG_ERR("Failed to read OTACFG page: %d", rc);
		return -EIO;
	}

	return 0;
}

/**
 * @brief Check if an OTA update is pending
 *
 * Reads the OTACFG page and checks if the update_available field contains
 * the magic value indicating a pending update request.
 *
 * @return true if update is pending, false otherwise
 */
static bool is_ota_update_pending(void)
{
	struct nxp_ota_config cfg;

	if (read_otacfg_page(&cfg) != 0) {
		return false;
	}

	return (cfg.update_available == OTA_UPDATE_AVAILABLE_MAGIC);
}

/**
 * @brief Get the last OTA update status
 *
 * Reads the update_status field from the OTACFG page. This field is written
 * by the ROM bootloader after an update attempt to indicate success or the
 * type of failure.
 *
 * @return OTA update status value, or 0 if read fails
 */
static uint32_t get_ota_update_status(void)
{
	struct nxp_ota_config cfg;

	if (read_otacfg_page(&cfg) != 0) {
		return 0;
	}

	return cfg.update_status;
}

/* ==========================================================================
 * SB3 Image Parsing Functions
 * ==========================================================================
 */

/**
 * @brief Parse and validate SB3.1 header
 *
 * Validates the SB3.1 header magic, version, and block size. If the header
 * is valid and info is not NULL, extracts image metadata into the info
 * structure.
 *
 * @param hdr  Pointer to SB3 image header to parse
 * @param info Pointer to image info structure to fill (may be NULL)
 * @return 0 on success, -ENOENT if header is invalid
 */
static int parse_sb3_header(const struct sb3_image_header *hdr, struct dfu_boot_img_info *info)
{
	/* Validate magic number */
	if (hdr->magic != SB3_IMAGE_MAGIC) {
		LOG_ERR("Invalid SB3 magic: 0x%08x (expected 0x%08x)", hdr->magic, SB3_IMAGE_MAGIC);
		return -ENOENT;
	}

	/* Validate SB3 version (must be 3.1) */
	if ((hdr->sb3_version_major != 3) || (hdr->sb3_version_minor != 1)) {
		LOG_ERR("Unsupported SB3 version: %u.%u (expected 3.1)", hdr->sb3_version_major,
			hdr->sb3_version_minor);
		return -ENOENT;
	}

	/* Validate block size (determines hash algorithm) */
	if ((hdr->block_size != SB3_BLOCK_SIZE_SHA256) &&
	    (hdr->block_size != SB3_BLOCK_SIZE_SHA384)) {
		LOG_ERR("Invalid SB3 block size: %u (expected %u or %u)", hdr->block_size,
			SB3_BLOCK_SIZE_SHA256, SB3_BLOCK_SIZE_SHA384);
		return -ENOENT;
	}

	/* Extract image information if requested */
	if (info != NULL) {
		/*
		 * SB3 firmware version encoding (32-bit value):
		 *   Bits 31-24: Major version
		 *   Bits 23-16: Minor version
		 *   Bits 15-0:  Revision
		 */
		info->version.major = (hdr->fw_version >> 24) & 0xFF;
		info->version.minor = (hdr->fw_version >> 16) & 0xFF;
		info->version.revision = hdr->fw_version & 0xFFFF;
		info->version.build_num = 0;

		/* Total image size = header + data + cipher blocks */
		info->img_size = hdr->total_length + (hdr->block_count * hdr->block_size);
		info->hdr_size = sizeof(struct sb3_image_header);
		info->load_addr = 0;
		info->flags = 0;

		/* Determine hash length based on block size */
		if (hdr->block_size == SB3_BLOCK_SIZE_SHA256) {
			info->hash_len = 32;
		} else {
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
			info->hash_len = MIN(48, DFU_BOOT_IMG_SHA_LEN);
#else
			info->hash_len = 48;
#endif
		}

		info->valid = true;
	}

	return 0;
}

/**
 * @brief Read SB3 image information from a slot
 *
 * Reads and parses the SB3 header from the specified slot. For the active
 * slot (slot0), returns synthetic information since the active image is
 * in plain binary format, not SB3.
 *
 * @param slot Slot number to read from
 * @param info Pointer to image info structure to fill
 * @return 0 on success, negative error code on failure
 */
static int read_sb3_img_info(int slot, struct dfu_boot_img_info *info)
{
	struct sb3_image_header hdr;
	size_t start_offset;
	int rc;

	start_offset = dfu_boot_get_image_start_offset(slot);

	rc = dfu_boot_read(slot, start_offset, &hdr, sizeof(hdr));
	if (rc != 0) {
		return rc;
	}

	if (slot == SB3_ACTIVE_SLOT) {
		/*
		 * The active slot contains a plain binary image (not SB3 format)
		 * since the ROM bootloader extracts the payload during installation.
		 * We provide synthetic information for API compatibility.
		 */
		info->img_size = PARTITION_SIZE(SLOT0_PARTITION);
		info->hdr_size = 0;
		info->load_addr = 0;
		info->hash_len = DFU_BOOT_IMG_SHA_LEN;
		info->flags = DFU_BOOT_STATE_F_CONFIRMED | DFU_BOOT_STATE_F_ACTIVE;
		info->valid = true;
	} else {
		/* Parse the SB3 header from staging slot */
		rc = parse_sb3_header(&hdr, info);
		if (rc != 0) {
			LOG_ERR("Failed to parse SB3 header from slot %d", slot);
			return rc;
		}

		/* Read the image hash (located immediately after header) */
		rc = dfu_boot_read(slot, start_offset + info->hdr_size, info->hash, info->hash_len);
		if (rc != 0) {
			LOG_ERR("Failed to read image hash from slot %d", slot);
			return rc;
		}

		/* Set flags based on OTACFG state */
		if (is_ota_update_pending()) {
			info->flags = DFU_BOOT_STATE_F_PENDING;
		} else {
			info->flags = 0;
		}
	}

	return 0;
}

/* ==========================================================================
 * MCUboot Hybrid Mode Functions
 *
 * These functions are only compiled when CONFIG_NXPBOOT_HYBRID_MCUBOOT is
 * enabled. They provide support for handling both MCUboot and SB3 image
 * formats in the same system.
 * ==========================================================================
 */

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT

/**
 * @brief Get the image start offset for swap-using-offset mode
 *
 * In MCUboot's swap-using-offset mode, the secondary slot image may be
 * located at an offset within the partition. This function determines
 * the correct offset based on the current swap state.
 *
 * @param area_id Flash area ID to check
 * @return Offset in bytes from partition start, or 0 if no offset needed
 */
static size_t get_image_start_offset(uint8_t area_id)
{
	size_t off = 0;

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_OFFSET)
	int image = IMAGE_INDEX_INVALID;

	/* Map flash area ID to image index */
	if (area_id == PARTITION_ID(slot1_partition)) {
		image = IMAGE_INDEX_0;
#if PARTITION_EXISTS(slot3_partition)
	} else if (area_id == PARTITION_ID(slot3_partition)) {
		image = IMAGE_INDEX_1;
#endif
#if PARTITION_EXISTS(slot5_partition)
	} else if (area_id == PARTITION_ID(slot5_partition)) {
		image = IMAGE_INDEX_2;
#endif
#if PARTITION_EXISTS(slot7_partition)
	} else if (area_id == PARTITION_ID(slot7_partition)) {
		image = IMAGE_INDEX_3;
#endif
#if PARTITION_EXISTS(slot9_partition)
	} else if (area_id == PARTITION_ID(slot9_partition)) {
		image = IMAGE_INDEX_4;
#endif
#if PARTITION_EXISTS(slot11_partition)
	} else if (area_id == PARTITION_ID(slot11_partition)) {
		image = IMAGE_INDEX_5;
#endif
#if PARTITION_EXISTS(slot13_partition)
	} else if (area_id == PARTITION_ID(slot13_partition)) {
		image = IMAGE_INDEX_6;
#endif
#if PARTITION_EXISTS(slot15_partition)
	} else if (area_id == PARTITION_ID(slot15_partition)) {
		image = IMAGE_INDEX_7;
#endif
	}

	if (image != IMAGE_INDEX_INVALID) {
		const struct flash_area *fa;
		uint32_t num_sectors = SWAP_USING_OFFSET_SECTOR_UPDATE_BEGIN;
		struct flash_sector sector_data;
		int rc;

		rc = flash_area_open(area_id, &fa);
		if (rc) {
			LOG_ERR("Failed to open flash area %u: %d", area_id, rc);
			goto done;
		}

		rc = flash_area_get_sectors(area_id, &num_sectors, &sector_data);
		if ((rc != 0 && rc != -ENOMEM) ||
		    (num_sectors != SWAP_USING_OFFSET_SECTOR_UPDATE_BEGIN)) {
			LOG_ERR("Failed to get sector details: %d", rc);
		} else {
			uint32_t magic;

			rc = flash_area_read(fa, sector_data.fs_size, &magic, sizeof(magic));
			if (rc != 0) {
				LOG_ERR("Failed to read magic from area %u: %d", area_id, rc);
			} else {
				if ((magic == SB3_IMAGE_MAGIC) ||
				    (boot_swap_type_multi(image) != BOOT_SWAP_TYPE_REVERT)) {
					off = sector_data.fs_size;
				}
			}
		}

		flash_area_close(fa);
	}

done:
	LOG_DBG("Image start offset for area %u: 0x%zx", area_id, off);
#else
	ARG_UNUSED(area_id);
#endif /* CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_OFFSET */

	return off;
}

/**
 * @brief Detect the image type in a slot
 *
 * Reads the magic number from the beginning of the image to determine
 * whether it's an MCUboot image or an SB3 container.
 *
 * @param slot Slot number to check
 * @return IMAGE_TYPE_MCUBOOT, IMAGE_TYPE_SB3, or IMAGE_TYPE_INVALID
 */
static enum IMAGE_TYPE get_image_type(int slot)
{
	uint32_t magic;
	int rc;

	rc = dfu_boot_read(slot, get_image_start_offset(dfu_boot_get_flash_area_id(slot)), &magic,
			   sizeof(magic));
	if (rc != 0) {
		LOG_ERR("Failed to read magic from slot %d: %d", slot, rc);
		return IMAGE_TYPE_INVALID;
	}

	if (magic == IMAGE_MAGIC) {
		return IMAGE_TYPE_MCUBOOT;
	} else if (magic == SB3_IMAGE_MAGIC) {
		return IMAGE_TYPE_SB3;
	}

	return IMAGE_TYPE_INVALID;
}

/**
 * @brief Calculate trailer status offset for MCUboot images
 *
 * @param area_size Total size of the flash area
 * @return Offset to the trailer status field
 */
static ssize_t get_trailer_status_offset(size_t area_size)
{
	return (ssize_t)area_size - BOOT_MAGIC_SZ - BOOT_MAX_ALIGN * 2;
}

/**
 * @brief Find TLV section in MCUboot image
 *
 * Searches for a TLV (Type-Length-Value) section with the specified magic
 * number in the image.
 *
 * @param slot      Slot number to search
 * @param start_off Input: search start offset; Output: TLV data start offset
 * @param end_off   Output: TLV section end offset
 * @param magic     TLV magic number to search for
 * @return 0 if found, -ENOENT if not found, other negative on error
 */
static int find_mcuboot_tlvs(int slot, size_t *start_off, size_t *end_off, uint16_t magic)
{
	struct image_tlv_info tlv_info;
	int rc;

	rc = dfu_boot_read(slot, *start_off, &tlv_info, sizeof(tlv_info));
	if (rc != 0) {
		return rc;
	}

	if (tlv_info.it_magic != magic) {
		return -ENOENT;
	}

	*start_off += sizeof(tlv_info);
	*end_off = *start_off + tlv_info.it_tlv_tot;

	return 0;
}

/**
 * @brief Parse MCUboot image header into DFU boot info structure
 *
 * @param hdr  Pointer to MCUboot image header
 * @param info Pointer to DFU boot info structure to fill
 */
static void parse_mcuboot_header(const struct image_header *hdr, struct dfu_boot_img_info *info)
{
	BUILD_ASSERT(sizeof(struct image_version) == sizeof(struct dfu_boot_img_version),
		     "image_version must match dfu_boot_img_version size");

	memcpy(&info->version, &hdr->ih_ver, sizeof(struct dfu_boot_img_version));

	info->img_size = hdr->ih_img_size;
	info->hdr_size = hdr->ih_hdr_size;
	info->load_addr = hdr->ih_load_addr;

	info->flags = 0;
	if (hdr->ih_flags & IMAGE_F_NON_BOOTABLE) {
		info->flags |= DFU_BOOT_IMG_F_NON_BOOTABLE;
	}
	if (hdr->ih_flags & IMAGE_F_ROM_FIXED) {
		info->flags |= DFU_BOOT_IMG_F_ROM_FIXED;
	}

	info->valid = true;
}

/**
 * @brief Read MCUboot image information from a slot
 *
 * Reads and parses the MCUboot image header and TLVs from the specified slot.
 * Extracts version information, image size, and hash.
 *
 * @param slot Slot number to read from
 * @param info Pointer to image info structure to fill
 * @return 0 on success, negative error code on failure
 */
static int read_mcuboot_img_info(int slot, struct dfu_boot_img_info *info)
{
	struct image_header hdr;
	struct image_tlv tlv;
	size_t data_off;
	size_t data_end;
	bool hash_found;
	int rc;

	if (info == NULL) {
		return -EINVAL;
	}

	rc = dfu_boot_read(slot, get_image_start_offset(dfu_boot_get_flash_area_id(slot)), &hdr,
			   sizeof(hdr));
	if (rc != 0) {
		return rc;
	}

	if (hdr.ih_magic != IMAGE_MAGIC) {
		return -ENOENT;
	}

	parse_mcuboot_header(&hdr, info);

	/*
	 * Read the image's TLVs to find the hash.
	 *
	 * TLV structure:
	 *   [Image Header][Image Data][Protected TLVs (optional)][Unprotected TLVs]
	 *
	 * We first try to find protected TLVs, then look for the hash in
	 * unprotected TLVs. All valid images must have a hash TLV.
	 */
	data_off = hdr.ih_hdr_size + hdr.ih_img_size +
		   get_image_start_offset(dfu_boot_get_flash_area_id(slot));

	/* Try to find protected TLVs first */
	rc = find_mcuboot_tlvs(slot, &data_off, &data_end, IMAGE_TLV_PROT_INFO_MAGIC);
	if (rc == 0) {
		/* Skip past protected TLVs to find unprotected TLVs */
		data_off = data_end - sizeof(struct image_tlv_info);
	}

	/* Find unprotected TLVs (required) */
	rc = find_mcuboot_tlvs(slot, &data_off, &data_end, IMAGE_TLV_INFO_MAGIC);
	if (rc != 0) {
		return rc;
	}

	/* Search for hash TLV */
	hash_found = false;
	while (data_off + sizeof(tlv) <= data_end) {
		rc = dfu_boot_read(slot, data_off, &tlv, sizeof(tlv));
		if (rc != 0) {
			return rc;
		}

		/* Check for erased flash (invalid TLV) */
		if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
			return -EINVAL;
		}

		/* Skip non-hash TLVs */
		if (tlv.it_type != IMAGE_TLV_SHA || tlv.it_len != IMAGE_SHA_LEN) {
			data_off += sizeof(tlv) + tlv.it_len;
			continue;
		}

		/* Found hash TLV - ensure there's only one */
		if (hash_found) {
			LOG_ERR("Multiple hash TLVs found in image");
			return -EINVAL;
		}

		data_off += sizeof(tlv);
		if (data_off + IMAGE_SHA_LEN > data_end) {
			return -EINVAL;
		}

		rc = dfu_boot_read(slot, data_off, info->hash, IMAGE_SHA_LEN);
		if (rc != 0) {
			return rc;
		}

		info->hash_len = IMAGE_SHA_LEN;
		hash_found = true;
		data_off += IMAGE_SHA_LEN;
	}

	if (!hash_found) {
		LOG_ERR("No hash TLV found in image");
		return -ENOENT;
	}

	return 0;
}

/**
 * @brief Mark the active MCUboot image as confirmed
 *
 * Writes the confirmation flag to the active slot's trailer area,
 * preventing automatic revert on next boot.
 *
 * @return 0 on success, negative error code on failure
 */
static int write_mcuboot_img_confirmed(void)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(ACTIVE_SLOT_FLASH_AREA_ID, &fa);
	if (rc != 0) {
		LOG_ERR("Failed to open active slot flash area: %d", rc);
		return -EIO;
	}

	rc = boot_set_next(fa, true, true);

	flash_area_close(fa);

	return rc;
}

#else /* !CONFIG_NXPBOOT_HYBRID_MCUBOOT */

/*
 * Stub for non-hybrid mode - images always start at partition beginning
 */
#define get_image_start_offset(...) 0

#endif /* CONFIG_NXPBOOT_HYBRID_MCUBOOT */

/* ==========================================================================
 * DFU Boot API Implementation
 *
 * These functions implement the public DFU Boot API defined in
 * <zephyr/dfu/dfu_boot.h>. They provide a bootloader-agnostic interface
 * for firmware update operations.
 * ==========================================================================
 */

/**
 * @brief Get the image start offset within a slot
 *
 * For hybrid mode with swap-using-offset, the image may not start at the
 * beginning of the partition. This function returns the actual offset.
 *
 * @param slot Slot number
 * @return Offset in bytes from partition start
 */
size_t dfu_boot_get_image_start_offset(int slot)
{
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	return get_image_start_offset(dfu_boot_get_flash_area_id(slot));
#else
	ARG_UNUSED(slot);
	return 0;
#endif
}

/**
 * @brief Get the trailer status offset within a flash area
 *
 * Returns the offset to the MCUboot trailer status field. For pure NXP
 * ROM boot mode, returns 0 as no trailer is used.
 *
 * @param area_size Total size of the flash area
 * @return Offset to trailer status, or 0 if not applicable
 */
size_t dfu_boot_get_trailer_status_offset(size_t area_size)
{
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	return get_trailer_status_offset(area_size);
#else
	ARG_UNUSED(area_size);
	return 0;
#endif
}

/**
 * @brief Read image information from a slot
 *
 * Reads and parses the image header from the specified slot. In hybrid mode,
 * automatically detects the image format (MCUboot or SB3) and uses the
 * appropriate parser.
 *
 * @param slot Slot number to read from
 * @param info Pointer to image info structure to fill
 * @return 0 on success, -EINVAL if info is NULL, -ENOENT if no valid image
 */
int dfu_boot_read_img_info(int slot, struct dfu_boot_img_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));
	info->valid = false;

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	enum IMAGE_TYPE type = get_image_type(slot);

	switch (type) {
	case IMAGE_TYPE_MCUBOOT:
		return read_mcuboot_img_info(slot, info);

	case IMAGE_TYPE_SB3:
		return read_sb3_img_info(slot, info);

	default:
		return -ENOENT;
	}
#else
	return read_sb3_img_info(slot, info);
#endif
}

/**
 * @brief Validate an image header from a buffer
 *
 * Validates the image header contained in the provided buffer without
 * reading from flash. Useful for validating incoming firmware data
 * before writing to flash.
 *
 * @param data Pointer to buffer containing image header
 * @param len  Length of buffer in bytes
 * @param info Pointer to image info structure to fill (may be NULL)
 * @return 0 if header is valid, -EINVAL if buffer too small, -ENOENT if invalid
 */
int dfu_boot_validate_header(const void *data, size_t len, struct dfu_boot_img_info *info)
{
	if (data == NULL) {
		return -EINVAL;
	}

	/* Check minimum size for SB3 header */
	if (len < sizeof(struct sb3_image_header)) {
		return -EINVAL;
	}

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	/* Also need space for MCUboot header in hybrid mode */
	if (len < sizeof(struct image_header)) {
		return -EINVAL;
	}
#endif

	if (info != NULL) {
		memset(info, 0, sizeof(*info));
	}

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	uint32_t magic;
	int rc;

	memcpy(&magic, data, sizeof(magic));

	if (magic == IMAGE_MAGIC) {
		/* MCUboot image */
		if (info != NULL) {
			parse_mcuboot_header((const struct image_header *)data, info);
		}
		rc = 0;
	} else if (magic == SB3_IMAGE_MAGIC) {
		/* SB3 image */
		rc = parse_sb3_header((const struct sb3_image_header *)data, info);
	} else {
		LOG_DBG("Unknown image magic: 0x%08x", magic);
		rc = -ENOENT;
	}

	return rc;
#else
	return parse_sb3_header((const struct sb3_image_header *)data, info);
#endif
}

/**
 * @brief Get the pending swap type for an image
 *
 * Returns the type of swap operation that will occur on next boot for
 * the specified image index.
 *
 * For NXP ROM boot:
 *   - Returns TEST if an OTA update is pending in OTACFG
 *   - Returns NONE otherwise
 *
 * For hybrid mode with MCUboot image:
 *   - Delegates to MCUboot's boot_swap_type_multi()
 *
 * @param image_index Image index (0 for primary image)
 * @return DFU_BOOT_SWAP_TYPE_* value
 */
int dfu_boot_get_swap_type(int image_index)
{
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	int slot = image_index * 2 + 1;

	if (get_image_type(slot) == IMAGE_TYPE_MCUBOOT) {
		switch (boot_swap_type_multi(image_index)) {
		case BOOT_SWAP_TYPE_NONE:
			return DFU_BOOT_SWAP_TYPE_NONE;
		case BOOT_SWAP_TYPE_TEST:
			return DFU_BOOT_SWAP_TYPE_TEST;
		case BOOT_SWAP_TYPE_PERM:
			return DFU_BOOT_SWAP_TYPE_PERM;
		case BOOT_SWAP_TYPE_REVERT:
			return DFU_BOOT_SWAP_TYPE_REVERT;
		default:
			return DFU_BOOT_SWAP_TYPE_UNKNOWN;
		}
	}
#else
	ARG_UNUSED(image_index);
#endif

	/*
	 * NXP ROM boot: Check OTACFG for pending update.
	 * The ROM bootloader will process the SB3 file on next boot
	 * if update_available contains the magic value.
	 */
	if (is_ota_update_pending()) {
		return DFU_BOOT_SWAP_TYPE_TEST;
	}

	return DFU_BOOT_SWAP_TYPE_NONE;
}

/**
 * @brief Set the next boot slot
 *
 * This function handles both confirming the current image and setting
 * a pending upgrade on another slot.
 *
 * For NXP ROM boot:
 *   - When active=true && confirm=true: No-op (confirmation is implicit)
 *   - When active=false: Programs OTACFG to trigger update on next boot
 *
 * For hybrid mode with MCUboot image:
 *   - Delegates to MCUboot's boot_set_confirmed_multi() or boot_set_pending_multi()
 *
 * @param slot Slot number to boot next
 * @param active Whether this slot is currently active
 * @param confirm If true, make the change permanent/confirmed
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_set_next(int slot, bool active, bool confirm)
{
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	if (get_image_type(slot) == IMAGE_TYPE_MCUBOOT) {
		if (active && confirm) {
			/* Confirming current MCUboot image */
			return boot_set_confirmed_multi(slot >> 1);
		}
		/* Setting pending on secondary slot */
		return boot_set_pending_multi(slot >> 1, confirm);
	}
#endif

	if (active && confirm) {
		/*
		 * NXP ROM boot: Confirmation is implicit.
		 *
		 * Once the ROM bootloader successfully processes the SB3 file
		 * and installs the new firmware, the update is permanent.
		 * There is no revert mechanism, so confirmation is a no-op.
		 */
		LOG_DBG("Image confirmation (no-op for NXP ROM boot)");
		return 0;
	}

	if (active) {
		/*
		 * Setting pending on active slot without confirm doesn't
		 * make sense for NXP ROM boot - the active slot is already
		 * running and cannot be "pending".
		 */
		LOG_DBG("set_next on active slot without confirm (no-op)");
		return 0;
	}

	/* Setting pending on inactive (staging) slot */
	return dfu_boot_set_pending(slot, confirm);
}

/**
 * @brief Mark an image as pending for installation
 *
 * For NXP ROM boot, this programs the OTACFG page with the location and
 * size of the SB3 file in the staging slot. The ROM bootloader will
 * process this on the next boot.
 *
 * For hybrid mode with MCUboot image, delegates to MCUboot's
 * boot_set_pending_multi().
 *
 * @param slot      Slot number containing the new image (must be staging slot)
 * @param permanent If true, mark as permanent (ignored for NXP ROM boot)
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_set_pending(int slot, bool permanent)
{
	const struct flash_area *fa;
	struct nxp_ota_config cfg;
	struct dfu_boot_img_info info;
	int area_id;
	int rc;

#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	if (get_image_type(slot) == IMAGE_TYPE_MCUBOOT) {
		return boot_set_pending_multi(slot >> 1, permanent);
	}
#else
	ARG_UNUSED(permanent);
#endif

	/* Validate slot number */
	if (slot == SB3_ACTIVE_SLOT) {
		LOG_ERR("Cannot set pending on active slot");
		return -EINVAL;
	}

	/* Verify staging slot contains a valid SB3 image */
	rc = dfu_boot_read_img_info(slot, &info);
	if (rc != 0 || !info.valid) {
		LOG_ERR("No valid image in slot %d", slot);
		return -ENOENT;
	}

	/* Get flash area for physical address calculation */
	area_id = dfu_boot_get_flash_area_id(slot);
	if (area_id < 0) {
		return area_id;
	}

	rc = flash_area_open(area_id, &fa);
	if (rc != 0) {
		LOG_ERR("Failed to open flash area %d: %d", area_id, rc);
		return -EIO;
	}

	/* Prepare OTA configuration structure */
	memset(&cfg, flash_area_erased_val(fa), sizeof(cfg));

	cfg.update_available = OTA_UPDATE_AVAILABLE_MAGIC;

#if SLOT1_ON_EXTERNAL_FLASH
	cfg.dump_location = OTA_EXTERNAL_FLASH_MAGIC;
	cfg.baud_rate = SLOT1_SPI_MAX_FREQ;
	LOG_INF("Staging slot on external SPI flash (baud_rate=%u Hz)", cfg.baud_rate);
#else
	cfg.dump_location = 0;
	cfg.baud_rate = 0;
	LOG_INF("Staging slot on internal flash");
#endif

	cfg.dump_address = fa->fa_off + dfu_boot_get_image_start_offset(slot);
	cfg.file_size = info.img_size;
	cfg.update_status = 0;
	memcpy(cfg.unlock_key, ota_unlock_key, sizeof(ota_unlock_key));

	flash_area_close(fa);

	LOG_INF("Scheduling OTA update: addr=0x%08x, size=%u bytes", cfg.dump_address,
		cfg.file_size);

	/* Program OTACFG page to trigger update on next boot */
	rc = write_otacfg_page(&cfg);
	if (rc != 0) {
		LOG_ERR("Failed to program OTACFG page: %d", rc);
		return rc;
	}

	LOG_INF("OTA update scheduled for next boot");
	return 0;
}

/**
 * @brief Confirm the currently running image
 *
 * For NXP ROM boot, this is a no-op since updates are permanent once
 * the ROM bootloader successfully installs them. There is no revert
 * mechanism.
 *
 * For hybrid mode with MCUboot image, writes the confirmation flag
 * to prevent automatic revert.
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_confirm(void)
{
#ifdef CONFIG_NXPBOOT_HYBRID_MCUBOOT
	if (get_image_type(SB3_ACTIVE_SLOT) == IMAGE_TYPE_MCUBOOT) {
		return write_mcuboot_img_confirmed();
	}
#endif

	/*
	 * NXP ROM boot: Confirmation is implicit.
	 *
	 * Once the ROM bootloader successfully processes the SB3 file and
	 * installs the new firmware, the update is permanent. The old
	 * firmware is overwritten and cannot be restored.
	 *
	 * This function is a no-op for pure NXP ROM boot mode.
	 */
	LOG_DBG("Image confirmation (no-op for NXP ROM boot)");
	return 0;
}

/* ==========================================================================
 * System Initialization
 *
 * The initialization function runs during POST_KERNEL to handle any
 * cleanup required after a ROM bootloader update attempt.
 * ==========================================================================
 */

/**
 * @brief NXP boot subsystem initialization
 *
 * Called during system startup to handle post-update cleanup:
 *   - Checks the OTA update status from the previous boot
 *   - Clears the OTACFG page after processing
 *   - Erases the staging slot after successful update
 *
 * This ensures the system is in a clean state and ready for the next
 * update cycle.
 *
 * @return 0 (always succeeds)
 */
static int nxp_boot_init(void)
{
	int rc;
	uint32_t update_status;
	uint8_t area_id;
	const struct flash_area *fa;

	update_status = get_ota_update_status();

	if ((update_status == OTA_UPDATE_SUCCESS) || (update_status == OTA_UPDATE_FAIL_ERASE) ||
	    (update_status == OTA_UPDATE_FAIL_SB3)) {

		/* Clear OTACFG to acknowledge the update attempt */
		erase_otacfg_page();

		if (update_status == OTA_UPDATE_SUCCESS) {
			LOG_INF("OTA update completed successfully");
		} else if (update_status == OTA_UPDATE_FAIL_SB3) {
			LOG_WRN("OTA update failed: SB3 processing error (status=0x%08x)",
				update_status);
		} else {
			LOG_WRN("OTA update failed: erase error (status=0x%08x)", update_status);
		}

		area_id = dfu_boot_get_flash_area_id(SB3_STAGING_SLOT);

		rc = flash_area_open(area_id, &fa);
		if (rc != 0) {
			return -EIO;
		}

		/* erase the SB3 header to invalidate the image */
		rc = flash_area_flatten(fa, get_image_start_offset(area_id),
					sizeof(struct sb3_image_header));
		if (rc != 0) {
			return rc;
		}

		flash_area_close(fa);
	}

	return 0;
}

/*
 * Initialize after flash driver but before application starts.
 * Priority 999 ensures we run late in POST_KERNEL phase.
 */
SYS_INIT(nxp_boot_init, POST_KERNEL, 999);
