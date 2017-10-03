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

#include <stdint.h>
#include <stdbool.h>
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
#include <ti/drivers/sd/SDHostCC32XX.h>
#include <ti/drivers/dma/UDMACC32XX.h>

/* Driverlib header files */
#include <ti/devices/cc32xx/inc/hw_common_reg.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_mmchs.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/sdhost.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

#define PinConfigPinMode(config)       (((config) >> 8) & 0xF)
#define PinConfigPin(config)           (((config) >> 0) & 0x3F)

#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

/* Definitions for SDC driverlib commands  */
#define CMD_GO_IDLE_STATE    (SDHOST_CMD_0)
#define CMD_SEND_OP_COND     (SDHOST_CMD_1 | SDHOST_RESP_LEN_48)
#define CMD_ALL_SEND_CID     (SDHOST_CMD_2 | SDHOST_RESP_LEN_136)
#define CMD_SEND_REL_ADDR    (SDHOST_CMD_3 | SDHOST_RESP_LEN_48)
#define CMD_SELECT_CARD      (SDHOST_CMD_7 | SDHOST_RESP_LEN_48B)
#define CMD_DESELECT_CARD    (SDHOST_CMD_7)
#define CMD_SEND_IF_COND     (SDHOST_CMD_8 | SDHOST_RESP_LEN_48)
#define CMD_SEND_CSD         (SDHOST_CMD_9 | SDHOST_RESP_LEN_136)
#define CMD_STOP_TRANS       (SDHOST_CMD_12 | SDHOST_RESP_LEN_48B)
#define CMD_READ_SINGLE_BLK  (SDHOST_CMD_17 | SDHOST_RD_CMD | SDHOST_RESP_LEN_48)
#define CMD_READ_MULTI_BLK   (SDHOST_CMD_18 | SDHOST_RD_CMD | \
                              SDHOST_RESP_LEN_48 | SDHOST_MULTI_BLK)
#define CMD_SET_BLK_CNT      (SDHOST_CMD_23 | SDHOST_RESP_LEN_48)
#define CMD_WRITE_SINGLE_BLK (SDHOST_CMD_24 | SDHOST_WR_CMD | SDHOST_RESP_LEN_48)
#define CMD_WRITE_MULTI_BLK  (SDHOST_CMD_25 | SDHOST_WR_CMD | \
                              SDHOST_RESP_LEN_48 | SDHOST_MULTI_BLK)
#define CMD_SD_SEND_OP_COND  (SDHOST_CMD_41 | SDHOST_RESP_LEN_48)
#define CMD_APP_CMD          (SDHOST_CMD_55 | SDHOST_RESP_LEN_48)

/* Group of all possible SD command and data error flags */
#define CMDERROR             (SDHOST_INT_CTO | SDHOST_INT_CEB)

#define DATAERROR            (SDHOST_INT_DTO | SDHOST_INT_DCRC | \
                              SDHOST_INT_DEB | SDHOST_INT_CERR | \
                              SDHOST_INT_BADA)

#define SECTORSIZE           (512)

/* Voltage window (VDD) used on this device (3.3-3.6 V) */
#define VOLTAGEWINDOW        (0x00E00000)

/* Bit set to indicate the host controller has high capacity support */
#define HIGHCAPSUPPORT       (0x40000000)

/* Voltage supplied 2.7-3.6V */
#define SUPPLYVOLTAGE        (0x00000100)

/* Checksum to validate SD command responses */
#define CHECKSUM             (0xA5)

/* Setup the UDMA controller to either read or write to the SDHost buffer */
#define UDMAWRITE            (0x01)

#define UDMAREAD             (0x00)

/* Define used for commands that do not require an argument */
#define NULLARG              (0x00)

/* SDHostCC32XX configuration - initialized in the board file */
extern const SD_Config SD_config[];

/* SDHostCC32XX functions */
void SDHostCC32XX_close(SD_Handle handle);
int_fast16_t SDHostCC32XX_control(SD_Handle handle, uint_fast16_t cmd,
    void *arg);
uint_fast32_t SDHostCC32XX_getNumSectors(SD_Handle handle);
uint_fast32_t SDHostCC32XX_getSectorSize(SD_Handle handle);
int_fast16_t SDHostCC32XX_initialize(SD_Handle handle);
void SDHostCC32XX_init(SD_Handle handle);
SD_Handle SDHostCC32XX_open(SD_Handle handle, SD_Params *params);
int_fast16_t SDHostCC32XX_read(SD_Handle handle, void *buf,
    int_fast32_t sector, uint_fast32_t secCount);
int_fast16_t SDHostCC32XX_write(SD_Handle handle, const void *buf,
    int_fast32_t sector, uint_fast32_t secCount);

/* Local Functions */
static void configDMAChannel(SD_Handle handle, uint_fast32_t channelSel,
    uint_fast32_t operation);
static int_fast32_t deSelectCard(SD_Handle handle);
static uint_fast32_t getPowerMgrId(uint_fast32_t baseAddr);
static void hwiIntFxn(uintptr_t handle);
static void initHw(SD_Handle handle);
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg);
static int_fast32_t send_cmd(SD_Handle handle, uint_fast32_t cmd,
    uint_fast32_t arg);
static int_fast32_t selectCard(SD_Handle handle);

/* SDHostCC32XX function table for SDSPICC32XX implementation */
const SD_FxnTable sdHostCC32XX_fxnTable = {
    SDHostCC32XX_close,
    SDHostCC32XX_control,
    SDHostCC32XX_getNumSectors,
    SDHostCC32XX_getSectorSize,
    SDHostCC32XX_init,
    SDHostCC32XX_initialize,
    SDHostCC32XX_open,
    SDHostCC32XX_read,
    SDHostCC32XX_write
};

/*
 *  ======== SDHostCC32XX_close ========
 */
void SDHostCC32XX_close(SD_Handle handle)
{
    SDHostCC32XX_Object          *object = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t                      padRegister;

    if (object->cardType != SD_NOCARD) {
        /* De-select the SD Card, move back to standby state */
        if (deSelectCard(handle) != SD_STATUS_SUCCESS) {
            DebugP_log1("SDHost:(%p) Failed to de-select SD Card",
                hwAttrs->baseAddr);
        }
    }

    /* Disable SD Host interrupts */
    MAP_SDHostIntDisable(hwAttrs->baseAddr, DATAERROR | CMDERROR);

    if (object->dmaHandle) {
        UDMACC32XX_close(object->dmaHandle);
    }
    if (object->cmdSem) {
        SemaphoreP_delete(object->cmdSem);
    }
    if (object->hwiHandle) {
        HwiP_delete(object->hwiHandle);
    }

    /* Remove Power driver settings */
    if (object->clkPin != (uint16_t)-1) {
        PowerCC32XX_restoreParkState((PowerCC32XX_Pin)object->clkPin,
            object->prevParkCLK);
        object->clkPin = (uint16_t)-1;
    }
    Power_unregisterNotify(&object->postNotify);
    Power_releaseDependency(object->powerMgrId);

    /* Restore pin pads to their reset states */
    padRegister = (PinToPadGet((hwAttrs->dataPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->cmdPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;
    padRegister = (PinToPadGet((hwAttrs->clkPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;

    object->isOpen = false;

    DebugP_log1("SDHost:(%p) closed and released power dependency",
        hwAttrs->baseAddr);
}

/*
 *  ======== SDHostCC32XX_control ========
 *  @pre    Function assumes that the handle is not NULL.
 */
int_fast16_t SDHostCC32XX_control(SD_Handle handle, uint_fast32_t cmd,
    void *arg)
{
    /* No implementation yet */
    return (SD_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== SDHostCC32XX_getNumSectors ========
 *  Function to read the CSD register of the SD card on the drive specified
 *  by the SD_Handle and calculate the total card capacity in sectors.
 */
uint_fast32_t SDHostCC32XX_getNumSectors(SD_Handle handle)
{
    uint_fast32_t                 sectors = 0;
    uint_fast32_t                 cSize; /* Device size */
    uint_fast32_t                 blockSize; /* Read block length */
    uint_fast32_t                 sizeMult;
    uint_fast32_t                 resp[4];
    SDHostCC32XX_Object          *object = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_getNumSectors set command"
        " power constraint", hwAttrs->baseAddr);

    /* De-Select the card on the input drive(Stand-by state) */
    if (deSelectCard(handle) != SD_STATUS_SUCCESS) {
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
        DebugP_log1("SDHost:(%p) SDHostCC32XX_getNumSectors released command"
            " power constraint", hwAttrs->baseAddr);
        return (0);
    }

    if (send_cmd(handle, CMD_SEND_CSD, (object->rca << 16)) ==
        SD_STATUS_SUCCESS) {

        MAP_SDHostRespGet(hwAttrs->baseAddr, (unsigned long *)resp);

       /*
        * 136 bit CSD register is read into an array of 4 words
        * Note: Does not include 8 bit CRC
        * resp[0] = CSD[31:0]
        * resp[1] = CSD[63:32]
        * resp[2] = CSD[95:64]
        * resp[3] = CSD[127:96]
        */
        if(((resp[3] >> 30) & 0x01) == 1) {
            sectors = (resp[1] >> 16 | ((resp[2] & 0x3F) << 16)) + 1;
            sectors *= 1024;
        }
        else {
            blockSize = 1 << ((resp[2] >> 16) & 0x0F);
            sizeMult = ((resp[1] >> 15) & 0x07);
            cSize = ((resp[1] >> 30) | (resp[2] & 0x3FF) << 2);
            sectors = (cSize + 1) * (1 << (sizeMult + 2));
            sectors = (sectors * blockSize) / SECTORSIZE;

        }
    }

    /* Select the card on the input drive(Transfer state) */
    if (selectCard(handle) != SD_STATUS_SUCCESS) {
        sectors = 0;
    }

    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_getNumSectors released command"
        " power constraint", hwAttrs->baseAddr);

    return (sectors);
}

/*
 *  ======== SDHostCC32XX_getSectorSize ========
 *  Function to perform a disk read from the SD Card on the specifed drive.
 */
uint_fast32_t SDHostCC32XX_getSectorSize(SD_Handle handle)
{
    return (SECTORSIZE);
}

/*
 *  ======== SDHostCC32XX_initialize ========
 *  Function to initialize the SD Card.
 */
int_fast16_t SDHostCC32XX_initialize(SD_Handle handle)
{
    int_fast32_t                  result;
    uint_fast32_t                 resp[4];
    SDHostCC32XX_Object          *object = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_initialize set read/write"
        " power constraint", hwAttrs->baseAddr);

    /* Send go to IDLE command */
    result = send_cmd(handle, CMD_GO_IDLE_STATE, NULLARG);

    if (result == SD_STATUS_SUCCESS) {
        /* Get interface operating condition for the card */
        result = send_cmd(handle, CMD_SEND_IF_COND,
            SUPPLYVOLTAGE | CHECKSUM);
        /* Verify response will be valid */
        if (result == SD_STATUS_SUCCESS) {
            MAP_SDHostRespGet(hwAttrs->baseAddr, (unsigned long *)resp);
        }
    }

    /* SD ver 2.0 or higher card */
    if ((result == SD_STATUS_SUCCESS) && ((resp[0] & 0xFF) ==
        CHECKSUM)) {
        object->cardType = SD_SDSC;

        /* Wait for card to be ready */
        do {
            /* Send ACMD41 */
            result = send_cmd(handle, CMD_APP_CMD, NULLARG);

            if (result == SD_STATUS_SUCCESS) {
                result = send_cmd(handle, CMD_SD_SEND_OP_COND,
                    HIGHCAPSUPPORT | VOLTAGEWINDOW);
                /* Response contains 32-bit OCR register */
                MAP_SDHostRespGet(hwAttrs->baseAddr, (unsigned long *)resp);
            }

        /* Wait until card is ready, spool on busy bit */
        } while((result == SD_STATUS_SUCCESS) && ((resp[0] >> 31) == 0));

        /* Card capacity status bit */
        if ((result == SD_STATUS_SUCCESS) && (resp[0] & (1UL << 30))) {
            object->cardType = SD_SDHC;
        }
    }
    /* It's a MMC or SD 1.x card */
    else {
        /* Wait for card to be ready */
        do {
            /* Send ACMD41 */
            result = send_cmd(handle, CMD_APP_CMD, NULLARG);

            if (result == SD_STATUS_SUCCESS) {
                result = send_cmd(handle, CMD_SD_SEND_OP_COND, VOLTAGEWINDOW);
            }

            if (result == SD_STATUS_SUCCESS) {
                /* Response contains 32-bit OCR register */
                MAP_SDHostRespGet(hwAttrs->baseAddr, (unsigned long *)resp);
            }

        /* Wait until card is ready, spool on busy bit */
        } while((result == SD_STATUS_SUCCESS) && ((resp[0] >> 31) == 0));

        if (result == SD_STATUS_SUCCESS) {
            object->cardType = SD_SDSC;
        }
        else if (send_cmd(handle, CMD_SEND_OP_COND, NULLARG) ==
            SD_STATUS_SUCCESS) {
            object->cardType = SD_MMC;
        }
        /* No command responses */
        else {
            object->cardType = SD_NOCARD;
        }
    }

    /* Get the relative card address (RCA) of the attached card */
    if (object->cardType != SD_NOCARD) {
        result = send_cmd(handle, CMD_ALL_SEND_CID, NULLARG);

        if (result == SD_STATUS_SUCCESS) {
            result = send_cmd(handle, CMD_SEND_REL_ADDR, NULLARG);
        }

        if (result == SD_STATUS_SUCCESS) {
            /* Fill in the RCA */
            MAP_SDHostRespGet(hwAttrs->baseAddr, (unsigned long *)resp);

            object->rca = resp[0] >> 16;

            /* Select the card on the input drive(Transfer state) */
            result = selectCard(handle);
            if (result == SD_STATUS_SUCCESS) {
                /* Set card read/write block length */
                MAP_SDHostBlockSizeSet(hwAttrs->baseAddr, SECTORSIZE);

                /* Initialization succeeded */
                result = SD_STATUS_SUCCESS;
            }
        }
    }
    else {
        DebugP_log1("SDHost:(%p) Could not select card",
            hwAttrs->baseAddr);
        result = SD_STATUS_ERROR;
    }

    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_initialize released read/write"
        " power constraint", hwAttrs->baseAddr);

    return (result);
}

/*
 *  ======== SDHostCC32XX_init ========
 *  Function to initialize the SDHost module
 */
void SDHostCC32XX_init(SD_Handle handle)
{
    SDHostCC32XX_Object *object = handle->object;

    object->writeBuf = NULL;
    object->readBuf = NULL;
    object->writeSecCount = 0;
    object->readSecCount = 0;
    object->cardType = SD_NOCARD;
    object->isOpen = false;

    UDMACC32XX_init();
}

/*
 *  ======== SDHostCC32XX_open ========
 */
SD_Handle SDHostCC32XX_open(SD_Handle handle, SD_Params *params)
{
    uintptr_t                     key;
    SemaphoreP_Params             semParams;
    HwiP_Params                   hwiParams;
    SDHostCC32XX_Object          *object = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint16_t pin;

    key = HwiP_disable();

    if (object->isOpen) {
        HwiP_restore(key);

        DebugP_log1("SDHost:(%p) already in use.", hwAttrs->baseAddr);
        return (NULL);
    }
    object->isOpen = true;

    HwiP_restore(key);

    /* Initialize the SDCARD_CLK pin ID to undefined */
    object->clkPin = (uint16_t)-1;

    /* Get the Power resource Id from the base address */
    object->powerMgrId = getPowerMgrId(hwAttrs->baseAddr);
    if (object->powerMgrId == (unsigned int)-1) {
        DebugP_log1("SDHost:(%p) Failed to determine Power resource id",
            hwAttrs->baseAddr);
        return (NULL);
    }

    /*
     *  Register power dependency. Keeps the clock running in SLP
     *  and DSLP modes.
     */
    Power_setDependency(object->powerMgrId);

    Power_registerNotify(&object->postNotify, PowerCC32XX_AWAKE_LPDS,
        postNotifyFxn, (uintptr_t)handle);

    object->dmaHandle = UDMACC32XX_open();
    if (object->dmaHandle == NULL) {
        DebugP_log1("SDHost:(%p) UDMACC32XX_open() failed.",
            hwAttrs->baseAddr);
        SDHostCC32XX_close(handle);
        return (NULL);
    }

    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;
    object->cmdSem = SemaphoreP_create(0, &semParams);

    if (object->cmdSem == NULL) {
        DebugP_log1("SDHost:(%p) SemaphoreP_create() failed.",
            hwAttrs->baseAddr);
        SDHostCC32XX_close(handle);
        return (NULL);
    }

    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t)handle;
    hwiParams.priority = hwAttrs->intPriority;
    object->hwiHandle = HwiP_create(INT_MMCHS, hwiIntFxn,
        &hwiParams);
    if (object->hwiHandle == NULL) {
        DebugP_log1("SDHostT:(%p) HwiP_create() failed", hwAttrs->baseAddr);
        SDHostCC32XX_close(handle);
        return (NULL);
    }

    /* Initialize the hardware */
    initHw(handle);

    /* Save clkPin park state; set to logic '0' during LPDS */
    pin = PinConfigPin(hwAttrs->clkPin);
    object->prevParkCLK =
        (PowerCC32XX_ParkState) PowerCC32XX_getParkState((PowerCC32XX_Pin)pin);
    PowerCC32XX_setParkState((PowerCC32XX_Pin)pin, 0);
    object->clkPin = pin;

    DebugP_log1("SDHost:(%p) opened", hwAttrs->baseAddr);

    return (handle);
}

/*
 *  ======== SDHostCC32XX_read ========
 */
int_fast16_t SDHostCC32XX_read(SD_Handle handle, void *buf,
    int_fast32_t sector, uint_fast32_t secCount)
{
    int_fast32_t                  result;
    uint_fast32_t                 ret;
    uint_fast32_t                 size;
    SDHostCC32XX_Object          *object  = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    object->stat = SD_STATUS_ERROR;

    /* Check for valid sector count */
    if (secCount == 0) {
        return (SD_STATUS_ERROR);
    }

    /* SDSC uses linear address, SDHC uses block address */
    if (object->cardType == SD_SDSC) {
        sector = sector * SECTORSIZE;
    }

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_read set read power constraint",
        hwAttrs->baseAddr);

    /* Set the block count */
    MAP_SDHostBlockCountSet(hwAttrs->baseAddr, secCount);

    /* If input buffer is word aligned use DMA */
    if (!(((uint_fast32_t)buf) & 0x03)) {
        object->readBuf = (uint_fast32_t *)buf;
        object->readSecCount = secCount;

        /* Configure Primary structure */
        configDMAChannel(handle, UDMA_PRI_SELECT , UDMAREAD);

        /* Add offset to input buffer */
        object->readBuf = (uint_fast32_t *)((uint_least8_t *)buf +
            SECTORSIZE);

        /* Configure alternate control structure */
        configDMAChannel(handle, UDMA_ALT_SELECT , UDMAREAD);

        MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_DMARD);

        /* Send multi block read command */
        result = send_cmd(handle, CMD_READ_MULTI_BLK | SDHOST_DMA_EN, sector);

        if (result == SD_STATUS_SUCCESS) {
            MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_DMARD);

            /* Wait for DMA read(s) to complete */
            SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

            MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_DMARD);
        }
    }
    /* Poll buffer for new data */
    else {
        /* Compute the number of words to read */
        size = (SECTORSIZE * secCount) / 4;

        /* Send multi block read command */
        result = send_cmd(handle, CMD_READ_MULTI_BLK, sector);

        if (result == SD_STATUS_SUCCESS) {
            /* Read single word of data */
            while (size > 0) {
                ret = MAP_SDHostDataNonBlockingRead(hwAttrs->baseAddr,
                    (unsigned long *)buf);
                /* Block until buffer is ready */
                if (ret == 0) {
                    MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_BRR);

                    SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

                    MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_BRR);
                }
                else {
                    buf = (uint_least8_t *)buf + 4;
                    size--;
                }
            }
        }
    }

    /* Verify host controller errors didn't occur */
    if (object->stat == SD_STATUS_SUCCESS) {

        MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_TC);

        /* Wait for full data transfer to complete */
        SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

        MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_TC);
    }

    /* Verify host controller errors didn't occur */
    if (object->stat == SD_STATUS_SUCCESS) {

        /* Wait for command transfer stop acknowledgement */
        result = send_cmd(handle, CMD_STOP_TRANS, NULLARG);

        if (result ==  SD_STATUS_SUCCESS) {
            MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_TC);

            SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

            MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_TC);
        }
    }

    if (object->stat != SD_STATUS_SUCCESS) {
        result = SD_STATUS_ERROR;
    }

    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_read released read power"
        " constraint", hwAttrs->baseAddr);

    return (result);
}

/*
 *  ======== SDHostCC32XX_write ========
 */
int_fast16_t SDHostCC32XX_write(SD_Handle handle, const void *buf,
    int_fast32_t sector, uint_fast32_t secCount)
{
    int_fast32_t                  result;
    uint_fast32_t                 ret;
    uint_fast32_t                 size;
    SDHostCC32XX_Object          *object  = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    object->stat = SD_STATUS_ERROR;

    /* Check for valid sector count */
    if (secCount == 0) {
        return (SD_STATUS_ERROR);
    }

    /* SDSC uses linear address, SDHC uses block address */
    if(object->cardType == SD_SDSC) {
        sector = sector * SECTORSIZE;
    }

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_write set write power"
        " constraint", hwAttrs->baseAddr);

    /* Set the block count */
    MAP_SDHostBlockCountSet(hwAttrs->baseAddr, secCount);

    /* Set the card write block count */
    result = send_cmd(handle, CMD_APP_CMD,  object->rca << 16);

    if (result == SD_STATUS_SUCCESS) {
        result = send_cmd(handle, CMD_SET_BLK_CNT, secCount);
    }

    /* If input buffer is word aligned use DMA */
    if (!(((uint_fast32_t)buf) & 0x03)) {
        object->writeBuf = (uint_fast32_t *)buf;
        object->writeSecCount = secCount;

        /* Configure Primary structure */
        configDMAChannel(handle, UDMA_PRI_SELECT, UDMAWRITE);

        /* Add offset to input buffer */
        object->writeBuf = (uint_fast32_t *)((uint_least8_t *)buf +
            SECTORSIZE);
        /* Configure alternate control structure */
        configDMAChannel(handle, UDMA_ALT_SELECT, UDMAWRITE);

        /* Verify that the block count had been previous set */
        if (result == SD_STATUS_SUCCESS) {
            result = send_cmd(handle, CMD_WRITE_MULTI_BLK | SDHOST_DMA_EN,
                sector);
        }

        if (result == SD_STATUS_SUCCESS) {
            /* Wait for DMA write(s) to complete */
            MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_DMAWR);

            SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

            MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_DMAWR);
        }

    }
    else {
        /* Compute the number of words to write */
        size = (SECTORSIZE * secCount) / 4;

        /* Verify that the block count had been previous set */
        if (result == SD_STATUS_SUCCESS) {
            result = send_cmd(handle, CMD_WRITE_MULTI_BLK, sector);
        }

        if (result == SD_STATUS_SUCCESS) {
            /* Write single word of data */
            while (size > 0) {
                ret = MAP_SDHostDataNonBlockingWrite(hwAttrs->baseAddr,
                    (*(unsigned long *)buf));

                /* Block until buffer is ready */
                if (ret == 0) {
                    MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_BWR);

                    SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

                    MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_BWR);
                }
                else {
                    buf = (uint_least8_t *)buf + 4;
                    size--;
                }
            }
        }
    }

    /* Verify host controller errors didn't occur */
    if (object->stat == SD_STATUS_SUCCESS) {
        /* Wait for the full data transfer to complete */
        MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_TC);

        SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

        MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_TC);
    }

    /* Verify host controller errors didn't occur */
    if (object->stat == SD_STATUS_SUCCESS) {
        /* Wait for command transfer stop acknowledgement */
        result = send_cmd(handle, CMD_STOP_TRANS, NULLARG);

        if (result ==  SD_STATUS_SUCCESS) {
            MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_TC);

            SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

            MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_TC);
        }
    }

    if (object->stat != SD_STATUS_SUCCESS) {
        result = SD_STATUS_ERROR;
    }

    Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    DebugP_log1("SDHost:(%p) SDHostCC32XX_write released write power"
        " constraint", hwAttrs->baseAddr);

    return (result);
}

/*
 *  ======== configDMAChannel ========
 *  Configures either the primary or alternate DMA control structures in
 *  ping-pong mode.
 *  channelSel is the PRI or ALT channel select flag
 */
static void configDMAChannel(SD_Handle handle, uint_fast32_t channelSel,
    uint_fast32_t operation)
{
    unsigned long                 channelControlOptions;
    SDHostCC32XX_Object          *object = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    if (operation == UDMAWRITE) {
        channelControlOptions = UDMA_SIZE_32 | UDMA_SRC_INC_32 |
            UDMA_DST_INC_NONE | UDMA_ARB_512;

        /* Primary control structure set-up */
        MAP_uDMAChannelControlSet(hwAttrs->txChIdx |
            channelSel, channelControlOptions);

        /* Transfer size is the sector size in words */
        MAP_uDMAChannelTransferSet(hwAttrs->txChIdx | channelSel,
            UDMA_MODE_PINGPONG, (void *)object->writeBuf,
            (void *)(hwAttrs->baseAddr + MMCHS_O_DATA), SECTORSIZE / 4);

        /* Enable the DMA channel */
        MAP_uDMAChannelEnable(hwAttrs->txChIdx | channelSel);
    }
    else {
        channelControlOptions = UDMA_SIZE_32 | UDMA_SRC_INC_NONE |
                UDMA_DST_INC_32 | UDMA_ARB_512;

        /* Primary control structure set-up */
        MAP_uDMAChannelControlSet(hwAttrs->rxChIdx |
            channelSel, channelControlOptions);

        /* Transfer size is the sector size in words */
        MAP_uDMAChannelTransferSet(hwAttrs->rxChIdx | channelSel,
            UDMA_MODE_PINGPONG, (void *)(hwAttrs->baseAddr + MMCHS_O_DATA),
            (void *)object->readBuf, SECTORSIZE / 4);

        /* Enable the DMA channel */
        MAP_uDMAChannelEnable(hwAttrs->rxChIdx | channelSel);
    }
}

/*
 *  ======== deSelectCard ========
 *  Function to de-select a card on the drive specified by the handle.
 */
static int_fast32_t deSelectCard(SD_Handle handle)
{
    int_fast32_t result;

    /* De-select the card */
    result = send_cmd(handle, CMD_DESELECT_CARD, NULLARG);

    return (result);
}

/*
 *  ======== getPowerMgrId ========
 */
static uint_fast32_t getPowerMgrId(uint_fast32_t baseAddr)
{
    if (baseAddr == SDHOST_BASE) {
        return (PowerCC32XX_PERIPH_SDHOST);
    }
    else {
        return ((uint_fast32_t)-1);
    }
}

/*
 *  ======== hwiIntFxn ========
 *  ISR to service pending SD or DMA commands.
 */
static void hwiIntFxn(uintptr_t handle)
{
    uint_fast32_t                 ret;
    uint_fast32_t                 priChannelMode;
    uint_fast32_t                 altChannelMode;
    SDHostCC32XX_Object          *object = ((SD_Handle)handle)->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = ((SD_Handle)handle)->hwAttrs;

    /* Get interrupt status */
    ret = MAP_SDHostIntStatus(hwAttrs->baseAddr);

    /* Don't unblock if an error occurred */
    if (ret & SDHOST_INT_ERRI) {
        /* Clear any spurious interrupts caused by the error */
        MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_CC | SDHOST_INT_TC |
            DATAERROR | CMDERROR);

        /* Error or unsupported interrupt occurred */
        object->stat = SD_STATUS_ERROR;
        SemaphoreP_post(object->cmdSem);
    }
    /* Command complete flag */
    else if (ret & SDHOST_INT_CC) {
        object->stat = SD_STATUS_SUCCESS;
        MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_CC);
        SemaphoreP_post(object->cmdSem);
    }
    /* DMA read complete flag */
    else if (ret & SDHOST_INT_DMARD) {
        object->readSecCount--;

        priChannelMode = MAP_uDMAChannelModeGet(hwAttrs->rxChIdx |
            UDMA_PRI_SELECT);
        altChannelMode = MAP_uDMAChannelModeGet(hwAttrs->rxChIdx |
            UDMA_ALT_SELECT);

        /* Corner case in which a DMA interrupt is missed, (both completed) */
        if ((priChannelMode != UDMA_MODE_STOP) ||
            (altChannelMode != UDMA_MODE_STOP)) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_DMARD);
        }

        /* Check if transfer is complete */
        if (object->readSecCount == 0) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_DMARD);

            object->stat = SD_STATUS_SUCCESS;

            /* Reset to primary channel */
            MAP_uDMAChannelAttributeDisable(hwAttrs->rxChIdx,
                UDMA_ATTR_ALTSELECT);
            SemaphoreP_post(object->cmdSem);
        }
        else {
            /* Set-up next portion of buffer to be written to */
            object->readBuf = object->readBuf + (SECTORSIZE / 4);

            /* Check if primary channel completed */
            if (priChannelMode == UDMA_MODE_STOP) {
                configDMAChannel((SD_Handle)handle, UDMA_PRI_SELECT, UDMAREAD);
            }
            /* Secondary completed */
            else {
                configDMAChannel((SD_Handle)handle, UDMA_ALT_SELECT, UDMAREAD);
            }
        }
    }
    /* DMA write complete flag */
    else if (ret & SDHOST_INT_DMAWR) {
        object->writeSecCount--;

        priChannelMode = MAP_uDMAChannelModeGet(hwAttrs->txChIdx |
            UDMA_PRI_SELECT);
        altChannelMode = MAP_uDMAChannelModeGet(hwAttrs->txChIdx |
            UDMA_ALT_SELECT);

        /* Corner case in which a DMA interrupt is missed (both completed) */
        if ((priChannelMode != UDMA_MODE_STOP) ||
            (altChannelMode != UDMA_MODE_STOP)) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_DMAWR);
        }

        /* Check if transfer is complete */
        if (object->writeSecCount == 0) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_DMAWR);

            object->stat = SD_STATUS_SUCCESS;

            /* Reset to primary channel */
            MAP_uDMAChannelAttributeDisable(hwAttrs->txChIdx,
                UDMA_ATTR_ALTSELECT);
            SemaphoreP_post(object->cmdSem);
        }
        else {
            /* Set-up next portion of buffer to be written to */
            object->writeBuf = object->writeBuf + (SECTORSIZE / 4);

            /* Check if primary channel completed */
            if (priChannelMode == UDMA_MODE_STOP) {
                configDMAChannel((SD_Handle)handle, UDMA_PRI_SELECT,
                    UDMAWRITE);
            }
            /* Secondary completed */
            else {
                configDMAChannel((SD_Handle)handle, UDMA_ALT_SELECT,
                    UDMAWRITE);
            }
        }
    }
    else {
        /* Transfer complete flag */
        if (ret & SDHOST_INT_TC) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_TC);
        }
        /* Data buffer read ready flag */
        else if (ret & SDHOST_INT_BRR) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_BRR);
        }
        /* Data buffer write ready flag */
        else if (ret & SDHOST_INT_BWR) {
            MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_BWR);
        }
        object->stat = SD_STATUS_SUCCESS;
        SemaphoreP_post(object->cmdSem);
    }
}

/*
 *  ======== initHw ========
 */
static void initHw(SD_Handle handle)
{
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t pin;
    uint32_t mode;

    /* Configure for SDHost Data */
    pin = PinConfigPin(hwAttrs->dataPin);
    mode = PinConfigPinMode(hwAttrs->dataPin);
    MAP_PinTypeSDHost(pin, mode);
    MAP_PinConfigSet(pin, PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

    /* Configure for SDHost Command */
    pin = PinConfigPin(hwAttrs->cmdPin);
    mode = PinConfigPinMode(hwAttrs->cmdPin);
    MAP_PinTypeSDHost(pin, mode);
    MAP_PinConfigSet(pin, PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

    /* Configure for SDHost clock output */
    pin = PinConfigPin(hwAttrs->clkPin);
    mode = PinConfigPinMode(hwAttrs->clkPin);
    MAP_PinTypeSDHost(pin, mode);
    MAP_PinDirModeSet(pin, PIN_DIR_MODE_OUT);

    MAP_PRCMPeripheralReset(PRCM_SDHOST);
    MAP_SDHostInit(hwAttrs->baseAddr);

    MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_CC | SDHOST_INT_TC |
        SDHOST_INT_DMAWR | SDHOST_INT_DMARD | SDHOST_INT_BRR | SDHOST_INT_BWR);

    /* Configure card clock */
    MAP_SDHostSetExpClk(hwAttrs->baseAddr,
        MAP_PRCMPeripheralClockGet(PRCM_SDHOST), hwAttrs->clkRate);

    MAP_SDHostIntClear(hwAttrs->baseAddr, SDHOST_INT_CC | SDHOST_INT_TC |
        DATAERROR | CMDERROR | SDHOST_INT_DMAWR | SDHOST_INT_DMARD |
        SDHOST_INT_BRR | SDHOST_INT_BWR);

    /* Configure DMA for TX and RX */
    MAP_uDMAChannelAssign(hwAttrs->txChIdx);
    MAP_uDMAChannelAssign(hwAttrs->rxChIdx);

    /* Enable SDHost Error Interrupts */
    MAP_SDHostIntEnable(hwAttrs->baseAddr, DATAERROR | CMDERROR);
}


/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up from LPDS.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
        uintptr_t clientArg)
{
    initHw((SD_Handle)clientArg);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== send_cmd ========
 *  Function to send a command to the SD Card
 */
static int_fast32_t send_cmd(SD_Handle handle, uint_fast32_t cmd,
    uint_fast32_t arg)
{
    int_fast32_t                  result  = SD_STATUS_SUCCESS;
    SDHostCC32XX_Object          *object  = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /* Send the command */
    MAP_SDHostCmdSend(hwAttrs->baseAddr, cmd, arg);

    /* Enable command complete interrupts */
    MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_CC);

    SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

    MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_CC);

    /* SD Card Error, reset the command line */
    if (object->stat == SD_STATUS_ERROR) {
        MAP_SDHostCmdReset(hwAttrs->baseAddr);
        result = SD_STATUS_ERROR;
    }

    return (result);
}

/*
 *  ======== selectCard ========
 *  Function to select a card on the specified drive.
 */
static int_fast32_t selectCard(SD_Handle handle)
{
    int_fast32_t                  result;
    SDHostCC32XX_Object          *object  = handle->object;
    SDHostCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    /* Select the card */
    result = send_cmd(handle, CMD_SELECT_CARD, object->rca << 16);

    if (result == SD_STATUS_SUCCESS) {
        /* Wait for transfer compelte interrupt */
        MAP_SDHostIntEnable(hwAttrs->baseAddr, SDHOST_INT_TC);

        SemaphoreP_pend(object->cmdSem, SemaphoreP_WAIT_FOREVER);

        MAP_SDHostIntDisable(hwAttrs->baseAddr, SDHOST_INT_TC);

        /* Host controller error occurred */
        if (object->stat != SD_STATUS_SUCCESS) {
            result = SD_STATUS_ERROR;
        }
    }
    return (result);
}
