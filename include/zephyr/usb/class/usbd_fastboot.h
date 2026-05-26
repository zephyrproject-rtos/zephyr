/*
 * Copyright (c) 2026 jeck chen <baidxi404629@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Fastboot class public API
 *
 * Implements Android Fastboot protocol for firmware download via USB.
 * The class layer handles USB protocol and command parsing.
 * The application layer provides hardware-specific operations via
 * fastboot_ops callbacks.
 *
 * @defgroup usbd_fastboot USB Fastboot device API
 * @ingroup usb
 * @since 4.2
 * @{
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_FASTBOOT_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_FASTBOOT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/usb/usbd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fastboot partition information
 */
struct fastboot_part {
	/** @brief Partition name (e.g. "spl", "tpl", "app") */
	const char *name;
	/** @brief Partition start address in flash */
	uint32_t addr;
	/** @brief Partition size in bytes */
	uint32_t size;
	/** @brief Firmware magic value for header verification */
	uint32_t magic;
	/** @brief True if partition resides on internal flash */
	bool internal;
};

/**
 * @brief Fastboot USB device configuration
 *
 * Bundles all USB descriptor nodes and configuration nodes needed
 * by the Fastboot device application layer. Defined by the application
 * using USBD_FASTBOOT_CONFIG_DEFINE and automatically placed in a
 * linker section for discovery by usbd_fastbootd.
 */
struct usbd_fastboot_config {
	/** Pointer to USB device context */
	struct usbd_context *ctx;
	/** Language string descriptor */
	struct usbd_desc_node *lang;
	/** Manufacturer string descriptor */
	struct usbd_desc_node *mfr;
	/** Product string descriptor */
	struct usbd_desc_node *product;
	/** Serial number string descriptor (optional, NULL if unused) */
	struct usbd_desc_node *sn;
	/** FS configuration string descriptor */
	struct usbd_desc_node *fs_cfg_str;
	/** HS configuration string descriptor */
	struct usbd_desc_node *hs_cfg_str;
	/** FS configuration node */
	struct usbd_config_node *fs_config;
	/** HS configuration node */
	struct usbd_config_node *hs_config;
};

/**
 * @brief Define a fastboot_part entry from a DTS fixed-partition node
 *
 * Extracts partition name (label property), start address, and size
 * from the device tree fixed-partitions node identified by its DTS
 * node label.  This is the preferred method when partition information
 * is defined in DTS.
 *
 * The DTS node label (the token before the colon in DTS) must match
 * the @p node_label argument.
 *
 * Example — given this DTS overlay:
 * @code
 *   &flash0 {
 *       partitions {
 *           boot: partition@0 {
 *               label = "boot";
 *               reg = <0x00000000 DT_SIZE_K(128)>;
 *           };
 *       };
 *   };
 * @endcode
 *
 * Usage in C:
 * @code
 *   FASTBOOT_PART_DEFINE(boot, FW_MAGIC_SPL, true)
 * @endcode
 *
 * @param node_label  DTS node label (e.g. boot, app — NOT DT_NODELABEL(...))
 * @param _magic      Firmware magic value for verification
 * @param _internal   Whether this partition is on internal flash
 */
#define FASTBOOT_PART_DEFINE(node_label, _magic, _internal)                                        \
	{                                                                                          \
		.name = DT_PROP(DT_NODELABEL(node_label), label),                                  \
		.addr = DT_REG_ADDR(DT_NODELABEL(node_label)),                                     \
		.size = DT_REG_SIZE(DT_NODELABEL(node_label)),                                     \
		.magic = _magic,                                                                   \
		.internal = _internal,                                                             \
	}

/**
 * @brief Define a fastboot_part entry with explicit (non-DTS) values
 *
 * Use this when partition information is not available from device tree
 * (e.g. in sample applications, or for partitions on external storage
 * that are not modeled as fixed-partitions in DTS).
 *
 * Example usage:
 * @code
 *   FASTBOOT_PART_DEFINE_MANUAL("spl", 0x00000000, 0x20000, FW_MAGIC_SPL, true)
 * @endcode
 *
 * @param _name     Partition name string (e.g. "spl", "app")
 * @param _addr     Partition start address in flash
 * @param _size     Partition size in bytes
 * @param _magic    Firmware magic value for verification
 * @param _internal Whether this partition is on internal flash
 */
#define FASTBOOT_PART_DEFINE_MANUAL(_name, _addr, _size, _magic, _internal)                        \
	{                                                                                          \
		.name = _name,                                                                     \
		.addr = _addr,                                                                     \
		.size = _size,                                                                     \
		.magic = _magic,                                                                   \
		.internal = _internal,                                                             \
	}

/**
 * @brief Fastboot application operations
 *
 * Application must implement these callbacks to provide
 * hardware-specific flash and partition operations.
 */
struct fastboot_ops {
	/**
	 * @brief Look up partition by name
	 *
	 * @param name  Partition name (e.g. "spl", "tpl", "app")
	 * @param part  Output: filled with partition info on success
	 * @return 0 on success, negative errno on failure
	 */
	int (*get_partition)(const char *name, struct fastboot_part *part);

	/**
	 * @brief Erase flash region
	 *
	 * @param addr  Start address
	 * @param size  Number of bytes to erase
	 * @return 0 on success, negative errno on failure
	 */
	int (*flash_erase)(uint32_t addr, uint32_t size);

	/**
	 * @brief Write data to flash
	 *
	 * @param addr  Start address
	 * @param data  Data to write
	 * @param size  Number of bytes to write
	 * @return 0 on success, negative errno on failure
	 */
	int (*flash_write)(uint32_t addr, const uint8_t *data, uint32_t size);

	/**
	 * @brief Verify firmware header
	 *
	 * @param data            Firmware data buffer
	 * @param size            Firmware data size
	 * @param expected_magic  Expected magic value for this partition
	 * @return 0 on success, negative errno on failure
	 */
	int (*firmware_verify)(const uint8_t *data, uint32_t size, uint32_t expected_magic);

	/**
	 * @brief Called after successful flash of a partition
	 *
	 * Optional. Can be NULL.
	 *
	 * @param partition  Name of the partition that was flashed
	 */
	void (*on_flash_done)(const char *partition);
};

/**
 * @brief Register application callbacks for Fastboot
 *
 * Must be called before usbd_init().
 *
 * @param ops  Pointer to fastboot_ops (must remain valid indefinitely)
 * @return 0 on success, negative errno on failure
 */
int fastboot_register_ops(const struct fastboot_ops *ops);

/**
 * @brief Register a static partition table
 *
 * Convenience helper for applications that have a simple static
 * partition table. Internally implements get_partition() by
 * searching the table by name.
 *
 * If this is used, the application does not need to provide
 * its own get_partition() callback in fastboot_ops.
 *
 * @param parts      Array of fastboot_part entries
 * @param num_parts  Number of entries in the array
 * @return 0 on success, negative errno on failure
 */
int fastboot_register_partitions(const struct fastboot_part *parts, size_t num_parts);

/**
 * @brief Initialize Fastboot download buffer
 *
 * Optional. If not called, buffer is allocated on first download.
 *
 * @return 0 on success, negative errno on failure
 */
int fastboot_init_download_buf(void);

/**
 * @brief Get the Fastboot USB device configuration
 *
 * Retrieves the usbd_fastboot_config structure placed in the
 * dedicated linker section by USBD_FASTBOOT_CONFIG_DEFINE.
 *
 * @return Pointer to usbd_fastboot_config, or NULL if none defined
 */
const struct usbd_fastboot_config *usbd_fastboot_config_get(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_FASTBOOT_H_ */
