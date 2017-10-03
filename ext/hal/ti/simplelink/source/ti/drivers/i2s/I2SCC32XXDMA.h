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
 *  @file       I2SCC32XXDMA.h
 *
 *  @brief      I2S driver implementation for a CC32XX I2S controller
 *
 *  The I2S header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/I2S.h>
 *  #include <ti/drivers/i2s/I2SCC32XXDMA.h>
 *  @endcode
 *
 *  Refer to @ref I2S.h for a complete description of APIs & example of use.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_i2s_I2SCC32XXDMA__include
#define ti_drivers_i2s_I2SCC32XXDMA__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ti/drivers/I2S.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/utils/List.h>
#include <ti/drivers/dma/UDMACC32XX.h>

/*
 *  Macros defining possible I2S signal pin mux options
 *
 *  The bits in the pin mode macros are as follows:
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.  For example, I2SCC32XXDMA_PIN_02_McAFSX & 0xff = 1,
 *  which equals PIN_02 in driverlib pin.h.  By matching the PIN_xx defines in
 *  driverlib pin.h, we can pass the pin directly to the driverlib functions.
 *  The upper 8 bits of the macro correspond to the pin mux confg mode
 *  value for the pin to operate in the I2S mode.  For example, pin 2 is
 *  configured with mode 13 to operate as McAFSX.
 */
#define I2SCC32XXDMA_PIN_02_McAFSX  0x0d01 /*!< PIN 2 is used for McAFSX */
#define I2SCC32XXDMA_PIN_03_McACLK  0x0302 /*!< PIN 3 is used for McCLK  */
#define I2SCC32XXDMA_PIN_15_McAFSX  0x070e /*!< PIN 15 is used for McAFSX */
#define I2SCC32XXDMA_PIN_17_McAFSX  0x0610 /*!< PIN 17 is used for McAFSX */
#define I2SCC32XXDMA_PIN_21_McAFSX  0x0214 /*!< PIN 21 is used for McAFSX */
#define I2SCC32XXDMA_PIN_45_McAXR0  0x062c /*!< PIN 45 is used for McXR0  */
#define I2SCC32XXDMA_PIN_45_McAFSX  0x0c2c /*!< PIN 45 is used for McAFSX */
#define I2SCC32XXDMA_PIN_50_McAXR0  0x0431 /*!< PIN 50 is used for McXR0  */
#define I2SCC32XXDMA_PIN_50_McAXR1  0x0631 /*!< PIN 50 is used for McXR1  */
#define I2SCC32XXDMA_PIN_52_McACLK  0x0233 /*!< PIN 52 is used for McCLK  */
#define I2SCC32XXDMA_PIN_52_McAXR0  0x0433 /*!< PIN 52 is used for McXR0  */
#define I2SCC32XXDMA_PIN_53_McACLK  0x0234 /*!< PIN 53 is used for McCLK  */
#define I2SCC32XXDMA_PIN_53_McAFSX  0x0334 /*!< PIN 53 is used for McAFSX */
#define I2SCC32XXDMA_PIN_60_McAXR1  0x063b /*!< PIN 60 is used for McXR1  */
#define I2SCC32XXDMA_PIN_62_McACLKX 0x0d3d /*!< PIN 62 is used for McCLKX */
#define I2SCC32XXDMA_PIN_63_McAFSX  0x073e /*!< PIN 53 is used for McAFSX */
#define I2SCC32XXDMA_PIN_64_McAXR0  0x073f /*!< PIN 64 is used for McXR0  */

/**
 *  @addtogroup I2S_STATUS
 *  I2SCC32XXDMA_STATUS_* macros are command codes only defined in the
 *  I2SCC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/i2s/I2SCC32XXDMA.h>
 *  @endcode
 *  @{
 */

/* Add I2SCC32XXDMA_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup I2S_CMD
 *  I2SCC32XXDMA_CMD_* macros are command codes only defined in the
 *  I2SCC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/i2s/I2SCC32XXDMA.h>
 *  @endcode
 *  @{
 */

#define I2SCC32XXDMA_CMD_SET_ZEROBUF_LEN      (I2S_CMD_RESERVED + 0)
#define I2SCC32XXDMA_CMD_SET_EMPTYBUF_LEN     (I2S_CMD_RESERVED + 1)

/** @}*/

/* BACKWARDS COMPATIBILITY */
#define I2SCC32XXDMA_SET_ZEROBUF_LEN      I2SCC32XXDMA_CMD_SET_ZEROBUF_LEN
#define I2SCC32XXDMA_SET_EMPTYBUF_LEN     I2SCC32XXDMA_CMD_SET_EMPTYBUF_LEN
/* END BACKWARDS COMPATIBILITY */

/* Value for Invalid Index */
#define I2SCC32XXDMA_INDEX_INVALID      0xFF

/*Number of Serial data pins supported*/
#define I2SCC32XXDMA_NUM_SERIAL_PINS    2

/*!
 *  @brief
 *  I2SCC32XXDMA data size is used to determine how to configure the
 *  DMA data transfers. This field is to be only used internally.
 *
 *  I2SCC32XXDMA_16bit: txBuf and rxBuf are arrays of uint16_t elements
 *  I2SCC32XXDMA_32bit: txBuf and rxBuf are arrays of uint32_t elements
 */
typedef enum I2SCC32XXDMA_DataSize {
    I2SCC32XXDMA_16bit = 0,
    I2SCC32XXDMA_32bit = 1
} I2SCC32XXDMA_DataSize;


/* I2S function table pointer */
extern const I2S_FxnTable I2SCC32XXDMA_fxnTable;


/*!
 *  @brief      I2SCC32XXDMA Hardware attributes
 *
 *  These fields, with the exception of intPriority,
 *  are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CC32XXWare these definitions are found in:
 *      - inc/hw_memmap.h
 *      - inc/hw_ints.h
 *
 *  intPriority is the I2S peripheral's interrupt priority, as defined by the
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
 *  const I2SCC32XXDMA_HWAttrsV1 i2sCC32XXHWAttrs[] = {
 *      {
 *          .baseAddr = I2S_BASE,
 *          .intNum = INT_I2S,
 *          .intPriority = (~0),
 *          .rxChannelIndex = UDMA_CH4_I2S_RX,
 *          .txChannelIndex = UDMA_CH5_I2S_TX,
 *          .xr0Pin = I2SCC32XXDMA_PIN_64_McAXR0,
 *          .xr1Pin = I2SCC32XXDMA_PIN_50_McAXR1,
 *          .clkxPin = I2SCC32XXDMA_PIN_62_McACLKX,
 *          .clkPin = I2SCC32XXDMA_PIN_53_McACLK,
 *          .fsxPin = I2SCC32XXDMA_PIN_63_McAFSX,
 *      }
 *  };
 *  @endcode
 */
typedef struct I2SCC32XXDMA_HWAttrsV1 {
    /*! I2S Peripheral's base address */
    uint32_t baseAddr;
    /*! I2S Peripheral's interrupt vector */
    uint32_t intNum;
    /*! I2S Peripheral's interrupt priority */
    uint32_t intPriority;
    /*! uDMA controlTable receive channel index */
    unsigned long rxChannelIndex;
    /*! uDMA controlTable transmit channel index */
    unsigned long txChannelIndex;
    /*! I2S audio port data 0 pin */
    uint16_t xr0Pin;
    /*! I2S audio port data 1 pin */
    uint16_t xr1Pin;
    /*! I2S audio port clock O pin */
    uint16_t clkxPin;
    /*! I2S audio port data pin */
    uint16_t clkPin;
    /*! I2S audio port frame sync */
    uint16_t fsxPin;
} I2SCC32XXDMA_HWAttrsV1;

/*!
 *  @brief    CC32XX Serial Pin Configuration
 */
typedef struct I2SCC32XXDMA_SerialPinConfig {
    /*!< Pin number  */
    unsigned char         pinNumber;

    /*!< Mode the pin will operate(Rx/Tx) */
    I2S_PinMode           pinMode;

     /*!< Pin configuration in inactive state */
    I2S_SerInActiveConfig inActiveConfig;

} I2SCC32XXDMA_SerialPinConfig;

/*!
 *  @brief    CC32XX specific I2S Parameters
 */
typedef struct I2SCC32XXDMA_SerialPinParams {

    /*!< CC32XX Serial Pin Configuration */
    I2SCC32XXDMA_SerialPinConfig serialPinConfig[I2SCC32XXDMA_NUM_SERIAL_PINS];

} I2SCC32XXDMA_SerialPinParams;

/*!
 *  @brief      I2SCC32XXDMA Serial pin variables
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct I2SCC32XXDMA_SerialPinVars {
    I2S_DataMode        readWriteMode;
    /* Pointer to read/write callback */
    I2S_Callback        readWriteCallback;
    /* Timeout for read/write semaphore */
    uint32_t            readWriteTimeout;

}I2SCC32XXDMA_SerialPinVars;

/*!
 *  @brief      I2SCC32XXDMA Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct I2SCC32XXDMA_Object {
    /* I2S control variables */
    bool                   opened;              /* Has the obj been opened */
    uint32_t               operationMode;       /* Mode of operation of I2S */

    /* I2S serial pin variables */
    I2SCC32XXDMA_SerialPinVars serialPinVars[I2SCC32XXDMA_NUM_SERIAL_PINS];

    uint16_t               readIndex;           /* read channel Index */
    uint16_t               writeIndex;          /* write channel Index */

    I2SCC32XXDMA_DataSize  dmaSize;             /* Config DMA word size  */

    /* I2S OSAL objects */
    SemaphoreP_Handle      writeSem;            /* I2S write semaphore*/
    SemaphoreP_Handle      readSem;             /* I2S read semaphore */
    HwiP_Handle            hwiHandle;

    /*!< Length of zero buffer to write in case of no data */
    unsigned long          zeroWriteBufLength;

    /*!< Length of empty buffer to read in case of no data
         requested */
    unsigned long          emptyReadBufLength;

    /* Current Write buffer descriptor pointer */
    I2S_BufDesc            *currentWriteBufDesc;

    /* Previous Write Buffer descriptor pointer */
    I2S_BufDesc            *prevWriteBufDesc;

    /* Current Read buffer descriptor pointer */
    I2S_BufDesc            *currentReadBufDesc;

    /* Previous Read Buffer descriptor pointer */
    I2S_BufDesc            *prevReadBufDesc;

    /* UDMA */
    UDMACC32XX_Handle    dmaHandle;

    /* Lists for issue-reclaim mode */
    List_List             readActiveQueue;
    List_List             readDoneQueue;
    List_List             writeActiveQueue;
    List_List             writeDoneQueue;
} I2SCC32XXDMA_Object, *I2SCC32XXDMA_Handle;

/*!
 *  @brief  Function to initialize the I2S_Params struct to its defaults
 *
 * params->serialPinConfig[0].pinNumber         = 0;
 * params->serialPinConfig[0].pinMode           = I2S_PINMODE_RX;
 * params->serialPinConfig[0].inActiveConfig    = I2S_SERCONFIG_INACT_LOW_LEVEL;
 *
 * params->serialPinConfig[1].pinNumber         = 1;
 * params->serialPinConfig[1].pinMode           = I2S_PINMODE_TX;
 * params->serialPinConfig[1].inActiveConfig    = I2S_SERCONFIG_INACT_LOW_LEVEL;
 *
 *  @param  params  Parameter structure to initialize
 */
extern void I2SCC32XXDMA_Params_init(I2SCC32XXDMA_SerialPinParams *params);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_i2s_I2SCC32XXDMA__include */
