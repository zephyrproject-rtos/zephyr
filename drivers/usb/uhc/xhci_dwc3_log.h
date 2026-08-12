/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI host logging helpers.
 */

#ifndef ZEPHYR_USB_XHCI_DWC3_LOG_H
#define ZEPHYR_USB_XHCI_DWC3_LOG_H

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
#define UHC_DWC3_DBG(...) LOG_DBG(__VA_ARGS__)
#else
#define UHC_DWC3_DBG(...)
#endif

#if IS_ENABLED(CONFIG_UHC_DWC3_BULK_FLOW_LOG)
#define UHC_DWC3_BULK_FLOW_INF(...) LOG_INF(__VA_ARGS__)
#else
#define UHC_DWC3_BULK_FLOW_INF(...)
#endif

#endif /* ZEPHYR_USB_XHCI_DWC3_LOG_H */
