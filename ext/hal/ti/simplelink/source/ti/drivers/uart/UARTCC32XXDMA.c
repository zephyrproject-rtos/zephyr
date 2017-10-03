/*
 * Copyright (c) 2014-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

/*
 * By default disable both asserts and log for this module.
 * This must be done before DebugP.h is included.
 */
#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif
#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED 0
#endif

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/uart/UARTCC32XXDMA.h>
#include <ti/drivers/dma/UDMACC32XX.h>

/* driverlib header files */
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/inc/hw_uart.h>
#include <ti/devices/cc32xx/driverlib/uart.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

/* DMA can handle transfers of at most 1024 elements */
#define MAXXFERSIZE 1024

/* Pad configuration defines */
#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

/* UARTCC32XXDMA functions */
void         UARTCC32XXDMA_close(UART_Handle handle);
int_fast16_t UARTCC32XXDMA_control(UART_Handle handle, uint_fast16_t cmd,
                                   void *arg);
void         UARTCC32XXDMA_init(UART_Handle handle);
UART_Handle  UARTCC32XXDMA_open(UART_Handle handle, UART_Params *params);
int_fast32_t UARTCC32XXDMA_read(UART_Handle handle, void *buffer, size_t size);
void         UARTCC32XXDMA_readCancel(UART_Handle handle);
int_fast32_t UARTCC32XXDMA_readPolling(UART_Handle handle, void *buffer,
                                       size_t size);
int_fast32_t UARTCC32XXDMA_write(UART_Handle handle, const void *buffer,
                                 size_t size);
void         UARTCC32XXDMA_writeCancel(UART_Handle handle);
int_fast32_t UARTCC32XXDMA_writePolling(UART_Handle handle, const void *buffer,
                                    size_t size);

/* UART function table for UARTCC32XXDMA implementation */
const UART_FxnTable UARTCC32XXDMA_fxnTable = {
    UARTCC32XXDMA_close,
    UARTCC32XXDMA_control,
    UARTCC32XXDMA_init,
    UARTCC32XXDMA_open,
    UARTCC32XXDMA_read,
    UARTCC32XXDMA_readPolling,
    UARTCC32XXDMA_readCancel,
    UARTCC32XXDMA_write,
    UARTCC32XXDMA_writePolling,
    UARTCC32XXDMA_writeCancel
};

/* Static functions */
static void UARTCC32XXDMA_configDMA(UART_Handle handle, bool isWrite);
static void UARTCC32XXDMA_hwiIntFxn(uintptr_t arg);

static unsigned int getPowerMgrId(unsigned int baseAddr);
static void initHw(UART_Handle handle);
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
        uintptr_t clientArg);

static size_t readCancel(UART_Handle handle);
static void readSemCallback(UART_Handle handle, void *buffer, size_t count);
static void startTxFifoEmptyClk(UART_Handle handle, unsigned int size);
static size_t writeCancel(UART_Handle handle);
static void writeFinishedDoCallback(UART_Handle handle);
static void writeSemCallback(UART_Handle handle, void *buffer, size_t count);

/*
 * Function for checking whether flow control is enabled.
 */
static inline bool isFlowControlEnabled(UARTCC32XXDMA_HWAttrsV1 const  *hwAttrs) {
    return ((hwAttrs->flowControl == UARTCC32XXDMA_FLOWCTRL_HARDWARE) &&
        (hwAttrs->ctsPin != UARTCC32XXDMA_PIN_UNASSIGNED) &&
            (hwAttrs->rtsPin != UARTCC32XXDMA_PIN_UNASSIGNED));
}


static const uint32_t dataLength[] = {
    UART_CONFIG_WLEN_5, /* UART_LEN_5 */
    UART_CONFIG_WLEN_6, /* UART_LEN_6 */
    UART_CONFIG_WLEN_7, /* UART_LEN_7 */
    UART_CONFIG_WLEN_8  /* UART_LEN_8 */
};

static const uint32_t stopBits[] = {
    UART_CONFIG_STOP_ONE,   /* UART_STOP_ONE */
    UART_CONFIG_STOP_TWO    /* UART_STOP_TWO */
};

static const uint32_t parityType[] = {
    UART_CONFIG_PAR_NONE,   /* UART_PAR_NONE */
    UART_CONFIG_PAR_EVEN,   /* UART_PAR_EVEN */
    UART_CONFIG_PAR_ODD,    /* UART_PAR_ODD */
    UART_CONFIG_PAR_ZERO,   /* UART_PAR_ZERO */
    UART_CONFIG_PAR_ONE     /* UART_PAR_ONE */
};

/*
 *  ======== UARTCC32XXDMA_close ========
 */
void UARTCC32XXDMA_close(UART_Handle handle)
{
    UARTCC32XXDMA_Object           *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const  *hwAttrs = handle->hwAttrs;
    uint32_t                       padRegister;

    /* Disable UART and interrupts. */
    MAP_UARTDMADisable(hwAttrs->baseAddr, UART_DMA_TX | UART_DMA_RX);
    MAP_UARTDisable(hwAttrs->baseAddr);

    if (object->hwiHandle) {
        HwiP_delete(object->hwiHandle);
    }
    if (object->writeSem) {
        SemaphoreP_delete(object->writeSem);
    }
    if (object->readSem) {
        SemaphoreP_delete(object->readSem);
    }
    if (object->txFifoEmptyClk) {
        ClockP_delete(object->txFifoEmptyClk);
    }

    if (object->dmaHandle) {
        UDMACC32XX_close(object->dmaHandle);
    }

    Power_unregisterNotify(&object->postNotify);
    Power_releaseDependency(object->powerMgrId);

    if (object->txPin != (uint16_t)-1) {
        PowerCC32XX_restoreParkState((PowerCC32XX_Pin)object->txPin,
            object->prevParkTX);
        object->txPin = (uint16_t)-1;
    }

    if (object->rtsPin != (uint16_t)-1) {
        PowerCC32XX_restoreParkState((PowerCC32XX_Pin)object->rtsPin,
            object->prevParkRTS);
        object->rtsPin = (uint16_t)-1;
    }

    /* Restore pin pads to their reset states */
    padRegister = (PinToPadGet((hwAttrs->rxPin) & 0xff)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->txPin) & 0xff)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    if (isFlowControlEnabled(hwAttrs)) {
        padRegister = (PinToPadGet((hwAttrs->ctsPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
        padRegister = (PinToPadGet((hwAttrs->rtsPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
    }

    object->opened = false;

    DebugP_log1("UART:(%p) closed", hwAttrs->baseAddr);
}

/*
 *  ======== UARTCC32XXDMA_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t UARTCC32XXDMA_control(UART_Handle handle, uint_fast16_t cmd,
        void *arg)
{
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    switch (cmd) {
        /* Specific UART CMDs */
        case (UARTCC32XXDMA_CMD_IS_BUSY):
            *(bool *)arg = MAP_UARTBusy(hwAttrs->baseAddr);
            return (UART_STATUS_SUCCESS);

        case (UARTCC32XXDMA_CMD_IS_RX_DATA_AVAILABLE):
            *(bool *)arg = MAP_UARTCharsAvail(hwAttrs->baseAddr);
            return (UART_STATUS_SUCCESS);

        case (UARTCC32XXDMA_CMD_IS_TX_SPACE_AVAILABLE):
            *(bool *)arg = MAP_UARTSpaceAvail(hwAttrs->baseAddr);
            return (UART_STATUS_SUCCESS);

        default:
            DebugP_log2("UART:(%p) UART CMD undefined: %d",
                hwAttrs->baseAddr, cmd);
            return (UART_STATUS_UNDEFINEDCMD);
    }
}

/*
 *  ======== UARTCC32XXDMA_init ========
 */
void UARTCC32XXDMA_init(UART_Handle handle)
{
}

/*
 *  ======== UARTCC32XXDMA_open ========
 */
UART_Handle UARTCC32XXDMA_open(UART_Handle handle, UART_Params *params)
{
    uintptr_t                      key;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    SemaphoreP_Params              semParams;
    HwiP_Params                    hwiParams;
    ClockP_Params                  clockParams;
    uint16_t                       pin;
    uint16_t                       mode;

    /* Timeouts cannot be 0 */
    DebugP_assert((params->writeTimeout != 0) && (params->readTimeout != 0));

    /* Check that a callback is set */
    DebugP_assert((params->readMode != UART_MODE_CALLBACK) ||
                  (params->readCallback != NULL));
    DebugP_assert((params->writeMode != UART_MODE_CALLBACK) ||
                  (params->writeCallback != NULL));

    /* Initialize the DMA */
    UDMACC32XX_init();

    object->powerMgrId = getPowerMgrId(hwAttrs->baseAddr);
    if (object->powerMgrId == (unsigned int)-1) {
        DebugP_log1("UART:(%p) Failed to determine power resource id",
                hwAttrs->baseAddr);
        return (NULL);
    }

    /* Disable preemption while checking if the UART is open. */
    key = HwiP_disable();

    /* Check if the UART is open already with the base addr. */
    if (object->opened == true) {
        HwiP_restore(key);

        DebugP_log1("UART:(%p) already in use.", hwAttrs->baseAddr);
        return (NULL);
    }

    object->opened = true;
    HwiP_restore(key);

    /*
     *  Register power dependency. Keeps the clock running in SLP
     *  and DSLP modes.
     */

    Power_setDependency(object->powerMgrId);

    Power_registerNotify(&object->postNotify, PowerCC32XX_AWAKE_LPDS,
            postNotifyFxn, (uintptr_t)handle);

    object->readMode       = params->readMode;
    object->writeMode      = params->writeMode;
    object->readTimeout    = params->readTimeout;
    object->writeTimeout   = params->writeTimeout;
    object->readCallback   = params->readCallback;
    object->writeCallback  = params->writeCallback;
    object->readReturnMode = params->readReturnMode;
    object->readDataMode   = params->readDataMode;
    object->writeDataMode  = params->writeDataMode;
    object->readEcho       = params->readEcho;
    object->baudRate       = params->baudRate;
    object->stopBits       = params->stopBits;
    object->dataLength     = params->dataLength;
    object->parityType     = params->parityType;

    /* Set UART variables to defaults. */
    object->writeBuf = NULL;
    object->readBuf = NULL;
    object->writeCount = 0;
    object->readCount = 0;
    object->writeSize = 0;
    object->readSize = 0;
    object->readSem = NULL;
    object->writeSem = NULL;
    object->txFifoEmptyClk = NULL;
    object->txPin = (uint16_t)-1;

    /* DMA first */
    object->dmaHandle = UDMACC32XX_open();
    if (object->dmaHandle == NULL) {
        UARTCC32XXDMA_close(handle);
        DebugP_log1("UART:(%p) UDMACC32XX_open() failed.", hwAttrs->baseAddr);
        return (NULL);
    }

    pin = (hwAttrs->rxPin) & 0xff;
    mode = (hwAttrs->rxPin >> 8) & 0xff;

    MAP_PinTypeUART((unsigned long)pin, (unsigned long)mode);

    pin = (hwAttrs->txPin) & 0xff;
    mode = (hwAttrs->txPin >> 8) & 0xff;

    MAP_PinTypeUART((unsigned long)pin, (unsigned long)mode);

    /*
     * Read and save TX pin park state; set to "don't park" while UART is
     * open as device default is logic '1' during LPDS
     */
    object->prevParkTX =
        (PowerCC32XX_ParkState) PowerCC32XX_getParkState((PowerCC32XX_Pin)pin);
    PowerCC32XX_setParkState((PowerCC32XX_Pin)pin, ~1);
    object->txPin = pin;

    if (isFlowControlEnabled(hwAttrs)) {
        pin = (hwAttrs->ctsPin) & 0xff;
        mode = (hwAttrs->ctsPin >> 8) & 0xff;
        MAP_PinTypeUART((unsigned long)pin, (unsigned long)mode);

        pin = (hwAttrs->rtsPin) & 0xff;
        mode = (hwAttrs->rtsPin >> 8) & 0xff;
        MAP_PinTypeUART((unsigned long)pin, (unsigned long)mode);

        /*
         * Read and save RTS pin park state; set to "don't park" while UART is
         * open as device default is logic '1' during LPDS
         */
        object->prevParkRTS = (PowerCC32XX_ParkState)PowerCC32XX_getParkState(
            (PowerCC32XX_Pin)pin);
        PowerCC32XX_setParkState((PowerCC32XX_Pin)pin, ~1);
        object->rtsPin = pin;

        /* Flow control will be enabled in initHw() */
    }
    HwiP_clearInterrupt(hwAttrs->intNum);

    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(hwAttrs->intNum,
                                    UARTCC32XXDMA_hwiIntFxn,
                                    &hwiParams);
    if (object->hwiHandle == NULL) {
        DebugP_log1("UART:(%p) HwiP_create() failed", hwAttrs->baseAddr);
        UARTCC32XXDMA_close(handle);
        return (NULL);
    }

    /* Disable the UART interrupt. */
    MAP_UARTIntDisable(hwAttrs->baseAddr,
                      (UART_INT_TX | UART_INT_RX | UART_INT_RT));

    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;

    /* Create semaphores and set callbacks for BLOCKING modes. */
    if (object->writeMode == UART_MODE_BLOCKING) {
        object->writeCallback = &writeSemCallback;

        object->writeSem = SemaphoreP_create(0, &semParams);
        if (object->writeSem == NULL) {
            UARTCC32XXDMA_close(handle);
            DebugP_log1("UART:(%p) Failed to create semaphore.",
                        hwAttrs->baseAddr);
            return (NULL);
        }
    }

    if (object->readMode == UART_MODE_BLOCKING) {
        object->readCallback = &readSemCallback;

        object->readSem = SemaphoreP_create(0, &semParams);
        if (object->readSem == NULL) {
            UARTCC32XXDMA_close(handle);
            DebugP_log1("UART:(%p) Failed to create semaphore.",
                        hwAttrs->baseAddr);
            return (NULL);
        }
    }

    /*
     *  Clock object to ensure FIFO is drained before releasing Power
     *  constraints.
     */
    ClockP_Params_init(&clockParams);
    clockParams.arg = (uintptr_t)handle;

    object->txFifoEmptyClk = ClockP_create((ClockP_Fxn)&writeFinishedDoCallback,
            0 /* timeout */, &(clockParams));

    if (object->txFifoEmptyClk == NULL) {
        UARTCC32XXDMA_close(handle);
        DebugP_log1("UART:(%p) ClockP_create() failed.",
                    hwAttrs->baseAddr);
        return (NULL);
    }

    /* Initialize the hardware */
    initHw(handle);


    DebugP_log1("UART:(%p) opened", hwAttrs->baseAddr);

    /* Return the handle */
    return (handle);
}

/*
 *  ======== UARTCC32XXDMA_read ========
 */
int_fast32_t UARTCC32XXDMA_read(UART_Handle handle, void *buffer, size_t size)
{
    uintptr_t             key;
    UARTCC32XXDMA_Object *object = handle->object;

    /* DMA cannot handle transfer sizes > 1024 elements */
    if (size > MAXXFERSIZE) {
        DebugP_log1("UART:(%p) Data size too large.",
                ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);

        return (UART_ERROR);
    }

    /* Disable preemption while checking if the uart is in use. */
    key = HwiP_disable();
    if (object->readSize) {
        HwiP_restore(key);

        DebugP_log1("UART:(%p) Could not read data, uart in use.",
                ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);
        return (UART_ERROR);
    }

    /* Save the data to be read and restore interrupts. */
    object->readBuf = buffer;
    object->readSize = size;
    object->readCount = 0;

    /* Set constraints to guarantee transaction */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    /*
     *  Start the DMA transfer.  Do this inside the critical
     *  section to prevent UARTCC32XXDMA_readCancel() being called
     *  after object->readSize is set, but before the DMA is
     *  configured.  If that happened, the size in the DMA control
     *  register would be 0, causing UARTCC32XXDMA_readCancel()
     *  to assume all bytes were transferred.
     */
    UARTCC32XXDMA_configDMA(handle, false /* isWrite */);

    HwiP_restore(key);

    /* If readMode is blocking, block and get the status. */
    if (object->readMode == UART_MODE_BLOCKING) {

        /* Pend on semaphore and wait for Hwi to finish. */
        if (SemaphoreP_OK != SemaphoreP_pend(object->readSem,
                    object->readTimeout)) {
            key = HwiP_disable();

            /* Cancel the DMA without posting the semaphore */
            (void)readCancel(handle);

            /*
             *  If ISR ran after timeout, but before the call to
             *  readCancel(), readSem would be posted. Pend on
             *  the semaphore with 0 timeout so the next read
             *  will block.
             */
            if (object->readCount == size) {
                SemaphoreP_pend(object->readSem, 0);
            }

            HwiP_restore(key);

            DebugP_log2("UART:(%p) Read timed out, %d bytes read",
                    ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr,
                    object->readCount);

        }
        return (object->readCount);
    }

    return (0);
}

/*
 *  ======== UARTCC32XXDMA_readPolling ========
 */
int_fast32_t UARTCC32XXDMA_readPolling(UART_Handle handle, void *buf,
        size_t size)
{
    int32_t                        count = 0;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    unsigned char                 *buffer = (unsigned char *)buf;

    /* Read characters. */
    while (size) {
        *buffer = MAP_UARTCharGet(hwAttrs->baseAddr);
        DebugP_log2("UART:(%p) Read character 0x%x",
                    hwAttrs->baseAddr, *buffer);
        count++;
        size--;

        if (object->readDataMode == UART_DATA_TEXT && *buffer == '\r') {
            /* Echo character if enabled. */
            if (object->readEcho) {
                MAP_UARTCharPut(hwAttrs->baseAddr, '\r');
            }
            *buffer = '\n';
        }

        /* Echo character if enabled. */
        if (object->readEcho) {
            MAP_UARTCharPut(hwAttrs->baseAddr, *buffer);
        }

        /* If read return mode is newline, finish if a newline was received. */
        if (object->readReturnMode == UART_RETURN_NEWLINE &&
            *buffer == '\n') {
            return (count);
        }

        buffer++;
    }

    DebugP_log2("UART:(%p) Read polling finished, %d bytes read",
                hwAttrs->baseAddr, count);

    return (count);
}

/*
 *  ======== UARTCC32XXDMA_readCancel ========
 */
void UARTCC32XXDMA_readCancel(UART_Handle handle)
{
    UARTCC32XXDMA_Object *object = handle->object;
    size_t                size;

    /* Stop any ongoing DMA read */
    size = readCancel(handle);

    if (size == 0) {
        return;
    }

    if (object->readMode == UART_MODE_CALLBACK) {
        object->readCallback(handle, object->readBuf, object->readCount);
    }
    else if (object->readMode == UART_MODE_BLOCKING) {
        /* We don't know if we're in an ISR, but we'll assume not. */
        SemaphoreP_post(object->readSem);
    }

    DebugP_log1("UART:(%p) Read canceled, 0 bytes read",
            ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);
}

/*
 *  ======== UARTCC32XXDMA_write ========
 */
int_fast32_t UARTCC32XXDMA_write(UART_Handle handle, const void *buffer,
        size_t size)
{
    uintptr_t             key;
    UARTCC32XXDMA_Object *object = handle->object;

    /* DMA cannot handle transfer sizes > 1024 elements */
    if (size > MAXXFERSIZE) {
        DebugP_log1("UART:(%p) Data size too large.",
                ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);

        return (UART_ERROR);
    }

    /* Disable preemption while checking if the uart is in use. */
    key = HwiP_disable();
    if (object->writeSize) {
        HwiP_restore(key);
        DebugP_log1("UART:(%p) Could not write data, uart in use.",
                ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);

        return (UART_ERROR);
    }

    /* Save the data to be written and restore interrupts. */
    object->writeBuf = buffer;
    object->writeCount = 0;
    object->writeSize = size;

    /* Set constraints to guarantee transaction */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    UARTCC32XXDMA_configDMA(handle, true /* isWrite */);

    HwiP_restore(key);

    /* If writeMode is blocking, block and get the status. */
    if (object->writeMode == UART_MODE_BLOCKING) {
        /* Pend on semaphore and wait for Hwi to finish. */
        if (SemaphoreP_OK != SemaphoreP_pend(object->writeSem,
                    object->writeTimeout)) {

            key = HwiP_disable();

            /* Stop any ongoing DMA writes and release Power constraints. */
            (void)writeCancel(handle);

            /*
             *  If ISR ran after timeout, but before the call to
             *  writeCancel(), writeSem would be posted. Pend on
             *  the semaphore so the next write call will block.
             */
            if (object->writeCount == size) {
                SemaphoreP_pend(object->writeSem, 0);
            }

            HwiP_restore(key);

            DebugP_log2("UART:(%p) Write timed out, %d bytes written",
                    ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr,
                    object->writeCount);

        }
        return (object->writeCount);
    }

    return (0);
}

/*
 *  ======== UARTCC32XXDMA_writePolling ========
 */
int_fast32_t UARTCC32XXDMA_writePolling(UART_Handle handle, const void *buf,
        size_t size)
{
    int32_t                        count = 0;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    unsigned char                 *buffer = (unsigned char *)buf;

    /* Write characters. */
    while (size) {
        if (object->writeDataMode == UART_DATA_TEXT && *buffer == '\n') {
            MAP_UARTCharPut(hwAttrs->baseAddr, '\r');
            count++;
        }
        MAP_UARTCharPut(hwAttrs->baseAddr, *buffer);

        DebugP_log2("UART:(%p) Wrote character 0x%x",
                ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr,
                *buffer);
        buffer++;
        count++;
        size--;
    }

    DebugP_log2("UART:(%p) Write polling finished, %d bytes written",
            ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr,
            count);

    return (count);
}

/*
 *  ======== UARTCC32XXDMA_writeCancel ========
 */
void UARTCC32XXDMA_writeCancel(UART_Handle handle)
{
    UARTCC32XXDMA_Object *object = handle->object;
    size_t                size;

    /* Stop any ongoing DMA transmits */
    size = writeCancel(handle);

    if (size == 0) {
        return;
    }

    if (object->writeMode == UART_MODE_CALLBACK) {
        object->writeCallback(handle, (uint8_t*)object->writeBuf,
                object->writeCount);
    }
    else  if (object->writeMode == UART_MODE_BLOCKING) {
        /* We don't know if we're in an ISR, but we'll assume not. */
        SemaphoreP_post(object->writeSem);
    }

    DebugP_log2("UART:(%p) Write canceled, %d bytes written",
            ((UARTCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr,
            object->writeCount);
}

/*
 *  ======== UARTCC32XXDMA_configDMA ========
 *  Call with interrupts disabled.
 */
static void UARTCC32XXDMA_configDMA(UART_Handle handle, bool isWrite)
{
    UARTCC32XXDMA_Object           *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const  *hwAttrs = handle->hwAttrs;
    unsigned long                   channelControlOptions;

    if (isWrite) {
        channelControlOptions = UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                UDMA_DST_INC_NONE | UDMA_ARB_4;

        MAP_uDMAChannelControlSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
                channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
                UDMA_MODE_BASIC,
                (void *)object->writeBuf,
                (void *)(hwAttrs->baseAddr + UART_O_DR),
                object->writeSize);

        /*
         *  Enable the DMA channel
         *  This sets the channel's corresponding bit in the uDMA ENASET register.
         *  The bit will be cleared when the transfer completes.
         */
        MAP_uDMAChannelEnable(hwAttrs->txChannelIndex);

        MAP_UARTIntClear(hwAttrs->baseAddr, UART_INT_DMATX);
        MAP_UARTIntEnable(hwAttrs->baseAddr, UART_INT_DMATX);
    }
    else {
        channelControlOptions = UDMA_SIZE_8 | UDMA_SRC_INC_NONE |
                UDMA_DST_INC_8 | UDMA_ARB_4;

        MAP_uDMAChannelControlSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
                channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
                UDMA_MODE_BASIC,
                (void *)(hwAttrs->baseAddr + UART_O_DR),
                object->readBuf,
                object->readSize);

        /* Enable DMA Channel */
        MAP_uDMAChannelEnable(hwAttrs->rxChannelIndex);

        MAP_UARTIntClear(hwAttrs->baseAddr, UART_INT_DMARX);
        MAP_UARTIntEnable(hwAttrs->baseAddr, UART_INT_DMARX);
    }

    DebugP_log1("UART:(%p) DMA transfer enabled", hwAttrs->baseAddr);

    if (isWrite) {
        DebugP_log3("UART:(%p) DMA transmit, txBuf: %p; Count: %d",
                    hwAttrs->baseAddr, (uintptr_t)(object->writeBuf),
                    object->writeSize);
    }
    else {
        DebugP_log3("UART:(%p) DMA receive, rxBuf: %p; Count: %d",
                    hwAttrs->baseAddr, (uintptr_t)(object->readBuf),
                    object->readSize);
    }
}

/*
 *  ======== UARTCC32XXDMA_hwiIntFxn ========
 *  Hwi function that processes UART interrupts.
 *
 *  Three UART interrupts are enabled: Transmit FIFO is 4/8 empty,
 *  receive FIFO is 4/8 full and a receive timeout between the time
 *  the last character was received.
 *
 *  @param(arg)         The UART_Handle for this Hwi.
 */
static void UARTCC32XXDMA_hwiIntFxn(uintptr_t arg)
{
    uint32_t                       status;
    UARTCC32XXDMA_Object          *object = ((UART_Handle)arg)->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = ((UART_Handle)arg)->hwAttrs;

    /*
     *  Clear interrupts
     *    UARTIntStatus(base, false) - read the raw interrupt status
     *    UARTIntStatus(base, true)  - read masked interrupt status
     */
    status = MAP_UARTIntStatus(hwAttrs->baseAddr, false);
    if (status & UART_INT_DMATX) {
        MAP_UARTIntDisable(hwAttrs->baseAddr, UART_INT_DMATX);
        MAP_UARTIntClear(hwAttrs->baseAddr, UART_INT_DMATX);
    }

    if (status & UART_INT_DMARX) {
        MAP_UARTIntDisable(hwAttrs->baseAddr, UART_INT_DMARX);
        MAP_UARTIntClear(hwAttrs->baseAddr, UART_INT_DMARX);
    }

    DebugP_log2("UART:(%p) Interrupt with mask 0x%x",
                hwAttrs->baseAddr, status);

    /* Read data if characters are available. */
    if (object->readSize &&
                !MAP_uDMAChannelIsEnabled(hwAttrs->rxChannelIndex)) {
        object->readCount = object->readSize;
        object->readSize = 0;
        object->readCallback((UART_Handle)arg, object->readBuf,
                object->readCount);

        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

        DebugP_log2("UART:(%p) Read finished, %d bytes read",
                    hwAttrs->baseAddr, object->readCount);
    }

    /* Write completed. */
    if (object->writeSize &&
                !MAP_uDMAChannelIsEnabled(hwAttrs->txChannelIndex)) {
        object->writeCount = object->writeSize;
        object->writeSize = 0;
        /*
         *  No more to write, but data is not shifted out yet.
         *  Start TX FIFO Empty clock.
         *  + 4 because it is 4 bytes left in TX FIFO when the TX FIFO
         *  threshold interrupt occurs.
         */
        startTxFifoEmptyClk((UART_Handle)arg, 4);

        DebugP_log2("UART:(%p) Write finished, %d bytes written",
                    hwAttrs->baseAddr, object->writeCount);
    }
}

/*
 *  ======== getPowerMgrId ========
 */
static unsigned int getPowerMgrId(unsigned int baseAddr)
{
    switch (baseAddr) {
        case UARTA0_BASE:
            return (PowerCC32XX_PERIPH_UARTA0);
        case UARTA1_BASE:
            return (PowerCC32XX_PERIPH_UARTA1);
        default:
            return ((unsigned int)-1);
    }
}

/*
 *  ======== initHw ========
 *  Initialize the hardware.
 */
static void initHw(UART_Handle handle)
{
    ClockP_FreqHz                 freq;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /*
     *  Set the FIFO level to 4/8 empty and 4/8 full.
     *  The UART generates a burst request based on the FIFO trigger
     *  level. The arbitration size should be set to the amount
     *  of data that the FIFO can transfer when the trigger level is reached.
     *  Since arbitration size is a power of 2, we'll set the FIFO levels
     *  to 4_8 so they can match the arbitration size of 4.
     */
    MAP_UARTFIFOLevelSet(hwAttrs->baseAddr, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    if (isFlowControlEnabled(hwAttrs)) {
        /* Set flow control */
        MAP_UARTFlowControlSet(hwAttrs->baseAddr,
                UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);
    }
    else {
        MAP_UARTFlowControlSet(hwAttrs->baseAddr, UART_FLOWCONTROL_NONE);
    }

    ClockP_getCpuFreq(&freq);
    MAP_UARTConfigSetExpClk(hwAttrs->baseAddr,
                            freq.lo,
                            object->baudRate,
                            dataLength[object->dataLength] |
                            stopBits[object->stopBits] |
                            parityType[object->parityType]);

    MAP_UARTDMAEnable(hwAttrs->baseAddr, UART_DMA_TX | UART_DMA_RX);

    /* Configure DMA for TX and RX */
    MAP_uDMAChannelAssign(hwAttrs->txChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->txChannelIndex, UDMA_ATTR_ALTSELECT);

    MAP_uDMAChannelAssign(hwAttrs->rxChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->rxChannelIndex, UDMA_ATTR_ALTSELECT);
    MAP_UARTEnable(hwAttrs->baseAddr);
}

/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up from LPDS.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
        uintptr_t clientArg)
{
    initHw((UART_Handle)clientArg);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== readCancel ========
 *  Stop the current DMA receive transfer.
 */
static size_t readCancel(UART_Handle handle)
{
    uintptr_t                      key;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t                       remainder;
    int                            bytesTransferred;
    size_t                         size;

    /* Disable interrupts to avoid reading data while changing state. */
    key = HwiP_disable();

    size = object->readSize;

    /* Return if there is no read. */
    if (!object->readSize) {
        HwiP_restore(key);
        return (size);
    }

    /* Set channel bit in the ENACLR register */
    MAP_uDMAChannelDisable(hwAttrs->rxChannelIndex);

    remainder = MAP_uDMAChannelSizeGet(hwAttrs->rxChannelIndex);
    bytesTransferred = object->readSize - remainder;

    /*
     *  Since object->readSize != 0, the ISR has not run and released
     *  the Power constraint.  Release the constraint here.  Setting
     *  object->readSize to 0 will prevent the ISR from releasing the
     *  constraint in case it is pending.
     */
    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

    /* Set size = 0 to prevent reading and restore interrupts. */
    object->readSize = 0;
    object->readCount = bytesTransferred;

    HwiP_restore(key);

    return (size);
}

/*
 *  ======== readSemCallback ========
 *  Simple callback to post a semaphore for the blocking mode.
 */
static void readSemCallback(UART_Handle handle, void *buffer, size_t count)
{
    UARTCC32XXDMA_Object *object = handle->object;

    SemaphoreP_post(object->readSem);
}

/*
 *  ======== writeSemCallback ========
 *  Simple callback to post a semaphore for the blocking mode.
 */
static void writeSemCallback(UART_Handle handle, void *buffer, size_t count)
{
    UARTCC32XXDMA_Object *object = handle->object;

    SemaphoreP_post(object->writeSem);
}

/*
 *  ======== writeCancel ========
 */
static size_t writeCancel(UART_Handle handle)
{
    uintptr_t                      key;
    UARTCC32XXDMA_Object          *object = handle->object;
    UARTCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t                       remainder;
    int                            bytesTransferred;
    size_t                         size;

    /* Disable interrupts to avoid writing data while changing state. */
    key = HwiP_disable();

    size = object->writeSize;

    /* Set channel bit in the ENACLR register */
    MAP_uDMAChannelDisable(hwAttrs->txChannelIndex);

    remainder = MAP_uDMAChannelSizeGet(hwAttrs->txChannelIndex);
    bytesTransferred = object->writeSize - remainder;

    /* Return if there is no write. */
    if (!object->writeSize) {
        HwiP_restore(key);

        return (size);
    }

    /*
     *  If the transfer didn't complete, the ISR will not run to
     *  release the Power constraint, so do it here.
     */
    if (bytesTransferred < object->writeSize) {
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    }

    /* Set size = 0 to prevent writing and restore interrupts. */
    object->writeSize = 0;
    object->writeCount = bytesTransferred;

    HwiP_restore(key);

    return (size);
}

/*
 *  ======== startTxFifoEmptyClk ========
 *  Last write to TX FIFO is done, but not shifted out yet. Start a clock
 *  which will trigger when the TX FIFO should be empty.
 *
 *  @param(handle)         The UART_Handle for ongoing write.
 *  @param(numData)        The number of data present in FIFO after last write
 */
static void startTxFifoEmptyClk(UART_Handle handle, unsigned int numData)
{
    UARTCC32XXDMA_Object *object = handle->object;
    unsigned int writeTimeout;
    unsigned int ticksPerSec;

    /* No more to write, but data is not shiftet out properly yet.
     *   1. Compute appropriate wait time for FIFO to empty out
     *       - 8 - for maximum data length
     *       - 3 - for one start bit and maximum of two stop bits
     */
    ticksPerSec = 1000000 / ClockP_getSystemTickPeriod();
    writeTimeout = ((numData * (8 + 3) * ticksPerSec) + object->baudRate - 1)
                           / object->baudRate;

    /*   2. Configure clock object to trigger when FIFO is empty */
    ClockP_setTimeout(object->txFifoEmptyClk, writeTimeout);
    ClockP_start(object->txFifoEmptyClk);
}

/*
 *  ======== writeFinishedDoCallback ========
 *  Write finished - make callback
 *
 *  This function is called when the txFifoEmptyClk times out. The TX FIFO
 *  should now be empty.  Standby is allowed again.
 *
 *  @param(handle)         The UART_Handle for ongoing write.
 */
static void writeFinishedDoCallback(UART_Handle handle)
{
    UARTCC32XXDMA_Object           *object;
    UARTCC32XXDMA_HWAttrsV1 const  *hwAttrs;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* Stop the txFifoEmpty clock */
    ClockP_stop((ClockP_Handle)object->txFifoEmptyClk);

    /*   1. Function verifies that the FIFO is empty via BUSY flag */
    /*   2. Polls this flag if not yet ready (should not be necessary) */
    while (MAP_UARTBusy(hwAttrs->baseAddr));

    /* Release constraint since transaction is done */
    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

    /* Make callback */
    object->writeCallback(handle, (uint8_t *)object->writeBuf,
                          object->writeCount);
    DebugP_log2("UART:(%p) Write finished, %d bytes written",
                hwAttrs->baseAddr, object->writeCount);
}
