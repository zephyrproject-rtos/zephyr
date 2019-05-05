/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_dc_mcux.h"

#if ((defined(USB_DEVICE_CONFIG_EHCI)) && (USB_DEVICE_CONFIG_EHCI > 0U))

#include "usb_device_ehci.h"
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
#include "usb_phy.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#if defined(USB_STACK_USE_DEDICATED_RAM) && (USB_STACK_USE_DEDICATED_RAM > 0U)

#error The SOC does not suppoort dedicated RAM case.

#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void USB_DeviceEhciSetDefaultState(usb_device_ehci_state_struct_t *ehciState);
static usb_status_t USB_DeviceEhciEndpointInit(usb_device_ehci_state_struct_t *ehciState,
                                               usb_device_endpoint_init_struct_t *epInit);
static usb_status_t USB_DeviceEhciEndpointDeinit(usb_device_ehci_state_struct_t *ehciState, uint8_t ep);
static usb_status_t USB_DeviceEhciEndpointStall(usb_device_ehci_state_struct_t *ehciState, uint8_t ep);
static usb_status_t USB_DeviceEhciEndpointUnstall(usb_device_ehci_state_struct_t *ehciState, uint8_t ep);
static void USB_DeviceEhciFillSetupBuffer(usb_device_ehci_state_struct_t *ehciState, uint8_t ep);
static void USB_DeviceEhciCancelControlPipe(usb_device_ehci_state_struct_t *ehciState,
                                            uint8_t endpoint,
                                            uint8_t direction);
static void USB_DeviceEhciInterruptTokenDone(usb_device_ehci_state_struct_t *ehciState);
static void USB_DeviceEhciInterruptPortChange(usb_device_ehci_state_struct_t *ehciState);
static void USB_DeviceEhciInterruptReset(usb_device_ehci_state_struct_t *ehciState);
static void USB_DeviceEhciInterruptSof(usb_device_ehci_state_struct_t *ehciState);
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
static void USB_DeviceEhciInterruptSuspend(usb_device_ehci_state_struct_t *ehciState);
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */
static usb_status_t USB_DeviceEhciTransfer(usb_device_ehci_state_struct_t *ehciState,
                                           uint8_t endpointAddress,
                                           uint8_t *buffer,
                                           uint32_t length);

extern usb_status_t USB_DeviceNotificationTrigger(void *handle, void *msg);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Apply for QH buffer, 2048-byte alignment */
USB_RAM_ADDRESS_ALIGNMENT(2048)
USB_CONTROLLER_DATA static uint8_t qh_buffer[(USB_DEVICE_CONFIG_EHCI - 1) * 2048 +
                                             2 * USB_DEVICE_CONFIG_ENDPOINTS * 2 * sizeof(usb_device_ehci_qh_struct_t)];

/* Apply for DTD buffer, 32-byte alignment */
USB_RAM_ADDRESS_ALIGNMENT(32)
USB_CONTROLLER_DATA static usb_device_ehci_dtd_struct_t
    s_UsbDeviceEhciDtd[USB_DEVICE_CONFIG_EHCI][USB_DEVICE_CONFIG_EHCI_MAX_DTD];

/* Apply for ehci device state structure */
static usb_device_ehci_state_struct_t g_UsbDeviceEhciState[USB_DEVICE_CONFIG_EHCI];

/* Apply for whether the corresponding g_UsbDeviceEhciState is used or not, if used, it is set to 1, if not used, it is set to 0 */
static uint8_t g_UsbDeviceEhciStateStatus[USB_DEVICE_CONFIG_EHCI]= {0}; 

#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
/* Apply for device dcd state structure */
static usb_device_dcd_state_struct_t s_UsbDeviceDcdHSState[USB_DEVICE_CONFIG_EHCI];
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief EHCI NC get USB NC bass address.
 *
 * This function is used to get USB NC bass address.
 *
 * @param[in] controllerId    EHCI controller ID; See the #usb_controller_index_t.
 *
 * @retval USB NC bass address.
 */
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
void *USB_EhciNCGetBase(uint8_t controllerId)
{
    void *usbNCBase = NULL;
#if ((defined FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
    uint32_t instance;
    uint32_t newinstance = 0;
    uint32_t usbnc_base_temp[] = USBNC_BASE_ADDRS;
    uint32_t usbnc_base[] = USBNC_BASE_ADDRS;

    if (controllerId < kUSB_ControllerEhci0)
    {
        return NULL;
    }

    controllerId = controllerId - kUSB_ControllerEhci0;

    for (instance = 0; instance < (sizeof(usbnc_base_temp) / sizeof(usbnc_base_temp[0])); instance++)
    {
        if (usbnc_base_temp[instance])
        {
            usbnc_base[newinstance++] = usbnc_base_temp[instance];
        }
    }
    if (controllerId > newinstance)
    {
        return NULL;
    }

    usbNCBase = (void *)usbnc_base[controllerId];
#endif
    return usbNCBase;
}
#endif
#endif

/*!
 * @brief Set device controller state to default state.
 *
 * The function is used to set device controller state to default state.
 * The function will be called when USB_DeviceEhciInit called or the control type kUSB_DeviceControlGetEndpointStatus
 * received in USB_DeviceEhciControl.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciSetDefaultState(usb_device_ehci_state_struct_t *ehciState)
{
    usb_device_ehci_dtd_struct_t *p;

    /* Initialize the dtd free queue */
    ehciState->dtdFree = ehciState->dtd;
    p = ehciState->dtdFree;
    for (uint32_t i = 1U; i < USB_DEVICE_CONFIG_EHCI_MAX_DTD; i++)
    {
        p->nextDtdPointer = (uint32_t)&ehciState->dtd[i];
        p = (usb_device_ehci_dtd_struct_t *)p->nextDtdPointer;
    }
    p->nextDtdPointer = 0U;
    ehciState->dtdCount = USB_DEVICE_CONFIG_EHCI_MAX_DTD;

    /* Not use interrupt threshold. */
    ehciState->registerBase->USBCMD &= ~USBHS_USBCMD_ITC_MASK;
    ehciState->registerBase->USBCMD |= USBHS_USBCMD_ITC(0U);

    /* Disable setup lockout, please refer to "Control Endpoint Operation" section in RM. */
    ehciState->registerBase->USBMODE |= USBHS_USBMODE_SLOM_MASK;

/* Set the endian by using CPU's endian */
#if defined(CONFIG_BIG_ENDIAN)
    ehciState->registerBase->USBMODE |= USBHS_USBMODE_ES_MASK;
#else
    ehciState->registerBase->USBMODE &= ~USBHS_USBMODE_ES_MASK;
#endif
    /* Initialize the QHs of endpoint. */
    for (uint32_t i = 0U; i < (USB_DEVICE_CONFIG_ENDPOINTS * 2U); i++)
    {
        ehciState->qh[i].nextDtdPointer = USB_DEVICE_ECHI_DTD_TERMINATE_MASK;
        ehciState->qh[i].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.maxPacketSize =
            USB_CONTROL_MAX_PACKET_SIZE;
        ehciState->dtdHard[i] = NULL;
        ehciState->dtdTail[i] = NULL;
        ehciState->qh[i].endpointStatusUnion.endpointStatusBitmap.isOpened = 0U;
    }

    /* Add QH buffer address to USBHS_EPLISTADDR_REG */
    ehciState->registerBase->EPLISTADDR = (uint32_t)ehciState->qh;

    /* Clear device address */
    ehciState->registerBase->DEVICEADDR = 0U;

#if defined(USB_DEVICE_CONFIG_DETACH_ENABLE) && (USB_DEVICE_CONFIG_DETACH_ENABLE > 0U)
    ehciState->registerBase->OTGSC = ehciState->registerBase->OTGSC & 0x0000FFFF;
    ehciState->registerBase->OTGSC |= USBHS_OTGSC_BSVIE_MASK;
#endif /* USB_DEVICE_CONFIG_DETACH_ENABLE */

    /* Enable USB Interrupt, USB Error Interrupt, Port Change detect Interrupt, USB-Reset Interrupt*/
    ehciState->registerBase->USBINTR =
        (USBHS_USBINTR_UE_MASK | USBHS_USBINTR_UEE_MASK | USBHS_USBINTR_PCE_MASK | USBHS_USBINTR_URE_MASK
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
         | USBHS_USBINTR_SLE_MASK
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */
         );

    /* Clear reset flag */
    ehciState->isResetting = 0U;
}

/*!
 * @brief Initialize a specified endpoint.
 *
 * The function is used to initialize a specified endpoint.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param epInit          The endpoint initialization structure pointer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceEhciEndpointInit(usb_device_ehci_state_struct_t *ehciState,
                                               usb_device_endpoint_init_struct_t *epInit)
{
    uint32_t primeBit = 1U << ((epInit->endpointAddress & USB_ENDPOINT_NUMBER_MASK) +
                               ((epInit->endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> 0x03U));
    uint16_t maxPacketSize = epInit->maxPacketSize & USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_SIZE_MASK;
    uint8_t endpoint = (epInit->endpointAddress & USB_ENDPOINT_NUMBER_MASK);
    uint8_t direction = (epInit->endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                        USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT;
    uint8_t index = ((uint8_t)((uint32_t)endpoint << 1U)) | direction;
    uint8_t transferType = epInit->transferType & USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TYPE_MASK;

    /* Cancel pending transfer of the endpoint */
    USB_DeviceEhciCancel(ehciState, epInit->endpointAddress);

    if ((ehciState->registerBase->EPPRIME & primeBit) || (ehciState->registerBase->EPSR & primeBit))
    {
        return kStatus_USB_Busy;
    }

    /* Make the endpoint max packet size align with USB Specification 2.0. */
    if (USB_ENDPOINT_ISOCHRONOUS == transferType)
    {
        if (maxPacketSize > USB_DEVICE_MAX_HS_ISO_MAX_PACKET_SIZE)
        {
            maxPacketSize = USB_DEVICE_MAX_HS_ISO_MAX_PACKET_SIZE;
        }
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.mult =
            1U + ((epInit->maxPacketSize & USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_MULT_TRANSACTIONS_MASK) >>
                  USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_MULT_TRANSACTIONS_SHFIT);
    }
    else
    {
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.mult = 0U;
    }

    /* Save the max packet size of the endpoint */
    ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.maxPacketSize =
        maxPacketSize;
    ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.zlt = epInit->zlt;
    if ((USB_CONTROL_ENDPOINT == endpoint))
    {
        /* Set ZLT bit. disable control endpoint automatic zlt by default,only send zlt when it is needed*/
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.zlt = 1U;
    }
    else
    {
        /* Set ZLT bit. */
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.zlt = !epInit->zlt;
    }
    
    /* Enable the endpoint. */
    if (USB_ENDPOINT_CONTROL == transferType)
    {
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.ios = 1U;
        ehciState->registerBase->EPCR0 |=
            (direction ?
                 (USBHS_EPCR_TXE_MASK | USBHS_EPCR_TXR_MASK | ((uint32_t)transferType << USBHS_EPCR_TXT_SHIFT)) :
                 (USBHS_EPCR_RXE_MASK | USBHS_EPCR_RXR_MASK | ((uint32_t)transferType << USBHS_EPCR_RXT_SHIFT)));
    }
    else
    {
        ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.ios = 0U;
        ehciState->registerBase->EPCR[endpoint - 1U] |=
            (direction ?
                 (USBHS_EPCR_TXE_MASK | USBHS_EPCR_TXR_MASK | ((uint32_t)transferType << USBHS_EPCR_TXT_SHIFT)) :
                 (USBHS_EPCR_RXE_MASK | USBHS_EPCR_RXR_MASK | ((uint32_t)transferType << USBHS_EPCR_RXT_SHIFT)));
    }

    ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.isOpened = 1U;
    return kStatus_USB_Success;
}

/*!
 * @brief De-initialize a specified endpoint.
 *
 * The function is used to de-initialize a specified endpoint.
 * Current transfer of the endpoint will be cancelled and the specified endpoint will be disabled.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param ep               The endpoint address, Bit7, 0U - USB_OUT, 1U - USB_IN.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceEhciEndpointDeinit(usb_device_ehci_state_struct_t *ehciState, uint8_t ep)
{
    uint32_t primeBit =
        1U << ((ep & USB_ENDPOINT_NUMBER_MASK) + ((ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> 0x03U));
    uint8_t endpoint = (ep & USB_ENDPOINT_NUMBER_MASK);
    uint8_t direction =
        (ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT;
    uint8_t index = ((uint8_t)((uint32_t)endpoint << 1U)) | direction;

    ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.isOpened = 0U;

    /* Cancel the transfer of the endpoint */
    USB_DeviceEhciCancel(ehciState, ep);

    if ((ehciState->registerBase->EPPRIME & primeBit) || (ehciState->registerBase->EPSR & primeBit))
    {
        return kStatus_USB_Busy;
    }

    /* Clear endpoint state */
    ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristics = 0U;
    /* Disable the endpoint */
    if (!endpoint)
    {
        ehciState->registerBase->EPCR0 &=
            ~(direction ? (USBHS_EPCR_TXE_MASK | USBHS_EPCR_TXT_MASK) : (USBHS_EPCR_RXE_MASK | USBHS_EPCR_RXT_MASK));
    }
    else
    {
        ehciState->registerBase->EPCR[endpoint - 1U] &=
            ~(direction ? (USBHS_EPCR_TXE_MASK | USBHS_EPCR_TXT_MASK) : (USBHS_EPCR_RXE_MASK | USBHS_EPCR_RXT_MASK));
    }

    return kStatus_USB_Success;
}

/*!
 * @brief Stall a specified endpoint.
 *
 * The function is used to stall a specified endpoint.
 * Current transfer of the endpoint will be cancelled and the specified endpoint will be stalled.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param ep               The endpoint address, Bit7, 0U - USB_OUT, 1U - USB_IN.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceEhciEndpointStall(usb_device_ehci_state_struct_t *ehciState, uint8_t ep)
{
    uint8_t endpoint = ep & USB_ENDPOINT_NUMBER_MASK;
    uint8_t direction =
        (ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT;
    uint8_t index = ((uint8_t)((uint32_t)endpoint << 1U)) | direction;

    /* Cancel the transfer of the endpoint */
    USB_DeviceEhciCancel(ehciState, ep);

    /* Set endpoint stall flag. */
    if (ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.ios)
    {
        if (!endpoint)
        {
            ehciState->registerBase->EPCR0 |= (USBHS_EPCR_TXS_MASK | USBHS_EPCR_RXS_MASK);
        }
        else
        {
            ehciState->registerBase->EPCR[endpoint - 1U] |= (USBHS_EPCR_TXS_MASK | USBHS_EPCR_RXS_MASK);
        }
    }
    else
    {
        if (!endpoint)
        {
            ehciState->registerBase->EPCR0 |= (direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK);
        }
        else
        {
            ehciState->registerBase->EPCR[endpoint - 1U] |= (direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK);
        }
    }

    return kStatus_USB_Success;
}

/*!
 * @brief Un-stall a specified endpoint.
 *
 * The function is used to un-stall a specified endpoint.
 * Current transfer of the endpoint will be cancelled and the specified endpoint will be un-stalled.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param ep               The endpoint address, Bit7, 0U - USB_OUT, 1U - USB_IN.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceEhciEndpointUnstall(usb_device_ehci_state_struct_t *ehciState, uint8_t ep)
{
    uint8_t endpoint = ep & USB_ENDPOINT_NUMBER_MASK;
    uint8_t direction =
        (ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT;

    /* Clear the endpoint stall state */
    if (!endpoint)
    {
        ehciState->registerBase->EPCR0 &= ~(direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK);
    }
    else
    {
        ehciState->registerBase->EPCR[endpoint - 1U] &= ~(direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK);
        ehciState->registerBase->EPCR[endpoint - 1U] |= (direction ? USBHS_EPCR_TXR_MASK : USBHS_EPCR_RXR_MASK);
    }
    /* Cancel the transfer of the endpoint */
    USB_DeviceEhciCancel(ehciState, ep);
    
    return kStatus_USB_Success;
}

/*!
 * @brief Get setup packet data.
 *
 * The function is used to get setup packet data and copy to a backup buffer.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param ep               The endpoint number.
 *
 */
static void USB_DeviceEhciFillSetupBuffer(usb_device_ehci_state_struct_t *ehciState, uint8_t ep)
{
    uint8_t waitingSafelyAccess = 1U;
    uint8_t index = (ep * 2U) | USB_OUT;

    /* Write 1U to clear corresponding bit in EPSETUPSR. */
    ehciState->registerBase->EPSETUPSR = 1U << ep;

    while (waitingSafelyAccess)
    {
        /* Set the setup tripwire bit. */
        ehciState->registerBase->USBCMD |= USBHS_USBCMD_SUTW_MASK;

        /* Copy setup packet data to backup buffer */
        ehciState->qh[index].setupBufferBack[0] = ehciState->qh[index].setupBuffer[0];
        ehciState->qh[index].setupBufferBack[1] = ehciState->qh[index].setupBuffer[1];

        /* Read the USBCMD[SUTW] bit. If set, jump out from the while loop; if cleared continue */
        if (ehciState->registerBase->USBCMD & USBHS_USBCMD_SUTW_MASK)
        {
            waitingSafelyAccess = 0U;
        }
    }
    /* Clear the setup tripwire bit */
    ehciState->registerBase->USBCMD &= ~USBHS_USBCMD_SUTW_MASK;
}

/*!
 * @brief Cancel the transfer of the control pipe.
 *
 * The function is used to cancel the transfer of the control pipe.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param endpoint         The endpoint number.
 * @param direction        The direction of the endpoint.
 *
 */
static void USB_DeviceEhciCancelControlPipe(usb_device_ehci_state_struct_t *ehciState,
                                            uint8_t endpoint,
                                            uint8_t direction)
{
    usb_device_ehci_dtd_struct_t *currentDtd;
    uint32_t index = ((uint32_t)endpoint << 1U) + (uint32_t)direction;
    usb_device_callback_message_struct_t message;

    message.buffer = NULL;
    message.length = 0U;
    /* Get the dtd of the control pipe */
    currentDtd =
        (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK);
    while (currentDtd)
    {
        /* Pass the transfer buffer address */
        if (NULL == message.buffer)
        {
            uint32_t bufferAddress = currentDtd->bufferPointerPage[0];
            message.buffer = (uint8_t *)((bufferAddress & USB_DEVICE_ECHI_DTD_PAGE_MASK) |
                                         (currentDtd->reservedUnion.originalBufferInfo.originalBufferOffest));
        }
        /* If the dtd is active, set the message length to USB_UNINITIALIZED_VAL_32. Or set the length by using finished
         * length. */
        if (currentDtd->dtdTokenUnion.dtdTokenBitmap.status & USB_DEVICE_ECHI_DTD_STATUS_ACTIVE)
        {
            message.length = USB_UNINITIALIZED_VAL_32;
        }
        else
        {
            message.length += (currentDtd->reservedUnion.originalBufferInfo.originalBufferLength -
                               currentDtd->dtdTokenUnion.dtdTokenBitmap.totalBytes);
        }

        /* Move the dtd head pointer to next. */
        /* If the pointer of the head equals to the tail, set the dtd queue to null. */
        if (ehciState->dtdHard[index] == ehciState->dtdTail[index])
        {
            ehciState->dtdHard[index] = NULL;
            ehciState->dtdTail[index] = NULL;
            ehciState->qh[index].nextDtdPointer = USB_DEVICE_ECHI_DTD_TERMINATE_MASK;
            ehciState->qh[index].dtdTokenUnion.dtdToken = 0U;
        }
        else
        {
            ehciState->dtdHard[index] = (usb_device_ehci_dtd_struct_t *)ehciState->dtdHard[index]->nextDtdPointer;
        }

        /* When the ioc is set or the dtd queue is empty, the up layer will be notified. */
        if ((currentDtd->dtdTokenUnion.dtdTokenBitmap.ioc) ||
            (0 == ((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK)))
        {
            message.code = endpoint | (uint8_t)((uint32_t)direction << 0x07U);
            message.isSetup = 0U;
            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
            message.buffer = NULL;
            message.length = 0U;
        }

        /* Clear the token field of the dtd. */
        currentDtd->dtdTokenUnion.dtdToken = 0U;
        /* Add the dtd to the free dtd queue. */
        currentDtd->nextDtdPointer = (uint32_t)ehciState->dtdFree;
        ehciState->dtdFree = currentDtd;
        ehciState->dtdCount++;

        /* Get the next in-used dtd. */
        currentDtd =
            (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK);
    }
}

/*!
 * @brief Handle the endpoint token done interrupt.
 *
 * The function is used to handle the endpoint token done interrupt.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciInterruptTokenDone(usb_device_ehci_state_struct_t *ehciState)
{
    uint32_t status;
    uint32_t primeBit;
    usb_device_ehci_dtd_struct_t *currentDtd;
    usb_device_callback_message_struct_t message;
    uint8_t endpoint;
    uint8_t direction;
    uint8_t count;
    uint8_t index;

    /* Get the EPSETUPSR to check the setup packect received in which one endpoint. */
    status = ehciState->registerBase->EPSETUPSR;

    if (status)
    {
        for (endpoint = 0U; endpoint < USB_DEVICE_CONFIG_ENDPOINTS; endpoint++)
        {
            /* Check the endpoint receive the setup packet. */
            if (status & (1U << endpoint))
            {
                /* Get last setup packet */
                usb_setup_struct_t *deviceSetup =
                    (usb_setup_struct_t *)&ehciState->qh[(uint8_t)((uint32_t)endpoint << 1U) + USB_OUT].setupBufferBack;

                /* Check the direction of the data phase. */
                direction = (deviceSetup->bmRequestType & USB_REQUEST_TYPE_DIR_IN) >> USB_REQUEST_TYPE_DIR_SHIFT;
                /* Cancel the data phase transfer */
                USB_DeviceEhciCancelControlPipe(ehciState, endpoint, direction);
                /* Cancel the status phase transfer */
                USB_DeviceEhciCancelControlPipe(ehciState, endpoint, 1U ^ direction);
                message.code = (endpoint) | (USB_OUT << 0x07U);
                message.buffer = (uint8_t *)deviceSetup;
                message.length = USB_SETUP_PACKET_SIZE;
                message.isSetup = 1U;
                /* Fill the setup packet to the backup buffer */
                USB_DeviceEhciFillSetupBuffer(ehciState, endpoint);
                /* Notify the up layer the EHCI status changed. */
                USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
            }
        }
    }
    /* Read the USBHS_EPCOMPLETE_REG to get the endpoint transfer done status */
    status = ehciState->registerBase->EPCOMPLETE;
    /* Clear the endpoint transfer done status */
    ehciState->registerBase->EPCOMPLETE = status;

    if (status)
    {
        for (count = 0U; count < 32U; count++)
        {
            /* Check the transfer is done or not in the specified endpoint. */
            if (status & ((uint32_t)(1U << count)))
            {
                if (count > 15U)
                {
                    endpoint = count - 16U;
                    direction = USB_IN;
                }
                else
                {
                    endpoint = count;
                    direction = USB_OUT;
                }
                if (endpoint >= USB_DEVICE_CONFIG_ENDPOINTS)
                {
                    continue;
                }
                index = (endpoint << 1U) + direction;
                message.buffer = NULL;
                message.length = 0U;
                if ((USB_CONTROL_ENDPOINT == endpoint) && (USB_IN == direction))
                {
                    if (1U == ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.zlt)
                    {
                        if(!ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.zlt)
                        {
                            /*disable zlt after send zlt*/
                            ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.zlt = 1U; 
                        }
                    }
                }
                /* Get the in-used dtd of the specified endpoint. */
                currentDtd = (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] &
                                                              USB_DEVICE_ECHI_DTD_POINTER_MASK);
                while (currentDtd)
                {
                    uint8_t isTokenDone = 0;
                    /* Get the in-used dtd of the specified endpoint. */
                    currentDtd = (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] &
                                                                  USB_DEVICE_ECHI_DTD_POINTER_MASK);

                    while (currentDtd)
                    {
                        /* Don't handle the active dtd. */
                        if ((currentDtd->dtdTokenUnion.dtdTokenBitmap.status & USB_DEVICE_ECHI_DTD_STATUS_ACTIVE) ||
                            (currentDtd->dtdTokenUnion.dtdTokenBitmap.ioc))
                        {
                            if ((!(currentDtd->dtdTokenUnion.dtdTokenBitmap.status &
                                   USB_DEVICE_ECHI_DTD_STATUS_ACTIVE)) &&
                                (currentDtd->dtdTokenUnion.dtdTokenBitmap.ioc))
                            {
                                isTokenDone = 1U;
                            }
                            break;
                        }
                        currentDtd = (usb_device_ehci_dtd_struct_t *)(currentDtd->nextDtdPointer &
                                                                      USB_DEVICE_ECHI_DTD_POINTER_MASK);
                    }

                    if ((0 == isTokenDone) && (currentDtd))
                    {
                        break;
                    }

                    /* Get the in-used dtd of the specified endpoint. */
                    currentDtd = (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] &
                                                                  USB_DEVICE_ECHI_DTD_POINTER_MASK);
                    while (currentDtd)
                    {
                        /* Don't handle the active dtd. */
                        if (currentDtd->dtdTokenUnion.dtdTokenBitmap.status & USB_DEVICE_ECHI_DTD_STATUS_ACTIVE)
                        {
                            break;
                        }

                        /* Save the transfer buffer address */
                        if (NULL == message.buffer)
                        {
                            message.buffer =
                                (uint8_t *)((currentDtd->bufferPointerPage[0] & USB_DEVICE_ECHI_DTD_PAGE_MASK) |
                                            (currentDtd->reservedUnion.originalBufferInfo.originalBufferOffest));
                        }
                        /* Save the transferred data length */
                        message.length += (currentDtd->reservedUnion.originalBufferInfo.originalBufferLength -
                                           currentDtd->dtdTokenUnion.dtdTokenBitmap.totalBytes);

                        /* Move the dtd queue head pointer to next */
                        if (ehciState->dtdHard[index] == ehciState->dtdTail[index])
                        {
                            ehciState->dtdHard[index] = NULL;
                            ehciState->dtdTail[index] = NULL;
                            ehciState->qh[index].nextDtdPointer = USB_DEVICE_ECHI_DTD_TERMINATE_MASK;
                            ehciState->qh[index].dtdTokenUnion.dtdToken = 0U;
                        }
                        else
                        {
                            ehciState->dtdHard[index] =
                                (usb_device_ehci_dtd_struct_t *)ehciState->dtdHard[index]->nextDtdPointer;
                        }

                        /* When the ioc is set or the dtd queue is empty, the up layer will be notified. */
                        if ((currentDtd->dtdTokenUnion.dtdTokenBitmap.ioc) ||
                            (0 == ((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK)))
                        {
                            message.code = endpoint | (uint8_t)((uint32_t)direction << 0x07U);
                            message.isSetup = 0U;
                            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
                            message.buffer = NULL;
                            message.length = 0U;
                        }
                        /* Clear the token field of the dtd */
                        currentDtd->dtdTokenUnion.dtdToken = 0U;
                        currentDtd->nextDtdPointer = (uint32_t)ehciState->dtdFree;
                        ehciState->dtdFree = currentDtd;
                        ehciState->dtdCount++;
                        /* Get the next in-used dtd */
                        currentDtd = (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] &
                                                                      USB_DEVICE_ECHI_DTD_POINTER_MASK);

                        if ((NULL != currentDtd) &&
                            (currentDtd->dtdTokenUnion.dtdTokenBitmap.status & USB_DEVICE_ECHI_DTD_STATUS_ACTIVE))
                        {
                            primeBit = 1U << (endpoint + 16U * direction);

                            /* Try to prime the next dtd. */
                            ehciState->registerBase->EPPRIME = primeBit;

                            /* Whether the endpoint transmit/receive buffer is ready or not. If not, wait for prime bit
                             * cleared and prime the next dtd. */
                            if (!(ehciState->registerBase->EPSR & primeBit))
                            {
                                /* Wait for the endpoint prime bit cleared by HW */
                                while (ehciState->registerBase->EPPRIME & primeBit)
                                {
                                }

                                /* If the endpoint transmit/receive buffer is not ready */
                                if (!(ehciState->registerBase->EPSR & primeBit))
                                {
                                    /* Prime next dtd and prime the transfer */
                                    ehciState->qh[index].nextDtdPointer = (uint32_t)currentDtd;
                                    ehciState->qh[index].dtdTokenUnion.dtdToken = 0U;
                                    ehciState->registerBase->EPPRIME = primeBit;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/*!
 * @brief Handle the port status change interrupt.
 *
 * The function is used to handle the port status change interrupt.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciInterruptPortChange(usb_device_ehci_state_struct_t *ehciState)
{
    usb_device_callback_message_struct_t message;

    message.buffer = (uint8_t *)NULL;
    message.length = 0U;
    message.isSetup = 0U;

    /* Whether the port is doing reset. */
    if (!(ehciState->registerBase->PORTSC1 & USBHS_PORTSC1_PR_MASK))
    {
        /* If not, update the USB speed. */
        if (ehciState->registerBase->PORTSC1 & USBHS_PORTSC1_HSP_MASK)
        {
            ehciState->speed = USB_SPEED_HIGH;
        }
        else
        {
            ehciState->speed = USB_SPEED_FULL;
        }

        /* If the device reset flag is non-zero, notify the up layer the device reset finished. */
        if (ehciState->isResetting)
        {
            message.code = kUSB_DeviceNotifyBusReset;
            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
            ehciState->isResetting = 0U;
        }
    }

#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
    if ((ehciState->isSuspending) && (!(ehciState->registerBase->PORTSC1 & USBHS_PORTSC1_SUSP_MASK)))
    {
        /* Set the resume flag */
        ehciState->isSuspending = 0U;

        message.code = kUSB_DeviceNotifyResume;
        USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
    }
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */
}

/*!
 * @brief Handle the reset interrupt.
 *
 * The function is used to handle the reset interrupt.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciInterruptReset(usb_device_ehci_state_struct_t *ehciState)
{
    uint32_t status = 0U;

    /* Clear the setup flag */
    status = ehciState->registerBase->EPSETUPSR;
    ehciState->registerBase->EPSETUPSR = status;
    /* Clear the endpoint complete flag */
    status = ehciState->registerBase->EPCOMPLETE;
    ehciState->registerBase->EPCOMPLETE = status;

    do
    {
        /* Flush the pending transfers */
        ehciState->registerBase->EPFLUSH = USBHS_EPFLUSH_FERB_MASK | USBHS_EPFLUSH_FETB_MASK;
    } while (ehciState->registerBase->EPPRIME & (USBHS_EPPRIME_PERB_MASK | USBHS_EPPRIME_PETB_MASK));

    /* Whether is the port reset. If yes, set the isResetting flag. Or, notify the up layer. */
    if (ehciState->registerBase->PORTSC1 & USBHS_PORTSC1_PR_MASK)
    {
        ehciState->isResetting = 1U;
    }
    else
    {
        usb_device_callback_message_struct_t message;
        message.buffer = (uint8_t *)NULL;
        message.code = kUSB_DeviceNotifyBusReset;
        message.length = 0U;
        message.isSetup = 0U;

        USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
    }
}

/*!
 * @brief Handle the sof interrupt.
 *
 * The function is used to handle the sof interrupt.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciInterruptSof(usb_device_ehci_state_struct_t *ehciState)
{
}

#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
/*!
 * @brief Handle the suspend interrupt.
 *
 * The function is used to handle the suspend interrupt.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 *
 */
static void USB_DeviceEhciInterruptSuspend(usb_device_ehci_state_struct_t *ehciState)
{
    /* If the port is in suspend state, notify the up layer */
    if (ehciState->registerBase->PORTSC1 & USBHS_PORTSC1_SUSP_MASK)
    {
#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
#else
        if (ehciState->registerPhyBase->USB1_VBUS_DET_STAT & USBPHY_USB1_VBUS_DET_STAT_VBUS_VALID_3V_MASK)
#endif
        {
            usb_device_callback_message_struct_t message;
            message.buffer = (uint8_t *)NULL;
            message.length = 0U;
            message.isSetup = 0U;
            message.code = kUSB_DeviceNotifySuspend;
            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
        }
    }
}
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */

/*!
 * @brief Get dtds and link to QH.
 *
 * The function is used to get dtds and link to QH.
 *
 * @param ehciState       Pointer of the device EHCI state structure.
 * @param endpointAddress The endpoint address, Bit7, 0U - USB_OUT, 1U - USB_IN.
 * @param buffer           The memory address needed to be transferred.
 * @param length           Data length.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
static usb_status_t USB_DeviceEhciTransfer(usb_device_ehci_state_struct_t *ehciState,
                                           uint8_t endpointAddress,
                                           uint8_t *buffer,
                                           uint32_t length)
{
    usb_device_ehci_dtd_struct_t *dtd;
    usb_device_ehci_dtd_struct_t *dtdHard;
    uint32_t index = ((endpointAddress & USB_ENDPOINT_NUMBER_MASK) << 1U) |
                     ((endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                      USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
    uint32_t primeBit = 1U << ((endpointAddress & USB_ENDPOINT_NUMBER_MASK) +
                               ((endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> 0x03U));
    uint32_t epStatus = primeBit;
    uint32_t sendLength;
    uint32_t currentIndex = 0U;
    uint32_t dtdRequestCount = (length + USB_DEVICE_ECHI_DTD_TOTAL_BYTES - 1U) / USB_DEVICE_ECHI_DTD_TOTAL_BYTES;
    uint8_t qhIdle = 0U;
    uint8_t waitingSafelyAccess = 1U;
    uint32_t primeTimesCount = 0U;
    USB_OSA_SR_ALLOC();

    if (!ehciState)
    {
        return kStatus_USB_InvalidHandle;
    }

    if (0U == ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.isOpened)
    {
        return kStatus_USB_Error;
    }
    /* Return error when ehci is doing reset */
    if (ehciState->isResetting)
    {
        return kStatus_USB_Error;
    }

    if (!dtdRequestCount)
    {
        dtdRequestCount = 1U;
    }

    USB_OSA_ENTER_CRITICAL();
    /* The free dtd count need to not less than the transfer requests. */
    if (dtdRequestCount > (uint32_t)ehciState->dtdCount)
    {
        USB_OSA_EXIT_CRITICAL();
        return kStatus_USB_Busy;
    }

    do
    {
        /* The transfer length need to not more than USB_DEVICE_ECHI_DTD_TOTAL_BYTES for each dtd. */
        if (length > USB_DEVICE_ECHI_DTD_TOTAL_BYTES)
        {
            sendLength = USB_DEVICE_ECHI_DTD_TOTAL_BYTES;
        }
        else
        {
            sendLength = length;
        }
        length -= sendLength;

        /* Get a free dtd */
        dtd = ehciState->dtdFree;

        ehciState->dtdFree = (usb_device_ehci_dtd_struct_t *)dtd->nextDtdPointer;
        ehciState->dtdCount--;

        /* Save the dtd head when current active buffer offset is zero. */
        if (!currentIndex)
        {
            dtdHard = dtd;
        }

        /* Set the dtd field */
        dtd->nextDtdPointer = USB_DEVICE_ECHI_DTD_TERMINATE_MASK;
        dtd->dtdTokenUnion.dtdToken = 0U;
        dtd->bufferPointerPage[0] = (uint32_t)(buffer + currentIndex);
        dtd->bufferPointerPage[1] =
            (dtd->bufferPointerPage[0] + USB_DEVICE_ECHI_DTD_PAGE_BLOCK) & USB_DEVICE_ECHI_DTD_PAGE_MASK;
        dtd->bufferPointerPage[2] = dtd->bufferPointerPage[1] + USB_DEVICE_ECHI_DTD_PAGE_BLOCK;
        dtd->bufferPointerPage[3] = dtd->bufferPointerPage[2] + USB_DEVICE_ECHI_DTD_PAGE_BLOCK;
        dtd->bufferPointerPage[4] = dtd->bufferPointerPage[3] + USB_DEVICE_ECHI_DTD_PAGE_BLOCK;

        dtd->dtdTokenUnion.dtdTokenBitmap.totalBytes = sendLength;

        /* Save the data length needed to be transferred. */
        dtd->reservedUnion.originalBufferInfo.originalBufferLength = sendLength;
        /* Save the original buffer address */
        dtd->reservedUnion.originalBufferInfo.originalBufferOffest =
            dtd->bufferPointerPage[0] & USB_DEVICE_ECHI_DTD_PAGE_OFFSET_MASK;
        dtd->reservedUnion.originalBufferInfo.dtdInvalid = 0U;

        /* Set the IOC field in last dtd. */
        if (!length)
        {
            dtd->dtdTokenUnion.dtdTokenBitmap.ioc = 1U;
        }

        /* Set dtd active */
        dtd->dtdTokenUnion.dtdTokenBitmap.status = USB_DEVICE_ECHI_DTD_STATUS_ACTIVE;

        /* Move the buffer offset index */
        currentIndex += sendLength;

        /* Add dtd to the in-used dtd queue */
        if (ehciState->dtdTail[index])
        {
            ehciState->dtdTail[index]->nextDtdPointer = (uint32_t)dtd;
            ehciState->dtdTail[index] = dtd;
        }
        else
        {
            ehciState->dtdHard[index] = dtd;
            ehciState->dtdTail[index] = dtd;
            qhIdle = 1U;
        }
    } while (length);
#if (defined USB_DEVICE_CONTROLLER_AUTO_CONTROL_TRANSFER_ZLP) && (USB_DEVICE_CONTROLLER_AUTO_CONTROL_TRANSFER_ZLP)
    if ((USB_CONTROL_ENDPOINT == (endpointAddress & USB_ENDPOINT_NUMBER_MASK)) && (USB_IN == ((endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                      USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT)))
    {
        uint8_t setupindex = ((endpointAddress & USB_ENDPOINT_NUMBER_MASK)* 2U);
        /* Get last setup packet */
        usb_setup_struct_t *deviceSetup =
                    (usb_setup_struct_t *)&ehciState->qh[setupindex].setupBufferBack[0];
        if (1U == ehciState->qh[index].endpointStatusUnion.endpointStatusBitmap.zlt)
        {
            if ((sendLength) && (sendLength < deviceSetup->wLength) && (!(sendLength % ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.maxPacketSize)))
            {
                /* enable ZLT. */
                ehciState->qh[index].capabilttiesCharacteristicsUnion.capabilttiesCharacteristicsBitmap.zlt = 0U;
            }
        }
    }
#endif
    /* If the QH is not empty */
    if (!qhIdle)
    {
        /* If the prime bit is set, nothing need to do. */
        if (ehciState->registerBase->EPPRIME & primeBit)
        {
            USB_OSA_EXIT_CRITICAL();
            return kStatus_USB_Success;
        }

        /* To safely a dtd */
        while (waitingSafelyAccess)
        {
            /* set the ATDTW flag to USBHS_USBCMD_REG. */
            ehciState->registerBase->USBCMD |= USBHS_USBCMD_ATDTW_MASK;
            /* Read EPSR */
            epStatus = ehciState->registerBase->EPSR;
            /* Wait the ATDTW bit set */
            if (ehciState->registerBase->USBCMD & USBHS_USBCMD_ATDTW_MASK)
            {
                waitingSafelyAccess = 0U;
            }
        }
        /* Clear the ATDTW bit */
        ehciState->registerBase->USBCMD &= ~USBHS_USBCMD_ATDTW_MASK;
    }

    /* If QH is empty or the endpoint is not primed, need to link current dtd head to the QH. */
    /* When the endpoint is not primed if qhIdle is zero, it means the QH is empty. */
    if ((qhIdle) || (!(epStatus & primeBit)))
    {
        ehciState->qh[index].nextDtdPointer = (uint32_t)dtdHard;
        ehciState->qh[index].dtdTokenUnion.dtdToken = 0U;
        ehciState->registerBase->EPPRIME = primeBit;
        while (!(ehciState->registerBase->EPSR & primeBit))
        {
            primeTimesCount++;
            if (primeTimesCount == USB_DEVICE_MAX_TRANSFER_PRIME_TIMES)
            {
                USB_OSA_EXIT_CRITICAL();
                return kStatus_USB_Error;
            }
            if (ehciState->registerBase->EPCOMPLETE & primeBit)
            {
                break;
            }
            else
            {
                ehciState->registerBase->EPPRIME = primeBit;
            }
        }
    }

    USB_OSA_EXIT_CRITICAL();
    return kStatus_USB_Success;
}

/*!
 * @brief Get a valid device EHCI state for the device EHCI instance.
 *
 * This function gets a valid device EHCI state for the USB device EHCI module specified by the controllerId.
 *
 * @param instanceIndex The instanceIndex is used for other EHCI device structure to identify their instance index.
 *
 * @return A valid EHCI state or NULL.
 */
void *USB_EhciGetValidEhciState(uint8_t *instanceIndex)
{
    for (uint8_t instance = 0; instance < USB_DEVICE_CONFIG_EHCI; instance++)
    {
        if (!g_UsbDeviceEhciStateStatus[instance])
        {
            g_UsbDeviceEhciStateStatus[instance] = 1;
            *instanceIndex = instance;
            return &g_UsbDeviceEhciState[instance];
        }
    }
    return NULL;
}

/*!
 * @brief Initialize the USB device EHCI instance.
 *
 * This function initializes the USB device EHCI module specified by the controllerId.
 *
 * @param controllerId The controller id of the USB IP. Please refer to enumeration type usb_controller_index_t.
 * @param handle        Pointer of the device handle, used to identify the device object is belonged to.
 * @param ehciHandle   It is out parameter, is used to return pointer of the device EHCI handle to the caller.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciInit(uint8_t controllerId,
                                usb_device_handle handle,
                                usb_device_controller_handle *ehciHandle)
{
    usb_device_ehci_state_struct_t *ehciState;
    uint32_t ehci_base[] = USBHS_BASE_ADDRS;
    uint8_t intanceIndex;

#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
    usb_device_dcd_state_struct_t *dcdHSState;
    uint32_t dcd_base[] = USBHSDCD_BASE_ADDRS;
    usb_device_callback_message_struct_t message;
#endif

    if ((controllerId < kUSB_ControllerEhci0) ||
        ((uint32_t)(controllerId - kUSB_ControllerEhci0) >= (sizeof(ehci_base) / sizeof(uint32_t))))
    {
        return kStatus_USB_ControllerNotFound;
    }

    ehciState = USB_EhciGetValidEhciState(&intanceIndex);

    ehciState->dtd = s_UsbDeviceEhciDtd[intanceIndex];
    ehciState->qh = (usb_device_ehci_qh_struct_t *)&qh_buffer[intanceIndex * 2048];

    ehciState->controllerId = controllerId;

    ehciState->registerBase = (USBHS_Type *)ehci_base[controllerId - kUSB_ControllerEhci0];
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
    ehciState->registerPhyBase = (USBPHY_Type *)USB_EhciPhyGetBase(controllerId);

#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
    ehciState->registerNcBase = (USBNC_Type *)USB_EhciNCGetBase(controllerId);
#endif

#endif

    /* Get the HW's endpoint count */
    ehciState->endpointCount =
        (uint8_t)((ehciState->registerBase->DCCPARAMS & USBHS_DCCPARAMS_DEN_MASK) >> USBHS_DCCPARAMS_DEN_SHIFT);

    if (ehciState->endpointCount < USB_DEVICE_CONFIG_ENDPOINTS)
    {
        return kStatus_USB_Error;
    }
    ehciState->deviceHandle = (usb_device_struct_t *)handle;

    /* Clear the controller mode field and set to device mode. */
    ehciState->registerBase->USBMODE &= ~USBHS_USBMODE_CM_MASK;
    ehciState->registerBase->USBMODE |= USBHS_USBMODE_CM(0x02U);

    /* Set the EHCI to default status. */
    USB_DeviceEhciSetDefaultState(ehciState);
    *ehciHandle = (usb_device_controller_handle)ehciState;
#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
    dcdHSState = &s_UsbDeviceDcdHSState[controllerId - kUSB_ControllerEhci0];

    dcdHSState->controllerId = controllerId;

    dcdHSState->dcdRegisterBase = (USBHSDCD_Type *)dcd_base[controllerId - kUSB_ControllerEhci0];

    dcdHSState->deviceHandle = (usb_device_struct_t *)handle;

    message.buffer = (uint8_t *)NULL;
    message.length = 0U;
    message.isSetup = 0U;
    if (ehciState->registerBase->OTGSC & USBHS_OTGSC_BSV_MASK)
    {
        /* Device is connected to a host. */
        message.code = kUSB_DeviceNotifyAttach;
        USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
    }
#endif

#ifdef DT_USBD_MCUX_EHCI_MAXIMUM_SPEED
    if (!strncmp(DT_USBD_MCUX_EHCI_MAXIMUM_SPEED, "full-speed", 10))
    {
        ehciState->registerBase->PORTSC1 |= USB_PORTSC1_PFSC_MASK;
    }
#endif /* DT_USBD_MCUX_EHCI_MAXIMUM_SPEED */

    return kStatus_USB_Success;
}

/*!
 * @brief De-initialize the USB device EHCI instance.
 *
 * This function de-initializes the USB device EHCI module.
 *
 * @param ehciHandle   Pointer of the device EHCI handle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciDeinit(usb_device_controller_handle ehciHandle)
{
    usb_device_ehci_state_struct_t *ehciState = (usb_device_ehci_state_struct_t *)ehciHandle;

    if (!ehciHandle)
    {
        return kStatus_USB_InvalidHandle;
    }
    for (uint8_t instance = 0; instance < USB_DEVICE_CONFIG_EHCI; instance++)
    {
        if (ehciState == &g_UsbDeviceEhciState[instance])
        {
            g_UsbDeviceEhciStateStatus[instance] = 0;
        }
    }
    
    /* Disable all interrupt. */
    ehciState->registerBase->USBINTR = 0U;
    /* Stop the device functionality. */
    ehciState->registerBase->USBCMD &= ~USBHS_USBCMD_RS_MASK;
    /* Reset the controller. */
    ehciState->registerBase->USBCMD |= USBHS_USBCMD_RST_MASK;

    return kStatus_USB_Success;
}

/*!
 * @brief Send data through a specified endpoint.
 *
 * This function sends data through a specified endpoint.
 *
 * @param ehciHandle      Pointer of the device EHCI handle.
 * @param endpointAddress Endpoint index.
 * @param buffer           The memory address to hold the data need to be sent.
 * @param length           The data length need to be sent.
 *
 * @return A USB error code or kStatus_USB_Success.
 *
 * @note The return value just means if the sending request is successful or not; the transfer done is notified by the
 * corresponding callback function.
 * Currently, only one transfer request can be supported for one specific endpoint.
 * If there is a specific requirement to support multiple transfer requests for one specific endpoint, the application
 * should implement a queue in the application level.
 * The subsequent transfer could begin only when the previous transfer is done (get notification through the endpoint
 * callback).
 */
usb_status_t USB_DeviceEhciSend(usb_device_controller_handle ehciHandle,
                                uint8_t endpointAddress,
                                uint8_t *buffer,
                                uint32_t length)
{
    /* Add dtd to the QH */
    return USB_DeviceEhciTransfer(
        (usb_device_ehci_state_struct_t *)ehciHandle,
        (endpointAddress & USB_ENDPOINT_NUMBER_MASK) | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        buffer, length);
}

/*!
 * @brief Receive data through a specified endpoint.
 *
 * This function Receives data through a specified endpoint.
 *
 * @param ehciHandle      Pointer of the device EHCI handle.
 * @param endpointAddress Endpoint index.
 * @param buffer           The memory address to save the received data.
 * @param length           The data length want to be received.
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
                                uint32_t length)
{
    /* Add dtd to the QH */
    return USB_DeviceEhciTransfer(
        (usb_device_ehci_state_struct_t *)ehciHandle,
        (endpointAddress & USB_ENDPOINT_NUMBER_MASK) | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        buffer, length);
}

/*!
 * @brief Cancel the pending transfer in a specified endpoint.
 *
 * The function is used to cancel the pending transfer in a specified endpoint.
 *
 * @param ehciHandle      Pointer of the device EHCI handle.
 * @param ep               Endpoint address, bit7 is the direction of endpoint, 1U - IN, 0U - OUT.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciCancel(usb_device_controller_handle ehciHandle, uint8_t ep)
{
    usb_device_ehci_state_struct_t *ehciState = (usb_device_ehci_state_struct_t *)ehciHandle;
    usb_device_callback_message_struct_t message;
    usb_device_ehci_dtd_struct_t *currentDtd;
    uint32_t primeBit =
        1U << ((ep & USB_ENDPOINT_NUMBER_MASK) + ((ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> 0x03U));
    uint8_t index =
        ((ep & USB_ENDPOINT_NUMBER_MASK) << 1U) | ((ep & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >> 0x07U);

    USB_OSA_SR_ALLOC();

    if (!ehciHandle)
    {
        return kStatus_USB_InvalidHandle;
    }

    USB_OSA_ENTER_CRITICAL();

    message.buffer = NULL;
    message.length = USB_UNINITIALIZED_VAL_32;

    /* Get the first dtd */
    currentDtd =
        (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK);

    while (currentDtd)
    {
        currentDtd->reservedUnion.originalBufferInfo.dtdInvalid = 1U;
        currentDtd = (usb_device_ehci_dtd_struct_t *)(currentDtd->nextDtdPointer & USB_DEVICE_ECHI_DTD_POINTER_MASK);
    }

    /* Get the first dtd */
    currentDtd =
        (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK);
    while (currentDtd)
    {
        if (!currentDtd->reservedUnion.originalBufferInfo.dtdInvalid)
        {
            break;
        }
        else
        {
            if (currentDtd->dtdTokenUnion.dtdTokenBitmap.status & USB_DEVICE_ECHI_DTD_STATUS_ACTIVE)
            {
                /* Flush the endpoint to stop a transfer. */
                do
                {
                    /* Set the corresponding bit(s) in the EPFLUSH register */
                    ehciState->registerBase->EPFLUSH |= primeBit;

                    /* Wait until all bits in the EPFLUSH register are cleared. */
                    while (ehciState->registerBase->EPFLUSH & primeBit)
                    {
                    }
                    /*
                     * Read the EPSR register to ensure that for all endpoints
                     * commanded to be flushed, that the corresponding bits
                     * are now cleared.
                     */
                } while (ehciState->registerBase->EPSR & primeBit);
            }

            /* Save the original buffer address. */
            if (NULL == message.buffer)
            {
                message.buffer = (uint8_t *)((currentDtd->bufferPointerPage[0] & USB_DEVICE_ECHI_DTD_PAGE_MASK) |
                                             (currentDtd->reservedUnion.originalBufferInfo.originalBufferOffest));
            }

            /* Remove the dtd from the dtd in-used queue. */
            if (ehciState->dtdHard[index] == ehciState->dtdTail[index])
            {
                ehciState->dtdHard[index] = NULL;
                ehciState->dtdTail[index] = NULL;
            }
            else
            {
                ehciState->dtdHard[index] = (usb_device_ehci_dtd_struct_t *)ehciState->dtdHard[index]->nextDtdPointer;
            }

            /* When the ioc is set or the dtd queue is empty, the up layer will be notified. */
            if ((currentDtd->dtdTokenUnion.dtdTokenBitmap.ioc) ||
                (0 == ((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK)))
            {
                message.code = ep;
                message.isSetup = 0U;
                USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
                message.buffer = NULL;
            }
            /* Clear the token field. */
            currentDtd->dtdTokenUnion.dtdToken = 0U;
            /* Save the dtd to the free queue. */
            currentDtd->nextDtdPointer = (uint32_t)ehciState->dtdFree;
            ehciState->dtdFree = currentDtd;
            ehciState->dtdCount++;
        }
        /* Get the next dtd. */
        currentDtd =
            (usb_device_ehci_dtd_struct_t *)((uint32_t)ehciState->dtdHard[index] & USB_DEVICE_ECHI_DTD_POINTER_MASK);
    }
    if (!currentDtd)
    {
        /* Set the QH to empty. */
        ehciState->qh[index].nextDtdPointer = USB_DEVICE_ECHI_DTD_TERMINATE_MASK;
        ehciState->qh[index].dtdTokenUnion.dtdToken = 0U;
    }
    USB_OSA_EXIT_CRITICAL();
    return kStatus_USB_Success;
}

/*!
 * @brief Control the status of the selected item.
 *
 * The function is used to control the status of the selected item.
 *
 * @param ehciHandle      Pointer of the device EHCI handle.
 * @param type             The selected item. Please refer to enumeration type usb_device_control_type_t.
 * @param param            The param type is determined by the selected item.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceEhciControl(usb_device_controller_handle ehciHandle, usb_device_control_type_t type, void *param)
{
    usb_device_ehci_state_struct_t *ehciState = (usb_device_ehci_state_struct_t *)ehciHandle;
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16;
    uint8_t *temp8;
#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
    usb_device_dcd_state_struct_t *dcdHSState;
    dcdHSState =
        &s_UsbDeviceDcdHSState[ehciState->controllerId - kUSB_ControllerEhci0]; /*The hard code should be replaced*/
    usb_device_dcd_charging_time_t *deviceDcdTimingConfig = (usb_device_dcd_charging_time_t *)param;
#endif
#if ((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U))
    usb_device_struct_t *deviceHandle;
    uint64_t startTick;
#endif

    if (!ehciHandle)
    {
        return kStatus_USB_InvalidHandle;
    }

#if ((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U))
    deviceHandle = (usb_device_struct_t *)ehciState->deviceHandle;
#endif

    switch (type)
    {
        case kUSB_DeviceControlRun:
            ehciState->registerBase->USBCMD |= USBHS_USBCMD_RS_MASK;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceControlStop:
            ehciState->registerBase->USBCMD &= ~USBHS_USBCMD_RS_MASK;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceControlEndpointInit:
            if (param)
            {
                error = USB_DeviceEhciEndpointInit(ehciState, (usb_device_endpoint_init_struct_t *)param);
            }
            break;
        case kUSB_DeviceControlEndpointDeinit:
            if (param)
            {
                temp8 = (uint8_t *)param;
                error = USB_DeviceEhciEndpointDeinit(ehciState, *temp8);
            }
            break;
        case kUSB_DeviceControlEndpointStall:
            if (param)
            {
                temp8 = (uint8_t *)param;
                error = USB_DeviceEhciEndpointStall(ehciState, *temp8);
            }
            break;
        case kUSB_DeviceControlEndpointUnstall:
            if (param)
            {
                temp8 = (uint8_t *)param;
                error = USB_DeviceEhciEndpointUnstall(ehciState, *temp8);
            }
            break;
        case kUSB_DeviceControlGetDeviceStatus:
            if (param)
            {
                temp16 = (uint16_t *)param;
                *temp16 = (USB_DEVICE_CONFIG_SELF_POWER << (USB_REQUEST_STANDARD_GET_STATUS_DEVICE_SELF_POWERED_SHIFT))
#if ((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U))
                          | (deviceHandle->remotewakeup << (USB_REQUEST_STANDARD_GET_STATUS_DEVICE_REMOTE_WARKUP_SHIFT))
#endif
                    ;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceControlGetEndpointStatus:
            if (param)
            {
                usb_device_endpoint_status_struct_t *endpointStatus = (usb_device_endpoint_status_struct_t *)param;
                uint8_t ep = (endpointStatus->endpointAddress) & USB_ENDPOINT_NUMBER_MASK;
                uint8_t direction =
                    ((endpointStatus->endpointAddress) & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                    USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT;

                if (ep < USB_DEVICE_CONFIG_ENDPOINTS)
                {
                    if (ep)
                    {
                        endpointStatus->endpointStatus = (ehciState->registerBase->EPCR[ep - 1U] &
                                                          (direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK)) ?
                                                             kUSB_DeviceEndpointStateStalled :
                                                             kUSB_DeviceEndpointStateIdle;
                    }
                    else
                    {
                        endpointStatus->endpointStatus =
                            (ehciState->registerBase->EPCR0 & (direction ? USBHS_EPCR_TXS_MASK : USBHS_EPCR_RXS_MASK)) ?
                                kUSB_DeviceEndpointStateStalled :
                                kUSB_DeviceEndpointStateIdle;
                    }
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceControlSetDeviceAddress:
            if (param)
            {
                temp8 = (uint8_t *)param;
                ehciState->registerBase->DEVICEADDR = (((uint32_t)(*temp8)) << USBHS_DEVICEADDR_USBADR_SHIFT);
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceControlGetSynchFrame:
            break;
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
#if defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U)
        case kUSB_DeviceControlResume:
#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
            ehciState->registerNcBase->USB_OTGn_CTRL &= ~USBNC_USB_OTGn_CTRL_WIE_MASK;
#else
            ehciState->registerBase->USBGENCTRL &= ~USBHS_USBGENCTRL_WU_IE_MASK;
#endif
            ehciState->registerBase->PORTSC1 &= ~USBHS_PORTSC1_PHCD_MASK;
            ehciState->registerBase->PORTSC1 |= USBHS_PORTSC1_FPR_MASK;
            startTick = deviceHandle->hwTick;
            while ((deviceHandle->hwTick - startTick) < 10)
            {
                __ASM("nop");
            }
            ehciState->registerBase->PORTSC1 &= ~USBHS_PORTSC1_FPR_MASK;
            error = kStatus_USB_Success;
            break;
#endif /* USB_DEVICE_CONFIG_REMOTE_WAKEUP */
        case kUSB_DeviceControlSuspend:
            ehciState->registerBase->OTGSC |= 0x007F0000U;
            ehciState->registerPhyBase->PWD = 0xFFFFFFFF;
            /* ehciState->registerBase->OTGCTL |= ((1U<<10) | (1U<<17) | (1U<<16)); */
            while (ehciState->registerPhyBase->CTRL & (USBPHY_CTRL_UTMI_SUSPENDM_MASK))
            {
                __ASM("nop");
            }
            /* ehciState->registerPhyBase->CTRL |= ((1U << 21) | (1U << 22) | (1U << 23)); */
            ehciState->registerBase->USBSTS |= USBHS_USBSTS_SRI_MASK;
            ehciState->registerBase->PORTSC1 |= USBHS_PORTSC1_PHCD_MASK;
#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))
            ehciState->registerPhyBase->CTRL |= USBPHY_CTRL_ENVBUSCHG_WKUP_MASK | USBPHY_CTRL_ENIDCHG_WKUP_MASK |
                                                USBPHY_CTRL_ENDPDMCHG_WKUP_MASK | USBPHY_CTRL_ENIRQRESUMEDETECT_MASK;
            ehciState->registerNcBase->USB_OTGn_CTRL |= USBNC_USB_OTGn_CTRL_WKUP_ID_EN_MASK |
                                                        USBNC_USB_OTGn_CTRL_WKUP_VBUS_EN_MASK |
                                                        USBNC_USB_OTGn_CTRL_WKUP_DPDM_EN_MASK;
            ehciState->registerNcBase->USB_OTGn_CTRL |= USBNC_USB_OTGn_CTRL_WIE_MASK;
#else
            ehciState->registerBase->USBGENCTRL = USBHS_USBGENCTRL_WU_IE_MASK;
#endif
            ehciState->registerPhyBase->CTRL |= USBPHY_CTRL_CLKGATE_MASK;
            ehciState->isSuspending = 1U;
            error = kStatus_USB_Success;
            break;
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */
        case kUSB_DeviceControlSetDefaultStatus:
            for (uint8_t count = 0U; count < USB_DEVICE_CONFIG_ENDPOINTS; count++)
            {
                USB_DeviceEhciEndpointDeinit(ehciState, (count | (USB_IN << 0x07U)));
                USB_DeviceEhciEndpointDeinit(ehciState, (count | (USB_OUT << 0x07U)));
            }
            USB_DeviceEhciSetDefaultState(ehciState);
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceControlGetSpeed:
            if (param)
            {
                temp8 = (uint8_t *)param;
                *temp8 = ehciState->speed;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceControlGetOtgStatus:
            break;
        case kUSB_DeviceControlSetOtgStatus:
            break;
#if (defined(USB_DEVICE_CONFIG_USB20_TEST_MODE) && (USB_DEVICE_CONFIG_USB20_TEST_MODE > 0U))
        case kUSB_DeviceControlSetTestMode:
            if (param)
            {
                temp8 = (uint8_t *)param;
                ehciState->registerBase->PORTSC1 |= ((uint32_t)(*temp8) << 16U);
                error = kStatus_USB_Success;
            }
            break;
#endif
#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
        case kUSB_DeviceControlDcdInitModule:
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
            dcdHSState->dcdRegisterBase->TIMER0 = USBDCD_TIMER0_TSEQ_INIT(deviceDcdTimingConfig->dcdSeqInitTime);
            dcdHSState->dcdRegisterBase->TIMER1 = USBDCD_TIMER1_TDCD_DBNC(deviceDcdTimingConfig->dcdDbncTime);
            dcdHSState->dcdRegisterBase->TIMER1 |= USBDCD_TIMER1_TVDPSRC_ON(deviceDcdTimingConfig->dcdDpSrcOnTime);
            dcdHSState->dcdRegisterBase->TIMER2_BC12 =
                USBDCD_TIMER2_BC12_TWAIT_AFTER_PRD(deviceDcdTimingConfig->dcdTimeWaitAfterPrD);
            dcdHSState->dcdRegisterBase->TIMER2_BC12 |=
                USBDCD_TIMER2_BC12_TVDMSRC_ON(deviceDcdTimingConfig->dcdTimeDMSrcOn);
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_IE_MASK;
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_BC12_MASK;
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_START_MASK;
            break;
        case kUSB_DeviceControlDcdDeinitModule:
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
            break;
#endif

        default:
            break;
    }

    return error;
}

/*!
 * @brief Handle the EHCI device interrupt.
 *
 * The function is used to handle the EHCI device interrupt.
 *
 * @param deviceHandle    The device handle got from USB_DeviceInit.
 *
 */
void USB_DeviceEhciIsrFunction(void *deviceHandle)
{
    usb_device_struct_t *handle = (usb_device_struct_t *)deviceHandle;
    usb_device_ehci_state_struct_t *ehciState;
    uint32_t status;

    if (NULL == deviceHandle)
    {
        return;
    }

    ehciState = (usb_device_ehci_state_struct_t *)(handle->controllerHandle);

#if ((defined(USB_DEVICE_CONFIG_LOW_POWER_MODE)) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))

#if (defined(FSL_FEATURE_SOC_USBNC_COUNT) && (FSL_FEATURE_SOC_USBNC_COUNT > 0U))

    if (ehciState->registerNcBase->USB_OTGn_CTRL & USBNC_USB_OTGn_CTRL_WIE_MASK)
    {
        if (ehciState->registerNcBase->USB_OTGn_CTRL & USBNC_USB_OTGn_CTRL_WIR_MASK)
        {
            ehciState->registerBase->PORTSC1 &= ~USBHS_PORTSC1_PHCD_MASK;
            ehciState->registerNcBase->USB_OTGn_CTRL &= ~USBNC_USB_OTGn_CTRL_WIE_MASK;
        }
    }
    else
    {
    }

#else
    if (ehciState->registerBase->USBGENCTRL & USBHS_USBGENCTRL_WU_IE_MASK)
    {
        if (ehciState->registerBase->USBGENCTRL & (1U << 8))
        {
            ehciState->registerBase->USBGENCTRL &= ~(1U << 8);
            ehciState->registerBase->USBGENCTRL |= USBHS_USBGENCTRL_WU_INT_CLR_MASK;
            ehciState->registerBase->PORTSC1 &= ~USBHS_PORTSC1_PHCD_MASK;
            ehciState->registerBase->USBGENCTRL &= ~USBHS_USBGENCTRL_WU_IE_MASK;
        }
    }
    else
    {
    }
#endif

#endif

#if defined(USB_DEVICE_CONFIG_DETACH_ENABLE) && (USB_DEVICE_CONFIG_DETACH_ENABLE > 0U)
    if (ehciState->registerBase->OTGSC & USBHS_OTGSC_BSVIS_MASK)
    {
        usb_device_callback_message_struct_t message;

        ehciState->registerBase->OTGSC |= USBHS_OTGSC_BSVIS_MASK;

        message.buffer = (uint8_t *)NULL;
        message.length = 0U;
        message.isSetup = 0U;
        if (ehciState->registerBase->OTGSC & USBHS_OTGSC_BSV_MASK)
        {
            /* Device is connected to a host. */
            message.code = kUSB_DeviceNotifyAttach;
            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
        }
        else
        {
            /* Device is disconnected from a host. */
            message.code = kUSB_DeviceNotifyDetach;
            USB_DeviceNotificationTrigger(ehciState->deviceHandle, &message);
        }
    }
#endif /* USB_DEVICE_CONFIG_DETACH_ENABLE */

    status = ehciState->registerBase->USBSTS;
    status &= ehciState->registerBase->USBINTR;

    ehciState->registerBase->USBSTS = status;

#if defined(USB_DEVICE_CONFIG_ERROR_HANDLING) && (USB_DEVICE_CONFIG_ERROR_HANDLING > 0U)
    if (status & USBHS_USBSTS_UEI_MASK)
    {
        /* Error interrupt */
        USB_DeviceEhciInterruptError(ehciState);
    }
#endif /* USB_DEVICE_CONFIG_ERROR_HANDLING */

    if (status & USBHS_USBSTS_URI_MASK)
    {
        /* Reset interrupt */
        USB_DeviceEhciInterruptReset(ehciState);
    }

    if (status & USBHS_USBSTS_UI_MASK)
    {
        /* Token done interrupt */
        USB_DeviceEhciInterruptTokenDone(ehciState);
    }

    if (status & USBHS_USBSTS_PCI_MASK)
    {
        /* Port status change interrupt */
        USB_DeviceEhciInterruptPortChange(ehciState);
    }

#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
    if (status & USBHS_USBSTS_SLI_MASK)
    {
        /* Suspend interrupt */
        USB_DeviceEhciInterruptSuspend(ehciState);
    }
#endif /* USB_DEVICE_CONFIG_LOW_POWER_MODE */

    if (status & USBHS_USBSTS_SRI_MASK)
    {
        /* Sof interrupt */
        USB_DeviceEhciInterruptSof(ehciState);
    }
}

#if (defined(USB_DEVICE_CHARGER_DETECT_ENABLE) && (USB_DEVICE_CHARGER_DETECT_ENABLE > 0U)) && \
    (defined(FSL_FEATURE_SOC_USBHSDCD_COUNT) && (FSL_FEATURE_SOC_USBHSDCD_COUNT > 0U))
void USB_DeviceDcdHSIsrFunction(void *deviceHandle)
{
    usb_device_struct_t *handle = (usb_device_struct_t *)deviceHandle;
    usb_device_ehci_state_struct_t *ehciState;
    usb_device_dcd_state_struct_t *dcdHSState;
    uint32_t status;
    uint32_t chargerType;
    usb_device_callback_message_struct_t message;

    if (NULL == deviceHandle)
    {
        return;
    }

    ehciState = (usb_device_ehci_state_struct_t *)(handle->controllerHandle);

    dcdHSState = &s_UsbDeviceDcdHSState[ehciState->controllerId - kUSB_ControllerEhci0];

    status = dcdHSState->dcdRegisterBase->STATUS;

    dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_IACK_MASK;

    message.buffer = (uint8_t *)NULL;
    message.length = 0U;
    message.isSetup = 0U;

    if (status & USBDCD_STATUS_ERR_MASK)
    {
        if (status & USBDCD_STATUS_TO_MASK)
        {
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
            message.code = kUSB_DeviceNotifyDcdTimeOut;
            USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
        }
        else
        {
            dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
            message.code = kUSB_DeviceNotifyDcdUnknownPortType;
            USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
        }
    }
    else
    {
        switch (status & USBDCD_STATUS_SEQ_STAT_MASK)
        {
            case USBDCD_STATUS_SEQ_STAT(kUSB_DcdChargingPortDetectionCompleted):
                chargerType = status & USBDCD_STATUS_SEQ_RES_MASK;
                if (chargerType == USBDCD_STATUS_SEQ_RES(kUSB_DcdDetectionStandardHost))
                {
                    dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
                    message.code = kUSB_DeviceNotifySDPDetected;
                    USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
                }
                else if (chargerType == USBDCD_STATUS_SEQ_RES(kUSB_DcdDetectionChargingPort))
                {
                    message.code = kUSB_DeviceNotifyChargingPortDetected;
                    USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
                }
                break;
            case USBDCD_STATUS_SEQ_STAT(kUSB_DcdChargerTypeDetectionCompleted):
                chargerType = status & USBDCD_STATUS_SEQ_RES_MASK;
                if (chargerType == USBDCD_STATUS_SEQ_RES(kUSB_DcdDetectionChargingPort))
                {
                    dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
                    message.code = kUSB_DeviceNotifyChargingHostDetected;
                    USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
                }
                else if (chargerType == USBDCD_STATUS_SEQ_RES(kUSB_DcdDetectionDedicatedCharger))
                {
                    dcdHSState->dcdRegisterBase->CONTROL |= USBDCD_CONTROL_SR_MASK;
                    message.code = kUSB_DeviceNotifyDedicatedChargerDetected;
                    USB_DeviceNotificationTrigger(dcdHSState->deviceHandle, &message);
                }
                break;

            default:
                break;
        }
    }
}
#endif

#endif /* USB_DEVICE_CONFIG_EHCI */
