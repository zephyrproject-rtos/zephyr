/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
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
#include <stdint.h>
#include <stdlib.h>

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
#include <ti/drivers/SD.h>
#include <ti/drivers/SDFatFS.h>

#include <third_party/fatfs/ff.h>

/* SDFatFS Specific Defines */
#define DRIVE_NOT_MOUNTED    (~(0U))

extern const SDFatFS_Config SDFatFS_config[];
extern const uint_least8_t SDFatFS_count;

static bool isInitialized = false;

/*
 * Array of SDFatFS_Handles to determine the association of the
 * FatFs drive number with a SDFatFS_Handle.
 * _VOLUMES is defined in <third_party/fatfs/ffconf.h>.
 */
static SDFatFS_Handle sdFatFSHandles[_VOLUMES];

/* FatFS function prototypes */
DSTATUS SDFatFS_diskInitialize(BYTE drive);
DRESULT SDFatFS_diskIOctrl(BYTE drive, BYTE ctrl, void *buffer);
DRESULT SDFatFS_diskRead(BYTE drive, BYTE *buffer,
    DWORD sector, UINT secCount);
DSTATUS SDFatFS_diskStatus(BYTE drive);
DRESULT SDFatFS_diskWrite(BYTE drive, const BYTE *buffer,
    DWORD sector, UINT secCount);

/*
 *  ======== SDFatFS_close ========
 */
void SDFatFS_close(SDFatFS_Handle handle)
{
    TCHAR           path[3];
    DRESULT         dresult;
    FRESULT         fresult;
    SDFatFS_Object *obj = handle->object;

    /* Construct base directory path */
    path[0] = (TCHAR)'0' + obj->driveNum;
    path[1] = (TCHAR)':';
    path[2] = (TCHAR)'\0';

    /* Close the SD driver */
    SD_close(obj->sdHandle);

    /* Unmount the FatFs drive */
    fresult = f_mount(NULL, path, 0);
    if (fresult != FR_OK) {
        DebugP_log1("SDFatFS: Could not unmount FatFs volume @ drive"
            " number %d", obj->driveNum);
    }

    /* Unregister the disk_*() functions */
    dresult = disk_unregister(obj->driveNum);
    if (dresult != RES_OK) {
        DebugP_log1("SDFatFS: Error unregistering disk"
            " functions @ drive number %d", obj->driveNum);
    }

    obj->driveNum = DRIVE_NOT_MOUNTED;
    DebugP_log0("SDFatFS closed");
}

/*
 *  ======== SDFatFS_diskInitialize ========
 */
DSTATUS SDFatFS_diskInitialize(BYTE drive)
{
    int_fast8_t     result;
    SDFatFS_Object *obj = sdFatFSHandles[drive]->object;

    result = SD_initialize(obj->sdHandle);

    /* Convert lower level driver status code */
    if (result == SD_STATUS_SUCCESS) {
        obj->diskState = ((DSTATUS) obj->diskState) & ~((DSTATUS)STA_NOINIT);
    }

    return (obj->diskState);
}

/*
 *  ======== SDFatFS_diskIOctrl ========
 *  Function to perform specified disk operations. This function is called by the
 *  FatFs module and must not be called by the application!
 */
DRESULT SDFatFS_diskIOctrl(BYTE drive, BYTE ctrl, void *buffer)
{
    SDFatFS_Object *obj   = sdFatFSHandles[drive]->object;
    DRESULT         fatfsRes = RES_ERROR;

    switch (ctrl) {
        case (BYTE)GET_SECTOR_COUNT:
            *(uint32_t*)buffer = (uint32_t)SD_getNumSectors(obj->sdHandle);

            DebugP_log1("SDFatFS: Disk IO control: sector count: %d",
                *(uint32_t*)buffer);
            fatfsRes = RES_OK;
            break;

        case (BYTE)GET_SECTOR_SIZE:
            *(WORD*)buffer = (WORD)SD_getSectorSize(obj->sdHandle);
            DebugP_log1("SDFatFS: Disk IO control: sector size: %d",
                *(WORD*)buffer);
            fatfsRes = RES_OK;
            break;

        case CTRL_SYNC:
            fatfsRes = RES_OK;
            break;

        default:
            DebugP_log0("SDFatFS: Disk IO control parameter error");
            fatfsRes = RES_PARERR;
            break;
    }
    return (fatfsRes);
}

/*
 *  ======== SDFatFS_diskRead ========
 *  Function to perform a disk read from the SDCard. This function is called by
 *  the FatFs module and must not be called by the application!
 */
DRESULT SDFatFS_diskRead(BYTE drive, BYTE *buffer,
    DWORD sector, UINT secCount)
{
    int_fast32_t    result;
    DRESULT         fatfsRes = RES_ERROR;
    SDFatFS_Object *obj   = sdFatFSHandles[drive]->object;

    /* Return if disk not initialized */
    if ((obj->diskState & (DSTATUS)STA_NOINIT) != 0) {
        fatfsRes = RES_PARERR;
    }
    else {
        result = SD_read(obj->sdHandle, (uint_least8_t *)buffer,
            (int_least32_t)sector, (uint_least32_t)secCount);

        /* Convert lower level driver status code */
        if (result == SD_STATUS_SUCCESS) {
            fatfsRes = RES_OK;
        }
    }

    return (fatfsRes);
}

/*
 *  ======== SDFatFS_diskStatus ========
 *  Function to return the current disk status. This function is called by
 *  the FatFs module and must not be called by the application!
 */
DSTATUS SDFatFS_diskStatus(BYTE drive)
{
    return (((SDFatFS_Object *)sdFatFSHandles[drive]->object)->diskState);
}


#if (_READONLY == 0)
/*
 *  ======== SDFatFS_diskWrite ========
 *  Function to perform a write to the SDCard. This function is called by
 *  the FatFs module and must not be called by the application!
 */
DRESULT SDFatFS_diskWrite(BYTE drive, const BYTE *buffer, DWORD sector,
    UINT secCount)
{
    int_fast32_t    result;
    DRESULT         fatfsRes = RES_ERROR;
    SDFatFS_Object *obj = sdFatFSHandles[drive]->object;

    /* Return if disk not initialized */
    if ((obj->diskState & (DSTATUS)STA_NOINIT) != 0) {
        fatfsRes = RES_PARERR;
    }
    else {
        result = SD_write(obj->sdHandle, (const uint_least8_t *)buffer,
            (int_least32_t)sector, (uint_least32_t)secCount);

        /* Convert lower level driver status code */
        if (result == SD_STATUS_SUCCESS) {
            fatfsRes = RES_OK;
        }
    }

    return (fatfsRes);
}
#endif

/*
 *  ======== SDFatFS_init ========
 */
void SDFatFS_init(void)
{
    uint_least8_t   i;
    uint_fast32_t   key;
    SDFatFS_Object *obj;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Initialize each SDFatFS object */
        for (i = 0; i < SDFatFS_count; i++) {
            obj = ((SDFatFS_Handle)&(SDFatFS_config[i]))->object;

            obj->diskState = STA_NOINIT;
            obj->driveNum = DRIVE_NOT_MOUNTED;
        }

        /* Initialize the SD Driver */
        SD_init();
    }

    HwiP_restore(key);
}


/*
 *  ======== SDFatFS_open ========
 *  Note: The index passed into this function must correspond directly
 *  to the SD driver index.
 */
SDFatFS_Handle SDFatFS_open(uint_least8_t idx, uint_least8_t drive)
{
    uintptr_t       key;
    DRESULT         dresult;
    FRESULT         fresult;
    TCHAR           path[3];
    SDFatFS_Handle  handle = NULL;
    SDFatFS_Object *obj;

    /* Verify driver index and state */
    if (isInitialized && (idx < SDFatFS_count)) {
        /* Get handle for this driver instance */
        handle = (SDFatFS_Handle)&(SDFatFS_config[idx]);
        obj = handle->object;

        /* Determine if the device was already opened */
        key = HwiP_disable();
        if (obj->driveNum != DRIVE_NOT_MOUNTED) {
            HwiP_restore(key);
            DebugP_log1("SDFatFS Drive %d already in use!", obj->driveNum);
            handle = NULL;
        }
        else {
            obj->driveNum = drive;

            /* Open SD Driver */
            obj->sdHandle = SD_open(idx, NULL);

            HwiP_restore(key);

            if (obj->sdHandle == NULL) {
                obj->driveNum = DRIVE_NOT_MOUNTED;
                /* Error occurred in lower level driver */
                handle = NULL;
            }
            else {

                /* Register FATFS Functions */
                dresult = disk_register(obj->driveNum,
                    SDFatFS_diskInitialize,
                    SDFatFS_diskStatus,
                    SDFatFS_diskRead,
                    SDFatFS_diskWrite,
                    SDFatFS_diskIOctrl);

                /* Check for drive errors */
                if (dresult != RES_OK) {
                    DebugP_log0("SDFatFS: Disk functions not registered");
                    SDFatFS_close(handle);
                    handle = NULL;
                }
                else {

                    /* Construct base directory path */
                    path[0] = (TCHAR)'0' + obj->driveNum;
                    path[1] = (TCHAR)':';
                    path[2] = (TCHAR)'\0';

                    /*
                     * Register the filesystem with FatFs. This operation does
                     * not access the SDCard yet.
                     */
                    fresult = f_mount(&(obj->filesystem), path, 0);
                    if (fresult != FR_OK) {
                        DebugP_log1("SDFatFS: Drive %d not mounted",
                                    obj->driveNum);

                        SDFatFS_close(handle);
                        handle = NULL;
                    }
                    else {

                        /*
                         * Store the new sdfatfs handle for the input drive
                         * number
                         */
                        sdFatFSHandles[obj->driveNum] = handle;

                        DebugP_log0("SDFatFS: opened");
                    }
                }
            }
        }
    }

    return (handle);
}
