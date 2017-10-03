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

#include <stdint.h>
#include <string.h>
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

#include <ti/drivers/utils/List.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/dma/UDMACC32XX.h>

#include <ti/drivers/i2s/I2SCC32XXDMA.h>

/* driverlib header files */
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_mcasp.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/interrupt.h>
#include <ti/devices/cc32xx/driverlib/i2s.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

#define PinConfigPinMode(config)       (((config) >> 8) & 0xF)
#define PinConfigPin(config)           (((config) >> 0) & 0x3F)

#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

/* I2SCC32XXDMA functions */
void         I2SCC32XXDMA_close(I2S_Handle handle);
int_fast16_t I2SCC32XXDMA_control(I2S_Handle handle, uint_fast16_t cmd,
                                 void *arg);
void         I2SCC32XXDMA_init(I2S_Handle handle);
I2S_Handle   I2SCC32XXDMA_open(I2S_Handle handle, I2S_Params *params);
int_fast16_t I2SCC32XXDMA_readIssue(I2S_Handle handle, I2S_BufDesc *desc);
size_t       I2SCC32XXDMA_readReclaim(I2S_Handle handle, I2S_BufDesc **desc);
int_fast16_t I2SCC32XXDMA_writeIssue(I2S_Handle handle, I2S_BufDesc *desc);
size_t       I2SCC32XXDMA_writeReclaim(I2S_Handle handle, I2S_BufDesc **desc);

/* I2S function table for I2SCC32XXDMA implementation */
const I2S_FxnTable I2SCC32XXDMA_fxnTable = {
    I2SCC32XXDMA_close,
    I2SCC32XXDMA_control,
    I2SCC32XXDMA_init,
    I2SCC32XXDMA_open,
    I2SCC32XXDMA_readIssue,
    I2SCC32XXDMA_readReclaim,
    I2SCC32XXDMA_writeIssue,
    I2SCC32XXDMA_writeReclaim
};

/*
 * This lookup table is used to configure the DMA channels for the appropriate
 * (16bit or 32bit) transfer sizes.
 * Table for an I2S DMA RX channel.
 */
static const uint32_t dmaRxConfig[] = {
    UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_CHCTL_DSTINC_16 | UDMA_ARB_8,
    UDMA_SIZE_32 | UDMA_SRC_INC_NONE | UDMA_CHCTL_DSTINC_32 | UDMA_ARB_8
};

/*
 * This lookup table is used to configure the DMA channels for the appropriate
 * (16bit or 32bit) transfer sizes.
 * Table for an I2 DMA TX channel
 */
static const uint32_t dmaTxConfig[] = {
    UDMA_SIZE_16 | UDMA_CHCTL_SRCINC_16 | UDMA_DST_INC_NONE | UDMA_ARB_8,
    UDMA_SIZE_32 | UDMA_CHCTL_SRCINC_32 | UDMA_DST_INC_NONE | UDMA_ARB_8
};

/*Zero buffer to write when there is no data from the application*/
uint8_t I2SCC32XXDMA_zeroBuffer[128];
static I2S_BufDesc  I2SCC32XXDMA_zeroBufDesc;

/*Empty buffer to read into when there is no data requested
  from the application*/
static uint8_t I2SCC32XXDMA_emptyBuffer[128];
static I2S_BufDesc  I2SCC32XXDMA_emptyBufDesc;


/*
 *  ======== I2SCC32XXDMA_configDMA ========
 */
static void I2SCC32XXDMA_configDMA(I2S_Handle handle, I2S_BufDesc *desc,
                                                            bool isWrite)
{
    I2SCC32XXDMA_Object           *object = handle->object;
    I2SCC32XXDMA_HWAttrsV1 const  *hwAttrs = handle->hwAttrs;
    unsigned long                  channelControlOptions;
    uint32_t                       divider = 1;

    if (object->dmaSize == I2SCC32XXDMA_16bit) {
        divider = 2;
    }
    else if (object->dmaSize == I2SCC32XXDMA_32bit) {
        divider = 4;
    }

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    if (isWrite) {
        channelControlOptions = dmaTxConfig[object->dmaSize];

        /* Setup ping-pong transfer */
        MAP_uDMAChannelControlSet(hwAttrs->txChannelIndex,
                                  channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex,
            UDMA_MODE_PINGPONG, (void *)desc->bufPtr,
            (void *)I2S_TX_DMA_PORT, ((desc->bufSize / 2) / divider));

        /* Pong Buffer */
        MAP_uDMAChannelControlSet(hwAttrs->txChannelIndex | UDMA_ALT_SELECT,
            channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex | UDMA_ALT_SELECT,
            UDMA_MODE_PINGPONG,
            (void *)(((char *)desc->bufPtr) + (desc->bufSize / 2)),
            (void *)I2S_TX_DMA_PORT,
            ((desc->bufSize / 2) / divider));

        MAP_uDMAChannelAttributeEnable(hwAttrs->txChannelIndex, UDMA_ATTR_USEBURST);
        MAP_uDMAChannelEnable(hwAttrs->txChannelIndex);

        /* Configure TX FIFO */
        MAP_I2STxFIFOEnable(hwAttrs->baseAddr,8,1);
    }
    else {

        /*
         * TX is always required so configure TX dma
         *  if 'read' is called first
         */
        I2SCC32XXDMA_configDMA(handle, &I2SCC32XXDMA_zeroBufDesc, true);

        channelControlOptions = dmaRxConfig[object->dmaSize];

        /* Setup ping-pong transfer */
        MAP_uDMAChannelControlSet(hwAttrs->rxChannelIndex,
                                   channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex,
            UDMA_MODE_PINGPONG, (void *)I2S_RX_DMA_PORT,
            (void *)desc->bufPtr, ((desc->bufSize / 2) / divider));

        /* Pong Buffer */
        MAP_uDMAChannelControlSet(hwAttrs->rxChannelIndex | UDMA_ALT_SELECT,
            channelControlOptions);

        MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex | UDMA_ALT_SELECT,
            UDMA_MODE_PINGPONG, (void *)I2S_RX_DMA_PORT,
            (void *)(((char *)desc->bufPtr) + (desc->bufSize / 2)),
            ((desc->bufSize / 2) / divider));

        MAP_uDMAChannelAttributeEnable(hwAttrs->rxChannelIndex, UDMA_ATTR_USEBURST);
        MAP_uDMAChannelEnable(hwAttrs->rxChannelIndex);

        /* Configure RX FIFO */
        MAP_I2SRxFIFOEnable(hwAttrs->baseAddr,8,1);

    }

    DebugP_log1("I2S:(%p) DMA transfer enabled", hwAttrs->baseAddr);

    if (isWrite) {
        DebugP_log3("I2S:(%p) DMA transmit, txBuf: %p; Count: %d",
            hwAttrs->baseAddr, (uintptr_t)(desc->bufPtr), desc->bufSize);
    }
    else {
        DebugP_log3("I2S:(%p) DMA receive, rxBuf: %p; Count: %d",
            hwAttrs->baseAddr, (uintptr_t)(desc->bufPtr), desc->bufSize);
    }
}

/*
 *  ======== I2SCC32XXDMA_hwiIntFxn ========
 *  Hwi function that processes I2S interrupts.
 *
 *  @param(arg)         The I2S_Handle for this Hwi.
 */
static void I2SCC32XXDMA_hwiIntFxn(uintptr_t arg)
{
    I2SCC32XXDMA_Object          *object = ((I2S_Handle)arg)->object;
    I2SCC32XXDMA_HWAttrsV1 const *hwAttrs = ((I2S_Handle)arg)->hwAttrs;
    uint32_t            divider = 1;
    uint32_t            intStatus;

    if (object->dmaSize == I2SCC32XXDMA_16bit) {
        divider = 2;
    }
    else if (object->dmaSize == I2SCC32XXDMA_32bit) {
        divider = 4;
    }

    /* Read the interrupt status */
    intStatus = MAP_I2SIntStatus(hwAttrs->baseAddr);

    /* Check for TX DMA done interrupt */
    if (intStatus & I2S_STS_XDMA) {


        /* Clear the interrupt */
        MAP_I2SIntClear(hwAttrs->baseAddr,I2S_STS_XDMA);

        /* Check if ping is over */
        if (UDMA_MODE_STOP == MAP_uDMAChannelModeGet(hwAttrs->txChannelIndex)){


            /* Get the next user buffer from the write queue*/
            object->currentWriteBufDesc = (I2S_BufDesc *)List_get(&object->writeActiveQueue);

            /* If no user write buffer use the zero buffer */
            if(NULL == object->currentWriteBufDesc)
            {
                object->currentWriteBufDesc = &I2SCC32XXDMA_zeroBufDesc;
            }

            /* Setup the DMA control table entry */
            MAP_uDMAChannelTransferSet(hwAttrs->txChannelIndex,
                         UDMA_MODE_PINGPONG,
                        (void *)object->currentWriteBufDesc->bufPtr,
                        (void *)I2S_TX_DMA_PORT,
                        ((object->currentWriteBufDesc->bufSize / 2) / divider));

        }
        else {

            /* If prevWriteBufDesc points to valid user buffer release it */
            if( (NULL != object->prevWriteBufDesc) &&
                    (object->prevWriteBufDesc != &I2SCC32XXDMA_zeroBufDesc) ){

                object->serialPinVars[object->writeIndex].readWriteCallback(
                                                    (I2S_Handle)arg, object->prevWriteBufDesc);
            }

            /* Setup the DMA control table alternate entry */
            MAP_uDMAChannelTransferSet(
                            hwAttrs->txChannelIndex | UDMA_ALT_SELECT,
                            UDMA_MODE_PINGPONG,
                            (void *)((char*)object->currentWriteBufDesc->bufPtr +
                                     (object->currentWriteBufDesc->bufSize / 2)),
                            (void *)I2S_TX_DMA_PORT,
                            ((object->currentWriteBufDesc->bufSize / 2) / divider));

            /* Save pointer to issued buffer descriptor.*/
            object->prevWriteBufDesc = object->currentWriteBufDesc;
        }

        DebugP_log2("I2S:(%p) Write finished, %d bytes written",
               hwAttrs->baseAddr, object->currentWriteBufDesc->bufSize);
    }

    /* Check if Rx DMA done interrupt */
    if (intStatus & I2S_STS_RDMA) {

        /* Clear the read interrupt */
        MAP_I2SIntClear(hwAttrs->baseAddr,I2S_STS_RDMA);

        /* Check if ping is over */
        if (UDMA_MODE_STOP == MAP_uDMAChannelModeGet(hwAttrs->rxChannelIndex)) {

            /* Get the next user buffer from the read queue*/
            object->currentReadBufDesc = (I2S_BufDesc *)List_get(&object->readActiveQueue);

            /* If no user read buffer use the empty buffer */
            if(NULL == object->currentReadBufDesc)
            {
                object->currentReadBufDesc = &I2SCC32XXDMA_emptyBufDesc;
            }


            MAP_uDMAChannelTransferSet(hwAttrs->rxChannelIndex,
                         UDMA_MODE_PINGPONG,
                        (void *)I2S_RX_DMA_PORT,
                        (void *)object->currentReadBufDesc->bufPtr,
                        ((object->currentReadBufDesc->bufSize / 2) / divider));

        }
        else {

            /* If prevReadBufDesc points to valid user buffer release it */
            if( (NULL != object->prevReadBufDesc) &&
                    (object->prevReadBufDesc != &I2SCC32XXDMA_emptyBufDesc) ){

                object->serialPinVars[object->readIndex].readWriteCallback(
                                                    (I2S_Handle)arg, object->prevReadBufDesc);

            }

            /* Setup the DMA control table alternate entry */
            MAP_uDMAChannelTransferSet(
                hwAttrs->rxChannelIndex | UDMA_ALT_SELECT,
                UDMA_MODE_PINGPONG, (void *)I2S_RX_DMA_PORT,
                (void *)((char*)object->currentReadBufDesc->bufPtr +
                        (object->currentReadBufDesc->bufSize / 2)),
                ((object->currentReadBufDesc->bufSize / 2) / divider));

            /* Save pointer to issued buffer descriptor.*/
            object->prevReadBufDesc = object->currentReadBufDesc;
        }

        DebugP_log2("I2S:(%p) Read finished, %d bytes read",
            hwAttrs->baseAddr, object->currentReadBufDesc->bufSize);
    }
}

/*
 *  ======== I2SCC32XX_Params_init ========
 */
void I2SCC32XXDMA_Params_init(I2SCC32XXDMA_SerialPinParams *params)
{
     DebugP_assert(params != NULL);

     params->serialPinConfig[0].pinNumber         = 0;
     params->serialPinConfig[0].pinMode           = I2S_PINMODE_TX;
     params->serialPinConfig[0].inActiveConfig    =
                                     I2S_SERCONFIG_INACT_LOW_LEVEL;


     params->serialPinConfig[1].pinNumber         = 1;
     params->serialPinConfig[1].pinMode           = I2S_PINMODE_RX;
     params->serialPinConfig[1].inActiveConfig    =
                                     I2S_SERCONFIG_INACT_LOW_LEVEL;

}

/*
 *  ======== readIssueCallback ========
 *  Simple callback to post a semaphore for the issue-reclaim mode.
 */
static void readIssueCallback(I2S_Handle handle, I2S_BufDesc *desc)
{
    I2SCC32XXDMA_Object *object = handle->object;

    List_put(&object->readDoneQueue, &(desc->qElem));
    SemaphoreP_post(object->readSem);
}

/*
 *  ======== writeIssueCallback ========
 *  Simple callback to post a semaphore for the issue-reclaim mode.
 */
static void writeIssueCallback(I2S_Handle handle, I2S_BufDesc *desc)
{
    I2SCC32XXDMA_Object *object = handle->object;

    List_put(&object->writeDoneQueue, &(desc->qElem));
    SemaphoreP_post(object->writeSem);
}

/*
 *  ======== I2SCC32XXDMA_close ========
 */
void I2SCC32XXDMA_close(I2S_Handle handle)
{
    I2SCC32XXDMA_Object           *object = handle->object;
    I2SCC32XXDMA_HWAttrsV1 const  *hwAttrs = handle->hwAttrs;
    uint32_t                      padRegister;

    /* Disable I2S and interrupts. */
    MAP_I2SDisable(hwAttrs->baseAddr);

    if (object->hwiHandle) {
        HwiP_delete(object->hwiHandle);
    }
    if (object->writeSem != NULL) {
        SemaphoreP_delete(object->writeSem);
    }
    if (object->readSem != NULL){
        SemaphoreP_delete(object->readSem);
    }
    if (object->dmaHandle != NULL) {
        UDMACC32XX_close(object->dmaHandle);
    }

    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

    Power_releaseDependency(PowerCC32XX_PERIPH_I2S);

    /* Restore pin pads to their reset states */
    padRegister = (PinToPadGet((hwAttrs->xr0Pin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->xr1Pin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->clkxPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->clkPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->fsxPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;

    object->opened = false;

    DebugP_log1("I2S:(%p) closed", hwAttrs->baseAddr);
}

/*
 *  ======== I2SCC32XXDMA_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t I2SCC32XXDMA_control(I2S_Handle handle, uint_fast16_t cmd, void *arg)
{
    I2SCC32XXDMA_Object *object = handle->object;
    uint32_t newSize = *(uint32_t *)arg;

    switch(cmd){
        case I2SCC32XXDMA_CMD_SET_ZEROBUF_LEN:
            if (newSize > 32){
                return -1;
            }
            I2SCC32XXDMA_zeroBufDesc.bufSize = newSize;
            object->zeroWriteBufLength       = newSize;
            return(I2SCC32XXDMA_CMD_SET_ZEROBUF_LEN);

        case I2SCC32XXDMA_CMD_SET_EMPTYBUF_LEN:
            if (newSize > 32){
                return -1;
            }
            I2SCC32XXDMA_emptyBufDesc.bufSize = newSize;
            object->emptyReadBufLength        = newSize;
            return(I2SCC32XXDMA_CMD_SET_EMPTYBUF_LEN);

        default:
            return (I2S_STATUS_UNDEFINEDCMD);
    }
}

/*
 *  ======== I2SCC32XXDMA_init ========
 */
void I2SCC32XXDMA_init(I2S_Handle handle)
{
    I2SCC32XXDMA_Object *object = handle->object;

    object->opened = false;

    UDMACC32XX_init();
}

/*
 *  ======== I2SCC32XXDMA_open ========
 */
I2S_Handle I2SCC32XXDMA_open(I2S_Handle handle, I2S_Params *params)
{
    uintptr_t                     key;
    unsigned int                  bitClock;
    unsigned int                  slotConfig;
    I2SCC32XXDMA_Object          *object = handle->object;
    I2SCC32XXDMA_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    I2SCC32XXDMA_SerialPinParams *cc3200CustomParams;
    SemaphoreP_Params             semParams;
    HwiP_Params                   hwiParams;
    int                           i;
    uint32_t                      pin;
    uint32_t                      mode;
    unsigned long                 i2s_data_line[] = {
        I2S_DATA_LINE_0, I2S_DATA_LINE_1
    };
    I2SCC32XXDMA_SerialPinParams defaultSerialPinParams;

    cc3200CustomParams = (I2SCC32XXDMA_SerialPinParams *)params->customParams;

    /* If customParams are NULL use defaults. */
    if (!params->customParams) {

        I2SCC32XXDMA_Params_init(&defaultSerialPinParams);
        cc3200CustomParams = &defaultSerialPinParams;
    }

    /* Disable preemption while checking if the I2S is open. */
    key = HwiP_disable();

    /* Check if the I2S is open already with the base addr. */
    if (object->opened == true) {
        HwiP_restore(key);

        DebugP_log1("I2S:(%p) already in use.", hwAttrs->baseAddr);
        return (NULL);
    }
    object->opened = true;

    HwiP_restore(key);

    /* Set I2S variables to defaults. */
    object->readSem = NULL;
    object->writeSem = NULL;
    object->writeIndex = I2SCC32XXDMA_INDEX_INVALID;
    object->readIndex  = I2SCC32XXDMA_INDEX_INVALID;
    object->dmaHandle = NULL;
    object->currentWriteBufDesc = NULL;
    object->currentReadBufDesc = NULL;

    object->emptyReadBufLength = sizeof(I2SCC32XXDMA_emptyBuffer);
    object->zeroWriteBufLength = sizeof(I2SCC32XXDMA_zeroBuffer);

    memset(&I2SCC32XXDMA_zeroBuffer,0,sizeof(I2SCC32XXDMA_zeroBuffer));

    I2SCC32XXDMA_zeroBufDesc.bufPtr  = &I2SCC32XXDMA_zeroBuffer;
    I2SCC32XXDMA_zeroBufDesc.bufSize = object->zeroWriteBufLength;

    I2SCC32XXDMA_emptyBufDesc.bufPtr  = &I2SCC32XXDMA_emptyBuffer;
    I2SCC32XXDMA_emptyBufDesc.bufSize = object->emptyReadBufLength;

    object->operationMode  = params->operationMode;

    /*
     *  Register power dependency. Keeps the clock running in SLP
     *  and DSLP modes.
     */
    Power_setDependency(PowerCC32XX_PERIPH_I2S);
    MAP_PRCMPeripheralReset(PRCM_I2S);

    pin = PinConfigPin(hwAttrs->xr0Pin);
    mode = PinConfigPinMode(hwAttrs->xr0Pin);
    MAP_PinTypeI2S((unsigned long)pin, (unsigned long)mode);

    pin = PinConfigPin(hwAttrs->xr1Pin);
    mode = PinConfigPinMode(hwAttrs->xr1Pin);
    MAP_PinTypeI2S((unsigned long)pin, (unsigned long)mode);

    pin = PinConfigPin(hwAttrs->clkxPin);
    mode = PinConfigPinMode(hwAttrs->clkxPin);
    MAP_PinTypeI2S((unsigned long)pin, (unsigned long)mode);

    pin = PinConfigPin(hwAttrs->clkPin);
    mode = PinConfigPinMode(hwAttrs->clkPin);
    MAP_PinTypeI2S((unsigned long)pin, (unsigned long)mode);

    pin = PinConfigPin(hwAttrs->fsxPin);
    mode = PinConfigPinMode(hwAttrs->fsxPin);
    MAP_PinTypeI2S((unsigned long)pin, (unsigned long)mode);

    for (i = 0; i < 2; i++) {
        if (cc3200CustomParams->serialPinConfig[i].pinMode !=
            I2S_PINMODE_INACTIVE) {

            if (cc3200CustomParams->serialPinConfig[i].pinMode ==
                I2S_PINMODE_RX) {

                object->readIndex = i;
                object->serialPinVars[i].readWriteMode = params->readMode;
                object->serialPinVars[i].readWriteCallback =
                     params->readCallback;
                object->serialPinVars[i].readWriteTimeout = params->readTimeout;

                MAP_I2SSerializerConfig(hwAttrs->baseAddr,i2s_data_line[i],
                    I2S_SER_MODE_RX, I2S_INACT_LOW_LEVEL);
            }
            else if (cc3200CustomParams->serialPinConfig[i].pinMode ==
                I2S_PINMODE_TX) {

                object->writeIndex = i;
                object->serialPinVars[i].readWriteMode =
                    params->writeMode;
                object->serialPinVars[i].readWriteCallback =
                    params->writeCallback;
                object->serialPinVars[i].readWriteTimeout =
                    params->writeTimeout;
                MAP_I2SSerializerConfig(hwAttrs->baseAddr,i2s_data_line[i],
                    I2S_SER_MODE_TX, I2S_INACT_LOW_LEVEL);
            }
        }
    }

    /* Disable the I2S interrupt. */
    MAP_I2SIntDisable(hwAttrs->baseAddr,I2S_INT_XDMA | I2S_INT_RDMA);

    HwiP_clearInterrupt(hwAttrs->intNum);

    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(hwAttrs->intNum,
                                    I2SCC32XXDMA_hwiIntFxn,
                                    &hwiParams);
    if (object->hwiHandle == NULL) {
        I2SCC32XXDMA_close(handle);
        return (NULL);
    }

    object->dmaHandle = UDMACC32XX_open();
    if (object->dmaHandle == NULL) {
        I2SCC32XXDMA_close(handle);
        return (NULL);
    }

    MAP_I2SIntEnable(hwAttrs->baseAddr,I2S_INT_XDMA | I2S_INT_RDMA);

    if (params->operationMode == I2S_OPMODE_TX_ONLY) {
        object->operationMode = I2S_MODE_TX_ONLY;
    }
    else if (params->operationMode == I2S_OPMODE_TX_RX_SYNC) {
        object->operationMode = I2S_MODE_TX_RX_SYNC;
    }
    else {
        I2SCC32XXDMA_close(handle);
        return (NULL);
    }
    if (params->slotLength == 16) {
        slotConfig = I2S_SLOT_SIZE_16;
        object->dmaSize = I2SCC32XXDMA_16bit;
    }
    else if (params->slotLength == 24) {
        slotConfig = I2S_SLOT_SIZE_24;
        object->dmaSize = I2SCC32XXDMA_32bit;
    }
    else {
        I2SCC32XXDMA_close(handle);
        return (NULL);
    }

    SemaphoreP_Params_init(&semParams);
    if (object->writeIndex != I2SCC32XXDMA_INDEX_INVALID) {
        /* Initialize write Queue */
        List_clearList(&object->writeActiveQueue);

        if (object->serialPinVars[object->writeIndex].readWriteMode ==
                                               I2S_MODE_ISSUERECLAIM) {
            object->serialPinVars[object->writeIndex].readWriteCallback =
                                                   &writeIssueCallback;
            semParams.mode = SemaphoreP_Mode_COUNTING;

            object->writeSem = SemaphoreP_create(0, &semParams);
            if (object->writeSem == NULL) {
                DebugP_log1("I2S:(%p) Failed to create semaphore.",
                    hwAttrs->baseAddr);
                I2SCC32XXDMA_close(handle);
                return (NULL);
            }
        }

        /* Initialize write Queue */
        List_clearList(&object->writeDoneQueue);
    }

    if (object->readIndex != I2SCC32XXDMA_INDEX_INVALID) {
        List_clearList(&object->readActiveQueue);

        if (object->serialPinVars[object->readIndex].readWriteMode ==
                I2S_MODE_ISSUERECLAIM) {
            object->serialPinVars[object->readIndex].readWriteCallback =
                                                      &readIssueCallback;
            semParams.mode = SemaphoreP_Mode_COUNTING;
            object->readSem = SemaphoreP_create(0, &semParams);

            if (object->readSem == NULL) {
                if (object->writeSem) {
                    SemaphoreP_delete(object->writeSem);

                    DebugP_log1("I2S:(%p) Failed to create semaphore.",
                        hwAttrs->baseAddr);
                    I2SCC32XXDMA_close(handle);
                    return (NULL);
                }

                /* Initialize read Queues */
                List_clearList(&object->readDoneQueue);
            }
        }
    }

    bitClock = (params->samplingFrequency *
        params->slotLength * params->numChannels);
    MAP_PRCMI2SClockFreqSet(bitClock);

    MAP_I2SConfigSetExpClk(hwAttrs->baseAddr,bitClock,bitClock,slotConfig |
                                      I2S_PORT_DMA);

    MAP_uDMAChannelAssign(hwAttrs->rxChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->rxChannelIndex,
                                     UDMA_ATTR_ALTSELECT);

    MAP_uDMAChannelAssign(hwAttrs->txChannelIndex);
    MAP_uDMAChannelAttributeDisable(hwAttrs->txChannelIndex,
                                     UDMA_ATTR_ALTSELECT);


    /* Configure the DMA with zero/empty buffers */
    if (params->operationMode == I2S_OPMODE_TX_ONLY){

        I2SCC32XXDMA_configDMA(handle, &I2SCC32XXDMA_zeroBufDesc,true);
    }
    else{

        I2SCC32XXDMA_configDMA(handle, &I2SCC32XXDMA_emptyBufDesc,false);
    }

    /* Enable the I2S */
    MAP_I2SEnable(hwAttrs->baseAddr,object->operationMode);


    DebugP_log1("I2S:(%p) opened", hwAttrs->baseAddr);

    /* Return the handle */
    return (handle);
}

/*
 *  ======== I2SCC32XXDMA_readIssue ========
 */
int_fast16_t I2SCC32XXDMA_readIssue(I2S_Handle handle, I2S_BufDesc *desc)
{
    I2SCC32XXDMA_Object *object = handle->object;

    if (object->readIndex == I2SCC32XXDMA_INDEX_INVALID) {
        return (I2S_STATUS_UNDEFINEDCMD);
    }

    List_put(&object->readActiveQueue, &(desc->qElem));

    return (0);
}

/*
 *  ======== I2SCC32XXDMA_readReclaim ========
 */
size_t I2SCC32XXDMA_readReclaim(I2S_Handle handle, I2S_BufDesc **desc)
{
    I2SCC32XXDMA_Object *object = handle->object;
    size_t               size = 0;

    *desc = NULL;
    if (SemaphoreP_OK != SemaphoreP_pend(object->readSem,
                object->serialPinVars[object->readIndex].readWriteTimeout)) {
        DebugP_log1("I2S:(%p) Read timed out",
                ((I2SCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);
    }
    else {

        *desc = (I2S_BufDesc *)List_get(&object->readDoneQueue);

        DebugP_assert(*desc != NULL);
        size = (*desc)->bufSize;
    }
    return (size);
}

/*
 *  ======== I2SCC32XXDMA_writeIssue ========
 */
int_fast16_t I2SCC32XXDMA_writeIssue(I2S_Handle handle, I2S_BufDesc *desc)
{
    I2SCC32XXDMA_Object  *object = handle->object;

    if (object->writeIndex == I2SCC32XXDMA_INDEX_INVALID) {
        return (I2S_STATUS_UNDEFINEDCMD);
    }

    List_put(&object->writeActiveQueue, &(desc->qElem));

    return (0);
}

/*
 *  ======== I2SCC32XXDMA_writeReclaim ========
 */
size_t I2SCC32XXDMA_writeReclaim(I2S_Handle handle, I2S_BufDesc **desc)
{
    I2SCC32XXDMA_Object *object = handle->object;
    size_t               size = 0;

    *desc = NULL;

    if (SemaphoreP_OK != SemaphoreP_pend(object->writeSem,
                object->serialPinVars[object->writeIndex].readWriteTimeout)) {
        DebugP_log1("I2S:(%p) Write timed out",
                ((I2SCC32XXDMA_HWAttrsV1 const *)(handle->hwAttrs))->baseAddr);

    }
    else {

        *desc = (I2S_BufDesc *)List_get(&object->writeDoneQueue);

        DebugP_assert(*desc != NULL);
        size = (*desc)->bufSize;
    }
    return (size);
}
