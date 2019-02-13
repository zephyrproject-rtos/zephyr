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
 *  ======== SDSPI.c ========
 */

#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SD.h>
#include <ti/drivers/sd/SDSPI.h>
#include <ti/drivers/SPI.h>

/* Definitions for MMC/SDC command */
#define CMD0                      (0x40+0)     /* GO_IDLE_STATE */
#define CMD1                      (0x40+1)     /* SEND_OP_COND */
#define CMD8                      (0x40+8)     /* SEND_IF_COND */
#define CMD9                      (0x40+9)     /* SEND_CSD */
#define CMD10                     (0x40+10)    /* SEND_CID */
#define CMD12                     (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16                     (0x40+16)    /* SET_BLOCKLEN */
#define CMD17                     (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18                     (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23                     (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24                     (0x40+24)    /* WRITE_BLOCK */
#define CMD25                     (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41                     (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55                     (0x40+55)    /* APP_CMD */
#define CMD58                     (0x40+58)    /* READ_OCR */
#define START_BLOCK_TOKEN         (0xFE)
#define START_MULTIBLOCK_TOKEN    (0xFC)
#define STOP_MULTIBLOCK_TOKEN     (0xFD)

#define SD_SECTOR_SIZE            (512)

#define DRIVE_NOT_MOUNTED         ((uint16_t) ~0)

void SDSPI_close(SD_Handle handle);
int_fast16_t SDSPI_control(SD_Handle handle, uint_fast16_t cmd,
    void *arg);
uint_fast32_t SDSPI_getNumSectors(SD_Handle handle);
uint_fast32_t SDSPI_getSectorSize(SD_Handle handle);
int_fast16_t SDSPI_initialize(SD_Handle handle);
void SDSPI_init(SD_Handle handle);
SD_Handle SDSPI_open(SD_Handle handle, SD_Params *params);
int_fast16_t SDSPI_read(SD_Handle handle, void *buf,
    int_fast32_t sector, uint_fast32_t sectorCount);
int_fast16_t SDSPI_write(SD_Handle handle, const void *buf,
    int_fast32_t sector, uint_fast32_t sectorCount);

static inline void assertCS(SDSPI_HWAttrs const *hwAttrs);
static inline void deassertCS(SDSPI_HWAttrs const *hwAttrs);
static bool recvDataBlock(SPI_Handle handle, void *buf, uint32_t count);
static uint8_t sendCmd(SPI_Handle handle, uint8_t cmd, uint32_t arg);
static int_fast16_t sendInitialClockTrain(SPI_Handle handle);
static int_fast16_t spiTransfer(SPI_Handle handle, void *rxBuf,
    void *txBuf, size_t count);
static bool waitUntilReady(SPI_Handle handle);
static bool transmitDataBlock(SPI_Handle handle, void *buf, uint32_t count,
    uint8_t token);

/* SDSPI function table for SDSPIMSP432 implementation */
const SD_FxnTable SDSPI_fxnTable = {
    SDSPI_close,
    SDSPI_control,
    SDSPI_getNumSectors,
    SDSPI_getSectorSize,
    SDSPI_init,
    SDSPI_initialize,
    SDSPI_open,
    SDSPI_read,
    SDSPI_write
};

/*
 *  ======== SDSPI_close ========
 */
void SDSPI_close(SD_Handle handle)
{
    SDSPI_Object *object = handle->object;

    if (object->spiHandle) {
        SPI_close(object->spiHandle);
        object->spiHandle = NULL;
    }

    if (object->lockSem) {
        SemaphoreP_delete(object->lockSem);
        object->lockSem = NULL;
    }

    object->cardType = SD_NOCARD;
    object->isOpen = false;
}

/*
 *  ======== SDSPI_control ========
 */
int_fast16_t SDSPI_control(SD_Handle handle, uint_fast16_t cmd,
    void *arg)
{
    return (SD_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== SDSPI_getNumSectors ========
 */
uint_fast32_t SDSPI_getNumSectors(SD_Handle handle)
{
    uint8_t              n;
    uint8_t              csd[16];
    uint16_t             csize;
    uint32_t             sectors = 0;
    SDSPI_Object        *object = handle->object;
    SDSPI_HWAttrs const *hwAttrs = handle->hwAttrs;

    SemaphoreP_pend(object->lockSem, SemaphoreP_WAIT_FOREVER);

    assertCS(hwAttrs);

    /* Get number of sectors on the disk (uint32_t) */
    if ((sendCmd(object->spiHandle, CMD9, 0) == 0) &&
        recvDataBlock(object->spiHandle, csd, 16)) {
        /* SDC ver 2.00 */
        if ((csd[0] >> 6) == 1) {
            csize = csd[9] + (csd[8] << 8) + 1;
            sectors = csize << 10;
        }
        /* MMC or SDC ver 1.XX */
        else {
            n = (csd[5] & 15) + ((csd[10] & 128) >> 7) +
                ((csd[9] & 3) << 1) + 2;

            csize = (csd[8] >> 6) + ((uint16_t) csd[7] << 2) +
                ((uint16_t) (csd[6] & 3) << 10) + 1;
            sectors = (uint32_t)csize << (n - 9);
        }
    }

    deassertCS(hwAttrs);

    SemaphoreP_post(object->lockSem);

    return (sectors);
}

/*
 *  ======== SDSPI_getSectorSize ========
 */
uint_fast32_t SDSPI_getSectorSize(SD_Handle handle)
{
    return (SD_SECTOR_SIZE);
}

/*
 *  ======== SDSPI_init ========
 */
void SDSPI_init(SD_Handle handle)
{
    GPIO_init();
    SPI_init();
}

/*
 *  ======== SDSPI_initialize ========
 */
int_fast16_t SDSPI_initialize(SD_Handle handle)
{
    SD_CardType          cardType = SD_NOCARD;
    uint8_t              i;
    uint8_t              ocr[4];
    uint8_t              txDummy[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    int_fast16_t         status;
    uint32_t             currentTime;
    uint32_t             startTime;
    uint32_t             timeout;
    SPI_Params           spiParams;
    SDSPI_Object        *object = handle->object;
    SDSPI_HWAttrs const *hwAttrs = handle->hwAttrs;

    SemaphoreP_pend(object->lockSem, SemaphoreP_WAIT_FOREVER);

    /*
     * Part of the process to initialize the SD Card to SPI mode - Do not
     * assert CS during this time
     */
    sendInitialClockTrain(object->spiHandle);

    /* Now select the SD Card's chip select to send CMD0 command */
    assertCS(hwAttrs);

    /*
     * Send CMD0 to put the SD card in idle SPI mode.  Some SD cards may not
     * respond correctly to CMD0 on the first attempt; so we will try for up
     * to 10 attempts (cards will usually respond correctly on the 3rd or 4th
     * attempt). Failure is returned if the card does not return the correct
     * response with the 10 attempts.
     */
    i = 10;
    do {
        status = sendCmd(object->spiHandle, CMD0, 0);
    } while ((status != 1) && (--i));

    /* Proceed with initialization only if SD Card is in "Idle" state */
    if (status == 1) {
        /*
         * Determine what SD Card version we are dealing with
         * Depending on which SD Card version, we need to send different SD
         * commands to the SD Card, which will have different response fields.
         */
        if (sendCmd(object->spiHandle, CMD8, 0x1AA) == 1) {
            /* SD Version 2.0 or higher */
            status = spiTransfer(object->spiHandle, &ocr, &txDummy, 4);
            if (status == SD_STATUS_SUCCESS) {
                /*
                 * Ensure that the card's voltage range is valid
                 * The card can work at VDD range of 2.7-3.6V
                 */
                if ((ocr[2] == 0x01) && (ocr[3] == 0xAA)) {
                    /*
                     * Wait for data packet in timeout of 1s - status used to
                     * indicate if a timeout occurred before operation
                     * completed.
                     */
                    status = SD_STATUS_ERROR;
                    timeout = 1000000/ClockP_getSystemTickPeriod();
                    startTime = ClockP_getSystemTicks();

                    do {
                        /* ACMD41 with HCS bit */
                        if ((sendCmd(object->spiHandle, CMD55, 0) <= 1) &&
                            (sendCmd(object->spiHandle, CMD41, 1UL << 30) == 0)) {
                            status = SD_STATUS_SUCCESS;
                            break;
                        }
                        currentTime = ClockP_getSystemTicks();
                    } while ((currentTime - startTime) < timeout);

                    /*
                     * Check CCS bit to determine which type of capacity we are
                     * dealing with
                     */
                    if ((status == SD_STATUS_SUCCESS) &&
                        sendCmd(object->spiHandle, CMD58, 0) == 0) {
                        status = spiTransfer(object->spiHandle, &ocr, &txDummy, 4);
                        if (status == SD_STATUS_SUCCESS) {
                            cardType = (ocr[0] & 0x40) ? SD_SDHC : SD_SDSC;
                        }
                    }
                }
            }
        }
        else {
            /* SDC Ver1 or MMC */
            /*
             * The card version is not SDC V2+ so check if we are dealing with a
             * SDC or MMC card
             */
            if ((sendCmd(object->spiHandle, CMD55, 0) <= 1) &&
                (sendCmd(object->spiHandle, CMD41, 0) <= 1)) {
                cardType = SD_SDSC;
            }
            else {
                cardType = SD_MMC;
            }

            /*
             * Wait for data packet in timeout of 1s - status used to
             * indicate if a timeout occurred before operation
             * completed.
             */
            status = SD_STATUS_ERROR;
            timeout = 1000000/ClockP_getSystemTickPeriod();
            startTime = ClockP_getSystemTicks();
            do {
                if (cardType == SD_SDSC) {
                    /* ACMD41 */
                    if ((sendCmd(object->spiHandle, CMD55, 0) <= 1) &&
                        (sendCmd(object->spiHandle, CMD41, 0) == 0)) {
                        status = SD_STATUS_SUCCESS;
                        break;
                    }
                }
                else {
                    /* CMD1 */
                    if (sendCmd(object->spiHandle, CMD1, 0) == 0) {
                        status = SD_STATUS_SUCCESS;
                        break;
                    }
                }
                currentTime = ClockP_getSystemTicks();
            } while ((currentTime - startTime) < timeout);

            /* Select R/W block length */
            if ((status == SD_STATUS_ERROR) ||
                (sendCmd(object->spiHandle, CMD16, SD_SECTOR_SIZE) != 0)) {
                cardType = SD_NOCARD;
            }
        }
    }

    object->cardType = cardType;

    deassertCS(hwAttrs);

    /* Check to see if a card type was determined */
    if (cardType == SD_NOCARD) {
        status = SD_STATUS_ERROR;
    }
    else {
        /* Reconfigure the SPI to operate @ 2.5 MHz */
        SPI_close(object->spiHandle);

        SPI_Params_init(&spiParams);
        spiParams.bitRate = 2500000;
        object->spiHandle = SPI_open(hwAttrs->spiIndex, &spiParams);
        status = (object->spiHandle == NULL) ? SD_STATUS_ERROR :
            SD_STATUS_SUCCESS;
    }

    SemaphoreP_post(object->lockSem);

    return (status);
}

/*
 *  ======== SDSPI_open ========
 */
SD_Handle SDSPI_open(SD_Handle handle, SD_Params *params)
{
    uintptr_t            key;
    SPI_Params           spiParams;
    SDSPI_Object        *object = handle->object;
    SDSPI_HWAttrs const *hwAttrs = handle->hwAttrs;

    key = HwiP_disable();

    if (object->isOpen) {
        HwiP_restore(key);

        return (NULL);
    }
    object->isOpen = true;

    HwiP_restore(key);

    object->lockSem = SemaphoreP_createBinary(1);
    if (object->lockSem == NULL) {
        object->isOpen = false;

        return (NULL);
    }

    /*
     * SPI is initially set to 400 kHz to perform SD initialization.  This is
     * is done to ensure compatibility with older SD cards.  Once the card has
     * been initialized (in SPI mode) the SPI peripheral will be closed &
     * reopened at 2.5 MHz.
     */
    SPI_Params_init(&spiParams);
    spiParams.bitRate = 400000;
    object->spiHandle = SPI_open(hwAttrs->spiIndex, &spiParams);
    if (object->spiHandle == NULL) {
        SDSPI_close(handle);

        return (NULL);
    }

    /* Configure the SPI CS pin as output set high */
    GPIO_setConfig(hwAttrs->spiCsGpioIndex,
        GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);

    return (handle);
}

/*
 *  ======== SDSPI_read ========
 */
int_fast16_t SDSPI_read(SD_Handle handle, void *buf, int_fast32_t sector,
    uint_fast32_t sectorCount)
{
    int_fast16_t         status = SD_STATUS_ERROR;
    SDSPI_Object        *object = handle->object;
    SDSPI_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (sectorCount == 0) {
        return (SD_STATUS_ERROR);
    }

    SemaphoreP_pend(object->lockSem, SemaphoreP_WAIT_FOREVER);

    /*
     * On a SDSC card, the sector address is a byte address on the SD Card
     * On a SDHC card, the sector addressing is via sector blocks
     */
    if (object->cardType != SD_SDHC) {
        /* Convert to byte address */
        sector *= SD_SECTOR_SIZE;
    }

    assertCS(hwAttrs);

    /* Single block read */
    if (sectorCount == 1) {
        if ((sendCmd(object->spiHandle, CMD17, sector) == 0) &&
            recvDataBlock(object->spiHandle, buf, SD_SECTOR_SIZE)) {
            status = SD_STATUS_SUCCESS;
        }
    }
    /* Multiple block read */
    else {
        if (sendCmd(object->spiHandle, CMD18, sector) == 0) {
            do {
                if (!recvDataBlock(object->spiHandle, buf, SD_SECTOR_SIZE)) {
                    break;
                }
                buf = (void *) (((uint32_t) buf) + SD_SECTOR_SIZE);
            } while (--sectorCount);

            /*
             * STOP_TRANSMISSION - order is important; always want to send
             * stop signal
             */
            if (sendCmd(object->spiHandle, CMD12, 0) == 0 && sectorCount == 0) {
                status = SD_STATUS_SUCCESS;
            }
        }
    }

    deassertCS(hwAttrs);

    SemaphoreP_post(object->lockSem);

    return (status);
}

/*
 *  ======== SDSPI_write ========
 */
int_fast16_t SDSPI_write(SD_Handle handle, const void *buf,
    int_fast32_t sector, uint_fast32_t sectorCount)
{
    int_fast16_t        status = SD_STATUS_SUCCESS;
    SDSPI_Object        *object = handle->object;
    SDSPI_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (sectorCount == 0) {
        return (SD_STATUS_ERROR);
    }

    SemaphoreP_pend(object->lockSem, SemaphoreP_WAIT_FOREVER);

    /*
     * On a SDSC card, the sector address is a byte address on the SD Card
     * On a SDHC card, the sector addressing is via sector blocks
     */
    if (object->cardType != SD_SDHC) {
        /* Convert to byte address if needed */
        sector *= SD_SECTOR_SIZE;
    }

    assertCS(hwAttrs);

    /* Single block write */
    if (sectorCount == 1) {
        if ((sendCmd(object->spiHandle, CMD24, sector) == 0) &&
            transmitDataBlock(object->spiHandle, (void *) buf, SD_SECTOR_SIZE,
                START_BLOCK_TOKEN)) {
            sectorCount = 0;
        }
    }
    /* Multiple block write */
    else {
        if ((object->cardType == SD_SDSC) || (object->cardType == SD_SDHC)) {
            if (sendCmd(object->spiHandle, CMD55, 0) != 0) {
                status = SD_STATUS_ERROR;
            }

            /* ACMD23 */
            if ((status == SD_STATUS_SUCCESS) &&
                (sendCmd(object->spiHandle, CMD23, sectorCount) != 0)) {
                status = SD_STATUS_ERROR;
            }
        }

        /* WRITE_MULTIPLE_BLOCK command */
        if ((status == SD_STATUS_SUCCESS) &&
            (sendCmd(object->spiHandle, CMD25, sector) == 0)) {
            do {
                if (!transmitDataBlock(object->spiHandle, (void *) buf,
                    SD_SECTOR_SIZE, START_MULTIBLOCK_TOKEN)) {
                    break;
                }
                buf = (void *) (((uint32_t) buf) + SD_SECTOR_SIZE);
            } while (--sectorCount);

            /* STOP_TRAN token */
            if (!transmitDataBlock(object->spiHandle, NULL, 0,
                STOP_MULTIBLOCK_TOKEN)) {
                sectorCount = 1;
            }
        }
    }

    deassertCS(hwAttrs);

    SemaphoreP_post(object->lockSem);

    return ((sectorCount) ? SD_STATUS_ERROR : SD_STATUS_SUCCESS);
}

/*
 *  ======== assertCS ========
 */
static inline void assertCS(SDSPI_HWAttrs const *hwAttrs)
{
    GPIO_write(hwAttrs->spiCsGpioIndex, 0);
}

/*
 *  ======== deassertCS ========
 */
static inline void deassertCS(SDSPI_HWAttrs const *hwAttrs)
{
    GPIO_write(hwAttrs->spiCsGpioIndex, 1);
}

/*
 *  ======== recvDataBlock ========
 *  Function to receive a block of data from the SDCard
 */
static bool recvDataBlock(SPI_Handle handle, void *buf, uint32_t count)
{
    uint8_t      rxBuf[2];
    uint8_t      txBuf[2] = {0xFF, 0xFF};
    int_fast16_t status;
    uint32_t     currentTime;
    uint32_t     startTime;
    uint32_t     timeout;

    /*
     * Wait for SD card to be ready up to 1s.  SD card is ready when the
     * START_BLOCK_TOKEN is received.
     */
    timeout = 1000000/ClockP_getSystemTickPeriod();
    startTime = ClockP_getSystemTicks();
    do {
        status = spiTransfer(handle, &rxBuf, &txBuf, 1);
        currentTime = ClockP_getSystemTicks();
    } while ((status == SD_STATUS_SUCCESS) && (rxBuf[0] == 0xFF) &&
        (currentTime - startTime) < timeout);

    if (rxBuf[0] != START_BLOCK_TOKEN) {
        /* Return error if valid data token was not received */
        return (false);
    }

    /* Receive the data block into buffer */
    if (spiTransfer(handle, buf, NULL, count) != SD_STATUS_SUCCESS) {
        return (false);
    }

    /* Read the 16 bit CRC, but discard it */
    if (spiTransfer(handle, &rxBuf, &txBuf, 2) != SD_STATUS_SUCCESS) {
        return (false);
    }

    /* Return with success */
    return (true);
}

/*
 *  ======== sendCmd ========
 *  Function to send a command to the SD card.  Command responses from
 *  SD card are returned.  (0xFF) is returned on failures.
 */
static uint8_t sendCmd(SPI_Handle handle, uint8_t cmd, uint32_t arg)
{
    uint8_t      i;
    uint8_t      rxBuf;
    uint8_t      txBuf[6];
    int_fast16_t status;

    if ((cmd != CMD0) && !waitUntilReady(handle)) {
        return (0xFF);
    }

    /* Setup SPI transaction */
    txBuf[0] = cmd;                  /* Command */
    txBuf[1] = (uint8_t)(arg >> 24); /* Argument[31..24] */
    txBuf[2] = (uint8_t)(arg >> 16); /* Argument[23..16] */
    txBuf[3] = (uint8_t)(arg >> 8);  /* Argument[15..8] */
    txBuf[4] = (uint8_t) arg;        /* Argument[7..0] */

    if (cmd == CMD0) {
        /* CRC for CMD0(0) */
        txBuf[5] = 0x95;
    }
    else if (cmd == CMD8) {
        /* CRC for CMD8(0x1AA) */
        txBuf[5] = 0x87;
    }
    else {
        /* Default CRC should be at least 0x01 */
        txBuf[5] = 0x01;
    }

    if (spiTransfer(handle, NULL, &txBuf, 6) != SD_STATUS_SUCCESS) {
        return (0xFF);
    }

    /* Prepare to receive SD card response (send 0xFF) */
    txBuf[0] = 0xFF;

    /*
     * CMD 12 has R1b response which transfers an additional
     * "busy" byte
     */
    if ((cmd == CMD12) &&
        (spiTransfer(handle, &rxBuf, &txBuf, 1) != SD_STATUS_SUCCESS)) {
            return (0xFF);
    }

    /* Wait for a valid response; 10 attempts */
    i = 10;
    do {
        status = spiTransfer(handle, &rxBuf, &txBuf, 1);
    } while ((status == SD_STATUS_SUCCESS) && (rxBuf & 0x80) && (--i));

    /* Return with the response value */
    return (rxBuf);
}

/*
 *  ======== sendInitialClockTrain ========
 *  Function to get the SDCard into SPI mode
 */
static int_fast16_t sendInitialClockTrain(SPI_Handle handle)
{
    uint8_t txBuf[10] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    /*
     * To put the SD card in SPI mode we must keep the TX line high while
     * toggling the clock line several times.  To do this we transmit 0xFF
     * 10 times.
     */
    return (spiTransfer(handle, NULL, &txBuf, 10));
}

/*
 *  ======== spiTransfer ========
 *  Returns SD_STATUS_SUCCESS when transfer is completed;
 *  SD_STATUS_ERROR otherwise.
 */
static int_fast16_t spiTransfer(SPI_Handle handle, void *rxBuf,
    void *txBuf, size_t count) {
    int_fast16_t    status;
    SPI_Transaction transaction;

    transaction.rxBuf = rxBuf;
    transaction.txBuf = txBuf;
    transaction.count = count;

    status = (SPI_transfer(handle, &transaction)) ?
        SD_STATUS_SUCCESS : SD_STATUS_ERROR;

    return (status);
}

/*
 *  ======== transmitDataBlock ========
 *  Function to transmit a block of data to the SD card.  A valid command
 *  token must be sent to the SD card prior to sending the data block.
 *  The available tokens are:
 *      START_BLOCK_TOKEN
 *      START_MULTIBLOCK_TOKEN
 *      STOP_MULTIBLOCK_TOKEN
 */
static bool transmitDataBlock(SPI_Handle handle, void *buf, uint32_t count,
    uint8_t token)
{
    uint8_t rxBuf;
    uint8_t txBuf[2] = {0xFF, 0xFF};

    if (!waitUntilReady(handle)) {
        return (false);
    }

    /* transmit data token */
    txBuf[0] = token;
    if (spiTransfer(handle, NULL, &txBuf, 1) != SD_STATUS_SUCCESS) {
        return (false);
    }

    /* Send data only when token != STOP_MULTIBLOCK_TOKEN */
    if (token != STOP_MULTIBLOCK_TOKEN) {
        /* Write data to the SD card */
        if (spiTransfer(handle, NULL, buf, count) != SD_STATUS_SUCCESS) {
            return (false);
        }

        /* Receive the 16 bit CRC, but discard it */
        txBuf[0] = (0xFF);
        if (spiTransfer(handle, NULL, &txBuf, 2) != SD_STATUS_SUCCESS) {
            return (false);
        }

        /* Receive data response token from SD card */
        if (spiTransfer(handle, &rxBuf, &txBuf, 1) != SD_STATUS_SUCCESS) {
            return (false);
        }

        /* Check data response; return error if data was rejected  */
        if ((rxBuf & 0x1F) != 0x05) {
            return (false);
        }
    }

    return (true);
}

/*
 *  ======== waitUntilReady ========
 *  Function to check if the SD card is busy.
 *
 *  Returns true if SD card is ready; false indicates the SD card is still busy
 *  & a timeout occurred.
 */
static bool waitUntilReady(SPI_Handle handle)
{
    uint8_t      rxDummy;
    uint8_t      txDummy = 0xFF;
    int_fast16_t status;
    uint32_t     currentTime;
    uint32_t     startTime;
    uint32_t     timeout;

    /* Wait up to 1s for data packet */
    timeout = 1000000/ClockP_getSystemTickPeriod();
    startTime = ClockP_getSystemTicks();
    do {
        status = spiTransfer(handle, &rxDummy, &txDummy, 1);
        currentTime = ClockP_getSystemTicks();
    } while ((status == SD_STATUS_SUCCESS) && (rxDummy != 0xFF) &&
        (currentTime - startTime) < timeout);

    return (rxDummy == 0xFF);
}
