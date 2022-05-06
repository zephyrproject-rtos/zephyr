/*
 * Copyright 2018 - 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_DC_MCUX_H__
#define __USB_DC_MCUX_H__

#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/sys/byteorder.h>
#include "usb_spec.h"
#include "usb.h"
#include "usb_device_dci.h"
#include "usb_device.h"

/******************************************************************************
 * Definitions
 *****************************************************************************/
/* EHCI instance count */
#ifdef CONFIG_USB_DC_NXP_EHCI
#define USB_DEVICE_CONFIG_EHCI (1U)
/* How many the DTD are supported. */
#define USB_DEVICE_CONFIG_EHCI_MAX_DTD (16U)
/* Whether device is self power. 1U supported, 0U not supported */
#define USB_DEVICE_CONFIG_SELF_POWER (1U)
#endif

#ifdef CONFIG_USB_DC_NXP_LPCIP3511

#ifdef USBHSD_BASE_ADDRS
#define USB_DEVICE_CONFIG_LPCIP3511HS (1U)
#else
#define USB_DEVICE_CONFIG_LPCIP3511HS (0U)
#endif

#ifdef USB_BASE_ADDRS
#define USB_DEVICE_CONFIG_LPCIP3511FS (1U)
#else
#define USB_DEVICE_CONFIG_LPCIP3511FS (0U)
#endif

/* Whether device is self power. 1U supported, 0U not supported */
#define USB_DEVICE_CONFIG_SELF_POWER (1U)

#endif

#if defined(CONFIG_BIG_ENDIAN)
#define USB_SHORT_FROM_LITTLE_ENDIAN(n) __bswap_16(n)
#else
#define USB_SHORT_FROM_LITTLE_ENDIAN(n) (n)
#endif

#if (((defined(USB_DEVICE_CONFIG_LPCIP3511FS)) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)) || \
	((defined(USB_DEVICE_CONFIG_LPCIP3511HS)) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)))
#define USB_DATA_ALIGN_SIZE 64U
#else
#define USB_DATA_ALIGN_SIZE 4U
#endif

#define USB_DATA_ALIGN_SIZE_MULTIPLE(n) (((n) + USB_DATA_ALIGN_SIZE - 1U) & \
						(~(USB_DATA_ALIGN_SIZE - 1U)))

/* Number of endpoints supported */
#define USB_DEVICE_CONFIG_ENDPOINTS (DT_INST_PROP(0, num_bidir_endpoints))

/* controller driver do the ZLP for controller transfer automatically or not */
#define USB_DEVICE_CONTROLLER_AUTO_CONTROL_TRANSFER_ZLP (0)

/* endpoint related macros */
#define EP0_MAX_PACKET_SIZE    64
#define EP0_OUT 0
#define EP0_IN  0x80

/* enter critical macros */
#define OSA_SR_ALLOC() int usbOsaCurrentSr
#define OSA_ENTER_CRITICAL() usbOsaCurrentSr = irq_lock()
#define OSA_EXIT_CRITICAL() irq_unlock(usbOsaCurrentSr)

/* NXP SDK USB controller driver configuration macros */
#if defined(CONFIG_USB_DEDICATED_MEMORY)
#define USB_BDT __attribute__((section("m_usb_bdt, \"aw\", %nobits @")))
#define USB_GLOBAL __attribute__((section("m_usb_global, \"aw\", %nobits @")))
#else
#define USB_BDT
#define USB_GLOBAL
#endif

#define USB_RAM_ADDRESS_ALIGNMENT(n) __aligned(n)

/* EHCI */
#if defined(CONFIG_NOCACHE_MEMORY)
#define USB_CONTROLLER_DATA __nocache
#elif defined(CONFIG_USB_DEDICATED_MEMORY)
#define USB_CONTROLLER_DATA __attribute__((section("m_usb_global, \"aw\", %nobits @")))
#else
#define USB_CONTROLLER_DATA
#endif

struct usb_ep_ctrl_data {
	usb_device_callback_message_struct_t transfer_message;
	struct k_mem_block block;
	usb_dc_ep_callback callback;
	uint16_t ep_mps;
	uint8_t ep_type;
	uint8_t ep_enabled : 1;
	uint8_t ep_occupied : 1;
};

#define USBD_MCUX_THREAD_STACK_SIZE		1024

struct usb_device_struct {
#if ((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && \
		(USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U)) || \
	(defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && \
		(FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
	/* Current hw tick(ms)*/
	volatile uint64_t hwTick;
#endif
	/* Controller handle */
	usb_device_controller_handle controllerHandle;
	/* Controller interface handle */
	const usb_device_controller_interface_struct_t *interface;
	usb_dc_status_callback status_callback;
	struct usb_ep_ctrl_data *eps;
	bool attached;
	/* Current device address */
	uint8_t address;
	/* Controller ID */
	uint8_t controllerId;
	/* Current device state */
	uint8_t state;
#if ((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U))
	/* Remote wakeup is enabled or not */
	uint8_t remotewakeup;
#endif
	/* Is doing device reset or not */
	uint8_t isResetting;
	uint8_t setupDataStage;
	K_KERNEL_STACK_MEMBER(thread_stack, USBD_MCUX_THREAD_STACK_SIZE);
	struct k_thread thread;
};

#endif /* __USB_DC_MCUX_H__ */
