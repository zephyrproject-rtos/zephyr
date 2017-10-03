/*
 * Copyright (c) 2014-2017, Texas Instruments Incorporated
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
 *  @file       UARTCC32XXDMA.h
 *
 *  @brief      UART driver implementation for a CC32XX UART controller, using
 *              the micro DMA controller.
 *
 *  The UART header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/UART.h>
 *  #include <ti/drivers/uart/UARTCC32XXDMA.h>
 *  @endcode
 *
 *  Refer to @ref UART.h for a complete description of APIs & example of use.
 *
 *
 *  # Device Specific Pin Mode Macros #
 *  This header file contains pin mode definitions used to specify the
 *  UART TX and RX pin assignment in the UARTCC32XXDMA_HWAttrsV1 structure.
 *  Please refer to the CC32XX Techincal Reference Manual for details on pin
 *  multiplexing.
 *
 *  # Flow Control #
 *  To enable Flow Control, the RTS and CTS pins must be assigned in the
 *  ::UARTCC32XX_HWAttrsV1.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_uart_UARTCC32XXDMA__include
#define ti_drivers_uart_UARTCC32XXDMA__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>

#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/dma/UDMACC32XX.h>


/*!
 * @brief Indicates a pin is not being used
 *
 *  If hardware flow control is not being used, the UART CTS and RTS
 *  pins should be set to UARTCC32XX_PIN_UNASSIGNED.
 */
#define UARTCC32XXDMA_PIN_UNASSIGNED   0xFFF
/*
 *  The bits in the pin mode macros are as follows:
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.  For example, UARTCC32XXDMA_PIN_01_UART1_TX & 0xff = 0,
 *  which equals PIN_01 in driverlib pin.h.  By matching the PIN_xx defines in
 *  driverlib pin.h, we can pass the pin directly to the driverlib functions.
 *  The upper 8 bits of the macro correspond to the pin mux confg mode
 *  value for the pin to operate in the UART mode.  For example, pin 1 is
 *  configured with mode 7 to operate as UART1 TX.
 */
#define UARTCC32XXDMA_PIN_01_UART1_TX  0x700 /*!< PIN 1 is used for UART1 TX */
#define UARTCC32XXDMA_PIN_02_UART1_RX  0x701 /*!< PIN 2 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_03_UART0_TX  0x702 /*!< PIN 3 is used for UART0 TX */
#define UARTCC32XXDMA_PIN_04_UART0_RX  0x703 /*!< PIN 4 is used for UART0 RX */
#define UARTCC32XXDMA_PIN_07_UART1_TX  0x506 /*!< PIN 7 is used for UART1 TX */
#define UARTCC32XXDMA_PIN_08_UART1_RX  0x507 /*!< PIN 8 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_16_UART1_TX  0x20F /*!< PIN 16 is used for UART1 TX */
#define UARTCC32XXDMA_PIN_17_UART1_RX  0x210 /*!< PIN 17 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_45_UART0_RX  0x92C /*!< PIN 45 is used for UART0 RX */
#define UARTCC32XXDMA_PIN_45_UART1_RX  0x22C /*!< PIN 45 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_53_UART0_TX  0x934 /*!< PIN 53 is used for UART0 TX */
#define UARTCC32XXDMA_PIN_55_UART0_TX  0x336 /*!< PIN 55 is used for UART0 TX */
#define UARTCC32XXDMA_PIN_55_UART1_TX  0x636 /*!< PIN 55 is used for UART1 TX */
#define UARTCC32XXDMA_PIN_57_UART0_RX  0x338 /*!< PIN 57 is used for UART0 RX */
#define UARTCC32XXDMA_PIN_57_UART1_RX  0x638 /*!< PIN 57 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_58_UART1_TX  0x639 /*!< PIN 58 is used for UART1 TX */
#define UARTCC32XXDMA_PIN_59_UART1_RX  0x63A /*!< PIN 59 is used for UART1 RX */
#define UARTCC32XXDMA_PIN_62_UART0_TX  0xB3D /*!< PIN 62 is used for UART0 TX */

/*
 *  Flow control pins.
 */
#define UARTCC32XXDMA_PIN_50_UART0_CTS  0xC31 /*!< PIN 50 is used for UART0 CTS */
#define UARTCC32XXDMA_PIN_50_UART0_RTS  0x331 /*!< PIN 50 is used for UART0 RTS */
#define UARTCC32XXDMA_PIN_50_UART1_RTS  0xA31 /*!< PIN 50 is used for UART1 RTS */
#define UARTCC32XXDMA_PIN_52_UART0_RTS  0x633 /*!< PIN 52 is used for UART0 RTS */
#define UARTCC32XXDMA_PIN_61_UART0_RTS  0x53C /*!< PIN 61 is used for UART0 RTS */
#define UARTCC32XXDMA_PIN_61_UART0_CTS  0x63C /*!< PIN 61 is used for UART0 CTS */
#define UARTCC32XXDMA_PIN_61_UART1_CTS  0x33C /*!< PIN 61 is used for UART1 CTS */
#define UARTCC32XXDMA_PIN_62_UART0_RTS  0xA3D /*!< PIN 62 is used for UART0 RTS */
#define UARTCC32XXDMA_PIN_62_UART1_RTS  0x33D /*!< PIN 62 is used for UART1 RTS */

/*!
 * @brief No hardware flow control
 */
#define UARTCC32XXDMA_FLOWCTRL_NONE 0

/*!
 * @brief Hardware flow control
 */
#define UARTCC32XXDMA_FLOWCTRL_HARDWARE 1

/**
 *  @addtogroup UART_STATUS
 *  UARTCC32XXDMA_STATUS_* macros are command codes only defined in the
 *  UARTCC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/uart/UARTCC32XXDMA.h>
 *  @endcode
 *  @{
 */

/* Add UARTCC32XXDMA_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup UART_CMD
 *  UARTCC32XXDMA_CMD_* macros are command codes only defined in the
 *  UARTCC32XXDMA.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/uart/UARTCC32XXDMA.h>
 *  @endcode
 *  @{
 */

/*!
 * @brief Command used by UART_control to determines
 * whether the UART transmitter is busy or not
 *
 * With this command code, @b arg is a pointer to a @c bool.
 * @b *arg contains @c true if the UART is transmitting,
 * else @c false if all transmissions are complete.
 */
#define UARTCC32XXDMA_CMD_IS_BUSY                       (UART_CMD_RESERVED + 0)


/*!
 * @brief Command used by UART_control to determines
 * if there are any characters in the receive FIFO
 *
 * With this command code, @b arg is a pointer to a @c bool.
 * @b *arg contains @c true if there is data in the receive FIFO,
 * or @c false if there is no data in the receive FIFO.
 */
#define UARTCC32XXDMA_CMD_IS_RX_DATA_AVAILABLE          (UART_CMD_RESERVED + 1)


/*!
 * @brief Command used by UART_control to determines
 * if there is any space in the transmit FIFO
 *
 * With this command code, @b arg is a pointer to a @c bool.
 * @b *arg contains @c true if there is space available in the transmit FIFO,
 * or @c false if there is no space available in the transmit FIFO.
 */
#define UARTCC32XXDMA_CMD_IS_TX_SPACE_AVAILABLE         (UART_CMD_RESERVED + 2)


/** @}*/

/* UART function table pointer */
extern const UART_FxnTable UARTCC32XXDMA_fxnTable;

/*!
 *  @brief      UARTCC32XXDMA Hardware attributes
 *
 *  These fields, with the exception of intPriority,
 *  are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CC32XXWare these definitions are found in:
 *      - inc/hw_memmap.h
 *      - inc/hw_ints.h
 *
 *  intPriority is the UART peripheral's interrupt priority, as defined by the
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
 *  const UARTCC32XXDMA_HWAttrsV1 uartCC32XXHWAttrs[] = {
 *      {
 *          .baseAddr = UARTA0_BASE,
 *          .intNum = INT_UARTA0,
 *          .intPriority = (~0),
 *          .flowControl = UARTCC32XXDMA_FLOWCTRL_NONE,
 *          .rxChannelIndex = DMA_CH8_UARTA0_RX,
 *          .txChannelIndex = UDMA_CH9_UARTA0_TX,
 *          .rxPin = UARTCC32XXDMA_PIN_57_UART0_RX,
 *          .txPin = UARTCC32XXDMA_PIN_55_UART0_TX,
 *          .rtsPin = UARTCC32XXDMA_PIN_UNASSIGNED,
 *          .ctsPin = UARTCC32XX_DMA_PIN_UNASSIGNED
 *      },
 *      {
 *          .baseAddr = UARTA1_BASE,
 *          .intNum = INT_UARTA1,
 *          .intPriority = (~0),
 *          .flowControl = UARTCC32XXDMA_FLOWCTRL_HARDWARE,
 *          .rxChannelIndex = UDMA_CH10_UARTA1_RX,
 *          .txChannelIndex = UDMA_CH11_UARTA1_TX,
 *          .rxPin = UARTCC32XXDMA_PIN_08_UART1_RX,
 *          .txPin = UARTCC32XXDMA_PIN_07_UART1_TX,
 *          .rtsPin = UARTCC32XXDMA_PIN_50_UART1_RTS,
 *          .ctsPin = UARTCC32XXDMA_PIN_61_UART1_CTS
 *      },
 *  };
 *  @endcode
 */
typedef struct UARTCC32XXDMA_HWAttrsV1 {
    /*! UART Peripheral's base address */
    unsigned int baseAddr;
    /*! UART Peripheral's interrupt vector */
    unsigned int intNum;
    /*! UART Peripheral's interrupt priority */
    unsigned int intPriority;
    /*! Hardware flow control setting defined by driverlib */
    uint32_t        flowControl;
    /*! uDMA controlTable receive channel index */
    unsigned long rxChannelIndex;
    /*! uDMA controlTable transmit channel index */
    unsigned long txChannelIndex;
    /*! UART RX pin assignment */
    uint16_t        rxPin;
    /*! UART TX pin assignment */
    uint16_t        txPin;
    /*! UART clear to send (CTS) pin assignment */
    uint16_t        ctsPin;
    /*! UART request to send (RTS) pin assignment */
    uint16_t        rtsPin;
} UARTCC32XXDMA_HWAttrsV1;

/*!
 *  @brief      UARTCC32XXDMA Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct UARTCC32XXDMA_Object {
    /* UART control variables */
    bool                 opened;           /* Has the obj been opened */
    UART_Mode            readMode;         /* Mode for all read calls */
    UART_Mode            writeMode;        /* Mode for all write calls */
    unsigned int         readTimeout;      /* Timeout for read semaphore */
    unsigned int         writeTimeout;     /* Timeout for write semaphore */
    UART_Callback        readCallback;     /* Pointer to read callback */
    UART_Callback        writeCallback;    /* Pointer to write callback */
    UART_ReturnMode      readReturnMode;   /* Receive return mode */
    UART_DataMode        readDataMode;     /* Type of data being read */
    UART_DataMode        writeDataMode;    /* Type of data being written */
    uint32_t             baudRate;         /* Baud rate for UART */
    UART_LEN             dataLength;       /* Data length for UART */
    UART_STOP            stopBits;         /* Stop bits for UART */
    UART_PAR             parityType;       /* Parity bit type for UART */
    UART_Echo            readEcho;         /* Echo received data back */

    /* UART write variables */
    const void          *writeBuf;         /* Buffer data pointer */
    size_t               writeCount;       /* Number of Chars sent */
    size_t               writeSize;        /* Chars remaining in buffer */

    /* UART receive variables */
    void                *readBuf;          /* Buffer data pointer */
    size_t               readCount;        /* Number of Chars read */
    size_t               readSize;         /* Chars remaining in buffer */

    /* Semaphores for blocking mode */
    SemaphoreP_Handle    writeSem;         /* UART write semaphore */
    SemaphoreP_Handle    readSem;          /* UART read semaphore */

    HwiP_Handle    hwiHandle;

    /* For Power management */
    ClockP_Handle         txFifoEmptyClk;  /* UART TX FIFO empty clock */
    Power_NotifyObj       postNotify;      /* LPDS wake-up notify object */
    unsigned int          powerMgrId;      /* Determined from base address */
    PowerCC32XX_ParkState prevParkTX;      /* Previous park state TX pin */
    uint16_t              txPin;           /* TX pin ID */
    PowerCC32XX_ParkState prevParkRTS;     /* Previous park state of RTS pin */
    uint16_t              rtsPin;           /* RTS pin ID */

    /* UDMA */
    UDMACC32XX_Handle    dmaHandle;
} UARTCC32XXDMA_Object, *UARTCC32XXDMA_Handle;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_uart_UARTCC32XXDMA__include */
