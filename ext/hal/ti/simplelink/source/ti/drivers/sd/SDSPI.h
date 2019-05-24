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
/** ============================================================================
 *  @file       SDSPI.h
 *
 *  @brief      SD driver implementation built on the TI SPI driver.
 *
 *  The SDSPI header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/SD.h>
 *  #include <ti/drivers/sd/SDSPI.h>
 *  @endcode
 *
 *  Refer to @ref SD.h for a complete description of APIs & example of use.
 *
 *  This SD driver implementation can be used to communicate with SD cards
 *  via a SPI peripheral.  This driver leverages the TI SPI driver to transfer
 *  data to/from the host processor to the SD card.  The SD card chip select
 *  is also handled by this driver via the TI GPIO driver.  Both the SPI
 *  driver instance & the GPIO pin (used as chip select) must be specified
 *  in the SDSPI hardware attributes.
 *
 *  Note: This driver requires that the 'defaultTxBufValue' field in the SPI
 *  driver hardware attributes be set to '0xFF'.
 *
 *  ## Data location & alignment #
 *
 *  This driver relies on the TI SPI driver to configure the SPI peripheral &
 *  perform data transfers.  This means that data to be transferred must comply
 *  with rules & restrictions set SPI driver (memory alignment & DMA
 *  accessibility requirements).  Refer to @ref SPI.h & the device specific
 *  SPI implementation header files for details.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_sd_SDSPI__include
#define ti_drivers_sd_SDSPI__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/SD.h>
#include <ti/drivers/SPI.h>

/* SDSPI function table */
extern const SD_FxnTable SDSPI_fxnTable;

/*!
 *  @brief  SDSPI Hardware attributes
 *
 *  The SDSPI HWAttrs configuration structure contains the index of the SPI
 *  peripheral to be used for data transfers & the index of the GPIO Pin which
 *  will act as chip select.  This driver uses this information to:
 *      - configure & open the SPI driver instance for data transfers
 *      - select the SD card (via chip select) when performing data transfers
 *
 *  @struct SDSPI_HWAttrs
 *  An example configuration structure could look as the following:
 *  @code
 *  const SDSPI_HWAttrs sdspiHWAttrs[1] = {
 *      {
 *          // SPI driver index
 *          .spiIndex = 0,
 *
 *          //  GPIO driver pin index
 *          .spiCsGpioIndex = 3
 *      }
 *  };
 *  @endcode
 */
typedef struct SDSPI_HWAttrs_ {
    uint_least8_t spiIndex;
    uint16_t      spiCsGpioIndex;
} SDSPI_HWAttrs;

/*!
 *  @brief  SDSPI Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct SDSPI_Object_ {
    SemaphoreP_Handle lockSem;
    SPI_Handle        spiHandle;
    SD_CardType       cardType;
    bool              isOpen;
} SDSPI_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_sd_SDSPI__include */
