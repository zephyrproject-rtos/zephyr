/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
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
 *  @file       UART.h
 *  @brief      UART driver interface
 *
 *  To use the UART driver, ensure that the correct driver library for your
 *  device is linked in and include this header file as follows:
 *  @code
 *  #include <ti/drivers/UART.h>
 *  @endcode
 *
 *  This module serves as the main interface for applications.  Its purpose
 *  is to redirect the UART APIs to specific driver implementations
 *  which are specified using a pointer to a #UART_FxnTable.
 *
 *  # Overview #
 *  A UART is used to translate data between the chip and a serial port.
 *  The UART driver simplifies reading and writing to any of the UART
 *  peripherals on the board, with multiple modes of operation and performance.
 *  These include blocking, non-blocking, and polling, as well as text/binary
 *  mode, echo and return characters.
 *
 *  The APIs in this driver serve as an interface to a typical RTOS
 *  application. The specific peripheral implementations are responsible for
 *  creating all the RTOS specific primitives to allow for thread-safe
 *  operation.
 *
 *  # Usage #
 *
 *  The UART driver interface provides device independent APIs, data types,
 *  and macros.  The following code example opens a UART instance, reads
 *  a byte from the UART, and then writes the byte back to the UART.
 *
 *  @code
 *    char        input;
 *    UART_Handle uart;
 *    UART_Params uartParams;
 *
 *    // Initialize the UART driver.
 *    UART_init();
 *
 *    // Create a UART with data processing off.
 *    UART_Params_init(&uartParams);
 *    uartParams.writeDataMode = UART_DATA_BINARY;
 *    uartParams.readDataMode = UART_DATA_BINARY;
 *    uartParams.readReturnMode = UART_RETURN_FULL;
 *    uartParams.readEcho = UART_ECHO_OFF;
 *    uartParams.baudRate = 115200;
 *
 *    // Open an instance of the UART drivers
 *    uart = UART_open(Board_UART0, &uartParams);
 *
 *    if (uart == NULL) {
 *        // UART_open() failed
 *        while (1);
 *    }
 *
 *    // Loop forever echoing
 *    while (1) {
 *        UART_read(uart, &input, 1);
 *        UART_write(uart, &input, 1);
 *    }
 *  @endcode
 *
 *  Details for the example code above are described in the following
 *  subsections.
 *
 *
 *  ### UART Driver Configuration #
 *
 *  In order to use the UART APIs, the application is required
 *  to provide device-specific UART configuration in the Board.c file.
 *  The UART driver interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct UART_Config_ {
 *      UART_FxnTable const    *fxnTablePtr;
 *      void                   *object;
 *      void          const    *hwAttrs;
 *  } UART_Config;
 *  @endcode
 *
 *  The application must declare an array of UART_Config elements, named
 *  UART_config[].  Each element of UART_config[] are populated with
 *  pointers to a device specific UART driver implementation's function
 *  table, driver object, and hardware attributes.  The hardware attributes
 *  define properties such as the UART peripheral's base address, and
 *  the pins for RX and TX.  Each element in UART_config[] corresponds to
 *  a UART instance, and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the peripheral designation
 *  (such as UART0 or UART1).  For example, it is possible to use
 *  UART_config[0] for UART1.
 *
 *  You will need to check the device-specific UART driver implementation's
 *  header file for example configuration.  Please also refer to the
 *  Board.c file of any of your examples to see the UART configuration.
 *
 *  ### Initializing the UART Driver #
 *
 *  UART_init() must be called before any other UART APIs.  This function
 *  calls the device implementation's UART initialization function, for each
 *  element of UART_config[].
 *
 *  ### Opening the UART Driver #
 *
 *  Opening a UART requires four steps:
 *  1.  Create and initialize a UART_Params structure.
 *  2.  Fill in the desired parameters.
 *  3.  Call UART_open(), passing the index of the UART in the UART_config
 *      structure, and the address of the UART_Params structure.  The
 *      UART instance is specified by the index in the UART_config structure.
 *  4.  Check that the UART handle returned by UART_open() is non-NULL,
 *      and save it.  The handle will be used to read and write to the
 *      UART you just opened.
 *
 *  Only one UART index can be used at a time; calling UART_open() a second
 *  time with the same index previosly passed to UART_open() will result in
 *  an error.  You can, though, re-use the index if the instance is closed
 *  via UART_close().
 *  In the example code, Board_UART0 is passed to UART_open().  This macro
 *  is defined in the example's Board.h file.
 *
 *
 *  ### Modes of Operation #
 *
 *  The UART driver can operate in blocking mode or callback mode, by
 *  setting the writeMode and readMode parameters passed to UART_open().
 *  If these parameters are not set, as in the example code, the UART
 *  driver defaults to blocking mode.  Options for the writeMode and
 *  readMode parameters are #UART_MODE_BLOCKING and #UART_MODE_CALLBACK:
 *
 *  - #UART_MODE_BLOCKING uses a semaphore to block while data is being sent.
 *    The context of calling UART_read() or UART_write() must be a Task when
 *    using #UART_MODE_BLOCKING.  The UART_write() or UART_read() call
 *    will block until all data is sent or received, or the write timeout or
 *    read timeout expires, whichever happens first.
 *
 *  - #UART_MODE_CALLBACK is non-blocking and UART_read() and UART_write()
 *    will return while data is being sent in the context of a hardware
 *    interrupt.  When the read or write finishes, the UART driver will call
 *    the user's callback function.  In some cases, the UART data transfer
 *    may have been canceled, or a newline may have been received, so the
 *    number of bytes sent/received are passed to the callback function.  Your
 *    implementation of the callback function can use this information
 *    as needed.  Since the user's callback may be called in the context of an
 *    ISR, the callback function must not make any RTOS blocking calls.
 *    The buffer passed to UART_write() in #UART_MODE_CALLBACK is not copied.
 *    The buffer must remain coherent until all the characters have been sent
 *    (ie until the tx callback has been called with a byte count equal to
 *    that passed to UART_write()).
 *
 *  The example sets the writeDataMode and readDataMode parameters to
 *  #UART_DATA_BINARY.  Options for these parameters are #UART_DATA_BINARY
 *  and #UART_DATA_TEXT:
 *
 *  - #UART_DATA_BINARY:  The data is passed as is, without processing.
 *
 *  - #UART_DATA_TEXT: Write actions add a carriage return before a
 *    newline character, and read actions replace a return with a newline.
 *    This effectively treats all device line endings as LF and all host
 *    PC line endings as CRLF.
 *
 *  Other parameters set by the example are readReturnMode and readEcho.
 *  Options for the readReturnMode parameter are #UART_RETURN_FULL and
 *  #UART_RETURN_NEWLINE:
 *
 *  - #UART_RETURN_FULL:  The read action unblocks or returns when the buffer
 *    is full.
 *  - #UART_RETURN_NEWLINE:  The read action unblocks or returns when a
 *    newline character is read, before the buffer is full.
 *
 *  Options for the readEcho parameter are #UART_ECHO_OFF and #UART_ECHO_ON.
 *  This parameter determines whether the driver echoes data back to the
 *  UART.  When echo is turned on, each character that is read by the target
 *  is written back, independent of any write operations.  If data is
 *  received in the middle of a write and echo is turned on, the echoed
 *  characters will be mixed in with the write data.
 *
 *  ### Reading and Writing data #
 *
 *  The example code reads one byte frome the UART instance, and then writes
 *  one byte back to the same instance:
 *
 *  @code
 *  UART_read(uart, &input, 1);
 *  UART_write(uart, &input, 1);
 *  @endcode
 *
 *  The UART driver allows full duplex data transfers. Therefore, it is
 *  possible to call UART_read() and UART_write() at the same time (for
 *  either blocking or callback modes). It is not possible, however,
 *  to issue multiple concurrent operations in the same direction.
 *  For example, if one thread calls UART_read(uart0, buffer0...),
 *  any other thread attempting UART_read(uart0, buffer1...) will result in
 *  an error of UART_STATUS_ERROR, until all the data from the first UART_read()
 *  has been transferred to buffer0. This applies to both blocking and
 *  and callback modes. So applications must either synchronize
 *  UART_read() (or UART_write()) calls that use the same UART handle, or
 *  check for the UART_STATUS_ERROR return code indicating that a transfer is
 *  still ongoing.
 *
 *  # Implementation #
 *
 *  The UART driver interface module is joined (at link time) to an
 *  array of UART_Config data structures named *UART_config*.
 *  UART_config is implemented in the application with each entry being an
 *  instance of a UART peripheral. Each entry in *UART_config* contains a:
 *  - (UART_FxnTable *) to a set of functions that implement a UART peripheral
 *  - (void *) data object that is associated with the UART_FxnTable
 *  - (void *) hardware attributes that are associated with the UART_FxnTable
 *
 *  The UART APIs are redirected to the device specific implementations
 *  using the UART_FxnTable pointer of the UART_config entry.
 *  In order to use device specific functions of the UART driver directly,
 *  link in the correct driver library for your device and include the
 *  device specific UART driver header file (which in turn includes UART.h).
 *  For example, for the MSP432 family of devices, you would include the
 *  following header file:
 *    @code
 *    #include <ti/drivers/uart/UARTMSP432.h>
 *    @endcode
 *
 *  ============================================================================
 */

#ifndef ti_drivers_UART__include
#define ti_drivers_UART__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 *  @defgroup UART_CONTROL UART_control command and status codes
 *  These UART macros are reservations for UART.h
 *  @{
 */

/*!
 * Common UART_control command code reservation offset.
 * UART driver implementations should offset command codes with
 * UART_CMD_RESERVED growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define UARTXYZ_CMD_COMMAND0     UART_CMD_RESERVED + 0
 * #define UARTXYZ_CMD_COMMAND1     UART_CMD_RESERVED + 1
 * @endcode
 */
#define UART_CMD_RESERVED           (32)

/*!
 * Common UART_control status code reservation offset.
 * UART driver implementations should offset status codes with
 * UART_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define UARTXYZ_STATUS_ERROR0    UART_STATUS_RESERVED - 0
 * #define UARTXYZ_STATUS_ERROR1    UART_STATUS_RESERVED - 1
 * #define UARTXYZ_STATUS_ERROR2    UART_STATUS_RESERVED - 2
 * @endcode
 */
#define UART_STATUS_RESERVED        (-32)

/**
 *  @defgroup UART_STATUS Status Codes
 *  UART_STATUS_* macros are general status codes returned by UART_control()
 *  @{
 *  @ingroup UART_CONTROL
 */

/*!
 * @brief   Successful status code returned by UART_control().
 *
 * UART_control() returns UART_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define UART_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code returned by UART_control().
 *
 * UART_control() returns UART_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define UART_STATUS_ERROR           (-1)

/*!
 * @brief   An error status code returned by UART_control() for undefined
 * command codes.
 *
 * UART_control() returns UART_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define UART_STATUS_UNDEFINEDCMD    (-2)
/** @}*/

/**
 *  @defgroup UART_CMD Command Codes
 *  UART_CMD_* macros are general command codes for UART_control(). Not all UART
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup UART_CONTROL
 */

/*!
 * @brief   Command code used by UART_control() to read the next unsigned char.
 *
 * This command is used to read the next unsigned char from the UART's circular
 * buffer without removing it. With this command code, @b arg is a pointer to an
 * integer. @b *arg contains the next @c unsigned @c char read if data is
 * present, else @b *arg is set to #UART_STATUS_ERROR.
 */
#define UART_CMD_PEEK               (0)

/*!
 * @brief   Command code used by UART_control() to determine if the read buffer
 *          is empty.
 *
 * This command is used to determine if there are any unsigned chars available
 * to read from the UART's circular buffer using UART_read(). With this command
 * code, @b arg is a pointer to a @c bool. @b *arg contains @c true if data is
 * available, else @c false.
 */
#define UART_CMD_ISAVAILABLE        (1)

/*!
 * @brief   Command code used by UART_control() to determine how many unsigned
 *          chars are in the read buffer.
 *
 * This command is used to determine how many @c unsigned @c chars are available
 * to read from the UART's circular buffer using UART_read(). With this command
 * code, @b arg is a pointer to an @a integer. @b *arg contains the number of
 * @c unsigned @c chars available to read.
 */
#define UART_CMD_GETRXCOUNT         (2)

/*!
 * @brief   Command code used by UART_control() to enable data receive by the
 *          UART.
 *
 * This command is used to enable the UART in such a way that it stores received
 * unsigned chars into the circular buffer. For drivers that support power
 * management, this typically means that the UART will set a power constraint
 * while receive is enabled. UART_open() will always have this option
 * enabled. With this command code, @b arg is @a don't @a care.
 */
#define UART_CMD_RXENABLE           (3)

/*!
 * @brief   Command code used by UART_control() to disable data received by the
 *          UART.
 *
 * This command is used to disable the UART in such a way that ignores the data
 * it receives. For drivers that support power management, this typically means
 * that the driver will release any power constraints, to permit the system to
 * enter low power modes. With this command code, @b arg is @a don't @a care.
 *
 * @warning A call to UART_read() does @b NOT re-enable receive.
 */
#define UART_CMD_RXDISABLE          (4)
/** @}*/

/** @}*/

#define UART_ERROR                  (UART_STATUS_ERROR)

/*!
 *  @brief    Wait forever define
 */
#define UART_WAIT_FOREVER           (~(0U))

/*!
 *  @brief      A handle that is returned from a UART_open() call.
 */
typedef struct UART_Config_    *UART_Handle;

/*!
 *  @brief      The definition of a callback function used by the UART driver
 *              when used in #UART_MODE_CALLBACK
 *              The callback can occur in task or HWI context.
 *
 *  @param      UART_Handle             UART_Handle
 *
 *  @param      buf                     Pointer to read/write buffer
 *
 *  @param      count                   Number of elements read/written
 */
typedef void (*UART_Callback) (UART_Handle handle, void *buf, size_t count);

/*!
 *  @brief      UART mode settings
 *
 *  This enum defines the read and write modes for the configured UART.
 */
typedef enum UART_Mode_ {
    /*!
      *  Uses a semaphore to block while data is being sent.  Context of the call
      *  must be a Task.
      */
    UART_MODE_BLOCKING,

    /*!
      *  Non-blocking and will return immediately. When UART_write() or
      *  UART_read() has finished, the callback function is called from either
      *  the caller's context or from an interrupt context.
      */
    UART_MODE_CALLBACK
} UART_Mode;

/*!
 *  @brief      UART return mode settings
 *
 *  This enumeration defines the return modes for UART_read() and
 *  UART_readPolling(). This mode only functions when in #UART_DATA_TEXT mode.
 *
 *  #UART_RETURN_FULL unblocks or performs a callback when the read buffer has
 *  been filled.
 *  #UART_RETURN_NEWLINE unblocks or performs a callback whenever a newline
 *  character has been received.
 *
 *  UART operation | UART_RETURN_FULL | UART_RETURN_NEWLINE |
 *  -------------- | ---------------- | ------------------- |
 *  UART_read()    | Returns when buffer is full | Returns when buffer is full or newline was read |
 *  UART_write()   | Sends data as is | Sends data with an additional newline at the end |
 *
 *  @pre        UART driver must be used in #UART_DATA_TEXT mode.
 */
typedef enum UART_ReturnMode_ {
    /*! Unblock/callback when buffer is full. */
    UART_RETURN_FULL,

    /*! Unblock/callback when newline character is received. */
    UART_RETURN_NEWLINE
} UART_ReturnMode;

/*!
 *  @brief      UART data mode settings
 *
 *  This enumeration defines the data mode for reads and writes.
 *
 *  In #UART_DATA_BINARY, data is passed as is, with no processing.
 *
 *  In #UART_DATA_TEXT mode, the driver will examine the #UART_ReturnMode
 *  value, to determine whether or not to unblock/callback when a newline
 *  is received.  Read actions replace a carriage return with a newline,
 *  and write actions add a carriage return before a newline.  This
 *  effectively treats all device line endings as LF, and all host PC line
 *  endings as CRLF.
 */
typedef enum UART_DataMode_ {
    UART_DATA_BINARY = 0, /*!< Data is not processed */
    UART_DATA_TEXT = 1    /*!< Data is processed according to above */
} UART_DataMode;

/*!
 *  @brief      UART echo settings
 *
 *  This enumeration defines if the driver will echo data when uses in
 *  #UART_DATA_TEXT mode. This only applies to data received by the UART.
 *
 *  #UART_ECHO_ON will echo back characters it received while in #UART_DATA_TEXT
 *  mode.
 *  #UART_ECHO_OFF will not echo back characters it received in #UART_DATA_TEXT
 *  mode.
 *
 *  @pre        UART driver must be used in #UART_DATA_TEXT mode.
 */
typedef enum UART_Echo_ {
    UART_ECHO_OFF = 0,  /*!< Data is not echoed */
    UART_ECHO_ON = 1    /*!< Data is echoed */
} UART_Echo;

/*!
 *  @brief    UART data length settings
 *
 *  This enumeration defines the UART data lengths.
 */
typedef enum UART_LEN_ {
    UART_LEN_5 = 0,  /*!< Data length is 5 bits */
    UART_LEN_6 = 1,  /*!< Data length is 6 bits */
    UART_LEN_7 = 2,  /*!< Data length is 7 bits */
    UART_LEN_8 = 3   /*!< Data length is 8 bits */
} UART_LEN;

/*!
 *  @brief    UART stop bit settings
 *
 *  This enumeration defines the UART stop bits.
 */
typedef enum UART_STOP_ {
    UART_STOP_ONE = 0,  /*!< One stop bit */
    UART_STOP_TWO = 1   /*!< Two stop bits */
} UART_STOP;

/*!
 *  @brief    UART parity type settings
 *
 *  This enumeration defines the UART parity types.
 */
typedef enum UART_PAR_ {
    UART_PAR_NONE = 0,  /*!< No parity */
    UART_PAR_EVEN = 1,  /*!< Parity bit is even */
    UART_PAR_ODD  = 2,  /*!< Parity bit is odd */
    UART_PAR_ZERO = 3,  /*!< Parity bit is always zero */
    UART_PAR_ONE  = 4   /*!< Parity bit is always one */
} UART_PAR;

/*!
 *  @brief    UART Parameters
 *
 *  UART parameters are used with the UART_open() call. Default values for
 *  these parameters are set using UART_Params_init().
 *
 *  @sa       UART_Params_init()
 */
typedef struct UART_Params_ {
    UART_Mode       readMode;        /*!< Mode for all read calls */
    UART_Mode       writeMode;       /*!< Mode for all write calls */
    uint32_t        readTimeout;     /*!< Timeout for read calls in blocking mode. */
    uint32_t        writeTimeout;    /*!< Timeout for write calls in blocking mode. */
    UART_Callback   readCallback;    /*!< Pointer to read callback function for callback mode. */
    UART_Callback   writeCallback;   /*!< Pointer to write callback function for callback mode. */
    UART_ReturnMode readReturnMode;  /*!< Receive return mode */
    UART_DataMode   readDataMode;    /*!< Type of data being read */
    UART_DataMode   writeDataMode;   /*!< Type of data being written */
    UART_Echo       readEcho;        /*!< Echo received data back */
    uint32_t        baudRate;        /*!< Baud rate for UART */
    UART_LEN        dataLength;      /*!< Data length for UART */
    UART_STOP       stopBits;        /*!< Stop bits for UART */
    UART_PAR        parityType;      /*!< Parity bit type for UART */
    void           *custom;          /*!< Custom argument used by driver implementation */
} UART_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_CloseFxn().
 */
typedef void (*UART_CloseFxn) (UART_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_ControlFxn().
 */
typedef int_fast16_t (*UART_ControlFxn) (UART_Handle handle, uint_fast16_t cmd, void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_InitFxn().
 */
typedef void (*UART_InitFxn) (UART_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_OpenFxn().
 */
typedef UART_Handle (*UART_OpenFxn) (UART_Handle handle, UART_Params *params);
/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_ReadFxn().
 */
typedef int_fast32_t (*UART_ReadFxn) (UART_Handle handle, void *buffer,
    size_t size);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_ReadPollingFxn().
 */
typedef int_fast32_t (*UART_ReadPollingFxn) (UART_Handle handle, void *buffer,
    size_t size);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_ReadCancelFxn().
 */
typedef void (*UART_ReadCancelFxn) (UART_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_WriteFxn().
 */
typedef int_fast32_t (*UART_WriteFxn) (UART_Handle handle, const void *buffer,
    size_t size);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_WritePollingFxn().
 */
typedef int_fast32_t (*UART_WritePollingFxn) (UART_Handle handle,
    const void *buffer, size_t size);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              UART_WriteCancelFxn().
 */
typedef void (*UART_WriteCancelFxn) (UART_Handle handle);

/*!
 *  @brief      The definition of a UART function table that contains the
 *              required set of functions to control a specific UART driver
 *              implementation.
 */
typedef struct UART_FxnTable_ {
    /*! Function to close the specified peripheral */
    UART_CloseFxn        closeFxn;

    /*! Function to implementation specific control function */
    UART_ControlFxn      controlFxn;

    /*! Function to initialize the given data object */
    UART_InitFxn         initFxn;

    /*! Function to open the specified peripheral */
    UART_OpenFxn         openFxn;

    /*! Function to read from the specified peripheral */
    UART_ReadFxn         readFxn;

    /*! Function to read via polling from the specified peripheral */
    UART_ReadPollingFxn  readPollingFxn;

    /*! Function to cancel a read from the specified peripheral */
    UART_ReadCancelFxn   readCancelFxn;

    /*! Function to write from the specified peripheral */
    UART_WriteFxn        writeFxn;

    /*! Function to write via polling from the specified peripheral */
    UART_WritePollingFxn writePollingFxn;

    /*! Function to cancel a write from the specified peripheral */
    UART_WriteCancelFxn  writeCancelFxn;
} UART_FxnTable;

/*!
 *  @brief  UART Global configuration
 *
 *  The UART_Config structure contains a set of pointers used to characterize
 *  the UART driver implementation.
 *
 *  This structure needs to be defined before calling UART_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     UART_init()
 */
typedef struct UART_Config_ {
    /*! Pointer to a table of driver-specific implementations of UART APIs */
    UART_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void                *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void          const *hwAttrs;
} UART_Config;

/*!
 *  @brief  Function to close a UART peripheral specified by the UART handle
 *
 *  @pre    UART_open() has been called.
 *  @pre    Ongoing asynchronous read or write have been canceled using
 *          UART_readCancel() or UART_writeCancel() respectively.
 *
 *  @param  handle      A #UART_Handle returned from UART_open()
 *
 *  @sa     UART_open()
 */
extern void UART_close(UART_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          #UART_Handle.
 *
 *  Commands for %UART_control() can originate from UART.h or from implementation
 *  specific UART*.h (_UARTCC26XX.h_, _UARTMSP432.h_, etc.. ) files.
 *  While commands from UART.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific UART*.h files add
 *  unique driver capabilities but are not API portable across all UART driver
 *  implementations.
 *
 *  Commands supported by UART.h follow a UART_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by UART*.h follow a UART*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref UART_CMD "UART_control command codes" for command codes.
 *
 *  See @ref UART_STATUS "UART_control return status codes" for status codes.
 *
 *  @pre    UART_open() has to be called.
 *
 *  @param  handle      A UART handle returned from UART_open()
 *
 *  @param  cmd         UART.h or UART*.h commands.
 *
 *  @param  arg         An optional R/W (read/write) command argument
 *                      accompanied with cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     UART_open()
 */
extern int_fast16_t UART_control(UART_Handle handle, uint_fast16_t cmd, void *arg);

/*!
 *  @brief  Function to initialize the UART module
 *
 *  @pre    The UART_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other UART driver APIs.
 */
extern void UART_init(void);

/*!
 *  @brief  Function to initialize a given UART peripheral
 *
 *  Function to initialize a given UART peripheral specified by the
 *  particular index value.
 *
 *  @pre    UART_init() has been called
 *
 *  @param  index         Logical peripheral number for the UART indexed into
 *                        the UART_config table
 *
 *  @param  params        Pointer to a parameter block. If NULL, default
 *                        parameter values will be used. All the fields in
 *                        this structure are RO (read-only).
 *
 *  @return A #UART_Handle upon success. NULL if an error occurs, or if the
 *          indexed UART peripheral is already opened.
 *
 *  @sa     UART_init()
 *  @sa     UART_close()
 */
extern UART_Handle UART_open(uint_least8_t index, UART_Params *params);

/*!
 *  @brief  Function to initialize the UART_Params struct to its defaults
 *
 *  @param  params      An pointer to UART_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      readMode = UART_MODE_BLOCKING;
 *      writeMode = UART_MODE_BLOCKING;
 *      readTimeout = UART_WAIT_FOREVER;
 *      writeTimeout = UART_WAIT_FOREVER;
 *      readCallback = NULL;
 *      writeCallback = NULL;
 *      readReturnMode = UART_RETURN_NEWLINE;
 *      readDataMode = UART_DATA_TEXT;
 *      writeDataMode = UART_DATA_TEXT;
 *      readEcho = UART_ECHO_ON;
 *      baudRate = 115200;
 *      dataLength = UART_LEN_8;
 *      stopBits = UART_STOP_ONE;
 *      parityType = UART_PAR_NONE;
 */
extern void UART_Params_init(UART_Params *params);

/*!
 *  @brief  Function that writes data to a UART with interrupts enabled.
 *
 *  %UART_write() writes data from a memory buffer to the UART interface.
 *  The source is specified by \a buffer and the number of bytes to write
 *  is given by \a size.
 *
 *  In #UART_MODE_BLOCKING, UART_write() blocks task execution until all
 *  the data in buffer has been written.
 *
 *  In #UART_MODE_CALLBACK, %UART_write() does not block task execution.
 *  Instead, a callback function specified by UART_Params::writeCallback is
 *  called when the transfer is finished.  The buffer passed to UART_write()
 *  in #UART_MODE_CALLBACK is not copied. The buffer must remain coherent
 *  until all the characters have been sent (ie until the tx callback has
 *  been called with a byte count equal to that passed to UART_write()).
 *  The callback function can occur in the caller's task context or in a HWI or
 *  SWI context, depending on the device implementation.
 *  An unfinished asynchronous write operation must always be canceled using
 *  UART_writeCancel() before calling UART_close().
 *
 *  %UART_write() is mutually exclusive to UART_writePolling(). For an opened
 *  UART peripheral, either UART_write() or UART_writePolling() can be used,
 *  but not both.
 *
 *  @warning Do not call %UART_write() from its own callback function when in
 *  #UART_MODE_CALLBACK.
 *
 *  @sa UART_writePolling()
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 *
 *  @param  buffer      A read-only pointer to buffer containing data to
 *                      be written to the UART
 *
 *  @param  size        The number of bytes in the buffer that should be written
 *                      to the UART
 *
 *  @return Returns the number of bytes that have been written to the UART.
 *          If an error occurs, #UART_STATUS_ERROR is returned.
 *          In #UART_MODE_CALLBACK mode, the return value is always 0.
 */
extern int_fast32_t UART_write(UART_Handle handle, const void *buffer, size_t size);

/*!
 *  @brief  Function that writes data to a UART, polling the peripheral to
 *          wait until new data can be written. Usage of this API is mutually
 *          exclusive with usage of UART_write().
 *
 *  This function initiates an operation to write data to a UART controller.
 *
 *  UART_writePolling() will not return until all the data was written to the
 *  UART (or to its FIFO if applicable).
 *
 *  @sa UART_write()
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 *
 *  @param  buffer      A read-only pointer to the buffer containing the data to
 *                      be written to the UART
 *
 *  @param  size        The number of bytes in the buffer that should be written
 *                      to the UART
 *
 *  @return Returns the number of bytes that have been written to the UART.
 *          If an error occurs, #UART_STATUS_ERROR is returned.
 */
extern int_fast32_t UART_writePolling(UART_Handle handle, const void *buffer, size_t size);

/*!
 *  @brief  Function that cancels a UART_write() function call.
 *
 *  This function cancels an asynchronous UART_write() operation and is only
 *  applicable in #UART_MODE_CALLBACK.
 *  UART_writeCancel() calls the registered TX callback function no matter how many bytes
 *  were sent. It is the application's responsibility to check the count argument in
 *  the callback function and handle cases where only a subset of the bytes were sent.
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 */
extern void UART_writeCancel(UART_Handle handle);

/*!
 *  @brief  Function that reads data from a UART with interrupt enabled.
 *
 *  %UART_read() reads data from a UART controller. The destination is specified
 *  by \a buffer and the number of bytes to read is given by \a size.
 *
 *  In #UART_MODE_BLOCKING, %UART_read() blocks task execution until all
 *  the data in buffer has been read.
 *
 *  In #UART_MODE_CALLBACK, %UART_read() does not block task execution.
 *  Instead, a callback function specified by UART_Params::readCallback
 *  is called when the transfer is finished.
 *  The callback function can occur in the caller's context or in HWI or SWI
 *  context, depending on the device-specific implementation.
 *  An unfinished asynchronous read operation must always be canceled using
 *  UART_readCancel() before calling UART_close().
 *
 *  %UART_read() is mutually exclusive to UART_readPolling(). For an opened
 *  UART peripheral, either %UART_read() or UART_readPolling() can be used,
 *  but not both.
 *
 *  @warning Do not call %UART_read() from its own callback function when in
 *  #UART_MODE_CALLBACK.
 *
 *  @sa UART_readPolling()
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 *
 *  @param  buffer      A pointer to an empty buffer to which
 *                      received data should be written
 *
 *  @param  size        The number of bytes to be written into buffer
 *
 *  @return Returns the number of bytes that have been read from the UART,
 *          #UART_STATUS_ERROR on an error.
 */
extern int_fast32_t UART_read(UART_Handle handle, void *buffer, size_t size);

/*!
 *  @brief  Function that reads data from a UART without interrupts. This API
 *          must be used mutually exclusive with UART_read().
 *
 *  This function initiates an operation to read data from a UART peripheral.
 *
 *  %UART_readPolling() will not return until size data was read to the UART.
 *
 *  @sa UART_read()
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 *
 *  @param  buffer      A pointer to an empty buffer in which
 *                      received data should be written to
 *
 *  @param  size        The number of bytes to be written into buffer
 *
 *  @return Returns the number of bytes that have been read from the UART,
 *          #UART_STATUS_ERROR on an error.
 */
extern int_fast32_t UART_readPolling(UART_Handle handle, void *buffer, size_t size);

/*!
 *  @brief  Function that cancels a UART_read() function call.
 *
 *  This function cancels an asynchronous UART_read() operation and is only
 *  applicable in #UART_MODE_CALLBACK.
 *  UART_readCancel() calls the registered RX callback function no matter how many bytes
 *  were received. It is the application's responsibility to check the count argument in
 *  the callback function and handle cases where only a subset of the bytes were received.
 *
 *  @param  handle      A #UART_Handle returned by UART_open()
 */
extern void UART_readCancel(UART_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_UART__include */
