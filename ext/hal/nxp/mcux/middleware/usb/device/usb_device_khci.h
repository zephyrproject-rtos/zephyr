/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_DEVICE_KHCI_H__
#define __USB_DEVICE_KHCI_H__

/*!
 * @addtogroup usb_device_controller_khci_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define USB_DATA_ALIGN_SIZE 4

#define USB_DEVICE_CONFIG_KHCI_DMA_ALIGN_BUFFER_LENGTH (64U)

#define USB_DEVICE_CONFIG_ENDPOINTS (DT_USBD_MCUX_KHCI_NUM_BIDIR_EP / 2)

/*! @brief The maximum value of ISO maximum packet size for FS in USB specification 2.0 */
#define USB_DEVICE_MAX_FS_ISO_MAX_PACKET_SIZE (1023U)

/*! @brief The maximum value of non-ISO maximum packet size for FS in USB specification 2.0 */
#define USB_DEVICE_MAX_FS_NONE_ISO_MAX_PACKET_SIZE (64U)

#define USB_KHCI_BDT_DEVICE_OUT_TOKEN (0x01U)
#define USB_KHCI_BDT_DEVICE_IN_TOKEN (0x09U)
#define USB_KHCI_BDT_DEVICE_SETUP_TOKEN (0x0DU)

#define USB_KHCI_BDT_OWN (0x80U)
#define USB_KHCI_BDT_DATA01(x) ((((uint32_t)(x)) & 0x01U) << 0x06U)
#define USB_KHCI_BDT_BC(x) ((((uint32_t)(x)) & 0x3FFU) << 0x10U)
#define UBS_KHCI_BDT_KEEP (0x20U)
#define UBS_KHCI_BDT_NINC (0x10U)
#define USB_KHCI_BDT_DTS (0x08U)
#define USB_KHCI_BDT_STALL (0x04U)

typedef enum _usb_khci_interrupt_type
{
    kUSB_KhciInterruptReset = 0x01U,
    kUSB_KhciInterruptError = 0x02U,
    kUSB_KhciInterruptSofToken = 0x04U,
    kUSB_KhciInterruptTokenDone = 0x08U,
    kUSB_KhciInterruptSleep = 0x10U,
    kUSB_KhciInterruptResume = 0x20U,
    kUSB_KhciInterruptAttach = 0x40U,
    kUSB_KhciInterruptStall = 0x80U,
} usb_khci_interrupt_type_t;

/*! @brief Set BDT buffer address */
#define USB_KHCI_BDT_SET_ADDRESS(bdt_base, ep, direction, odd, address)                          \
    *((volatile uint32_t *)((bdt_base & 0xfffffe00U) | (((uint32_t)ep & 0x0fU) << 5U) |          \
                            (((uint32_t)direction & 1U) << 4U) | (((uint32_t)odd & 1U) << 3U)) + \
      1U) = address

/*! @brief Set BDT control fields*/
#define USB_KHCI_BDT_SET_CONTROL(bdt_base, ep, direction, odd, control)                \
    *(volatile uint32_t *)((bdt_base & 0xfffffe00U) | (((uint32_t)ep & 0x0fU) << 5U) | \
                           (((uint32_t)direction & 1U) << 4U) | (((uint32_t)odd & 1U) << 3U)) = control

/*! @brief Get BDT buffer address*/
#define USB_KHCI_BDT_GET_ADDRESS(bdt_base, ep, direction, odd)                                    \
    (*((volatile uint32_t *)((bdt_base & 0xfffffe00U) | (((uint32_t)ep & 0x0fU) << 5U) |          \
                             (((uint32_t)direction & 1U) << 4U) | (((uint32_t)odd & 1U) << 3U)) + \
       1U))

/*! @brief Get BDT control fields*/
#define USB_KHCI_BDT_GET_CONTROL(bdt_base, ep, direction, odd)                          \
    (*(volatile uint32_t *)((bdt_base & 0xfffffe00U) | (((uint32_t)ep & 0x0fU) << 5U) | \
                            (((uint32_t)direction & 1U) << 4U) | (((uint32_t)odd & 1U) << 3U)))

/*! @brief Endpoint state structure */
typedef struct _usb_device_khci_endpoint_state_struct
{
    uint8_t *transferBuffer; /*!< Address of buffer containing the data to be transmitted */
    uint32_t transferLength; /*!< Length of data to transmit. */
    uint32_t transferDone;   /*!< The data length has been transferred*/
    union
    {
        uint32_t state; /*!< The state of the endpoint */
        struct
        {
            uint32_t maxPacketSize : 10U; /*!< The maximum packet size of the endpoint */
            uint32_t stalled : 1U;        /*!< The endpoint is stalled or not */
            uint32_t data0 : 1U;          /*!< The data toggle of the transaction */
            uint32_t bdtOdd : 1U;         /*!< The BDT toggle of the endpoint */
            uint32_t dmaAlign : 1U;       /*!< Whether the transferBuffer is DMA aligned or not */
            uint32_t transferring : 1U;   /*!< The endpoint is transferring */
            uint32_t zlt : 1U;            /*!< zlt flag */
        } stateBitField;
    } stateUnion;
} usb_device_khci_endpoint_state_struct_t;

/*! @brief KHCI state structure */
typedef struct _usb_device_khci_state_struct
{
    usb_device_struct_t *deviceHandle; /*!< Device handle used to identify the device object belongs to */
    uint8_t *bdt;                      /*!< BDT buffer address */
    USB_Type *registerBase;            /*!< The base address of the register */
    uint8_t setupPacketBuffer[USB_SETUP_PACKET_SIZE * 2]; /*!< The setup request buffer */
    uint8_t *dmaAlignBuffer; /*!< This buffer is used to fix the transferBuffer or transferLength does
                               not align to 4-bytes when the function USB_DeviceKhciRecv is called.
                               The macro USB_DEVICE_CONFIG_KHCI_DMA_ALIGN is used to enable or disable this feature.
                               If the feature is enabled, when the transferBuffer or transferLength does not align to
                               4-bytes,
                               the transferLength is not more than USB_DEVICE_CONFIG_KHCI_DMA_ALIGN_BUFFER_LENGTH, and
                               the flag isDmaAlignBufferInusing is zero, the dmaAlignBuffer is used to receive data
                               and the flag isDmaAlignBufferInusing is set to 1.
                               When the transfer is done, the received data, kept in dmaAlignBuffer, is copied
                               to the transferBuffer, and the flag isDmaAlignBufferInusing is cleared.
                                */
    usb_device_khci_endpoint_state_struct_t
        endpointState[USB_DEVICE_CONFIG_ENDPOINTS * 2]; /*!< Endpoint state structures */
    uint8_t isDmaAlignBufferInusing;                    /*!< The dmaAlignBuffer is used or not */
    uint8_t isResetting;                                /*!< Is doing device reset or not */
    uint8_t controllerId;                               /*!< Controller ID */
    uint8_t setupBufferIndex;                           /*!< A valid setup buffer flag */
#if (defined(USB_DEVICE_CONFIG_OTG) && (USB_DEVICE_CONFIG_OTG))
    uint8_t otgStatus;
#endif
} usb_device_khci_state_struct_t;

#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBDCD_COUNT) && (FSL_FEATURE_SOC_USBDCD_COUNT > 0U))
typedef struct _usb_device_dcd_state_struct
{
    usb_device_struct_t *deviceHandle; /*!< Device handle used to identify the device object belongs to */
    USBDCD_Type *dcdRegisterBase;      /*!< The base address of the dcd module */
    uint8_t controllerId;              /*!< Controller ID */
} usb_device_dcd_state_struct_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name USB device KHCI functions
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initializes the USB device KHCI instance.
 *
 * This function initializes the USB device KHCI module specified by the controllerId.
 *
 * @param[in] controllerId The controller ID of the USB IP. See the enumeration type usb_controller_index_t.
 * @param[in] handle        Pointer of the device handle used to identify the device object belongs to.
 * @param[out] khciHandle   An out parameter used to return the pointer of the device KHCI handle to the caller.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceKhciInit(uint8_t controllerId,
                                usb_device_handle handle,
                                usb_device_controller_handle *khciHandle);

/*!
 * @brief Deinitializes the USB device KHCI instance.
 *
 * This function deinitializes the USB device KHCI module.
 *
 * @param[in] khciHandle   Pointer of the device KHCI handle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceKhciDeinit(usb_device_controller_handle khciHandle);

/*!
 * @brief Sends data through a specified endpoint.
 *
 * This function sends data through a specified endpoint.
 *
 * @param[in] khciHandle      Pointer of the device KHCI handle.
 * @param[in] endpointAddress Endpoint index.
 * @param[in] buffer           The memory address to hold the data need to be sent.
 * @param[in] length           The data length need to be sent.
 *
 * @return A USB error code or kStatus_USB_Success.
 *
 * @note The return value indicates whether the sending request is successful or not. The transfer completion is
 * notified by the
 * corresponding callback function.
 * Currently, only one transfer request can be supported for a specific endpoint.
 * If there is a specific requirement to support multiple transfer requests for a specific endpoint, the application
 * should implement a queue in the application level.
 * The subsequent transfer can begin only when the previous transfer is done (a notification is obtained through the
 * endpoint
 * callback).
 */
usb_status_t USB_DeviceKhciSend(usb_device_controller_handle khciHandle,
                                uint8_t endpointAddress,
                                uint8_t *buffer,
                                uint32_t length);

/*!
 * @brief Receives data through a specified endpoint.
 *
 * This function receives data through a specified endpoint.
 *
 * @param[in] khciHandle      Pointer of the device KHCI handle.
 * @param[in] endpointAddress Endpoint index.
 * @param[in] buffer           The memory address to save the received data.
 * @param[in] length           The data length to be received.
 *
 * @return A USB error code or kStatus_USB_Success.
 *
 * @note The return value indicates whether the receiving request is successful or not. The transfer completion is
 * notified by the
 * corresponding callback function.
 * Currently, only one transfer request can be supported for a specific endpoint.
 * If there is a specific requirement to support multiple transfer requests for a specific endpoint, the application
 * should implement a queue in the application level.
 * The subsequent transfer can begin only when the previous transfer is done (a notification is obtained through the
 * endpoint
 * callback).
 */
usb_status_t USB_DeviceKhciRecv(usb_device_controller_handle khciHandle,
                                uint8_t endpointAddress,
                                uint8_t *buffer,
                                uint32_t length);

/*!
 * @brief Cancels the pending transfer in a specified endpoint.
 *
 * The function is used to cancel the pending transfer in a specified endpoint.
 *
 * @param[in] khciHandle      Pointer of the device KHCI handle.
 * @param[in] ep               Endpoint address, bit7 is the direction of endpoint, 1U - IN, abd 0U - OUT.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceKhciCancel(usb_device_controller_handle khciHandle, uint8_t ep);

/*!
 * @brief Controls the status of the selected item.
 *
 * The function is used to control the status of the selected item.
 *
 * @param[in] khciHandle      Pointer of the device KHCI handle.
 * @param[in] type             The selected item. See enumeration type usb_device_control_type_t.
 * @param[in,out] param            The parameter type is determined by the selected item.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceKhciControl(usb_device_controller_handle khciHandle,
                                   usb_device_control_type_t type,
                                   void *param);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __USB_DEVICE_KHCI_H__ */
