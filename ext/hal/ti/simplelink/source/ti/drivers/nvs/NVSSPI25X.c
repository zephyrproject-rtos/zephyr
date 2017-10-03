/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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

/* highest supported SPI instance index */
#define MAX_SPI_INDEX               3

static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset, size_t size);
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size);
static int_fast16_t doRead(NVS_Handle handle, size_t offset, void *buffer,
                        size_t bufferSize);
static int_fast16_t doWriteVerify(NVS_Handle handle, size_t offset,
                        void *src, size_t srcBufSize, void *dst,
                        size_t dstBufSize, bool preFlag);

static int_fast16_t extFlashSpiWrite(const uint8_t *buf, size_t len);
static int_fast16_t extFlashSpiRead(uint8_t *buf, size_t len);
static int_fast16_t extFlashPowerDown(void);
static int_fast16_t extFlashPowerStandby(void);
static int_fast16_t extFlashWaitReady(void);
static int_fast16_t extFlashWriteEnable(void);
static int_fast16_t extFlashMassErase(void);
static void extFlashSelect(void);
static void extFlashDeselect(void);

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

/* manage SPI indexes */
static SPI_Handle spiHandles[MAX_SPI_INDEX + 1];
static uint8_t spiHandleUsers[MAX_SPI_INDEX + 1];

/*
 * currently active (protected within Semaphore_pend() block)
 * SPI handle and CSN pin
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

    /* close the SPI if we opened it */
    if (hwAttrs->spiHandle == NULL) {
        spiHandleUsers[hwAttrs->spiIndex] -= 1;

        /* close SPI if this is the last region that uses it */
        if (spiHandleUsers[hwAttrs->spiIndex] == 0) {
            // Put the part in low power mode
            extFlashPowerDown();

            SPI_close(object->spiHandle);
            spiHandles[hwAttrs->spiIndex] = NULL;
        }
    }

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

    /* set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    return (extFlashMassErase());
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
    SemaphoreP_Handle sem;

    GPIO_init();
    SPI_init();

    /* speculatively create a binary semaphore for thread safety */
    sem = SemaphoreP_createBinary(1);
    /* sem == NULL will be detected in 'open' */

    key = HwiP_disable();

    if (writeSem == NULL) {
        /* use the binary sem created above */
        writeSem = sem;
        HwiP_restore(key);
    }
    else {
        /* init already called */
        HwiP_restore(key);
        /* delete unused Semaphore */
        if (sem) SemaphoreP_delete(sem);
    }
}

/*
 *  ======== NVSSPI25X_lock =======
 */
int_fast16_t NVSSPI25X_lock(NVS_Handle handle, uint32_t timeout)
{
    switch (SemaphoreP_pend(writeSem, timeout)) {
        case SemaphoreP_OK:
            return (NVS_STATUS_SUCCESS);

        case SemaphoreP_TIMEOUT:
            return (NVS_STATUS_TIMEOUT);

        case SemaphoreP_FAILURE:
        default:
            return (NVS_STATUS_ERROR);
    }
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

    /* verify NVS region index */
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

    /* The regionBase must be aligned on a flaah page boundary */
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
        /* use the provided SPI Handle */
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
        /* keep track of how many regions use the same SPI handle */
        spiHandleUsers[hwAttrs->spiIndex] += 1;
    }

    /* set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    /*
     * Make SPI Chip Select GPIO an output, and set it high.
     * Since the same device may be used for multiple regions, configuring
     * the same Chip Select pin may be done multiple times. No harm done.
     */
    GPIO_setConfig(hwAttrs->spiCsnGpioIndex, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);

    object->opened = true;

    /* Put the part in standby mode */
    extFlashPowerStandby();

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
    size_t xferSize;
    uint8_t *dstBuf;
    int retval = NVS_STATUS_SUCCESS;

    hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    dstBuf = buffer;

    /*
     *  Get exclusive access to the region.  We don't want someone
     *  else to erase the region while we are reading it.
     */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    /*
     * break read down into 1024 byte pieces to workaround TIDRIVERS-1173
     */
    while (bufferSize) {
        if (bufferSize <= 1024) {
            xferSize = bufferSize;
        }
        else {
            xferSize = 1024;
        }

        retval = doRead(handle, offset, dstBuf, xferSize);

        if (retval != NVS_STATUS_SUCCESS) {
            break;
        }
        else {
            offset += xferSize;
            dstBuf += xferSize;
            bufferSize -= xferSize;
        }
    }

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

    /* set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    /* If erase is set, erase destination sector(s) first */
    if (flags & NVS_WRITE_ERASE) {
        retval = doErase(handle, offset & object->sectorBaseMask,
                     (bufferSize + hwAttrs->sectorSize) & object->sectorBaseMask);
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
        size_t ilen; /* interim length per instruction */

        /* Wait till previous erase/program operation completes */
        int ret = extFlashWaitReady();

        if (ret) {
            status = false;
            break;
        }

        ret = extFlashWriteEnable();

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
        extFlashSelect();

        if (extFlashSpiWrite(wbuf, sizeof(wbuf)) != NVS_STATUS_SUCCESS) {
            status = false;
            break;
        }

        if (extFlashSpiWrite(srcBuf, ilen) != NVS_STATUS_SUCCESS) {
            status = false;
            break;
        }

        srcBuf += ilen;
        extFlashDeselect();
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
        return (NVS_STATUS_INV_ALIGNMENT);    /* poorly aligned start address */
    }

    if (offset >= hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);   /* offset is past end of region */
    }

    if (offset + size > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_SIZE);     /* size is too big */
    }

    if (size != (size & object->sectorBaseMask)) {
        return (NVS_STATUS_INV_SIZE);     /* size is not a multiple of sector size */
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
    size_t sectorSize;
    int_fast16_t rangeStatus;
    uint8_t wbuf[4];

    /* sanity test the erase args */
    rangeStatus = checkEraseRange(handle, offset, size);

    if (rangeStatus != NVS_STATUS_SUCCESS) {
        return (rangeStatus);
    }

    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    sectorBase = (uint32_t)hwAttrs->regionBaseOffset + offset;
    sectorSize = hwAttrs->sectorSize;

    /* assume that 4k sectors use SS ERASE, else Sector Erase */
    if (sectorSize == 4096) {
        wbuf[0] = SPIFLASH_SUBSECTOR_ERASE;
    }
    else {
        wbuf[0] = SPIFLASH_SECTOR_ERASE;
    }

    while (size) {
        /* Wait till previous erase/program operation completes */
        int ret = extFlashWaitReady();
        if (ret) {
            return (NVS_STATUS_ERROR);
        }

        ret = extFlashWriteEnable();
        if (ret) {
            return (NVS_STATUS_ERROR);
        }

        wbuf[1] = (sectorBase >> 16) & 0xff;
        wbuf[2] = (sectorBase >> 8) & 0xff;
        wbuf[3] = sectorBase & 0xff;

        extFlashSelect();

        if (extFlashSpiWrite(wbuf, sizeof(wbuf))) {
            /* failure */
            extFlashDeselect();
            return (NVS_STATUS_ERROR);
        }
        extFlashDeselect();

        sectorBase += sectorSize;
        size -= sectorSize;
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

    /* set protected global variables */
    spiHandle = object->spiHandle;
    spiCsnGpioIndex = hwAttrs->spiCsnGpioIndex;

    loffset = offset + hwAttrs->regionBaseOffset;

    /* Wait till previous erase/program operation completes */
    retval = extFlashWaitReady();
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

    extFlashSelect();

    if (extFlashSpiWrite(wbuf, sizeof(wbuf))) {
        /* failure */
        extFlashDeselect();
        return (NVS_STATUS_ERROR);
    }

    retval = extFlashSpiRead(buffer, bufferSize);

    extFlashDeselect();

    return (retval);
}

/*
 *  ======== extFlashSelect =======
 *  Assert SPI flash /CS
 */
static void extFlashSelect(void)
{
    GPIO_write(spiCsnGpioIndex, 0);
}

/*
 *  ======== extFlashDeselect =======
 *  De-assert SPI flash /CS
 */
static void extFlashDeselect(void)
{
    GPIO_write(spiCsnGpioIndex, 1);
}

/*
 *  ======== extFlashPowerDown =======
 *  Issue power down command
 */
static int_fast16_t extFlashPowerDown(void)
{
    uint8_t cmd;
    int_fast16_t status;

    cmd = SPIFLASH_DP;
    extFlashSelect();
    status = extFlashSpiWrite(&cmd,sizeof(cmd));
    extFlashDeselect();

    return (status);
}

/*
 *  ======== extFlashPowerStandby =======
 *  Issue standby command
 */
static int_fast16_t extFlashPowerStandby(void)
{
    uint8_t cmd;
    int_fast16_t status;

    cmd = SPIFLASH_RDP;
    extFlashSelect();
    status = extFlashSpiWrite(&cmd, sizeof(cmd));
    extFlashDeselect();

    if (status == NVS_STATUS_SUCCESS) {
        status = extFlashWaitReady();
    }

    return (status);
}

/*
 *  ======== extFlashMassErase =======
 *  Issue mass erase command
 */
static int_fast16_t extFlashMassErase(void)
{
    uint8_t cmd;
    int_fast16_t status;

    /* wait for previous operation to complete */
    if (extFlashWaitReady()) {
        return (NVS_STATUS_ERROR);
    }

    cmd = SPIFLASH_MASS_ERASE;
    extFlashSelect();
    status = extFlashSpiWrite(&cmd,sizeof(cmd));
    extFlashDeselect();

    if (status != NVS_STATUS_SUCCESS) {
        return (status);
    }

    /* wait for mass erase to complete */
    return (extFlashWaitReady());
}

/*
 *  ======== extFlashWaitReady =======
 *  Wait for any previous job to complete.
 */
static int_fast16_t extFlashWaitReady(void)
{
    const uint8_t wbuf[1] = { SPIFLASH_READ_STATUS };
    int_fast16_t ret;
    uint8_t buf;

    for (;;) {
        extFlashSelect();
        extFlashSpiWrite(wbuf, sizeof(wbuf));
        ret = extFlashSpiRead(&buf,sizeof(buf));
        extFlashDeselect();

        if (ret != NVS_STATUS_SUCCESS) {
            /* Error */
            return (ret);
        }
        if (!(buf & SPIFLASH_STATUS_BIT_BUSY)) {
            /* Now ready */
            break;
        }
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== extFlashWriteEnable =======
 *  Issue SPIFLASH_WRITE_ENABLE command
 */
static int_fast16_t extFlashWriteEnable(void)
{
    const uint8_t wbuf[] = { SPIFLASH_WRITE_ENABLE };
    int_fast16_t ret;

    extFlashSelect();
    ret = extFlashSpiWrite(wbuf,sizeof(wbuf));
    extFlashDeselect();

    return (ret);
}

/*
 *  ======== extFlashSpiWrite =======
 */
static int_fast16_t extFlashSpiWrite(const uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.count  = len;
    masterTransaction.txBuf  = (void*)buf;
    masterTransaction.arg    = NULL;
    masterTransaction.rxBuf  = NULL;

    return (SPI_transfer(spiHandle, &masterTransaction) ? NVS_STATUS_SUCCESS : NVS_STATUS_ERROR);
}


/*
 *  ======== extFlashSpiRead =======
 */
static int_fast16_t extFlashSpiRead(uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.count = len;
    masterTransaction.txBuf = NULL;
    masterTransaction.arg = NULL;
    masterTransaction.rxBuf = buf;

    return (SPI_transfer(spiHandle, &masterTransaction) ? NVS_STATUS_SUCCESS : NVS_STATUS_ERROR);
}
