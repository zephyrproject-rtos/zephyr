/***************************************************************************//**
* \file cy_scb_i2c.c
* \version 2.20
*
* Provides I2C API implementation of the SCB driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_scb_i2c.h"

#ifdef CY_IP_MXSCB

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*        Function Prototypes
***************************************/

static void SlaveHandleAddress     (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void SlaveHandleDataReceive (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void SlaveHandleDataTransmit(CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void SlaveHandleStop        (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);

static void MasterHandleEvents      (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void MasterHandleDataTransmit(CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void MasterHandleDataReceive (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void MasterHandleStop        (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
static void MasterHandleComplete    (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);

static cy_en_scb_i2c_status_t HandleStatus(CySCB_Type *base, uint32_t status,
                                           cy_stc_scb_i2c_context_t *context);
static uint32_t WaitOneUnit(uint32_t *timeout);


/*******************************************************************************
* Function Name: Cy_SCB_I2C_Init
****************************************************************************//**
*
* Initializes the SCB for the I2C operation.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param config
* The pointer to the configuration structure \ref cy_stc_scb_i2c_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_Init(CySCB_Type *base, cy_stc_scb_i2c_config_t const *config, cy_stc_scb_i2c_context_t *context)
{
    /* Input parameters verification */
    if ((NULL == base) || (NULL == config) || (NULL == context))
    {
        return CY_SCB_I2C_BAD_PARAM;
    }

    CY_ASSERT_L3(CY_SCB_I2C_IS_MODE_VALID(config->i2cMode));
    CY_ASSERT_L2((config->useRxFifo) ? (!config->acceptAddrInFifo) : true);
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID     (config->slaveAddress));
    CY_ASSERT_L2(CY_SCB_I2C_IS_ADDR_MASK_VALID(config->slaveAddressMask));

    /* Configure the I2C interface */
    base->CTRL = _BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, config->acceptAddrInFifo)   |
                 _BOOL2FLD(SCB_CTRL_EC_AM_MODE, config->enableWakeFromSleep) |
                  SCB_CTRL_BYTE_MODE_Msk;

    base->I2C_CTRL = _BOOL2FLD(SCB_I2C_CTRL_S_GENERAL_IGNORE, !config->ackGeneralAddr) |
                     _VAL2FLD(CY_SCB_I2C_CTRL_MODE, (uint32_t) config->i2cMode);

    /* Configure the RX direction */
    base->RX_CTRL      =  CY_SCB_I2C_RX_CTRL;
    base->RX_FIFO_CTRL = (config->useRxFifo ? (CY_SCB_I2C_FIFO_SIZE - 1UL) : 0UL);

    /* Set the default address and mask */
    base->RX_MATCH = _VAL2FLD(SCB_RX_MATCH_ADDR, ((uint32_t) config->slaveAddress << 1UL)) |
                     _VAL2FLD(SCB_RX_MATCH_MASK, (uint32_t)  config->slaveAddressMask);

    /* Configure the TX direction */
    base->TX_CTRL      = CY_SCB_I2C_TX_CTRL;
    base->TX_FIFO_CTRL = (config->useTxFifo ? CY_SCB_I2C_HALF_FIFO_SIZE : 1UL);

    /* Configure interrupt sources */
    base->INTR_SPI_EC_MASK = 0UL;
    base->INTR_I2C_EC_MASK = 0UL;
    base->INTR_RX_MASK     = 0UL;
    base->INTR_TX_MASK     = 0UL;
    base->INTR_M_MASK      = 0UL;

    base->INTR_S_MASK = (CY_SCB_I2C_MASTER != config->i2cMode) ? CY_SCB_I2C_SLAVE_INTR : 0UL;

    /* Initialize the context */
    context->useRxFifo = config->useRxFifo;
    context->useTxFifo = config->useTxFifo;

    context->state = CY_SCB_I2C_IDLE;

    /* Master-specific */
    context->masterStatus     = 0UL;
    context->masterBufferIdx  = 0UL;

    /* Slave-specific */
    context->slaveStatus       = 0UL;

    context->slaveRxBufferIdx  = 0UL;
    context->slaveRxBufferSize = 0UL;

    context->slaveTxBufferIdx  = 0UL;
    context->slaveTxBufferSize = 0UL;

    /* Unregister callbacks */
    context->cbEvents = NULL;
    context->cbAddr   = NULL;

    return CY_SCB_I2C_SUCCESS;
}


/*******************************************************************************
*  Function Name: Cy_SCB_I2C_DeInit
****************************************************************************//**
*
* De-initializes the SCB block and returns register values to default.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
void Cy_SCB_I2C_DeInit(CySCB_Type *base)
{
    /* Returns block registers into the default state */
    base->CTRL     = CY_SCB_CTRL_DEF_VAL;
    base->I2C_CTRL = CY_SCB_I2C_CTRL_DEF_VAL;
    base->I2C_CFG  = CY_SCB_I2C_CFG_DEF_VAL;

    base->RX_CTRL      = CY_SCB_RX_CTRL_DEF_VAL;
    base->RX_FIFO_CTRL = 0UL;
    base->RX_MATCH     = 0UL;

    base->TX_CTRL      = CY_SCB_TX_CTRL_DEF_VAL;
    base->TX_FIFO_CTRL = 0UL;

    base->INTR_SPI_EC_MASK = 0UL;
    base->INTR_I2C_EC_MASK = 0UL;
    base->INTR_RX_MASK     = 0UL;
    base->INTR_TX_MASK     = 0UL;
    base->INTR_M_MASK      = 0UL;
    base->INTR_S_MASK      = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_Disable
****************************************************************************//**
*
* Disables the SCB block and clears context statuses.
* Note that after the block is disabled, the TX and RX FIFOs and hardware
* statuses are cleared. Also, the hardware stops driving the output and
* ignores the input.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* Calling this function when I2C is busy (master preforms transaction or slave
* was address and communicates with master) may cause transaction corruption
* because the hardware stops driving the outputs and ignores the inputs.
* Ensure that I2C is not busy before calling this function.
*
*******************************************************************************/
void Cy_SCB_I2C_Disable(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    base->CTRL &= (uint32_t) ~SCB_CTRL_ENABLED_Msk;

    /* Set the state to default and clear statuses */
    context->state        = CY_SCB_I2C_IDLE;
    context->masterStatus = 0UL;
    context->slaveStatus  = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_DeepSleepCallback
****************************************************************************//**
*
* This function handles the transition of the I2C SCB into and out of
* Deep Sleep mode. It prevents the device from entering Deep Sleep
* mode if the I2C slave or master is actively communicating.
* The following behavior of the I2C SCB depends on whether the SCB block is
* wakeup-capable or not:
* * The SCB <b>wakeup-capable</b>: on the incoming I2C slave address, the slave
*   receives address and stretches the clock until the device is awoken from
*   Deep Sleep mode. If the slave address occurs before the device enters
*   Deep Sleep mode, the device will not enter Deep Sleep mode.
*   Only the I2C slave can be configured to be a wakeup source from Deep Sleep
*   mode.
* * The SCB is <b>not wakeup-capable</b>: the I2C is disabled. It is enabled when
*   the device failed to enter Deep Sleep mode or when it is awoken from Deep
*   Sleep mode. While the I2C is disabled, it does stops driving the outputs and
*   ignores the inputs. The slave NACKs all incoming addresses.
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
* For proper operation, when the I2C slave is configured to be a wakeup source
* from Deep Sleep mode, this function must be copied and modified by the user.
* The I2C clock disable code must be inserted in the \ref CY_SYSPM_BEFORE_TRANSITION
* and clock enable code in the \ref CY_SYSPM_AFTER_TRANSITION mode processing.
*
*******************************************************************************/
cy_en_syspm_status_t Cy_SCB_I2C_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    CySCB_Type *locBase = (CySCB_Type *) callbackParams->base;
    cy_stc_scb_i2c_context_t *locContext = (cy_stc_scb_i2c_context_t *) callbackParams->context;

    cy_en_syspm_status_t retStatus = CY_SYSPM_FAIL;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            /* Disable the slave interrupt sources to protect the state */
            Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_CLEAR_ALL_INTR_SRC);

            /* If the I2C is in the IDLE state, it is ready for Deep Sleep mode
            * (either the master or the slave is not busy),
            * otherwise return fail and restore the slave interrupt sources.
            */
            if (CY_SCB_I2C_IDLE == locContext->state)
            {
                if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
                {
                    /* The SCB is wakeup-capable: do not restore the address
                    * match and general call interrupt sources. The next
                    * transaction intended to the slave will be paused
                    * (SCL is stretched) before the address is ACKed because
                    * the corresponding interrupt source is disabled.
                    */
                    Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR_NO_ADDR);
                }
                else
                {
                    /* The SCB is NOT wakeup-capable: disable the I2C. The slave
                    * stops responding to the master and the master stops
                    * driving the bus until the I2C is enabled. This happens
                    * when the device failed to enter into Deep Sleep mode or it
                    * is awaken from Deep Sleep mode.
                    */
                    Cy_SCB_I2C_Disable(locBase, locContext);
                    Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR);
                }

                retStatus = CY_SYSPM_SUCCESS;
            }
            else
            {
                /* Restore the slave interrupt sources */
                Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR);
            }
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            /* The other driver is not ready for Deep Sleep mode. Restore
            * Active mode configuration.
            */

            if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
            {
                /* The SCB is wakeup-capable: restore the slave interrupt sources */
                Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR);
            }
            else
            {
                /* The SCB is NOT wakeup-capable: enable the I2C to operate */
                Cy_SCB_I2C_Enable(locBase);
            }

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* This code executes inside the critical section. Enabling the
            * active interrupt source makes the interrupt pending in the NVIC.
            * However, the interrupt processing is delayed until the code exits
            * the critical section. The pending interrupt force WFI instruction
            * does nothing and the device remains in Active mode.
            */

            if (_FLD2BOOL(SCB_CTRL_EC_AM_MODE, locBase->CTRL))
            {
                /* The SCB is wakeup-capable: enable the I2C wakeup interrupt
                * source. If any transaction was paused, the I2C interrupt
                * becomes pending and prevents entering Deep Sleep mode.
                * The transaction continues as soon as the global interrupts
                * are enabled.
                */
                Cy_SCB_SetI2CInterruptMask(locBase, CY_SCB_I2C_INTR_WAKEUP);

                /* Disable SCB clock */
                locBase->I2C_CFG &= (uint32_t) ~CY_SCB_I2C_CFG_CLK_ENABLE_Msk;

                /* IMPORTANT (replace line above for the CY8CKIT-062 rev-08):
                * for proper entering Deep Sleep mode the I2C clock must be disabled.
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
                * for proper exiting Deep Sleep, the I2C clock must be enabled.
                * This code must be inserted by the user because the driver
                * does not have access to the clock.
                */

                /* The SCB is wakeup-capable: disable the I2C wakeup interrupt
                * source and restore slave interrupt sources.
                */
                Cy_SCB_SetI2CInterruptMask  (locBase, CY_SCB_CLEAR_ALL_INTR_SRC);
                Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR);
            }
            else
            {
                /* The SCB is NOT wakeup-capable: enable the I2C to operate */
                Cy_SCB_I2C_Enable(locBase);
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
* Function Name: Cy_SCB_I2C_HibernateCallback
****************************************************************************//**
*
* This function handles the transition of the I2C SCB block into Hibernate
* mode. It prevents the device from entering Hibernate mode if the I2C slave or
* master is actively communicating.
* If the I2C is ready to enter Hibernate mode, it is disabled. If the device failed
* to enter Hibernate mode, the I2C is enabled. After the I2C is disabled, it stops
* driving the outputs and ignores the inputs. The slave NACKs all incoming
* addresses.
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
cy_en_syspm_status_t Cy_SCB_I2C_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    CySCB_Type *locBase = (CySCB_Type *) callbackParams->base;
    cy_stc_scb_i2c_context_t *locContext = (cy_stc_scb_i2c_context_t *) callbackParams->context;

    cy_en_syspm_status_t retStatus = CY_SYSPM_FAIL;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            /* Disable the slave interrupt sources to protect the state */
            Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_CLEAR_ALL_INTR_SRC);

            /* If the I2C is in the IDLE state, it is ready for Hibernate mode
            * (either the master or the slave is not busy).
            * Otherwise, return fail and restore the slave interrupt sources.
            */
            if (CY_SCB_I2C_IDLE == locContext->state)
            {
                /* Disable the I2C. The slave stops responding to the master and
                * the master stops driving the bus until the I2C is enabled.
                * This happens if the device failed to enter Hibernate mode.
                */
                Cy_SCB_I2C_Disable(locBase, locContext);

                retStatus = CY_SYSPM_SUCCESS;
            }

            /* Restore the slave interrupt sources */
            Cy_SCB_SetSlaveInterruptMask(locBase, CY_SCB_I2C_SLAVE_INTR);
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            /* The other driver is not ready for Hibernate mode. Restore the
            * Active mode configuration.
            */

            /* Enable the I2C to operate */
            Cy_SCB_I2C_Enable(locBase);

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* The SCB is not capable of waking up from Hibernate mode: do nothing */
            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SetDataRate
****************************************************************************//**
*
* Configures the SCB to work at the desired data rate.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param dataRateHz
* The desired data Rate in Hz.
*
* \param scbClockHz
* The frequency of the clock connected to the SCB in Hz.
*
* \return
* The achieved data rate in Hz.
*
* \note
* This function does not change the values of the clock divider connected
* to the SCB, it changes only the SCB clock oversample registers. If this
* function is not able to achieve the desired data rate, then the clock
* divider must be adjusted. Call this function only while the SCB is
* disabled. For the slave, this function only checks that the attached clock is
* fast enough to meet the desired data rate. It does not change any registers.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SetDataRate(CySCB_Type *base, uint32_t dataRateHz, uint32_t scbClockHz)
{
    CY_ASSERT_L2(scbClockHz > 0UL);
    CY_ASSERT_L2(CY_SCB_I2C_IS_DATA_RATE_VALID(dataRateHz));

    uint32_t actualDataRate = 0UL;

    if (((uint32_t) CY_SCB_I2C_SLAVE) == _FLD2VAL(CY_SCB_I2C_CTRL_MODE, base->I2C_CTRL))
    {
        actualDataRate = Cy_SCB_I2C_GetDataRate(base, scbClockHz);

        /* Use an analog filter for the slave */
        base->RX_CTRL &= (uint32_t) ~SCB_RX_CTRL_MEDIAN_Msk;
        base->I2C_CFG  = CY_SCB_I2C_ENABLE_ANALOG_FITLER;
    }
    else
    {
        if ((scbClockHz > 0U) && (dataRateHz > 0U))
        {
            uint32_t sclLow;
            uint32_t sclHigh;
            uint32_t lowPhase;
            uint32_t highPhase;

            /* Convert SCB clock and data rate in kHz */
            uint32_t scbClockKHz = scbClockHz / 1000UL;
            uint32_t dataRateKHz = dataRateHz / 1000UL;

            /* Get period of the SCB clock in ns */
            uint32_t period = 1000000000UL / scbClockHz;

            /* Get duration of SCL low and high for the selected data rate */
            if (dataRateHz <= CY_SCB_I2C_STD_DATA_RATE)
            {
                sclLow  = CY_SCB_I2C_MASTER_STD_SCL_LOW;
                sclHigh = CY_SCB_I2C_MASTER_STD_SCL_HIGH;
            }
            else if (dataRateHz <= CY_SCB_I2C_FST_DATA_RATE)
            {
                sclLow  = CY_SCB_I2C_MASTER_FST_SCL_LOW;
                sclHigh = CY_SCB_I2C_MASTER_FST_SCL_HIGH;
            }
            else
            {
                sclLow  = CY_SCB_I2C_MASTER_FSTP_SCL_LOW;
                sclHigh = CY_SCB_I2C_MASTER_FSTP_SCL_HIGH;
            }

            /* Get low phase minimum value in SCB clocks */
            lowPhase = sclLow / period;
            while (((period * lowPhase) < sclLow) && (lowPhase < CY_SCB_I2C_LOW_PHASE_MAX))
            {
                ++lowPhase;
            }

            /* Get high phase minimum value in SCB clocks */
            highPhase = sclHigh / period;
            while (((period * highPhase) < sclHigh) && (highPhase < CY_SCB_I2C_HIGH_PHASE_MAX))
            {
                ++highPhase;
            }

            /* Get actual data rate */
            actualDataRate = scbClockKHz / (lowPhase + highPhase);

            uint32_t idx = 0UL;
            while ((actualDataRate > dataRateKHz) &&
                   ((lowPhase + highPhase) < CY_SCB_I2C_DUTY_CYCLE_MAX))
            {
                /* Increase low and high phase to reach desired data rate */
                if (0UL != (idx & 0x1UL))
                {
                    if (highPhase < CY_SCB_I2C_HIGH_PHASE_MAX)
                    {
                        highPhase++;
                    }
                }
                else
                {
                    if (lowPhase < CY_SCB_I2C_LOW_PHASE_MAX)
                    {
                        lowPhase++;
                    }
                }

                idx++;

                /* Update actual data rate */
                actualDataRate = scbClockKHz / (lowPhase + highPhase);
            }

            /* Set filter configuration based on actual data rate */
            if (actualDataRate > CY_SCB_I2C_FST_DATA_RATE)
            {
                /* Use a digital filter */
                base->RX_CTRL |= (uint32_t) SCB_RX_CTRL_MEDIAN_Msk;
                base->I2C_CFG  = CY_SCB_I2C_DISABLE_ANALOG_FITLER;
            }
            else
            {
                /* Use an analog filter */
                base->RX_CTRL &= (uint32_t) ~SCB_RX_CTRL_MEDIAN_Msk;
                base->I2C_CFG  = CY_SCB_I2C_ENABLE_ANALOG_FITLER;
            }

            /* Set phase low and high */
            Cy_SCB_I2C_MasterSetLowPhaseDutyCycle (base, lowPhase);
            Cy_SCB_I2C_MasterSetHighPhaseDutyCycle(base, highPhase);

            /* Convert actual data rate in Hz */
            actualDataRate = scbClockHz / (lowPhase + highPhase);
        }
    }

    return (actualDataRate);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_GetDataRate
****************************************************************************//**
*
* Returns the data rate for the selected SCB block.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param scbClockHz
* The frequency of the clock connected to the SCB in Hz.
*
* \return
* The data rate in Hz.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_GetDataRate(CySCB_Type const *base, uint32_t scbClockHz)
{
    CY_ASSERT_L2(scbClockHz > 0UL);

    uint32_t actualDataRate = 0UL;

    if (((uint32_t) CY_SCB_I2C_SLAVE) == _FLD2VAL(CY_SCB_I2C_CTRL_MODE, base->I2C_CTRL))
    {
        /* Check the clock frequency range to get maximum supported data rate */
        if ((scbClockHz >= CY_SCB_I2C_SLAVE_FST_CLK_MIN) && (scbClockHz <= CY_SCB_I2C_SLAVE_FST_CLK_MAX))
        {
            actualDataRate = CY_SCB_I2C_FST_DATA_RATE;
        }
        else if ((scbClockHz >= CY_SCB_I2C_SLAVE_STD_CLK_MIN) && (scbClockHz <= CY_SCB_I2C_SLAVE_STD_CLK_MAX))
        {
            actualDataRate = CY_SCB_I2C_STD_DATA_RATE;
        }
        else if ((scbClockHz >= CY_SCB_I2C_SLAVE_FSTP_CLK_MIN) && (scbClockHz <= CY_SCB_I2C_SLAVE_FSTP_CLK_MAX))
        {
            actualDataRate = CY_SCB_I2C_FSTP_DATA_RATE;
        }
        else
        {
            /* The clock frequency is too low or it gets to the gap between
            * Fast and Fast Plus data rates.
            */
            actualDataRate = 0UL;
        }
    }
    else
    {
        if (scbClockHz > 0U)
        {
            uint32_t dutyCycle;

            /* Get number of clocks in one SCL period */
            dutyCycle = _FLD2VAL(SCB_I2C_CTRL_LOW_PHASE_OVS,  base->I2C_CTRL) +
                        _FLD2VAL(SCB_I2C_CTRL_HIGH_PHASE_OVS, base->I2C_CTRL) +
                        2UL;

            /* Calculate the actual data rate */
            actualDataRate = (scbClockHz / dutyCycle);
        }
    }

    return (actualDataRate);
}


/*******************************************************************************
*                         I2C Slave API
*******************************************************************************/

/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveGetStatus
****************************************************************************//**
*
* Returns the current I2C slave status.
* This status is a bit mask and the value returned may have multiple bits set.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_i2c_macros_slave_status.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SlaveGetStatus(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->slaveStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveConfigReadBuf
****************************************************************************//**
*
* Configures the buffer pointer and the read buffer size. This is the buffer
* from which the master reads data. After this function is called, data
* transfer from the read buffer to the master is handled by
* \ref Cy_SCB_I2C_Interrupt.
*
* When the Read transaction is completed (master generated Stop, ReStart or
* error occurred), the \ref CY_SCB_I2C_SLAVE_RD_BUSY status is cleared and
* the \ref CY_SCB_I2C_SLAVE_RD_CMPLT is set. Also
* the \ref CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT event is generated.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param buffer
* The pointer to the buffer with data to be read by the master.
*
* \param size
* Size of the buffer.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* * The Read buffer must not be modified and stay allocated until it has been
*   read by the master.
* * If this function has not been called, and the master tries to read data
*   from the slave a \ref CY_SCB_I2C_DEFAULT_TX is returned to the master.
* * If the master tries to read more bytes than available in the Read buffer,
*   a \ref CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT event occurs. The
*   \ref CY_SCB_I2C_DEFAULT_TX is returned to the master if the buffer remains
*   empty after an event notification.
*
*******************************************************************************/
void Cy_SCB_I2C_SlaveConfigReadBuf(CySCB_Type const *base, uint8_t *buffer, uint32_t size,
                                   cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L1(CY_SCB_IS_I2C_BUFFER_VALID(buffer, size));

    /* Suppress a compiler warning about unused variables */
    (void) base;

    context->slaveTxBuffer     = buffer;
    context->slaveTxBufferSize = size;
    context->slaveTxBufferIdx  = 0UL;
    context->slaveTxBufferCnt  = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveAbortRead
****************************************************************************//**
*
* Aborts the configured slave read buffer to be read by the master.
* If the master reads and "abort operation" is requested, the
* \ref CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT event occurs. The
* \ref CY_SCB_I2C_DEFAULT_TX is returned to the master if the buffer remains
* empty after the event notification.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \sideeffect
* If the TX FIFO is used, this function clears it.
* The TX FIFO clear operation also clears the shift register, thus
* the shifter can be cleared in the middle of a data element transfer,
* corrupting it. The data element corruption means that all bits that have
* not been transmitted are transmitted as "ones" on the bus.
*
*******************************************************************************/
void Cy_SCB_I2C_SlaveAbortRead(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t intrState;

    /* Suppress a compiler warning about unused variables */
    (void) base;

    intrState = Cy_SysLib_EnterCriticalSection();

    /* Reset index to make write buffer empty */
    context->slaveTxBufferSize = 0UL;

    if ((context->useTxFifo) &&
        (0UL != (CY_SCB_I2C_SLAVE_RD_BUSY & context->slaveStatus)))
    {
        /* Clear TX FIFO from available data */
        Cy_SCB_ClearTxFifo(base);
    }

    Cy_SysLib_ExitCriticalSection(intrState);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveGetReadTransferCount
****************************************************************************//**
*
* Returns the number of bytes read by the master since the last time
* \ref Cy_SCB_I2C_SlaveConfigReadBuf is called.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* The number of bytes read by the master.
*
* \note
* * This function returns an invalid value if a read transaction was
*   aborted or any listed event occurs during the transaction:
*   \ref CY_SCB_I2C_SLAVE_ARB_LOST, \ref CY_SCB_I2C_SLAVE_BUS_ERR.
* * This number is updated only when a transaction completes, either through
*   an error or successfully.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SlaveGetReadTransferCount(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->slaveTxBufferCnt);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveClearReadStatus
****************************************************************************//**
*
* Clears the read status and error conditions flags and returns their values.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_i2c_macros_slave_status.
*
* \note
* The \ref CY_SCB_I2C_SLAVE_RD_BUSY flag is not cleared.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SlaveClearReadStatus(CySCB_Type const *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t retStatus;

    /* Suppress a compiler warning about unused variables */
    (void) base;

    retStatus = (context->slaveStatus & CY_SCB_I2C_SLAVE_RD_CLEAR);
    context->slaveStatus &= (uint32_t) ~CY_SCB_I2C_SLAVE_RD_CLEAR;

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveConfigWriteBuf
****************************************************************************//**
*
* Configures the buffer pointer and size of the write buffer. This is the buffer
* that the master writes data to. After this function is called data transfer
* from the master into the write buffer is handled by \ref Cy_SCB_I2C_Interrupt.
*
* When write transaction is completed (master generated Stop, ReStart or
* error occurred) the \ref CY_SCB_I2C_SLAVE_WR_BUSY status is cleared and
* the \ref CY_SCB_I2C_SLAVE_WR_CMPLT is set, also
* the \ref CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT event is generated.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param buffer
* The pointer to buffer to store data written by the master.
*
* \param size
* Size of the buffer.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* * The write buffer must not be modified and must stay allocated until it has been
*   written by the master.
* * If this function has not been called and the master tries to write data,
*   the first byte is NAKed and discarded.
* * If the master writes more bytes than the slave can store in the write buffer,
*   the \ref CY_SCB_I2C_SLAVE_WR_OVRFL status is set and the slave will NACK last
*   byte, unless the RX FIFO is used. Then the slave will NAK only after
*   RX FIFO becomes full.
* * If the RX FIFO is used, the minimum write buffer size is automatically
*   the size of the RX FIFO. If a write buffer is less than the RX FIFO size, extra
*   bytes are ACKed and stored into RX FIFO but ignored by firmware.
*
*******************************************************************************/
void Cy_SCB_I2C_SlaveConfigWriteBuf(CySCB_Type const *base, uint8_t *buffer, uint32_t size,
                                    cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L1(CY_SCB_IS_I2C_BUFFER_VALID(buffer, size));

    /* Suppress a compiler warning about unused variables */
    (void) base;

    context->slaveRxBuffer     = buffer;
    context->slaveRxBufferSize = size;
    context->slaveRxBufferIdx  = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveAbortWrite
****************************************************************************//**
*
* Aborts the configured slave write buffer to be written by the master.
* If master writes and "abort operation" are requested, the next incoming byte will
* be NAKed.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* If the RX FIFO is used, the NAK will not be sent until RX FIFO
* becomes full, however bytes accepted after an abort request are ignored.
*
*******************************************************************************/
void Cy_SCB_I2C_SlaveAbortWrite(CySCB_Type *base,  cy_stc_scb_i2c_context_t *context)
{
    uint32_t intrState;

    /* Suppress a compiler warning about unused variables */
    (void) base;

    intrState = Cy_SysLib_EnterCriticalSection();

    /* Reset index to make read buffer empty */
    context->slaveRxBufferSize = 0UL;

    if ((context->useRxFifo) &&
        (0UL != (CY_SCB_I2C_SLAVE_WR_BUSY & context->slaveStatus)))
    {
        /* Configure to NACK when RX FIFO is full and disable RX level
        * interrupt sources to stop getting data from RX FIFO.
        */
        base->I2C_CTRL |= SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk;
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }

    Cy_SysLib_ExitCriticalSection(intrState);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveGetWriteTransferCount
****************************************************************************//**
*
* Returns the number of bytes written by the master since the last time
* \ref Cy_SCB_I2C_SlaveConfigWriteBuf is called.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* Number of bytes written by the master.
*
* \note
* * This function returns an invalid value if write transaction is
*   aborted or any listed event occurs during the transaction:
*   \ref CY_SCB_I2C_SLAVE_ARB_LOST, \ref CY_SCB_I2C_SLAVE_BUS_ERR.
* * This number is updated only when the transaction completes, either through
*   an error or successfully.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SlaveGetWriteTransferCount(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->slaveRxBufferIdx);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveClearWriteStatus
****************************************************************************//**
*
* Clears the write status flags and error condition flags and returns their
* values.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_i2c_macros_slave_status.
*
* \note
* The \ref CY_SCB_I2C_SLAVE_WR_BUSY flag is not cleared.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_SlaveClearWriteStatus(CySCB_Type const *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t retStatus;

    /* Suppress a compiler warning about unused variables */
    (void) base;

    retStatus = (context->slaveStatus & CY_SCB_I2C_SLAVE_WR_CLEAR);
    context->slaveStatus &= (uint32_t) ~CY_SCB_I2C_SLAVE_WR_CLEAR;

    return (retStatus);
}


/*******************************************************************************
*                         I2C Master API: High level
*******************************************************************************/

/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterGetStatus
****************************************************************************//**
*
* Returns the current I2C master status.
* This status is a bit mask and the value returned may have multiple bits set.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_i2c_macros_master_status.
* Note that not all I2C master statuses are returned by this function. Refer to
* more details of each status.
*
* \note
* Status is cleared by calling \ref Cy_SCB_I2C_MasterRead or
* \ref Cy_SCB_I2C_MasterWrite.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_MasterGetStatus(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->masterStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterRead
****************************************************************************//**
*
* This function configures the master to automatically read an entire buffer
* of data from the slave device. After the transaction is initiated by this
* function it returns and \ref Cy_SCB_I2C_Interrupt manages further data
* transfer.
*
* When a read transaction is completed (requested number of bytes are read or
* error occurred) the \ref CY_SCB_I2C_MASTER_BUSY status is cleared and
* the \ref CY_SCB_I2C_MASTER_RD_CMPLT_EVENT event is generated.
*
* Note that the master must read at least one byte.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param xferConfig
* Master transfer configuration structure
* \ref cy_stc_scb_i2c_master_xfer_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* * The buffer must not be modified and must stay allocated until read operation
*   completion.
*
* * \ref Cy_SCB_I2C_MasterRead requests the SCB hardware to generate a Start
*   Condition. The hardware will not generate the Start while the I2C bus is busy.
*   The SCB hardware sets the busy status after the Start detection, and clears
*   it on the Stop detection. Noise caused by the ESD or other events may cause
*   an erroneous Start condition on the bus. Then, the master will never generate
*   a Start condition because the hardware assumes the bus is busy. If this occurs,
*   the \ref Cy_SCB_I2C_MasterGetStatus returns \ref CY_SCB_I2C_MASTER_BUSY
*   status and the transaction will never finish. The option is to implement a
*   timeout to detect the transfer completion. If the transfer never completes,
*   the SCB needs a reset by calling the \ref Cy_SCB_I2C_Disable and
*   \ref Cy_SCB_I2C_Enable functions. The \ref Cy_SCB_I2C_MasterAbortRead
*   function will not work, the block must be reset.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterRead(CySCB_Type *base,
                            cy_stc_scb_i2c_master_xfer_config_t *xferConfig,
                            cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L1(xferConfig != NULL);
    CY_ASSERT_L1(CY_SCB_IS_BUFFER_VALID  (xferConfig->buffer, xferConfig->bufferSize));
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID(xferConfig->slaveAddress));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    /* Disable I2C slave interrupt sources to protect state */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    if (0UL != (CY_SCB_I2C_IDLE_MASK & context->state))
    {
        uint32_t intrState;

        /* Set address byte (bit0 = 1, read direction) */
        uint32_t address = _VAL2FLD(CY_SCB_I2C_ADDRESS, xferConfig->slaveAddress) |
                         (uint32_t) CY_SCB_I2C_READ_XFER;

        /* Setup context */
        context->masterStatus     = CY_SCB_I2C_MASTER_BUSY;

        context->masterBuffer     = xferConfig->buffer;
        context->masterBufferSize = xferConfig->bufferSize;
        context->masterBufferIdx  = 0UL;
        context->masterNumBytes   = 0UL;
        context->masterPause      = xferConfig->xferPending;
        context->masterRdDir      = true;

        /* Clean-up hardware before transfer. Note RX FIFO is empty at here. */
        Cy_SCB_ClearMasterInterrupt(base, CY_SCB_I2C_MASTER_INTR_ALL);
        Cy_SCB_ClearTxFifo(base);

        if (CY_SCB_I2C_IDLE == context->state)
        {
            /* Put the address in the TX FIFO, then generate a Start condition. 
            * This sequence ensures that after the Start condition generation 
            * the address is available to be sent onto the bus.
            */
            Cy_SCB_WriteTxFifo(base, address);
            base->I2C_M_CMD = SCB_I2C_M_CMD_M_START_ON_IDLE_Msk;
        }
        else
        {
            /* Generate a ReStart condition.
            * If the previous transfer was read, NACK is generated before
            * ReStart to complete the previous transfer.
            */
            base->I2C_M_CMD = (SCB_I2C_M_CMD_M_START_Msk | (_FLD2BOOL(SCB_I2C_STATUS_M_READ, base->I2C_STATUS) ?
                                  SCB_I2C_M_CMD_M_NACK_Msk : 0UL));

            /* Put address in TX FIFO */
            Cy_SCB_WriteTxFifo(base, address);
        }

        /* Configure interrupt for data reception */
        if ((context->useRxFifo) && (!context->masterPause) && (context->masterBufferSize >= 2UL))
        {
            uint32_t fifoSize = CY_SCB_I2C_FIFO_SIZE;

            /* Enable Auto data ACK */
            base->I2C_CTRL |= SCB_I2C_CTRL_M_READY_DATA_ACK_Msk;

            /* Adjust level in RX FIFO */
            Cy_SCB_SetRxFifoLevel(base, (context->masterBufferSize <= fifoSize) ?
                                        (context->masterBufferSize - 2UL) : ((fifoSize / 2UL) - 1UL));

            context->state = CY_SCB_I2C_MASTER_RX1;
        }
        else
        {
            /* Adjust level in RX FIFO */
            Cy_SCB_SetRxFifoLevel(base, 0UL);

            context->state = CY_SCB_I2C_MASTER_RX0;
        }

        /* Enable interrupt sources to continue transfer.
        * Requires critical section to not cause race condition between RX and Master
        * interrupt sources.
        */
        intrState = Cy_SysLib_EnterCriticalSection();
        Cy_SCB_SetRxInterruptMask    (base, CY_SCB_RX_INTR_LEVEL);
        Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR);
        Cy_SysLib_ExitCriticalSection(intrState);

        retStatus = CY_SCB_I2C_SUCCESS;
    }

    /* Enable I2C slave interrupt sources */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_I2C_SLAVE_INTR);

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterAbortRead
****************************************************************************//**
*
* This function requests master to abort read operation by NAKing the next byte
* and generating a Stop condition. The function does not wait until these
* actions are completed. Therefore the next read operation can be initiated only
* after the \ref CY_SCB_I2C_MASTER_BUSY is cleared.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
******************************************************************************/
void Cy_SCB_I2C_MasterAbortRead(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t intrState;

    intrState = Cy_SysLib_EnterCriticalSection();

    if (0UL != (CY_SCB_I2C_MASTER_BUSY & context->masterStatus))
    {
        /* Catch state to abort read operation */
        if ((CY_SCB_I2C_MASTER_RX0 == context->state) || (CY_SCB_I2C_MASTER_RX1 == context->state))
        {
            if (context->useRxFifo)
            {
                /* Disable RX processing */
                Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

                /* Change state to request Stop generation */
                context->state = CY_SCB_I2C_MASTER_STOP;

                /* Enable ACK interrupt source to generate Stop after Start was generated */
                Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR_ALL);
            }
            else
            {
                /* Reduce buffer size to minimum */
                context->masterBufferSize = 1UL;
            }

            /* Cancel pending read operation if it was requested */
            context->masterPause = false;
        }
    }
    else
    {
        /* There are two possible states when master is not busy:
        * CY_SCB_I2C_MASTER_WAIT and CY_SCB_I2C_IDLE.
        * Do nothing for CY_SCB_I2C_IDLE.
        */
        if (CY_SCB_I2C_MASTER_WAIT == context->state)
        {
            /* Clear master previous transaction results:
            * - status to indicate that master is busy;
            * - number of bytes (only Stop is generated);
            * - cancel previous pending operation.
            */
            context->masterStatus   = CY_SCB_I2C_MASTER_BUSY;
            context->masterNumBytes = 0UL;
            context->masterPause    = false;

            /* Enable master interrupt sources to catch Stop condition */
            Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR);

            /* Complete transaction generating Stop */
            base->I2C_M_CMD = (SCB_I2C_M_CMD_M_STOP_Msk | SCB_I2C_M_CMD_M_NACK_Msk);
            context->state  = CY_SCB_I2C_MASTER_WAIT_STOP;
        }
    }

    Cy_SysLib_ExitCriticalSection(intrState);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterWrite
****************************************************************************//**
*
* This function configures the master to automatically write an entire buffer
* of data to a slave device. After the transaction is initiated by this
* function it returns and \ref Cy_SCB_I2C_Interrupt manages further data
* transfer.
*
* When a write transaction is completed (requested number of bytes are written
* or error occurred) the \ref CY_SCB_I2C_MASTER_BUSY status is cleared and
* the \ref CY_SCB_I2C_MASTER_WR_CMPLT_EVENT event is generated.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param xferConfig
* Master transfer configuration structure
* \ref cy_stc_scb_i2c_master_xfer_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* * The buffer must not be modified and must stay allocated until data has been
*   copied into TX FIFO.
*
* * \ref Cy_SCB_I2C_MasterWrite requests the SCB hardware to generate a Start
*   Condition. The hardware will not generate the Start while the I2C bus is busy.
*   The SCB hardware sets the busy status after the Start detection, and clears
*   it on the Stop detection. Noise caused by the ESD or other events may cause
*   an erroneous Start condition on the bus. Then, the master will never generate
*   a Start condition because the hardware assumes the bus is busy. If this occurs,
*   the \ref Cy_SCB_I2C_MasterGetStatus returns \ref CY_SCB_I2C_MASTER_BUSY
*   status and the transaction will never finish. The option is to implement a
*   timeout to detect the transfer completion. If the transfer never completes,
*   the SCB needs a reset by calling the \ref Cy_SCB_I2C_Disable and
*   \ref Cy_SCB_I2C_Enable functions. The \ref Cy_SCB_I2C_MasterAbortWrite
*   function will not work, the block must be reset.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterWrite(CySCB_Type *base,
                             cy_stc_scb_i2c_master_xfer_config_t *xferConfig,
                             cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L1(xferConfig != NULL);
    CY_ASSERT_L1(CY_SCB_IS_I2C_BUFFER_VALID(xferConfig->buffer, xferConfig->bufferSize));
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID  (xferConfig->slaveAddress));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    /* Disable I2C slave interrupt sources to protect state */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    if (0UL != (CY_SCB_I2C_IDLE_MASK & context->state))
    {
        uint32_t intrState;

        /* Set address byte (bit0 = 0, write direction) */
        uint32_t address = _VAL2FLD(CY_SCB_I2C_ADDRESS, xferConfig->slaveAddress);

        /* Setup context */
        context->masterStatus     = CY_SCB_I2C_MASTER_BUSY;

        context->masterBuffer     = xferConfig->buffer;
        context->masterBufferSize = xferConfig->bufferSize;
        context->masterBufferIdx  = 0UL;
        context->masterNumBytes   = 0UL;
        context->masterPause      = xferConfig->xferPending;
        context->masterRdDir      = false;

        /* Clean-up hardware before transfer. Note RX FIFO is empty at here. */
        Cy_SCB_ClearMasterInterrupt(base, CY_SCB_I2C_MASTER_INTR_ALL);
        Cy_SCB_ClearTxFifo(base);

        if (CY_SCB_I2C_IDLE == context->state)
        {
            /* Put the address in the TX FIFO, then generate a Start condition. 
            * This sequence ensures that after the Start condition generation 
            * the address is available to be sent onto the bus.
            */
            Cy_SCB_WriteTxFifo     (base, address);
            Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UNDERFLOW);
            base->I2C_M_CMD = SCB_I2C_M_CMD_M_START_ON_IDLE_Msk;
        }
        else
        {
            /* Generate a ReStart condition.
            * If the previous transfer was read, NACK is generated before
            * ReStart to complete the previous transfer.
            */
            base->I2C_M_CMD = (SCB_I2C_M_CMD_M_START_Msk | (_FLD2BOOL(SCB_I2C_STATUS_M_READ, base->I2C_STATUS) ?
                               SCB_I2C_M_CMD_M_NACK_Msk : 0UL));

            if (0U == context->masterBufferSize)
            {
                /* The address is the last byte to transfer.
                * Put the address byte in the TX FIFO and clear the TX 
                * Underflow interrupt source inside the critical section 
                * to ensure that the TX Underflow interrupt will trigger 
                * after the address byte is sent onto the bus.
                */
                intrState = Cy_SysLib_EnterCriticalSection();

                /* Put address in TX FIFO */
                Cy_SCB_WriteTxFifo     (base, address);
                Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UNDERFLOW);

                Cy_SysLib_ExitCriticalSection(intrState);
            }
            else
            {
                /* Put address in TX FIFO */
                Cy_SCB_WriteTxFifo(base, address);
            }
        }

        context->state = CY_SCB_I2C_MASTER_TX;

        /* TX FIFO is empty. Set level to start transfer */
        Cy_SCB_SetTxFifoLevel(base, (context->useTxFifo) ? CY_SCB_I2C_HALF_FIFO_SIZE : (1UL));

        /* Enable interrupt sources to continue transfer.
        * Requires critical section to not cause race condition between TX and Master
        * interrupt sources.
        */
        intrState = Cy_SysLib_EnterCriticalSection();
        Cy_SCB_SetTxInterruptMask    (base, CY_SCB_TX_INTR_LEVEL);
        Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR);
        Cy_SysLib_ExitCriticalSection(intrState);

        retStatus = CY_SCB_I2C_SUCCESS;
    }

    /* Enable I2C slave interrupt sources */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_I2C_SLAVE_INTR);

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterAbortWrite
****************************************************************************//**
*
* This function requests the master to abort write operation by generating a Stop
* condition. The function does not wait until this action is completed.
* Therefore next write operation can be initiated only after the
* \ref CY_SCB_I2C_MASTER_BUSY is cleared.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \sideeffect
* If the TX FIFO is used, it is cleared before Stop generation.
* The TX FIFO clear operation also clears shift register. Thus the shifter
* could be cleared in the middle of a data element transfer, corrupting it.
* The remaining bits to transfer within corrupted data element are
* complemented with ones.\n
* If the clear operation is requested while the master transmits the address,
* the direction of transaction is changed to read and one byte is read
* before Stop is issued. This byte is discarded.
*
*******************************************************************************/
void Cy_SCB_I2C_MasterAbortWrite(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t intrState;

    intrState = Cy_SysLib_EnterCriticalSection();

    if (0UL != (CY_SCB_I2C_MASTER_BUSY & context->masterStatus))
    {
        /* Disable TX processing */
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

        if (context->useTxFifo)
        {
            /* Clear TX FIFO to allow Stop generation */
            Cy_SCB_ClearTxFifo(base);
        }

        if ((CY_SCB_I2C_MASTER_TX == context->state) || (CY_SCB_I2C_MASTER_TX_DONE == context->state))
        {
            /* Change state to request Stop generation */
            context->state = CY_SCB_I2C_MASTER_STOP;

            /* Enable ACK interrupt source to trigger Stop generation */
            Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR_ALL);
        }

        /* Cancel pending write operation if it was requested */
        context->masterPause = false;
    }
    else
    {
        /* There are two possible states when master is not busy:
        * CY_SCB_I2C_MASTER_WAIT and CY_SCB_I2C_IDLE.
        * Do nothing for CY_SCB_I2C_IDLE.
        */
        if (CY_SCB_I2C_MASTER_WAIT == context->state)
        {
            /* Clear master previous transaction results:
            * - status to indicate that master is busy;
            * - number of bytes (only Stop is generated);
            * - cancel previous pending operation.
            */
            context->masterStatus   = CY_SCB_I2C_MASTER_BUSY;
            context->masterNumBytes = 0UL;
            context->masterPause    = false;

            /* Enable master interrupt sources to catch Stop condition */
            Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR);

            /* Complete transaction generating Stop */
            base->I2C_M_CMD = (SCB_I2C_M_CMD_M_STOP_Msk | SCB_I2C_M_CMD_M_NACK_Msk);
            context->state  = CY_SCB_I2C_MASTER_WAIT_STOP;
        }
    }

    Cy_SysLib_ExitCriticalSection(intrState);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterGetTransferCount
****************************************************************************//**
*
* Returns the number of bytes transferred since the last call of
* \ref Cy_SCB_I2C_MasterWrite or \ref Cy_SCB_I2C_MasterRead function.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* Number of bytes read or written by the master.
*
* \note
* * This function returns an invalid value if read or write transaction was
*   aborted or any listed event occurs during the transaction:
*   \ref CY_SCB_I2C_MASTER_ARB_LOST, \ref CY_SCB_I2C_MASTER_BUS_ERR or
*   \ref CY_SCB_I2C_MASTER_ABORT_START.
*
* * This number is updated only when the transaction completes, either through
*   an error or successfully.
*
*******************************************************************************/
uint32_t Cy_SCB_I2C_MasterGetTransferCount(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->masterNumBytes);
}


/*******************************************************************************
*                         I2C Master API: Low level
*******************************************************************************/


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterSendStart
****************************************************************************//**
*
* Generates a Start condition and sends a slave address with the Read/Write bit.
* This function is blocking. It does not return until the Start condition
* and address byte are sent and a ACK/NAK is received, or an error or timeout
* occurs.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param address
* 7 bit right justified slave address.
*
* \param bitRnW
* This sets the value of the Read/Write bit in the address, thus defining
* the direction of the following transfer.
* See \ref cy_en_scb_i2c_direction_t for the set of constants.
*
* \param timeoutMs
* Defines in milliseconds the time for which this function can block.
* If that time expires, the function returns. If a zero is passed,
* the function waits forever for the action to complete. If a timeout occurs,
* the SCB block is reset. Note The maximum value is UINT32_MAX/1000.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* After a read transaction is initiated and the slave ACKs the address, at
* least one byte must be read before completing the transaction or changing
* its direction.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendStart(CySCB_Type *base,
                            uint32_t address, cy_en_scb_i2c_direction_t bitRnW,
                            uint32_t timeoutMs,
                            cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID   (address));
    CY_ASSERT_L2(CY_SCB_I2C_IS_TIMEOUT_VALID(timeoutMs));
    CY_ASSERT_L3(CY_SCB_I2C_IS_RW_BIT_VALID (bitRnW));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    /* Disable the I2C slave interrupt sources to protect the state */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    if (CY_SCB_I2C_IDLE == context->state)
    {
        uint32_t locStatus;
        uint32_t timeout = CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs);

        /* Set the read or write direction */
        context->state       = CY_SCB_I2C_MASTER_ADDR;
        context->masterRdDir = (CY_SCB_I2C_READ_XFER == bitRnW);

        /* Clean up the hardware before a transfer. Note RX FIFO is empty at here */
        Cy_SCB_ClearMasterInterrupt(base, CY_SCB_I2C_MASTER_INTR_ALL);
        Cy_SCB_ClearRxInterrupt    (base, CY_SCB_RX_INTR_NOT_EMPTY);
        Cy_SCB_ClearTxFifo(base);

        /* Generate a Start and send address byte */
        Cy_SCB_WriteTxFifo(base, (_VAL2FLD(CY_SCB_I2C_ADDRESS, address) | (uint32_t) bitRnW));
        base->I2C_M_CMD  = SCB_I2C_M_CMD_M_START_ON_IDLE_Msk;

        /* Wait for a completion event from the master or slave */
        do
        {
            locStatus  = (CY_SCB_I2C_MASTER_TX_BYTE_DONE & Cy_SCB_GetMasterInterruptStatus(base));
            locStatus |= (CY_SCB_I2C_SLAVE_ADDR_DONE & Cy_SCB_GetSlaveInterruptStatus(base));
            locStatus |= WaitOneUnit(&timeout);

        } while (0UL == locStatus);

        /* Convert the status from register plus timeout to the return status */
        retStatus = HandleStatus(base, locStatus, context);
    }

    /* Enable I2C slave interrupt sources */
    Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_I2C_SLAVE_INTR);

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterSendReStart
****************************************************************************//**
*
* Generates a ReStart condition and sends a slave address with the Read/Write
* bit.
* This function is blocking. It does not return until the ReStart condition
* and address byte are sent and an ACK/NAK is received, or an error or timeout
* occurs.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param address
* A 7-bit right-justified slave address.
*
* \param bitRnW
* This sets the value of the Read/Write bit in the address, thus defining
* the direction of the following transfer.
* See \ref cy_en_scb_i2c_direction_t for the set of constants.
*
* \param timeoutMs
* Defines in milliseconds the time for which this function can block.
* If that time expires, the function returns. If a zero is passed,
* the function waits forever for the action to complete. If a timeout occurs,
* the SCB block is reset. Note The maximum value is UINT32_MAX/1000.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* * A successful transaction must be initiated by \ref Cy_SCB_I2C_MasterSendStart
*   before calling this function. If this condition is not met, this function
*   does nothing and returns \ref CY_SCB_I2C_MASTER_NOT_READY.
* * After a read transaction is initiated and the slave ACKs the address,
*   at least one byte must be read before completing the transaction or
*   changing its direction.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendReStart(CySCB_Type *base,
                        uint32_t address, cy_en_scb_i2c_direction_t bitRnW,
                        uint32_t timeoutMs,
                        cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID   (address));
    CY_ASSERT_L2(CY_SCB_I2C_IS_TIMEOUT_VALID(timeoutMs));
    CY_ASSERT_L3(CY_SCB_I2C_IS_RW_BIT_VALID (bitRnW));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    if (0UL != (CY_SCB_I2C_MASTER_ACTIVE & context->state))
    {
        uint32_t locStatus = 0U;
        uint32_t timeout = CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs);

        /* Set the read or write direction */
        context->state       = CY_SCB_I2C_MASTER_ADDR;
        context->masterRdDir = (CY_SCB_I2C_READ_XFER == bitRnW);

        /* Generate ReStart condition.
        * If previous transfer was read, NACK is generated before ReStart to
        * complete previous transfer.
        */
        base->I2C_M_CMD = SCB_I2C_M_CMD_M_START_Msk | (_FLD2BOOL(SCB_I2C_STATUS_M_READ, base->I2C_STATUS) ?
                          SCB_I2C_M_CMD_M_NACK_Msk : 0UL);

        /* Previous transfer was a write */
        if (false == _FLD2BOOL(SCB_I2C_STATUS_M_READ, base->I2C_STATUS))
        {
            /* Cypress ID #295908: Wait until ReStart is generated to complete
            * the previous write transfer. This ensures that the address byte 
            * will not be interpreted as the data byte of the previous 
            * transfer.
            */
            while ((0U == locStatus) &&
                   (0U != (SCB_I2C_M_CMD_M_START_Msk & base->I2C_M_CMD)))
            {
                locStatus = WaitOneUnit(&timeout);
            }
        }

        /* Check for timeout and continue */
        if (0U == locStatus)
        {
            /* Send the address byte */
            Cy_SCB_WriteTxFifo(base, (_VAL2FLD(CY_SCB_I2C_ADDRESS, address) | (uint32_t) bitRnW));

            /* Wait for a completion event from the or slave */
            do
            {
                locStatus  = (CY_SCB_I2C_MASTER_TX_BYTE_DONE & Cy_SCB_GetMasterInterruptStatus(base));
                locStatus |= WaitOneUnit(&timeout);

            } while (0UL == locStatus);
        }

        /* Convert the status from register plus timeout to the return status */
        retStatus = HandleStatus(base, locStatus, context);
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterSendStop
****************************************************************************//**
*
* Generates a Stop condition to complete the current transaction.
* This function is blocking. It does not return until the Stop condition
* is generated, or an error or timeout occurs.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param timeoutMs
* Defines in milliseconds the time for which this function can block.
* If that time expires, the function returns. If a zero is passed,
* the function waits forever for the action to complete. If a timeout occurs,
* the SCB block is reset. Note The maximum value is UINT32_MAX/1000.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* * A successful transaction must be initiated by
*   \ref Cy_SCB_I2C_MasterSendStart or \ref Cy_SCB_I2C_MasterSendReStart
*   before calling this function. If this condition is not met, this function
*   does nothing and returns.
*   \ref CY_SCB_I2C_MASTER_NOT_READY.
* * Even after the slave NAKs the address, this function must be called
*   to complete the transaction.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendStop(CySCB_Type *base,uint32_t timeoutMs,
                                cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L2(CY_SCB_I2C_IS_TIMEOUT_VALID(timeoutMs));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    if (0UL != (CY_SCB_I2C_MASTER_ACTIVE & context->state))
    {
        uint32_t locStatus;
        uint32_t timeout = CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs);

        /* Generate a stop (for Write direction) and NACK plus stop for the Read direction */
        base->I2C_M_CMD = (SCB_I2C_M_CMD_M_STOP_Msk | SCB_I2C_M_CMD_M_NACK_Msk);

        /* Wait for a completion event from the master or slave */
        do
        {
            locStatus  = (CY_SCB_I2C_MASTER_STOP_DONE & Cy_SCB_GetMasterInterruptStatus(base));
            locStatus |= WaitOneUnit(&timeout);

        } while (0UL == locStatus);

        /* Convert the status from register plus timeout to the return status */
        retStatus = HandleStatus(base, locStatus, context);
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterReadByte
****************************************************************************//**
*
* Reads one byte from a slave and generates an ACK or prepares to generate
* a NAK. The NAK will be generated before a Stop or ReStart condition by
* \ref Cy_SCB_I2C_MasterSendStop or \ref Cy_SCB_I2C_MasterSendReStart function
* appropriately.
* This function is blocking. It does not return until a byte is
* received, or an error or timeout occurs.
* The corresponding non-blocking function is \ref Cy_SCB_I2C_MasterRead.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param ackNack
* A response to a received byte.
* See \ref cy_en_scb_i2c_command_t for the set of constants.
*
* \param byte
* The pointer to the location to store the Read byte.
*
* \param timeoutMs
* Defines in milliseconds the time for which this function can block.
* If that time expires, the function returns. If a zero is passed,
* the function waits forever for the action to complete. If a timeout occurs,
* the SCB block is reset. Note The maximum value is UINT32_MAX/1000.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* A successful transaction must be initiated by \ref Cy_SCB_I2C_MasterSendStart
* or \ref Cy_SCB_I2C_MasterSendReStart before calling this function. If this
* condition is not met, this function does nothing and returns
* \ref CY_SCB_I2C_MASTER_NOT_READY.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterReadByte(CySCB_Type *base,
                            cy_en_scb_i2c_command_t ackNack, uint8_t *byte,
                            uint32_t timeoutMs,
                            cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L1(CY_SCB_IS_BUFFER_VALID      (byte, 1UL));
    CY_ASSERT_L2(CY_SCB_I2C_IS_TIMEOUT_VALID (timeoutMs));
    CY_ASSERT_L3(CY_SCB_I2C_IS_RESPONSE_VALID(ackNack));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    if (CY_SCB_I2C_MASTER_RX0 == context->state)
    {
        bool     rxNotEmpty;
        uint32_t locStatus;
        uint32_t timeout = CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs);

        /* Wait for ACK/NAK transmission and data byte reception */
        do
        {
            rxNotEmpty = (0UL != (CY_SCB_RX_INTR_NOT_EMPTY & Cy_SCB_GetRxInterruptStatus(base)));
            locStatus  = (CY_SCB_I2C_MASTER_RX_BYTE_DONE & Cy_SCB_GetMasterInterruptStatus(base));
            locStatus |= WaitOneUnit(&timeout);

        } while ((!rxNotEmpty) && (0UL == locStatus));


        if (rxNotEmpty)
        {
            /* Get the received data byte */
            *byte = (uint8_t) Cy_SCB_ReadRxFifo(base);

            Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_NOT_EMPTY | CY_SCB_RX_INTR_LEVEL);
        }

        /* Convert the status from register plus timeout to the return status */
        retStatus = HandleStatus(base, locStatus, context);

        if (CY_SCB_I2C_SUCCESS == retStatus)
        {
            if (CY_SCB_I2C_ACK == ackNack)
            {
                /* Generate ACK */
                base->I2C_M_CMD = SCB_I2C_M_CMD_M_ACK_Msk;
            }
            else
            {
                /* NAK is generated by SendStop() or SendReStart() */
            }
        }
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterWriteByte
****************************************************************************//**
*
* Sends one byte to a slave.
* This function is blocking. It does not return until a byte is
* transmitted, or an error or timeout occurs.
* The corresponding non-blocking function is \ref Cy_SCB_I2C_MasterWrite.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param byte
* The byte to write to a slave.
*
* \param timeoutMs
* Defines in milliseconds the time for which this function can block.
* If that time expires, the function returns. If a zero is passed,
* the function waits forever for the action to complete. If a timeout occurs,
* the SCB block is reset. Note The maximum value is UINT32_MAX/1000.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
* \note
* A successful transaction must be initiated by \ref Cy_SCB_I2C_MasterSendStart
* or \ref Cy_SCB_I2C_MasterSendReStart before calling this function. If this
* condition is not met, this function does nothing and returns
* \ref CY_SCB_I2C_MASTER_NOT_READY.
*
*******************************************************************************/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterWriteByte(CySCB_Type *base, uint8_t byte, uint32_t timeoutMs,
                                                  cy_stc_scb_i2c_context_t *context)
{
    CY_ASSERT_L2(CY_SCB_I2C_IS_TIMEOUT_VALID(timeoutMs));

    cy_en_scb_i2c_status_t retStatus = CY_SCB_I2C_MASTER_NOT_READY;

    if (CY_SCB_I2C_MASTER_TX == context->state)
    {
        uint32_t locStatus;
        uint32_t timeout = CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs);

        /* Send the data byte */
        Cy_SCB_WriteTxFifo(base, (uint32_t) byte);

        /* Wait for a completion event from the master or slave */
        do
        {
            locStatus  = (CY_SCB_I2C_MASTER_TX_BYTE_DONE & Cy_SCB_GetMasterInterruptStatus(base));
            locStatus |= WaitOneUnit(&timeout);

        } while (0UL == locStatus);

        /* Convert the status from register plus timeout to the API status */
        retStatus = HandleStatus(base, locStatus, context);
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_Interrupt
****************************************************************************//**
*
* This is the interrupt function for the SCB configured in the I2C mode.
* The interrupt is mandatory for I2C operation and this function must be called
* inside the user-defined interrupt service. The exception is the I2C master,
* which uses only the \ref group_scb_i2c_master_low_level_functions functions.
* To reduce the flash consumed by the I2C driver call
* \ref Cy_SCB_I2C_SlaveInterrupt when I2C mode is the slave and
* \ref Cy_SCB_I2C_MasterInterrupt when I2C mode is the master.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_I2C_Interrupt(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    if (0UL != (CY_SCB_I2C_MASTER_ACTIVE & context->state))
    {
        /* Execute a transfer as the master */
        Cy_SCB_I2C_MasterInterrupt(base, context);
    }
    else
    {
        /* Execute a transfer as the slave */
        Cy_SCB_I2C_SlaveInterrupt(base, context);
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveInterrupt
****************************************************************************//**
*
* This is the interrupt function for the SCB configured in I2C mode as the
* slave. This function should be called inside the user-defined interrupt
* service routine to make any of the slave functions to work.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_I2C_SlaveInterrupt(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t slaveIntrStatus;

    /* Handle an I2C wake-up event */
    if (0UL != (CY_SCB_I2C_INTR_WAKEUP & Cy_SCB_GetI2CInterruptStatusMasked(base)))
    {
        /* Move from IDLE state, the slave was addressed. Following address match
        * interrupt continue transfer.
        */
        context->state = CY_SCB_I2C_SLAVE_ACTIVE;

        Cy_SCB_ClearI2CInterrupt(base, CY_SCB_I2C_INTR_WAKEUP);
    }

    /* Handle the slave interrupt sources */
    slaveIntrStatus = Cy_SCB_GetSlaveInterruptStatusMasked(base);

    /* Handle the error conditions */
    if (0UL != (CY_SCB_I2C_SLAVE_INTR_ERROR & slaveIntrStatus))
    {
        /* Update the status */
        context->slaveStatus |= (0UL != (CY_SCB_SLAVE_INTR_I2C_BUS_ERROR & slaveIntrStatus)) ?
                                            CY_SCB_I2C_SLAVE_BUS_ERR : CY_SCB_I2C_SLAVE_ARB_LOST;

        /* Disable the RX interrupt source to drop data into RX FIFO if any */
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

        /* Add the stop status to back into the default state and set completion statuses */
        slaveIntrStatus |= CY_SCB_SLAVE_INTR_I2C_STOP;
    }
    else
    {
        if (0UL != (CY_SCB_SLAVE_INTR_I2C_STOP & slaveIntrStatus))
        {
            /* Get data from the RX FIFO after a stop is generated if there is
            * space to store it.
            */
            if ((Cy_SCB_GetNumInRxFifo(base) > 0UL) && (context->slaveRxBufferSize > 0UL))
            {
                Cy_SCB_SetRxInterrupt    (base, CY_SCB_RX_INTR_LEVEL);
                Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);
            }
        }
    }

    /* Handle the receive direction (master writes data) */
    if (0UL != (CY_SCB_RX_INTR_LEVEL & Cy_SCB_GetRxInterruptStatusMasked(base)))
    {
        SlaveHandleDataReceive(base, context);

        Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
    }

    /* Handle the transfer completion */
    if (0UL != (CY_SCB_SLAVE_INTR_I2C_STOP & slaveIntrStatus))
    {
        SlaveHandleStop(base, context);

        Cy_SCB_ClearSlaveInterrupt(base, CY_SCB_SLAVE_INTR_I2C_STOP);

        /* Update the slave interrupt status */
        slaveIntrStatus = Cy_SCB_GetSlaveInterruptStatusMasked(base);
    }

    /* Handle the address reception */
    if (0UL != (CY_SCB_I2C_SLAVE_INTR_ADDR & slaveIntrStatus))
    {
        SlaveHandleAddress(base, context);

        Cy_SCB_ClearI2CInterrupt(base, CY_SCB_I2C_INTR_WAKEUP);
        Cy_SCB_ClearSlaveInterrupt(base, CY_SCB_I2C_SLAVE_INTR_ADDR);
    }

    /* Handle the transmit direction (master reads data) */
    if (0UL != (CY_SCB_I2C_SLAVE_INTR_TX & Cy_SCB_GetTxInterruptStatusMasked(base)))
    {
        SlaveHandleDataTransmit(base, context);

        Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_LEVEL);
    }
}


/*******************************************************************************
* Function Name: SlaveHandleAddress
****************************************************************************//**
*
* Prepares the slave for the following Read or Write transfer after the
* matched address was received.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void SlaveHandleAddress(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    /* The default command is the ACK address. It can be overridden in an address callback */
    cy_en_scb_i2c_command_t cmd = CY_SCB_I2C_ACK;

    /* The callback for the address in RX FIFO or a general call */
    if (NULL != context->cbAddr)
    {
        uint32_t events = 0UL;

        /* Set an address in the FIFO event if the address accept is enabled */
        if (_FLD2BOOL(SCB_CTRL_ADDR_ACCEPT, base->CTRL))
        {
            events = (0UL != (CY_SCB_SLAVE_INTR_I2C_ADDR_MATCH & Cy_SCB_GetSlaveInterruptStatusMasked(base))) ?
                              CY_SCB_I2C_ADDR_IN_FIFO_EVENT : 0UL;
        }

        /* Set a general call event if "ignore general call" is disabled */
        if (!_FLD2BOOL(SCB_I2C_CTRL_S_GENERAL_IGNORE, base->I2C_CTRL))
        {
            events |= (0UL != (CY_SCB_SLAVE_INTR_I2C_GENERAL_ADDR & Cy_SCB_GetSlaveInterruptStatusMasked(base))) ?
                               CY_SCB_I2C_GENERAL_CALL_EVENT : 0UL;
        }

        /* Check presence of events before involve callback */
        if (0UL != events)
        {
            /* Involve a callback for the address phase and get the ACK/NACK command */
            cmd = context->cbAddr(events);

            /* Clear RX level interrupt after address reception */
            Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);

            if (cmd == CY_SCB_I2C_ACK)
            {
                /* Clear the stall stop status and enable the stop interrupt source */
                Cy_SCB_ClearSlaveInterrupt(base, CY_SCB_SLAVE_INTR_I2C_STOP);
                Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_I2C_SLAVE_INTR);
            }
            else
            {
                /* Disable the stop interrupt source */
                Cy_SCB_SetSlaveInterruptMask(base, CY_SCB_I2C_SLAVE_INTR_NO_STOP);
            }
        }
    }

    /* Clear the TX FIFO before continue the transaction */
    Cy_SCB_ClearTxFifo(base);

    /* Set the command to an ACK or NACK address */
    base->I2C_S_CMD = (cmd == CY_SCB_I2C_ACK) ? SCB_I2C_S_CMD_S_ACK_Msk : SCB_I2C_S_CMD_S_NACK_Msk;

    if (cmd == CY_SCB_I2C_ACK)
    {
        bool readDirection = _FLD2BOOL(SCB_I2C_STATUS_S_READ,base->I2C_STATUS);

        /* Notify the user about start of transfer */
        if (NULL != context->cbEvents)
        {
            context->cbEvents(readDirection ? CY_SCB_I2C_SLAVE_READ_EVENT : CY_SCB_I2C_SLAVE_WRITE_EVENT);
        }

        /* Prepare for a transfer */
        if (readDirection)
        {
            context->state        = CY_SCB_I2C_SLAVE_TX;
            context->slaveStatus |= CY_SCB_I2C_SLAVE_RD_BUSY;

            /* Prepare to transmit data */
            context->slaveTxBufferIdx = context->slaveTxBufferCnt;
            context->slaveRdBufEmpty  = false;
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_LEVEL);
        }
        else
        {
            uint32_t level = 0UL;

            context->state        = CY_SCB_I2C_SLAVE_RX;
            context->slaveStatus |= CY_SCB_I2C_SLAVE_WR_BUSY;

            /* Prepare to receive data */
            Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);

            if (context->useRxFifo)
            {
                if (context->slaveRxBufferSize > 0UL)
                {
                    uint32_t fifoSize = CY_SCB_I2C_FIFO_SIZE;

                    /* ACK data automatically until RX FIFO is full */
                    base->I2C_CTRL |= SCB_I2C_CTRL_S_READY_DATA_ACK_Msk;

                    if (context->slaveRxBufferSize > fifoSize)
                    {
                        /* Set a level in RX FIFO to trigger the receive interrupt source */
                        level = (context->slaveRxBufferSize - fifoSize);
                        level = ((level > fifoSize) ? (fifoSize / 2UL) : level) - 1UL;
                    }
                    else
                    {
                        /* Set a level in RX FIFO to read the number of bytes */
                        level = (context->slaveRxBufferSize - 1UL);

                        /* NACK when RX FIFO becomes full */
                        base->I2C_CTRL |= SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk;

                        /* Disable the RX level interrupt and wait until RX FIFO is full or stops */
                        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
                    }
                }
            }

            Cy_SCB_SetRxFifoLevel(base, level);
        }
    }
}


/*******************************************************************************
* Function Name: SlaveHandleDataReceive
****************************************************************************//**
*
* Reads data from RX FIFO into the buffer provided by
* \ref Cy_SCB_I2C_SlaveConfigWriteBuf.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void SlaveHandleDataReceive(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    /* Check whether there is space to put data */
    if (context->slaveRxBufferSize > 0UL)
    {
        if (context->useRxFifo)
        {
            uint32_t level;
            uint32_t fifoSize = CY_SCB_I2C_FIFO_SIZE;

            /* Get the number of bytes to read from RX FIFO */
            uint32_t numToCopy = Cy_SCB_GetRxFifoLevel(base) + 1UL;

            /* Get data from RX FIFO */
            numToCopy = Cy_SCB_ReadArray(base, context->slaveRxBuffer, numToCopy);
            context->slaveRxBufferIdx  += numToCopy;
            context->slaveRxBufferSize -= numToCopy;
            context->slaveRxBuffer      = &context->slaveRxBuffer[numToCopy];

            /* Prepare to read a next chunk of data */
            if (context->slaveRxBufferSize > fifoSize)
            {
                level = context->slaveRxBufferSize - fifoSize;
                level = ((level > fifoSize) ? (fifoSize / 2UL) : level) - 1UL;
            }
            else
            {
                base->I2C_CTRL |= SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk;

                level = (context->slaveRxBufferSize == 0UL) ? (0UL) : (context->slaveRxBufferSize - 1UL);
                Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
            }

            /* Set the RX level to trigger an interrupt */
            Cy_SCB_SetRxFifoLevel(base, level);
        }
        else
        {
            /* Continue the transfer: send an ACK */
            base->I2C_S_CMD = SCB_I2C_S_CMD_S_ACK_Msk;

            /* Put data into the RX buffer */
            context->slaveRxBuffer[context->slaveRxBufferIdx] = (uint8_t) Cy_SCB_ReadRxFifo(base);
            ++context->slaveRxBufferIdx;
            --context->slaveRxBufferSize;
        }
    }
    else
    {
        /* Finish a transfer: send a NACK and discard the received byte */
        base->I2C_S_CMD = SCB_I2C_S_CMD_S_NACK_Msk;
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }
}


/*******************************************************************************
* Function Name: SlaveHandleDataTransmit
****************************************************************************//**
*
* Loads TX FIFO with data provided by \ref Cy_SCB_I2C_SlaveConfigReadBuf.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void SlaveHandleDataTransmit(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t numToCopy;

    /* Notify the user that there is no data to send to the master.
    * This event triggers once in scope of a transfer.
    */
    if ((!context->slaveRdBufEmpty) && (0UL == context->slaveTxBufferSize))
    {
        /* Involve a callback if registered: no data to send */
        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT);
        }

        /* Update the Read buffer empty status after a callback is involved */
        context->slaveRdBufEmpty = (0UL == context->slaveTxBufferSize);

        /* Enable the TX level interrupt source to continue sending data */
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_LEVEL);
    }

    /* Check whether the Read buffer was updated in the callback */
    if (context->slaveRdBufEmpty)
    {
        /* The Read buffer is empty: copy CY_SCB_I2C_DEFAULT_TX into TX FIFO */
        numToCopy = (context->useTxFifo) ? Cy_SCB_GetFifoSize(base) : 1UL;

        numToCopy = Cy_SCB_WriteDefaultArray(base, CY_SCB_I2C_DEFAULT_TX, numToCopy);
        context->slaveTxBufferIdx += numToCopy;

        context->slaveStatus |= CY_SCB_I2C_SLAVE_RD_UNDRFL;
    }
    else
    {
        if (context->slaveTxBufferSize > 1UL)
        {
            /* Get the number of bytes to copy into TX FIFO */
            numToCopy = (context->useTxFifo) ? (context->slaveTxBufferSize - 1UL) : (1UL);

            /* Write data into TX FIFO */
            numToCopy = Cy_SCB_WriteArray(base, context->slaveTxBuffer, numToCopy);
            context->slaveTxBufferIdx  += numToCopy;
            context->slaveTxBufferSize -= numToCopy;
            context->slaveTxBuffer      = &context->slaveTxBuffer[numToCopy];
        }

        /* Put the last byte */
        if ((CY_SCB_I2C_FIFO_SIZE != Cy_SCB_GetNumInTxFifo(base)) && (1UL == context->slaveTxBufferSize))
        {
            uint32_t intrStatus;

             /* Put the last data byte in the TX FIFO and clear the TX Underflow 
            * interrupt source inside the critical section to ensure that the 
            * TX Underflow interrupt will trigger after all data bytes from the 
            * TX FIFO are transferred onto the bus.
            */
            intrStatus = Cy_SysLib_EnterCriticalSection();

            Cy_SCB_WriteTxFifo     (base, (uint32_t) context->slaveTxBuffer[0UL]);
            Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UNDERFLOW);

            Cy_SysLib_ExitCriticalSection(intrStatus);

            /* Move the pointers */
            ++context->slaveTxBufferIdx;
            context->slaveTxBufferSize = 0UL;
            context->slaveTxBuffer     = &context->slaveTxBuffer[1UL];

            /* Enable the TX underflow interrupt to catch when there is no data to send */
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_UNDERFLOW);

            if (context->useTxFifo)
            {
                /* Data is copied into TX FIFO */
                context->slaveStatus |= CY_SCB_I2C_SLAVE_RD_IN_FIFO;

                /* Involve a callback if registered: data copied into TX FIFO */
                if (NULL != context->cbEvents)
                {
                    context->cbEvents(CY_SCB_I2C_SLAVE_RD_IN_FIFO_EVENT);
                }
            }
        }
    }
}


/*******************************************************************************
* Function Name: SlaveHandleStop
****************************************************************************//**
*
* Handles transfer completion. It is triggered by a stop or restart
* condition on the bus.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void SlaveHandleStop(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t locEvents;

    if (CY_SCB_I2C_SLAVE_RX == context->state)
    {
        /* If any data is left in RX FIFO, this is an overflow */
        if (Cy_SCB_GetNumInRxFifo(base) > 0UL)
        {
            context->slaveStatus |= CY_SCB_I2C_SLAVE_WR_OVRFL;

            if (context->useRxFifo)
            {
                Cy_SCB_ClearRxFifo(base);
            }
            else
            {
                (void) Cy_SCB_ReadRxFifo(base);
            }
        }

        locEvents             = (uint32_t)  CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT;
        context->slaveStatus |= (uint32_t)  CY_SCB_I2C_SLAVE_WR_CMPLT;
        context->slaveStatus &= (uint32_t) ~CY_SCB_I2C_SLAVE_WR_BUSY;

        /* Clean up the RX direction */
        base->I2C_CTRL &= (uint32_t) ~(SCB_I2C_CTRL_S_READY_DATA_ACK_Msk |
                                          SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk);

        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
        Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
    }
    else
    {
        /* The number of bytes left in TX FIFO */
        uint32_t size = Cy_SCB_GetNumInTxFifo(base) + Cy_SCB_GetTxSrValid(base);

        /* Get the number of bytes transferred from the read buffer */
        context->slaveTxBufferCnt = (context->slaveTxBufferIdx - size);

        /* Update buffer pointer and its size if there is no overflow */
        if (0UL == (CY_SCB_I2C_SLAVE_RD_UNDRFL & context->slaveStatus))
        {
            context->slaveTxBufferSize += size;
            context->slaveTxBuffer     -= size;
        }

        locEvents             = (uint32_t)  CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT;
        context->slaveStatus |= (uint32_t)  CY_SCB_I2C_SLAVE_RD_CMPLT;
        context->slaveStatus &= (uint32_t) ~CY_SCB_I2C_SLAVE_RD_BUSY;

        /* Clean up the TX direction */
        Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }

    /* Return scb into the known state after an error */
    if (0UL != (CY_SCB_I2C_SLAVE_INTR_ERROR & Cy_SCB_GetSlaveInterruptStatusMasked(base)))
    {
        /* After scb IP is reset, the interrupt statuses are cleared */
        Cy_SCB_FwBlockReset(base);

        locEvents |= CY_SCB_I2C_SLAVE_ERR_EVENT;
    }

    /* After a stop or error, set the state to idle */
    context->state = CY_SCB_I2C_IDLE;

    /* Call a completion callback if registered */
    if (NULL != context->cbEvents)
    {
        context->cbEvents(locEvents);
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterInterrupt
****************************************************************************//**
*
* This is the interrupt function for the SCB configured in I2C mode as the
* master. This function should be called inside the user-defined interrupt
* service routine to make \ref group_scb_i2c_master_high_level_functions
* functions to work.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_I2C_MasterInterrupt(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t intrCause = Cy_SCB_GetInterruptCause(base);

    /* Check whether the slave is active. It can be addressed during the master set-up transfer */
    if (0UL != (CY_SCB_SLAVE_INTR & intrCause))
    {
        /* Abort the transfer due to slave operation */
        if (0UL != base->I2C_M_CMD)
        {
            base->I2C_M_CMD = 0UL;

            context->masterStatus |= CY_SCB_I2C_MASTER_ABORT_START;
        }

        context->state = CY_SCB_I2C_MASTER_CMPLT;
    }

    /* Check for master error conditions */
    if (0UL != (CY_SCB_MASTER_INTR & intrCause))
    {
        MasterHandleEvents(base, context);

        /* Any master event does not require further TX processing */
        intrCause &= (uint32_t) ~CY_SCB_TX_INTR;
    }

    if (0UL != (CY_SCB_RX_INTR & intrCause))
    {
        MasterHandleDataReceive(base, context);

        Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
    }

    if (0UL != (CY_SCB_TX_INTR & intrCause))
    {
        MasterHandleDataTransmit(base, context);

        Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_LEVEL);
    }

    /* Complete the transfer */
    if (CY_SCB_I2C_MASTER_CMPLT == context->state)
    {
        MasterHandleComplete(base, context);
    }

    /* Generate stop to complete transfer */
    if (CY_SCB_I2C_MASTER_STOP == context->state)
    {
        MasterHandleStop(base, context);
    }
}


/*******************************************************************************
* Function Name: MasterHandleMasterEvents
****************************************************************************//**
*
* Reads data from RX FIFO into the buffer provided by \ref Cy_SCB_I2C_MasterRead.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* The master CY_SCB_MASTER_INTR_I2C_ACK interrupt source is used for Stop
* generation or request to abort transfer.
*
*******************************************************************************/
static void MasterHandleEvents(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t masterIntrStatus = Cy_SCB_GetMasterInterruptStatusMasked(base);

    /* The master has not received the acknowledgment for slave */
    if (0UL != (CY_SCB_MASTER_INTR_I2C_NACK & masterIntrStatus))
    {
        /* Clear NAK interrupt source */
        Cy_SCB_ClearMasterInterrupt(base, CY_SCB_MASTER_INTR_I2C_NACK);

        /* Update status to indicate address or data was NACKed */
        context->masterStatus |= (0UL != (CY_SCB_MASTER_INTR_I2C_ACK & Cy_SCB_GetMasterInterruptStatus(base))) ?
                                            CY_SCB_I2C_MASTER_DATA_NAK : CY_SCB_I2C_MASTER_ADDR_NAK;

        /* Check whether Stop generation was requested before */
        if (CY_SCB_I2C_MASTER_WAIT_STOP != context->state)
        {
            context->state = (context->masterPause) ? CY_SCB_I2C_MASTER_CMPLT : CY_SCB_I2C_MASTER_STOP;
        }
    }

    /* The master detected a bus error condition */
    if (0UL != (CY_SCB_MASTER_INTR_I2C_BUS_ERROR & masterIntrStatus))
    {
        context->masterStatus |= CY_SCB_I2C_MASTER_BUS_ERR;
    }

    /* The master detected an arbitration lost condition */
    if (0UL != (CY_SCB_MASTER_INTR_I2C_ARB_LOST & masterIntrStatus))
    {
        context->masterStatus |= CY_SCB_I2C_MASTER_ARB_LOST;
    }

    /* Complete the transfer: stop, bus error or arbitration lost */
    if (0UL != (CY_SCB_I2C_MASTER_INTR_CMPLT & masterIntrStatus))
    {
        context->state = CY_SCB_I2C_MASTER_CMPLT;
    }
}


/*******************************************************************************
* Function Name: MasterHandleDataReceive
****************************************************************************//**
*
* Reads data from RX FIFO into the buffer provided by \ref Cy_SCB_I2C_MasterRead.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void MasterHandleDataReceive(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    switch (context->state)
    {
        case CY_SCB_I2C_MASTER_RX0:
        {
            /* Put data into the component buffer */
            context->masterBuffer[0UL] = (uint8_t) Cy_SCB_ReadRxFifo(base);

            ++context->masterBufferIdx;
            --context->masterBufferSize;

            if (context->masterBufferSize > 0UL)
            {
                /* Continue the transaction: move pointer send an ACK */
                context->masterBuffer = &context->masterBuffer[1UL];
                base->I2C_M_CMD = SCB_I2C_M_CMD_M_ACK_Msk;
            }
            else
            {
                /* Complete the transaction */
                context->state = (context->masterPause) ? CY_SCB_I2C_MASTER_CMPLT : CY_SCB_I2C_MASTER_STOP;
            }
        }
        break;

        case CY_SCB_I2C_MASTER_RX1:
        {
            uint32_t numToCopied;

            /* Get data from RX FIFO */
            numToCopied = Cy_SCB_ReadArray(base, context->masterBuffer, context->masterBufferSize);
            context->masterBufferIdx  += numToCopied;
            context->masterBufferSize -= numToCopied;
            context->masterBuffer      = &context->masterBuffer[numToCopied];

            if (context->masterBufferSize < 2UL)
            {
                /* Stop ACKing data */
                base->I2C_CTRL &= (uint32_t) ~SCB_I2C_CTRL_M_READY_DATA_ACK_Msk;

                if (1UL == context->masterBufferSize)
                {
                    /* Catch the last byte */
                    Cy_SCB_SetRxFifoLevel(base, 0UL);

                    context->state = CY_SCB_I2C_MASTER_RX0;
                }
                else
                {
                    /* Stop RX processing */
                    Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

                    context->state = CY_SCB_I2C_MASTER_STOP;
                }
            }
            else
            {
                uint32_t halfFifoSize = CY_SCB_I2C_HALF_FIFO_SIZE;

                /* Continue the transfer: Adjust the level in RX FIFO */
                Cy_SCB_SetRxFifoLevel(base, (context->masterBufferSize <= halfFifoSize) ?
                                            (context->masterBufferSize - 2UL) : (halfFifoSize - 1UL));
            }
        }
        break;

        default:
            /* Do nothing: drop data into RX FIFO */
        break;
    }
}


/*******************************************************************************
* Function Name: MasterHandleDataTransmit
****************************************************************************//**
*
* Loads TX FIFO with data provided by \ref Cy_SCB_I2C_MasterWrite.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void MasterHandleDataTransmit(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    if (CY_SCB_I2C_MASTER_TX_DONE == context->state)
    {
        context->state = CY_SCB_I2C_MASTER_CMPLT;
    }
    else if (CY_SCB_I2C_MASTER_TX == context->state)
    {
        if (context->masterBufferSize > 1UL)
        {
            /* Get the number of bytes to copy into TX FIFO */
            uint32_t NumToCopy = (context->useTxFifo) ? (context->masterBufferSize - 1UL) : (1UL);

            /* Write data into TX FIFO */
            NumToCopy = Cy_SCB_WriteArray(base, context->masterBuffer, NumToCopy);
            context->masterBufferIdx  += NumToCopy;
            context->masterBufferSize -= NumToCopy;
            context->masterBuffer      = &context->masterBuffer[NumToCopy];
        }

        /* Put the last byte */
        if ((CY_SCB_I2C_FIFO_SIZE != Cy_SCB_GetNumInTxFifo(base)) && (1UL == context->masterBufferSize))
        {
            uint32_t intrStatus;

            /* Put the last data byte in the TX FIFO and clear the TX Underflow 
            * interrupt source inside the critical section to ensure that the 
            * TX Underflow interrupt will trigger after all data bytes from the 
            * TX FIFO are transferred onto the bus.
            */
            intrStatus = Cy_SysLib_EnterCriticalSection();

            Cy_SCB_WriteTxFifo     (base, (uint32_t) context->masterBuffer[0UL]);
            Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UNDERFLOW);

            Cy_SysLib_ExitCriticalSection(intrStatus);

            ++context->masterBufferIdx;
            context->masterBufferSize = 0UL;
        }

        /* Complete the transfer */
        if (0UL == context->masterBufferSize)
        {
            if (context->masterPause)
            {
                /* Wait until data is transfered onto the bus */
                Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_UNDERFLOW);

                context->state = CY_SCB_I2C_MASTER_TX_DONE;
            }
            else
            {
                /* Disable TX processing */
                Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

                /* Request Stop generation */
                context->state = CY_SCB_I2C_MASTER_STOP;
            }

            if (context->useTxFifo)
            {
                /* Notify the user that data is in TX FIFO */
                context->masterStatus |= CY_SCB_I2C_MASTER_WR_IN_FIFO;

                if (NULL != context->cbEvents)
                {
                    context->cbEvents(CY_SCB_I2C_MASTER_WR_IN_FIFO_EVENT);
                }
            }
        }
    }
    else
    {
        /* Do nothing */
    }
}


/*******************************************************************************
* Function Name: MasterHandleStop
****************************************************************************//**
*
* Handles the stop condition generation
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void MasterHandleStop(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    /* Stop RX and TX processing */
    Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    if (0UL != base->I2C_M_CMD)
    {
        /* Enable ACK interrupt source: it triggers after ACK response to
        * address was received.
        */
        Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR_ALL);
    }
    else
    {
        /* Disable ACK interrupt source */
        Cy_SCB_SetMasterInterruptMask(base, CY_SCB_I2C_MASTER_INTR);

        /* Complete transaction generating Stop */
        base->I2C_M_CMD = (SCB_I2C_M_CMD_M_STOP_Msk | SCB_I2C_M_CMD_M_NACK_Msk);
        context->state  = CY_SCB_I2C_MASTER_WAIT_STOP;
    }
}


/*******************************************************************************
* Function Name: MasterHandleComplete
****************************************************************************//**
*
* Handles the transfer completion on a stop or restart - the normal case or
* completion due to an error on the bus or lost arbitration.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_i2c_context_t allocated
* by the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void MasterHandleComplete(CySCB_Type *base, cy_stc_scb_i2c_context_t *context)
{
    uint32_t masterIntrStatus = Cy_SCB_GetMasterInterruptStatusMasked(base);

    /* Clean-up hardware */

    /* Disalbe auto data ACK option */
    base->I2C_CTRL &= (uint32_t) ~SCB_I2C_CTRL_M_READY_DATA_ACK_Msk;

    /* Disable the interrupt source for master operation */
    Cy_SCB_SetRxInterruptMask    (base, CY_SCB_CLEAR_ALL_INTR_SRC);
    Cy_SCB_SetTxInterruptMask    (base, CY_SCB_CLEAR_ALL_INTR_SRC);
    Cy_SCB_SetMasterInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    Cy_SCB_ClearMasterInterrupt(base, CY_SCB_I2C_MASTER_INTR_ALL);

    /* Operation complete - master is not busy anymore */
    context->masterStatus &= (uint32_t) ~CY_SCB_I2C_MASTER_BUSY;

    /* Get number of byte transferred on the bus */
    if (context->masterRdDir)
    {
        context->masterNumBytes = context->masterBufferIdx;
    }
    else
    {
        context->masterNumBytes = context->masterBufferIdx -
                    (Cy_SCB_GetNumInTxFifo(base) + Cy_SCB_GetTxSrValid(base));
    }

    /* Clean up after a not completed transfer */
    if (0UL != (CY_SCB_I2C_MASTER_INTR_ERR & masterIntrStatus))
    {
        /* Reset the scb IP block when:
        *  1. Master mode: Reset IP when arbitration is lost or a bus error occurs.
        *  2. Master-Slave mode: Reset IP if it is not the address phase (ACK is 0).
        *  Otherwise, reset only on a bus error. If "lost arbitration" happens, the slave
        *  can be addressed, so let the slave accept the address.
        */

        bool resetIp = true;

        /* Check the Master-Slave address an ACK/NACK */
        if (((uint32_t) CY_SCB_I2C_MASTER_SLAVE) == _FLD2VAL(CY_SCB_I2C_CTRL_MODE, base->I2C_CTRL))
        {
            resetIp = ((0UL != (CY_SCB_MASTER_INTR_I2C_ACK & masterIntrStatus)) ? true :
                            ((0UL != (CY_SCB_MASTER_INTR_I2C_BUS_ERROR & masterIntrStatus)) ? true : false));
        }

        if (resetIp)
        {
            /* Reset to get it back in an known state */
            Cy_SCB_FwBlockReset(base);
        }

        /* Back to the idle state. The master is not active anymore */
        context->state = CY_SCB_I2C_IDLE;
    }
    else
    {
        if (context->useRxFifo)
        {
            /* Clear RX FIFO from remaining data and level interrupt source */
            Cy_SCB_ClearRxFifo(base);
            Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
        }

        context->state = (context->masterPause) ? CY_SCB_I2C_MASTER_WAIT : CY_SCB_I2C_IDLE;
    }

    /* An operation completion callback */
    if (NULL != context->cbEvents)
    {
        /* Get completion events based on the hardware status */
        uint32_t locEvents = context->masterRdDir ? CY_SCB_I2C_MASTER_RD_CMPLT_EVENT : CY_SCB_I2C_MASTER_WR_CMPLT_EVENT;

        /* Add errors if any */
        locEvents |= (0UL != (CY_SCB_I2C_MASTER_ERR & context->masterStatus)) ? CY_SCB_I2C_MASTER_ERR_EVENT : 0UL;

        context->cbEvents(locEvents);
    }
}



/******************************************************************************
* Function Name: WaitOneUnit
****************************************************************************//**
*
* Waits for one unit before unblock code execution.
* Note that if a timeout value is 0, this function does nothing and returns 0.
*
* \param timeout
* The pointer to a timeout value.
*
* \return
* Returns 0 if a timeout does not expire or the timeout mask.
*
*******************************************************************************/
static uint32_t WaitOneUnit(uint32_t *timeout)
{
    uint32_t status = 0UL;

    /* If the timeout equal to 0. Ignore the timeout */
    if (*timeout > 0UL)
    {
        Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
        --(*timeout);

        if (0UL == *timeout)
        {
            status = CY_SCB_I2C_MASTER_TIMEOUT_DONE;
        }
    }

    return (status);
}


/******************************************************************************
* Function Name: HandleStatus
****************************************************************************//**
*
* Converts passed status into the cy_en_scb_i2c_status_t.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param status
* The status to covert.
*
* \return
* \ref cy_en_scb_i2c_status_t
*
*******************************************************************************/
static cy_en_scb_i2c_status_t HandleStatus(CySCB_Type *base, uint32_t status, cy_stc_scb_i2c_context_t *context)
{
    cy_en_scb_i2c_status_t retStatus;
    bool resetBlock = false;

    /* Convert the master status to the API status */
    if (0UL != (CY_SCB_I2C_MASTER_TIMEOUT_DONE & status))
    {
        retStatus  = CY_SCB_I2C_MASTER_MANUAL_TIMEOUT;
        resetBlock = true;
    }
    else if (0UL != (CY_SCB_I2C_SLAVE_ADDR_DONE & status))
    {
        /* Abort the master operation, the slave was addressed first */
        retStatus = CY_SCB_I2C_MASTER_MANUAL_ABORT_START;

        base->I2C_M_CMD = 0UL;
        context->state = CY_SCB_I2C_IDLE;
    }
    else if (0UL != (CY_SCB_MASTER_INTR_I2C_BUS_ERROR & status))
    {
        retStatus = CY_SCB_I2C_MASTER_MANUAL_BUS_ERR;
        resetBlock = true;
    }
    else if (0UL != (CY_SCB_MASTER_INTR_I2C_ARB_LOST & status))
    {
        retStatus = CY_SCB_I2C_MASTER_MANUAL_ARB_LOST;

        if (CY_SCB_I2C_MASTER_ADDR == context->state)
        {
            /* This is the address phase:
            *  1. Master mode: Reset IP when "arbitration lost" occurs.
            *  2. Master-Slave mode: If "lost arbitration" occurs, the slave
            *  can be addressed to let the slave accept the address; do not
            *  reset IP.
            */
            resetBlock = !_FLD2BOOL(SCB_I2C_CTRL_SLAVE_MODE, base->I2C_CTRL);

            context->state = CY_SCB_I2C_IDLE;
        }
        else
        {
            resetBlock = true;
        }
    }
    else if (0UL != (CY_SCB_MASTER_INTR_I2C_NACK & status))
    {
        /* An address or data was NAKed */
        retStatus = (CY_SCB_I2C_MASTER_ADDR == context->state) ?
                     CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK : CY_SCB_I2C_MASTER_MANUAL_NAK;
    }
    else
    {
        retStatus = CY_SCB_I2C_SUCCESS;

        if (0UL != (CY_SCB_MASTER_INTR_I2C_STOP & status))
        {
            /* End of transaction, go to idle state */
            context->state = CY_SCB_I2C_IDLE;
        }
        else
        {
            /* Continue trasaction */
            if (CY_SCB_I2C_MASTER_ADDR == context->state)
            {
                /* Switch from address to data state */
                context->state = (context->masterRdDir) ?
                                    CY_SCB_I2C_MASTER_RX0 : CY_SCB_I2C_MASTER_TX;
            }
        }
    }

    if (resetBlock)
    {
        /* Back block into default state */
        Cy_SCB_FwBlockReset(base);

        context->state = CY_SCB_I2C_IDLE;
    }
    else
    {
        Cy_SCB_ClearMasterInterrupt(base, CY_SCB_I2C_MASTER_INTR_ALL);
    }

    return (retStatus);
}


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSCB */

/* [] END OF FILE */

