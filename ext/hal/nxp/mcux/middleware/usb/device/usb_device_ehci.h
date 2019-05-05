/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_DEVICE_EHCI_H__
#define __USB_DEVICE_EHCI_H__


/*!
 * @addtogroup usb_device_controller_ehci_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define USB_DEVICE_CONFIG_ENDPOINTS (DT_USBD_MCUX_EHCI_NUM_BIDIR_EP / 2)

/*! @brief The maximum value of ISO type maximum packet size for HS in USB specification 2.0 */
#define USB_DEVICE_MAX_HS_ISO_MAX_PACKET_SIZE (1024U)

/*! @brief The maximum value of interrupt type maximum packet size for HS in USB specification 2.0 */
#define USB_DEVICE_MAX_HS_INTERUPT_MAX_PACKET_SIZE (1024U)

/*! @brief The maximum value of bulk type maximum packet size for HS in USB specification 2.0 */
#define USB_DEVICE_MAX_HS_BULK_MAX_PACKET_SIZE (512U)

/*! @brief The maximum value of control type maximum packet size for HS in USB specification 2.0 */
#define USB_DEVICE_MAX_HS_CONTROL_MAX_PACKET_SIZE (64U)

#define USB_DEVICE_MAX_TRANSFER_PRIME_TIMES (10000000U)  /* The max prime times of EPPRIME, if still doesn't take effect, means status has been reset*/

/* Device QH */
#define USB_DEVICE_EHCI_QH_POINTER_MASK (0xFFFFFFC0U)
#define USB_DEVICE_EHCI_QH_MULT_MASK (0xC0000000U)
#define USB_DEVICE_EHCI_QH_ZLT_MASK (0x20000000U)
#define USB_DEVICE_EHCI_QH_MAX_PACKET_SIZE_MASK (0x07FF0000U)
#define USB_DEVICE_EHCI_QH_MAX_PACKET_SIZE (0x00000800U)
#define USB_DEVICE_EHCI_QH_IOS_MASK (0x00008000U)

/* Device DTD */
#define USB_DEVICE_ECHI_DTD_POINTER_MASK (0xFFFFFFE0U)
#define USB_DEVICE_ECHI_DTD_TERMINATE_MASK (0x00000001U)
#define USB_DEVICE_ECHI_DTD_PAGE_MASK (0xFFFFF000U)
#define USB_DEVICE_ECHI_DTD_PAGE_OFFSET_MASK (0x00000FFFU)
#define USB_DEVICE_ECHI_DTD_PAGE_BLOCK (0x00001000U)
#define USB_DEVICE_ECHI_DTD_TOTAL_BYTES_MASK (0x7FFF0000U)
#define USB_DEVICE_ECHI_DTD_TOTAL_BYTES (0x00004000U)
#define USB_DEVICE_ECHI_DTD_IOC_MASK (0x00008000U)
#define USB_DEVICE_ECHI_DTD_MULTIO_MASK (0x00000C00U)
#define USB_DEVICE_ECHI_DTD_STATUS_MASK (0x000000FFU)
#define USB_DEVICE_EHCI_DTD_STATUS_ERROR_MASK (0x00000068U)
#define USB_DEVICE_ECHI_DTD_STATUS_ACTIVE (0x00000080U)
#define USB_DEVICE_ECHI_DTD_STATUS_HALTED (0x00000040U)
#define USB_DEVICE_ECHI_DTD_STATUS_DATA_BUFFER_ERROR (0x00000020U)
#define USB_DEVICE_ECHI_DTD_STATUS_TRANSACTION_ERROR (0x00000008U)

typedef struct _usb_device_ehci_qh_struct
{
    union
    {
        volatile uint32_t capabilttiesCharacteristics;
        struct
        {
            volatile uint32_t reserved1 : 15;
            volatile uint32_t ios : 1;
            volatile uint32_t maxPacketSize : 11;
            volatile uint32_t reserved2 : 2;
            volatile uint32_t zlt : 1;
            volatile uint32_t mult : 2;
        } capabilttiesCharacteristicsBitmap;
    } capabilttiesCharacteristicsUnion;
    volatile uint32_t currentDtdPointer;
    volatile uint32_t nextDtdPointer;
    union
    {
        volatile uint32_t dtdToken;
        struct
        {
            volatile uint32_t status : 8;
            volatile uint32_t reserved1 : 2;
            volatile uint32_t multiplierOverride : 2;
            volatile uint32_t reserved2 : 3;
            volatile uint32_t ioc : 1;
            volatile uint32_t totalBytes : 15;
            volatile uint32_t reserved3 : 1;
        } dtdTokenBitmap;
    } dtdTokenUnion;
    volatile uint32_t bufferPointerPage[5];
    volatile uint32_t reserved1;
    uint32_t setupBuffer[2];
    uint32_t setupBufferBack[2];
    union
    {
        uint32_t endpointStatus;
        struct
        {
            uint32_t isOpened : 1;
            uint32_t zlt: 1;
            uint32_t : 30;
        } endpointStatusBitmap;
    } endpointStatusUnion;
    uint32_t reserved2;
} usb_device_ehci_qh_struct_t;

typedef struct _usb_device_ehci_dtd_struct
{
    volatile uint32_t nextDtdPointer;
    union
    {
        volatile uint32_t dtdToken;
        struct
        {
            volatile uint32_t status : 8;
            volatile uint32_t reserved1 : 2;
            volatile uint32_t multiplierOverride : 2;
            volatile uint32_t reserved2 : 3;
            volatile uint32_t ioc : 1;
            volatile uint32_t totalBytes : 15;
            volatile uint32_t reserved3 : 1;
        } dtdTokenBitmap;
    } dtdTokenUnion;
    volatile uint32_t bufferPointerPage[5];
    union
    {
        volatile uint32_t reserved;
        struct
        {
            uint32_t originalBufferOffest : 12;
            uint32_t originalBufferLength : 19;
            uint32_t dtdInvalid : 1;
        } originalBufferInfo;
    } reservedUnion;
} usb_device_ehci_dtd_struct_t;

/*! @brief EHCI state structure */
typedef struct _usb_device_ehci_state_struct
{
    usb_device_struct_t *deviceHandle; /*!< Device handle used to identify the device object is belonged to */
    USBHS_Type *registerBase;          /*!< The base address of the register */
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
    USBPHY_Type *registerPhyBase; /*!< The base address of the PHY register */
#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
    USBNC_Type *registerNcBase; /*!< The base address of the USBNC register */
#endif
#endif
    usb_device_ehci_qh_struct_t *qh;       /*!< The QH structure base address */
    usb_device_ehci_dtd_struct_t *dtd;     /*!< The DTD structure base address */
    usb_device_ehci_dtd_struct_t *dtdFree; /*!< The idle DTD list head */
    usb_device_ehci_dtd_struct_t
        *dtdHard[USB_DEVICE_CONFIG_ENDPOINTS * 2]; /*!< The transferring DTD list head for each endpoint */
    usb_device_ehci_dtd_struct_t
        *dtdTail[USB_DEVICE_CONFIG_ENDPOINTS * 2]; /*!< The transferring DTD list tail for each endpoint */
    int8_t dtdCount;                               /*!< The idle DTD node count */
    uint8_t endpointCount;                         /*!< The endpoint number of EHCI */
    uint8_t isResetting;                           /*!< Whether a PORT reset is occurring or not  */
    uint8_t controllerId;                          /*!< Controller ID */
    uint8_t speed;                                 /*!< Current speed of EHCI */
    uint8_t isSuspending;                          /*!< Is suspending of the PORT */
} usb_device_ehci_state_struct_t;

#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
typedef struct _usb_device_dcd_state_struct
{
    usb_device_struct_t *deviceHandle; /*!< Device handle used to identify the device object belongs to */
    USBHSDCD_Type *dcdRegisterBase;    /*!< The base address of the dcd module */
    uint8_t controllerId;              /*!< Controller ID */
} usb_device_dcd_state_struct_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name USB device EHCI functions
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initializes the USB device EHCI instance.
 *
 * This function initializes the USB device EHCI module specified by the controllerId.
 *
 * @param[in] controllerId The controller ID of the USB IP. See the enumeration type usb_controller_index_t.
 * @param[in] handle        Pointer of the device handle used to identify the device object is belonged to.
 * @param[out] ehciHandle   An out parameter used to return the pointer of the device EHCI handle to the caller.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciInit(uint8_t controllerId,
                                usb_device_handle handle,
                                usb_device_controller_handle *ehciHandle);

/*!
 * @brief Deinitializes the USB device EHCI instance.
 *
 * This function deinitializes the USB device EHCI module.
 *
 * @param[in] ehciHandle   Pointer of the device EHCI handle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciDeinit(usb_device_controller_handle ehciHandle);

/*!
 * @brief Sends data through a specified endpoint.
 *
 * This function sends data through a specified endpoint.
 *
 * @param[in] ehciHandle      Pointer of the device EHCI handle.
 * @param[in] endpointAddress Endpoint index.
 * @param[in] buffer           The memory address to hold the data need to be sent.
 * @param[in] length           The data length to be sent.
 *
 * @return A USB error code or kStatus_USB_Success.
 *
 * @note The return value means whether the sending request is successful or not. The transfer completion is indicated
 * by the
 * corresponding callback function.
 * Currently, only one transfer request can be supported for a specific endpoint.
 * If there is a specific requirement to support multiple transfer requests for a specific endpoint, the application
 * should implement a queue in the application level.
 * The subsequent transfer can begin only when the previous transfer is done (a notification is received through the
 * endpoint
 * callback).
 */
usb_status_t USB_DeviceEhciSend(usb_device_controller_handle ehciHandle,
                                uint8_t endpointAddress,
                                uint8_t *buffer,
                                uint32_t length);

/*!
 * @brief Receive data through a specified endpoint.
 *
 * This function Receives data through a specified endpoint.
 *
 * @param[in] ehciHandle      Pointer of the device EHCI handle.
 * @param[in] endpointAddress Endpoint index.
 * @param[in] buffer           The memory address to save the received data.
 * @param[in] length           The data length want to be received.
 *
 * @return A USB error code or kStatus_USB_Success.
 *
 * @note The return value just means if the receiving request is successful or not; the transfer done is notified by the
 * corresponding callback function.
 * Currently, only one transfer request can be supported for one specific endpoint.
 * If there is a specific requirement to support multiple transfer requests for one specific endpoint, the application
 * should implement a queue in the application level.
 * The subsequent transfer could begin only when the previous transfer is done (get notification through the endpoint
 * callback).
 */
usb_status_t USB_DeviceEhciRecv(usb_device_controller_handle ehciHandle,
                                uint8_t endpointAddress,
                                uint8_t *buffer,
                                uint32_t length);

/*!
 * @brief Cancels the pending transfer in a specified endpoint.
 *
 * The function is used to cancel the pending transfer in a specified endpoint.
 *
 * @param[in] ehciHandle      Pointer of the device EHCI handle.
 * @param[in] ep               Endpoint address, bit7 is the direction of endpoint, 1U - IN, 0U - OUT.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciCancel(usb_device_controller_handle ehciHandle, uint8_t ep);

/*!
 * @brief Controls the status of the selected item.
 *
 * The function is used to control the status of the selected item.
 *
 * @param[in] ehciHandle      Pointer of the device EHCI handle.
 * @param[in] type             The selected item. See enumeration type usb_device_control_type_t.
 * @param[in,out] param            The parameter type is determined by the selected item.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciControl(usb_device_controller_handle ehciHandle,
                                   usb_device_control_type_t type,
                                   void *param);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __USB_DEVICE_EHCI_H__ */
