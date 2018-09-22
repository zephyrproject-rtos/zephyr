/***************************************************************************//**
* \file cy_usbfs_dev_drv_io_dma.c
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

#include "cy_dma.h"
#include "cy_trigmux.h"

#include "cy_usbfs_dev_drv_pvt.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                        Internal Constants
*******************************************************************************/

/* Invalid channel */
#define DMA_INVALID_CHANNEL     ((uint32_t) (-1))

/* Arbiter interrupt sources for OUT and IN endpoints when mode is
* CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO.
*/
#define IN_ENDPOINT_ARB_INTR_SOURCES    (USBFS_USBDEV_ARB_EP_IN_BUF_FULL_Msk | \
                                         USBFS_USBDEV_ARB_EP_BUF_OVER_Msk    | \
                                         USBFS_USBDEV_ARB_EP_BUF_UNDER_Msk)

#define OUT_ENDPOINT_ARB_INTR_SOURCES   (USBFS_USBDEV_ARB_EP_DMA_TERMIN_Msk | \
                                         USBFS_USBDEV_ARB_EP_BUF_OVER_Msk   | \
                                         USBFS_USBDEV_ARB_EP_BUF_UNDER_Msk)

/* Dma configuration defines*/
#define DMA_XLOOP_INCREMENT    (1UL)
#define DMA_YLOOP_INCREMENT    (32UL)
#define DMA_NO_INCREMENT       (0UL)

/* Timeout for dynamic reconfiguration */
#define DYN_RECONFIG_ONE_TICK   (1UL)       /* 1 tick = 1 us */
#define DYN_RECONFIG_TIMEOUT    (25000UL)   /* (25000 * tick)us = 25 ms ( TDRQCMPLTND / 2 ) */

/* Timeout for DMA read operation */
#define DMA_READ_REQUEST_ONE_TICK   (1UL)     /* 1 tick = 1 us */
#define DMA_READ_REQUEST_TIMEOUT    (25000UL) /* (25000 * tick)us = 25 ms ( TDRQCMPLTND / 2 ) */


/*******************************************************************************
*                        Internal Functions Prototypes
*******************************************************************************/
static void DmaInEndpointInit (cy_stc_dma_descriptor_t* descr, 
                               cy_en_dma_data_size_t dataSize, 
                               void *dataReg);

static void DmaOutEndpointInit(cy_stc_dma_descriptor_t* descr, 
                               cy_en_dma_data_size_t dataSize, 
                               void *dataReg);

static void DmaOutEndpointInitComplete(cy_stc_usbfs_dev_drv_endpoint_data_t *endpoint);

static cy_en_usbfs_dev_drv_status_t ChangeEndpointDirection(USBFS_Type *base,
                                                            bool inDirection,
                                                            uint32_t endpoint);


/*******************************************************************************
*                            Internal Constants
*******************************************************************************/

/* Used for getting data in X loops */
static const cy_stc_dma_descriptor_config_t DmaDescr1DCfg =
{
    .retrigger      = CY_DMA_RETRIG_IM,
    .interruptType  = CY_DMA_DESCR,
    .triggerOutType = CY_DMA_DESCR,
    .triggerInType  = CY_DMA_DESCR,
    .channelState   = CY_DMA_CHANNEL_DISABLED,

    .dataSize        = CY_DMA_BYTE,
    .srcTransferSize = 0UL,
    .dstTransferSize = 0UL,
    .descriptorType  = CY_DMA_1D_TRANSFER,

    .srcAddress     = 0UL,
    .dstAddress     = 0UL,
    .srcXincrement  = 0UL,
    .dstXincrement  = 0UL,
    .xCount         = 1UL,
    .srcYincrement  = 0UL,
    .dstYincrement  = 0UL,
    .yCount         = 1UL,
    .nextDescriptor = NULL,
};

/* Used for getting data in Y loops */
static const cy_stc_dma_descriptor_config_t DmaDescr2DCfg =
{
    .retrigger      = CY_DMA_RETRIG_IM,
    .interruptType  = CY_DMA_X_LOOP,
    .triggerOutType = CY_DMA_X_LOOP,
    .triggerInType  = CY_DMA_X_LOOP,
    .channelState   = CY_DMA_CHANNEL_ENABLED,

    .dataSize        = CY_DMA_BYTE,
    .srcTransferSize = 0UL,
    .dstTransferSize = 0UL,
    .descriptorType  = CY_DMA_2D_TRANSFER,

    .srcAddress     = 0UL,
    .dstAddress     = 0UL,
    .srcXincrement  = 0UL,
    .dstXincrement  = 0UL,
    .xCount         = 1UL,
    .srcYincrement  = 0UL,
    .dstYincrement  = 0UL,
    .yCount         = 1UL,
    .nextDescriptor = NULL,
};


/*******************************************************************************
* Function Name: DmaInitAll
****************************************************************************//**
*
* Initialize all Dma channels utilized by the USBFS Device.
*
* \param config
* The pointer to the driver configuration structure \ref cy_stc_usbfs_dev_drv_config_t.
*
* \param context
* The pointer to the driver context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t DmaInit(cy_stc_usbfs_dev_drv_config_t const *config, cy_stc_usbfs_dev_drv_context_t *context)
{
    cy_en_usbfs_dev_drv_status_t retStatus = CY_USBFS_DEV_DRV_DMA_CFG_FAILED;

    uint32_t endpoint;

    /* Configure DMA descriptors and channels for data endpoints */
    for (endpoint = 0UL; endpoint < CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++endpoint)
    {
        /* Initialize pointers to user memcpy replacement */
        context->epPool[endpoint].userMemcpy = NULL;
        
        /* DMA configuration status for endpoint */
        retStatus = CY_USBFS_DEV_DRV_DMA_CFG_FAILED;

        if (config->dmaConfig[endpoint] != NULL)
        {
            cy_stc_dma_channel_config_t chConfig;

            /* Copy DMA config required for driver operation */
            context->epPool[endpoint].base   = config->dmaConfig[endpoint]->base;
            context->epPool[endpoint].chNum  = config->dmaConfig[endpoint]->chNum;
            context->epPool[endpoint].descr0 = config->dmaConfig[endpoint]->descr0;
            context->epPool[endpoint].descr1 = config->dmaConfig[endpoint]->descr1;
            
            context->epPool[endpoint].userMemcpy = NULL;

            if (context->mode == CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA)
            {
                cy_stc_dma_descriptor_t* descr0 = context->epPool[endpoint].descr0;

                /* Initialize DMA descriptor 0: 1D configuration */
                if (CY_DMA_SUCCESS != Cy_DMA_Descriptor_Init(descr0, &DmaDescr1DCfg))
                {
                    break;
                }

                /* Link descriptor to itself */
                Cy_DMA_Descriptor_SetNextDescriptor(descr0, descr0);
            }
            else
            {
                cy_stc_dma_descriptor_t* descr0 = context->epPool[endpoint].descr0;
                cy_stc_dma_descriptor_t* descr1 = context->epPool[endpoint].descr1;

                /* Copy DMA config required for driver operation */
                context->epPool[endpoint].outTrigMux  = config->dmaConfig[endpoint]->outTrigMux;

                /* Initialize Dma descriptors 0: 2D configuration */
                if (CY_DMA_SUCCESS != Cy_DMA_Descriptor_Init(descr0, &DmaDescr2DCfg))
                {
                    break;
                }

                /* Initialize DMA descriptors 1: 1D configuration */
                if (CY_DMA_SUCCESS != Cy_DMA_Descriptor_Init(descr1, &DmaDescr1DCfg))
                {
                    break;
                }

                Cy_DMA_Descriptor_SetNextDescriptor(descr0, descr1);
                Cy_DMA_Descriptor_SetNextDescriptor(descr1, descr0);
            }

            /* Set DMA channel configuration */
            chConfig.enable      = false;
            chConfig.bufferable  = false;
            chConfig.descriptor  = config->dmaConfig[endpoint]->descr0;
            chConfig.preemptable = config->dmaConfig[endpoint]->preemptable;
            chConfig.priority    = config->dmaConfig[endpoint]->priority;

            /* Initialize DMA channel */
            if (CY_DMA_SUCCESS != Cy_DMA_Channel_Init(context->epPool[endpoint].base,
                                                      context->epPool[endpoint].chNum,
                                                      &chConfig))
            {
                break;
            }

            /* Enable DMA block */
            Cy_DMA_Enable(context->epPool[endpoint].base);
        }
        else
        {
            context->epPool[endpoint].chNum = DMA_INVALID_CHANNEL;
        }

        /* Configuration complete successfully */
        retStatus = CY_USBFS_DEV_DRV_SUCCESS;
    }

    return retStatus;
}


/*******************************************************************************
* Function Name: DmaDisableAll
****************************************************************************//**
*
* TBD
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
void DmaDisable(cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint;

    /* Disable DMA channels for data endpoints */
    for (endpoint = 0UL; endpoint < CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++endpoint)
    {
        if (context->epPool[endpoint].chNum != DMA_INVALID_CHANNEL)
        {
            Cy_DMA_Channel_Disable(context->epPool[endpoint].base,
                                   context->epPool[endpoint].chNum);
        }
    }
}


/*******************************************************************************
* Function Name: DmaInEndpointInit
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpointAddr
* Data endpoint address (includes direction bit).
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.

*******************************************************************************/
static void DmaInEndpointInit(cy_stc_dma_descriptor_t* descr, cy_en_dma_data_size_t dataSize, void *dataReg)
{
    Cy_DMA_Descriptor_SetDataSize       (descr, dataSize);
    Cy_DMA_Descriptor_SetSrcTransferSize(descr, CY_DMA_TRANSFER_SIZE_DATA);
    Cy_DMA_Descriptor_SetDstTransferSize(descr, CY_DMA_TRANSFER_SIZE_WORD);

    Cy_DMA_Descriptor_SetXloopSrcIncrement(descr, DMA_XLOOP_INCREMENT);
    Cy_DMA_Descriptor_SetXloopDstIncrement(descr, DMA_NO_INCREMENT);

    Cy_DMA_Descriptor_SetDstAddress(descr, dataReg);
}


/*******************************************************************************
* Function Name: DmaOutEndpointInit
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpointAddr
* Data endpoint address (includes direction bit).
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.

*******************************************************************************/
static void DmaOutEndpointInit(cy_stc_dma_descriptor_t* descr, cy_en_dma_data_size_t dataSize, void *dataReg)
{
    Cy_DMA_Descriptor_SetDataSize       (descr, dataSize);
    Cy_DMA_Descriptor_SetSrcTransferSize(descr, CY_DMA_TRANSFER_SIZE_WORD);
    Cy_DMA_Descriptor_SetDstTransferSize(descr, CY_DMA_TRANSFER_SIZE_DATA);

    Cy_DMA_Descriptor_SetXloopSrcIncrement(descr, DMA_NO_INCREMENT);
    Cy_DMA_Descriptor_SetXloopDstIncrement(descr, DMA_XLOOP_INCREMENT);

    Cy_DMA_Descriptor_SetSrcAddress(descr, dataReg);
}


/*******************************************************************************
* Function Name: DmaOutEndpointInitComplete
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \return
* TBD
*
*******************************************************************************/
static void DmaOutEndpointInitComplete(cy_stc_usbfs_dev_drv_endpoint_data_t *endpoint)
{
    uint8_t *buffer = endpoint->buffer;
    uint32_t size   = endpoint->bufSize;
    
    /*
    * Descriptor 0: get number of Y loops. It transfers data in multiples of 32 bytes.
    * Descriptor 1: get number of X loops. It transfers data what was left 1-31 bytes.
    */
    uint32_t numYloops = size / DMA_YLOOP_INCREMENT;
    uint32_t numXloops = size % DMA_YLOOP_INCREMENT;

    /* Store pointer to destination to be used in Cy_USBFS_Dev_Drv_ReadOutEpDmaAuto */
    Cy_DMA_Descriptor_SetDstAddress(endpoint->descr0, (const void*) buffer);
                    
    if (numYloops > 0UL)
    {
        /* Descriptor 0: Configure destination address and  number of Y loops */
        Cy_DMA_Descriptor_SetYloopDataCount(endpoint->descr0, numYloops);
    }

    if (numXloops > 0UL)
    {
        /* Descriptor 1: Configure destination address and X loop data count */
        Cy_DMA_Descriptor_SetDstAddress    (endpoint->descr1, (const void*) &buffer[size - numXloops]);
        Cy_DMA_Descriptor_SetXloopDataCount(endpoint->descr1, numXloops);
    }

    /* Set Descriptor 0 channel state to enable to mark it as 1st descriptor */
    Cy_DMA_Descriptor_SetChannelState(endpoint->descr0,((numYloops > 0UL) ? 
                                                         CY_DMA_CHANNEL_ENABLED : CY_DMA_CHANNEL_DISABLED));

    /* Start execution from Descriptor 0 (length >= 32) or Descriptor 1 (length < 32) */
    Cy_DMA_Channel_SetDescriptor(endpoint->base, endpoint->chNum,
                                ((numYloops > 0UL) ? endpoint->descr0 : endpoint->descr1));
    
    /* Enable channel: configuration complete */
    Cy_DMA_Channel_Enable(endpoint->base, endpoint->chNum);
}


/*******************************************************************************
* Function Name: DmaEndpointInit
****************************************************************************//**
*
* TBD.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpointAddr
* Data endpoint address (includes direction bit).
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \return
* TBD
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t DmaEndpointInit(USBFS_Type *base,
                                             uint32_t    endpointAddr,
                                             cy_stc_usbfs_dev_drv_context_t *context)
{
    cy_stc_dma_descriptor_t* descr;
    cy_en_dma_data_size_t dataSize;
    uint32_t yLoopCount;
    void *dataReg;

    uint32_t endpoint = EPADDR2PHY(endpointAddr);

    if (DMA_INVALID_CHANNEL == context->epPool[endpoint].chNum)
    {
        return CY_USBFS_DEV_DRV_DMA_CFG_FAILED;
    }

    /* Set configuration variables depends access type */
    if (context->useReg16)
    {
        dataReg    = Cy_USBFS_Dev_Drv_GetDataReg16Addr(base, endpoint);
        dataSize   = CY_DMA_HALFWORD;
        yLoopCount = (DMA_YLOOP_INCREMENT / 2UL);
    }
    else
    {
        dataReg    = Cy_USBFS_Dev_Drv_GetDataRegAddr(base, endpoint);
        dataSize   = CY_DMA_BYTE;
        yLoopCount = DMA_YLOOP_INCREMENT;
    }

    /* Disable channel before configuration */
    Cy_DMA_Channel_Disable(context->epPool[endpoint].base,
                           context->epPool[endpoint].chNum);


    /* Get DMA descriptor associated with endpoint */
    descr = context->epPool[endpoint].descr0;
    
    if (IS_EP_DIR_IN(endpointAddr))
    {
        /* Configure DMA for IN endpoint */
        
        /* Descriptor 0 (1D): DMA reads from array (uint8_t) and write into register (reg8 or reg16) */
        DmaInEndpointInit(descr, dataSize, dataReg);

        if (CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO == context->mode)
        {
            /* Descriptor 0 (2D): it reads 32 bytes (X loop) and increments by 32 bytes (Y loop)
            * When access 16-bits registers, the 2 bytes are transfered in one X loop, so
            * increment is divided by 2.
            */
            Cy_DMA_Descriptor_SetXloopDataCount(descr, yLoopCount);

            Cy_DMA_Descriptor_SetYloopSrcIncrement(descr, yLoopCount);
            Cy_DMA_Descriptor_SetYloopDstIncrement(descr, DMA_NO_INCREMENT);

            /* Descriptor 1 (1D): DMA reads from array (uint8_t) and write into register (reg8 or reg16) */
            descr = context->epPool[endpoint].descr1;

            /* Configure base DMA for IN endpoint */
            DmaInEndpointInit(descr, dataSize, dataReg);
        }
    }
    else 
    {
        /* Configure DMA for OUT endpoint */

        /* Descriptor 0 (1D): DMA reads from register (reg8 or reg16) and writes to array (uint8_t) */
        DmaOutEndpointInit(descr, dataSize, dataReg);

        if (CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO == context->mode)
        {
            /* Descriptor 0 (2D): it reads 32 bytes (X loop) and increments by 32 bytes (Y loop)
            * When access 16-bits registers, the 2 bytes are transfered in one X loop, so
            * increment is divided by 2.
            */
            Cy_DMA_Descriptor_SetXloopDataCount(descr, yLoopCount);

            Cy_DMA_Descriptor_SetYloopSrcIncrement(descr, DMA_NO_INCREMENT);
            Cy_DMA_Descriptor_SetYloopDstIncrement(descr, yLoopCount);

            
            descr = context->epPool[endpoint].descr1;
            
            /* Descriptor 1 (1D): DMA reads from register (reg8 or reg16) and writes to array (uint8_t) */
            DmaOutEndpointInit(descr, dataSize, dataReg);

            /* Complete DMA configuration based on endpoint size */
            DmaOutEndpointInitComplete(&context->epPool[endpoint]);
        }
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


            
/*******************************************************************************
* Function Name: DmaOutEndpointRestore
****************************************************************************//**
*   
* TBD
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
void DmaOutEndpointRestore(cy_stc_usbfs_dev_drv_endpoint_data_t *endpoint)
{
    /* Number of clocks for Dma completion pulse */
    const uint32_t CY_USBFS_DEV_DRV_TRIG_CYCLES = 2UL;

    /* Define which descriptor is 1st in the chain */
    bool setDescr0 = (CY_DMA_CHANNEL_ENABLED == Cy_DMA_Descriptor_GetChannelState(endpoint->descr0));

    /* Channel disable aborts on-going transfer */
    Cy_DMA_Channel_Disable(endpoint->base, endpoint->chNum);

    /* Send pulse to UBSFS IP to indicate end of Dma transfer */
    (void) Cy_TrigMux_SwTrigger(endpoint->outTrigMux, CY_USBFS_DEV_DRV_TRIG_CYCLES);

    /* Set 1st Dma descriptor for the following transfer */
    Cy_DMA_Channel_SetDescriptor(endpoint->base, endpoint->chNum,
                                                (setDescr0 ? endpoint->descr0 : endpoint->descr1));

    Cy_DMA_Channel_Enable(endpoint->base, endpoint->chNum);
}


/*******************************************************************************
* Function Name: ChangeEndpointDirection
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
static cy_en_usbfs_dev_drv_status_t ChangeEndpointDirection(USBFS_Type *base,
                                                            bool        inDirection,
                                                            uint32_t    endpoint)
{
    cy_en_usbfs_dev_drv_status_t drvStatus = CY_USBFS_DEV_DRV_EP_DYN_RECONFIG_TIMEOUT;

    uint32_t timeout = DYN_RECONFIG_TIMEOUT;

    /* Only enabled endpoint can be re-configured */
    if (0U == (base->USBDEV.EP_ACTIVE & EP2MASK(endpont)))
    {
        return CY_USBFS_DEV_DRV_BUF_ALLOC_FAILED;
    }

    /* Clear pre-fetch request */
    Cy_USBFS_Dev_Drv_ClearArbCfgEpInReady(base, endpoint);

    /* Request reconfiguration for endpoint */
    base->USBDEV.DYN_RECONFIG = (USBFS_USBDEV_DYN_RECONFIG_EN_Msk |
                        _VAL2FLD(USBFS_USBDEV_DYN_RECONFIG_EPNO, endpoint));

    /* Cypress ID#295549: execute dummy read */
    (void) base->USBDEV.DYN_RECONFIG;

    /* Wait for dynamic re-configuration completion */
    while ((0U == (base->USBDEV.DYN_RECONFIG & USBFS_USBDEV_DYN_RECONFIG_RDY_STS_Msk)) &&
           (timeout > 0U))
    {
        Cy_SysLib_DelayUs(DYN_RECONFIG_ONE_TICK);
        --timeout;
    }

    /* Verify operation result */
    if (timeout > 0U)
    {
        Cy_USBFS_Dev_Drv_SetEpType(base, inDirection, endpoint);
        drvStatus = CY_USBFS_DEV_DRV_SUCCESS;
    }

    /* Clear reconfiguration register */
    base->USBDEV.DYN_RECONFIG = 0UL;

    return drvStatus;
}


/*******************************************************************************
* Function Name: AddEndpointRamBuffer
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
cy_en_usbfs_dev_drv_status_t AddEndpointRamBuffer(USBFS_Type *base,
                                                  cy_stc_usb_dev_ep_config_t const *config,
                                                  cy_stc_usbfs_dev_drv_context_t     *context)
{
    cy_en_usbfs_dev_drv_status_t drvStatus = CY_USBFS_DEV_DRV_BAD_PARAM;

    uint32_t endpoint = EPADDR2PHY(config->endpointAddr);

    /* Get buffer for endpoint using buffer allocated by the user */
    if (config->allocBuffer)
    {
        /* Configure active endpoint */
        base->USBDEV.EP_ACTIVE |= EP2MASK(endpont);
        
        /* Get buffer for OUT endpoint */
        if (0U != config->outBufferSize)
        {
            uint32_t startBufIdx;
            
            drvStatus = GetEndpointBuffer(config->outBufferSize, &startBufIdx, context);
            if (CY_USBFS_DEV_DRV_SUCCESS != drvStatus)
            {
                return drvStatus;
            }

            /* Store pointer to buffer for this endpoint */
            context->epPool[endpoint].buffer = &context->epSharedBuf[startBufIdx];
        }
    }

    /* Enable endpoint for operation */
    if (config->enableEndpoint)
    {
        bool inDirection = IS_EP_DIR_IN(config->endpointAddr);
        
        /* Check endpoint direction change: IN->OUT or OUT->IN */
        bool changeDirection = (context->epPool[endpoint].address != config->endpointAddr);        

        /* Clear variables related to endpoint */
        context->epPool[endpoint].state   = CY_USB_DEV_EP_IDLE;
        context->epPool[endpoint].address = config->endpointAddr;
        context->epPool[endpoint].toggle  = 0UL;
        context->epPool[endpoint].bufSize = config->maxPacketSize;
        context->epPool[endpoint].sieMode = GetEndpointActiveMode(inDirection, config->attributes);
        context->epPool[endpoint].isPending = false;
            
        /* Handle direction change */
        if (changeDirection)
        {
            if (config->allocBuffer)
            {
                /* Configure endpoint type: OUT - 1, IN - 0 */
                Cy_USBFS_Dev_Drv_SetEpType(base, IS_EP_DIR_IN(config->endpointAddr), endpoint);                
            }
            else
            {
                /* Reconfigure endpoint direction dynamically */
                drvStatus = ChangeEndpointDirection(base, inDirection, endpoint);
                if (CY_USBFS_DEV_DRV_SUCCESS != drvStatus)
                {
                    return drvStatus;
                }
            }
        
            /* Configure DMA direction */
            drvStatus = DmaEndpointInit(base, config->endpointAddr, context);
            if (CY_USBFS_DEV_DRV_SUCCESS != drvStatus)
            {
                return drvStatus;
            }
            
            /* Enable Arbiter interrupt sources for endpoint */
            Cy_USBFS_Dev_Drv_SetArbEpInterruptMask(base, endpoint,(inDirection ?
                                                                 IN_ENDPOINT_ARB_INTR_SOURCES :
                                                                OUT_ENDPOINT_ARB_INTR_SOURCES));
        }
        
        /* Enable SIE and arbiter interrupt for endpoint */
        Cy_USBFS_Dev_Drv_EnableSieEpInterrupt(base, endpoint);
        Cy_USBFS_Dev_Drv_EnableArbEpInterrupt(base, endpoint);

        /* Set SIE mode to respond to host */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, GetEndpointInactiveMode(context->epPool[endpoint].sieMode));
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: LoadInEndpointDma
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
cy_en_usbfs_dev_drv_status_t LoadInEndpointDma(USBFS_Type   *base,
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

    /* Clear toggle for ISOC IN endpoint */
    if (CY_USBFS_DEV_DRV_EP_CR_ISO_IN == context->epPool[endpoint].sieMode)
    {
        context->epPool[endpoint].toggle = 0UL;
    }
    
    /* Set count and data toggle */
    Cy_USBFS_Dev_Drv_SetSieEpCount(base, endpoint, size, context->epPool[endpoint].toggle);

    /* Clear transfer complete notification */
    context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;

    if (0U == size)
    {
        /* Arm endpoint: host is allowed to read data */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
    }
    else
    {
        /* Channel is disabled after initialization or descriptor completion  */

        /* Get number of data elements to transfer */
        size = context->useReg16 ? GET_SIZE16(size) : size;

        /* 1D descriptor: configure destination address and length */
        Cy_DMA_Descriptor_SetSrcAddress    (context->epPool[endpoint].descr0, (const void*) buffer);
        Cy_DMA_Descriptor_SetXloopDataCount(context->epPool[endpoint].descr0, size);

        /* Enable DMA channel: configuration complete */
        Cy_DMA_Channel_Enable(context->epPool[endpoint].base,
                              context->epPool[endpoint].chNum);

        /* Generate DMA request: the endpoint will be armed when DMA done in Arbiter interrupt */
        Cy_USBFS_Dev_Drv_TriggerArbCfgEpDmaReq(base, endpoint);
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: ReadOutEndpointDma
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
cy_en_usbfs_dev_drv_status_t ReadOutEndpointDma(USBFS_Type *base,
                                                uint32_t    endpoint,
                                                uint8_t     buffer[],
                                                uint32_t    size,
                                                uint32_t   *actSize,
                                                cy_stc_usbfs_dev_drv_context_t *context)
{
    cy_en_usbfs_dev_drv_status_t drvStatus = CY_USBFS_DEV_DRV_EP_DMA_READ_TIMEOUT;

    uint32_t timeout = DMA_READ_REQUEST_TIMEOUT;

    /* Get number of received bytes */
    uint32_t  numToCopy = Cy_USBFS_Dev_Drv_GetSieEpCount(base, endpoint);

    /* Endpoint received more bytes than provided buffer */
    if (numToCopy > size)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    /* Store number of copied bytes */
    *actSize = numToCopy;

    /* Channel is disabled after initialization or descriptor completion  */

    /* Get number of data elements to transfer */
    numToCopy = context->useReg16 ? GET_SIZE16(numToCopy) : numToCopy;

    /* 1D descriptor: configure destination address and length */
    Cy_DMA_Descriptor_SetDstAddress    (context->epPool[endpoint].descr0, (const void*) buffer);
    Cy_DMA_Descriptor_SetXloopDataCount(context->epPool[endpoint].descr0, numToCopy);

    /* Enable DMA channel: configuration complete */
    Cy_DMA_Channel_Enable(context->epPool[endpoint].base,
                          context->epPool[endpoint].chNum);

    /* Clear completion to track DMA read complete */
    context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;

    /* Generate DMA request to read data from hardware buffer */
    Cy_USBFS_Dev_Drv_TriggerArbCfgEpDmaReq(base, endpoint);

    /* Wait until DMA complete reading */
    while ((CY_USB_DEV_EP_PENDING == context->epPool[endpoint].state) && 
           (timeout > 0U))
    {
        Cy_SysLib_DelayUs(DMA_READ_REQUEST_ONE_TICK);
        --timeout;
    }

    /* Check timeout */
    if (timeout > 0U)
    {
        drvStatus = CY_USBFS_DEV_DRV_SUCCESS;
    }

    return drvStatus;
}


/*******************************************************************************
* Function Name: LoadInEndpointDmaAuto
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
cy_en_usbfs_dev_drv_status_t LoadInEndpointDmaAuto(USBFS_Type   *base,
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

    /* Clear toggle for ISOC IN endpoint */
    if (CY_USBFS_DEV_DRV_EP_CR_ISO_IN == context->epPool[endpoint].sieMode)
    {
        context->epPool[endpoint].toggle = 0UL;
    }
    
    /* Set count and data toggle */
    Cy_USBFS_Dev_Drv_SetSieEpCount(base, endpoint, size, context->epPool[endpoint].toggle);

    /* Clear transfer completion */
    context->epPool[endpoint].state = CY_USB_DEV_EP_PENDING;

    if (0U == size)
    {
        /* Arm endpoint: host is allowed to read data */
        Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
    }
    else
    {
        /* Channel is disabled after initialization or transfer completion */

        /*
        * Descriptor 0: get number of Y loops. It transfers data in multiples of 32 bytes.
        * Descriptor 1: get number of X loops. It transfers data remaining data 1-31 bytes.
        */
        uint32_t numXloops = size % DMA_YLOOP_INCREMENT;
        uint32_t numYloops = size / DMA_YLOOP_INCREMENT;

        if (numYloops > 0UL)
        {
            /* Descriptor 0: configure number of Y loops and source to get data from */
            Cy_DMA_Descriptor_SetSrcAddress    (context->epPool[endpoint].descr0, buffer);
            Cy_DMA_Descriptor_SetYloopDataCount(context->epPool[endpoint].descr0, numYloops);
        }

        if (numXloops > 0UL)
        {
            /* Descriptor 1: configure number of X loops and source to get data from */
            Cy_DMA_Descriptor_SetSrcAddress    (context->epPool[endpoint].descr1, &buffer[size - numXloops]);
            Cy_DMA_Descriptor_SetXloopDataCount(context->epPool[endpoint].descr1, numXloops);
        }

        /* Keep channel enabled after execution of Descriptor 0 to execute Descriptor */
        Cy_DMA_Descriptor_SetChannelState(context->epPool[endpoint].descr0,
                                           ((numYloops > 0UL) && (numXloops > 0UL)) ?
                                                CY_DMA_CHANNEL_ENABLED : CY_DMA_CHANNEL_DISABLED);

        /* Start execution from Descriptor 0 (length >= 32) or Descriptor 1 (length < 32) */
        Cy_DMA_Channel_SetDescriptor(context->epPool[endpoint].base,
                                     context->epPool[endpoint].chNum,
                                     ((numYloops > 0UL) ? context->epPool[endpoint].descr0 :
                                                          context->epPool[endpoint].descr1));

        /* Configuration complete */
        Cy_DMA_Channel_Enable(context->epPool[endpoint].base,
                              context->epPool[endpoint].chNum);

        /* Generate DMA request to pre-load data into endpoint buffer */
        Cy_USBFS_Dev_Drv_SetArbCfgEpInReady(base, endpoint);

        /* IN endpoint will be armed in the Arbiter interrupt (source: IN_BUF_FULL)
        * after DMA pre-load data buffer.
        */
    }

    return CY_USBFS_DEV_DRV_SUCCESS;
}


/*******************************************************************************
* Function Name: ReadOutEndpointDmaAuto
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
cy_en_usbfs_dev_drv_status_t ReadOutEndpointDmaAuto(USBFS_Type *base,
                                                    uint32_t    endpoint,
                                                    uint8_t     buffer[],
                                                    uint32_t    size,
                                                    uint32_t   *actSize,
                                                    cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Get destination address */
    void * dstAddr = Cy_DMA_Descriptor_GetDstAddress(context->epPool[endpoint].descr0);
    
    /* Get number of received bytes */
    uint32_t  numToCopy = Cy_USBFS_Dev_Drv_GetSieEpCount(base, endpoint);
    

    /* Endpoint received more bytes than provided buffer */
    if (numToCopy > size)
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }
    
    if (NULL == context->epPool[endpoint].userMemcpy)
    {
        /* Copy data into the user provided buffer */
        memcpy(buffer, dstAddr, numToCopy);
    }
    else
    {
        context->epPool[endpoint].userMemcpy(buffer, dstAddr, numToCopy);
    }
    
    *actSize = numToCopy;

    return CY_USBFS_DEV_DRV_SUCCESS;
}


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */


/* [] END OF FILE */
