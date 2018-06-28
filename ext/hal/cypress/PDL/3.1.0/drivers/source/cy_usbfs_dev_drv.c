/***************************************************************************//**
* \file cy_usbfs_dev_drv.c
* \version 1.0
*
* Provides general API implementation of the USBFS driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_usbfs_dev_drv.h"
#include "cy_usbfs_dev_drv_pvt.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                        Internal Constants
*******************************************************************************/

/* Wait 2us: number re-used from MKAD-172 */
#define WAIT_SUSPEND_DISABLE    (2UL)

/* The bus reset counter is driven by 100kHz clock and detects bus reset
* condition after 100 us.
*/
#define BUS_RESET_PERIOD        (10UL)


/*******************************************************************************
*                        Internal Functions Prototypes
*******************************************************************************/

static void LpmIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
static void SofIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
static void Ep0IntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
static void BusResetIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
static void ArbiterIntrHandler (USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context);
static void SieEnpointIntrHandler(USBFS_Type *base, uint32_t endpoint, cy_stc_usbfs_dev_drv_context_t *context);

static uint32_t WriteEp0Buffer(USBFS_Type *base, uint8_t *buf, uint32_t size);
static uint32_t ReadEp0Buffer(USBFS_Type *base, uint8_t *buf, uint32_t size);


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Init
****************************************************************************//**
*
* Initializes the USBFS in device mode. If DMAs are used, initialize the DMAs.
*
* \param base
* The pointer to the USBFS instance.
*
* \param config
* The pointer to the configuration structure \ref cy_stc_usbfs_dev_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \return
* Status of executed operation \ref cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
cy_en_usbfs_dev_drv_status_t Cy_USBFS_Dev_Drv_Init(USBFS_Type *base, 
                                                   cy_stc_usbfs_dev_drv_config_t const *config,
                                                   cy_stc_usbfs_dev_drv_context_t      *context)
{
    cy_en_usbfs_dev_drv_status_t retStatus = CY_USBFS_DEV_DRV_BAD_PARAM;

    /* Input parameters verification */
    if ((NULL == base) || (NULL == config) || (NULL == context))
    {
        return CY_USBFS_DEV_DRV_BAD_PARAM;
    }

    CY_ASSERT_L3(CY_USBFS_DEV_DRV_IS_MODE_VALID(config->mode));

    /* Enable clock to mxusb IP */
    base->USBDEV.USB_CLK_EN = CY_USBFS_DEV_DRV_WRITE_ODD(USBFS_USBDEV_USB_CLK_EN_CSR_CLK_EN_Msk);

    /* Clear register (except reserved): the IOMODE = 0 means usb IP controls the usb pins */
    base->USBDEV.USBIO_CR1 = (base->USBDEV.USBIO_CR1 & USBFS_USBDEV_USBIO_CR1_RESERVED_2_Msk);

    /* Set number of clocks (divided version of Clk_Peri) to detect bus reset */
    base->USBDEV.BUS_RST_CNT = BUS_RESET_PERIOD;

    /* Enable PHY detector and single-ended and differential receivers */
    base->USBLPM.POWER_CTL = (USBFS_USBLPM_POWER_CTL_SUSPEND_Msk    |
                              USBFS_USBLPM_POWER_CTL_ENABLE_DPO_Msk |
                              USBFS_USBLPM_POWER_CTL_ENABLE_DMO_Msk);

    /* Push bufferable write to execute */
    (void) base->USBLPM.POWER_CTL;

    /* Suspend clear sequence */
    Cy_SysLib_DelayUs(WAIT_SUSPEND_DISABLE);
    base->USBLPM.POWER_CTL &= ~USBFS_USBLPM_POWER_CTL_SUSPEND_Msk;

    /* Push bufferable write to execute */
    (void) base->USBLPM.POWER_CTL;

    /* Clear register (except reserved) and enable IMO lock */
    base->USBDEV.CR1 = USBFS_USBDEV_CR1_ENABLE_LOCK_Msk |
                      (base->USBDEV.CR1 & USBFS_USBDEV_CR1_RESERVED_3_Msk);

    /* Configure level selection (HI, MED, LO) for each interrupt source */
    base->USBLPM.INTR_LVL_SEL = config->intrLevelSel;

    /* Enable interrupt sources: Bus Reset and EP0 */
    base->USBLPM.INTR_SIE_MASK = (USBFS_USBLPM_INTR_SIE_BUS_RESET_INTR_Msk |
                                  USBFS_USBLPM_INTR_SIE_EP0_INTR_Msk);

    if (config->enableLmp)
    {
        /* Enable device to ACK LMP requests */
        base->USBLPM.LPM_CTL = (USBFS_USBLPM_LPM_CTL_LPM_EN_Msk |
                                USBFS_USBLPM_LPM_CTL_LPM_ACK_RESP_Msk);
    }

    /* Copy configuration in the context */
    context->mode     = config->mode;
    context->useReg16 = (config->epAccess == CY_USBFS_DEV_DRV_USE_16_BITS_DR);
    context->epSharedBuf     = config->epBuffer;
    context->epSharedBufSize = config->epBufferSize;

    /* Initialize pointers to functions which works with data endpoint */
    switch(config->mode)
    {
        case CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU:
            context->addEndpoint     = &AddEndpointHwBuffer;
            context->loadInEndpoint  = &LoadInEndpointCpu;
            context->readOutEndpoint = &ReadOutEndpointCpu;

            base->USBDEV.ARB_CFG = _VAL2FLD(USBFS_USBDEV_ARB_CFG_DMA_CFG,
                                            CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU);

            retStatus = CY_USBFS_DEV_DRV_SUCCESS;
        break;

        case CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA:
            context->addEndpoint     = &AddEndpointHwBuffer;
            context->loadInEndpoint  = &LoadInEndpointDma;
            context->readOutEndpoint = &ReadOutEndpointDma;

            base->USBDEV.ARB_CFG = _VAL2FLD(USBFS_USBDEV_ARB_CFG_DMA_CFG,
                                            CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA);
        break;

        case CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO:
            context->addEndpoint     = &AddEndpointRamBuffer;
            context->loadInEndpoint  = &LoadInEndpointDmaAuto;
            context->readOutEndpoint = &ReadOutEndpointDmaAuto;

            base->USBDEV.ARB_CFG = (_VAL2FLD(USBFS_USBDEV_ARB_CFG_DMA_CFG,
                                             CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO) |
                                             USBFS_USBDEV_ARB_CFG_AUTO_MEM_Msk);
        break;

        default:
            break;
    }

    /* Configure DMA and store info about DMA channels */
    if (context->mode != CY_USBFS_DEV_DRV_EP_MANAGEMENT_CPU)
    {
        retStatus = DmaInit(config, context);   
    }

    return retStatus;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_DeInit
****************************************************************************//**
*
* Deinit UBSFS hardware.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_DeInit(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t i;
    uint32_t regVal;
    cy_stc_usbfs_dev_drv_acces_sie_t  *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    /* Set USBLPM registers into the default state */
    base->USBLPM.POWER_CTL     = 0UL;
    base->USBLPM.USBIO_CTL     = 0UL;
    base->USBLPM.FLOW_CTL      = 0UL;
    base->USBLPM.LPM_CTL       = 0UL;
    base->USBLPM.INTR_SIE      = 0UL;
    base->USBLPM.INTR_SIE_SET  = 0UL;
    base->USBLPM.INTR_SIE_MASK = 0UL;
    base->USBLPM.INTR_LVL_SEL  = 0UL;

    /* Set USBDEV registers into the default state */
    base->USBDEV.CR0 = 0UL;
    /* Do not touch reserved bits */
    base->USBDEV.CR1 = (base->USBDEV.CR1 & USBFS_USBDEV_CR1_RESERVED_3_Msk);

    base->USBDEV.USBIO_CR0 = 0UL;
    /* Do not touch reserved bits */
    regVal = CY_USBFS_DEV_READ_ODD(base->USBDEV.USBIO_CR2);
    base->USBDEV.USBIO_CR2 = CY_USBFS_DEV_DRV_WRITE_ODD(regVal) & USBFS_USBDEV_USBIO_CR2_RESERVED_7_Msk; 
    base->USBDEV.USBIO_CR1 = USBFS_USBDEV_USBIO_CR1_IOMODE_Msk;

    base->USBDEV.BUS_RST_CNT = BUS_RESET_PERIOD;
    base->USBDEV.USB_CLK_EN  = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);

    base->USBDEV.EP0_CR  = 0UL;
    base->USBDEV.EP0_CNT = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);

    base->USBDEV.ARB_CFG    = 0UL;
    base->USBDEV.ARB_INT_EN = 0UL;

    base->USBDEV.DYN_RECONFIG = 0UL;
    base->USBDEV.BUF_SIZE     = 0UL;
    base->USBDEV.DMA_THRES16  = 0UL;
    base->USBDEV.EP_ACTIVE    = 0UL;
    base->USBDEV.EP_TYPE      = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);
    base->USBDEV.CWA16        = 0UL;

    base->USBDEV.SIE_EP_INT_EN = 0UL;
    base->USBDEV.SIE_EP_INT_SR = 0UL;

    /* Set SIE endpoint register into the default state */
    for (i = 0UL; i < CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++i)
    {
        sieRegs->EP[i].SIE_EP_CR0  = 0UL;
        sieRegs->EP[i].SIE_EP_CNT0 = 0UL;
        sieRegs->EP[i].SIE_EP_CNT1 = 0UL;
    }

    /* Set ARBITER endpoint register into the default state */
    for (i = 0UL; i < CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++i)
    {
        arbRegs->EP8[i].ARB_EP_CFG    = 0UL;
        arbRegs->EP8[i].ARB_EP_INT_EN = 0UL;
        arbRegs->EP16[i].ARB_RW_RA16  = 0UL;
        arbRegs->EP16[i].ARB_RW_WA16  = 0UL;
    }
    
    /* Clean-up context callbacks */
    context->cbSof = NULL;
    context->cbLpm = NULL;
    
    for (i = 0UL; i < CY_USBFS_DEV_DRV_NUM_EPS_MAX; ++i)
    {
        context->epPool[i].address      = 0U;
        context->epPool[i].userMemcpy   = NULL;
        context->epPool[i].cbEpComplete = NULL;
        context->epPool[i].buffer       = NULL;
    }
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Enable
****************************************************************************//**
*
* Enable UBSFS hardware.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Enable(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) context;

    /* Clear EP0 count register */
    base->USBDEV.EP0_CNT = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);

    /* Set EP0.CR: ACK Setup, NAK IN/OUT */
    base->USBDEV.EP0_CR  = CY_USBFS_DEV_DRV_EP_CR_NAK_INOUT;

    /* Enable D+ pull-up, the device appears on the bus */
    base->USBLPM.POWER_CTL |= USBFS_USBLPM_POWER_CTL_DP_UP_EN_Msk;

    /* Push bufferable write to execute */
    (void) base->USBLPM.POWER_CTL;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Disable
****************************************************************************//**
*
* Disable UBSFS hardware.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Disable(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) context;

    /* Disable D+ pull-up */
    base->USBLPM.POWER_CTL &= ~USBFS_USBLPM_POWER_CTL_DP_UP_EN_Msk;

    /* Push bufferable write to execute */
    (void) base->USBLPM.POWER_CTL;    

    /* Disable device to respond to usb traffic */
    base->USBDEV.CR0 &= ~USBFS_USBDEV_CR0_USB_ENABLE_Msk;

    /* Disable DMA channels */
    DmaDisable(context);
}


/*******************************************************************************
* Function Name: LpmIntrHandler
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
static void LpmIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    if (context->cbLpm != NULL)
    {
        context->cbLpm(base, context);
    }
}


/*******************************************************************************
* Function Name: SofIntrHandler
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
static void SofIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    if (context->cbSof != NULL)
    {
        context->cbSof(base, context);
    }
}


/*******************************************************************************
* Function Name: Ep0IntrHandler
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
static void Ep0IntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Read CR register */
    uint32_t ep0Cr = base->USBDEV.EP0_CR;

    /* Check if packet was ACKed */
    if (0U != (ep0Cr & USBFS_USBDEV_EP0_CR_ACKED_TXN_Msk))
    {
        bool updateCr0 = false;

        /* Check packet direction */
        if (_FLD2BOOL(USBFS_USBDEV_EP0_CR_SETUP_RCVD, ep0Cr))
        {
            /* Handle SETUP */
            if (_FLD2VAL(USBFS_USBDEV_EP0_CR_MODE, ep0Cr) == CY_USBFS_DEV_DRV_EP_CR_NAK_INOUT)
            {
                /* Try to unlock CR0 register: read and then write.
                * The success write clears 8-4 bits in the register.
                */
                ep0Cr = base->USBDEV.EP0_CR;
                base->USBDEV.EP0_CR = ep0Cr;

                /* Check if CR0 register unlocked (bits cleared) */
                ep0Cr = base->USBDEV.EP0_CR;
                if (false == _FLD2BOOL(USBFS_USBDEV_EP0_CR_SETUP_RCVD, ep0Cr))
                {
                    /* Reset EP0 cnt register (data toggle 0) */
                    context->ep0CntReg = 0UL;

                    /* Call Device layer to service request */
                    context->ep0Setup(base, context);

                    updateCr0 = true;
                }
            }
        }
        /* Handle IN */
        else if (_FLD2BOOL(USBFS_USBDEV_EP0_CR_IN_RCVD, ep0Cr))
        {
            context->ep0In(base, context);
            updateCr0 = true;
        }
        else if (_FLD2BOOL(USBFS_USBDEV_EP0_CR_OUT_RCVD, ep0Cr))
        {
            /* Handle OUT */
            context->ep0Out(base, context);
            updateCr0 = true;
        }
        else
        {
            /* Do nothing - unknown source */
        }

        if (updateCr0)
        {
            /* Check if CR0 register unlocked (bits cleared) */
            ep0Cr = base->USBDEV.EP0_CR;

            if (false == _FLD2BOOL(USBFS_USBDEV_EP0_CR_SETUP_RCVD, ep0Cr))
            {
                /* Update count and mode registers */
                base->USBDEV.EP0_CNT = CY_USBFS_DEV_DRV_WRITE_ODD(context->ep0CntReg);
                base->USBDEV.EP0_CR  = context->ep0ModeReg;

                /* Push bufferable write to execute */
                base->USBDEV.EP0_CR;
            }
        }
    }
}


/*******************************************************************************
* Function Name: BusResetIntrHandler
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
static void BusResetIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Reset driver variables if any */

    /* Cypress ID# 293217: write CR0 when pull-up is enabled */
    if (_FLD2BOOL(USBFS_USBLPM_POWER_CTL_DP_UP_EN, base->USBLPM.POWER_CTL))
    {
        /* Pass event to the Device layer */
        context->busReset(base, context);

        /* Clear EP0 count register */
        base->USBDEV.EP0_CNT = CY_USBFS_DEV_DRV_WRITE_ODD(0UL);

        /* Set EP0.CR: ACK Setup, NAK IN/OUT */
        base->USBDEV.EP0_CR = CY_USBFS_DEV_DRV_EP_CR_NAK_INOUT;

        /* Enable device to responds to usb traffic with address 0 */
        base->USBDEV.CR0 = USBFS_USBDEV_CR0_USB_ENABLE_Msk;

        /* Push bufferable write to execute */
        (void) base->USBDEV.CR0;
    }
}


/*******************************************************************************
* Function Name: ArbiterIntrHandler
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
static void ArbiterIntrHandler(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint = 0UL;
    uint32_t intrMask = Cy_USBFS_Dev_Drv_GetArbAllEpsInterruptStatus(base);

    /* Handle active interrupt sources */
    while (0U != intrMask)
    {
        if (0U != (intrMask & 0x01U))
        {
            /* Get endpoint enable interrupt sources */
            uint32_t sourceMask = Cy_USBFS_Dev_Drv_GetArbEpInterruptStatusMasked(base, endpoint);

            Cy_USBFS_Dev_Drv_ClearArbEpInterrupt(base, endpoint, sourceMask);

            /* Mode 2/3: Handle IN endpoint buffer full event: trigger after
            * endpoint buffer has been loaded
            */
            if (0U != (sourceMask & USBFS_USBDEV_ARB_EP_IN_BUF_FULL_Msk))
            {
                Cy_USBFS_Dev_Drv_ClearArbCfgEpInReady(base, endpoint);

                /* Arm IN endpoint */
                Cy_USBFS_Dev_Drv_SetSieEpMode(base, endpoint, context->epPool[endpoint].sieMode);
            }

            /* Mode 2: Handle DMA completion event for OUT endpoints */
            if (0U != (sourceMask & USBFS_USBDEV_ARB_EP_DMA_GNT_Msk))
            {
                /* Notify user that data has been copied from endpoint buffer */
                context->epPool[endpoint].state = CY_USB_DEV_EP_COMPLETED;
            }

            /* Mode 3: Handle DMA completion event for OUT endpoints */
            if (0U != (sourceMask & USBFS_USBDEV_ARB_EP_DMA_TERMIN_Msk))
            {
                DmaOutEndpointRestore(&context->epPool[endpoint]);

                /* Set complete event and update data toggle */
                context->epPool[endpoint].state   = CY_USB_DEV_EP_COMPLETED;
                context->epPool[endpoint].toggle ^= USBFS_USBDEV_SIE_EP_DATA_TOGGLE_Msk;

                /* Involve callback if registered */
                if (context->epPool[endpoint].cbEpComplete != NULL)
                {
                    uint32_t errorType = 0UL;
                    
                    /* Check transfer errors (detect by hardware) */
                    if (0U != Cy_USBFS_Dev_Drv_GetSieEpError(base, endpoint))
                    {
                        errorType |= CY_USBFS_DEV_ENDPOINT_TRANSFER_ERROR;
                    }
                    
                    /* Check data toggle bit of current transfer */
                    if (context->epPool[endpoint].toggle == Cy_USBFS_Dev_Drv_GetSieEpToggle(base, endpoint))
                    {
                        errorType |= CY_USBFS_DEV_ENDPOINT_SAME_DATA_TOGGLE;
                    }                    
                    
                    context->epPool[endpoint].cbEpComplete(base, errorType, context);
                }
            }

            /* This error condition indicates system failure */
            if (0U != (sourceMask & USBFS_USBDEV_ARB_EP_BUF_OVER_Msk))
            {
                /* The DMA is not capable to move data from the mxusbfs IP 
                * hardware buffer fast enough what caused overflow. Give DMA 
                * channel for this endpoint greater priority or increase clock 
                * it operates.
                */
                while(1UL);
            }

            /* This error condition indicates system failure */
            if (0U != (sourceMask & USBFS_USBDEV_ARB_EP_BUF_UNDER_Msk))
            {
                /* The DMA is not capable to move data into the mxusbfs IP 
                * hardware buffer fast enough what caused underflow. Give DMA 
                * channel for this endpoint greater priority or increase clock 
                * it operates.
                */                
                while(1UL);
            }
        }

        /* Move to next endpoint */
        intrMask >>= 1UL;
        ++endpoint;
    }
}


/*******************************************************************************
* Function Name: SieEnpointIntrHandler
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
static void SieEnpointIntrHandler(USBFS_Type *base, uint32_t endpoint, cy_stc_usbfs_dev_drv_context_t *context)
{
    Cy_USBFS_Dev_Drv_ClearSieEpInterrupt(base, endpoint);

    /* Special case:  mode = CY_USBFS_DEV_DRV_EP_MANAGEMENT_DMA_AUTO
    * OUT endpoints completion is handled by Arbiter Interrupt (source DMA_TERMIN)
    */
    if (0U == (Cy_USBFS_Dev_Drv_GetArbEpInterruptMask(base, endpoint) & USBFS_USBDEV_ARB_EP_DMA_TERMIN_Msk))
    {
        /* Set complete event and update data toggle */
        context->epPool[endpoint].state   = CY_USB_DEV_EP_COMPLETED;
        context->epPool[endpoint].toggle ^= USBFS_USBDEV_SIE_EP_DATA_TOGGLE_Msk;

        /* Involve callback if it is registered */
        if (context->epPool[endpoint].cbEpComplete != NULL)
        {
            uint32_t errorType = 0UL;
            
            /* Check transfer errors (detect by hardware) */
            if (0U != Cy_USBFS_Dev_Drv_GetSieEpError(base, endpoint))
            {
                errorType |= CY_USBFS_DEV_ENDPOINT_TRANSFER_ERROR;
            }
            
            /* Check data toggle bit of current transfer */
            if (context->epPool[endpoint].toggle == Cy_USBFS_Dev_Drv_GetSieEpToggle(base, endpoint))
            {
                errorType |= CY_USBFS_DEV_ENDPOINT_SAME_DATA_TOGGLE;
            }
            
            context->epPool[endpoint].cbEpComplete(base, errorType, context);
        }
    }
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Interrupt
****************************************************************************//**
*
* Handles interrupt events.
*
* \param base
* The pointer to the USBFS instance.
*
* \param intrCause
* The interrupt cause register value.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user. The structure is used during the EZI2C operation for
* internal configuration and data keeping. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Interrupt(USBFS_Type *base, uint32_t intrCause, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t endpoint = 0u;

    /* Clear SIE interrupts while are served below */
    Cy_USBFS_Dev_Drv_ClearSieInterrupt(base, intrCause);

    /* LPM */
    if (0u != (intrCause & USBFS_USBLPM_INTR_CAUSE_LPM_INTR_Msk))
    {
        LpmIntrHandler(base, context);
    }

    /* Arbiter: Data endpoints */
    if (0u != (intrCause & USBFS_USBLPM_INTR_CAUSE_ARB_EP_INTR_Msk))
    {
        /* This interrupt is cleared inside the handler */
        ArbiterIntrHandler(base, context);
    }

    /* Control EP0 */
    if (0u != (intrCause & USBFS_USBLPM_INTR_CAUSE_EP0_INTR_Msk))
    {
        Ep0IntrHandler(base, context);
    }

    /* SOF */
    if (0u != (intrCause & USBFS_USBLPM_INTR_CAUSE_SOF_INTR_Msk))
    {
        SofIntrHandler(base, context);
    }

    /* Bus Reset */
    if (0u != (intrCause & USBFS_USBLPM_INTR_CAUSE_BUS_RESET_INTR_Msk))
    {
        BusResetIntrHandler(base, context);
    }

    /* Remove handled interrupt statuses */
    intrCause >>= USBFS_USBLPM_INTR_CAUSE_EP1_INTR_Pos;

    /* SIE: Data endpoints */
    while (0u != intrCause)
    {
        if (0u != (intrCause & 0x1u))
        {
            /* These interrupts are cleared inside the handler */
            SieEnpointIntrHandler(base, endpoint, context);
        }

        intrCause >>= 1u;
        ++endpoint;
    }
}



/*******************************************************************************
* Function Name: WriteEp0Buffer
****************************************************************************//**
*
* Writes data into endpoint 0 buffer and returns how many bytes were written.
*
* \param base
* The pointer to the USBFS instance.
*
* \param buffer
* Pointer to the buffer where data is written.
*
* \param size
* Number of bytes to write into the buffer.
*
* \return
* Number of bytes which were written.
*
*******************************************************************************/
static uint32_t WriteEp0Buffer(USBFS_Type *base, uint8_t *buf, uint32_t size)
{
    uint32_t cnt;
    
    /* Cut message size if too many bytes to write are requested */
    if (size > CY_USBFS_DEV_DRV_EP0_BUFFER_SIZE)
    {
        size = CY_USBFS_DEV_DRV_EP0_BUFFER_SIZE;
    }
    
    /* Write data into the hardware buffer */
    for (cnt = 0UL; cnt < size; ++cnt)
    {
        if (0U == (cnt & 0x1U))
        {
            base->USBDEV.EP0_DR[cnt] = buf[cnt];
        }
        else
        {
            /* Apply special write for odd offset registers */
            base->USBDEV.EP0_DR[cnt] = CY_USBFS_DEV_DRV_WRITE_ODD(buf[cnt]);
        }
    }
    
    return cnt;
}


/*******************************************************************************
* Function Name: ReadEp0Buffer
****************************************************************************//**
*
* Reads data from endpoint 0 buffer and returns how many bytes were read.
*
* \param base
* The pointer to the USBFS instance.
*
* \param buffer
* Pointer to the buffer where data is read.
*
* \param size
* Number of bytes to read from the buffer.
*
* \return
* Number of bytes which were read.
*
*******************************************************************************/
static uint32_t ReadEp0Buffer(USBFS_Type *base, uint8_t *buf, uint32_t size)
{
    uint32_t cnt;
    
    /* Get number of received bytes */
    uint32_t numToCopy = Cy_USBFS_Dev_Drv_GetEp0Count(base);

    /* Read only received bytes */
    if (size > numToCopy)
    {
        size = numToCopy;
    }
    
    /* Get data from the buffer */
    for (cnt = 0UL; cnt < size; ++cnt)
    {
        if (0U == (cnt & 0x1U))
        {
            buf[cnt] = base->USBDEV.EP0_DR[cnt];
        }
        else
        {
            /* Apply special write for odd offset registers */
            buf[cnt] = CY_USBFS_DEV_READ_ODD(base->USBDEV.EP0_DR[cnt]);
        }
    }
    
    return cnt;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Ep0GetSetup
****************************************************************************//**
*
* Reads setup packed from endpoint 0 buffer to provided buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param buffer
* Pointer to the buffer where store data.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Ep0GetSetup(USBFS_Type *base, uint8_t *buffer, cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) context;

    (void) ReadEp0Buffer(base, buffer, CY_USBFS_DEV_DRV_EP0_BUFFER_SIZE);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Ep0Write
****************************************************************************//**
*
* Writes data into endpoint 0 buffer and returns how many bytes were written.
*
* \param base
* The pointer to the USBFS instance.
*
* \param buffer
* Pointer to the buffer where stored data to be written.
*
* \param size
* Number of bytes to write into the endpoint 0 buffer.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Number of bytes which were written into the endpoint 0 buffer.
*
*******************************************************************************/
uint32_t Cy_USBFS_Dev_Drv_Ep0Write(USBFS_Type *base, uint8_t *buffer, uint32_t size, cy_stc_usbfs_dev_drv_context_t *context)
{
    CY_ASSERT_L1((size > 0U) ? (NULL != buffer) : true);
    
    uint32_t numBytes = 0UL;

    if (buffer != NULL)
    {
        /* DATA stage (IN direction): load data to be sent (include zero length packet) */
        
        /* Put data into the buffer */
        numBytes = WriteEp0Buffer(base, buffer, size);

        /* Update data toggle and counter */
        context->ep0CntReg ^= USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Msk;
        context->ep0CntReg  = _CLR_SET_FLD32U(context->ep0CntReg, USBFS_USBDEV_EP0_CNT_BYTE_COUNT, numBytes);
        
        /* Update EP0 mode register to continue transfer */
        context->ep0ModeReg = CY_USBFS_DEV_DRV_EP_CR_ACK_IN_STATUS_OUT;
    }
    else
    {
        /* STATUS stage (IN direction): prepare return zero-length and get ACK response */
        context->ep0CntReg  = USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Msk;
        context->ep0ModeReg = CY_USBFS_DEV_DRV_EP_CR_STATUS_IN_ONLY;
    }

    return numBytes;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Ep0Read
****************************************************************************//**
*
* Reads data from endpoint 0 buffer and returns how many bytes were read.
*
* \param base
* The pointer to the USBFS instance.
*
* \param buffer
* Pointer to the buffer where store data.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
*
* \return
* Number of bytes which were read from the endpoint 0 buffer.
*
*******************************************************************************/
uint32_t Cy_USBFS_Dev_Drv_Ep0Read(USBFS_Type *base, uint8_t *buffer, uint32_t size, cy_stc_usbfs_dev_drv_context_t *context)
{    
    uint32_t numBytes = 0UL;

    if (buffer != NULL)
    {
        /* DATA stage (OUT direction): get receive data and continue */

        /* Get received data */
        numBytes = ReadEp0Buffer(base, buffer, size);

        /* Update EP0 registers to continue transfer */
        context->ep0CntReg ^= USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Msk;
        context->ep0ModeReg = CY_USBFS_DEV_DRV_EP_CR_ACK_OUT_STATUS_IN;
    }
    else
    {
        /* STATUS stage (OUT direction): prepare to send ACK handshake */
        context->ep0CntReg  = USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Msk;
        context->ep0ModeReg = CY_USBFS_DEV_DRV_EP_CR_STATUS_OUT_ONLY;
    }

    return numBytes;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_RegisterServiceCallback
****************************************************************************//**
*
* Register callbacks to the service events (Bus Reset and Endpoint 0).
*
* \param base
* The pointer to the USBFS instance.
*
* \param source
* \ref cy_en_usbfs_dev_drv_service_cb_t
*
* \param callback
* Callback function which is called for event define by source parameter.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbfs_dev_drv_context_t
* allocated by the user.
*
* \return
* Status of executed operation cy_en_usbfs_dev_drv_status_t.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_RegisterServiceCallback(cy_en_usb_dev_service_cb_t     source,
                                              cy_cb_usbfs_dev_drv_callback_t callback,
                                              cy_stc_usbfs_dev_drv_context_t *context)
{
    switch(source)
    {
        case CY_USB_DEV_BUS_RESET:
            context->busReset = callback;
        break;
            
        case CY_USB_DEV_EP0_SETUP:
            context->ep0Setup = callback;
        break;
        
        case CY_USB_DEV_EP0_IN:
            context->ep0In = callback;
        break;
            
        case CY_USB_DEV_EP0_OUT:
            context->ep0Out = callback;
        break;
            
        default:
        break;
    }
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Suspend
****************************************************************************//**
*
* Prepares the USBFS component to enter deep sleep mode. The pull-up is enabled 
* on the Dp line while device is low power mode.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the driver context structure 
*
* \note
* After entering low power mode, the data which is left in the IN or OUT 
* endpoint buffers are not restored after wakeup, and lost. Therefore, it should
* be stored in the SRAM for OUT endpoint or read by the host for IN endpoint
* before entering low power mode.
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Suspend(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    (void) base; (void) context;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_Resume
****************************************************************************//**
*
* Enables the USBFS block after power down mode. It should be called just after
* waking from sleep. While the device is suspended, it periodically checks to 
* determine if the conditions to leave the suspended state were met. One way to
* check resume conditions is to use the sleep timer to periodically wake the 
* device. The second way is to configure the device to wake up from the PICU.
* If the resume conditions are met, the application calls the Resume() function.
* This function enables the SIE and Transceiver, bringing them out of power down
* mode. It does not change the USB address field of the USBCR register; it 
* maintains the USB address previously assigned by the host.
*
* \param base
* The pointer to the USBFS instance.
*
* \param context
* The pointer to the driver context structure 
*
*******************************************************************************/
void Cy_USBFS_Dev_Drv_Resume(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    (void) base; (void) context;
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */


/* [] END OF FILE */
