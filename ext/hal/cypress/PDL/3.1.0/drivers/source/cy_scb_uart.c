/***************************************************************************//**
* \file cy_scb_uart.c
* \version 2.20
*
* Provides UART API implementation of the SCB driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_scb_uart.h"

#ifdef CY_IP_MXSCB

#if defined(__cplusplus)
extern "C" {
#endif

/* Static functions */
static void HandleDataReceive (CySCB_Type *base, cy_stc_scb_uart_context_t *context);
static void HandleRingBuffer  (CySCB_Type *base, cy_stc_scb_uart_context_t *context);
static void HandleDataTransmit(CySCB_Type *base, cy_stc_scb_uart_context_t *context);


/*******************************************************************************
* Function Name: Cy_SCB_UART_Init
****************************************************************************//**
*
* Initializes the SCB for UART operation.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param config
* The pointer to configuration structure \ref cy_stc_scb_uart_config_t.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only UART functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return
* \ref cy_en_scb_uart_status_t
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
cy_en_scb_uart_status_t Cy_SCB_UART_Init(CySCB_Type *base, cy_stc_scb_uart_config_t const *config, cy_stc_scb_uart_context_t *context)
{
    if ((NULL == base) || (NULL == config))
    {
        return CY_SCB_UART_BAD_PARAM;
    }

    CY_ASSERT_L3(CY_SCB_UART_IS_MODE_VALID     (config->uartMode));
    CY_ASSERT_L3(CY_SCB_UART_IS_STOP_BITS_VALID(config->stopBits));
    CY_ASSERT_L3(CY_SCB_UART_IS_PARITY_VALID   (config->parity));
    CY_ASSERT_L3(CY_SCB_UART_IS_POLARITY_VALID (config->ctsPolarity));
    CY_ASSERT_L3(CY_SCB_UART_IS_POLARITY_VALID (config->rtsPolarity));

    CY_ASSERT_L2(CY_SCB_UART_IS_OVERSAMPLE_VALID  (config->oversample, config->uartMode, config->irdaEnableLowPowerReceiver));
    CY_ASSERT_L2(CY_SCB_UART_IS_DATA_WIDTH_VALID  (config->dataWidth));
    CY_ASSERT_L2(CY_SCB_UART_IS_ADDRESS_VALID     (config->receiverAddress));
    CY_ASSERT_L2(CY_SCB_UART_IS_ADDRESS_MASK_VALID(config->receiverAddressMask));
    CY_ASSERT_L2(CY_SCB_UART_IS_MUTLI_PROC_VALID  (config->enableMutliProcessorMode, config->uartMode, config->dataWidth,
                                                   config->parity));

    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(config->rxFifoIntEnableMask, CY_SCB_UART_RX_INTR_MASK));
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(config->txFifoIntEnableMask, CY_SCB_UART_TX_INTR_MASK));

    uint32_t ovs;

    if ((CY_SCB_UART_IRDA == config->uartMode) && (!config->irdaEnableLowPowerReceiver))
    {
        /* For Normal IrDA mode oversampling is always zero */
        ovs = 0UL;
    }
    else
    {
        ovs = (config->oversample - 1UL);
    }

    /* Configure the UART interface */
    base->CTRL = _BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, config->acceptAddrInFifo)               |
                 _BOOL2FLD(SCB_CTRL_BYTE_MODE, (config->dataWidth <= CY_SCB_BYTE_WIDTH)) |
                 _VAL2FLD(SCB_CTRL_OVS, ovs)                                             |
                 _VAL2FLD(SCB_CTRL_MODE, CY_SCB_CTRL_MODE_UART);

    /* Configure SCB_CTRL.BYTE_MODE then verify levels */
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, config->rxFifoTriggerLevel));
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, config->txFifoTriggerLevel));
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, config->rtsRxFifoLevel));

    base->UART_CTRL = _VAL2FLD(SCB_UART_CTRL_MODE, (uint32_t) config->uartMode);

    /* Configure the RX direction */
    base->UART_RX_CTRL = _BOOL2FLD(SCB_UART_RX_CTRL_POLARITY, config->irdaInvertRx)                  |
                         _BOOL2FLD(SCB_UART_RX_CTRL_MP_MODE, config->enableMutliProcessorMode)       |
                         _BOOL2FLD(SCB_UART_RX_CTRL_DROP_ON_PARITY_ERROR, config->dropOnParityError) |
                         _BOOL2FLD(SCB_UART_RX_CTRL_DROP_ON_FRAME_ERROR, config->dropOnFrameError)   |
                         _VAL2FLD(SCB_UART_RX_CTRL_BREAK_WIDTH, (config->breakWidth - 1UL))          |
                         _VAL2FLD(SCB_UART_RX_CTRL_STOP_BITS,   ((uint32_t) config->stopBits) - 1UL) |
                         _VAL2FLD(CY_SCB_UART_RX_CTRL_SET_PARITY, (uint32_t) config->parity);

    base->RX_CTRL = _BOOL2FLD(SCB_RX_CTRL_MSB_FIRST, config->enableMsbFirst)          |
                    _BOOL2FLD(SCB_RX_CTRL_MEDIAN, ((config->enableInputFilter) || \
                                             (config->uartMode == CY_SCB_UART_IRDA))) |
                    _VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, (config->dataWidth - 1UL));

    base->RX_MATCH = _VAL2FLD(SCB_RX_MATCH_ADDR, config->receiverAddress) |
                     _VAL2FLD(SCB_RX_MATCH_MASK, config->receiverAddressMask);

    /* Configure SCB_CTRL.RX_CTRL then verify break width */
    CY_ASSERT_L2(CY_SCB_UART_IS_RX_BREAK_WIDTH_VALID(base, config->breakWidth));

    /* Configure the TX direction */
    base->UART_TX_CTRL = _BOOL2FLD(SCB_UART_TX_CTRL_RETRY_ON_NACK, ((config->smartCardRetryOnNack) && \
                                                              (config->uartMode == CY_SCB_UART_SMARTCARD))) |
                         _VAL2FLD(SCB_UART_TX_CTRL_STOP_BITS, ((uint32_t) config->stopBits) - 1UL)          |
                         _VAL2FLD(CY_SCB_UART_TX_CTRL_SET_PARITY, (uint32_t) config->parity);

    base->TX_CTRL  = _BOOL2FLD(SCB_TX_CTRL_MSB_FIRST,  config->enableMsbFirst)    |
                     _VAL2FLD(SCB_TX_CTRL_DATA_WIDTH,  (config->dataWidth - 1UL)) |
                     _BOOL2FLD(SCB_TX_CTRL_OPEN_DRAIN, (config->uartMode == CY_SCB_UART_SMARTCARD));

    base->RX_FIFO_CTRL = _VAL2FLD(SCB_RX_FIFO_CTRL_TRIGGER_LEVEL, config->rxFifoTriggerLevel);

    /* Configure the flow control */
    base->UART_FLOW_CTRL = _BOOL2FLD(SCB_UART_FLOW_CTRL_CTS_ENABLED, config->enableCts) |
                           _BOOL2FLD(SCB_UART_FLOW_CTRL_CTS_POLARITY, (CY_SCB_UART_ACTIVE_HIGH == config->ctsPolarity)) |
                           _BOOL2FLD(SCB_UART_FLOW_CTRL_RTS_POLARITY, (CY_SCB_UART_ACTIVE_HIGH == config->rtsPolarity)) |
                           _VAL2FLD(SCB_UART_FLOW_CTRL_TRIGGER_LEVEL, config->rtsRxFifoLevel);

    base->TX_FIFO_CTRL = _VAL2FLD(SCB_TX_FIFO_CTRL_TRIGGER_LEVEL, config->txFifoTriggerLevel);

    /* Set up interrupt sources */
    base->INTR_RX_MASK = (config->rxFifoIntEnableMask & CY_SCB_UART_RX_INTR_MASK);
    base->INTR_TX_MASK = (config->txFifoIntEnableMask & CY_SCB_UART_TX_INTR_MASK);

    /* Initialize context */
    if (NULL != context)
    {
        context->rxStatus  = 0UL;
        context->txStatus  = 0UL;

        context->rxRingBuf = NULL;
        context->rxRingBufSize = 0UL;

        context->rxBufIdx  = 0UL;
        context->txLeftToTransmit = 0UL;

        context->cbEvents = NULL;

    #if !defined(NDEBUG)
        /* Put an initialization key into the initKey variable to verify
        * context initialization in the transfer API.
        */
        context->initKey = CY_SCB_UART_INIT_KEY;
    #endif /* !(NDEBUG) */
    }

    return CY_SCB_UART_SUCCESS;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_DeInit
****************************************************************************//**
*
* De-initializes the SCB block. Returns the register values to default.
*
* \param base
* The pointer to the UART SCB instance.
*
* \note
* Ensure that the SCB block is disabled before calling this function.
*
*******************************************************************************/
void Cy_SCB_UART_DeInit(CySCB_Type *base)
{
    /* De-initialize the UART interface */
    base->CTRL      = CY_SCB_CTRL_DEF_VAL;
    base->UART_CTRL = CY_SCB_UART_CTRL_DEF_VAL;

    /* De-initialize the RX direction */
    base->UART_RX_CTRL = 0UL;
    base->RX_CTRL      = CY_SCB_RX_CTRL_DEF_VAL;
    base->RX_FIFO_CTRL = 0UL;
    base->RX_MATCH     = 0UL;

    /* De-initialize the TX direction */
    base->UART_TX_CTRL = 0UL;
    base->TX_CTRL      = CY_SCB_TX_CTRL_DEF_VAL;
    base->TX_FIFO_CTRL = 0UL;

    /* De-initialize the flow control */
    base->UART_FLOW_CTRL = 0UL;

    /* De-initialize the interrupt sources */
    base->INTR_SPI_EC_MASK = 0UL;
    base->INTR_I2C_EC_MASK = 0UL;
    base->INTR_RX_MASK     = 0UL;
    base->INTR_TX_MASK     = 0UL;
    base->INTR_M_MASK      = 0UL;
    base->INTR_S_MASK      = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_Disable
****************************************************************************//**
*
* Disables the SCB block and clears context statuses.
* Note that after the block is disabled, the TX and RX FIFOs and
* hardware statuses are cleared. Also, the hardware stops driving the
* output and ignores the input.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only UART functions that do not require context will be used to pass NULL
* as pointer to context.
*
* \note
* Calling this function when the UART is busy (transmitter preforms data
* transfer or receiver is in the middle of data reception) may result transfer
* corruption because the hardware stops driving the outputs and ignores
* the inputs.
* Ensure that the UART is not busy before calling this function.
*
*******************************************************************************/
void Cy_SCB_UART_Disable(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    base->CTRL &= (uint32_t) ~SCB_CTRL_ENABLED_Msk;

    if (NULL != context)
    {
        context->rxStatus  = 0UL;
        context->txStatus  = 0UL;

        context->rxBufIdx  = 0UL;
        context->txLeftToTransmit = 0UL;
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_DeepSleepCallback
****************************************************************************//**
*
* This function handles the transition of the SCB UART into and out of
* Deep Sleep mode. It prevents the device from entering Deep Sleep 
* mode if the UART is transmitting data or has any data in the RX FIFO. If the
* UART is ready to enter Deep Sleep mode, it is disabled. The UART is enabled
* when the device fails to enter Deep Sleep mode or it is awakened from
* Deep Sleep mode. While the UART is disabled, it stops driving the outputs
* and ignores the inputs. Any incoming data is ignored.
*
* This function must be called during execution of \ref Cy_SysPm_DeepSleep,
* to do it, register this function as a callback before calling
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
*******************************************************************************/
cy_en_syspm_status_t Cy_SCB_UART_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    cy_en_syspm_status_t retStatus = CY_SYSPM_FAIL;

    CySCB_Type *locBase = (CySCB_Type *) callbackParams->base;
    cy_stc_scb_uart_context_t *locContext = (cy_stc_scb_uart_context_t *) callbackParams->context;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            /* Check whether the High-level API is not busy executing the transmit
            * or receive operation.
            */
            if ((0UL == (CY_SCB_UART_TRANSMIT_ACTIVE & Cy_SCB_UART_GetTransmitStatus(locBase, locContext))) &&
                (0UL == (CY_SCB_UART_RECEIVE_ACTIVE  & Cy_SCB_UART_GetReceiveStatus (locBase, locContext))))
            {
                /* If all data elements are transmitted from the TX FIFO and
                * shifter and the RX FIFO is empty: the UART is ready to enter
                * Deep Sleep mode.
                */
                if (Cy_SCB_UART_IsTxComplete(locBase))
                {
                    if (0UL == Cy_SCB_UART_GetNumInRxFifo(locBase))
                    {
                        /* Disable the UART. The transmitter stops driving the
                        * lines and the receiver stops receiving data until
                        * the UART is enabled.
                        * This happens when the device failed to enter Deep
                        * Sleep or it is awaken from Deep Sleep mode.
                        */
                        Cy_SCB_UART_Disable(locBase, locContext);

                        retStatus = CY_SYSPM_SUCCESS;
                    }
                }
            }
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {
            /* The other driver is not ready for Deep Sleep mode. Restore the
            * Active mode configuration.
            */

            /* Enable the UART to operate */
            Cy_SCB_UART_Enable(locBase);

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
            /* Do noting: the UART is not capable of waking up from
            * Deep Sleep mode.
            */
        break;

        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* Enable the UART to operate */
            Cy_SCB_UART_Enable(locBase);

            retStatus = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_HibernateCallback
****************************************************************************//**
*
* This function handles the transition of the SCB UART into Hibernate mode. 
* It prevents the device from entering Hibernate mode if the UART is
* transmitting data or has any data in the RX FIFO. If the UART is ready
* to enter Hibernate mode, it is disabled. If the device fails to enter
* Hibernate mode, the UART is enabled. While the UART is disabled, it stops
* driving the outputs and ignores the inputs. Any incoming data is ignored.
*
* This function must be called during execution of \ref Cy_SysPm_Hibernate.
* To do it, register this function as a callback before calling
* \ref Cy_SysPm_Hibernate : specify \ref CY_SYSPM_HIBERNATE as the callback type
* and call \ref Cy_SysPm_RegisterCallback.
*
* \param callbackParams
* The pointer to the callback parameters structure
* \ref cy_stc_syspm_callback_params_t.
*
* \return
* \ref cy_en_syspm_status_t
*
*******************************************************************************/
cy_en_syspm_status_t Cy_SCB_UART_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    return Cy_SCB_UART_DeepSleepCallback(callbackParams);
}


/************************* High-Level Functions ********************************
* The following functions are considered high-level. They provide the layer of
* intelligence to the SCB. These functions require interrupts.
* Low-level and high-level functions must not be mixed because low-level API
* can adversely affect the operation of high-level functions.
*******************************************************************************/


/*******************************************************************************
* Function Name: Cy_SCB_UART_StartRingBuffer
****************************************************************************//**
*
* Starts the receive ring buffer operation.
* The RX interrupt source is configured to get data from the RX
* FIFO and put into the ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param buffer
* Pointer to the user defined ring buffer.
* The item size is defined by the data type, which depends on the configured
* data width.
*
* \param size
* The size of the receive ring buffer.
* Note that one data element is used for internal use, so if the size is 32,
* then only 31 data elements are used for data storage.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* * The buffer must not be modified and stay allocated while the ring buffer
*   operates.
* * This function overrides the RX interrupt sources and changes the
*   RX FIFO level.
*
*******************************************************************************/
void Cy_SCB_UART_StartRingBuffer(CySCB_Type *base, void *buffer, uint32_t size, cy_stc_scb_uart_context_t *context)
{
    CY_ASSERT_L1(NULL != context);
    CY_ASSERT_L1(CY_SCB_UART_INIT_KEY == context->initKey);
    CY_ASSERT_L1(CY_SCB_IS_BUFFER_VALID(buffer, size));

    if ((NULL != buffer) && (size > 0UL))
    {
        uint32_t halfFifoSize =  (Cy_SCB_GetFifoSize(base) / 2UL);

        context->rxRingBuf     = buffer;
        context->rxRingBufSize = size;
        context->rxRingBufHead = 0UL;
        context->rxRingBufTail = 0UL;

        /* Set up an RX interrupt to handle the ring buffer */
        Cy_SCB_SetRxFifoLevel(base, (size >= halfFifoSize) ? (halfFifoSize - 1UL) : (size - 1UL));

        Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_StopRingBuffer
****************************************************************************//**
*
* Stops receiving data into the ring buffer and clears the ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_UART_StopRingBuffer(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    Cy_SCB_SetRxInterruptMask  (base, CY_SCB_CLEAR_ALL_INTR_SRC);
    Cy_SCB_UART_ClearRingBuffer(base, context);

    context->rxRingBuf     = NULL;
    context->rxRingBufSize = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_GetNumInRingBuffer
****************************************************************************//**
*
* Returns the number of data elements in the ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* The number of data elements in the receive ring buffer.
*
* \note
* One data element is used for internal use, so when the buffer is full,
* this function returns (Ring Buffer size - 1).
*
*******************************************************************************/
uint32_t Cy_SCB_UART_GetNumInRingBuffer(CySCB_Type const *base, cy_stc_scb_uart_context_t const *context)
{
    uint32_t size;
    uint32_t locHead = context->rxRingBufHead;

    /* Suppress a compiler warning about unused variables */
    (void) base;

    if (locHead >= context->rxRingBufTail)
    {
        size = (locHead - context->rxRingBufTail);
    }
    else
    {
        size = (locHead + (context->rxBufSize - context->rxRingBufTail));
    }

    return (size);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_ClearRingBuffer
****************************************************************************//**
*
* Clears the ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_UART_ClearRingBuffer(CySCB_Type const *base, cy_stc_scb_uart_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    context->rxRingBufHead = context->rxRingBufTail;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_Receive
****************************************************************************//**
*
* This function starts a UART receive operation.
* It configures the receive interrupt sources to get data available in the
* receive FIFO and returns. The \ref Cy_SCB_UART_Interrupt manages the further
* data transfer.
*
* If the ring buffer is enabled, this function first reads data from the ring
* buffer. If there is more data to receive, it configures the receive interrupt
* sources to copy the remaining bytes from the RX FIFO when they arrive.
*
* When the receive operation is completed (requested number of data elements
* received) the \ref CY_SCB_UART_RECEIVE_ACTIVE status is cleared and
* the \ref CY_SCB_UART_RECEIVE_DONE_EVENT event is generated.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param buffer
* Pointer to buffer to store received data.
* The item size is defined by the data type, which depends on the configured
* data width.
*
* \param size
* The number of data elements to receive.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_uart_status_t
*
* \note
* * The buffer must not be modified and stay allocated until end of the
*   receive operation.
* * This function overrides the RX interrupt sources and changes the
*   RX FIFO level.
*
*******************************************************************************/
cy_en_scb_uart_status_t Cy_SCB_UART_Receive(CySCB_Type *base, void *buffer, uint32_t size, cy_stc_scb_uart_context_t *context)
{
    CY_ASSERT_L1(NULL != context);
    CY_ASSERT_L1(CY_SCB_UART_INIT_KEY == context->initKey);
    CY_ASSERT_L1(CY_SCB_IS_BUFFER_VALID(buffer, size));

    cy_en_scb_uart_status_t retStatus = CY_SCB_UART_RECEIVE_BUSY;

    /* check whether there are no active transfer requests */
    if (0UL == (context->rxStatus & CY_SCB_UART_RECEIVE_ACTIVE))
    {
        uint8_t  *tmpBuf = (uint8_t *) buffer;
        uint32_t numToCopy = 0UL;

        /* Disable the RX interrupt source to stop the ring buffer update */
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

        if (NULL != context->rxRingBuf)
        {
            /* Get the items available in the ring buffer */
            numToCopy = Cy_SCB_UART_GetNumInRingBuffer(base, context);

            if (numToCopy > 0UL)
            {
                uint32_t idx;
                uint32_t locTail = context->rxRingBufTail;
                bool     byteMode = Cy_SCB_IsRxDataWidthByte(base);

                /* Adjust the number of items to be read */
                if (numToCopy > size)
                {
                    numToCopy = size;
                }

                /* Copy the data elements from the ring buffer */
                for (idx = 0UL; idx < numToCopy; ++idx)
                {
                    ++locTail;

                    if (locTail == context->rxRingBufSize)
                    {
                        locTail = 0UL;
                    }

                    if (byteMode)
                    {
                        uint8_t *buf = (uint8_t *) buffer;
                        buf[idx] = ((uint8_t *) context->rxRingBuf)[locTail];
                    }
                    else
                    {
                        uint16_t *buf = (uint16_t *) buffer;
                        buf[idx] = ((uint16_t *) context->rxRingBuf)[locTail];
                    }
                }

                /* Update the ring buffer tail after data has been copied */
                context->rxRingBufTail = locTail;

                /* Update with the copied bytes */
                size -= numToCopy;
                context->rxBufIdx = numToCopy;

                /* Check whether all requested data has been read from the ring buffer */
                if (0UL == size)
                {
                    /* Enable the RX-error interrupt sources to update the error status */
                    Cy_SCB_SetRxInterruptMask(base, CY_SCB_UART_RECEIVE_ERR);

                    /* Call a completion callback if there was no abort receive called
                    * in the interrupt. The abort clears the number of the received bytes.
                    */
                    if (context->rxBufIdx > 0UL)
                    {
                        if (NULL != context->cbEvents)
                        {
                            context->cbEvents(CY_SCB_UART_RECEIVE_DONE_EVENT);
                        }
                    }

                    /* Continue receiving data in the ring buffer */
                    Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);
                }
                else
                {
                    tmpBuf = &tmpBuf[(byteMode) ? (numToCopy) : (2UL * numToCopy)];
                }
            }
        }

        /* Set up a direct RX FIFO receive */
        if (size > 0UL)
        {
            uint32_t halfFifoSize = Cy_SCB_GetFifoSize(base) / 2UL;

            /* Set up context */
            context->rxStatus  = CY_SCB_UART_RECEIVE_ACTIVE;

            context->rxBuf     = (void *) tmpBuf;
            context->rxBufSize = size;
            context->rxBufIdx =  numToCopy;

            /* Set the RX FIFO level to the trigger interrupt */
            Cy_SCB_SetRxFifoLevel(base, (size > halfFifoSize) ? (halfFifoSize - 1UL) : (size - 1UL));

            /* Enable the RX interrupt sources to continue data reading */
            Cy_SCB_SetRxInterruptMask(base, CY_SCB_UART_RX_INTR);
        }

        retStatus = CY_SCB_UART_SUCCESS;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_AbortReceive
****************************************************************************//**
*
* Abort the current receive operation by clearing the receive status.
* * If the ring buffer is disabled, the receive interrupt sources are disabled.
* * If the ring buffer is enabled, the receive interrupt source is configured
*   to get data from the receive FIFO and put it into the ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \note
* * The RX FIFO and ring buffer are not cleared after abort of receive
*   operation.
* * If after the abort of the receive operation the transmitter continues
*   sending data, it gets into the RX FIFO. To drop this data, the RX FIFO
*   and ring buffer (if enabled) must be cleared when the transmitter
*   stops sending data. Otherwise, received data will be kept and copied
*   to the buffer when \ref Cy_SCB_UART_Receive is called.
*
*******************************************************************************/
void Cy_SCB_UART_AbortReceive(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    if (NULL == context->rxRingBuf)
    {
        Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
    }

    context->rxBufSize = 0UL;
    context->rxBufIdx  = 0UL;

    context->rxStatus  = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_GetNumReceived
****************************************************************************//**
*
* Returns the number of data elements received since the last call to \ref
* Cy_SCB_UART_Receive.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* The number of data elements received.
*
*******************************************************************************/
uint32_t Cy_SCB_UART_GetNumReceived(CySCB_Type const *base, cy_stc_scb_uart_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->rxBufIdx);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_GetReceiveStatus
****************************************************************************//**
*
* Returns the status of the receive operation.
* This status is a bit mask and the value returned may have multiple bits set.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_uart_macros_receive_status.
*
* \note
* The status is only cleared by calling \ref Cy_SCB_UART_Receive again.
*
*******************************************************************************/
uint32_t Cy_SCB_UART_GetReceiveStatus(CySCB_Type const *base, cy_stc_scb_uart_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->rxStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_Transmit
****************************************************************************//**
*
* This function starts a UART transmit operation.
* It configures the transmit interrupt sources and returns.
* The \ref Cy_SCB_UART_Interrupt manages the further data transfer.
*
* When the transmit operation is completed (requested number of data elements
* sent on the bus), the \ref CY_SCB_UART_TRANSMIT_ACTIVE status is cleared and
* the \ref CY_SCB_UART_TRANSMIT_DONE_EVENT event is generated.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param buffer
* Pointer to user data to place in transmit buffer.
* The item size is defined by the data type, which depends on the configured
* data width.
*
* \param size
* The number of data elements to transmit.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref cy_en_scb_uart_status_t
*
* \note
* * The buffer must not be modified and must stay allocated until its content is
*   copied into the TX FIFO.
* * This function overrides the TX FIFO interrupt sources and changes the
*   TX FIFO level.
*
*******************************************************************************/
cy_en_scb_uart_status_t Cy_SCB_UART_Transmit(CySCB_Type *base, void *buffer, uint32_t size, cy_stc_scb_uart_context_t *context)
{
    CY_ASSERT_L1(NULL != context);
    CY_ASSERT_L1(CY_SCB_UART_INIT_KEY == context->initKey);
    CY_ASSERT_L1(CY_SCB_IS_BUFFER_VALID(buffer, size));

    cy_en_scb_uart_status_t retStatus = CY_SCB_UART_TRANSMIT_BUSY;

    /* Check whether there are no active transfer requests */
    if (0UL == (CY_SCB_UART_TRANSMIT_ACTIVE & context->txStatus))
    {
        /* Set up context */
        context->txStatus  = CY_SCB_UART_TRANSMIT_ACTIVE;

        context->txBuf     = buffer;
        context->txBufSize = size;

        /* Set the level in TX FIFO to start a transfer */
        Cy_SCB_SetTxFifoLevel(base, (Cy_SCB_GetFifoSize(base) / 2UL));

        /* Enable the interrupt sources */
        if (((uint32_t) CY_SCB_UART_SMARTCARD) == _FLD2VAL(SCB_UART_CTRL_MODE, base->UART_CTRL))
        {
            /* Transfer data into TX FIFO and track SmartCard-specific errors */
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_UART_TX_INTR);
        }
        else
        {
            /* Transfer data into TX FIFO */
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_TX_INTR_LEVEL);
        }

        retStatus = CY_SCB_UART_SUCCESS;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_AbortTransmit
****************************************************************************//**
*
* Aborts the current transmit operation.
* It disables the transmit interrupt sources and clears the transmit FIFO
* and status.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \sideeffect
* The transmit FIFO clear operation also clears the shift register, so that
* the shifter can be cleared in the middle of a data element transfer,
* corrupting it. The data element corruption means that all bits that have
* not been transmitted are transmitted as "ones" on the bus.
*
*******************************************************************************/
void Cy_SCB_UART_AbortTransmit(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

    Cy_SCB_UART_ClearTxFifo(base);

    context->txBufSize = 0UL;
    context->txLeftToTransmit = 0UL;

    context->txStatus  = 0UL;
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_GetNumLeftToTransmit
****************************************************************************//**
*
* Returns the number of data elements left to transmit since the last call to
* \ref Cy_SCB_UART_Transmit.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* The number of data elements left to transmit.
*
*******************************************************************************/
uint32_t Cy_SCB_UART_GetNumLeftToTransmit(CySCB_Type const *base, cy_stc_scb_uart_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->txLeftToTransmit);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_GetTransmitStatus
****************************************************************************//**
*
* Returns the status of the transmit operation.
* This status is a bit mask and the value returned may have multiple bits set.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \return
* \ref group_scb_uart_macros_transmit_status.
*
* \note
* The status is only cleared by calling \ref Cy_SCB_UART_Transmit or
* \ref Cy_SCB_UART_AbortTransmit.
*
*******************************************************************************/
uint32_t Cy_SCB_UART_GetTransmitStatus(CySCB_Type const *base, cy_stc_scb_uart_context_t const *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    return (context->txStatus);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_SendBreakBlocking
****************************************************************************//**
*
* Sends a break condition (logic low) of specified width on UART TX line.
* Blocks until break is completed. Only call this function when UART TX FIFO
* and shifter are empty.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param breakWidth
* Width of break condition. Valid range is the TX data width (4 to 16 bits)
*
* \note
* Before sending break all UART TX interrupt sources are disabled. The state
* of UART TX interrupt sources is restored before function returns.
*
* \sideeffect
* If this function is called while there is data in the TX FIFO or shifter that
* data will be shifted out in packets the size of breakWidth.
*
*******************************************************************************/
void Cy_SCB_UART_SendBreakBlocking(CySCB_Type *base, uint32_t breakWidth)
{
    uint32_t txCtrlReg;
    uint32_t txIntrReg;

    CY_ASSERT_L2(CY_SCB_UART_IS_TX_BREAK_WIDTH_VALID(breakWidth));

    /* Disable all UART TX interrupt sources and clear UART TX Done history */
    txIntrReg = Cy_SCB_GetTxInterruptMask(base);
    Cy_SCB_SetTxInterruptMask(base, 0UL);
    Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UART_DONE);

    /* Store TX_CTRL configuration */
    txCtrlReg = base->TX_CTRL;

    /* Set break width: start bit adds one 0 bit */
    base->TX_CTRL = _CLR_SET_FLD32U(base->TX_CTRL, SCB_TX_CTRL_DATA_WIDTH, (breakWidth - 1UL));

    /* Generate break */
    Cy_SCB_WriteTxFifo(base, 0UL);

    /* Wait for break completion */
    while (0UL == (Cy_SCB_GetTxInterruptStatus(base) & CY_SCB_TX_INTR_UART_DONE))
    {
    }

    /* Clear all UART TX interrupt sources */
    Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_MASK);

    /* Restore TX data width and interrupt sources */
    base->TX_CTRL = txCtrlReg;
    Cy_SCB_SetTxInterruptMask(base, txIntrReg);
}


/*******************************************************************************
* Function Name: Cy_SCB_UART_Interrupt
****************************************************************************//**
*
* This is the interrupt function for the SCB configured in the UART mode.
* This function must be called inside a user-defined interrupt service
* routine to make \ref Cy_SCB_UART_Transmit and \ref Cy_SCB_UART_Receive
* work.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void Cy_SCB_UART_Interrupt(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    if (0UL != (CY_SCB_RX_INTR & Cy_SCB_GetInterruptCause(base)))
    {
        /* Get RX error events: a frame error, parity error, and overflow */
        uint32_t locRxErr = (CY_SCB_UART_RECEIVE_ERR & Cy_SCB_GetRxInterruptStatusMasked(base));

        /* Handle the error conditions */
        if (0UL != locRxErr)
        {
            context->rxStatus |= locRxErr;

            Cy_SCB_ClearRxInterrupt(base, locRxErr);

            if (NULL != context->cbEvents)
            {
                context->cbEvents(CY_SCB_UART_RECEIVE_ERR_EVENT);
            }
        }

        /* Break the detect */
        if (0UL != (CY_SCB_RX_INTR_UART_BREAK_DETECT & Cy_SCB_GetRxInterruptStatusMasked(base)))
        {
            context->rxStatus |= CY_SCB_UART_RECEIVE_BREAK_DETECT;

            Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_UART_BREAK_DETECT);
        }

        /* Copy the received data */
        if (0UL != (CY_SCB_RX_INTR_LEVEL & Cy_SCB_GetRxInterruptStatusMasked(base)))
        {
            if (context->rxBufSize > 0UL)
            {
                HandleDataReceive(base, context);
            }
            else
            {
                if (NULL != context->rxRingBuf)
                {
                    HandleRingBuffer(base, context);
                }
            }

            Cy_SCB_ClearRxInterrupt(base, CY_SCB_RX_INTR_LEVEL);
        }
    }

    if (0UL != (CY_SCB_TX_INTR & Cy_SCB_GetInterruptCause(base)))
    {
        uint32_t locTxErr = (CY_SCB_UART_TRANSMIT_ERR & Cy_SCB_GetTxInterruptStatusMasked(base));

        /* Handle the TX error conditions */
        if (0UL != locTxErr)
        {
            context->txStatus |= locTxErr;
            Cy_SCB_ClearTxInterrupt(base, locTxErr);

            if (NULL != context->cbEvents)
            {
                context->cbEvents(CY_SCB_UART_TRANSMIT_ERR_EVENT);
            }
        }

        /* Load data to transmit */
        if (0UL != (CY_SCB_TX_INTR_LEVEL & Cy_SCB_GetTxInterruptStatusMasked(base)))
        {
            HandleDataTransmit(base, context);

            Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_LEVEL);
        }

        /* Handle the TX complete */
        if (0UL != (CY_SCB_TX_INTR_UART_DONE & Cy_SCB_GetTxInterruptStatusMasked(base)))
        {
            /* Disable all TX interrupt sources */
            Cy_SCB_SetTxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

            context->txStatus &= (uint32_t) ~CY_SCB_UART_TRANSMIT_ACTIVE;
            context->txLeftToTransmit = 0UL;

            if (NULL != context->cbEvents)
            {
                context->cbEvents(CY_SCB_UART_TRANSMIT_DONE_EVENT);
            }
        }
    }
}



/*******************************************************************************
* Function Name: HandleDataReceive
****************************************************************************//**
*
* Reads data from the receive FIFO into the buffer provided by
* \ref Cy_SCB_UART_Receive.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void HandleDataReceive(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    uint32_t numCopied;
    uint32_t halfFifoSize = Cy_SCB_GetFifoSize(base) / 2UL;

    /* Get data from RX FIFO */
    numCopied = Cy_SCB_UART_GetArray(base, context->rxBuf, context->rxBufSize);

    /* Move the buffer */
    context->rxBufIdx  += numCopied;
    context->rxBufSize -= numCopied;

    if (0UL == context->rxBufSize)
    {
        if (NULL != context->rxRingBuf)
        {
            /* Adjust the level to proceed with the ring buffer */
            Cy_SCB_SetRxFifoLevel(base, (context->rxRingBufSize >= halfFifoSize) ?
                                            (halfFifoSize - 1UL) : (context->rxRingBufSize - 1UL));

            Cy_SCB_SetRxInterruptMask(base, CY_SCB_RX_INTR_LEVEL);
        }
        else
        {
            Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);
        }

        /* Update the status */
        context->rxStatus &= (uint32_t) ~CY_SCB_UART_RECEIVE_ACTIVE;

        /* Notify that receive is done in a callback */
        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_UART_RECEIVE_DONE_EVENT);
        }
    }
    else
    {
        uint8_t *buf = (uint8_t *) context->rxBuf;

        buf = &buf[(Cy_SCB_IsRxDataWidthByte(base) ? (numCopied) : (2UL * numCopied))];
        context->rxBuf = (void *) buf;

        if (context->rxBufSize < halfFifoSize)
        {
            /* Set the RX FIFO level to trigger an interrupt */
            Cy_SCB_SetRxFifoLevel(base, (context->rxBufSize - 1UL));
        }
    }
}


/*******************************************************************************
* Function Name: HandleRingBuffer
****************************************************************************//**
*
* Reads data from the receive FIFO into the receive ring buffer.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void HandleRingBuffer(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    uint32_t halfFifoSize = Cy_SCB_GetFifoSize(base) / 2UL;
    uint32_t numToCopy = Cy_SCB_GetNumInRxFifo(base);
    uint32_t locHead = context->rxRingBufHead;
    uint32_t rxData;

    /* Get data into the ring buffer */
    while (numToCopy > 0UL)
    {
        ++locHead;

        if (locHead == context->rxRingBufSize)
        {
            locHead = 0UL;
        }

        if (locHead == context->rxRingBufTail)
        {
            /* The ring buffer is full, trigger a callback */
            if (NULL != context->cbEvents)
            {
                context->cbEvents(CY_SCB_UART_RB_FULL_EVENT);
            }

            /* The ring buffer is still full. Disable the RX interrupt not to put data into the ring buffer.
            * The data is stored in the RX FIFO until it overflows. Revert the head index.
            */
            if (locHead == context->rxRingBufTail)
            {
                Cy_SCB_SetRxInterruptMask(base, CY_SCB_CLEAR_ALL_INTR_SRC);

                locHead = (locHead > 0UL) ? (locHead - 1UL) : (context->rxRingBufSize - 1UL);
                break;
            }
        }

        /* Get data from RX FIFO. */
        rxData = Cy_SCB_ReadRxFifo(base);

        /* Put a data item in the ring buffer */
        if (Cy_SCB_IsRxDataWidthByte(base))
        {
            ((uint8_t *) context->rxRingBuf)[locHead] = (uint8_t) rxData;
        }
        else
        {
            ((uint16_t *) context->rxRingBuf)[locHead] = (uint16_t) rxData;
        }

        --numToCopy;
    }

    /* Update the head index */
    context->rxRingBufHead = locHead;

    /* Get free entries in the ring buffer */
    numToCopy = context->rxRingBufSize - Cy_SCB_UART_GetNumInRingBuffer(base, context);

    if (numToCopy < halfFifoSize)
    {
        /* Adjust the level to copy to the ring buffer */
        uint32_t level = (numToCopy > 0UL) ? (numToCopy - 1UL) : 0UL;
        Cy_SCB_SetRxFifoLevel(base, level);
    }
}


/*******************************************************************************
* Function Name: HandleDataTransmit
****************************************************************************//**
*
* Loads the transmit FIFO with data provided by \ref Cy_SCB_UART_Transmit.
*
* \param base
* The pointer to the UART SCB instance.
*
* \param context
* The pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
static void HandleDataTransmit(CySCB_Type *base, cy_stc_scb_uart_context_t *context)
{
    uint32_t numToCopy;
    uint32_t fifoSize = Cy_SCB_GetFifoSize(base);
    bool     byteMode = Cy_SCB_IsTxDataWidthByte(base);

    if (context->txBufSize > 1UL)
    {
        uint8_t *buf = (uint8_t *) context->txBuf;

        /* Get the number of items left for transmission */
        context->txLeftToTransmit = context->txBufSize;

        /* Put data into TX FIFO */
        numToCopy = Cy_SCB_UART_PutArray(base, context->txBuf, (context->txBufSize - 1UL));

        /* Move the buffer */
        context->txBufSize -= numToCopy;

        buf = &buf[(byteMode) ? (numToCopy) : (2UL * numToCopy)];
        context->txBuf = (void *) buf;
    }

    /* Put the last data item into TX FIFO */
    if ((fifoSize != Cy_SCB_GetNumInTxFifo(base)) && (1UL == context->txBufSize))
    {
        uint32_t txData;
        uint32_t intrStatus;

        context->txBufSize = 0UL;

        /* Get the last item from the buffer */
        txData = (uint32_t) ((byteMode) ? ((uint8_t *)  context->txBuf)[0UL] :
                                          ((uint16_t *) context->txBuf)[0UL]);

        /* Put the last data element and make sure that "TX done" will happen for it */
        intrStatus = Cy_SysLib_EnterCriticalSection();

        Cy_SCB_WriteTxFifo(base, txData);
        Cy_SCB_ClearTxInterrupt(base, CY_SCB_TX_INTR_UART_DONE);

        Cy_SysLib_ExitCriticalSection(intrStatus);

        /* Disable the level interrupt source and enable "transfer done" */
        Cy_SCB_SetTxInterruptMask(base, (CY_SCB_TX_INTR_UART_DONE |
                    (Cy_SCB_GetTxInterruptMask(base) & (uint32_t) ~CY_SCB_TX_INTR_LEVEL)));

        /* Data is copied into TX FIFO */
        context->txStatus |= CY_SCB_UART_TRANSMIT_IN_FIFO;

        if (NULL != context->cbEvents)
        {
            context->cbEvents(CY_SCB_UART_TRANSMIT_IN_FIFO_EVENT);
        }
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSCB */

/* [] END OF FILE */

