/***************************************************************************//**
* \file cy_usbfs_dev_drv_pvt.h
* \version 1.0
*
* Provides API declarations of the USBFS driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_USBFS_DEV_DRV_PVT_H)
#define CY_USBFS_DEV_DRV_PVT_H

#include "cy_usbfs_dev_drv.h"
#include "cy_usbfs_dev_drv_reg.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                              Function Prototypes
*******************************************************************************/

/** \cond INTERNAL */
__STATIC_INLINE uint32_t GetEndpointActiveMode(uint32_t inDirection, uint32_t attributes);
__STATIC_INLINE uint32_t GetEndpointInactiveMode(uint32_t mode);

/* Dma configuration functions */
cy_en_usbfs_dev_drv_status_t DmaInit(cy_stc_usbfs_dev_drv_config_t const *config,
                                     cy_stc_usbfs_dev_drv_context_t      *context);

                        void DmaDisable(cy_stc_usbfs_dev_drv_context_t *context);

                        void DmaOutEndpointRestore(cy_stc_usbfs_dev_drv_endpoint_data_t *endpoint);
                        
cy_en_usbfs_dev_drv_status_t DmaEndpointInit(USBFS_Type *base,
                                             uint32_t    endpointAddr,
                                             cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t GetEndpointBuffer(uint32_t size, 
                                               uint32_t *idx, 
                                               cy_stc_usbfs_dev_drv_context_t *context);

/* Endpoint buffer allocation functions (driver specific) */
cy_en_usbfs_dev_drv_status_t AddEndpointHwBuffer(USBFS_Type *base,
                                                 cy_stc_usb_dev_ep_config_t const *config,
                                                 cy_stc_usbfs_dev_drv_context_t   *context);

cy_en_usbfs_dev_drv_status_t AddEndpointRamBuffer(USBFS_Type *base,
                                                  cy_stc_usb_dev_ep_config_t const *config,
                                                  cy_stc_usbfs_dev_drv_context_t   *context);

/* Endpoint buffer allocation functions (driver specific) */
cy_en_usbfs_dev_drv_status_t LoadInEndpointCpu(USBFS_Type   *base,
                                               uint32_t      endpoint,
                                               const uint8_t buffer[],
                                               uint32_t      size,
                                               cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t LoadInEndpointDma(USBFS_Type   *base,
                                               uint32_t      endpoint,
                                               const uint8_t buffer[],
                                               uint32_t      size,
                                               cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t LoadInEndpointDmaAuto(USBFS_Type   *base,
                                                   uint32_t      endpoint,
                                                   const uint8_t buffer[],
                                                   uint32_t      size,
                                                   cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t ReadOutEndpointCpu(USBFS_Type *base,
                                                uint32_t    endpoint,
                                                uint8_t     buffer[],
                                                uint32_t    size,
                                                uint32_t   *actSize,
                                                           cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t ReadOutEndpointDma(USBFS_Type *base,
                                                uint32_t    endpoint,
                                                uint8_t     buffer[],
                                                uint32_t    size,
                                                uint32_t   *actSize,
                                                cy_stc_usbfs_dev_drv_context_t *context);

cy_en_usbfs_dev_drv_status_t ReadOutEndpointDmaAuto(USBFS_Type *base,
                                                    uint32_t    endpoint,
                                                    uint8_t     buffer[],
                                                    uint32_t    size,
                                                    uint32_t   *actSize,
                                                    cy_stc_usbfs_dev_drv_context_t *context);
/** \endcond */


/*******************************************************************************
*                              Internal Constants
*******************************************************************************/

/** \cond INTERNAL */

/* Endpoint attributes */
#define ENDPOINT_ATTR_MASK    (0x03UL)
#define ENDPOINT_TYPE_ISOC    (0x01UL)

/* Returns size to access 16-bit registers */
#define GET_SIZE16(size)          (((size) / 2U) + ((size) & 0x01U))

/* Conversion macros */
#define IS_EP_VALID(endpoint)       CY_USBFS_DEV_DRV_IS_EP_VALID(endpoint)
#define EP2PHY(endpoint)            CY_USBFS_DEV_DRV_EP2PHY(endpoint)
#define EP2MASK(endpont)            CY_USBFS_DEV_DRV_EP2MASK(endpoint)

#define EPADDR2EP(endpointAddr)     CY_USBFS_DEV_DRV_EPADDR2EP(endpointAddr)
#define EPADDR2PHY(endpointAddr)    CY_USBFS_DEV_DRV_EPADDR2PHY(endpointAddr)
#define IS_EP_DIR_IN(endpointAddr)  CY_USBFS_DEV_DRV_IS_EP_DIR_IN(endpointAddr)
#define IS_EP_DIR_OUT(endpointAddr) CY_USBFS_DEV_DRV_IS_EP_DIR_OUT(endpointAddr)
/** \endcond */

/** \} group_usbfs_drv_macros */


/*******************************************************************************
*                         In-line Function Implementation
*******************************************************************************/
/** \cond INTERNAL */

/*******************************************************************************
* Function Name: GetEndpointActiveMode
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
__STATIC_INLINE uint32_t GetEndpointActiveMode(uint32_t inDirection, uint32_t attributes)
{
    uint32_t mode;

    if (ENDPOINT_TYPE_ISOC == (attributes & ENDPOINT_ATTR_MASK))
    {
        mode = (inDirection) ? CY_USBFS_DEV_DRV_EP_CR_ISO_IN : CY_USBFS_DEV_DRV_EP_CR_ISO_OUT;
    }
    else
    {
        mode = (inDirection) ? CY_USBFS_DEV_DRV_EP_CR_ACK_IN : CY_USBFS_DEV_DRV_EP_CR_ACK_OUT;
    }

    return mode;
}


/*******************************************************************************
* Function Name: GetEndpointInactiveMode
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
__STATIC_INLINE uint32_t GetEndpointInactiveMode(uint32_t mode)
{
    switch(mode)
    {
        case CY_USBFS_DEV_DRV_EP_CR_ACK_IN:
            mode = CY_USBFS_DEV_DRV_EP_CR_NAK_IN;
        break;

        case CY_USBFS_DEV_DRV_EP_CR_ACK_OUT:
            mode = CY_USBFS_DEV_DRV_EP_CR_NAK_OUT;
        break;

        case CY_USBFS_DEV_DRV_EP_CR_ISO_OUT:
        case CY_USBFS_DEV_DRV_EP_CR_ISO_IN:
            /* Return mode parameter for ISOC because they are always ready */
        break;

        default:
            /* Unknown mode: disable endpoint */
            mode = CY_USBFS_DEV_DRV_EP_CR_DISABLE;
        break;
    }

    return mode;
}

/** \endcond */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */

#endif /* (CY_USBFS_DEV_DRV_PRIV_H) */


/* [] END OF FILE */
