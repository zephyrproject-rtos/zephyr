/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host Mass Storage — configuration descriptor helpers
 *
 * Locate the MSC interface and bulk IN/OUT endpoints from the device's
 * active configuration descriptor. With ``CONFIG_USBH_MSC_STRICT_PROBE``,
 * filters match MSC class probe rules per the USB Mass Storage specification:
 * ``bDeviceClass == 0``, interface class mass-storage, subclass range,
 * CB/CBI/BBB protocol, bulk endpoints, optional interrupt for CBI,
 * and the UFI/8070/SCSI subclass gate for SCSI commands.
 *
 * @since 4.3
 * @defgroup usbh_msc_api USB host Mass Storage Class
 * @ingroup usb_interfaces
 */

#ifndef ZEPHYR_INCLUDE_USB_USBH_MSC_H_
#define ZEPHYR_INCLUDE_USB_USBH_MSC_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh_msc_bot.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Device class: definitions at interface level (required for strict MSC probe). */
#define USBH_MSC_DEV_CLASS_PER_INTERFACE 0U

/** @name Mass storage subclasses (USB Mass Storage class specification) */
/**@{*/
#define USBH_MSC_SC_RBC  1U
#define USBH_MSC_SC_8020 2U
#define USBH_MSC_SC_QIC  3U
#define USBH_MSC_SC_UFI  4U
#define USBH_MSC_SC_8070 5U
#define USBH_MSC_SC_SCSI 6U
#define USBH_MSC_SC_MIN  USBH_MSC_SC_RBC
#define USBH_MSC_SC_MAX  USBH_MSC_SC_SCSI
/**@}*/

/** @name Mass storage protocols */
/**@{*/
#define USBH_MSC_PR_CBI  0U
#define USBH_MSC_PR_CB   1U
/** Bulk-Only Transport (BBB) protocol id. */
#define USBH_MSC_PR_BULK 0x50U
/**@}*/

/** Subclass: transparent SCSI command set */
#define USBH_MSC_SUBCLASS_SCSI_TRANSPARENT USBH_MSC_SC_SCSI
/** Protocol: USB Mass Storage Bulk-Only (BBB) */
#define USBH_MSC_PROTOCOL_BBB              USBH_MSC_PR_BULK

/**
 * USB Mass Storage class request GET_MAX_LUN (Bulk-Only Transport spec).
 * Issued after SET_INTERFACE to learn how many LUNs exist.
 */
#define USBH_MSC_REQ_GET_MAX_LUN 0xFEU

/** bmRequestType for GET_MAX_LUN: class, IN, interface recipient */
#define USBH_MSC_BMREQ_GET_MAX_LUN                                                                 \
	((uint8_t)((USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |                \
		   USB_REQTYPE_RECIPIENT_INTERFACE))

/**
 * @brief Resolved MSC bulk interface (one alternate setting)
 *
 * ``ep_in_num`` / ``ep_out_num`` / ``ep_int_num`` are endpoint numbers 1–15
 * without the direction bit. ``ep_*_addr`` values are full ``bEndpointAddress``
 * bytes for Zephyr UHC.
 */
struct usbh_msc_iface {
	/** Interface number (bInterfaceNumber) */
	uint8_t iface_num;
	/** Alternate setting (bAlternateSetting) */
	uint8_t alt_setting;
	/** Interface subclass (bInterfaceSubClass) */
	uint8_t subclass;
	/** Interface protocol (bInterfaceProtocol) */
	uint8_t protocol;
	/** Bulk IN endpoint address (bEndpointAddress) */
	uint8_t ep_in_addr;
	/** Bulk OUT endpoint address (bEndpointAddress) */
	uint8_t ep_out_addr;
	/** Bulk IN endpoint number 1–15 (direction bit cleared) */
	uint8_t ep_in_num;
	/** Bulk OUT endpoint number 1–15 (direction bit cleared) */
	uint8_t ep_out_num;
	/** Interrupt endpoint (CBI); 0 if none */
	uint8_t ep_int_addr;
	/** Interrupt endpoint number 1–15; 0 if none */
	uint8_t ep_int_num;
	/** Interrupt endpoint bInterval when present */
	uint8_t irq_interval;
	/** Bulk IN max packet size */
	uint16_t mps_in;
	/** Bulk OUT max packet size */
	uint16_t mps_out;
	/** Bulk-Only Transport (protocol 0x50) */
	bool bulk_only;
	/**
	 * Maximum LUN index from GET_MAX_LUN (0 = single LUN). Filled by
	 * @ref usbh_msc_get_max_lun or @ref usbh_msc_prepare for BBB devices;
	 * otherwise 0. Number of LUNs = @a max_lun + 1.
	 */
	uint8_t max_lun;
};

/**
 * @brief Subclass gate for SCSI-family MSC commands
 *
 * Accepts UFI (4), 8070 (5), or SCSI transparent (6).
 */
static inline bool usbh_msc_subclass_scsi_supported(uint8_t subclass)
{
	return subclass == USBH_MSC_SC_UFI || subclass == USBH_MSC_SC_8070 ||
	       subclass == USBH_MSC_SC_SCSI;
}

/**
 * @brief Find MSC interface with bulk IN and bulk OUT in @a udev's configuration
 *
 * Walks the configuration descriptor blob starting at @c udev->cfg_desc.
 * With ``CONFIG_USBH_MSC_STRICT_PROBE``, applies MSC class descriptor
 * rules (alternate 0 only, class/subclass/protocol checks, CBI int EP).
 *
 * @param udev USB device (must be @ref USB_STATE_CONFIGURED with valid @c cfg_desc)
 * @param out Filled on success
 *
 * @retval 0 MSC bulk interface found
 * @retval -EINVAL NULL argument or bad descriptor
 * @retval -EPERM Device not configured
 * @retval -ENOENT No MSC class or missing bulk pair
 * @retval -ENOTSUP Present but not accepted by SCSI subclass gate
 */
int usbh_msc_find_bulk_interface(const struct usb_device *udev, struct usbh_msc_iface *out);

/**
 * @brief Test whether a configuration descriptor blob contains a usable MSC bulk pair.
 *
 * Used during enumeration before SET_CONFIGURATION. Applies the same class/subclass
 * rules as @ref usbh_msc_find_bulk_interface when ``CONFIG_USBH_MSC_STRICT_PROBE`` is set.
 *
 * @param udev USB device (device descriptor used for strict class check)
 * @param cfg_desc Configuration descriptor (including configuration header)
 * @param len Length of @a cfg_desc in bytes
 *
 * @retval 0 MSC bulk interface present
 * @retval -ENOENT No suitable MSC interface
 * @retval negative errno on malformed descriptor
 */
int usbh_msc_configuration_has_bulk(const struct usb_device *udev, const void *cfg_desc,
				    uint16_t len);

/**
 * @brief GET_MAX_LUN (Bulk-Only), single-LUN fallback
 *
 * Class IN control transfer to the MSC interface. On success the device returns
 * one byte: the maximum LUN index (number of LUNs = that value + 1).
 * If the device STALLs or does not support the request, @a *max_lun_out is set
 * to 0 and 0 is returned (single LUN).
 * @c -ENOMEM and @c -ETIMEDOUT are propagated.
 *
 * @param udev USB device (must be configured)
 * @param iface Interface number from @ref usbh_msc_find_bulk_interface
 * @param max_lun_out Filled with maximum LUN index on success
 *
 * @retval 0 Request succeeded or single-LUN fallback applied
 * @retval -EINVAL Invalid argument
 * @retval -ENOMEM Buffer allocation failed
 * @retval -ETIMEDOUT Control transfer timed out
 */
int usbh_msc_get_max_lun(struct usb_device *udev, uint8_t iface, uint8_t *max_lun_out);

/**
 * @brief Bind MSC interface: find, SET_INTERFACE(alt 0), GET_MAX_LUN (BBB)
 *
 * After endpoint discovery: ``usb_set_interface(dev, ifnum, 0)`` then GET_MAX_LUN
 * for Bulk-Only devices. Uses @ref usbh_device_interface_set() and
 * @ref usbh_msc_get_max_lun(); caller must not hold @c udev->mutex.
 *
 * @param udev USB device to prepare
 * @param out Filled with resolved MSC interface and endpoints on success
 *
 * @retval 0 MSC interface prepared
 * @retval negative errno from find, SET_INTERFACE, or GET_MAX_LUN
 */
int usbh_msc_prepare(struct usb_device *udev, struct usbh_msc_iface *out);

/**
 * @brief Optional INFO log of @ref usbh_msc_find_bulk_interface() result
 *
 * No-op unless ``CONFIG_USBH_MSC_LOG_PROBE`` is enabled.
 *
 * @param udev USB device to probe and log
 */
void usbh_msc_log_probe(const struct usb_device *udev);

/**
 * @brief Optional LUN0 hook during @ref usbh_msc_storage_bringup.
 *
 * When non-NULL, invoked instead of the default LUN0 path (auto scsi_disk attach
 * or probe-only verification). Prefer leaving this NULL and enabling
 * @kconfig:option:`CONFIG_USBH_MSC_DISK_AUTO_ATTACH`.
 *
 * @param udev USB device
 * @param msc Resolved MSC interface from bring-up
 * @param uhc_dev Host controller device for BOT recovery
 *
 * @retval 0 Verification succeeded
 * @retval negative errno on failure
 */
typedef int (*usbh_msc_lun0_verify_fn)(struct usb_device *udev, const struct usbh_msc_iface *msc,
				       const struct device *uhc_dev);

/**
 * @brief MSC Bulk-Only reset (BOT class reset + HALT clear).
 *
 * Issues MSC class request 0xFF to the interface, then clears HALT on bulk IN and
 * OUT endpoints. Optionally invokes UHC bulk pipe sync when @a uhc_dev is non-NULL.
 *
 * @param udev USB device
 * @param uhc_dev Host controller device for endpoint recovery, or NULL
 */
void usbh_msc_bot_reset(struct usb_device *udev, const struct device *uhc_dev);

/**
 * @brief SCSI INQUIRY via SCSI mid-layer (legacy BOT helper).
 *
 * Prefer @ref scsi_inquiry on a bound @ref scsi_device. When @a msc_opt is NULL,
 * the MSC interface is discovered via @ref usbh_msc_find_bulk_interface().
 *
 * @param udev USB device
 * @param msc_opt Resolved MSC interface, or NULL to discover
 * @param lun Logical unit number (0–15)
 *
 * @retval 0 INQUIRY succeeded
 * @retval -EINVAL Invalid argument or unsupported subclass
 * @retval negative errno from BOT transport or CSW status
 */
int usbh_msc_scsi_inquiry(struct usb_device *udev, const struct usbh_msc_iface *msc_opt,
			  uint8_t lun);

/**
 * @brief SCSI MODE SENSE(6) over BOT.
 *
 * @param udev USB device
 * @param msc_opt Resolved MSC interface, or NULL to discover
 * @param lun Logical unit number (0–15)
 * @param pc Page control field (current, changeable, default, saved)
 * @param page_code Mode page to return, or 0x3F for all pages
 * @param subpage Mode subpage code
 * @param alloc_len Maximum number of bytes to return
 * @param disable_block_descriptors When true, sets DBD in the CDB
 * @param uhc_host Optional UHC device for BOT recovery on transport errors
 *
 * @retval 0 MODE SENSE succeeded
 * @retval negative errno from BOT transport or CSW status
 */
int usbh_msc_scsi_mode_sense6(struct usb_device *udev, const struct usbh_msc_iface *msc_opt,
			      uint8_t lun, uint8_t pc, uint8_t page_code, uint8_t subpage,
			      uint8_t alloc_len, bool disable_block_descriptors,
			      const struct device *uhc_host);

/**
 * @brief SCSI READ(10) via SCSI mid-layer (legacy helper).
 *
 * Prefer @ref scsi_read_10 or @ref scsi_io_read on a bound @ref scsi_device.
 *
 * @param udev USB device
 * @param msc Resolved MSC interface
 * @param lun Logical unit number (0–15)
 * @param lba Starting logical block address
 * @param blocks Number of logical blocks to read
 * @param block_size Logical block size in bytes
 * @param uhc_host Optional; CSW PHASE_ERR and HCD errors trigger BOT reset when non-NULL
 * @param data Buffer for read data (@a blocks * @a block_size bytes)
 *
 * @retval 0 READ(10) succeeded
 * @retval negative errno from BOT transport, CSW status, or HCD
 */
int usbh_msc_scsi_read10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			 uint32_t lba, uint16_t blocks, uint32_t block_size,
			 const struct device *uhc_host, void *data);

/**
 * @brief SCSI TEST UNIT READY over BOT (no bulk DATA; CBW OUT only).
 *
 * @param udev USB device
 * @param msc Resolved MSC interface
 * @param lun Logical unit number (0–15)
 * @param uhc_host Optional; CSW errors use the same recovery path as @ref usbh_msc_scsi_read10
 *
 * @retval 0 Device reports ready
 * @retval negative errno from BOT transport or CSW status
 */
int usbh_msc_scsi_test_unit_ready(struct usb_device *udev, const struct usbh_msc_iface *msc,
				  uint8_t lun, const struct device *uhc_host);

/**
 * @brief SCSI WRITE(10) over BOT.
 *
 * @warning Overwrites medium.
 *
 * @param udev USB device
 * @param msc Resolved MSC interface
 * @param lun Logical unit number (0–15)
 * @param lba Starting logical block address
 * @param blocks Number of logical blocks to write
 * @param block_size Logical block size in bytes
 * @param uhc_host Optional; CSW PHASE_ERR and HCD errors trigger BOT reset when non-NULL
 * @param data Buffer containing write data (@a blocks * @a block_size bytes)
 *
 * @retval 0 WRITE(10) succeeded
 * @retval negative errno from BOT transport, CSW status, or HCD
 */
int usbh_msc_scsi_write10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			  uint32_t lba, uint16_t blocks, uint32_t block_size,
			  const struct device *uhc_host, const void *data);

/**
 * @brief SCSI VERIFY(10) over BOT without data (BYTCHK=0).
 *
 * Asks the device to verify @a blocks logical blocks beginning at @a lba with no bulk DATA phase
 * (@c dCBWDataTransferLength = 0). Typical USB MSC devices honor this; others may reply
 * CSW FAILED / PHASE_ERR with the same handling as @ref usbh_msc_scsi_read10 (including
 * REQUEST_SENSE after COMMAND_FAILED when applicable).
 *
 * @param uhc_host Optional; CSW errors use the same recovery path as @ref usbh_msc_scsi_read10
 *
 * @retval 0 VERIFY(10) succeeded
 * @retval negative errno from BOT transport or CSW status
 */
int usbh_msc_scsi_verify10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			   uint32_t lba, uint16_t blocks, const struct device *uhc_host);

/**
 * @brief Storage bring-up (SCSI probe per LUN, optional LUN0 scsi_disk attach).
 *
 * Runs @ref usbh_msc_prepare(), then for each LUN either:
 *
 * - LUN0 with @a lun0_verify: custom hook (legacy)
 * - LUN0 with @kconfig:option:`CONFIG_USBH_MSC_DISK_AUTO_ATTACH`: @ref usb_msc_disk_attach_lun0
 * - otherwise: @c scsi_device_probe via mid-layer (verify-only, unbind after)
 *
 * @param lun0_verify Optional LUN0 override; NULL selects the default path above.
 * @param udev USB device (must reach @ref USB_STATE_CONFIGURED before call)
 * @param uhc_dev Host controller device for BOT recovery on probe retry
 *
 * @retval 0 All required LUNs brought up successfully
 * @retval negative errno from prepare, LUN probe, attach, or @a lun0_verify
 */
int usbh_msc_storage_bringup(struct usb_device *udev, const struct device *uhc_dev,
			     usbh_msc_lun0_verify_fn lun0_verify);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USBH_MSC_H_ */
