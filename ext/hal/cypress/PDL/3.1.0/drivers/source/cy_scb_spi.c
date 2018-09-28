/***************************************************************************//**
* \file cy_scb_spi.c
* \version 2.20
*
* Provides SPI API implementation of the SCB driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_scb_spi.h"

#ifdef CY_IP_MXSCB

#if defined(__cplusplus)
extern "C" {
#endif

/* Static functions */
static void HandleTransmit(CySCB_Type *base, cy_stc_scb_spi_context_t *context);
static void HandleReceive (CySCB_Type *base, cy_stc_scb_spi_context_t *context);
static void DiscardArrayNoCheck(CySCB_Type const *base, uint32_t size);

/*******************************************************************************
* Function Name: Cy_SCB_SPI_Init
****************************************************************************//**
*
* Initializes the SCB for SPI operation.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param config
* The pointer to the configuration structure \ref cy_stc_scb_spi_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SPI functions that do not require context will be used to pass NULL
* as pointer to context.
*
* \return
* \ref cy_en_scb_spi_status_t
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
cy_en_scb_spi_status_t Cy_SCB_SPI_Init(CySCB_Type *base, cy_stc_scb_spi_config_t const *config, cy_stc_scb_spi_context_t *context)
{
    /* Input parameters verification */
    if ((NULL == base) || (NULL == config))
    {
        return CY_SCB_SPI_BAD_PARAM;
    }

    CY_ASSERT_L3(CY_SCB_SPI_IS_MODE_VALID     (config->spiMode));
    CY_ASSERT_L3(CY_SCB_SPI_IS_SUB_MODE_VALID (config->subMode));
    CY_ASSERT_L3(CY_SCB_SPI_IS_SCLK_MODE_VALID(config->sclkMode));

    CY_ASSERT_L2(CY_SCB_SPI_IS_OVERSAMPLE_VALID (config->oversample, config->spiMode));
    CY_ASSERT_L2(CY_SCB_SPI_IS_SS_POLARITY_VALID(config->ssPolarity));
    CY_ASSERT_L2(CY_SCB_SPI_IS_DATA_WIDTH_VALID (config->rxDataWidth));
    CY_ASSERT_L2(CY_SCB_SPI_IS_DATA_WIDTH_VALID (config->txDataWidth));
    CY_ASSERT_L2(CY_SCB_SPI_IS_BOTH_DATA_WIDTH_VALID(config->subMode, config->rxDataWidth, config->txDataWidth));

    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(config->rxFifoIntEnableMask, CY_SCB_SPI_RX_INTR_MASK));
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(config->txFifoIntEnableMask, CY_SCB_SPI_TX_INTR_MASK));
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(config->masterSlaveIntEnableMask, CY_SCB_SPI_MASTER_SLAVE_INTR_MASK));

    uint32_t locSclkMode = CY_SCB_SPI_GetSclkMode(config->subMode, config->sclkMode);

    bool byteMode = (config->rxDataWidth <= CY_SCB_BYTE_WIDTH) && (config->txDataWidth <= CY_SCB_BYTE_WIDTH);

    /* Configure an SPI interface */
    base->CTRL = _BOOL2FLD(SCB_CTRL_BYTE_MODE, byteMode)                     |
                 _BOOL2FLD(SCB_CTRL_EC_AM_MODE, config->enableWakeFromSleep) |
                 _VAL2FLD(SCB_CTRL_OVS, (config->oversample - 1UL))          |
                 _VAL2FLD(SCB_CTRL_MODE, CY_SCB_CTRL_MODE_SPI);

    /* Configure SCB_CTRL.BYTE_MODE then verify levels */
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, config->rxFifoTriggerLevel));
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, config->txFifoTriggerLevel));

    base->SPI_CTRL = _BOOL2FLD(SCB_SPI_CTRL_SSEL_CONTINUOUS, (!config->enableTransferSeperation)) |
                     _BOOL2FLD(SCB_SPI_CTRL_SELECT_PRECEDE,  (CY_SCB_SPI_TI_PRECEDES == config->subMode)) |
                     _BOOL2FLD(SCB_SPI_CTRL_LATE_MISO_SAMPLE, config->enableMisoLateSample)       |
                     _BOOL2FLD(SCB_SPI_CTRL_SCLK_CONTINUOUS,  config->enableFreeRunSclk)          |
                     _BOOL2FLD(SCB_SPI_CTRL_MASTER_MODE,     (CY_SCB_SPI_MASTER == config->spiMode)) |
                     _VAL2FLD(CY_SCB_SPI_CTRL_CLK_MODE,      locSclkMode)                         |
                     _VAL2FLD(CY_SCB_SPI_CTRL_SSEL_POLARITY, config->ssPolarity)                  |
                     _VAL2FLD(SCB_SPI_CTRL_MODE,  (uint32_t) config->subMode);

    /* Configure the RX direction */
    base->RX_CTRL = _BOOL2FLD(SCB_RX_CTRL_MSB_FIRST, config->enableMsbFirst) |
                    _BOOL2FLD(SCB_RX_CTRL_MEDIAN, config->enableInputFilter) |
                    _VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, (config->rxDataWidth - 1UL));

    base->RX_FIFO_CTRL = _VAL2FLD(SCB_RX_FIFO_CTRL_TRIGGER_LEVEL, config->rxFifoTriggerLevel);

    /* Configure the TX direction */
    base->TX_CTRL = _BOOL2FLD(SCB_TX_CTRL_MSB_FIRST, config->enableMsbFirst) |
                    _VAL2FLD(SCB_TX_CTRL_DATA_WIDTH, (config->txDataWidth - 1UL));

    base->TX_FIFO_CTRL = _VAL2FLD(SCB_TX_FIFO_CTRL_TRIGGER_LEVEL, config->txFifoTriggerLevel);

    /* Set up interrupt sources */
    base->INTR_RX_MASK = (config->rxFifoIntEnableMask & CY_SCB_SPI_RX_INTR_MASK);
    base->INTR_TX_MASK = (config->txFifoIntEnableMask & CY_SCB_SPI_TX_INTR_MASK);
    base->INTR_M       = (config->masterSlaveIntEnableMask & CY_SCB_SPI_MASTER_DONE);
    base->INTR_S       = (config->masterSlaveIntEnableMask & CY_SCB_SPI_SLAVE_ERR);
    base->INTR_SPI_EC_MASK = 0UL;

    /* Initialize the context */
    if (NULL != context)
    {
        context->status    = 0UL;

        context->txBufIdx  = 0UL;
        context->rxBufIdx  = 0UL;

        context->cbEvents = NULL;

    #if !defined(NDEBUG)
        /* Put an initialization key into the initKey variable to verify
        * context initialization in the transfer API.
        */
        context->initKey = CY_SCB_SPI_INIT_KEY;
    #endif /* !(NDEBUG) */
    }

    return CY_SCB_SPI_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_DeInit
****************************************************************************//**
*
* De-initializes the SCB block; returns the register values to default.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
void Cy_SCB_SPI_DeInit(CySCB_Type *base)
{
    /* SPI interface */
    base->CTRL         = CY_SCB_CTRL_DEF_VAL;
    base->SPI_CTRL     = CY_SCB_SPI_CTRL_DEF_VAL;

    /* RX direction */
    base->RX_CTRL      = CY_SCB_RX_CTRL_DEF_VAL;
    base->RX_FIFO_CTRL = 0UL;

    /* TX direction */
    base->TX_CTRL      = CY_SCB_TX_CTRL_DEF_VAL;
    base->TX_FIFO_CTRL = 0UL;

    /* Disable all interrupt sources */
    base->INTR_SPI_EC_MASK = 0UL;
    base->INTR_I2C_EC_MASK = 0UL;
    base->INTR_RX_MASK     = 0UL;
    base->INTR_TX_MASK     = 0UL;
    base->INTR_M_MASK      = 0UL;
    base->INTR_S_MASK      = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_Disable
****************************************************************************//**
*
* Disables the SCB block, clears context statuses, and disables
* TX and RX interrupt sources.
* Note that after the block is disabled, the TX and RX FIFOs and
* hardware statuses are cleared. Also, the hardware stops driving the output
* and ignores the input.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SPI functions that do not require context will be used to pass NULL
* as pointer to context.
*
* \note
* Calling this function when the SPI is busy (master preforms data transfer or
* slave communicates with the master) may cause transfer corruption because the
* hardware stops driving the outputs and ignores the inputs.
* Ensure that the SPI is not busy before calling this
* function.
*
*******************************************************************************/
void Cy_SCB_SPI_Disable(CySCB_Type *base, cy_stc_scb_spi_context_t *context)
{
    base->CTRL &= (uint32_t) ~SCB_CTRL_ENABLED_Msk;

    if (NULL != context)
    {
        context->status    = 0UL;

        context->rxBufIdx  = 0UL;
        context->txBufIdx  = 0UL;
    }

    /* Disable RX and TX interrupt sources for the slave because
    * RX overflow and TX underflow are kept enabled after the 1st call of
    * Cy_SCB_SPI_Transfer().
    */
    if (!_FLD2BOOL(SCB_SPI_CTRL_MASTER_MODE, base->SPI_CTRL))
    {
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_DeepSleepCallback
****************************************************************************//**
*
* This function handles the transition of the SCB SPI into and out of
* Deep Sleep mode. It prevents the device from entering Deep Sleep mode
* if the SPI slave or master is actively communicating, or there is any data 
* in the TX or RX FIFOs.
* The following behavior of the SPI SCB depends on whether the SCB block is
* wakeup-capable or not:
* * The SCB is <b>wakeup-capable</b>: any transfer intended to the slave wakes up 
*   the device from Deep Sleep mode. The slave responds with 0xFF to the transfer 
*   and incoming data is ignored.   
*   If the transfer occurs before the device enters Deep Sleep mode, the device
*   will not enter Deep Sleep mode and incoming data is stored in the RX FIFO.
*   The SCB clock is disabled before entering Deep Sleep and enabled after the 
*   device exits Deep Sleep mode. The SCB clock must be enabled after exiting 
*   Deep Sleep mode and after the source of hf_clk[0] gets stable, this includes 
*   the FLL/PLL. The SysClk callback ensures that hf_clk[0] gets stable and 
*   it must be called before Cy_SCB_SPI_DeepSleepCallback. The SCB clock 
*   disabling may lead to corrupted data in the RX FIFO. Clear the RX FIFO 
*   after this callback is executed. If the transfer occurs before the device 
*   enters Deep Sleep mode, the device will not enter Deep Sleep mode and 
*   incoming data will be stored in the RX FIFO. \n
*   Only the SPI slave can be configured to be a wakeup source from Deep Sleep
*   mode.
* * The SCB is not <b>wakeup-capable</b>: the SPI is disabled. It is enabled when 
*   the device fails to enter Deep Sleep mode or it is awakened from Deep Sleep 
*   mode. While the SPI is disabled, it stops driving the outputs and ignores the
*   inputs. Any incoming data is ignored.
*
* This function must be called during execution of \ref Cy_SysPm_DeepSleep.
* To do it, register this function as a callback before calling
* \ref Cy_SysPm_DeepSleep : specify \ref CY_SYSPM_DEEPSLEEP as the callback
* type and call \ref Cy_SysPm_RegisterCallback.
*
* \param callbackParams
* The pointer to the callback parameters structure
* \ref cy_stc_syspm_callback_params_t.
*
* \return
* \ref cy_en_syspm_status_t
*
* \note
* Only applicable for <b>rev-08 of the CY8CKIT-062-BLE</b>.
* For proper operation, when the SPI slave is configured to be a wakeup source
* from Deep Sleep mode, this function must be copied and modified by the user.
* The SPI clock disable code must be inserted in the \ref CY_SYSPM_BEFORE_TRANSITION
* and clock enable code in the \ref CY_SYSPM_AFTER_TRANSITION mode processing.
*
*******************************************************************************/
cy_en_syspm_status_t Cy_SCB_SPI_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    CySCB_Type *locBase = (CySCB_Type *) callbackParams->base;
    cy_stc_scb_spi_context_t *locContext = (cy_stc_scb_spi_context_t *) callbackParams->context;

    cy_en_syspm_status_t retStatus = CY_SYSPM_FAIL;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            /* Check whether the High-level API is not busy executing the transfer
            * operation.
            */
            if (0UL == (CY_SCB_SPI_TRANSFER_ACTIVE & Cy_SCB_SPI_GetTransferStatus(locBase, locContext)))
            {
                /* If the SPI bus is not busy, all data elements are transferred
                * on the bus from the TX FIFO and shifter and the RX FIFOs are
                * empty - the SPI is ready to enter Deep Sleep mode.
                */
                if (!Cy_SCB_SPI_IsBusBusy(locBase))
                {
                    if (Cy_SCB_SPI_IsTxComplete(locBase))
                    {
                        if (0UL == Cy_SCB_SPI_GetNumInRxFifo(locBase))
                        {
                            if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
                            {
                                /* The SCB is wakeup-capable: clear the SPI
                                * wakeup interrupt source because it triggers
                                * during Active mode communication and make
                                * sure that a new transfer is not started after
                                * clearing.
                                */
                                Cy_SCB_ClearSpiInterrupt(locBase, CY_SCB_SPI_INTR_WAKEUP);

                                if (!Cy_SCB_SPI_IsBusBusy(locBase))
                                {
                                    retStatus = CY_SYSPM_SUCCESS;
                                }
                            }
                            else
                            {
                                /* The SCB is NOT wakeup-capable: disable the
                                * SPI. The master and slave stop driving the
                                * bus until the SPI is enabled. This happens
                                * when the device fails to enter Deep Sleep
                                * mode or it is awakened from Deep Sleep mode.
                                */
                                Cy_SCB_SPI_Disable(locBase, locContext);

                                retStatus = CY_SYSPM_SUCCESS;
                            }
                        }
                    }
                }
            }
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            /* The other driver is not ready for Deep Sleep mode. Restore
            * Active mode configuration.
            */

            if (!_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
            {
                /* The SCB is NOT wakeup-capable: enable the SPI to operate */
                Cy_SCB_SPI_Enable(locBase);
            }

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* This code executes inside the critical section and enabling the
            * active interrupt source makes the interrupt pending in the NVIC.
            * However, the interrupt processing is delayed until the code exits
            * the critical section. The pending interrupt force WFI instruction
            * does nothing and the device remains in Active mode.
            */

            if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
            {
                /* The SCB is wakeup-capable: enable the SPI wakeup interrupt
                * source. If any transaction happens, the wakeup interrupt
                * becomes pending and prevents entering Deep Sleep mode.
                */
                Cy_SCB_SetSpiInterruptMask(locBase, CY_SCB_I2C_INTR_WAKEUP);
                
                /* Disable SCB clock */
                locBase->I2C_CFG &= (uint32_t) ~CY_SCB_I2C_CFG_CLK_ENABLE_Msk;
                            
                /* IMPORTANT (replace line above for the CY8CKIT-062 rev-08): 
                * for proper entering Deep Sleep mode the SPI clock must be disabled. 
                * This code must be inserted by the user because the driver 
                * does not have access to the clock.
                */
            }

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_AFTER_TRANSITION:
        {
            if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
            {
                /* Enable SCB clock */
                locBase->I2C_CFG |= CY_SCB_I2C_CFG_CLK_ENABLE_Msk;
                
                /* IMPORTANT (replace line above for the CY8CKIT-062 rev-08): 
                * for proper exiting Deep Sleep mode, the SPI clock must be enabled. 
                * This code must be inserted by the user because the driver 
                * does not have access to the clock.
                */
                
                /* The SCB is wakeup-capable: disable the SPI wakeup interrupt
                * source
                */
                Cy_SCB_SetSpiInterruptMask(locBase, CY_SCB_CLEAR_ALL_INTR_SRC);
            }
            else
            {
                /* The SCB is NOT wakeup-capable: enable the SPI to operate */
                Cy_SCB_SPI_Enable(locBase);
            }

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_HibernateCallback
****************************************************************************//**
*
* This function handles the transition of the SCB SPI into Hibernate mode.
* It prevents the device from entering Hibernate mode if the SPI slave or 
* master is actively communicating, or there is any data in the TX or RX FIFOs.
* If the SPI is ready to enter Hibernate mode, it is disabled. If the device
* failed to enter Hibernate mode, the SPI is enabled. While the SPI is
* disabled, it stops driving the outputs and ignores the inputs.
* Any incoming data is ignored.
*
* This function must be called during execution of \ref Cy_SysPm_Hibernate.
* To do it, register this function as a callback before calling
* \ref Cy_SysPm_Hibernate : specify \ref CY_SYSPM_HIBERNATE as the callback
* type and call \ref Cy_SysPm_RegisterCallback.
*
* \param callbackParams
* The pointer to the callback parameters structure
* \ref cy_stc_syspm_callback_params_t.
*
* \return
* \ref cy_en_syspm_status_t
*
*******************************************************************************/
cy_en_syspm_status_t Cy_SCB_SPI_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    CySCB_Type *locBase = (CySCB_Type *) callbackParams->base;
    cy_stc_scb_spi_context_t *locContext = (cy_stc_scb_spi_context_t *) callbackParams->context;

    cy_en_syspm_status_t  retStatus = CY_SYSPM_FAIL;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            /* Check whether the High-level API is not busy executing the transfer
            * operation.
            */
            if (0UL == (CY_SCB_SPI_TRANSFER_ACTIVE & Cy_SCB_SPI_GetTransferStatus(locBase, locContext)))
            {
                /* If the SPI bus is not busy, all data elements are transferred
                * on the bus from the TX FIFO and shifter and the RX FIFOs are
                * empty - the SPI is ready to enter Hibernate mode.
                */
                if (!Cy_SCB_SPI_IsBusBusy(locBase))
                {
                    if (Cy_SCB_SPI_IsTxComplete(locBase))
                    {
                        if (0UL == Cy_SCB_SPI_GetNumInRxFifo(locBase))
                        {
                            /* Disable the SPI. The master or slave stops
                            * driving the bus until the SPI is enabled.
                            * This happens if the device failed to enter
                            * Hibernate mode.
                            */
                            Cy_SCB_SPI_Disable(locBase, locContext);

                            retStatus = CY_SYSPM_SUCCESS;
                        }
                    }
                }
            }
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            /* The other driver is not ready for Hibernate mode. Restore Active
            * mode configuration.
            */

            /* Enable the SPI to operate */
            Cy_SCB_SPI_Enable(locBase);

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* The SCB is not capable of waking up from Hibernate mode:
            * do nothing.
            */
            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (retStatus);
}


/************************* High-Level Functions ********************************
* The following functions are considered high-level. They provide the layer of
* intelligence to the SCB. These functions require interrupts.
* Low-level and high-level functions must not be mixed because low-level API
* can adversely affect the operation of high-level functions.
*******************************************************************************/


/*******************************************************************************
* Function Name: Cy_SCB_SPI_Transfer
****************************************************************************//**
*
* This function starts an SPI transfer operation.
* It configures transmit and receive buffers for an SPI transfer.
* If the data that will be received is not important, pass NULL as rxBuffer.
* If the data that will be transmitted is not important, pass NULL as txBuffer
* and then the \ref CY_SCB_SPI_DEFAULT_TX is sent out as each data element.
* Note that passing NULL as rxBuffer and txBuffer are considered invalid cases.
*
* After the function configures TX and RX interrupt sources, it returns and
* \ref Cy_SCB_SPI_Interrupt manages further data transfer.
*
* * In the master mode, the transfer operation starts after calling this
*   function
* * In the slave mode, the transfer registers and will start when
*   the master request arrives.
*
* When the transfer operation is completed (requested number of data elements
* sent and received), the \ref CY_SCB_SPI_TRANSFER_ACTIVE status is cleared
* and the \ref CY_SCB_SPI_TRANSFER_CMPLT_EVENT event is generated.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param txBuffer
* The pointer of the buffer with data to transmit.
* The item size is defined by the data type that depends on the configured
* TX data width.
*
* \param rxBuffer
* The pointer to the buffer to store received data.
* The item size is defined by the data type that depends on the configured
* RX data width.
*
* \param size
* The number of data elements to transmit and receive.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_spi_status_t
*
* \note
* * The buffers must not be modified and must stay allocated until the end of the
*   transfer.
* * This function overrides all RX and TX FIFO interrupt sources and changes
*   the RX and TX FIFO level.
*
*******************************************************************************/
cy_en_scb_spi_status_t Cy_SCB_SPI_Transfer(CySCB_Type *base, void *txBuffer, void *rxBuffer, uint32_t size,
                                           cy_stc_scb_spi_context_t *context)
{
    CY_ASSERT_L1(NULL != context);
    CY_ASSERT_L1(CY_SCB_SPI_INIT_KEY == context->initKey);
    CY_ASSERT_L1(CY_SCB_SPI_IS_BUFFER_VALID(txBuffer, rxBuffer, size));

    cy_en_scb_spi_status_t retStatus = CY_SCB_SPI_TRANSFER_BUSY;

    /* Check whether there are no active transfer requests */
    if (0UL == (CY_SCB_SPI_TRANSFER_ACTIVE & context->status))
    {
        uint32_t fifoSize = Cy_SCB_GetFifoSize(base);

        /* Set up the context */
        context->status    = CY_SCB_SPI_TRANSFER_ACTIVE;

        context->txBuf     = txBuffer;
        context->txBufSize = size;
        context->txBufIdx =  0UL;

        context->rxBuf     = rxBuffer;
        context->rxBufSize = size;
        context->rxBufIdx  = 0UL;

        /* Set the TX interrupt when half of FIFO was transmitted */
        Cy_SCB_SetTxFifoLevel(base, fifoSize / 2UL);

        if (_FLD2BOOL(SCB_SPI_CTRL_MASTER_MODE, base->SPI_CTRL))
        {
            /* Trigger an RX interrupt:
            * - If the transfer size is equal to or less than FIFO, trigger at the end of the transfer.
            * - If the transfer size is greater than FIFO, trigger 1 byte earlier than the TX interrupt.
            */
            Cy_SCB_SetRxFifoLevel(base, (size > fifoSize) ? ((fifoSize / 2UL) - 2UL) : (size - 1UL));

            Cy_SCB_SetMasterInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

            /* Enable interrupt sources to perform a transfer */
            Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_LEVEL);
        }
        else
        {
            /* Trigger an RX interrupt:
            * - If the transfer size is equal to or less than half of FIFO, trigger ??at the end of the transfer.
            * - If the transfer size is greater than half of FIFO, trigger 1 byte earlier than a TX interrupt.
            */
            Cy_SCB_SetRxFifoLevel(base, (size > (fifoSize / 2UL)) ? ((fifoSize / 2UL) - 2UL) : (size - 1UL));

            Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_SLAVE_INTR_SPI_BUS_ERROR);

            /* Enable interrupt sources to perform a transfer */
            Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL | CY_SCB_RX_INTR_OVERFLOW);
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_LEVEL | CY_SCB_TX_INTR_UNDERFLOW);
        }

        retStatus = CY_SCB_SPI_SUCCESS;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_AbortTransfer
****************************************************************************//**
*
* Aborts the current SPI transfer.
* It disables the transmit and RX interrupt sources, clears the TX
* and RX FIFOs and the status.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* In the slave mode and after abort of transfer operation master continue
* sending data it gets into RX FIFO and TX FIFO is underflow as there is
* nothing to send. To drop this data, RX FIFO must be cleared when
* the transfer is complete. Otherwise, received data will be kept and
* copied to the buffer when \ref Cy_SCB_SPI_Transfer is called.
*
* \sideeffect
* The transmit FIFO clear operation also clears the shift register, so that
* the shifter can be cleared in the middle of a data element transfer,
* corrupting it. The data element corruption means that all bits that have
* not been transmitted are transmitted as "ones" on the bus.
*
*******************************************************************************/
void Cy_SCB_SPI_AbortTransfer(CySCB_Type *base, cy_stc_scb_spi_context_t *context)
{
    /* Disable interrupt sources */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    if (_FLD2BOOL(SCB_SPI_CTRL_MASTER_MODE, base->SPI_CTRL))
    {
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }
    else
    {
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_OVERFLOW);
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_UNDERFLOW);
    }

    /* Clear FIFOs */
    Cy_SCB_SPI_ClearTxFifo(base);
    Cy_SCB_SPI_ClearRxFifo(base);

    /* Clear the status to allow a new transfer */
    context->status = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_GetNumTransfered
****************************************************************************//**
*
* Returns the number of data elements transferred since the last call to \ref
* Cy_SCB_SPI_Transfer.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* The number of data elements transferred.
*
*******************************************************************************/
uint32_t Cy_SCB_SPI_GetNumTransfered(CySCB_Type const *base, cy_stc_scb_spi_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->rxBufIdx);
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_GetTransferStatus
****************************************************************************//**
*
* Returns the status of the transfer operation started by
* \ref Cy_SCB_SPI_Transfer.
* This status is a bit mask and the value returned may have a multiple-bit set.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_spi_macros_xfer_status.
*
* \note
* The status is cleared by calling \ref Cy_SCB_SPI_Transfer or
* \ref Cy_SCB_SPI_AbortTransfer.
*
*******************************************************************************/
uint32_t Cy_SCB_SPI_GetTransferStatus(CySCB_Type const *base, cy_stc_scb_spi_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->status);
}


/*******************************************************************************
* Function Name: Cy_SCB_SPI_Interrupt
****************************************************************************//**
*
* This is the interrupt function for the SCB configured in the SPI mode.
* This function must be called inside the  user-defined interrupt service
* routine for \ref Cy_SCB_SPI_Transfer to work.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_SPI_Interrupt(CySCB_Type *base, cy_stc_scb_spi_context_t *context)
{
     bool locXferErr = false;

    /* Wake up on the slave select condition */
    if (0UL != (CY_SCB_SPI_INTR_WAKEUP & Cy_SCB_GetSpiInterruptStatusMasked(base)))
    {
        Cy_SCB_ClearSpiInterrupt(base, CY_SCB_SPI_INTR_WAKEUP);
    }

    /* The slave error condition */
    if (0UL != (CY_SCB_SLAVE_INTR_SPI_BUS_ERROR & Cy_SCB_GetSlaveInterruptStatusMasked(base)))
    {
        locXferErr       = true;
        context->status |= CY_SCB_SPI_SLAVE_TRANSFER_ERR;

        Cy_SCB_ClearSlaveInterrupt(base, CY_SCB_SLAVE_INTR_SPI_BUS_ERROR);
    }

    /* The RX overflow error condition */
    if (0UL != (CY_SCB_RX_INTR_OVERFLOW & Cy_SCB_GetRxInterruptStatusMasked(base)))
    {
        locXferErr       = true;
        context->status |= CY_SCB_SPI_TRANSFER_OVERFLOW;

        Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_OVERFLOW);
    }

    /* The TX underflow error condition or slave complete data transfer */
    if (0UL != (CY_SCB_TX_INTR_UNDERFLOW & Cy_SCB_GetTxInterruptStatusMasked(base)))
    {
        locXferErr       = true;
        context->status |= CY_SCB_SPI_TRANSFER_UNDERFLOW;

        Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UNDERFLOW);
    }

    /* Report an error, use a callback */
    if (locXferErr)
    {
        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_SPI_TRANSFER_ERR_EVENT);
        }
    }

    /* RX direction */
    if (0UL != (CY_SCB_RX_INTR_LEVEL & Cy_SCB_GetRxInterruptStatusMasked(base)))
    {
        HandleReceive(base, context);

        Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
    }

    /* TX direction */
    if (0UL != (CY_SCB_TX_INTR_LEVEL & Cy_SCB_GetTxInterruptStatusMasked(base)))
    {
        HandleTransmit(base, context);

        Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_LEVEL);
    }

    /* The transfer is complete: all data is loaded in the TX FIFO
    * and all data is read from the RX FIFO
    */
    if ((0UL != (context->status & CY_SCB_SPI_TRANSFER_ACTIVE)) &&
        (0UL == context->rxBufSize) && (0UL == context->txBufSize))
    {
        /* The transfer is complete */
        context->status &= (uint32_t) ~CY_SCB_SPI_TRANSFER_ACTIVE;

        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_SPI_TRANSFER_CMPLT_EVENT);
        }
    }
}


/*******************************************************************************
* Function Name: HandleReceive
****************************************************************************//**
*
* Reads data from RX FIFO into the buffer provided by \ref Cy_SCB_SPI_Transfer.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void HandleReceive(CySCB_Type *base, cy_stc_scb_spi_context_t *context)
{
    /* Get data in RX FIFO */
    uint32_t numToCopy = Cy_SCB_GetNumInRxFifo(base);

    /* Adjust the number to read */
    if (numToCopy > context->rxBufSize)
    {
        numToCopy = context->rxBufSize;
    }

    /* Move the buffer */
    context->rxBufIdx  += numToCopy;
    context->rxBufSize -= numToCopy;

    /* Read data from RX FIFO */
    if (NULL != context->rxBuf)
    {
        uint8_t *buf = (uint8_t *) context->rxBuf;

        Cy_SCB_ReadArrayNoCheck(base, context->rxBuf, numToCopy);

        buf = &buf[(Cy_SCB_IsRxDataWidthByte(base) ? (numToCopy) : (2UL * numToCopy))];
        context->rxBuf = (void *) buf;
    }
    else
    {
        /* Discard read data. */
        DiscardArrayNoCheck(base, numToCopy);
    }

    if (0UL == context->rxBufSize)
    {
        /* Disable the RX level interrupt */
        Cy_SCB_SetRxInterruptMask(base, (Cy_SCB_GetRxInterruptMask(base) & (uint32_t) ~CY_SCB_RX_INTR_LEVEL));
    }
    else
    {
        uint32_t level = (_FLD2BOOL(SCB_SPI_CTRL_MASTER_MODE, base->SPI_CTRL)) ?
                                    Cy_SCB_GetFifoSize(base) : (Cy_SCB_GetFifoSize(base) / 2UL);

        if (context->rxBufSize < level)
        {
            Cy_SCB_SetRxFifoLevel(base, (context->rxBufSize - 1UL));
        }
    }
}


/*******************************************************************************
* Function Name: HandleTransmit
****************************************************************************//**
*
* Loads TX FIFO with data provided by \ref Cy_SCB_SPI_Transfer.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_spi_context_t allocated
* by the user. The structure is used during the SPI operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void HandleTransmit(CySCB_Type *base, cy_stc_scb_spi_context_t *context)
{
    uint32_t numToCopy;
    uint32_t fifoSize = Cy_SCB_GetFifoSize(base);

    numToCopy = fifoSize - Cy_SCB_GetNumInTxFifo(base);

    /* Adjust the number to load */
    if (numToCopy > context->txBufSize)
    {
        numToCopy = context->txBufSize;
    }

    /* Move the buffer */
    context->txBufIdx  += numToCopy;
    context->txBufSize -= numToCopy;

    /* Load TX FIFO with data */
    if (NULL != context->txBuf)
    {
        uint8_t *buf = (uint8_t *) context->txBuf;

        Cy_SCB_WriteArrayNoCheck(base, context->txBuf, numToCopy);

        buf = &buf[(Cy_SCB_IsTxDataWidthByte(base) ? (numToCopy) : (2UL * numToCopy))];
        context->txBuf = (void *) buf;
    }
    else
    {
        Cy_SCB_WriteDefaultArrayNoCheck(base, CY_SCB_SPI_DEFAULT_TX, numToCopy);
    }

    if (0UL == context->txBufSize)
    {
        /* Data is transferred into TX FIFO */
        context->status |= CY_SCB_SPI_TRANSFER_IN_FIFO;

        /* Disable the TX level interrupt */
        Cy_SCB_SetTxInterruptMask(base, (Cy_SCB_GetTxInterruptMask(base) & (uint32_t) ~CY_SCB_TX_INTR_LEVEL));

        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_SPI_TRANSFER_IN_FIFO_EVENT);
        }
    }
}


/*******************************************************************************
* Function Name: DiscardArrayNoCheck
****************************************************************************//**
*
* Reads the number of data elements from the SPI RX FIFO. The read data is
* discarded. Before calling this function, make sure that RX FIFO has
* enough data elements to read.
*
* \param base
* The pointer to the SPI SCB instance.
*
* \param size
* The number of data elements to read.
*
*******************************************************************************/
static void DiscardArrayNoCheck(CySCB_Type const *base, uint32_t size)
{
    while (size > 0UL)
    {
        (void) Cy_SCB_SPI_Read(base);
        --size;
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSCB */

/* [] END OF FILE */

