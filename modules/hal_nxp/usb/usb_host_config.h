/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_HOST_CONFIG_H__
#define __USB_HOST_CONFIG_H__

#include <zephyr/devicetree.h>
#include "usb.h"

/******************************************************************************
 * Definitions
 *****************************************************************************/
/* EHCI instance count */
#ifdef CONFIG_USB_UHC_NXP_EHCI
#define USB_HOST_CONFIG_EHCI (2U)
#define USB_HOST_CONFIG_EHCI_FRAME_LIST_SIZE (1024U)
#define USB_HOST_CONFIG_EHCI_MAX_QH (8U)
#define USB_HOST_CONFIG_EHCI_MAX_QTD (8U)
#define USB_HOST_CONFIG_EHCI_MAX_ITD (0U)
#define USB_HOST_CONFIG_EHCI_MAX_SITD (0U)
#endif /* CONFIG_USB_DC_NXP_EHCI */

#ifdef CONFIG_USB_UHC_NXP_KHCI
#define USB_HOST_CONFIG_KHCI (1U)
#define USB_HOST_CONFIG_KHCI_DMA_ALIGN_BUFFER (64U)
#endif /* CONFIG_USB_UHC_NXP_KHCI */

#define USB_HOST_CONFIG_MAX_HOST (USB_HOST_CONFIG_KHCI + USB_HOST_CONFIG_EHCI)

#define USB_HOST_CONFIG_MAX_PIPES (16U)
#define USB_HOST_CONFIG_MAX_TRANSFERS (16U)
#define USB_HOST_CONFIG_INTERFACE_MAX_EP (4U)
#define USB_HOST_CONFIG_CONFIGURATION_MAX_INTERFACE (5U)
#define USB_HOST_CONFIG_MAX_POWER (250U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_RETRIES (3U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_STALL_RETRIES (1U)
#define USB_HOST_CONFIG_MAX_NAK (3000U)
#define USB_HOST_CONFIG_CLASS_AUTO_CLEAR_STALL (0U)

#endif /* __USB_HOST_CONFIG_H__ */
