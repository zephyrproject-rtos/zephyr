/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USB_HOST_USBH_MSC_FLOW_H_
#define ZEPHYR_SUBSYS_USB_HOST_USBH_MSC_FLOW_H_

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

/** Human-readable Zephyr errno for MSC flow logs (numeric value still logged). */
static inline const char *usbh_msc_errno_name(int err)
{
	switch (err) {
	case 0:
		return "OK";
	case -EINVAL:
		return "EINVAL";
	case -ENOMEM:
		return "ENOMEM";
	case -EIO:
		return "EIO";
	case -EPIPE:
		return "EPIPE";
	case -ETIMEDOUT:
		return "ETIMEDOUT";
	case -ECONNRESET:
		return "ECONNRESET";
	case -EPERM:
		return "EPERM";
	case -ENODEV:
		return "ENODEV";
	case -EBUSY:
		return "EBUSY";
	case -ENOTCONN:
		return "ENOTCONN";
	case -ENOENT:
		return "ENOENT";
	case -EAGAIN:
		return "EAGAIN";
	case -EPROTO:
		return "EPROTO";
	default:
		return "errno";
	}
}

#define USBH_MSC_ERR_FMT    "ret=%d (%s)"
#define USBH_MSC_ERR_ARG(e) (e), usbh_msc_errno_name(e)

#if IS_ENABLED(CONFIG_USBH_MSC_FLOW_LOG)
#define USBH_MSC_FLOW_INF(...) LOG_INF(__VA_ARGS__)
#else
#define USBH_MSC_FLOW_INF(...)
#endif

#endif /* ZEPHYR_SUBSYS_USB_HOST_USBH_MSC_FLOW_H_ */
