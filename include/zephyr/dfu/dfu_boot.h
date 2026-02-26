/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DFU boot abstraction API.
 *
 * The header declares API functions that can be used to get information
 * on and select application images for boot.
 */

#ifndef ZEPHYR_INCLUDE_DFU_BOOT_H_
#define ZEPHYR_INCLUDE_DFU_BOOT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief DFU Boot Abstraction API
 * @defgroup dfu_boot_api DFU Boot Abstraction API
 * @ingroup os_services
 * @{
 */

/** Split image app. */
#define DFU_BOOT_IMG_F_NON_BOOTABLE             0x00000010
/**
 * Indicates that load_addr stores information on flash/ROM address the
 * image has been built for.
 */
#define DFU_BOOT_IMG_F_ROM_FIXED                0x00000100

/** Image is set for next swap */
#define DFU_BOOT_STATE_F_PENDING	0x01
/** Image has been confirmed. */
#define DFU_BOOT_STATE_F_CONFIRMED	0x02
/** Image is currently active. */
#define DFU_BOOT_STATE_F_ACTIVE		0x04
/** Image is to stay in primary slot after the next boot. */
#define DFU_BOOT_STATE_F_PERMANENT	0x08

/** No swap */
#define DFU_BOOT_SWAP_TYPE_NONE		0
/** Test swap */
#define DFU_BOOT_SWAP_TYPE_TEST		1
/** Permanent swap */
#define DFU_BOOT_SWAP_TYPE_PERM		2
/** Revert swap */
#define DFU_BOOT_SWAP_TYPE_REVERT	3
/** Unknown swap */
#define DFU_BOOT_SWAP_TYPE_UNKNOWN	255

#if defined(CONFIG_MCUBOOT_IMG_MANAGER) || defined(CONFIG_NXPBOOT_HYBRID_MCUBOOT)
#ifdef CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512
#define DFU_BOOT_IMG_SHA_LEN 64
#else
#define DFU_BOOT_IMG_SHA_LEN 32
#endif
#elif defined(CONFIG_NXPBOOT_IMG_MANAGER)
/* NXP SB image can use either SHA-256 or SHA-384 */
#define DFU_BOOT_IMG_SHA_LEN 48
#else
/** Image hash size */
#define DFU_BOOT_IMG_SHA_LEN 64
#endif

/**
 * @brief Image version structure
 */
struct dfu_boot_img_version {
	/** major version number */
	uint8_t major;
	/** minor version number */
	uint8_t minor;
	/** revision version number */
	uint16_t revision;
	/** build number */
	uint32_t build_num;
} __packed;

/**
 * @brief Image information structure
 *
 * This structure is used to pass image information from header validation
 * and image info reading.
 */
struct dfu_boot_img_info {
	/** Image version */
	struct dfu_boot_img_version version;
	/** Image flags */
	uint32_t flags;
	/** Image load address */
	uint32_t load_addr;
	/** Image size */
	uint32_t img_size;
	/** Header size */
	uint16_t hdr_size;
	/** Hash length */
	uint8_t hash_len;
	/** Hash value */
	uint8_t hash[DFU_BOOT_IMG_SHA_LEN];
	/** Whether the image info is valid */
	bool valid;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the flash area ID for a slot
 *
 * @param slot Slot number
 *
 * @return Flash area ID, or negative error code on failure
 */
int dfu_boot_get_flash_area_id(int slot);

/**
 * @brief Get the erased value for a slot's flash
 *
 * @param slot Slot number
 * @param erased_val Pointer to store the erased value
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_get_erased_val(int slot, uint8_t *erased_val);

/**
 * @brief Read image information from a slot
 *
 * @param slot Slot number
 * @param info Pointer to store image information
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_read_img_info(int slot, struct dfu_boot_img_info *info);

/**
 * @brief Read data from a slot
 *
 * @param slot Slot number
 * @param offset Offset within the slot
 * @param dst Destination buffer
 * @param len Number of bytes to read
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_read(int slot, size_t offset, void *dst, size_t len);

/**
 * @brief Validate an image header
 *
 * @param data Pointer to the image data (must contain at least the header)
 * @param len Length of the data
 * @param info Pointer to store extracted image information (can be NULL)
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_validate_header(const void *data, size_t len, struct dfu_boot_img_info *info);

/**
 * @brief Get the swap type for an image
 *
 * @param image_index image index
 *
 * @return Swap type (DFU_BOOT_SWAP_TYPE_* value)
 */
int dfu_boot_get_swap_type(int image_index);

/**
 * @brief Set the next boot slot
 *
 * This function handles both confirming the current image and setting
 * a pending upgrade on another slot.
 *
 * @param slot Slot number to boot next
 * @param active Whether this slot is currently active
 * @param confirm If true, make the change permanent/confirmed
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_set_next(int slot, bool active, bool confirm);

/**
 * @brief Set a slot as pending for next boot
 *
 * @param slot Slot number
 * @param permanent If true, make the change permanent
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_set_pending(int slot, bool permanent);

/**
 * @brief Confirm the current image
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_confirm(void);

/**
 * @brief Erase a slot
 *
 * @param slot Slot number
 *
 * @return 0 on success, negative error code on failure
 */
int dfu_boot_erase_slot(int slot);

/**
 * @brief Get the image offset for a slot
 *
 * @param slot Slot number
 *
 * @return Image offset, or 0 if not applicable
 */
size_t dfu_boot_get_image_start_offset(int slot);

/**
 * @brief Get the trailer status offset for a given flash area size
 *
 * @param area_size Size of the flash area
 *
 * @return Offset of the trailer status area
 */
size_t dfu_boot_get_trailer_status_offset(size_t area_size);

/**
 * @brief Check if a flash area is empty (erased)
 *
 * @param area_id Flash area ID
 *
 * @return 1 if empty, 0 if not empty, negative errno on error
 */
int dfu_boot_flash_area_check_empty(int area_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DFU_BOOT_H_ */
