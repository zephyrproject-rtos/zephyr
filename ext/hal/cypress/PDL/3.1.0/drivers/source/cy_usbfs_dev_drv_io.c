/***************************************************************************//**
* \file cy_usbfs_dev_drv_io.c
* \version 1.0
*
* Provides data transfer API implementation of the USBFS driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include <string.h>
#include "cy_usbfs_dev_drv_pvt.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                        Internal Constants
*******************************************************************************/

/* Dedicated IN and OUT endpoints buffer: 32 bytes (2^5) */
#define ENDPOINTS_BUFFER_SIZE   (0x55UL)

/* Number of bytes transferred by 1 Y-loop */
#define DMA_YLOOP_INCREMENT     (32UL)


/*******************************************************************************
*                        Internal Functions Prototypes
*******************************************************************************/
static void DisableEndpoint(USBFS_Type *base, uint32_t endpoint, cy_stc_usbfs_dev_drv_context_t *context);


/*******************************************************************************
* Function Name: DisableEndpoint
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number from where the data should be read.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
static void DisableEndpoint(USBFS_Type *base, uint32_t endpoint, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Disable endpoint SIE mode and set appropriate state */
    Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, CY_USBFS_DEV_DRV_EP_CR_DISABLE);
    context->epPool[endpoint].state = CY_USB_DEV_EP_DISABLED;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ConfigureDevice
****************************************************************************//**
*
* Sets general device configuration.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_ConfigureDevice(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint;

    /* Clear buffer pointer */
    context->curBufAddr = 0UL;

    /* Disable SIE and Arbiter interrupts */
    base->USBDEV.SIE_EP_INT_EN = 0UL;
    base->USBDEV.ARB_INT_EN    = 0UL;

    /* Prepare endpoints for configuration */
    for (endpoint = 0U; endpoint <  CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++endpoint)
    {
        /* Disable endpoint operation */
        DisableEndpoint(base, endpoint, context);

        /* Set default arbiter configuration */
        Cy_USBFS_Dev_Drv_SetArbEpConfig(base, endpoint, (USBFS_USBDEV_ARB_EP1_CFG_CRC_BYPASS_Msk |
                                                         USBFS_USBDEV_ARB_EP1_CFG_RESET_PTR_Msk));
    }

    if (CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU != context->mode)
    {
        /* Allow re-configuration: clear configuration complete bit */
        base->USBDEV.ARB_CFG &= ~USBFS_USBDEV_ARB_CFG_CFG_CMP_Msk;
        (void) base->USBDEV.ARB_CFG;

        if (CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO == context->mode)
        {
            /* Reset configuration registers */
            base->USBDEV.EP_ACTIVE = 0UL;
            base->USBDEV.EP_TYPE   = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);

            /* Configure DMA burst size */
            base->USBDEV.BUF_SIZE    = ENDPOINTS_BUFFER_SIZE;
            base->USBDEV.DMA_THRES16 = DMA_YLOOP_INCREMENT;
        }
    }
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_UnConfigureDevice
****************************************************************************//**
*
* Clears current device configuration. This function is called before apply 
* new configuration.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_UnConfigureDevice(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint;

    /* Clear endpoints configuration */
    for (endpoint = 0U; endpoint <  CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++endpoint)
    {
        /* Disable endpoint operation */
        DisableEndpoint(base, endpoint, context);
    }
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ConfigureDeviceComplete
****************************************************************************//**
*
* Locks device configuration to complete it.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_ConfigureDeviceComplete(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    if (context->mode != CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU)
    {
        /* Configuration complete: set configuration complete bit (generate rising edge) */
        base->USBDEV.ARB_CFG |= USBFS_USBDEV_ARB_CFG_CFG_CMP_Msk;
    }
}


/*******************************************************************************
* Function Name: GetEndpointBuffer
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number from where the data should be read.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t GetEndpointBuffer(uint32_t size, uint32_t *idx, cy_stc_usbfs_dev_drv_context_t *context)
{
    cy_en_usbfs_dev_drv_status_t drvStatus = CY_USBFS_DEV_DRV_BUF_ALLOC_FAILED;
    uint32_t bufferSize;
    uint32_t nextBufAddr;

    /* Get max buffer size */
    bufferSize = (CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO != context->mode) ? 
                    CY_USBFS_DEV_DRV_HW_BUFFER_SIZE : 
                    context->epSharedBufSize;

    /* Get next buffer address. Note the end buffer size must be even for 16-bit access */
    nextBufAddr  = (context->curBufAddr + size);
    nextBufAddr += (context->useReg16) ? (size & 0x01U) : 0U;

    /* Check if there is enough space in the buffer */
    if (nextBufAddr <= bufferSize)
    {
        /* Update pointers */
        *idx = context->curBufAddr;
        context->curBufAddr = nextBufAddr;

        drvStatus = CY_USBFS_DEV_DRV_SUCCESS;
    }

    return drvStatus;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_AddEpUsbBuffer
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number from where the data should be read.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t AddEndpointHwBuffer(USBFS_Type *base,
                                                 cy_stc_usb_dev_ep_config_t const *config,
                                                 cy_stc_usbfs_dev_drv_context_t     *context)
{
    uint32_t endpoint = EPADDR2PHY(config->endpointAddr);

    /* Get buffer for endpoint using hardware buffer */
    if (config->allocBuffer)
    {
        uint32_t startBufIdx;

        /* Get buffer for endpoint */
        cy_en_usbfs_dev_drv_status_t drvStatus = GetEndpointBuffer(config->bufferSize, &startBufIdx, context);
        if (CY_USBFS_DEV_DRV_SUCCESS != drvStatus)
        {
            return drvStatus;
        }

        /* Set Arbiter read and write pointers */
        Cy_USBFS_Dev_Drv_SetArbWriteAddr(base, endpoint, startBufIdx);
        Cy_USBFS_Dev_Drv_SetArbReadAddr (base, endpoint, startBufIdx);
    }

    /* Enable endpoint for the operation */
    if (config->enableEndpoint)
    {
        bool inDirection = IS_EP_DIR_IN(config->endpointAddr);

        /* Configure endpoint */
        context->epPool[endpoint].state   = CY_USB_DEV_EP_IDLE;
        context->epPool[endpoint].address = config->endpointAddr;
        context->epPool[endpoint].toggle  = 0UL;
        context->epPool[endpoint].bufSize = config->maxPacketSize;
        context->epPool[endpoint].sieMode = GetEndpointActiveMode(inDirection, config->attributes);
        context->epPool[endpoint].isPending = false;

        /* Enable endpoint Arbiter interrupt sources */
        if (CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA == context->mode)
        {
            /* Configure DMA based on endpoint direction */
            cy_en_usbfs_dev_drv_status_t drvStatus = DmaEndpointInit(base, config->endpointAddr, context);
            if (CY_USBFS_DEV_DRV_SUCCESS != drvStatus)
            {
                return drvStatus;
            }

            /* Enable Arbiter interrupt sources for endpoint */
            Cy_USBFS_Dev_Drv_SetArbEpInterruptMask(base, endpoint, (inDirection ?
                                                                    USBFS_USBDEV_ARB_EP_IN_BUF_FULL_Msk :
                                                                    USBFS_USBDEV_ARB_EP_DMA_GNT_Msk));

            /* Enable Arbiter interrupt for endpoint */
            Cy_USBFS_Dev_Drv_EnableArbEpInterrupt(base, endpoint);
        }

        /* Enable SIE interrupt for endpoint */
        Cy_USBFS_Dev_Drv_EnableSieEpInterrupt(base, endpoint);

        /* Set endpoint mode to not respond to host */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, GetEndpointInactiveMode(context->epPool[endpoint].sieMode));
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_RemoveEndpoint
****************************************************************************//**
*
* Removes data endpoint - release hardware resources allocated by data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpointAddr
* Data endpoint number and direction.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_RemoveEndpoint(USBFS_Type *base,
                                                         uint32_t    endpointAddr,
                                                         cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint = EPADDR2PHY(endpointAddr);

    /* Disable endpoint operation */
    DisableEndpoint(base, endpoint, context);

    /* Disable SIE and Arbiter interrupt for endpoint */
    Cy_USBFS_Dev_Drv_DisableSieEpInterrupt(base, endpoint);
    Cy_USBFS_Dev_Drv_DisableArbEpInterrupt(base, endpoint);

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_EnableOutEndpoint
****************************************************************************//**
*
* Enables the specified endpoint for OUT transfers. Do not call this function
* for IN endpoints.
* The OUT endpoints are not enabled by default. The endpoints must be enabled
* before reading the data from the endpoints. You should also enable the
* endpoints after a new Set Configuration request is received from the host.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_EnableOutEndpoint(USBFS_Type *base,
                                        uint32_t    endpoint,
                                        cy_stc_usbfs_dev_drv_context_t *context)
{
    CY_ASSERT_L1(IS_EP_VALID(endpoint));
    CY_ASSERT_L1(false == IS_EP_DIR_IN(context->epPool[EP2PHY(endpoint)].address));

    endpoint = EP2PHY(endpoint);

    /* Clear transfer complete notification */
    context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;
    
    /* Clear toggle for ISOC OUT endpoint */
    if (CY_USBFS_DEV_DRV_EP_CR_ISO_OUT == context->epPool[endpoint].sieMode)
    {
        context->epPool[endpoint].toggle = 0UL;
    }

    /* Arm endpoint: host is allowed to write data */
    Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
}


/*******************************************************************************
* Function Name: LoadInEndpointCpu
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t LoadInEndpointCpu(USBFS_Type   *base,
                                                        uint32_t      endpoint,
                                                        const uint8_t buffer[],
                                                        uint32_t      size,
                                                        cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Request to load more bytes than endpoint buffer */
    if (size > context->epPool[endpoint].bufSize)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    /* Clear transfer completion notification */
    context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;

    /* Clear toggle for ISOC IN endpoint */
    if (CY_USBFS_DEV_DRV_EP_CR_ISO_IN == context->epPool[endpoint].sieMode)
    {
        context->epPool[endpoint].toggle = 0UL;
    }
    
    /* Set count and data toggle */
    Cy_USBFS_Dev_Drv_SetSieEpCount(base, endpoint, size, context->epPool[endpoint].toggle);
    
    if (0U == size)
    {
        /* Arm endpoint: endpoint ACK host request */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
    }
    else
    {
        if (context->useReg16)
        {
            uint32_t idx;

            /* Get pointer to the buffer */
            uint16_t *ptr = (uint16_t *) buffer;

            /* Copy data into the internal IP SRAM */
            for (idx = 0u; idx < GET_SIZE16(size); ++idx)
            {
                Cy_USBFS_Dev_Drv_WriteData16(base, endpoint, ptr[idx]);
            }
        }
        else
        {
            uint32_t idx;

            /* Copy data into the internal IP SRAM */
            for (idx = 0u; idx < size; ++idx)
            {
                Cy_USBFS_Dev_Drv_WriteData(base, endpoint, buffer[idx]);
            }
        }

        /* Arm endpoint: endpoint ACK host request */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: ReadOutEndpointCpu
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t ReadOutEndpointCpu(USBFS_Type *base,
                                                       uint32_t    endpoint,
                                                       uint8_t     buffer[],
                                                       uint32_t    size,
                                                       uint32_t   *actSize,
                                                       cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t idx;
    uint32_t numToCopy;

    /* Suppress a compiler warning about unused variables */
    (void) context;

    /* Get number of received bytes */
    numToCopy = Cy_USBFS_Dev_Drv_GetSieEpCount(base, endpoint);

    /* Endpoint received more bytes than provided buffer */
    if (numToCopy > size)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    /* Store number of copied bytes */
    *actSize = numToCopy;

    if (context->useReg16)
    {
        /* Get pointer to the buffer */
        uint16_t *ptr = (uint16_t *) buffer;

        /* Copy data from the internal IP SRAM */
        for (idx = 0U; idx < GET_SIZE16(numToCopy); ++idx)
        {
            ptr[idx] = Cy_USBFS_Dev_Drv_ReadData16(base, endpoint);
        }
    }
    else
    {
        /* Copy data from the internal IP SRAM */
        for (idx = 0U; idx < numToCopy; ++idx)
        {
            buffer[idx] = Cy_USBFS_Dev_Drv_ReadData(base, endpoint);
        }
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_StallEndpoint
****************************************************************************//**
*
* Configures data endpoint to STALL any request intended to it.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_StallEndpoint(USBFS_Type *base,
                                                            uint32_t    endpoint,
                                                            cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Check if endpoint is supported by driver */
    if (false == IS_EP_VALID(endpoint))
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    /* Check if endpoint is configured */
    if (CY_USB_DEV_EP_DISABLED == context->epPool[EP2PHY(endpoint)].state)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    endpoint = EP2PHY(endpoint);

    /* Store current endpoint state to restore it after Stall clear */
    context->epPool[endpoint].isPending = (CY_USB_DEV_EP_PENDING == context->epPool[endpoint].state);

    /* Stall endpoint */
    Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, CY_USBFS_DEV_DRV_EP_CR_STALL_INOUT);
    context->epPool[endpoint].state = CY_USB_DEV_EP_STALLED;

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_UnStallEndpoint
****************************************************************************//**
*
* Release data endpoint STALL condition and clears its state and data toggle.
* The endpoint is returned to the same state before it was STALLed.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* The data endpoint number to which the data should be transferred.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_UnStallEndpoint(USBFS_Type *base,
                                                              uint32_t    endpoint,
                                                              cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Check if endpoint is supported by driver */
    if (false == IS_EP_VALID(endpoint))
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    /* Check if endpoint is configured */
    if (CY_USB_DEV_EP_DISABLED == context->epPool[EP2PHY(endpoint)].state)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    endpoint = EP2PHY(endpoint);

    /* Clear toggle bit */
    context->epPool[endpoint].toggle = 0UL;

    if (context->epPool[endpoint].isPending)
    {
        /* Restore pending state */
        context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;

        /* Clear toggle and enable endpoint to respond to host */
        Cy_USBFS_Dev_Drv_ClearSieEpToggle(base, endpoint);
        Cy_USBFS_Dev_Drv_SetSieEpMode    (base, endpoint, context->epPool[endpoint].sieMode);
    }
    else
    {
        /* Endpoint is ready for operation */
        context->epPool[endpoint].state = CY_USB_DEV_EP_IDLE;

        /* Set endpoint inactive mode */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, GetEndpointInactiveMode(context->epPool[endpoint].sieMode));
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */


/* [] END OF FILE */
