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
 *  @file       I2S.h
 *
 *  @brief      I2S driver interface
 *
 *  The I2S header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/I2S.h>
 *  @endcode
 *
 *  # Overview #
 *  The I2S driver facilitates the use of Inter-IC Sound (I2S), which is
 *  used to connect digital audio devices so that audio signals can be
 *  communicated between devices. The I2S driver simplifies reading and
 *  writing to any of the Multichannel Audio Serial Port (McASP) peripherals
 *  on the board with Receive and Transmit support. These include blocking,
 *  non-blocking, read and write characters on the McASP peripheral.
 *
 *  The APIs in this driver serve as an interface to a typical RTOS
 *  application.  Its purpose is to redirect the I2S APIs to specific
 *  driver implementations which are specified using a pointer to an
 *  #I2S_FxnTable.
 *  The specific peripheral implementations are responsible
 *  for creating all the RTOS specific primitives to allow for thread-safe
 *  operation.
 *
 *  # Usage #
 *
 *  To use the I2S driver for reaading and writing data to the I2S peripheral,
 *  the application calls the following APIs:
 *    - I2S_init(): Initialize the I2S driver.
 *    - I2S_Params_init():  Initialize a #I2S_Params structure with default
 *      vaules.  Then change the parameters from non-default values as
 *      needed.
 *    - I2S_open():  Open an instance of the I2S driver, passing the
 *      initialized parameters, or NULL, and an index (described later).
 *    - If using callback mode, I2S_read() and I2S_write().
 *    - If using issue/reclaim mode, I2S_readIssue(), I2S_readReclaim(),
 *      I2S_writeIssue() and I2S_writeReclaim().
 *    - I2S_close():  De-initialize the I2S instance.
 *
 *  ### I2S Driver Configuration #
 *
 *  In order to use the I2S APIs, the application is required
 *  to provide device-specific I2S configuration in the Board.c file.
 *  The I2S driver interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct I2S_Config_ {
 *      // Pointer to driver-specific implementation of I2S functions
 *      I2S_FxnTable const *fxnTablePtr;
 *      void               *object;   // Driver specific data object
 *      void         const *hwAttrs;  // Driver specific hardware attributes
 *  } I2S_Config;
 *  @endcode
 *
 *  The application must declare an array of I2S_Config elements, named
 *  I2S_config[].  Each element of I2S_config[] must be populated with
 *  pointers to a device specific I2S driver implementation's function
 *  table, driver object, and hardware attributes.  The hardware attributes
 *  define properties such as the I2S peripheral's base address and pins.
 *  Each element in I2S_config[] corresponds to an I2S instance, and
 *  and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the peripheral
 *  designation (such as I2S0 or I2S1).  For example, it is possible
 *  to use I2S_config[0] for I2S1.
 *
 *  Because I2S configuration is very device dependent, you will need to
 *  check the doxygen for the device specific I2S implementation.  There you
 *  will find a description of the I2S hardware attributes.  Please also
 *  refer to the board.c file of any of your examples to see the I2S
 *  configuration.
 *
 *  ### Initializing the I2S Driver #
 *
 *  I2S_init() must be called before any other I2S APIs.  This function
 *  iterates through the elements of the I2S_config[] array, calling
 *  the element's device implementation I2S initialization function.
 *
 *  ### I2S Parameters
 *
 *  The #I2S_Params structure is passed to the I2S_open() call.  If NULL
 *  is passed for the parameters, I2S_open() uses default parameters.
 *  An #I2S_Params structure is initialized with default values by passing
 *  it to I2S_Params_init().
 *  Some of the I2S parameters are described below.  To see brief descriptions
 *  of all the parameters, see #I2S_Params.
 *
 *  #### I2S Operation Mode
 *  The I2S operation mode determines whether transmit and/or receive modes
 *  are enabled. The mode is specified with one of the following constants:
 *  - #I2S_OPMODE_TX_ONLY: Enable transmit only.
 *  - #I2S_OPMODE_RX_ONLY: Enable receive only.
 *  - #I2S_OPMODE_TX_RX_SYNC: Enable both receive and transmit.
 *
 *  #### I2S Data Mode
 *  A separate data mode may be specified for read calls and write calls.
 *  The available modes are:
 *  - #I2S_MODE_CALLBACK: This mode is non-blocking. Calls to I2S_read() or
 *    I2S_write() return immediately. When the transfer is finished, the
 *    user configured callback function is called.
 *  - #I2S_MODE_ISSUERECLAIM: Call I2S_readIssue() and I2S_writeIssue() to
 *    queue buffers to the I2S.  I2S_readReclaim() blocks until a buffer
 *    of data is available. I2S_writeReclaim() blocks until a buffer of
 *    data has been issued and the descriptor can be returned back to the
 *    caller.
 *
 *  ### Opening the I2S Driver #
 *  After initializing the I2S driver by calling I2S_init(), the application
 *  can open an I2S instance by calling I2S_open().  This function
 *  takes an index into the I2S_config[] array, and an I2S parameters data
 *  structure.   The I2S instance is specified by the index of the I2S in
 *  I2S_config[].  Only one I2S index can be used at a time;
 *  calling I2S_open() a second time with the same index previosly
 *  passed to I2S_open() will result in an error.  You can,
 *  though, re-use the index if the instance is closed via I2S_close().
 *
 *  If NULL is passed for the I2S_Params structure to I2S_open(), default values
 *  are used. If the open call is successful, it returns a non-NULL value.
 *
 *  Example opening an I2S driver instance:
 *  @code
 *  I2S_Handle      handle;
 *  I2S_Params      params;
 *
 *  I2S_Params_init(&params);
 *  params.operationMode = I2S_MODE_TX_RX_SYNC;
 *  < Change other params as required >
 *
 *  handle = I2S_open(Board_I2S0, &params);
 *  if (!handle) {
 *      // Error opening I2S, handle accordingly
 *  }
 *  @endcode
 *
 *  ### Writing Data #
 *  The following example calls I2S_writeIssue() to write to an I2S driver
 *  instance that has been opened. It first queues up two buffers of text.
 *  Within an infinite loop, it calls I2S_writeReclaim() to retrieve a
 *  buffer and then re-queues the buffer.
 *
 *  @code
 *  const unsigned char hello[] = "Hello World\n";
 *  const unsigned char hello1[] = "Hello World1\n";
 *  I2S_BufDesc writeBuffer1;
 *  I2S_BufDesc writeBuffer2;
 *  I2S_BufDesc *pDesc = NULL;
 *
 *  writeBuffer1.bufPtr  = &hello;
 *  writeBuffer1.bufSize = sizeof(hello);
 *  writeBuffer2.bufPtr  = &hello1;
 *  writeBuffer2.bufSize = sizeof(hello1);
 *
 *  ret = I2S_writeIssue(handle, &writeBuffer1);
 *  ret = I2S_writeIssue(handle, &writeBuffer2);
 *
 *  while(1) {
 *      ret = I2S_writeReclaim(handle, &pDesc);
 *      pDesc->bufPtr  = &hello;;
 *      pDesc->bufSize = sizeof(hello);
 *      ret = I2S_writeIssue(handle, pDesc);
 *  }
 *
 *  @endcode
 *
 *  ### Reading Data #
 *  The following example calls I2S_readIssue() to queue a buffer for
 *  reading from an I2S driver instance. It first queues up two buffers of
 *  text. Within an infinite loop, it then calls I2S_readReclaim() to retrieve
 *  a full buffer of data.
 *
 *  @code
 *  unsigned char rxBuffer[20];
 *  unsigned char rxBuffer1[20];
 *  I2S_BufDesc readBuffer1;
 *  I2S_BufDesc readBuffer2;
 *  I2S_BufDesc *pDesc = NULL;
 *
 *  readBuffer1.bufPtr = &rxBuffer;
 *  readBuffer1.bufSize = 20;
 *  readBuffer2.bufPtr = &rxBuffer1;
 *  readBuffer2.bufSize = 20;
 *
 *  ret = I2S_readIssue(handle, &readBuffer1);
 *  ret = I2S_readIssue(handle, &readBuffer2);
 *
 *  while(1)
 *  {
 *      ret = I2S_readReclaim(handle, &pDesc);
 *      pDesc->bufPtr = &rxBuffer;
 *      pDesc->bufSize = 20;
 *      ret = I2S_readIssue(handle, pDesc);
 *  }
 *  @endcode
 *
 *  # Implementation #
 *
 *  The I2S driver interface module is joined (at link time) to an
 *  array of I2S_Config data structures named *I2S_config*.
 *  *I2S_config* is implemented in the application with each entry being an
 *  instance of a I2S peripheral. Each entry in *I2S_config* contains a:
 *  - (I2S_FxnTable *) to a set of functions that implement a I2S peripheral
 *  - (void *) data object that is associated with the I2S_FxnTable
 *  - (void *) hardware attributes that are associated to the I2S_FxnTable
 *
 *******************************************************************************
 */

#ifndef ti_drivers_I2S__include
#define ti_drivers_I2S__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <ti/drivers/utils/List.h>

/**
 *  @defgroup I2S_CONTROL I2S_control command and status codes
 *  These I2S macros are reservations for I2S.h
 *  @{
 */

/*!
 * Common I2S_control command code reservation offset.
 * I2S driver implementations should offset command codes with I2S_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define I2SXYZ_CMD_COMMAND0     I2S_CMD_RESERVED + 0
 * #define I2SXYZ_CMD_COMMAND1     I2S_CMD_RESERVED + 1
 * @endcode
 */
#define I2S_CMD_RESERVED            (32)

/*!
 * Common I2S_control status code reservation offset.
 * I2S driver implementations should offset status codes with
 * I2S_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define I2SXYZ_STATUS_ERROR0    I2S_STATUS_RESERVED - 0
 * #define I2SXYZ_STATUS_ERROR1    I2S_STATUS_RESERVED - 1
 * #define I2SXYZ_STATUS_ERROR2    I2S_STATUS_RESERVED - 2
 * @endcode
 */
#define I2S_STATUS_RESERVED        (-32)

/**
 *  @defgroup I2S_STATUS Status Codes
 *  I2S_STATUS_* macros are general status codes returned by I2S_control()
 *  @{
 *  @ingroup I2S_CONTROL
 */

/*!
 * @brief   Successful status code returned by I2S_control().
 *
 * I2S_control() returns I2S_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define I2S_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code returned by I2S_control().
 *
 * I2S_control() returns I2S_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define I2S_STATUS_ERROR          (-1)

/*!
 * @brief   An error status code returned by I2S_control() for undefined
 * command codes.
 *
 * I2S_control() returns I2S_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define I2S_STATUS_UNDEFINEDCMD   (-2)
/** @}*/

/**
 *  @defgroup I2S_CMD Command Codes
 *  I2S_CMD_* macros are general command codes for I2S_control(). Not all I2S
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup I2S_CONTROL
 */

/* Add I2S_CMD_<commands> here */

/** @}*/

/** @}*/

#define I2S_ERROR  (I2S_STATUS_ERROR)

/*!
 *  @brief    Wait forever define
 */
#define I2S_WAIT_FOREVER (~(0U))

/*!
 *  @brief      A handle that is returned from a I2S_open() call.
 */
typedef struct I2S_Config_ *I2S_Handle;

/*!
 *  @brief I2S buffer descriptor for issue/reclaim mode.
 */
typedef struct I2S_BufDesc_ {

    /*! Used internally to link descriptors together */
    List_Elem              qElem;

    /*! Pointer to the buffer */
    void                   *bufPtr;

    /*! Size of the buffer (target MAUs). */
    size_t                 bufSize;

    /*! Optional argument associated with the descriptor. */
    uintptr_t              descArg;
} I2S_BufDesc;

/*!
 *  @brief      The definition of a callback function used by the I2S driver
 *              when used in ::I2S_MODE_CALLBACK
 *
 *  @param      I2S_Handle             I2S_Handle
 *
 *  @param      buf                     Pointer to read/write buffer
 *
 *  @param      count                   Number of elements read/written
 */
typedef void (*I2S_Callback)(I2S_Handle handle, I2S_BufDesc *desc);

/*!
 *  @brief      I2S mode settings
 *
 *  This enum defines the read and write modes for the
 *  configured I2S.
 */
typedef enum I2S_DataMode_ {
    /*!
     *  Non-blocking and will return immediately.  When the transfer by the intr
     *  is finished the configured callback function is called.
     */
    I2S_MODE_CALLBACK,

    /*!
     *  Use I2S_readIssue, I2S_writeIssue calls to queue buffers to the
     *  I2S.  I2S_readReclaim() blocks until a buffer of data is available.
     *  I2S_writeReclaim() blocks until a buffer of data has been written
     *  and the descriptor can be returned back to the caller.
     */
    I2S_MODE_ISSUERECLAIM
} I2S_DataMode;

/*!
 *  @brief      I2S mode settings
 *
 *  This enumeration defines the mode for I2S operation.
 */
typedef enum I2S_OpMode_ {
    I2S_OPMODE_TX_ONLY,       /*!< Only Transmit enabled */
    I2S_OPMODE_RX_ONLY,       /*!< Only Receive enabled */
    I2S_OPMODE_TX_RX_SYNC     /*!< Receive and Transmit are enabled in Sync */
} I2S_OpMode;

/*!
 *  @brief    I2S Serializer InActive state settings
 *
 *  This enumeration defines the Serializer configuration
 *  in inactive state.
 */
typedef enum I2S_SerInActiveConfig_ {
    I2S_SERCONFIG_INACT_TRI_STATE,  /*!< Inactive state to tristate */
    I2S_SERCONFIG_INACT_LOW_LEVEL,  /*!< Inactive state to low */
    I2S_SERCONFIG_INACT_HIGH_LEVEL  /*!< Inactive state to high */
} I2S_SerInActiveConfig;

/*!
 *  @brief    I2S serial pin mode
 *
 *  This enumeration defines the Serial pin configuration
 */
typedef enum I2S_PinMode_ {
    I2S_PINMODE_RX,                 /*!< Operate the pin in Rx mode */
    I2S_PINMODE_TX,                 /*!< Operate the pin in Tx mode */
    I2S_PINMODE_INACTIVE            /*!< Pin in inactive mode       */
} I2S_PinMode;

/*!
 *  @brief    Basic I2S Parameters
 *
 *  I2S parameters are used to with the I2S_open() call. Default values for
 *  these parameters are set using I2S_Params_init().
 *
 *  @sa       I2S_Params_init()
 */
typedef struct I2S_Params_ {
    /*!< I2S operational mode */
    I2S_OpMode            operationMode;

    /*!< I2S sampling frequency configuration in samples/second */
    uint32_t              samplingFrequency;

    /*!< Slot length */
    uint8_t               slotLength;

    /*!< Bits per sample (Word length) */
    uint8_t               bitsPerSample;

    /*!< Number of channels (slots per frame) */
    uint8_t               numChannels;

    /*!< Mode for all read calls   */
    I2S_DataMode          readMode;

    /*!< Pointer to read callback */
    I2S_Callback          readCallback;

    /*!< Timeout for read semaphore */
    uint32_t              readTimeout;

    /*!< Mode for all write calls   */
    I2S_DataMode          writeMode;

    /*!< Pointer to write callback */
    I2S_Callback          writeCallback;

    /*!< Timeout for write semaphore */
    uint32_t              writeTimeout;

    /*!< Pointer to device specific custom params */
    void                 *customParams;
} I2S_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_CloseFxn().
 */
typedef void (*I2S_CloseFxn) (I2S_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_control().
 */
typedef int_fast16_t (*I2S_ControlFxn)(I2S_Handle handle,
                                       uint_fast16_t cmd,
                                       void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_init().
 */
typedef void (*I2S_InitFxn)(I2S_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_OpenFxn().
 */
typedef I2S_Handle (*I2S_OpenFxn)(I2S_Handle handle, I2S_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_IssueFxn().
 */
typedef int_fast16_t (*I2S_IssueFxn)(I2S_Handle handle, I2S_BufDesc *desc);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2S_ReclaimFxn().
 */
typedef size_t (*I2S_ReclaimFxn)(I2S_Handle handle, I2S_BufDesc **desc);

/*!
 *  @brief      The definition of a I2S function table that contains the
 *              required set of functions to control a specific I2S driver
 *              implementation.
 */
typedef struct I2S_FxnTable_ {
    /*! Function to close the specified peripheral */
    I2S_CloseFxn           closeFxn;

    /*! Function to implementation specific control function */
    I2S_ControlFxn         controlFxn;

    /*! Function to initialize the given data object */
    I2S_InitFxn            initFxn;

    /*! Function to open the specified peripheral */
    I2S_OpenFxn            openFxn;

    /*! Function to queue a buffer for reading from the specified peripheral */
    I2S_IssueFxn           readIssueFxn;

    /*! Function to retrieve a received buffer of data from the specified peripheral */
    I2S_ReclaimFxn         readReclaimFxn;

    /*! Function to queue a buffer for writing from the specified peripheral */
    I2S_IssueFxn           writeIssueFxn;

    /*! Function to retrieve a sent buffer of data from the specified peripheral */
    I2S_ReclaimFxn         writeReclaimFxn;

} I2S_FxnTable;

/*! @brief  I2S Global configuration
 *
 *  The I2S_Config structure contains a set of pointers used to characterize
 *  the I2S driver implementation.
 *
 *  This structure needs to be defined before calling I2S_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     I2S_init()
 */
typedef struct I2S_Config_ {
    /*! Pointer to a table of a driver-specific implementation of I2S
        functions */
    I2S_FxnTable const    *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void                   *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void          const    *hwAttrs;
} I2S_Config;

/*!
 *  @brief  Function to close a given I2S peripheral specified by the I2S
 *  handle.
 *
 *  @pre    I2S_open() had to be called first.
 *
 *  @param  handle  A I2S_Handle returned from I2S_open
 *
 *  @sa     I2S_open()
 */
extern void I2S_close(I2S_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          I2S_Handle.
 *
 *  Commands for I2S_control can originate from I2S.h or from
 *  implementation specific I2S*.h (_I2SCC32XX.h_, etc.. ) files.
 *  While commands from I2S.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific I2S*.h files add
 *  unique driver capabilities but are not API portable across all I2S driver
 *  implementations.
 *
 *  Commands supported by I2S.h follow a I2S_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by I2S*.h follow a I2S*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref I2S_CMD "I2S_control command codes" for command codes.
 *
 *  See @ref I2S_STATUS "I2S_control return status codes" for status codes.
 *
 *  @pre    I2S_open() has to be called first.
 *
 *  @param  handle A I2S handle returned from I2S_open()
 *
 *  @param  cmd    I2S.h or I2S*.h commands.
 *
 *  @param  arg    An optional R/W (read/write) command argument
 *                 accompanied with cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     I2S_open()
 */
extern int_fast16_t I2S_control(I2S_Handle handle,
                                uint_fast16_t cmd,
                                void *arg);

/*!
 *  @brief  Function to initializes the I2S module
 *
 *  @pre    The I2S_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other I2S driver APIs. This function call does not modify any
 *          peripheral registers.
 */
extern void I2S_init(void);

/*!
 *  @brief  Function to initialize a given I2S peripheral specified by the
 *          particular index value. The parameter specifies which mode the I2S
 *          will operate.
 *
 *  @pre    I2S controller has been initialized
 *
 *  @param  index         Logical peripheral number for the I2S indexed into
 *                        the I2S_config table
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A I2S_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     I2S_init()
 *  @sa     I2S_close()
 */
extern I2S_Handle I2S_open(uint_least8_t index, I2S_Params *params);

/*!
 *  @brief  Function to initialize the I2S_Params struct to its defaults
 *
 *  @param  params      An pointer to I2S_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *  @code
 *  params.operationMode        = #I2S_OPMODE_TX_RX_SYNC;
 *  params.samplingFrequency    = 16000;
 *  params.slotLength           = 16;
 *  params.bitsPerSample        = 16;
 *  params.numChannels          = 2;
 *  params.readMode             = #I2S_MODE_ISSUERECLAIM;
 *  params.readCallback         = NULL;
 *  params.readTimeout          = #I2S_WAIT_FOREVER;
 *  params.writeMode            = #I2S_MODE_ISSUERECLAIM;
 *  params.writeCallback        = NULL;
 *  params.writeTimeout         = #I2S_WAIT_FOREVER;
 *  params.customParams         = NULL;
 *  @endcode
 *
 *  @param  params  Parameter structure to initialize
 */
extern void I2S_Params_init(I2S_Params *params);

/*!
 *  @brief Function to queue a buffer of data to the I2S in callback mode
 *         for reading.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  desc        A pointer to a I2S_BufDesc object.  The bufPtr
 *                      and bufSize fields must be set to a buffer and the
 *                      size of the buffer before passing to this function.
 *  @return             Returns 0 if successful else would return
 *                      I2S_STATUS_UNDEFINEDCMD on an error.
 */
extern int_fast16_t I2S_read(I2S_Handle handle, I2S_BufDesc *desc);

/*!

 *  @brief Function to queue a buffer of data to the I2S in Issue/Reclaim
 *          mode for reading.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  desc        A pointer to a I2S_BufDesc object.  The bufPtr
 *                      and bufSize fields must be set to a buffer and the
 *                      size of the buffer before passing to this function.
 *  @return             Returns 0 if successful else would return
 *                      I2S_STATUS_UNDEFINEDCMD on an error.
 */

extern int_fast16_t I2S_readIssue(I2S_Handle handle, I2S_BufDesc *desc);

/*!
 *  @brief Function to retrieve a full buffer of data read by the I2S.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  pDesc       A pointer to a I2S_BufDesc pointer.
 *
 *  @return Returns the number of bytes read from the I2S, or 0 on timeout.
 */
extern size_t I2S_readReclaim(I2S_Handle handle, I2S_BufDesc **pDesc);

/*!
 *  @brief Function to queue a buffer of data to the I2S in
 *         callback mode for writing.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  desc        A pointer to a I2S_BufDesc object.  The bufPtr
 *                      and bufSize fields must be set to a buffer and the
 *                      size of the buffer before passing to this function.
 *  @return             Returns 0 if successful else would return
 *                      I2S_STATUS_UNDEFINEDCMD on an error.
 */
extern int_fast16_t I2S_write(I2S_Handle handle, I2S_BufDesc *desc);

/*!
 *  @brief Function to queue a buffer of data to the I2S in
 *         Issue/Reclaim mode for writing.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  desc        A pointer to a I2S_BufDesc object.  The bufPtr
 *                      and bufSize fields must be set to a buffer and the
 *                      size of the buffer before passing to this function.
 *  @return             Returns 0 if successful else would return
 *                      I2S_STATUS_UNDEFINEDCMD on an error.
 */
extern int_fast16_t I2S_writeIssue(I2S_Handle handle, I2S_BufDesc *desc);

/*!
 *  @brief Function to retrieve a buffer that the I2S has finished writing.
 *
 *  @param  handle      A I2S_Handle
 *
 *  @param  pDesc       A pointer to a I2S_BufDesc pointer.
 *
 *  @return Returns the number of bytes that have been written to the I2S,
 *          0 on timeout.
 */
extern size_t I2S_writeReclaim(I2S_Handle handle, I2S_BufDesc **pDesc);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_I2S__include */
