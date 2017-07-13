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

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/spi/SPICC32XXDMA.h>
#include <ti/drivers/dma/UDMACC32XX.h>

#include <ti/devices/cc32xx/inc/hw_mcspi.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/spi.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

void SPICC32XXDMA_close(SPI_Handle handle);
int_fast16_t SPICC32XXDMA_control(SPI_Handle handle, uint_fast16_t cmd, void *arg);
void SPICC32XXDMA_init(SPI_Handle handle);
SPI_Handle SPICC32XXDMA_open(SPI_Handle handle, SPI_Params *params);
bool SPICC32XXDMA_transfer(SPI_Handle handle, SPI_Transaction *transaction);
void SPICC32XXDMA_transferCancel(SPI_Handle handle);

static void configDMA(SPI_Handle handle, SPI_Transaction *transaction);
static unsigned int getPowerMgrId(SPIBaseAddrType baseAddr);
static void initHw(SPI_Handle handle);
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg);
static void spiHwiFxn(uintptr_t arg);
static inline void spiPollingTransfer(SPICC32XXDMA_Object *object,
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs, SPI_Transaction *transaction);
static void spiPollingTransfer8(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count);
static void spiPollingTransfer16(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count);
static void spiPollingTransfer32(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count);
static void transferCallback(SPI_Handle handle, SPI_Transaction *transaction);

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
    SPI_MODE_MASTER,    /* SPI_MASTER */
    SPI_MODE_SLAVE      /* SPI_SLAVE */
};

static const uint32_t frameFormat[] = {
    SPI_SUB_MODE_0,    /* SPI_POLO_PHA0 */
    SPI_SUB_MODE_1,    /* SPI_POLO_PHA1 */
    SPI_SUB_MODE_2,    /* SPI_POL1_PHA0 */
    SPI_SUB_MODE_3,    /* SPI_POL1_PHA1 */
    (0),               /* SPI_TI - not supported on CC32XX */
    (0)                /* SPI_MW - not supported on CC32XX */
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
 *  ======== configDMA ========
 *  This functions configures the transmit and receive DMA channels for a given
 *  SPI_Handle and SPI_Transaction
 *
 *  @pre    Function assumes that the handle and transaction is not NULL
 */
static void configDMA(SPI_Handle handle, SPI_Transaction *transaction)
{
    uintptr_t                     key;
    void                         *buf;
    uint32_t                      channelControlOptions;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /* Configure DMA for RX */
    MAP_uDMAChannelAssign(hwAttrs->rxChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->rxChannelIndex,
        UDMA_ATTR_ALTSELECT);

    if (transaction->rxBuf) {
        channelControlOptions = dmaRxConfig[object->frameSize];
        buf = transaction->rxBuf;
    }
    else {
        channelControlOptions = dmaNullConfig[object->frameSize];
        buf = hwAttrs->scratchBufPtr;
    }
    MAP_uDMAChannelControlSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
        channelControlOptions);
    MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex | UDMA_PRI_SELECT,
        UDMA_MODE_BASIC, (void *) (hwAttrs->baseAddr + MCSPI_O_RX0), buf,
        transaction->count);

    /* Configure DMA for TX */
    MAP_uDMAChannelAssign(hwAttrs->txChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->txChannelIndex,
        UDMA_ATTR_ALTSELECT);

    if (transaction->txBuf) {
        channelControlOptions = dmaTxConfig[object->frameSize];
        buf = transaction->txBuf;
    }
    else {
        channelControlOptions = dmaNullConfig[object->frameSize];
        *hwAttrs->scratchBufPtr = hwAttrs->defaultTxBufValue;
        buf = hwAttrs->scratchBufPtr;
    }
    MAP_uDMAChannelControlSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
        channelControlOptions);
    MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex | UDMA_PRI_SELECT,
        UDMA_MODE_BASIC, buf, (void *) (hwAttrs->baseAddr + MCSPI_O_TX0),
        transaction->count);

    DebugP_log1("SPI:(%p) DMA transfer enabled", hwAttrs->baseAddr);
    DebugP_log4("SPI: DMA transaction: %p, rxBuf: %p; txBuf: %p; Count: %d",
        (uintptr_t) transaction, (uintptr_t) transaction->rxBuf,
        (uintptr_t) transaction->txBuf, (uintptr_t) transaction->count);

    key = HwiP_disable();
    MAP_uDMAChannelEnable(hwAttrs->rxChannelIndex);
    MAP_uDMAChannelEnable(hwAttrs->txChannelIndex);
    HwiP_restore(key);

    MAP_SPIWordCountSet(hwAttrs->baseAddr, transaction->count);
}

/*
 *  ======== getPowerMgrId ========
 */
static unsigned int getPowerMgrId(SPIBaseAddrType baseAddr)
{
    switch (baseAddr) {
        case GSPI_BASE:
            return (PowerCC32XX_PERIPH_GSPI);
        case LSPI_BASE:
            return (PowerCC32XX_PERIPH_LSPI);
        default:
            return ((unsigned int) -1);
    }
}

/*
 *  ======== initHw ========
 */
static void initHw(SPI_Handle handle)
{
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /* Configure the SPI peripheral */
    MAP_SPICSDisable(hwAttrs->baseAddr);
    MAP_SPIDisable(hwAttrs->baseAddr);
    MAP_SPIReset(hwAttrs->baseAddr);

    /*
     * To support 4-32 bit lengths, object->dataSize must be formatted to meet
     * hardware requirement.
     */
    MAP_SPIConfigSetExpClk(hwAttrs->baseAddr,
        MAP_PRCMPeripheralClockGet(hwAttrs->spiPRCM), object->bitRate,
        mode[object->spiMode], frameFormat[object->frameFormat],
        (hwAttrs->csControl | hwAttrs->pinMode | hwAttrs->turboMode |
        hwAttrs->csPolarity | ((object->dataSize - 1) << 7)));

    /* Enable FIFOs, DMA, & interrupts  */
    MAP_SPIFIFOEnable(hwAttrs->baseAddr, SPI_RX_FIFO | SPI_TX_FIFO);
    MAP_SPIDmaEnable(hwAttrs->baseAddr, SPI_RX_DMA | SPI_TX_DMA);
    MAP_SPIFIFOLevelSet(hwAttrs->baseAddr, object->txFifoTrigger,
        object->rxFifoTrigger);
}

/*
 *  ======== postNotifyFxn ========
 *  This functions is called to notify the SPI driver of an ongoing transition
 *  out of LPDS mode.
 *  clientArg should be pointing to a hardware module which has already
 *  been opened.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    initHw((SPI_Handle) clientArg);

    return (Power_NOTIFYDONE);
}

/*
*  ======== spiHwiFxn ========
*  ISR for the SPI when we use the DMA is used.
*/
static void spiHwiFxn(uintptr_t arg)
{
    SPI_Transaction              *msg;
    SPICC32XXDMA_Object          *object = ((SPI_Handle)arg)->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = ((SPI_Handle)arg)->hwAttrs;
    uint32_t                      intCode = 0;

    DebugP_log1("SPI:(%p) interrupt context start", hwAttrs->baseAddr);

    intCode = MAP_SPIIntStatus(hwAttrs->baseAddr, false);
    if (intCode & SPI_INT_DMATX) {
        /* DMA finished transferring; clear & disable interrupt */
        MAP_SPIIntDisable(hwAttrs->baseAddr, SPI_INT_DMATX);
        MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMATX);
    }

    /* Determine if the TX & RX DMA channels have completed */
    if ((object->transaction) &&
        (MAP_uDMAChannelIsEnabled(hwAttrs->rxChannelIndex) == false) &&
        (MAP_uDMAIntStatus() & (1 << hwAttrs->rxChannelIndex))) {

        MAP_SPIDisable(hwAttrs->baseAddr);
        MAP_SPICSDisable(hwAttrs->baseAddr);

        MAP_SPIIntDisable(hwAttrs->baseAddr, SPI_INT_DMARX);
        MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMARX);
        MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_EOW);

        /*
         * Clear any pending interrupt
         * As the TX DMA channel interrupt gets service, it may be possible
         * that the RX DMA channel finished in the meantime, which means
         * the IRQ for RX DMA channel is still pending...
         */
        HwiP_clearInterrupt(hwAttrs->intNum);

        /*
         * Use a temporary transaction pointer in case the callback function
         * attempts to perform another SPI_transfer call
         */
        msg = object->transaction;

        /* Indicate we are done with this transfer */
        object->transaction = NULL;

        /* Release constraint since transaction is done */
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

        DebugP_log2("SPI:(%p) DMA transaction: %p complete", hwAttrs->baseAddr,
            (uintptr_t) msg);

        object->transferCallbackFxn((SPI_Handle) arg, msg);
    }

    DebugP_log1("SPI:(%p) interrupt context end", hwAttrs->baseAddr);
}

/*
 *  ======== spiPollingTransfer ========
 */
static inline void spiPollingTransfer(SPICC32XXDMA_Object *object,
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs, SPI_Transaction *transaction)
{
    void       *rxBuf;
    void       *txBuf;
    uint8_t     rxIncrement = 0;
    uint8_t     txIncrement = 0;

    if (transaction->rxBuf) {
        rxBuf = transaction->rxBuf;
        rxIncrement = (object->frameSize) ? (object->frameSize << 1) : 1;
    }
    else {
        rxBuf = hwAttrs->scratchBufPtr;
    }

    if (transaction->txBuf) {
        txBuf = transaction->txBuf;
        txIncrement = (object->frameSize) ? (object->frameSize << 1) : 1;
    }
    else {
        *hwAttrs->scratchBufPtr = hwAttrs->defaultTxBufValue;
        txBuf = hwAttrs->scratchBufPtr;
    }

    /* Start the polling transfer */
    MAP_SPIWordCountSet(hwAttrs->baseAddr, 0);
    MAP_SPIEnable(hwAttrs->baseAddr);
    MAP_SPICSEnable(hwAttrs->baseAddr);

    (*object->spiPollingFxn) (hwAttrs->baseAddr, rxBuf, txBuf, rxIncrement,
            txIncrement, transaction->count);

    MAP_SPICSDisable(hwAttrs->baseAddr);
    MAP_SPIDisable(hwAttrs->baseAddr);
}

/*
 *  ======== spiPollingTransfer8 ========
 */
static void spiPollingTransfer8(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count)
{
    unsigned long buffer;
    uint8_t *rxBuf = (uint8_t *) rx;
    uint8_t *txBuf = (uint8_t *) tx;

    while (count--) {
        MAP_SPIDataPut(baseAddr, *txBuf);
        MAP_SPIDataGet(baseAddr, &buffer);
        *rxBuf = (uint8_t) buffer;

        rxBuf = (uint8_t *) ((uint32_t) rxBuf + rxInc);
        txBuf = (uint8_t *) ((uint32_t) txBuf + txInc);
    }
}

/*
 *  ======== spiPollingTransfer16 ========
 */
static void spiPollingTransfer16(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count)
{
    unsigned long buffer;
    uint16_t *rxBuf = (uint16_t *) rx;
    uint16_t *txBuf = (uint16_t *) tx;

    while (count--) {
        MAP_SPIDataPut(baseAddr, *txBuf);
        MAP_SPIDataGet(baseAddr, &buffer);
        *rxBuf = (uint16_t) buffer;

        rxBuf = (uint16_t *) ((uint32_t) rxBuf + rxInc);
        txBuf = (uint16_t *) ((uint32_t) txBuf + txInc);
    }
}

/*
 *  ======== spiPollingTransfer32 ========
 */
static void spiPollingTransfer32(uint32_t baseAddr, void *rx, void *tx,
    uint8_t rxInc, uint8_t txInc, size_t count)
{
    uint32_t *rxBuf = (uint32_t *) rx;
    uint32_t *txBuf = (uint32_t *) tx;

    while (count--) {
        MAP_SPIDataPut(baseAddr, *txBuf);
        MAP_SPIDataGet(baseAddr, (unsigned long *) rxBuf);

        rxBuf = (uint32_t *) ((uint32_t) rxBuf + rxInc);
        txBuf = (uint32_t *) ((uint32_t) txBuf + txInc);
    }
}

/*
 *  ======== transferCallback ========
 *  Callback function for when the SPI is in SPI_MODE_BLOCKING
 *
 *  @pre    Function assumes that the handle is not NULL
 */
static void transferCallback(SPI_Handle handle, SPI_Transaction *transaction)
{
    SPICC32XXDMA_Object *object = handle->object;

    DebugP_log1("SPI:(%p) posting transferComplete semaphore",
        ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

    SemaphoreP_post(object->transferComplete);
}

/*
 *  ======== SPICC32XXDMA_close ========
 *  @pre    Function assumes that the handle is not NULL
 */
void SPICC32XXDMA_close(SPI_Handle handle)
{
    uintptr_t                     key;
    uint32_t                      padRegister;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    MAP_SPIDisable(hwAttrs->baseAddr);
    MAP_SPICSDisable(hwAttrs->baseAddr);

    MAP_SPIDmaDisable(hwAttrs->baseAddr, SPI_RX_DMA | SPI_TX_DMA);
    MAP_SPIFIFODisable(hwAttrs->baseAddr, SPI_RX_FIFO | SPI_TX_FIFO);

    /* Release power dependency on SPI. */
    Power_releaseDependency(object->powerMgrId);
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

    DebugP_log1("SPI:(%p) closed", hwAttrs->baseAddr);

    key = HwiP_disable();
    object->isOpen = false;
    HwiP_restore(key);
}

/*
 *  ======== SPICC32XXDMA_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t SPICC32XXDMA_control(SPI_Handle handle, uint_fast16_t cmd, void *arg)
{
    /* No implementation yet */
    return (SPI_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== SPICC32XXDMA_init ========
 *  @pre    Function assumes that the handle is not NULL
 */
void SPICC32XXDMA_init(SPI_Handle handle)
{
    UDMACC32XX_init();
}

/*
 *  ======== SPICC32XXDMA_open ========
 *  @pre    Function assumes that the handle is not NULL
 */
SPI_Handle SPICC32XXDMA_open(SPI_Handle handle, SPI_Params *params)
{
    uintptr_t                     key;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    union {
        HwiP_Params               hwiParams;
        SemaphoreP_Params         semParams;
    } paramsUnion;
    uint16_t                      pin;
    uint16_t                      mode;

    object->powerMgrId = getPowerMgrId(hwAttrs->baseAddr);
    if (object->powerMgrId == (unsigned int) -1) {
        DebugP_log1("SPI:(%p) Failed to determine Power resource Id",
            hwAttrs->baseAddr);
        return (NULL);
    }

    key = HwiP_disable();
    if(object->isOpen == true) {
        HwiP_restore(key);
        return (NULL);
    }

    object->isOpen = true;
    HwiP_restore(key);

    DebugP_assert((params->dataSize >= 4) && (params->dataSize <= 32));

    /* Determine if we need to use an 8, 16 or 32-bit frameSize for the DMA */
    if (params->dataSize <= 8) {
        object->frameSize = SPICC32XXDMA_8bit;
        object->spiPollingFxn = spiPollingTransfer8;
    }
    else if (params->dataSize <= 16) {
        object->frameSize = SPICC32XXDMA_16bit;
        object->spiPollingFxn = spiPollingTransfer16;
    }
    else {
        object->frameSize = SPICC32XXDMA_32bit;
        object->spiPollingFxn = spiPollingTransfer32;
    }

    /* Store SPI mode of operation */
    object->spiMode = params->mode;
    object->transferMode = params->transferMode;
    object->transaction = NULL;
    object->rxFifoTrigger = (1 << object->frameSize);
    object->txFifoTrigger = (1 << object->frameSize);
    object->bitRate = params->bitRate;
    object->frameFormat = params->frameFormat;
    object->dataSize = params->dataSize;
    object->transferComplete = NULL;

    object->dmaHandle = UDMACC32XX_open();
    if (object->dmaHandle == NULL) {
        DebugP_log1("SPI:(%p) UDMACC32XX_open() failed.", hwAttrs->baseAddr);
        object->isOpen = false;
        return (NULL);
    }

    /* Register power dependency - i.e. power up and enable clock for SPI. */
    Power_setDependency(object->powerMgrId);
    Power_registerNotify(&(object->notifyObj), PowerCC32XX_AWAKE_LPDS,
        postNotifyFxn, (uintptr_t) handle);

    /* Configure the pins */
    if (hwAttrs->mosiPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->mosiPin) & 0xff;
        mode = (hwAttrs->mosiPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long)pin, (unsigned long)mode);
    }

    if (hwAttrs->misoPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->misoPin) & 0xff;
        mode = (hwAttrs->misoPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long)pin, (unsigned long)mode);
    }

    if (hwAttrs->clkPin != SPICC32XXDMA_PIN_NO_CONFIG) {
        pin = (hwAttrs->clkPin) & 0xff;
        mode = (hwAttrs->clkPin >> 8) & 0xff;
        MAP_PinTypeSPI((unsigned long)pin, (unsigned long)mode);
    }

    if (hwAttrs->pinMode == SPI_4PIN_MODE) {
        if (hwAttrs->csPin != SPICC32XXDMA_PIN_NO_CONFIG) {
            pin = (hwAttrs->csPin) & 0xff;
            mode = (hwAttrs->csPin >> 8) & 0xff;
            MAP_PinTypeSPI((unsigned long)pin, (unsigned long)mode);
        }
    }

    HwiP_Params_init(&(paramsUnion.hwiParams));
    paramsUnion.hwiParams.arg = (uintptr_t) handle;
    paramsUnion.hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(hwAttrs->intNum, spiHwiFxn,
            &(paramsUnion.hwiParams));
    if (object->hwiHandle == NULL) {
        SPICC32XXDMA_close(handle);
        return (NULL);
    }

    if (object->transferMode == SPI_MODE_BLOCKING) {
        DebugP_log1("SPI:(%p) in SPI_MODE_BLOCKING mode", hwAttrs->baseAddr);

        SemaphoreP_Params_init(&(paramsUnion.semParams));
        paramsUnion.semParams.mode = SemaphoreP_Mode_BINARY;
        object->transferComplete = SemaphoreP_create(0,
                &(paramsUnion.semParams));
        if (object->transferComplete == NULL) {
            SPICC32XXDMA_close(handle);
            return (NULL);
        }

        object->transferCallbackFxn = transferCallback;
    }
    else {
        DebugP_log1("SPI:(%p) in SPI_MODE_CALLBACK mode", hwAttrs->baseAddr);

        DebugP_assert(params->transferCallbackFxn != NULL);
        object->transferCallbackFxn = params->transferCallbackFxn;
    }

    initHw(handle);

    DebugP_log3("SPI:(%p) CPU freq: %d; SPI freq to %d", hwAttrs->baseAddr,
        MAP_PRCMPeripheralClockGet(hwAttrs->spiPRCM), params->bitRate);
    DebugP_log1("SPI:(%p) opened", hwAttrs->baseAddr);

    return (handle);
}

/*
 *  ======== SPICC32XXDMA_transferCancel ========
 *  A function to cancel a transaction (if one is in progress) when the driver
 *  is in SPI_MODE_CALLBACK.
 *
 *  @pre    Function assumes that the handle is not NULL
 */
void SPICC32XXDMA_transferCancel(SPI_Handle handle)
{
    /* No implementation yet */
    DebugP_assert(false);
}

/*
 *  ======== SPICC32XXDMA_transfer ========
 *  @pre    Function assumes that handle and transaction is not NULL
 */
bool SPICC32XXDMA_transfer(SPI_Handle handle, SPI_Transaction *transaction)
{
    uintptr_t                     key;
    SPICC32XXDMA_Object          *object = handle->object;
    SPICC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    bool                          buffersAligned;
    uint8_t                       alignMask;

    /* This is a limitation by the micro DMA controller */
    if ((transaction->count == 0) || (transaction->count > 1024) ||
        !(transaction->rxBuf || transaction->txBuf) ||
        (!(transaction->rxBuf && transaction->txBuf) && !hwAttrs->scratchBufPtr)) {
        return (false);
    }

    alignMask = (object->frameSize) ? ((object->frameSize << 1) - 1) : (0);
    buffersAligned = ((((uint32_t) transaction->rxBuf & alignMask) == 0) &&
            (((uint32_t) transaction->txBuf & alignMask) == 0));

    key = HwiP_disable();
    if (object->transaction) {
        HwiP_restore(key);
        DebugP_log1("SPI:(%p) ERROR! Transaction still in progress",
            ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

        return (false);
    }
    else if (object->transferMode == SPI_MODE_CALLBACK && !buffersAligned) {
            HwiP_restore(key);

        DebugP_log1("SPI:(%p) ERROR! RX/TX buffers not aligned to data size.",
            ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

        return (false);
    }
    else {
        object->transaction = transaction;
        HwiP_restore(key);
    }

    /*
     * Perform polling transfer if in blocking mode and one of the following
     * is true:
     *   - RX/TX buffers are not alligned.
     *   - transfer->count is less than DMA_MIN_TRANSFER_SIZE threshold.
     */
    if (object->transferMode == SPI_MODE_BLOCKING &&
            (transaction->count < hwAttrs->minDmaTransferSize || !buffersAligned)) {
        DebugP_log1("SPI:(%p) starting polling SPI transfer",
            ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

            spiPollingTransfer(object, hwAttrs, transaction);

            object->transaction = NULL;

            DebugP_log1("SPI:(%p) polling SPI transfer complete.",
                ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

            return (true);
    }

    /* Set constraints to guarantee transaction */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    configDMA(handle, transaction);
    MAP_SPIIntClear(hwAttrs->baseAddr, SPI_INT_DMARX | SPI_INT_DMATX | SPI_INT_EOW);
    MAP_SPIIntEnable(hwAttrs->baseAddr, SPI_INT_DMARX | SPI_INT_DMATX | SPI_INT_EOW);

    MAP_SPIEnable(hwAttrs->baseAddr);
    MAP_SPICSEnable(hwAttrs->baseAddr);

    if (object->transferMode == SPI_MODE_BLOCKING) {
        DebugP_log1("SPI:(%p) transfer pending on transferComplete semaphore",
            ((SPICC32XXDMA_HWAttrsV1 const *) (handle->hwAttrs))->baseAddr);

        SemaphoreP_pend(object->transferComplete, SemaphoreP_WAIT_FOREVER);
    }

    return (true);
}
