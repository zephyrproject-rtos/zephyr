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
 *  ======== NVSRAM.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSRAM.h>

static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset, size_t size);
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size);

extern NVS_Config NVS_config[];
extern const uint8_t NVS_count;

/* NVS function table for NVSRAM implementation */
const NVS_FxnTable NVSRAM_fxnTable = {
    NVSRAM_close,
    NVSRAM_control,
    NVSRAM_erase,
    NVSRAM_getAttrs,
    NVSRAM_init,
    NVSRAM_lock,
    NVSRAM_open,
    NVSRAM_read,
    NVSRAM_unlock,
    NVSRAM_write
};

/*
 *  Semaphore to synchronize access to the region.
 */
static SemaphoreP_Handle  writeSem;

/*
 *  ======== NVSRAM_close ========
 */
void NVSRAM_close(NVS_Handle handle)
{
    ((NVSRAM_Object *) handle->object)->isOpen = false;
}

/*
 *  ======== NVSRAM_control ========
 */
int_fast16_t NVSRAM_control(NVS_Handle handle, uint_fast16_t cmd,
    uintptr_t arg)
{
    return (NVS_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== NVSRAM_erase ========
 */
int_fast16_t NVSRAM_erase(NVS_Handle handle, size_t offset, size_t size)
{
    int_fast16_t status;

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    status = doErase(handle, offset, size);

    SemaphoreP_post(writeSem);

    return (status);
}

/*
 *  ======== NVSRAM_getAttrs ========
 */
void NVSRAM_getAttrs(NVS_Handle handle, NVS_Attrs *attrs)
{
    NVSRAM_HWAttrs const *hwAttrs = handle->hwAttrs;

    attrs->regionBase = hwAttrs->regionBase;
    attrs->regionSize = hwAttrs->regionSize;
    attrs->sectorSize = hwAttrs->sectorSize;
}

/*
 *  ======== NVSRAM_init ========
 */
void NVSRAM_init()
{
    uintptr_t         key;
    SemaphoreP_Handle sem;

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
        if (sem) {
            SemaphoreP_delete(sem);
        }
    }
}

/*
 *  ======== NVSRAM_lock =======
 */
int_fast16_t NVSRAM_lock(NVS_Handle handle, uint32_t timeout)
{
    if (SemaphoreP_pend(writeSem, timeout) != SemaphoreP_OK) {
        return (NVS_STATUS_TIMEOUT);
    }
    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== NVSRAM_open =======
 */
NVS_Handle NVSRAM_open(uint_least8_t index, NVS_Params *params)
{
    NVS_Handle            handle;
    NVSRAM_Object        *object;
    NVSRAM_HWAttrs const *hwAttrs;

    /* Confirm that 'init' has successfully completed */
    if (writeSem == NULL) {
        NVSRAM_init();
        if (writeSem == NULL) {
            return (NULL);
        }
    }

    /* verify NVS region index */
    if (index >= NVS_count) {
        return (NULL);
    }

    handle = &NVS_config[index];
    object = NVS_config[index].object;
    hwAttrs = NVS_config[index].hwAttrs;

    /* for efficient argument checking */
    object->sectorBaseMask = ~(hwAttrs->sectorSize - 1);

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    if (object->isOpen) {
        SemaphoreP_post(writeSem);

        return (NULL);
    }

    /* The regionBase must be aligned on a page boundary */
    if ((size_t) (hwAttrs->regionBase) & (hwAttrs->sectorSize - 1)) {
        SemaphoreP_post(writeSem);

        return (NULL);
    }

    /* The region cannot be smaller than a sector size */
    if (hwAttrs->regionSize < hwAttrs->sectorSize) {
        SemaphoreP_post(writeSem);

        return (NULL);
    }

    /* The region size must be a multiple of sector size */
    if (hwAttrs->regionSize !=
        (hwAttrs->regionSize & object->sectorBaseMask)) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    object->isOpen = true;

    SemaphoreP_post(writeSem);

    return (handle);
}

/*
 *  ======== NVSRAM_read =======
 */
int_fast16_t NVSRAM_read(NVS_Handle handle, size_t offset, void *buffer,
    size_t bufferSize)
{
    NVSRAM_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /*
     *  Get exclusive access to the region.  We don't want someone
     *  else to erase the region while we are reading it.
     */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    memcpy(buffer, (char *)(hwAttrs->regionBase) + offset, bufferSize);

    SemaphoreP_post(writeSem);

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== NVSRAM_unlock =======
 */
void NVSRAM_unlock(NVS_Handle handle)
{
    SemaphoreP_post(writeSem);
}

/*
 *  ======== NVSRAM_write =======
 */
int_fast16_t NVSRAM_write(NVS_Handle handle, size_t offset, void *buffer,
    size_t bufferSize, uint_fast16_t flags)
{
    size_t                i;
    uint8_t              *dstBuf;
    uint8_t              *srcBuf;
    int_fast16_t          result;
    size_t                size;
    NVSRAM_Object        *object = handle->object;
    NVSRAM_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /* Get exclusive access to the Flash region */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    /* If erase is set, erase destination sector(s) first */
    if (flags & NVS_WRITE_ERASE) {
        size = bufferSize & object->sectorBaseMask;
        if (bufferSize & (~object->sectorBaseMask)) {
            size += hwAttrs->sectorSize;
        }

        result = doErase(handle, offset & object->sectorBaseMask, size);
        if (result != NVS_STATUS_SUCCESS) {
            SemaphoreP_post(writeSem);

            return (result);
        }
    }
    else if (flags & NVS_WRITE_PRE_VERIFY) {
        /*
         *  If pre-verify, each destination byte must be able to be changed to the
         *  source byte (1s to 0s, not 0s to 1s).
         *  this is satisfied by the following test:
         *     src == (src & dst)
         */
        dstBuf = (uint8_t *)((uint32_t)(hwAttrs->regionBase) + offset);
        srcBuf = buffer;
        for (i = 0; i < bufferSize; i++) {
            if (srcBuf[i] != (srcBuf[i] & dstBuf[i])) {
                SemaphoreP_post(writeSem);
                return (NVS_STATUS_INV_WRITE);
            }
        }
    }

    dstBuf = (uint8_t *)((uint32_t)(hwAttrs->regionBase) + offset);
    srcBuf = buffer;
    memcpy((void *) dstBuf, (void *) srcBuf, bufferSize);

    SemaphoreP_post(writeSem);

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== checkEraseRange ========
 */
static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset, size_t size)
{
    NVSRAM_Object        *object = handle->object;
    NVSRAM_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (offset != (offset & object->sectorBaseMask)) {
        /* poorly aligned start address */
        return (NVS_STATUS_INV_ALIGNMENT);
    }

    if (offset >= hwAttrs->regionSize) {
        /* offset is past end of region */
        return (NVS_STATUS_INV_OFFSET);
    }

    if (offset + size > hwAttrs->regionSize) {
        /* size is too big */
        return (NVS_STATUS_INV_SIZE);
    }

    if (size != (size & object->sectorBaseMask)) {
        /* size is not a multiple of sector size */
        return (NVS_STATUS_INV_SIZE);
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== doErase ========
 */
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size)
{
    void *                sectorBase;
    int_fast16_t          rangeStatus;
    NVSRAM_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* sanity test the erase args */
    rangeStatus = checkEraseRange(handle, offset, size);
    if (rangeStatus != NVS_STATUS_SUCCESS) {
        return (rangeStatus);
    }

    sectorBase = (void *) ((uint32_t) hwAttrs->regionBase + offset);

    memset(sectorBase, 0xFF, size);

    return (NVS_STATUS_SUCCESS);
}
