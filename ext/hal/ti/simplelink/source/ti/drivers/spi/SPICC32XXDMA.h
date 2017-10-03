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
/*!*****************************************************************************
 *  @file       SPICC32XXDMA.h
 *
 *  @brief      SPI driver implementation for a CC32XX SPI controller using the
 *              micro DMA controller.
 *
 *  The SPI header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/SPI.h>
 *  #include <ti/drivers/spi/SPICC32XXDMA.h>
 *  @endcode
 *
 *  Refer to @ref SPI.h for a complete description of APIs & example of use.
 *
 *  This SPI driver implementation is designed to operate on a CC32XX SPI
 *  controller using a micro DMA controller.
 *
 *  ## Frame Formats #
 *  This SPI controller supports 4 phase & polarity formats. Refer to the device
 *  specific data sheets & technical reference manuals for specifics on each
 *  format.
 *
 *  ## SPI Chip Select #
 *  This SPI controller supports a hardware chip select pin. Refer to the
 *  device's user manual on how this hardware chip select pin behaves in regards
 *  to the SPI frame format.
 *
 *  <table>
 *  <tr>
 *  <th>Chip select type</th>
 *  <th>SPI_MASTER mode</th>
 *  <th>SPI_SLAVE mode</th>
 *  </tr>
 *  <tr>
 *  <td>Hardware chip select</td>
 *  <td>No action is needed by the application to select the peripheral.</td>
 *  <td>See the device documentation on it's chip select requirements.</td>
 *  </tr>
 *  <tr>
 *  <td>Software chip select</td>
 *  <td>The application is responsible to ensure that correct SPI slave is
 *      selected before performing a SPI_transfer().</td>
 *  <td>See the device documentation on it's chip select requirements.</td>
 *  </tr>
 *  </table>
 *
 *  ## SPI data frames #
 *  SPI data frames can be any size from 4-bits to 32-bits.  The SPI data
 *  frame size is set in ::SPI_Params.dataSize passed to SPI_open.
 *  The SPICC32XXDMA driver implementation makes assumptions on the element
 *  size of the ::SPI_Transaction txBuf and rxBuf arrays, based on the data
 *  frame size.  If the data frame size is less than or equal to 8 bits,
 *  txBuf and rxBuf are assumed to be arrays of 8-bit uint8_t elements.
 *  If the data frame size is greater than 8 bits, but less than or equal
 *  to 16 bits, txBuf and rxBuf are assumed to be arrays of 16-bit uint16_t
 *  elements.  Otherwise, txBuf and rxBuf are assumed to point to 32-bit
 *  uint32_t elements.
 *
 *  data frame size  | buffer element size |
 *  --------------   | ------------------- |
 *  4-8 bits         | uint8_t             |
 *  9-16 bits        | uint16_t            |
 *  17-32 bits       | uint32_t            |
 *
 *  Data buffers in transactions (rxBuf & txBuf) must be address aligned
 *  according to the data frame size.  For example, if data frame is 9-bit
 *  (driver assumes buffers are uint16_t) rxBuf & txBuf must be aligned
 *  on a 16-bit address boundary, if data frame is 20-bit (driver assumes
 *  buffers are uint32_t) rxBuf & txBuf must be aligned on a 32-bit address
 *  boundary.
 *
 *  ## DMA Interrupts #
 *  This driver is designed to operate with the micro DMA. The micro DMA
 *  generates an interrupt on the perpheral's interrupt vector. This
 *  implementation automatically installs a DMA aware hardware ISR to service
 *  the assigned micro DMA channels.
 *
 *  ## DMA accessible memory #
 *  As this driver uses uDMA to transfer data/from data buffers, it is the
 *  responsibility of the application to ensure that these buffers reside in
 *  memory that is accessible by the DMA.
 *
 *  ## Scratch Buffers #
 *  A uint32_t scratch buffer is used to allow SPI_transfers where txBuf or
 *  rxBuf are NULL. Rather than requiring txBuf or rxBuf to have a dummy buffer
 *  of size of the transfer count, a single DMA accessible uint32_t scratch
 *  buffer is used. When rxBuf is NULL, the uDMA will transfer all the SPI data
 *  receives into the scratch buffer as a "bit-bucket". When txBuf is NULL, the
 *  scratch buffer is initialized to defaultTxBufValue so the uDMA will send
 *  some known value. Each SPI driver instance must have its own scratch buffer.
 *
 *  ## Polling SPI transfers #
 *  When used in blocking mode small SPI transfers are can be done by polling
 *  the peripheral & sending data frame-by-frame.  This will not block the task
 *  which requested the transfer, but instead immediately perform the transfer
 *  & return.  The minDmaTransferSize field in the hardware attributes is
 *  the threshold; if the transaction count is below the threshold a polling
 *  transfer is performed; otherwise a DMA transfer is done.  This is intended
 *  to reduce the overhead of setting up a DMA transfer to only send a few
 *  data frames.  Keep in mind that during polling transfers the current task
 *  is still being executed; there is no context switch to another task.
 *******************************************************************************
 */

#ifndef ti_drivers_spi_SPICC32XXDMA__include
#define ti_drivers_spi_SPICC32XXDMA__include

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/dma/UDMACC32XX.h>

/**
 *  @addtogroup SPI_STATUS
 *  SPICC32XXDMA_STATUS_* macros are command codes only defined in the
 *  SPICC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/sdspi/SPICC32XXDMA.h>
 *  @endcode
 *  @{
 */

/* Add SPICC32XXDMA_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup SPI_CMD
 *  SPICC32XXDMA_CMD_* macros are command codes only defined in the
 *  SPICC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/sdspi/SPICC32XXDMA.h>
 *  @endcode
 *  @{
 */

/* Add SPICC32XXDMA_CMD_* macros here */

/** @}*/

/*
 *  Macros defining possible SPI signal pin mux options
 *
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.  For example, SPICC32XXDMA_PIN_05_CLK & 0xff = 4,
 *  which equals PIN_05 in driverlib pin.h.  By matching the PIN_xx defines in
 *  driverlib pin.h, we can pass the pin directly to the driverlib functions.
 *  The upper 8 bits of the macro correspond to the pin mux confg mode
 *  value for the pin to operate in the SPI mode.
 *
 *  PIN_62 is special for the SDSPI driver when using an SD Boosterpack,
 *  as PIN_62 doesn't have an assigned SPI function yet the SD Boosterpack
 *  has it tied to the CS signal.
 */
#define SPICC32XXDMA_PIN_05_CLK     0x0704 /*!< PIN 5 is used for SPI CLK */
#define SPICC32XXDMA_PIN_06_MISO    0x0705 /*!< PIN 6 is used for MISO */
#define SPICC32XXDMA_PIN_07_MOSI    0x0706 /*!< PIN 7 is used for MOSI */
#define SPICC32XXDMA_PIN_08_CS      0x0707 /*!< PIN 8 is used for CS */
#define SPICC32XXDMA_PIN_45_CLK     0x072C /*!< PIN 45 is used for SPI CLK */
#define SPICC32XXDMA_PIN_50_CS      0x0931 /*!< PIN 50 is used for CS */
#define SPICC32XXDMA_PIN_52_MOSI    0x0833 /*!< PIN 52 is used for MOSI */
#define SPICC32XXDMA_PIN_53_MISO    0x0734 /*!< PIN 53 is used for MISO */

/*!
 * @brief Indicates a pin is not to be configured by the SPICC32XXDMA driver.
 */
#define SPICC32XXDMA_PIN_NO_CONFIG  0xFFFF

/* SPI function table pointer */
extern const SPI_FxnTable SPICC32XXDMA_fxnTable;

/*!
 *  @brief  SPICC32XXDMA Hardware attributes
 *
 *  These fields, with the exception of intPriority,
 *  are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CCWare these definitions are found in:
 *      - driverlib/prcm.h
 *      - driverlib/spi.h
 *      - driverlib/udma.h
 *      - inc/hw_memmap.h
 *      - inc/hw_ints.h
 *
 *  intPriority is the SPI peripheral's interrupt priority, as defined by the
 *  underlying OS.  It is passed unmodified to the underlying OS's interrupt
 *  handler creation code, so you need to refer to the OS documentation
 *  for usage.  For example, for SYS/BIOS applications, refer to the
 *  ti.sysbios.family.arm.m3.Hwi documentation for SYS/BIOS usage of
 *  interrupt priorities.  If the driver uses the ti.dpl interface
 *  instead of making OS calls directly, then the HwiP port handles the
 *  interrupt priority in an OS specific way.  In the case of the SYS/BIOS
 *  port, intPriority is passed unmodified to Hwi_create().
 *
 *  A sample structure is shown below:
 *  @code
 *  #if defined(__TI_COMPILER_VERSION__)
 *  #pragma DATA_ALIGN(scratchBuf, 32)
 *  #elif defined(__IAR_SYSTEMS_ICC__)
 *  #pragma data_alignment=32
 *  #elif defined(__GNUC__)
 *  __attribute__ ((aligned (32)))
 *  #endif
 *  uint32_t scratchBuf;
 *
 *  const SPICC32XXDMA_HWAttrsV1 SPICC32XXDMAHWAttrs[] = {
 *      {
 *          .baseAddr = GSPI_BASE,
 *          .intNum = INT_GSPI,
 *          .intPriority = (~0),
 *          .spiPRCM = PRCM_GSPI,
 *          .csControl = SPI_HW_CTRL_CS,
 *          .csPolarity = SPI_CS_ACTIVELOW,
 *          .pinMode = SPI_4PIN_MODE,
 *          .turboMode = SPI_TURBO_OFF,
 *          .scratchBufPtr = &scratchBuf,
 *          .defaultTxBufValue = 0,
 *          .rxChannelIndex = UDMA_CH6_GSPI_RX,
 *          .txChannelIndex = UDMA_CH7_GSPI_TX,
 *          .minDmaTransferSize = 100,
 *          .mosiPin = SPICC32XXDMA_PIN_07_MOSI,
 *          .misoPin = SPICC32XXDMA_PIN_06_MISO,
 *          .clkPin = SPICC32XXDMA_PIN_05_CLK,
 *          .csPin = SPICC32XXDMA_PIN_08_CS,
 *      },
 *      ...
 *  };
 *  @endcode
 */
typedef struct SPICC32XXDMA_HWAttrsV1 {
    /*! SPICC32XXDMA Peripheral's base address */
    uint32_t   baseAddr;

    /*! SPICC32XXDMA Peripheral's interrupt vector */
    uint32_t   intNum;

    /*! SPICC32XXDMA Peripheral's interrupt priority */
    uint32_t   intPriority;

    /*! SPI PRCM peripheral number */
    uint32_t   spiPRCM;

    /*! Specify if chip select line will be controlled by SW or HW */
    uint32_t   csControl;

    uint32_t   csPolarity;

    /*! Set peripheral to work in 3-pin or 4-pin mode */
    uint32_t   pinMode;

    /*! Enable or disable SPI TURBO mode */
    uint32_t   turboMode;

    /*! Address of a scratch buffer of size uint32_t */
    uint32_t  *scratchBufPtr;

    /*! Default TX value if txBuf == NULL */
    uint32_t   defaultTxBufValue;

    /*! uDMA RX channel index */
    uint32_t   rxChannelIndex;

    /*! uDMA TX channel index */
    uint32_t   txChannelIndex;

    /*! Minimum amout of data to start a uDMA transfer */
    uint32_t   minDmaTransferSize;

    /*! GSPI MOSI pin assignment */
    uint16_t   mosiPin;

    /*! GSPI MISO pin assignment */
    uint16_t   misoPin;

    /*! GSPI CLK pin assignment */
    uint16_t   clkPin;

    /*! GSPI CS pin assignment */
    uint16_t   csPin;
} SPICC32XXDMA_HWAttrsV1;

/*!
 *  @brief  SPICC32XXDMA Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct SPICC32XXDMA_Object {
    HwiP_Handle        hwiHandle;
    Power_NotifyObj    notifyObj;
    SemaphoreP_Handle  transferComplete;
    SPI_CallbackFxn    transferCallbackFxn;
    SPI_Transaction   *transaction;
    UDMACC32XX_Handle  dmaHandle;

    size_t             amtDataXferred;
    size_t             currentXferAmt;
    uint32_t           bitRate;
    uint32_t           dataSize;
    uint32_t           transferTimeout;

    SPI_Mode           spiMode;
    SPI_TransferMode   transferMode;
    SPI_FrameFormat    frameFormat;

    bool               cancelInProgress;
    bool               isOpen;
    uint8_t            rxFifoTrigger;
    uint8_t            txFifoTrigger;
} SPICC32XXDMA_Object, *SPICC32XXDMA_Handle;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_spi_SPICC32XXDMA__include */
