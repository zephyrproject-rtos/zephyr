/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
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
 *  ======== NVSSPI25X.c ========
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/ClockP.h>

#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSSPI25X.h>

#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>

/* Instruction codes */
#define SPIFLASH_WRITE              0x02 /**< Page Program */
#define SPIFLASH_READ               0x03 /**< Read Data */
#define SPIFLASH_READ_STATUS        0x05 /**< Read Status Register */
#define SPIFLASH_WRITE_ENABLE       0x06 /**< Write Enable */
#define SPIFLASH_SUBSECTOR_ERASE    0x20 /**< SubSector (4K Byte) Erase */
#define SPIFLASH_SECTOR_ERASE       0xD8 /**< Sector (64K Byte) Erase */
#define SPIFLASH_MASS_ERASE         0xC7 /**< Erase entire flash */

#define SPIFLASH_RDP                0xAB /**< Release from Deep Power Down */
#define SPIFLASH_DP                 0xB9 /**< Deep Power Down */

/* Bitmasks of the status register */
#define SPIFLASH_STATUS_BIT_BUSY    0x01 /**< Busy bit of status register */

/* Write page size assumed by this driver */
#define SPIFLASH_PROGRAM_PAGE_SIZE  256

/* Highest supported SPI instance index */
#define MAX_SPI_INDEX               3

/* Size of hardware sector erased by SPIFLASH_SECTOR_ERASE */
#define SPIFLASH_SECTOR_SIZE        0x10000

static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset, size_t size);
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size);
static int_fast16_t doRead(NVS_Handle handle, size_t offset, void *buffer,
                        size_t bufferSize);
static int_fast16_t doWriteVerify(NVS_Handle handle, size_t offset,
                        void *src, size_t srcBufSize, void *dst,
                        size_t dstBufSize, bool preFlag);

static int_fast16_t extFlashSpiWrite(const uint8_t *buf, size_t len);
static int_fast16_t extFlashSpiRead(uint8_t *buf, size_t len);
static int_fast16_t extFlashPowerDown(NVS_Handle nvsHandle);
static int_fast16_t extFlashPowerStandby(NVS_Handle nvsHandle);
static int_fast16_t extFlashWaitReady(NVS_Handle nvsHandle);
static int_fast16_t extFlashWriteEnable(NVS_Handle nvsHandle);
static int_fast16_t extFlashMassErase(NVS_Handle nvsHandle);

extern NVS_Config NVS_config[];
extern const uint8_t NVS_count;

/* NVS function table for NVSSPI25X implementation */
const NVS_FxnTable NVSSPI25X_fxnTable = {
    NVSSPI25X_close,
    NVSSPI25X_control,
    NVSSPI25X_erase,
    NVSSPI25X_getAttrs,
    NVSSPI25X_init,
    NVSSPI25X_lock,
    NVSSPI25X_open,
    NVSSPI25X_read,
    NVSSPI25X_unlock,
    NVSSPI25X_write
};

/* Manage SPI indexes */
static SPI_Handle spiHandles[MAX_SPI_INDEX + 1];
static uint8_t spiHandleUsers[MAX_SPI_INDEX + 1];

/*
 * Currently active (protected within Semaphore_pend() block)
 * SPI handle, and CSN pin
 */
static SPI_Handle spiHandle;
static uint32_t spiCsnGpioIndex;

/*
 *  Semaphore to synchronize access to flash region.
 */
static SemaphoreP_Handle  writeSem;

/*
 *  ======== NVSSPI25X_close ========
 */
void NVSSPI25X_close(NVS_Handle handle)
{
    NVSSPI25X_HWAttrs const *hwAttrs;
    NVSSPI25X_Object *object;

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    /* Close the SPI if we opened it */
    if (hwAttrs->spiHandle == NULL) {
        spiHandleUsers[hwAttrs->spiIndex] -= 1;

        /* Close SPI if this is the last region that uses it */
        if (spiHandleUsers[hwAttrs->spiIndex] == 0) {
            /* Ensure part is responsive */
            extFlashWaitReady(handle);

            /* Put the part in low power mode */
            extFlashPowerDown(handle);

            SPI_close(object->spiHandle);
            spiHandles[hwAttrs->spiIndex] = NULL;
        }
    }

    NVSSPI25X_deinitSpiCs(handle, spiCsnGpioIndex);

    object->opened = false;

    SemaphoreP_post(writeSem);
}

/*
 *  ======== NVSSPI25X_control ========
 */
int_fast16_t NVSSPI25X_control(NVS_Handle handle, uint_fast16_t cmd, uintptr_t arg)
{
    NVSSPI25X_HWAttrs const *hwAttrs;
    NVSSPI25X_Object *object;

    if (cmd != NVSSPI25X_CMD_MASS_ERASE) return (NVS_STATUS_UNDEFINEDCMD);

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* Set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    return (extFlashMassErase(handle));
}

/*
 *  ======== NVSSPI25X_erase ========
 */
int_fast16_t NVSSPI25X_erase(NVS_Handle handle, size_t offset, size_t size)
{
    int_fast16_t status;

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    status = doErase(handle, offset, size);

    SemaphoreP_post(writeSem);

    return (status);
}

/*
 *  ======== NVSSPI25X_getAttrs ========
 */
void NVSSPI25X_getAttrs(NVS_Handle handle, NVS_Attrs *attrs)
{
    NVSSPI25X_HWAttrs const *hwAttrs;

    hwAttrs = handle->hwAttrs;

    /* FlashSectorSizeGet() returns the size of a flash sector in bytes. */
    attrs->regionBase  = NVS_REGION_NOT_ADDRESSABLE;
    attrs->regionSize  = hwAttrs->regionSize;
    attrs->sectorSize  = hwAttrs->sectorSize;
}

/*
 *  ======== NVSSPI25X_init ========
 */
void NVSSPI25X_init()
{
    unsigned int key;
    SemaphoreP_Handle tempSem;

    SPI_init();

    /* Speculatively create semaphore so critical section is faster */
    tempSem = SemaphoreP_createBinary(1);
    /* tempSem == NULL will be detected in 'open' */

    key = HwiP_disable();

    if (writeSem == NULL) {
        /* First time init, assign handle */
        writeSem = tempSem;

        HwiP_restore(key);
    }
    else {
        /* Init already called */
        HwiP_restore(key);

        /* Delete unused Semaphores */
        if (tempSem) {
            SemaphoreP_delete(tempSem);
        }
    }
}

/*
 *  ======== NVSSPI25X_lock =======
 */
int_fast16_t NVSSPI25X_lock(NVS_Handle handle, uint32_t timeout)
{
    if (SemaphoreP_pend(writeSem, timeout) != SemaphoreP_OK) {
        return (NVS_STATUS_TIMEOUT);
    }
    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== NVSSPI25X_open =======
 */
NVS_Handle NVSSPI25X_open(uint_least8_t index, NVS_Params *params)
{
    NVSSPI25X_Object *object;
    NVSSPI25X_HWAttrs const *hwAttrs;
    size_t sectorSize;
    NVS_Handle handle;
    SPI_Params spiParams;

    /* Confirm that 'init' has successfully completed */
    if (writeSem == NULL) {
        NVSSPI25X_init();
        if (writeSem == NULL) {
            return (NULL);
        }
    }

    /* Verify NVS region index */
    if (index >= NVS_count) {
        return (NULL);
    }

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    handle = &NVS_config[index];
    object = NVS_config[index].object;
    hwAttrs = NVS_config[index].hwAttrs;

    if (object->opened == true) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    sectorSize = hwAttrs->sectorSize;
    object->sectorBaseMask = ~(sectorSize - 1);

    /* The regionBase must be aligned on a flash page boundary */
    if ((hwAttrs->regionBaseOffset) & (sectorSize - 1)) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    /* The region cannot be smaller than a sector size */
    if (hwAttrs->regionSize < sectorSize) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    /* The region size must be a multiple of sector size */
    if (hwAttrs->regionSize != (hwAttrs->regionSize & object->sectorBaseMask)) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    if (hwAttrs->spiHandle) {
        /* Use the provided SPI Handle */
        object->spiHandle = *hwAttrs->spiHandle;
    }
    else {
        if (hwAttrs->spiIndex > MAX_SPI_INDEX) {
            SemaphoreP_post(writeSem);
            return (NULL);
        }
        /* Open SPI if this driver hasn't already opened this SPI instance */
        if (spiHandles[hwAttrs->spiIndex] == NULL) {
            SPI_Handle spi;

            SPI_Params_init(&spiParams);
            spiParams.bitRate = hwAttrs->spiBitRate;
            spiParams.mode = SPI_MASTER;
            spiParams.transferMode = SPI_MODE_BLOCKING;

            /* Attempt to open SPI. */
            spi = SPI_open(hwAttrs->spiIndex, &spiParams);

            if (spi == NULL) {
                SemaphoreP_post(writeSem);
                return (NULL);
            }

            spiHandles[hwAttrs->spiIndex] = spi;
        }
        object->spiHandle = spiHandles[hwAttrs->spiIndex];
        /* Keep track of how many regions use the same SPI handle */
        spiHandleUsers[hwAttrs->spiIndex] += 1;
    }

    /* Set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;


    /* Initialize chip select output */
    NVSSPI25X_initSpiCs(handle, spiCsnGpioIndex);

    object->opened = true;

    /* Put the part in standby mode */
    extFlashPowerStandby(handle);

    SemaphoreP_post(writeSem);

    return (handle);
}

/*
 *  ======== NVSSPI25X_read =======
 */
int_fast16_t NVSSPI25X_read(NVS_Handle handle, size_t offset, void *buffer,
        size_t bufferSize)
{
    NVSSPI25X_HWAttrs const *hwAttrs;
    int retval = NVS_STATUS_SUCCESS;

    hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /*
     *  Get exclusive access to the region.  We don't want someone
     *  else to erase the region while we are reading it.
     */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    retval = doRead(handle, offset, buffer, bufferSize);

    SemaphoreP_post(writeSem);

    return (retval);
}

/*
 *  ======== NVSSPI25X_unlock =======
 */
void NVSSPI25X_unlock(NVS_Handle handle)
{
    SemaphoreP_post(writeSem);
}

/*
 *  ======== NVSSPI25X_write =======
 */
int_fast16_t NVSSPI25X_write(NVS_Handle handle, size_t offset, void *buffer,
                      size_t bufferSize, uint_fast16_t flags)
{
    NVSSPI25X_Object *object;
    NVSSPI25X_HWAttrs const *hwAttrs;
    size_t length, foffset;
    uint32_t status = true;
    uint8_t *srcBuf;
    int retval = NVS_STATUS_SUCCESS;
    uint8_t wbuf[4];

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /* Get exclusive access to the Flash region */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    /* Set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    /* If erase is set, erase destination sector(s) first */
    if (flags & NVS_WRITE_ERASE) {
        length = bufferSize & object->sectorBaseMask;
        if (bufferSize & (~object->sectorBaseMask)) {
            length += hwAttrs->sectorSize;
        }

        retval = doErase(handle, offset & object->sectorBaseMask, length);
        if (retval != NVS_STATUS_SUCCESS) {
            SemaphoreP_post(writeSem);
            return (retval);
        }
    }
    else if (flags & NVS_WRITE_PRE_VERIFY) {
        if ((hwAttrs->verifyBuf == NULL) || (hwAttrs->verifyBufSize == 0)) {
            SemaphoreP_post(writeSem);
            return (NVS_STATUS_ERROR);
        }

        retval = doWriteVerify(handle, offset, buffer, bufferSize,
                     hwAttrs->verifyBuf, hwAttrs->verifyBufSize, true);

        if (retval != NVS_STATUS_SUCCESS) {
            SemaphoreP_post(writeSem);
            return (retval);
        }
    }

    srcBuf = buffer;
    length = bufferSize;
    foffset = (size_t)hwAttrs->regionBaseOffset + offset;

    while (length > 0)
    {
        size_t ilen; /* Interim length per instruction */

        /* Wait till previous erase/program operation completes */
        int ret = extFlashWaitReady(handle);

        if (ret) {
            status = false;
            break;
        }

        ret = extFlashWriteEnable(handle);

        if (ret) {
            status = false;
            break;
        }

        ilen = SPIFLASH_PROGRAM_PAGE_SIZE - (foffset % SPIFLASH_PROGRAM_PAGE_SIZE);
        if (length < ilen) {
            ilen = length;
        }

        wbuf[0] = SPIFLASH_WRITE;
        wbuf[1] = (foffset >> 16) & 0xff;
        wbuf[2] = (foffset >> 8) & 0xff;
        wbuf[3] = foffset & 0xff;

        foffset += ilen;
        length -= ilen;

        /*
         * Up to 100ns CS hold time (which is not clear
         * whether it's application only in between reads)
         * is not imposed here since above instructions
         * should be enough to delay
         * as much.
         */
        NVSSPI25X_assertSpiCs(handle, spiCsnGpioIndex);

        if (extFlashSpiWrite(wbuf, sizeof(wbuf)) != NVS_STATUS_SUCCESS) {
            status = false;
            break;
        }

        if (extFlashSpiWrite(srcBuf, ilen) != NVS_STATUS_SUCCESS) {
            status = false;
            break;
        }

        srcBuf += ilen;
        NVSSPI25X_deassertSpiCs(handle, spiCsnGpioIndex);
    }

    if (status == false) {
        retval = NVS_STATUS_ERROR;
    }
    else if (flags & NVS_WRITE_POST_VERIFY) {
        if ((hwAttrs->verifyBuf == NULL) || (hwAttrs->verifyBufSize == 0)) {
            SemaphoreP_post(writeSem);
            return (NVS_STATUS_ERROR);
        }

        retval = doWriteVerify(handle, offset, buffer, bufferSize,
                     hwAttrs->verifyBuf, hwAttrs->verifyBufSize, false);
    }

    SemaphoreP_post(writeSem);

    return (retval);
}

/*
 *  ======== doWriteVerify =======
 */
static int_fast16_t doWriteVerify(NVS_Handle handle, size_t offset, void *src,
           size_t srcBufSize, void *dst, size_t dstBufSize, bool preFlag)
{
    size_t i, j;
    uint8_t *srcBuf, *dstBuf;
    bool bad;
    int_fast16_t retval;

    srcBuf = src;
    dstBuf = dst;

    j = dstBufSize;

    for (i = 0; i < srcBufSize; i++, j++) {
        if (j == dstBufSize) {
            retval = doRead(handle, offset + i, dstBuf, j);
            if (retval != NVS_STATUS_SUCCESS) {
                break;
            }
            j = 0;
        }
        if (preFlag) {
            bad = srcBuf[i] != (srcBuf[i] & dstBuf[j]);
        }
        else {
            bad = srcBuf[i] != dstBuf[j];
        }
        if (bad) return (NVS_STATUS_INV_WRITE);
    }
    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== checkEraseRange ========
 */
static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset, size_t size)
{
    NVSSPI25X_Object   *object;
    NVSSPI25X_HWAttrs const *hwAttrs;

    object = handle->object;
    hwAttrs = handle->hwAttrs;

    if (offset != (offset & object->sectorBaseMask)) {
        return (NVS_STATUS_INV_ALIGNMENT);    /* Poorly aligned start address */
    }

    if (offset >= hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);   /* Offset is past end of region */
    }

    if (offset + size > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_SIZE);     /* Size is too big */
    }

    if (size != (size & object->sectorBaseMask)) {
        return (NVS_STATUS_INV_SIZE);     /* Size is not a multiple of sector size */
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== doErase ========
 */
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size)
{
    NVSSPI25X_HWAttrs const *hwAttrs;
    NVSSPI25X_Object *object;
    uint32_t sectorBase;
    size_t eraseSize;
    int_fast16_t rangeStatus;
    uint8_t wbuf[4];

    /* Sanity test the erase args */
    rangeStatus = checkEraseRange(handle, offset, size);

    if (rangeStatus != NVS_STATUS_SUCCESS) {
        return (rangeStatus);
    }

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* Set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    /* Start erase at this address */
    sectorBase = (uint32_t)hwAttrs->regionBaseOffset + offset;

    while (size) {
        /* Wait till previous erase/program operation completes */
        int ret = extFlashWaitReady(handle);
        if (ret) {
            return (NVS_STATUS_ERROR);
        }

        ret = extFlashWriteEnable(handle);
        if (ret) {
            return (NVS_STATUS_ERROR);
        }


        /* Determine which erase command to use */
        if (size >= SPIFLASH_SECTOR_SIZE &&
            ((sectorBase & (SPIFLASH_SECTOR_SIZE - 1)) == 0)){
            /* Erase size is one sector (64kB) */
            eraseSize = SPIFLASH_SECTOR_SIZE;
            wbuf[0] = SPIFLASH_SECTOR_ERASE;
        }
        else{
            /* Erase size is one sub-sector (4kB)*/
            eraseSize = hwAttrs->sectorSize;
            wbuf[0] = SPIFLASH_SUBSECTOR_ERASE;
        }


        /* Format command to send over SPI */
        wbuf[1] = (sectorBase >> 16) & 0xff;
        wbuf[2] = (sectorBase >> 8) & 0xff;
        wbuf[3] = sectorBase & 0xff;

        /* Send erase command to external flash */
        NVSSPI25X_assertSpiCs(handle, spiCsnGpioIndex);
        if (extFlashSpiWrite(wbuf, sizeof(wbuf))) {
            NVSSPI25X_deassertSpiCs(handle, spiCsnGpioIndex);
            return (NVS_STATUS_ERROR);
        }
        NVSSPI25X_deassertSpiCs(handle, spiCsnGpioIndex);

        sectorBase += eraseSize;
        size -= eraseSize;
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== doRead =======
 */
static int_fast16_t doRead(NVS_Handle handle, size_t offset, void *buffer,
        size_t bufferSize)
{
    NVSSPI25X_Object *object;
    NVSSPI25X_HWAttrs const *hwAttrs;
    size_t loffset;
    uint8_t wbuf[4];
    int retval = NVS_STATUS_SUCCESS;

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* Set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    loffset = offset + hwAttrs->regionBaseOffset;

    /* Wait till previous erase/program operation completes */
    retval = extFlashWaitReady(handle);
    if (retval) {
        return (retval);
    }

    /*
     * SPI is driven with very low frequency (1MHz < 33MHz fR spec)
     * in this temporary implementation.
     * and hence it is not necessary to use fast read.
     */
    wbuf[0] = SPIFLASH_READ;
    wbuf[1] = (loffset >> 16) & 0xff;
    wbuf[2] = (loffset >> 8) & 0xff;
    wbuf[3] = loffset & 0xff;

    NVSSPI25X_assertSpiCs(handle, spiCsnGpioIndex);

    if (extFlashSpiWrite(wbuf, sizeof(wbuf))) {
        NVSSPI25X_deassertSpiCs(handle, spiCsnGpioIndex);
        return (NVS_STATUS_ERROR);
    }

    retval = extFlashSpiRead(buffer, bufferSize);

    NVSSPI25X_deassertSpiCs(handle, spiCsnGpioIndex);

    return (retval);
}

/*
 *  ======== extFlashPowerDown =======
 *  Issue power down command
 */
static int_fast16_t extFlashPowerDown(NVS_Handle nvsHandle)
{
    uint8_t cmd;
    int_fast16_t status;

    cmd = SPIFLASH_DP;
    NVSSPI25X_assertSpiCs(nvsHandle, spiCsnGpioIndex);
    status = extFlashSpiWrite(&cmd,sizeof(cmd));
    NVSSPI25X_deassertSpiCs(nvsHandle, spiCsnGpioIndex);

    return (status);
}

/*
 *  ======== extFlashPowerStandby =======
 *  Issue standby command
 */
static int_fast16_t extFlashPowerStandby(NVS_Handle nvsHandle)
{
    uint8_t cmd;
    int_fast16_t status;

    cmd = SPIFLASH_RDP;
    NVSSPI25X_assertSpiCs(nvsHandle, spiCsnGpioIndex);
    status = extFlashSpiWrite(&cmd, sizeof(cmd));
    NVSSPI25X_deassertSpiCs(nvsHandle, spiCsnGpioIndex);

    if (status == NVS_STATUS_SUCCESS) {
        status = extFlashWaitReady(nvsHandle);
    }

    return (status);
}

/*
 *  ======== extFlashMassErase =======
 *  Issue mass erase command
 */
static int_fast16_t extFlashMassErase(NVS_Handle nvsHandle)
{
    uint8_t cmd;
    int_fast16_t status;

    /* Wait for previous operation to complete */
    if (extFlashWaitReady(nvsHandle)) {
        return (NVS_STATUS_ERROR);
    }

    cmd = SPIFLASH_MASS_ERASE;
    NVSSPI25X_assertSpiCs(nvsHandle, spiCsnGpioIndex);
    status = extFlashSpiWrite(&cmd,sizeof(cmd));
    NVSSPI25X_deassertSpiCs(nvsHandle, spiCsnGpioIndex);

    if (status != NVS_STATUS_SUCCESS) {
        return (status);
    }

    /* Wait for mass erase to complete */
    return (extFlashWaitReady(nvsHandle));
}

/*
 *  ======== extFlashWaitReady =======
 *  Wait for any previous job to complete.
 */
static int_fast16_t extFlashWaitReady(NVS_Handle nvsHandle)
{
    const uint8_t wbuf[1] = { SPIFLASH_READ_STATUS };
    int_fast16_t ret;
    uint8_t buf;

    NVSSPI25X_HWAttrs const *hwAttrs;
    hwAttrs = nvsHandle->hwAttrs;

    for (;;) {
        NVSSPI25X_assertSpiCs(nvsHandle, spiCsnGpioIndex);
        extFlashSpiWrite(wbuf, sizeof(wbuf));
        ret = extFlashSpiRead(&buf,sizeof(buf));
        NVSSPI25X_deassertSpiCs(nvsHandle, spiCsnGpioIndex);

        if (ret != NVS_STATUS_SUCCESS) {
            /* Error */
            return (ret);
        }
        if (!(buf & SPIFLASH_STATUS_BIT_BUSY)) {
            /* Now ready */
            break;
        }
        if (hwAttrs->statusPollDelayUs){
            /* Sleep to avoid excessive polling and starvation */
            ClockP_usleep(hwAttrs->statusPollDelayUs);
        }
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== extFlashWriteEnable =======
 *  Issue SPIFLASH_WRITE_ENABLE command
 */
static int_fast16_t extFlashWriteEnable(NVS_Handle nvsHandle)
{
    const uint8_t wbuf[] = { SPIFLASH_WRITE_ENABLE };
    int_fast16_t ret;

    NVSSPI25X_assertSpiCs(nvsHandle, spiCsnGpioIndex);
    ret = extFlashSpiWrite(wbuf,sizeof(wbuf));
    NVSSPI25X_deassertSpiCs(nvsHandle, spiCsnGpioIndex);

    return (ret);
}

/*
 *  ======== extFlashSpiWrite =======
 */
static int_fast16_t extFlashSpiWrite(const uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.rxBuf  = NULL;

    /*
     * Work around SPI transfer from address 0x0
     * transfer first byte from local buffer
     */
    if (buf == NULL) {
        uint8_t byte0;
        byte0 = *buf++;
        masterTransaction.count  = 1;
        masterTransaction.txBuf  = (void*)&byte0;
        if (!SPI_transfer(spiHandle, &masterTransaction)) {
            return (NVS_STATUS_ERROR);
        }
        len = len - 1;
        if (len == 0) {
            return (NVS_STATUS_SUCCESS);
        }
    }

    masterTransaction.count  = len;
    masterTransaction.txBuf  = (void*)buf;

    return (SPI_transfer(spiHandle, &masterTransaction) ? NVS_STATUS_SUCCESS : NVS_STATUS_ERROR);
}


/*
 *  ======== extFlashSpiRead =======
 */
static int_fast16_t extFlashSpiRead(uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.txBuf = NULL;

    /*
     * Work around SPI transfer to address 0x0
     * transfer first byte into local buffer
     */
    if (buf == NULL) {
        uint8_t byte0;
        masterTransaction.count  = 1;
        masterTransaction.rxBuf  = (void*)&byte0;
        if (!SPI_transfer(spiHandle, &masterTransaction)) {
            return (NVS_STATUS_ERROR);
        }
        *buf++ = byte0;
        len = len - 1;
        if (len == 0) {
            return (NVS_STATUS_SUCCESS);
        }
    }

    masterTransaction.count = len;
    masterTransaction.rxBuf = buf;

    return (SPI_transfer(spiHandle, &masterTransaction) ? NVS_STATUS_SUCCESS : NVS_STATUS_ERROR);
}

/*
 * Below are the default (weak) GPIO-driver based implementations of:
 *     NVSSPI25X_initSpiCs()
 *     NVSSPI25X_deinitSpiCs()
 *     NVSSPI25X_assertSpiCs()
 *     NVSSPI25X_deassertSpiCs()
 */

/*
 *  ======== NVSSPI25X_initSpiCs =======
 */
#if defined(__IAR_SYSTEMS_ICC__)
__weak void NVSSPI25X_initSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#elif defined(__GNUC__) && !defined(__ti__)
void __attribute__((weak)) NVSSPI25X_initSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#else
#pragma WEAK (NVSSPI25X_initSpiCs)
void NVSSPI25X_initSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#endif
{
    if (csId != NVSSPI25X_SPI_MANAGES_CS) {
        GPIO_init();

        /*
        * Make SPI Chip Select GPIO an output, and set it high.
        * Since the same device may be used for multiple regions, configuring
        * the same Chip Select pin may be done multiple times. No harm done.
        */
        GPIO_setConfig(csId, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);
    }
}

/*
 *  ======== NVSSPI25X_deinitSpiCs =======
 */
#if defined(__IAR_SYSTEMS_ICC__)
__weak void NVSSPI25X_deinitSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#elif defined(__GNUC__) && !defined(__ti__)
void __attribute__((weak)) NVSSPI25X_deinitSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#else
#pragma WEAK (NVSSPI25X_deinitSpiCs)
void NVSSPI25X_deinitSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#endif
{
}

/*
 *  ======== NVSSPI25X_assertSpiCs =======
 *  Assert SPI flash /CS
 */
#if defined(__IAR_SYSTEMS_ICC__)
__weak void NVSSPI25X_assertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#elif defined(__GNUC__) && !defined(__ti__)
void __attribute__((weak)) NVSSPI25X_assertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#else
#pragma WEAK (NVSSPI25X_assertSpiCs)
void NVSSPI25X_assertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#endif
{
    if (csId != NVSSPI25X_SPI_MANAGES_CS) {
        GPIO_write(csId, 0);
    }
}

/*
 *  ======== NVSSPI25X_deassertSpiCs =======
 *  De-assert SPI flash /CS
 */
#if defined(__IAR_SYSTEMS_ICC__)
__weak void NVSSPI25X_deassertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#elif defined(__GNUC__) && !defined(__ti__)
void __attribute__((weak)) NVSSPI25X_deassertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#else
#pragma WEAK (NVSSPI25X_deassertSpiCs)
void NVSSPI25X_deassertSpiCs(NVS_Handle nvsHandle, uint16_t csId)
#endif
{
    if (csId != NVSSPI25X_SPI_MANAGES_CS) {
        GPIO_write(csId, 1);
    }
}
