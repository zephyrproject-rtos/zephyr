/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_HOST_CONFIG_H__
#define __USB_HOST_CONFIG_H__

#include <zephyr/devicetree.h>

/******************************************************************************
 * Definitions
 *****************************************************************************/
/* EHCI instance count */
#ifdef CONFIG_UHC_NXP_EHCI
#define USB_HOST_CONFIG_EHCI                 (2U)
#define USB_HOST_CONFIG_EHCI_FRAME_LIST_SIZE (1024U)
#define USB_HOST_CONFIG_EHCI_MAX_QH          (16U)
#define USB_HOST_CONFIG_EHCI_MAX_QTD         (16U)
#define USB_HOST_CONFIG_EHCI_MAX_ITD         (16U)
#define USB_HOST_CONFIG_EHCI_MAX_SITD        (16U)
#else
#define USB_HOST_CONFIG_EHCI (0U)
#endif /* CONFIG_USB_DC_NXP_EHCI */

#ifdef CONFIG_UHC_NXP_KHCI
#define USB_HOST_CONFIG_KHCI                  (1U)
#define USB_HOST_CONFIG_KHCI_DMA_ALIGN_BUFFER (64U)
#else
#define USB_HOST_CONFIG_KHCI (0U)
#endif /* CONFIG_UHC_NXP_KHCI */

#ifdef CONFIG_UHC_NXP_IP3516HS
#define USB_HOST_CONFIG_IP3516HS          (1U)
#define USB_HOST_CONFIG_IP3516HS_MAX_PIPE (32U)
#define USB_HOST_CONFIG_IP3516HS_MAX_ATL  (32U)
#define USB_HOST_CONFIG_IP3516HS_MAX_INT  (32U)
#define USB_HOST_CONFIG_IP3516HS_MAX_ISO  (0U)
#else
#define USB_HOST_CONFIG_IP3516HS (0U)
#endif /* CONFIG_UHC_NXP_IP3516HS */

#ifdef CONFIG_UHC_NXP_OHCI
#define USB_HOST_CONFIG_OHCI         (1U)
#define USB_HOST_CONFIG_OHCI_MAX_ED  (8U)
#define USB_HOST_CONFIG_OHCI_MAX_GTD (8U)
#define USB_HOST_CONFIG_OHCI_MAX_ITD (8U)
#else
#define USB_HOST_CONFIG_OHCI (0U)
#endif /* CONFIG_UHC_NXP_OHCI */

#define USB_HOST_CONFIG_MAX_HOST                                                                   \
	(USB_HOST_CONFIG_KHCI + USB_HOST_CONFIG_EHCI + USB_HOST_CONFIG_IP3516HS +                  \
	 USB_HOST_CONFIG_OHCI)

#define USB_HOST_CONFIG_MAX_PIPES                     (16U)
#define USB_HOST_CONFIG_MAX_TRANSFERS                 (16U)
#define USB_HOST_CONFIG_INTERFACE_MAX_EP              (4U)
#define USB_HOST_CONFIG_CONFIGURATION_MAX_INTERFACE   (5U)
#define USB_HOST_CONFIG_MAX_POWER                     (250U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_RETRIES       (3U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_STALL_RETRIES (1U)
#define USB_HOST_CONFIG_MAX_NAK                       (3000U)
#define USB_HOST_CONFIG_CLASS_AUTO_CLEAR_STALL        (0U)

#endif /* __USB_HOST_CONFIG_H__ */
