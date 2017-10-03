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

/** ============================================================================
 *  @file       SDHostCC32XX.h
 *
 *  @brief      SDHost driver implementation for CC32XX devices.
 *
 *  The SDHost header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/SD.h>
 *  #include <ti/drivers/sd/SDHostCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref SD.h for a complete description of APIs & example of use.
 *
 *  This SDHost driver implementation is designed to operate on a CC32XX
 *  SD Host controller using a micro DMA controller.
 *
 *  Note: The driver API's are not thread safe and must not be accessed through
 *      multiple threads without the use of mutexes.
 *
 *  ## DMA buffer alignment #
 *
 *  When performing disk operations with a word aligned buffer the driver will
 *  make transfers using the DMA controller. Alternatively, if the buffer is
 *  not aligned, the data will be copied to the internal SD Host controller
 *  buffer using a polling method.
 *
 *  ## DMA Interrupts #
 *
 *  When DMA is used, the micro DMA controller generates and IRQ on the
 *  perpheral's interrupt vector. This implementation automatically installs
 *  a DMA interrupt to service the assigned micro DMA channels.
 *
 *  ## DMA accessible memory #
 *
 *  When DMA is used, it is the responsibility of the application to ensure
 *  that read/write buffers reside in memory that is accessible by the DMA.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_sd_SDHostCC32XX__include
#define ti_drivers_sd_SDHostCC32XX__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ti/drivers/SD.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/dma/UDMACC32XX.h>

#define SDHostCC32XX_PIN_06_SDCARD_DATA  0x0805
#define SDHostCC32XX_PIN_07_SDCARD_CLK   0x0806
#define SDHostCC32XX_PIN_08_SDCARD_CMD   0x0807
#define SDHostCC32XX_PIN_01_SDCARD_CLK   0x0600
#define SDHostCC32XX_PIN_02_SDCARD_CMD   0x0601
#define SDHostCC32XX_PIN_64_SDCARD_DATA  0x063f

/* SDHost function table */
extern const SD_FxnTable sdHostCC32XX_fxnTable;

/*!
 *  @brief  SDHostCC32XX Hardware attributes
 *
 *  The SDHostCC32XX configuration structure is passed to the SDHostCC32XX
 *  driver implementation with hardware specifics regarding GPIO Pins and Ports
 *  to be used.
 *
 *  The SDHostCC32XX driver uses this information to:
 *  - Configure and reconfigure specific ports/pins to initialize the SD Card
 *    for SD mode
 *  - Identify which GPIO port and pin is used for the SDHost clock, data and
 *    command lines
 *
 *  These fields are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CC32xxWare these definitions are found in:
 *      - inc/hw_memmap.h
 *      - driverlib/pin.h
 *
 *  @struct SDHostCC32XX_HWAttrs
 *  An example configuration structure could look as the following:
 *  @code
 *  const SDHostCC32XX_HWAttrsV1 sdhostCC32XXHWattrs[] = {
 *      {
 *          .clkRate = 8000000,
 *          .intPriority = ~0,
 *          .baseAddr = SDHOST_BASE,
 *          .rxChIdx = UDMA_CH23_SDHOST_RX,
 *          .txChIdx = UDMA_CH24_SDHOST_TX,
 *          .dataPin = SDHostCC32XX_PIN_06_SDCARD_DATA,
 *          .cmdPin = SDHostCC32XX_PIN_08_SDCARD_CMD,
 *          .clkPin = SDHostCC32XX_PIN_07_SDCARD_CLK,
 *      }
 *  };
 *  @endcode
 */
typedef struct SDHostCC32XX_HWAttrsV1 {
    /*!< SD interface clock rate */
    uint_fast32_t clkRate;

    /*!< Internal SDHost ISR command/transfer priorty */
    int_fast32_t intPriority;

    /*!< SDHost Peripheral base address */
    uint_fast32_t baseAddr;

    /*!< uDMA controlTable receive channel index */
    unsigned long rxChIdx;

    /*!< uDMA controlTable transmit channel index */
    unsigned long txChIdx;

    /*!< SD Host Data pin */
    uint32_t dataPin;

    /*!< SD Host CMD pin */
    uint32_t cmdPin;

    /*!< SD Host CLK pin */
    uint32_t clkPin;
} SDHostCC32XX_HWAttrsV1;

/*!
 *  @brief  SDHostCC32XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct SDHostCC32XX_Object {
    /* Relative Card Address */
    uint_fast32_t              rca;
    /* Write data pointer */
    const uint_fast32_t       *writeBuf;
    /* Number of sectors written */
    volatile uint_fast32_t     writeSecCount;
    /* Read data pointer */
    uint_fast32_t             *readBuf;
    /* Number of sectors read */
    volatile uint_fast32_t     readSecCount;
    /*
     *  Semaphore to suspend thread execution when waiting for SD Commands
     *  or data transfers to complete.
     */
    SemaphoreP_Handle      cmdSem;
    /*
     *  SD Card interrupt handle.
     */
    HwiP_Handle            hwiHandle;
    /* Determined from base address */
    unsigned int           powerMgrId;
    /* LPDS wake-up notify object */
    Power_NotifyObj        postNotify;
    /* Previous park state SDCARD_CLK pin */
    PowerCC32XX_ParkState  prevParkCLK;
    /* SDCARD_CLK pin */
    uint16_t               clkPin;
    /* UDMA Handle */
    UDMACC32XX_Handle      dmaHandle;
    /* SD Card command state */
    volatile int_fast8_t   stat;
    /* State of the driver (open or closed) */
    bool                   isOpen;
    /* SDCard Card Command Class(CCC) */
    SD_CardType            cardType;
} SDHostCC32XX_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_sd_SDHostCC32XX__include */
