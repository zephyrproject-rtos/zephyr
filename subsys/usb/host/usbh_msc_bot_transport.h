/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MSC Bulk-Only Transport (BOT) subsystem-internal API
 *
 * Implements CBW / DATA / CSW on top of Chapter 9 control requests and bulk UHC
 * transfers. Callers pass an optional @c uhc_dev pointer so recovery paths can
 * invoke @ref usbh_ep_sync_after_clear_feature and related HCD helpers.
 */

#ifndef SUBSYS_USB_HOST_USBH_MSC_BOT_TRANSPORT_H_
#define SUBSYS_USB_HOST_USBH_MSC_BOT_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct net_buf;
struct usb_device;
struct usbh_msc_iface;
struct device;

/**
 * @brief Run one BOT command (CBW OUT, optional DATA, CSW IN).
 *
 * Synchronous helper used by @c usbh_msc_scsi.c. Allocates and fills the CBW,
 * performs bulk OUT/IN phases as directed by @a data_in and @a data_len, then
 * validates the CSW.
 *
 * @param udev USB device
 * @param msc Resolved MSC bulk interface
 * @param lun Target logical unit (0–15)
 * @param cdb SCSI command descriptor block
 * @param cdb_len Valid CDB length in bytes
 * @param data_in True when bulk DATA IN is expected after CBW
 * @param data_len Expected DATA length in bytes (0 for command-only)
 * @param data_out_payload Source buffer for bulk DATA OUT, or NULL
 * @param phase Short label for logging (e.g. "READ10")
 * @param data_in_out When @a data_in is true, receives allocated IN buffer on success
 * @param uhc_host Optional UHC device for transport recovery on xfer/CSW errors
 *
 * @retval 0 BOT transaction completed with CSW GOOD
 * @retval @ref USBH_MSC_TRANSPORT_ERR_CSW_COMMAND_FAILED CSW command failed
 * @retval @ref USBH_MSC_TRANSPORT_ERR_CSW_PHASE_ERROR CSW phase error
 * @retval negative errno from control/bulk transfer or buffer allocation
 */
int usbh_msc_bot_issue(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
		       const uint8_t *cdb, uint8_t cdb_len, bool data_in, uint32_t data_len,
		       const uint8_t *data_out_payload, const char *phase,
		       struct net_buf **data_in_out, const struct device *uhc_host);

/**
 * @brief MSC Bulk-Only reset (class request 0xFF + HALT clear).
 *
 * Public wrapper declared in @ref usbh_msc.h; implemented here.
 *
 * @param udev USB device
 * @param uhc_dev Host controller device for endpoint recovery, or NULL
 */
void usbh_msc_bot_reset(struct usb_device *udev, const struct device *uhc_dev);

/**
 * @brief Seed BOT transport session state before a new bring-up sequence.
 *
 * Resets internal CBW tag sequencing and recovery flags.
 */
void usbh_msc_bot_transport_session_seed(void);

/**
 * @brief Run BOT transport recovery after a bulk or CSW error.
 *
 * Invokes @ref usbh_msc_bot_reset and re-seeds the transport session when
 * @a uhc_dev is non-NULL.
 *
 * @param udev USB device
 * @param uhc_dev Host controller device for endpoint recovery
 */
void usbh_msc_bot_transport_recover_after_err(struct usb_device *udev,
					      const struct device *uhc_dev);

/**
 * @brief Notify BOT transport that TEST UNIT READY succeeded.
 *
 * Clears transport-side "device not ready" pacing state used between commands.
 */
void usbh_msc_bot_transport_after_tur_ok(void);

/**
 * @brief Override BOT transfer timeout for the next command (0 = Kconfig default)
 */
void usbh_msc_bot_set_command_timeout_ms(uint32_t timeout_ms);

#endif /* SUBSYS_USB_HOST_USBH_MSC_BOT_TRANSPORT_H_ */
