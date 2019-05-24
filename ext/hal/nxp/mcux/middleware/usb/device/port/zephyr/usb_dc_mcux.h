/*
 * Copyright 2018 - 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_DC_MCUX_H__
#define __USB_DC_MCUX_H__

#include <usb/usb_dc.h>
#include "usb_spec.h"

/******************************************************************************
 * Definitions
 *****************************************************************************/
/* EHCI instance count */
#ifdef CONFIG_USB_DC_NXP_EHCI
    #define USB_DEVICE_CONFIG_EHCI (1U)
#else
    #define USB_DEVICE_CONFIG_EHCI (0U)
#endif

/* Macro to define controller handle */
typedef void *usb_device_handle;
#define usb_device_controller_handle usb_device_handle

/* controller driver do the ZLP for controler transfer automatically or not */
#define USB_DEVICE_CONTROLLER_AUTO_CONTROL_TRANSFER_ZLP (0)

/* endpoint related macros */
#define EP0_MAX_PACKET_SIZE    64
#define EP0_OUT 0
#define EP0_IN  0x80
/* USB endpoint mask */
#define USB_ENDPOINT_NUMBER_MASK (0x0FU)
/* The setup packet size of USB control transfer. */
#define USB_SETUP_PACKET_SIZE (8U)

/* enter critical macros */
#define USB_OSA_SR_ALLOC() int usbOsaCurrentSr
#define USB_OSA_ENTER_CRITICAL() usbOsaCurrentSr = irq_lock()
#define USB_OSA_EXIT_CRITICAL() irq_unlock(usbOsaCurrentSr)

/* Control endpoint index */
#define USB_CONTROL_ENDPOINT (0U)

/* Default invalid value or the endpoint callback length of cancelled transfer*/
#define USB_UNINITIALIZED_VAL_32 (0xFFFFFFFFU)

/* NXP SDK USB controller driver configuration macros */
#define USB_BDT
#define USB_GLOBAL
#define USB_DATA_ALIGN_SIZE 4
#define USB_RAM_ADDRESS_ALIGNMENT(n) __aligned(n)

/* EHCI */
#if defined(CONFIG_NOCACHE_MEMORY)
#define USB_CONTROLLER_DATA __nocache
#else
#define USB_CONTROLLER_DATA
#endif
/* How many the DTD are supported. */
#define USB_DEVICE_CONFIG_EHCI_MAX_DTD (16U)
/* Control endpoint maxPacketSize */
#define USB_CONTROL_MAX_PACKET_SIZE (64U)

/* Whether device is self power. 1U supported, 0U not supported */
#define USB_DEVICE_CONFIG_SELF_POWER (1U)

/* USB controller ID */
typedef enum _usb_controller_index {
	/* KHCI 0U */
	kUSB_ControllerKhci0            = 0U,
	/* KHCI 1U, Currently, there are no platforms which have two KHCI IPs,
	 * this is reserved to be used in the future.
	 */
	kUSB_ControllerKhci1            = 1U,

	/* EHCI 0U */
	kUSB_ControllerEhci0            = 2U,
	/* EHCI 1U, Currently, there are no platforms which have two EHCI IPs,
	 * this is reserved to be used in the future.
	 */
	kUSB_ControllerEhci1            = 3U,

	/* LPC USB IP3511 FS controller 0 */
	kUSB_ControllerLpcIp3511Fs0     = 4U,
	/* LPC USB IP3511 FS controller 1, there are no platforms which have two
	 * IP3511 IPs, this is reserved to be used in the future.
	 */
	kUSB_ControllerLpcIp3511Fs1     = 5U,

	/* LPC USB IP3511 HS controller 0 */
	kUSB_ControllerLpcIp3511Hs0     = 6U,
	/* LPC USB IP3511 HS controller 1, there are no platforms which have two
	 * IP3511 IPs, this is reserved to be used in the future.
	 */
	kUSB_ControllerLpcIp3511Hs1     = 7U,
} usb_controller_index_t;

/* Control type for controller */
typedef enum _usb_device_control_type {
	/* Enable the device functionality */
	kUSB_DeviceControlRun = 0U,
	/* Disable the device functionality */
	kUSB_DeviceControlStop,
	/* Initialize a specified endpoint */
	kUSB_DeviceControlEndpointInit,
	/* De-initialize a specified endpoint */
	kUSB_DeviceControlEndpointDeinit,
	/* Stall a specified endpoint */
	kUSB_DeviceControlEndpointStall,
	/* Unstall a specified endpoint */
	kUSB_DeviceControlEndpointUnstall,
	/* Get device status */
	kUSB_DeviceControlGetDeviceStatus,
	/* Get endpoint status */
	kUSB_DeviceControlGetEndpointStatus,
	/* Set device address */
	kUSB_DeviceControlSetDeviceAddress,
	/* Get current frame */
	kUSB_DeviceControlGetSynchFrame,
	/* Drive controller to generate a resume signal in USB bus */
	kUSB_DeviceControlResume,
	/* Drive controller to generate a LPM resume signal in USB bus */
	kUSB_DeviceControlSleepResume,
	/* Drive controller to enetr into suspend mode */
	kUSB_DeviceControlSuspend,
	/* Drive controller to enetr into sleep mode */
	kUSB_DeviceControlSleep,
	/* Set controller to default status */
	kUSB_DeviceControlSetDefaultStatus,
	/* Get current speed */
	kUSB_DeviceControlGetSpeed,
	/* Get OTG status */
	kUSB_DeviceControlGetOtgStatus,
	/* Set OTG status */
	kUSB_DeviceControlSetOtgStatus,
	/* Drive xCHI into test mode */
	kUSB_DeviceControlSetTestMode,
	/* Get flag of LPM Remote Wake-up Enabled by USB host. */
	kUSB_DeviceControlGetRemoteWakeUp,
#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && \
	(USB_DEVICE_CHARGER_DETECT_ENABLE > 0U))
	kUSB_DeviceControlDcdInitModule,
	kUSB_DeviceControlDcdDeinitModule,
#endif
} usb_device_control_type_t;

/* Available notify types for device notification */
typedef enum _usb_device_notification {
	/* Reset signal detected */
	kUSB_DeviceNotifyBusReset = 0x10U,
	/* Suspend signal detected */
	kUSB_DeviceNotifySuspend,
	/* Resume signal detected */
	kUSB_DeviceNotifyResume,
	/* LPM signal detected */
	kUSB_DeviceNotifyLPMSleep,
	/* Resume signal detected */
	kUSB_DeviceNotifyLPMResume,
	/* Errors happened in bus */
	kUSB_DeviceNotifyError,
	/* Device disconnected from a host */
	kUSB_DeviceNotifyDetach,
	/* Device connected to a host */
	kUSB_DeviceNotifyAttach,
#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && \
	(USB_DEVICE_CHARGER_DETECT_ENABLE > 0U))
	/* Device charger detection timeout */
	kUSB_DeviceNotifyDcdTimeOut,
	/* Device charger detection unknown port type */
	kUSB_DeviceNotifyDcdUnknownPortType,
	/* The SDP facility is detected */
	kUSB_DeviceNotifySDPDetected,
	/* The charging port is detected */
	kUSB_DeviceNotifyChargingPortDetected,
	/* The CDP facility is detected */
	kUSB_DeviceNotifyChargingHostDetected,
	/* The DCP facility is detected */
	kUSB_DeviceNotifyDedicatedChargerDetected,
#endif
} usb_device_notification_t;

/* USB error code */
typedef enum _usb_status {
	/* Success */
	kStatus_USB_Success = 0x00U,
	/* Failed */
	kStatus_USB_Error,

	/* Busy */
	kStatus_USB_Busy,
	/* Invalid handle */
	kStatus_USB_InvalidHandle,
	/* Invalid parameter */
	kStatus_USB_InvalidParameter,
	/* Invalid request */
	kStatus_USB_InvalidRequest,
	/* Controller cannot be found */
	kStatus_USB_ControllerNotFound,
	/* Invalid controller interface */
	kStatus_USB_InvalidControllerInterface,

	/* Configuration is not supported */
	kStatus_USB_NotSupported,
	/* Enumeration get configuration retry */
	kStatus_USB_Retry,
	/* Transfer stalled */
	kStatus_USB_TransferStall,
	/* Transfer failed */
	kStatus_USB_TransferFailed,
	/* Allocation failed */
	kStatus_USB_AllocFail,
	/* Insufficient swap buffer for KHCI */
	kStatus_USB_LackSwapBuffer,
	/* The transfer cancelled */
	kStatus_USB_TransferCancel,
	/* Allocate bandwidth failed */
	kStatus_USB_BandwidthFail,
	/* For MSD, the CSW status means fail */
	kStatus_USB_MSDStatusFail,
	kStatus_USB_EHCIAttached,
	kStatus_USB_EHCIDetached,
} usb_status_t;

/* Device notification message structure */
typedef struct _usb_device_callback_message_struct {
	u8_t *buffer;   /* Transferred buffer */
	u32_t length;   /* Transferred data length */
	u8_t code;      /* Notification code */
	u8_t isSetup;   /* Is in a setup phase */
} usb_device_callback_message_struct_t;

typedef struct usb_ep_ctrl_data {
	usb_device_callback_message_struct_t transfer_message;
	struct k_mem_block block;
	usb_dc_ep_callback callback;
	u16_t ep_mps;
	u8_t ep_type;
	u8_t ep_enabled : 1;
	u8_t ep_occupied : 1;
} usb_ep_ctrl_data_t;

/* USB device controller initialization function typedef */
typedef usb_status_t (*usb_device_controller_init_t)(u8_t controllerId,
						     usb_device_handle handle,
						     usb_device_controller_handle *controllerHandle);

/* USB device controller de-initialization function typedef */
typedef usb_status_t (*usb_device_controller_deinit_t)(usb_device_controller_handle controllerHandle);

/* USB device controller send data function typedef */
typedef usb_status_t (*usb_device_controller_send_t)(usb_device_controller_handle controllerHandle,
						     u8_t endpointAddress,
						     u8_t *buffer,
						     u32_t length);

/* USB device controller receive data function typedef */
typedef usb_status_t (*usb_device_controller_recv_t)(usb_device_controller_handle controllerHandle,
						     u8_t endpointAddress,
						     u8_t *buffer,
						     u32_t length);

/* USB device controller cancel transfer function
 * in a specified endpoint typedef
 */
typedef usb_status_t (*usb_device_controller_cancel_t)(usb_device_controller_handle controllerHandle,
						       u8_t endpointAddress);

/* USB device controller control function typedef */
typedef usb_status_t (*usb_device_controller_control_t)(usb_device_controller_handle controllerHandle,
							usb_device_control_type_t command,
							void *param);

/* USB device controller interface structure */
typedef struct _usb_device_controller_interface_struct {
	/* Controller initialization */
	usb_device_controller_init_t deviceInit;
	/* Controller de-initialization */
	usb_device_controller_deinit_t deviceDeinit;
	/* Controller send data */
	usb_device_controller_send_t deviceSend;
	/* Controller receive data */
	usb_device_controller_recv_t deviceRecv;
	/* Controller cancel transfer */
	usb_device_controller_cancel_t deviceCancel;
	/* Controller control */
	usb_device_controller_control_t deviceControl;
} usb_device_controller_interface_struct_t;

typedef struct _usb_device_struct {
	/* Controller handle */
	usb_device_controller_handle controllerHandle;
	/* Controller interface handle */
	const usb_device_controller_interface_struct_t *interface;
	usb_dc_status_callback status_callback;
	usb_ep_ctrl_data_t *eps;
	bool attached;
	/* Current device address */
	u8_t address;
	/* Controller ID */
	u8_t controllerId;
	/* Current device state */
	u8_t state;
	/* Is doing device reset or not */
	u8_t isResetting;
	u8_t setupDataStage;
} usb_device_struct_t;

/* Endpoint status structure */
typedef struct _usb_device_endpoint_status_struct {
	/* Endpoint address */
	u8_t endpointAddress;
	/* Endpoint status : idle or stalled */
	u16_t endpointStatus;
} usb_device_endpoint_status_struct_t;

/* Defines endpoint state */
typedef enum _usb_endpoint_status {
	/* Endpoint state, idle*/
	kUSB_DeviceEndpointStateIdle = 0U,
	/* Endpoint state, stalled*/
	kUSB_DeviceEndpointStateStalled,
} usb_device_endpoint_status_t;

/* Endpoint initialization structure */
typedef struct _usb_device_endpoint_init_struct {
	/* Endpoint maximum packet size */
	u16_t maxPacketSize;
	/* Endpoint address*/
	u8_t endpointAddress;
	/* Endpoint transfer type*/
	u8_t transferType;
	/* ZLT flag*/
	u8_t zlt;
} usb_device_endpoint_init_struct_t;

#endif /* __USB_DC_MCUX_H__ */
