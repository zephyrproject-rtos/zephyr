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
/** ============================================================================
 *  @file       SDSPICC32XX.h
 *
 *  @brief      SDSPI driver implementation for a CC32XX SPI peripheral used
 *              with the SDSPI driver.
 *
 *  The SDSPI header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/SDSPI.h>
 *  #include <ti/drivers/sdspi/SDSPICC32XX.h>
 *  @endcode
 *
 *  Refer to @ref SDSPI.h for a complete description of APIs & example of use.
 *
 *  This SDSPI driver implementation is designed to operate on a CC32XX SPI
 *  controller using a polling method.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_sdspi_SDSPICC32XX__include
#define ti_drivers_sdspi_SDSPICC32XX__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ti/drivers/SDSPI.h>
#include <ti/drivers/Power.h>

#include <third_party/fatfs/ff.h>
#include <third_party/fatfs/diskio.h>

#define SDSPICC32XX_GPIO0  (0x00 << 16)
#define SDSPICC32XX_GPIO1  (0x01 << 16)
#define SDSPICC32XX_GPIO2  (0x02 << 16)
#define SDSPICC32XX_GPIO3  (0x03 << 16)
#define SDSPICC32XX_GPIO4  (0x04 << 16)
#define SDSPICC32XX_GPIO5  (0x05 << 16)
#define SDSPICC32XX_GPIO6  (0x06 << 16)
#define SDSPICC32XX_GPIO7  (0x07 << 16)
#define SDSPICC32XX_GPIO8  (0x10 << 16)
#define SDSPICC32XX_GPIO9  (0x11 << 16)
#define SDSPICC32XX_GPIO10  (0x12 << 16)
#define SDSPICC32XX_GPIO11  (0x13 << 16)
#define SDSPICC32XX_GPIO12  (0x14 << 16)
#define SDSPICC32XX_GPIO13  (0x15 << 16)
#define SDSPICC32XX_GPIO14  (0x16 << 16)
#define SDSPICC32XX_GPIO15  (0x17 << 16)
#define SDSPICC32XX_GPIO16  (0x20 << 16)
#define SDSPICC32XX_GPIO17  (0x21 << 16)
#define SDSPICC32XX_GPIO22  (0x26 << 16)
#define SDSPICC32XX_GPIO23  (0x27 << 16)
#define SDSPICC32XX_GPIO24  (0x30 << 16)
#define SDSPICC32XX_GPIO25  (0x31 << 16)
#define SDSPICC32XX_GPIO28  (0x34 << 16)
#define SDSPICC32XX_GPIO29  (0x35 << 16)
#define SDSPICC32XX_GPIO30  (0x36 << 16)
#define SDSPICC32XX_GPIO31  (0x37 << 16)

#define SDSPICC32XX_GPIONONE  (0xFF << 16)

/*
 *  We need to encode a pin's GPIO function along with the SPI function,
 *  since certain SPI functions need to be handled via manual programming
 *  of a GPIO pin.
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.  For example, SDSPICC32XX_PIN_05_CLK & 0xff = 4,
 *  which equals PIN_05 in driverlib pin.h.  By matching the PIN_xx defines in
 *  driverlib pin.h, we can pass the pin directly to the driverlib functions.
 *  The upper 8 bits of the macro correspond to the pin mux confg mode
 *  value for the pin to operate in the SPI mode.  For pins used as CS,
 *  the upper 16 bits correspond to the GPIO pin.
 *
 *  PIN_62 is special for the SDSPI driver when using an SD Boosterpack,
 *  as PIN_62 doesn't have an assigned SPI function yet the SD Boosterpack
 *  has it tied to the CS signal.
 */
#define SDSPICC32XX_PIN_05_CLK    0x0704 /*!< PIN 5 is used for SPI CLK */
#define SDSPICC32XX_PIN_06_MISO   0x0705 /*!< PIN 6 is used for MISO */
#define SDSPICC32XX_PIN_07_MOSI   0x0706 /*!< PIN 7 is used for MOSI */
#define SDSPICC32XX_PIN_08_CS    (SDSPICC32XX_GPIO17 | 0x0707) /*!< PIN 8 is used for CS */
#define SDSPICC32XX_PIN_62_GPIO  (SDSPICC32XX_GPIO7  | 0x003d) /*!< PIN 62 is used for CS */
#define SDSPICC32XX_PIN_45_CLK    0x072c /*!< PIN 45 is used for SPI CLK */
#define SDSPICC32XX_PIN_50_CS    (SDSPICC32XX_GPIO0  | 0x0931) /*!< PIN 50 is used for CS */
#define SDSPICC32XX_PIN_52_MOSI   0x0833 /*!< PIN 52 is used for MOSI */
#define SDSPICC32XX_PIN_53_MISO   0x0734 /*!< PIN 53 is used for MISO */

/**
 *  @addtogroup SDSPI_STATUS
 *  SDSPICC32XX_STATUS_* macros are command codes only defined in the
 *  SDSPICC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/sdspi/SDSPICC32XX.h>
 *  @endcode
 *  @{
 */

/* Add SDSPICC32XX_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup SDSPI_CMD
 *  SDSPICC32XX_CMD_* macros are command codes only defined in the
 *  SDSPICC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/sdspi/SDSPICC32XX.h>
 *  @endcode
 *  @{
 */

/* Add SDSPICC32XX_CMD_* macros here */

/** @}*/

typedef unsigned long SDSPIBaseAddrType;
typedef unsigned long SDSPIDataType;

/* SDSPI function table */
extern const SDSPI_FxnTable SDSPICC32XX_fxnTable;

/*!
 *  @brief  SD Card type inserted
 */
typedef enum SDSPICC32XX_CardType {
    SDSPICC32XX_NOCARD = 0, /*!< Unrecognized Card */
    SDSPICC32XX_MMC = 1,    /*!< Multi-media Memory Card (MMC) */
    SDSPICC32XX_SDSC = 2,   /*!< Standard SDCard (SDSC) */
    SDSPICC32XX_SDHC = 3    /*!< High Capacity SDCard (SDHC) */
} SDSPICC32XX_CardType;

/*!
 *  @brief  SDSPICC32XX Hardware attributes
 *
 *  The SDSPICC32XX configuration structure describes to the SDSPICC32XX
 *  driver implementation hardware specifies on which SPI peripheral, GPIO Pins
 *  and Ports are to be used.
 *
 *  The SDSPICC32XX driver uses this information to:
 *  - configure and reconfigure specific ports/pins to initialize the SD Card
 *    for SPI mode
 *  - identify which SPI peripheral is used for data communications
 *  - identify which GPIO port and pin is used for the SPI chip select
 *    mechanism
 *  - identify which GPIO port and pin is concurrently located on the SPI's MOSI
 *    (TX) pin.
 *
 *  @remark
 *  To initialize the SD Card into SPI mode, the SDSPI driver changes the SPI's
 *  MOSI pin into a GPIO pin so it can kept driven HIGH while the SPI SCK pin
 *  can toggle. After the initialization, the TX pin is reverted back to the SPI
 *  MOSI mode.
 *
 *  These fields are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CCWare these definitions are found in:
 *      - inc/hw_memmap.h
 *      - inc/hw_ints.h
 *      - driverlib/gpio.h
 *      - driverlib/pin.h
 *      - driverlib/prcm.h
 *
 *  @struct SDSPICC32XX_HWAttrs
 *  An example configuration structure could look as the following:
 *  @code
 *  const SDSPICC32XX_HWAttrs sdspiCC32XXHWattrs = {
 *      {
 *            .baseAddr = GSPI_BASE,     // SSI Peripheral's base address
 *            .spiPRCM = PRCM_GSPI,      // SPI PRCM peripheral number
 *
 *            .clkPin = SDSPICC32XX_PIN_05_CLK,
 *            .mosiPin = SDSPICC32XX_PIN_07_MOSI,
 *            .misoPin = SDSPICC32XX_PIN_06_MISO,
 *            .csPin = SDSPICC32XX_PIN_62_GPIO,
 *      }
 *  };
 *  @endcode
 */
typedef struct SDSPICC32XX_HWAttrsV1 {
    /*!< SPI Peripheral base address */
    uint32_t baseAddr;
    /*! SPI PRCM peripheral number */
    uint32_t spiPRCM;
    /*!< GSPI CLK pin assignment */
    uint32_t clkPin;
    /*!< GSPI MOSI pin assignment & GPIO encoding when using MOSI as GPIO */
    uint32_t mosiPin;
    /*!< GSPI MISO pin assignment */
    uint32_t misoPin;
    /*!< CS pin assignment & GPIO encoding for pin use as GPIO */
    uint32_t csPin;
} SDSPICC32XX_HWAttrsV1;

/*!
 *  @brief  SDSPICC32XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct SDSPICC32XX_Object {
    uint32_t              driveNumber;   /* Drive number used by FatFs */
    DSTATUS               diskState;     /* Disk status */
    SDSPICC32XX_CardType  cardType;      /* SDCard Card Command Class (CCC) */
    uint32_t              bitRate;       /* SPI bus bit rate (Hz) */
    FATFS                 filesystem;    /* FATFS data object */
    uint_fast16_t         spiPowerMgrId;    /* Determined from base address */
    uint_fast16_t         gpioCsPowerMgrId; /* Determined from base address */
    Power_NotifyObj       postNotify;    /* LPDS wake-up notify object */
} SDSPICC32XX_Object, *SDSPICC32XX_Handle;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_sdspi_SDSPICC32XX__include */
