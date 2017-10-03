/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/devices/cc32xx/inc/hw_mcspi.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/spi.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

#include <ti/drivers/dma/UDMACC32XX.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/spi/SPICC32XXDMA.h>

#define MAX_DMA_TRANSFER_AMOUNT (1024)

#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

void SPICC32XXDMA_close(SPI_Handle handle);
int_fast16_t SPICC32XXDMA_control(SPI_Handle handle, uint_fast16_t cmd,
    void *arg);
void SPICC32XXDMA_init(SPI_Handle handle);
SPI_Handle SPICC32XXDMA_open(SPI_Handle handle, SPI_Params *params);
bool SPICC32XXDMA_transfer(SPI_Handle handle, SPI_Transaction *transaction);
void SPICC32XXDMA_transferCancel(SPI_Handle handle);

/* SPI function table for SPICC32XXDMA implementation */
const SPI_FxnTable SPICC32XXDMA_fxnTable = {
    SPICC32XXDMA_close,
    SPICC32XXDMA_control,
    SPICC32XXDMA_init,
    SPICC32XXDMA_open,
    SPICC32XXDMA_transfer,
    SPICC32XXDMA_transferCancel
};

static const uint32_t mode[] = {
    SPI_MODE_MASTER,
    SPI_MODE_SLAVE
};

/*
 * This lookup table is used to configure the DMA channels for the appropriate
 * (8bit, 16bit or 32bit) transfer sizes.
 * Table for an SPI DMA RX channel.
 */
static const uint32_t dmaRxConfig[] = {
    UDMA_SIZE_8  | UDMA_SRC_INC_NONE | UDMA_DST_INC_8  | UDMA_ARB_1,
    UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1,
    UDMA_SIZE_32 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1
};

/*
 * This lookup table is used to configure the DMA channels for the appropriate
 * (8bit, 16bit or 32bit) transfer sizes.
 * Table for an SPI DMA TX channel
 */
static const uint32_t dmaTxConfig[] = {
    UDMA_SIZE_8  | UDMA_SRC_INC_8  | UDMA_DST_INC_NONE | UDMA_ARB_1,
    UDMA_SIZE_16 | UDMA_SRC_INC_16 | UDMA_DST_INC_NONE | UDMA_ARB_1,
    UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_NONE | UDMA_ARB_1
};

/*
 * This lookup table is used to configure the DMA channels for the appropriate
 * (8bit, 16bit or 32bit) transfer sizes when either txBuf or rxBuf are NULL.
 */
static const uint32_t dmaNullConfig[] = {
    UDMA_SIZE_8  | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE | UDMA_ARB_1,
    UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE | UDMA_ARB_1,
    UDMA_SIZE_32 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE | UDMA_ARB_1
};

/*
 *  ======== blockingTransferCallback ========
 */
static void blockingTransferCallback(SPI_Handle handle,
    SPI_Transaction *transaction)
{
    SPICC32XXDMA_Object *object = handle->object;

    SemaphoreP_post(object->transferComplete);
}

/*
 *  ======== configDMA ========
 *  This functions configures the transmit and receive DMA channels for a given
 *  SPI_Handle and SPI_Transaction
 */
static void configDMA(SPICC32XXDMA_Object *object,
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs, SPI_Transaction *transaction)
{
    uintptr_t  key;
    void      *buf;
    uint32_t   channelControlOptions;
    uint8_t    dataFrameSizeInBytes;
    uint8_t    optionsIndex;

    /* DMA options used vary according to data size. */
    if (object->dataSize < 9) {
        optionsIndex = 0;
        dataFrameSizeInBytes = sizeof(uint8_t);
    }
    else if (object->dataSize < 17) {
        optionsIndex = 1;
        dataFrameSizeInBytes = sizeof(uint16_t);;
    }
    else {
        optionsIndex = 2;
        dataFrameSizeInBytes = sizeof(uint32_t);
    }

    /*
     * The DMA has a max transfer amount of 1024.  If the transaction is
     * greater; we must transfer it in chunks.  object->amtDataXferred has
     * how much data has already been sent.
     */
    if ((transaction->count - object->amtDataXferred) > MAX_DMA_TRANSFER_AMOUNT) {
        object->currentXferAmt = MAX_DMA_TRANSFER_AMOUNT;
    }
    else {
        object->currentXferAmt = (transaction->count - object->amtDataXferred);
    }

    if (transaction->txBuf) {
        channelControlOptions = dmaTxConfig[optionsIndex];
        /*
         * Add an offset for the amount of data transfered.  The offset is
         * calculated by: object->amtDataXferred * (dataFrameSizeInBytes).
         * This accounts for 8, 16 or 32-bit sized transfers.
         */
        buf = (void *) ((uint32_t) transaction->txBuf +
            ((uint32_t) object->amtDataXferred * dataFrameSizeInBytes));
    }
    else {
        channelControlOptions = dmaNullConfig[optionsIndex];
        *hwAttrs->scratchBufPtr = hwAttrs->defaultTxBufValue;
        buf = hwAttrs->scratchBufPtr;
    }

    /* Setup the TX transfer characteristics & buffers */
    MAP_uDMAChannelControlSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
        channelControlOptions);
    MAP_uDMAChannelAttributeDisable(hwAttrs->txChannelIndex,
        UDMA_ATTR_ALTSELECT);
    MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
        UDMA_MODE_BASIC, buf, (void *) (hwAttrs->baseAddr + MCSPI_O_TX0),
        object->currentXferAmt);

    if (transaction->rxBuf) {
        channelControlOptions = dmaRxConfig[optionsIndex];
        /*
         * Add an offset for the amount of data transfered.  The offset is
         * calculated by: object->amtDataXferred * (dataFrameSizeInBytes).
         * This accounts for 8 or 16-bit sized transfers.
         */
        buf = (void *) ((uint32_t) transaction->rxBuf +
            ((uint32_t) object->amtDataXferred * dataFrameSizeInBytes));
    }
    else {
        channelControlOptions = dmaNullConfig[optionsIndex];
        buf = hwAttrs->scratchBufPtr;
    }

    /* Setup the RX transfer characteristics & buffers */
    MAP_uDMAChannelControlSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
        channelControlOptions);
    MAP_uDMAChannelAttributeDisable(hwAttrs->rxChannelIndex,
        UDMA_ATTR_ALTSELECT);
    MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
        UDMA_MODE_BASIC, (void *) (hwAttrs->baseAddr + MCSPI_O_RX0), buf,
        object->currentXferAmt);

    /* A lock is needed because we are accessing shared uDMA memory */
    key = HwiP_disable();

    MAP_uDMAChannelAssign(hwAttrs->rxChannelIndex);
    MAP_uDMAChannelAssign(hwAttrs->txChannelIndex);

    /* Enable DMA to generate interrupt on SPI peripheral */
    MAP_SPIDmaEnable(hwAttrs->baseAddr, SPI_RX_DMA | SPI_TX_DMA);
    MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMARX);
    MAP_SPIIntEnable(hwAttrs->baseAddr, SPI_INT_DMARX);
    MAP_SPIWordCountSet(hwAttrs->baseAddr, object->currentXferAmt);

    /* Enable channels & start DMA transfers */
    MAP_uDMAChannelEnable(hwAttrs->txChannelIndex);
    MAP_uDMAChannelEnable(hwAttrs->rxChannelIndex);

    HwiP_restore(key);

    MAP_SPIEnable(hwAttrs->baseAddr);
    MAP_SPICSEnable(hwAttrs->baseAddr);
}

/*
 *  ======== getDmaRemainingXfers ========
 */
static inline uint32_t getDmaRemainingXfers(SPICC32XXDMA_HWAttrsV1 const *hwAttrs) {
    uint32_t          controlWord;
    tDMAControlTable *controlTable;

    controlTable = MAP_uDMAControlBaseGet();
    controlWord = controlTable[(hwAttrs->rxChannelIndex & 0x3f)].ulControl;

    return (((controlWord & UDMA_CHCTL_XFERSIZE_M) >> 4) + 1);
}

/*
 *  ======== getPowerMgrId ========
 */
static uint16_t getPowerMgrId(uint32_t baseAddr)
{
    switch (baseAddr) {
        case GSPI_BASE:
            return (PowerCC32XX_PERIPH_GSPI);
        case LSPI_BASE:
            return (PowerCC32XX_PERIPH_LSPI);
        default:
            return (~0);
    }
}

/*
 *  ======== initHw ========
 */
static void initHw(SPICC32XXDMA_Object *object,
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs)
{
    /*
     * SPI peripheral should remain disabled until a transfer is requested.
     * This is done to prevent the RX FIFO from gathering data from other
     * transfers.
     */
    MAP_SPICSDisable(hwAttrs->baseAddr);
    MAP_SPIDisable(hwAttrs->baseAddr);
    MAP_SPIReset(hwAttrs->baseAddr);

    MAP_SPIConfigSetExpClk(hwAttrs->baseAddr,
        MAP_PRCMPeripheralClockGet(hwAttrs->spiPRCM), object->bitRate,
        mode[object->spiMode], object->frameFormat,
        (hwAttrs->csControl | hwAttrs->pinMode | hwAttrs->turboMode |
        hwAttrs->csPolarity | ((object->dataSize - 1) << 7)));

    MAP_SPIFIFOEnable(hwAttrs->baseAddr, SPI_RX_FIFO | SPI_TX_FIFO);
    MAP_SPIFIFOLevelSet(hwAttrs->baseAddr, object->txFifoTrigger,
        object->rxFifoTrigger);
}

/*
 *  ======== postNotifyFxn ========
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    SPICC32XXDMA_Object          *object = ((SPI_Handle) clientArg)->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = ((SPI_Handle) clientArg)->hwAttrs;

    initHw(object, hwAttrs);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== spiHwiFxn ========
 */
static void spiHwiFxn(uintptr_t arg)
{
    SPI_Transaction              *msg;
    SPICC32XXDMA_Object          *object = ((SPI_Handle)arg)->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = ((SPI_Handle)arg)->hwAttrs;

    /* RX DMA channel has completed */
    MAP_SPIDmaDisable(hwAttrs->baseAddr, SPI_RX_DMA | SPI_TX_DMA);
    MAP_SPIIntDisable(hwAttrs->baseAddr, SPI_INT_DMARX);
    MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMARX);

    if (object->transaction->count - object->amtDataXferred >
        MAX_DMA_TRANSFER_AMOUNT) {
        /* Data still remaining, configure another DMA transfer */
        object->amtDataXferred += object->currentXferAmt;

        configDMA(object, hwAttrs, object->transaction);
    }
    else {
        /*
         * All data sent; disable peripheral, set status, perform
         * callback & return
         */
        MAP_SPICSDisable(hwAttrs->baseAddr);
        MAP_SPIDisable(hwAttrs->baseAddr);
        object->transaction->status = SPI_TRANSFER_COMPLETED;

        /* Release constraint since transaction is done */
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

        /*
         * Use a temporary transaction pointer in case the callback function
         * attempts to perform another SPI_transfer call
         */
        msg = object->transaction;

        /* Indicate we are done with this transfer */
        object->transaction = NULL;

        object->transferCallbackFxn((SPI_Handle) arg, msg);
    }
}

/*
 *  ======== spiPollingTransfer ========
 */
static inline void spiPollingTransfer(SPICC32XXDMA_Object *object,
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs, SPI_Transaction *transaction)
{
    uint8_t   increment;
    uint32_t  dummyBuffer;
    size_t    transferCount;
    void     *rxBuf;
    void     *txBuf;

    if (transaction->rxBuf) {
        rxBuf = transaction->rxBuf;
    }
    else {
        rxBuf = hwAttrs->scratchBufPtr;
    }

    if (transaction->txBuf) {
        txBuf = transaction->txBuf;
    }
    else {
        *hwAttrs->scratchBufPtr = hwAttrs->defaultTxBufValue;
        txBuf = hwAttrs->scratchBufPtr;
    }

    if (object->dataSize < 9) {
        increment = sizeof(uint8_t);
    }
    else if (object->dataSize < 17) {
        increment = sizeof(uint16_t);
    }
    else {
        increment = sizeof(uint32_t);
    }

    transferCount = transaction->count;

    /*
     * Start the polling transfer - we MUST set word count to 0; not doing so
     * will raise spurious RX interrupts flags (though interrupts are not
     * enabled).
     */
    MAP_SPIWordCountSet(hwAttrs->baseAddr, 0);
    MAP_SPIEnable(hwAttrs->baseAddr);
    MAP_SPICSEnable(hwAttrs->baseAddr);

    while (transferCount--) {
        if (object->dataSize < 9) {
            MAP_SPIDataPut(hwAttrs->baseAddr, *((uint8_t *) txBuf));
            MAP_SPIDataGet(hwAttrs->baseAddr, (unsigned long *)&dummyBuffer);
            *((uint8_t *) rxBuf) = (uint8_t) dummyBuffer;
        }
        else if (object->dataSize < 17) {
            MAP_SPIDataPut(hwAttrs->baseAddr, *((uint16_t *) txBuf));
            MAP_SPIDataGet(hwAttrs->baseAddr, (unsigned long *) &dummyBuffer);
            *((uint16_t *) rxBuf) = (uint16_t) dummyBuffer;
        }
        else {
            MAP_SPIDataPut(hwAttrs->baseAddr, *((uint32_t *) txBuf));
            MAP_SPIDataGet(hwAttrs->baseAddr, (unsigned long * ) rxBuf);
        }

        /* Only increment source & destination if buffers were provided */
        if (transaction->rxBuf) {
            rxBuf = (void *) (((uint32_t) rxBuf) + increment);
        }
        if (transaction->txBuf) {
            txBuf = (void *) (((uint32_t) txBuf) + increment);
        }
    }

    MAP_SPICSDisable(hwAttrs->baseAddr);
    MAP_SPIDisable(hwAttrs->baseAddr);
}

/*
 *  ======== SPICC32XXDMA_close ========
 */
void SPICC32XXDMA_close(SPI_Handle handle)
{
    uint32_t                      padRegister;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    MAP_SPICSDisable(hwAttrs->baseAddr);
    MAP_SPIDisable(hwAttrs->baseAddr);
    MAP_SPIFIFODisable(hwAttrs->baseAddr, SPI_RX_FIFO | SPI_TX_FIFO);

    /* Release power dependency on SPI. */
    Power_releaseDependency(getPowerMgrId(hwAttrs->baseAddr));
    Power_unregisterNotify(&(object->notifyObj));

    if (object->hwiHandle) {
        HwiP_delete(object->hwiHandle);
    }
    if (object->transferComplete) {
        SemaphoreP_delete(object->transferComplete);
    }

    if (object->dmaHandle) {
        UDMACC32XX_close(object->dmaHandle);
    }

    /* Restore pin pads to their reset states */
    if (hwAttrs->mosiPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        padRegister = (PinToPadGet((hwAttrs->mosiPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
    }
    if (hwAttrs->misoPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        padRegister = (PinToPadGet((hwAttrs->misoPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
    }
    if (hwAttrs->clkPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        padRegister = (PinToPadGet((hwAttrs->clkPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
    }
    if ((hwAttrs->pinMode == SPI_4PIN_MODE) &&
        (hwAttrs->csPin != SPICC32XXDMA_PIN_NO_CONFIG)) {
        padRegister = (PinToPadGet((hwAttrs->csPin) & 0xff)<<2)
            + PAD_CONFIG_BASE;
        HWREG(padRegister) = PAD_RESET_STATE;
    }

    object->isOpen = false;
}

/*
 *  ======== SPICC32XXDMA_control ========
 */
int_fast16_t SPICC32XXDMA_control(SPI_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (SPI_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== SPICC32XXDMA_init ========
 */
void SPICC32XXDMA_init(SPI_Handle handle)
{
    UDMACC32XX_init();
}

/*
 *  ======== SPICC32XXDMA_open ========
 */
SPI_Handle SPICC32XXDMA_open(SPI_Handle handle, SPI_Params *params)
{
    uintptr_t                     key;
    uint16_t                      pin;
    uint16_t                      mode;
    uint8_t                      powerMgrId;
    HwiP_Params                   hwiParams;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    key = HwiP_disable();

    if (object->isOpen) {
        HwiP_restore(key);

        return (NULL);
    }
    object->isOpen = true;

    HwiP_restore(key);

    /* SPI_TI & SPI_MW are not supported */
    if (params->frameFormat == SPI_TI || params->frameFormat == SPI_MW) {
        object->isOpen = false;

        return (NULL);
    }

    /* Register power dependency - i.e. power up and enable clock for SPI. */
    powerMgrId = getPowerMgrId(hwAttrs->baseAddr);
    if (powerMgrId > PowerCC32XX_NUMRESOURCES) {
        object->isOpen = false;

        return (NULL);
    }
    Power_setDependency(powerMgrId);
    Power_registerNotify(&(object->notifyObj), PowerCC32XX_AWAKE_LPDS,
        postNotifyFxn, (uintptr_t) handle);

    /* Configure the pins */
    if (hwAttrs->mosiPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->mosiPin) & 0xff;
        mode = (hwAttrs->mosiPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long) pin, (unsigned long) mode);
    }

    if (hwAttrs->misoPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->misoPin) & 0xff;
        mode = (hwAttrs->misoPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long) pin, (unsigned long) mode);
    }

    if (hwAttrs->clkPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->clkPin) & 0xff;
        mode = (hwAttrs->clkPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long) pin, (unsigned long) mode);
    }

    if (hwAttrs->pinMode == SPI_4PIN_MODE) {
        if (hwAttrs->csPin != SPICC32XXDMA_PIN_NO_CONFIG) {
            pin = (hwAttrs->csPin) & 0xff;
            mode = (hwAttrs->csPin >> 8) & 0xff;
            MAP_PinTypeSPI((unsigned long) pin, (unsigned long) mode);
        }
    }

    object->dmaHandle = UDMACC32XX_open();
    if (object->dmaHandle == NULL) {
        SPICC32XXDMA_close(handle);

        return (NULL);
    }

    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t) handle;
    hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(hwAttrs->intNum, spiHwiFxn, &hwiParams);
    if (object->hwiHandle == NULL) {
        SPICC32XXDMA_close(handle);

        return (NULL);
    }

    if (params->transferMode == SPI_MODE_BLOCKING) {
        /*
         * Create a semaphore to block task execution for the duration of the
         * SPI transfer
         */
        object->transferComplete = SemaphoreP_createBinary(0);
        if (object->transferComplete == NULL) {
            SPICC32XXDMA_close(handle);

            return (NULL);
        }

        object->transferCallbackFxn = blockingTransferCallback;
    }
    else {
        if (params->transferCallbackFxn == NULL) {
            SPICC32XXDMA_close(handle);

            return (NULL);
        }

        object->transferCallbackFxn = params->transferCallbackFxn;
    }

    object->bitRate = params->bitRate;
    object->dataSize = params->dataSize;
    object->frameFormat = params->frameFormat;
    object->spiMode = params->mode;
    object->transaction = NULL;
    object->transferMode = params->transferMode;
    object->transferTimeout = params->transferTimeout;

    /* SPI FIFO trigger sizes vary based on data frame size */
    if (object->dataSize < 9) {
        object->rxFifoTrigger = sizeof(uint8_t);
        object->txFifoTrigger = sizeof(uint8_t);
    }
    else if (object->dataSize < 17) {
        object->rxFifoTrigger = sizeof(uint16_t);
        object->txFifoTrigger = sizeof(uint16_t);
    }
    else {
        object->rxFifoTrigger = sizeof(uint32_t);
        object->txFifoTrigger = sizeof(uint32_t);
    }

    initHw(object, hwAttrs);

    return (handle);
}

/*
 *  ======== SPICC32XXDMA_transfer ========
 */
bool SPICC32XXDMA_transfer(SPI_Handle handle, SPI_Transaction *transaction)
{
    uintptr_t                     key;
    uint8_t                       alignMask;
    bool                          buffersAligned;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    if ((transaction->count == 0) ||
        (transaction->rxBuf == NULL && transaction->txBuf == NULL)) {
        return (false);
    }

    key = HwiP_disable();

    /*
     * alignMask is used to determine if the RX/TX buffers addresses are
     * aligned to the data frame size.
     */
    alignMask = (object->rxFifoTrigger - 1);
    buffersAligned = ((((uint32_t) transaction->rxBuf & alignMask) == 0) &&
        (((uint32_t) transaction->txBuf & alignMask) == 0));

    if (object->transaction) {
        HwiP_restore(key);

        return (false);
    }
    else {
        object->transaction = transaction;
        object->transaction->status = SPI_TRANSFER_STARTED;
        object->amtDataXferred = 0;
        object->currentXferAmt = 0;
    }

    HwiP_restore(key);

    /* Set constraints to guarantee transaction */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    /* Polling transfer if BLOCKING mode & transaction->count < threshold */
    if ((object->transferMode == SPI_MODE_BLOCKING &&
        transaction->count < hwAttrs->minDmaTransferSize) || !buffersAligned) {
        spiPollingTransfer(object, hwAttrs, transaction);

        /* Transaction completed; set status & mark SPI ready */
        object->transaction->status = SPI_TRANSFER_COMPLETED;
        object->transaction = NULL;

        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    }
    else {
        /* Perform a DMA backed SPI transfer */
        configDMA(object, hwAttrs, transaction);

        if (object->transferMode == SPI_MODE_BLOCKING) {
            if (SemaphoreP_pend(object->transferComplete,
                object->transferTimeout) != SemaphoreP_OK) {
                /* Timeout occurred; cancel the transfer */
                object->transaction->status = SPI_TRANSFER_FAILED;
                SPICC32XXDMA_transferCancel(handle);

                /*
                 * TransferCancel() performs callback which posts
                 * transferComplete semaphore. This call consumes this extra post.
                 */
                SemaphoreP_pend(object->transferComplete, SemaphoreP_NO_WAIT);

                return (false);
            }
        }
    }

    return (true);
}

/*
 *  ======== SPICC32XXDMA_transferCancel ========
 */
void SPICC32XXDMA_transferCancel(SPI_Handle handle)
{
    uintptr_t                     key;
    SPI_Transaction              *msg;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /*
     * There are 2 use cases in which to call transferCancel():
     *   1.  The driver is in CALLBACK mode.
     *   2.  The driver is in BLOCKING mode & there has been a transfer timeout.
     */
    if (object->transferMode == SPI_MODE_CALLBACK ||
        object->transaction->status == SPI_TRANSFER_FAILED) {

        key = HwiP_disable();

        if (object->transaction == NULL || object->cancelInProgress) {
            HwiP_restore(key);

            return;
        }
        object->cancelInProgress = true;

        /* Prevent interrupt from occurring while canceling the transfer */
        HwiP_disableInterrupt(hwAttrs->intNum);
        HwiP_clearInterrupt(hwAttrs->intNum);

        /* Clear DMA configuration */
        MAP_uDMAChannelDisable(hwAttrs->rxChannelIndex);
        MAP_uDMAChannelDisable(hwAttrs->txChannelIndex);

        MAP_SPIIntDisable(hwAttrs->baseAddr, SPI_INT_DMARX);
        MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMARX);
        MAP_SPIDmaDisable(hwAttrs->baseAddr, SPI_RX_DMA | SPI_TX_DMA);

        HwiP_restore(key);

        /*
         * Disables peripheral, clears all registers & reinitializes it to
         * parameters used in SPI_open()
         */
        initHw(object, hwAttrs);

        HwiP_enableInterrupt(hwAttrs->intNum);

        /*
         * Calculate amount of data which has already been sent & store
         * it in transaction->count
         */
        object->transaction->count = object->amtDataXferred +
            (object->currentXferAmt - getDmaRemainingXfers(hwAttrs));

        /* Set status CANCELED if we did not cancel due to timeout  */
        if (object->transaction->status == SPI_TRANSFER_STARTED) {
            object->transaction->status = SPI_TRANSFER_CANCELED;
        }

        /* Release constraint set during transaction */
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

        /*
         * Use a temporary transaction pointer in case the callback function
         * attempts to perform another SPI_transfer call
         */
        msg = object->transaction;

        /* Indicate we are done with this transfer */
        object->transaction = NULL;
        object->cancelInProgress = false;
        object->transferCallbackFxn(handle, msg);
    }
}
