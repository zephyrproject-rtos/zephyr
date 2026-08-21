/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MSC host transport completion codes (BOT CSW-derived)
 *
 * Opaque transport-layer status values returned by BOT helpers. SCSI CDB builders
 * in @ref usbh_msc_scsi_cmd.h do not use these directly.
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USB_USBH_MSC_TRANSPORT_ERRNO_H_
#define ZEPHYR_INCLUDE_USB_USBH_MSC_TRANSPORT_ERRNO_H_

#include <errno.h>

/** CSW @c bCSWStatus == command failed (REQUEST SENSE path; not for HCD xfer errors). */
#define USBH_MSC_TRANSPORT_ERR_CSW_COMMAND_FAILED (-(int)ENOTSUP)
/** CSW @c PHASE_ERR — bulk-only reset recovery when UHC device is available. */
#define USBH_MSC_TRANSPORT_ERR_CSW_PHASE_ERROR    (-(int)EPROTO)

#endif /* ZEPHYR_INCLUDE_USB_USBH_MSC_TRANSPORT_ERRNO_H_ */
