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
 *  @file       I2C.h
 *
 *  @brief      I2C driver interface
 *
 *  The I2C driver interface provides device independent APIs, data types,
 *  and macros. The I2C header file should be included in an application as
 *  follows:
 *  @code
 *  #include <ti/drivers/I2C.h>
 *  @endcode
 *
 *  # Overview #
 *  This section assumes that you have background knowledge and understanding
 *  about how the I2C protocol operates. For the full I2C specifications and
 *  user manual (UM10204), see the NXP Semiconductors website.
 *
 *  The I2C driver has been designed to operate as a single I2C master by
 *  performing I2C transactions between the target and I2C slave peripherals.
 *  The I2C driver does not support I2C slave mode.
 *  I2C is a communication protocol - the specifications define how data
 *  transactions are to occur via the I2C bus. The specifications do not
 *  define how data is to be formatted or handled, allowing for flexible
 *  implementations across different peripheral vendors. As a result, the
 *  I2C handles only the exchange of data (or transactions) between master
 *  and slaves. It is the left to the application to interpret and
 *  manipulate the contents of each specific I2C peripheral.
 *
 *  The I2C driver has been designed to operate in an RTOS environment.  It
 *  protects its transactions with OS primitives supplied by the underlying
 *  RTOS.
 *
 *  # Usage #
 *
 *  The I2C driver includes the following APIs:
 *    - I2C_init(): Initialize the I2C driver.
 *    - I2C_Params_init():  Initialize an #I2C_Params structure with default
 *      vaules.
 *    - I2C_open():  Open an instance of the I2C driver.
 *    - I2C_control():  Performs implemenation-specific features on a given
 *      I2C peripheral.
 *    - I2C_transfer():  Transfer the data.
 *    - I2C_close():  De-initialize the I2C instance.
 *
 *
 *  ### I2C Driver Configuration #
 *
 *  In order to use the I2C APIs, the application is required
 *  to provide device-specific I2C configuration in the Board.c file.
 *  The I2C driver interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct I2C_Config_ {
 *      I2C_FxnTable  const    *fxnTablePtr;
 *      void                   *object;
 *      void          const    *hwAttrs;
 *  } I2C_Config;
 *  @endcode
 *
 *  The application must declare an array of I2C_Config elements, named
 *  I2C_config[].  Each element of I2C_config[] must be populated with
 *  pointers to a device specific I2C driver implementation's function
 *  table, driver object, and hardware attributes.  The hardware attributes
 *  define properties such as the I2C peripheral's base address and
 *  pins.  Each element in I2C_config[] corresponds to
 *  an I2C instance, and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the
 *  peripheral designation (such as I2C0 or I2C1).  For example, it is
 *  possible to use I2C_config[0] for I2C1.
 *
 *  Because the I2C configuration is very device dependent, you will need to
 *  check the doxygen for the device specific I2C implementation.  There you
 *  will find a description of the I2C hardware attributes.  Please also
 *  refer to the Board.c file of any of your examples to see the I2C
 *  configuration.
 *
 *  ### Initializing the I2C Driver #
 *
 *  I2C_init() must be called before any other I2C APIs.  This function
 *  iterates through the elements of the I2C_config[] array, calling
 *  the element's device implementation I2C initialization function.
 *
 *  ### I2C Parameters
 *
 *  The #I2C_Params structure is passed to the I2C_open() call.  If NULL
 *  is passed for the parameters, I2C_open() uses default parameters.
 *  An #I2C_Params structure is initialized with default values by passing
 *  it to I2C_Params_init().
 *  Some of the I2C parameters are described below.  To see brief descriptions
 *  of all the parameters, see #I2C_Params.
 *
 *  #### I2C Transfer Mode
 *  The I2C driver supports two transfer modes of operation: blocking and
 *  callback:
 *  - #I2C_MODE_BLOCKING: The call to I2C_transfer() blocks until the
 *    transfer completes.
 *  - #I2C_MODE_CALLBACK: The call to I2C_transfer() returns immediately.
 *    When the transfer completes, the I2C driver will call a user-
 *    specified callback function.
 *
 *  The transfer mode is determined by the #I2C_Params.transferMode parameter
 *  passed to I2C_open().  The I2C driver defaults to blocking mode, if the
 *  application does not set it.
 *
 *  In blocking mode, a task calling I2C_transfer() is blocked until the
 *  transaction completes.  Other tasks requesting I2C transactions while
 *  a transaction is currently taking place, are also placed into a
 *  blocked state.
 *
 *  In callback mode, an I2C_transfer() functions asynchronously, which
 *  means that it does not block a calling task's execution.  In this
 *  mode, the user must set #I2C_Params.transferCallbackFxn to a user-
 *  provided callback function. After an I2C transaction has completed,
 *  the I2C driver calls the user- provided callback function.
 *  If another I2C transaction is requested, the transaction is queued up.
 *  As each transfer completes, the I2C driver will call the user-specified
 *  callback function.  The user callback will be called from either hardware
 *  or software interrupt context, depending upon the device implementation.
 *
 *  Once an I2C driver instance is opened, the
 *  only way to change the transfer mode is to close and re-open the I2C
 *  instance with the new transfer mode.
 *
 *  #### Specifying an I2C Bus Frequency
 *  The I2C controller's bus frequency is determined by #I2C_Params.bitRate
 *  passed to I2C_open().  The standard I2C bus frequencies are 100 kHz and
 *  400 kHz, with 100 kHz being the default.
 *
 *  ### Opening the I2C Driver #
 *  After initializing the I2C driver by calling I2C_init(), the application
 *  can open an I2C instance by calling I2C_open().  This function
 *  takes an index into the I2C_config[] array and an I2C parameters data
 *  structure.   The I2C instance is specified by the index of the I2C in
 *  I2C_config[].  Only one I2C index can be used at a time;
 *  calling I2C_open() a second time with the same index previosly
 *  passed to I2C_open() will result in an error.  You can,
 *  though, re-use the index if the instance is closed via I2C_close().
 *
 *  If no I2C_Params structure is passed to I2C_open(), default values are
 *  used. If the open call is successful, it returns a non-NULL value.
 *
 *  Example opening an I2C driver instance in blocking mode:
 *  @code
 *  I2C_Handle i2c;
 *
 *  // NULL params are used, so default to blocking mode, 100 KHz
 *  i2c = I2C_open(Board_I2C0, NULL);
 *
 *  if (!i2c) {
 *      // Error opening the I2C
 *  }
 *  @endcode
 *
 *  Example opening an I2C driver instance in callback mode and 400KHz bit rate:
 *
 *  @code
 *  I2C_Handle i2c;
 *  I2C_Params params;
 *
 *  I2C_Params_init(&params);
 *  params.transferMode  = I2C_MODE_CALLBACK;
 *  params.transferCallbackFxn = myCallbackFunction;
 *  params.bitRate  = I2C_400kHz;
 *
 *  handle = I2C_open(Board_I2C0, &params);
 *  if (!i2c) {
 *      // Error opening I2C
 *  }
 *  @endcode
 *
 *  ### Transferring data #
 *  An I2C transaction with an I2C peripheral is started by calling
 *  I2C_transfer().  Three types of transactions are supported: Write, Read,
 *  or Write/Read. Each transfer is completed before another transfer is
 *  initiated.
 *
 *  For Write/Read transactions, the specified data is first written to the
 *  peripheral, then a repeated start is sent by the driver, which initiates
 *  the read operation.  This type of transfer is useful if an I2C peripheral
 *  has a pointer register that needs to be adjusted prior to reading from
 *  the referenced data register.
 *
 *  The details of each transaction are specified with an #I2C_Transaction data
 *  structure. This structure defines the slave I2C address, pointers
 *  to write and read buffers, and their associated byte counts. If
 *  no data needs to be written or read, the corresponding byte counts should
 *  be set to zero.
 *
 *  If an I2C transaction is requested while a transaction is currently
 *  taking place, the new transaction is placed onto a queue to be processed
 *  in the order in which it was received.
 *
 *  The below example shows sending three bytes of data to a slave peripheral
 *  at address 0x50, in blocking mode:
 *
 *  @code
 *  unsigned char writeBuffer[3];
 *  I2C_Transaction i2cTransaction;
 *
 *  i2cTransaction.slaveAddress = 0x50;
 *  i2cTransaction.writeBuf = writeBuffer;
 *  i2cTransaction.writeCount = 3;
 *  i2cTransaction.readBuf = NULL;
 *  i2cTransaction.readCount = 0;
 *
 *  status = I2C_transfer(i2c, &i2cTransaction);
 *  if (!status) {
 *      // Unsuccessful I2C transfer
 *  }
 *  @endcode
 *
 *  The next example shows reading of five bytes of data from the I2C
 *  peripheral, also in blocking mode:
 *
 *  @code
 *  unsigned char readBuffer[5];
 *  I2C_Transaction i2cTransaction;
 *
 *  i2cTransaction.slaveAddress = 0x50;
 *  i2cTransaction.writeBuf = NULL;
 *  i2cTransaction.writeCount = 0;
 *  i2cTransaction.readBuf = readBuffer;
 *  i2cTransaction.readCount = 5;
 *
 *  status = I2C_transfer(i2c, &i2cTransaction);
 *  if (!status) {
 *      // Unsuccessful I2C transfer
 *  }
 *  @endcode
 *
 *  This example shows writing of two bytes and reading of four bytes in a
 *  single transaction.
 *
 *  @code
 *  unsigned char readBuffer[4];
 *  unsigned char writeBuffer[2];
 *  I2C_Transaction i2cTransaction;
 *
 *  i2cTransaction.slaveAddress = 0x50;
 *  i2cTransaction.writeBuf = writeBuffer;
 *  i2cTransaction.writeCount = 2;
 *  i2cTransaction.readBuf = readBuffer;
 *  i2cTransaction.readCount = 4;
 *
 *  status = I2C_transfer(i2c, &i2cTransaction);
 *  if (!status) {
 *      // Unsuccessful I2C transfer
 *  }
 *  @endcode
 *
 *  This final example shows usage of asynchronous callback mode, with queuing
 *  of multiple transactions.  Because multiple transactions are simultaneously
 *  queued, separate I2C_Transaction structures must be used.  (This is a
 *  general rule, that I2C_Transaction structures cannot be reused until
 *  it is known that the previous transaction has completed.)
 *
 *  First, for the callback function (that is specified in the I2C_open() call)
 *  the "arg" in the I2C_Transaction structure is a semaphore handle. When
 *  this value is non-NULL, sem_post() is called in the callback using
 *  the specified handle, to signal completion to the task that queued the
 *  transactions:
 *
 *  @code
 *  Void callbackFxn(I2C_Handle handle, I2C_Transaction *msg, Bool transfer) {
 *      if (msg->arg != NULL) {
 *          sem_post((sem_t *)(msg->arg));
 *      }
 *  }
 *  @endcode
 *
 *  Snippets of the task code that initiates the transactions are shown below.
 *  Note the use of multiple I2C_Transaction structures, and passing of the
 *  handle of the semaphore to be posted via i2cTransaction2.arg.
 *  I2C_transfer() is called three times to initiate each transaction.
 *  Since callback mode is used, these functions return immediately.  After
 *  the transactions have been queued, other work can be done, and then
 *  eventually sem_wait() is called to wait for the last I2C
 *  transaction to complete.  Once the callback posts the semaphore the task
 *  will be moved to the ready state, so the task can resume execution.
 *
 *  @code
 *  Void taskfxn(arg0, arg1) {
 *
 *      I2C_Transaction i2cTransaction0;
 *      I2C_Transaction i2cTransaction1;
 *      I2C_Transaction i2cTransaction2;
 *
 *      ...
 *      i2cTransaction0.arg = NULL;
 *      i2cTransaction1.arg = NULL;
 *      i2cTransaction2.arg = semaphoreHandle;
 *
 *      ...
 *      I2C_transfer(i2c, &i2cTransaction0);
 *      I2C_transfer(i2c, &i2cTransaction1);
 *      I2C_transfer(i2c, &i2cTransaction2);
 *
 *      ...
 *
 *      sem_wait(semaphoreHandle);
 *
 *      ...
 *  }
 *  @endcode
 *
 *  # Implementation #
 *
 *  This top-level I2C module serves as the main interface for RTOS
 *  applications. Its purpose is to redirect the module's APIs to specific
 *  peripheral implementations which are specified using a pointer to an
 *  #I2C_FxnTable.
 *
 *  The I2C driver interface module is joined (at link time) to an
 *  array of I2C_Config data structures named *I2C_config*.
 *  *I2C_config* is typically defined in the Board.c file used for the
 *  application.  If there are multiple instances of I2C peripherals on the
 *  device, there will typically be multiple I2C_Config structures defined in
 *  the board file. Each entry in *I2C_config* contains a:
 *  - (I2C_FxnTable *) to a set of functions that implement a I2C peripheral
 *  - (void *) data object that is associated with the I2C_FxnTable
 *  - (void *) hardware attributes that are associated to the I2C_FxnTable
 *
 *******************************************************************************
 */

#ifndef ti_drivers_I2C__include
#define ti_drivers_I2C__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 *  @defgroup I2C_CONTROL I2C_control command and status codes
 *  These I2C macros are reservations for I2C.h
 *  @{
 */

/*!
 * Common I2C_control command code reservation offset.
 * I2C driver implementations should offset command codes with I2C_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define I2CXYZ_CMD_COMMAND0      I2C_CMD_RESERVED + 0
 * #define I2CXYZ_CMD_COMMAND1      I2C_CMD_RESERVED + 1
 * @endcode
 */
#define I2C_CMD_RESERVED           (32)

/*!
 * Common I2C_control status code reservation offset.
 * I2C driver implementations should offset status codes with
 * I2C_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define I2CXYZ_STATUS_ERROR0     I2C_STATUS_RESERVED - 0
 * #define I2CXYZ_STATUS_ERROR1     I2C_STATUS_RESERVED - 1
 * #define I2CXYZ_STATUS_ERROR2     I2C_STATUS_RESERVED - 2
 * @endcode
 */
#define I2C_STATUS_RESERVED        (-32)

/**
 *  @defgroup I2C_STATUS Status Codes
 *  I2C_STATUS_* macros are general status codes returned by I2C_control()
 *  @{
 *  @ingroup I2C_CONTROL
 */

/*!
 * @brief   Successful status code returned by I2C_control().
 *
 * I2C_control() returns I2C_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define I2C_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code returned by I2C_control().
 *
 * I2C_control() returns I2C_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define I2C_STATUS_ERROR           (-1)

/*!
 * @brief   An error status code returned by I2C_control() for undefined
 * command codes.
 *
 * I2C_control() returns I2C_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define I2C_STATUS_UNDEFINEDCMD    (-2)
/** @}*/

/**
 *  @defgroup I2C_CMD Command Codes
 *  I2C_CMD_* macros are general command codes for I2C_control(). Not all I2C
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup I2C_CONTROL
 */

/* Add I2C_CMD_<commands> here */

/** @}*/

/** @}*/

/*!
 *  @brief      A handle that is returned from an I2C_open() call.
 */
typedef struct I2C_Config_    *I2C_Handle;

/*!
 *  @brief  I2C transaction
 *
 *  This structure defines an I2C transaction. It specifies the buffer(s) and
 *  buffer size(s) to be written to and/or read from an I2C slave peripheral.
 *  arg is an optional user-supplied argument that will be passed
 *  to the user-supplied callback function when the I2C driver is in
 *  I2C_MODE_CALLBACK.
 *  nextPtr is a pointer used internally by the driver for queuing of multiple
 *  transactions; this value must never be modified by the user application.
 */
typedef struct I2C_Transaction_ {
    void    *writeBuf;    /*!< Buffer containing data to be written */
    size_t   writeCount;  /*!< Number of bytes to be written to the slave */

    void    *readBuf;     /*!< Buffer to which data is to be read into */
    size_t   readCount;   /*!< Number of bytes to be read from the slave */

    uint_least8_t slaveAddress; /*!< Address of the I2C slave peripheral */

    void    *arg;         /*!< Argument to be passed to the callback function */
    void    *nextPtr;     /*!< Used for queuing in I2C_MODE_CALLBACK mode */
} I2C_Transaction;

/*!
 *  @brief  I2C transfer mode
 *
 *  I2C_MODE_BLOCKING blocks task execution while an I2C transfer is in
 *  progress.
 *  I2C_MODE_CALLBACK does not block task execution, but calls a callback
 *  function when the I2C transfer has completed.
 */
typedef enum I2C_TransferMode_ {
    I2C_MODE_BLOCKING,  /*!< I2C_transfer() blocks execution */
    I2C_MODE_CALLBACK   /*!< I2C_transfer() does not block */
} I2C_TransferMode;

/*!
 *  @brief  I2C callback function
 *
 *  User-definable callback function prototype. The I2C driver will call this
 *  callback upon transfer completion, specifying the I2C handle for the
 *  transfer (as returned from I2C_open()), the pointer to the I2C_Transaction
 *  that just completed, and the return value of I2C_transfer().  Note that
 *  this return value will be the same as if the transfer were performed in
 *  blocking mode.
 *
 *  @param  I2C_Handle          I2C_Handle

 *  @param  I2C_Transaction*    Address of the I2C_Transaction

 *  @param  bool                Result of the I2C transfer
 */
typedef void (*I2C_CallbackFxn)(I2C_Handle handle, I2C_Transaction *transaction,
    bool transferStatus);

/*!
 *  @brief  I2C bitRate
 *
 *  Specifies one of the standard I2C bus bit rates for I2C communications.
 *  The default is I2C_100kHz.
 */
typedef enum I2C_BitRate_ {
    I2C_100kHz = 0,
    I2C_400kHz = 1
} I2C_BitRate;

/*!
 *  @brief  I2C Parameters
 *
 *  I2C parameters are used with the I2C_open() call. Default values for
 *  these parameters are set using I2C_Params_init().
 *
 *  If I2C_TransferMode is set to I2C_MODE_BLOCKING, I2C_transfer() function
 *  calls will block thread execution until the transaction has completed.  In
 *  this case, the transferCallbackFxn parameter will be ignored.
 *
 *  If I2C_TransferMode is set to I2C_MODE_CALLBACK, I2C_transfer() will not
 *  block thread execution, but it will call the function specified by
 *  transferCallbackFxn upon transfer completion. Sequential calls to
 *  I2C_transfer() in I2C_MODE_CALLBACK will put the I2C_Transaction structures
 *  onto an internal queue that automatically starts queued transactions after
 *  the previous transaction has completed. This queuing occurs regardless of
 *  any error state from previous transactions.
 *
 *  I2C_BitRate specifies the I2C bus rate used for I2C communications.
 *
 *  @sa     I2C_Params_init()
 */
typedef struct I2C_Params_ {
    I2C_TransferMode transferMode;        /*!< Blocking or Callback mode */
    I2C_CallbackFxn  transferCallbackFxn; /*!< Callback function pointer */
    I2C_BitRate      bitRate;             /*!< I2C bus bit rate */
    void            *custom;              /*!< Custom argument used by driver
                                               implementation */
} I2C_Params;

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_cancel().
 */
typedef void (*I2C_CancelFxn) (I2C_Handle handle);

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_close().
 */
typedef void (*I2C_CloseFxn) (I2C_Handle handle);

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_control().
 */
typedef int_fast16_t (*I2C_ControlFxn) (I2C_Handle handle, uint_fast16_t cmd,
    void *controlArg);

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_init().
 */
typedef void (*I2C_InitFxn) (I2C_Handle handle);

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_open().
 */
typedef I2C_Handle (*I2C_OpenFxn) (I2C_Handle handle, I2C_Params *params);

/*!
 *  @brief      A function pointer to a driver-specific implementation of
 *              I2C_transfer().
 */
typedef bool (*I2C_TransferFxn) (I2C_Handle handle,
    I2C_Transaction *transaction);

/*!
 *  @brief      The definition of an I2C function table that contains the
 *              required set of functions to control a specific I2C driver
 *              implementation.
 */
typedef struct I2C_FxnTable_ {
    /*! Cancel all I2C data transfers */
    I2C_CancelFxn   cancelFxn;

    /*! Close the specified peripheral */
    I2C_CloseFxn    closeFxn;

    /*! Implementation-specific control function */
    I2C_ControlFxn  controlFxn;

    /*! Initialize the given data object */
    I2C_InitFxn     initFxn;

    /*! Open the specified peripheral */
    I2C_OpenFxn     openFxn;

    /*! Initiate an I2C data transfer */
    I2C_TransferFxn transferFxn;
} I2C_FxnTable;

/*!
 *  @brief  I2C global configuration
 *
 *  The I2C_Config structure contains a set of pointers used to characterize
 *  the I2C driver implementation.
 *
 *  This structure needs to be defined before calling I2C_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     I2C_init()
 */
typedef struct I2C_Config_ {
    /*! Pointer to a table of driver-specific implementations of I2C APIs */
    I2C_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver-specific data object */
    void               *object;

    /*! Pointer to a driver-specific hardware attributes structure */
    void         const *hwAttrs;
} I2C_Config;

/*!
 *  @brief  Cancel all I2C transfers
 *
 *  This function will cancel asynchronous I2C_transfer() operations, and is
 *  applicable only for I2C_MODE_CALLBACK.  An in progress transfer, as well
 *  as any queued transfers will be canceled. The individual callback functions
 *  for each transfer will be called from the context that I2C_cancel() is
 *  called.
 *
 *  @pre    I2C_Transfer() has been called.
 *
 *  @param  handle  An I2C_Handle returned from I2C_open()
 *
 *  @note   Different I2C slave devices will behave differently when an
 *          in-progress transfer fails and needs to be canceled.  The slave
 *          may need to be reset, or there may be other slave-specific
 *          steps that can be used to successfully resume communication.
 *
 *  @sa     I2C_transfer()
 */
extern void I2C_cancel(I2C_Handle handle);

/*!
 *  @brief  Close an I2C peripheral specified by an I2C_Handle
 *
 *  @pre    I2C_open() has been called.
 *
 *  @param  handle  An I2C_Handle returned from I2C_open()
 *
 *  @sa     I2C_open()
 */
extern void I2C_close(I2C_Handle handle);

/*!
 *  @brief  Perform implementation-specific features on a given
 *          I2C_Handle.
 *
 *  Commands for I2C_control() can originate from I2C.h or from implementation
 *  specific I2C*.h (I2CCC26XX.h_, I2CMSP432.h_, etc.) files.
 *  While commands from I2C.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific I2C*.h files add
 *  unique driver capabilities but are not API portable across all I2C driver
 *  implementations.
 *
 *  Commands supported by I2C.h follow a I2C_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by I2C*.h follow a I2C*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref I2C_CMD "I2C_control command codes" for command codes.
 *
 *  See @ref I2C_STATUS "I2C_control return status codes" for status codes.
 *
 *  @pre    I2C_open() has to be called first.
 *
 *  @param  handle      An I2C_Handle returned from I2C_open()
 *
 *  @param  cmd         I2C.h or I2C*.h command.
 *
 *  @param  controlArg  An optional R/W (read/write) command argument
 *                      accompanied with cmd
 *
 *  @return Implementation-specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     I2C_open()
 */
extern int_fast16_t I2C_control(I2C_Handle handle, uint_fast16_t cmd,
    void *controlArg);

/*!
 *  @brief  Initializes the I2C module
 *
 *  @pre    The I2C_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other I2C driver APIs. This function call does not modify any
 *          peripheral registers.
 */
extern void I2C_init(void);

/*!
 *  @brief  Initialize a given I2C peripheral as identified by an index value.
 *          The I2C_Params structure defines the operating mode, and any
 *          related settings.
 *
 *  @pre    The I2C controller has been initialized, via a previous call to
 *          I2C_init()
 *
 *  @param  index         Logical peripheral number for the I2C indexed into
 *                        the I2C_config table
 *
 *  @param  params        Pointer to a parameter block. Default values will be
 *                        used if NULL is specified for params. All the fields
 *                        in this structure are are considered RO (read-only).
 *
 *  @return An I2C_Handle on success, or NULL on an error, or if the peripheral
 *          is already opened.
 *
 *  @sa     I2C_init()
 *  @sa     I2C_close()
 */
extern I2C_Handle I2C_open(uint_least8_t index, I2C_Params *params);

/*!
 *  @brief  Initialize an I2C_Params struct to its defaults
 *
 *  @param  params      A pointer to I2C_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      transferMode = I2C_MODE_BLOCKING
 *      transferCallbackFxn = NULL
 *      bitRate = I2C_100kHz
 */
extern void I2C_Params_init(I2C_Params *params);

/*!
 *  @brief  Perform an I2C transaction with an I2C slave peripheral.
 *
 *  This function will perform an I2C transfer, as specified by an
 *  I2C_Transaction structure.
 *
 *  An I2C transaction may write data to a peripheral, or read data from a
 *  peripheral, or both write and read data, in a single transaction.  If there
 *  is any data to be written, it will always be sent before any data is read
 *  from the peripheral.
 *
 *  The data written to the peripheral is preceded with the peripheral's 7-bit
 *  I2C slave address (with the Write bit set).
 *  After all the data has been transmitted, the driver will evaluate if any
 *  data needs to be read from the device.
 *  If yes, another START bit is sent, along with the same 7-bit I2C slave
 *  address (with the Read bit).  After the specified number of bytes have been
 *  read, the transfer is ended with a NACK and a STOP bit.  Otherwise, if
 *  no data is to be read, the transfer is concluded with a STOP bit.
 *
 *  In I2C_MODE_BLOCKING, I2C_transfer() will block thread execution until the
 *  transaction completes. Therefore, this function must only be called from an
 *  appropriate thread context (e.g., Task context for the TI-RTOS kernel).
 *
 *  In I2C_MODE_CALLBACK, the I2C_transfer() call does not block thread
 *  execution. Instead, a callback function (specified during I2C_open(), via
 *  the transferCallbackFxn field in the I2C_Params structure) is called when
 *  the transfer completes. Success or failure of the transaction is reported
 *  via the callback function's bool argument. If a transfer is already in
 *  progress, the new transaction is put on an internal queue. The driver
 *  services the queue in a first come first served basis.
 *
 *  @param  handle      An I2C_Handle
 *
 *  @param  transaction A pointer to an I2C_Transaction. All of the fields
 *                      within the transaction structure should be considered
 *                      write only, unless otherwise noted in the driver
 *                      implementation.
 *
 *  @note The I2C_Transaction structure must persist unmodified until the
 *  corresponding call to I2C_transfer() has completed.
 *
 *  @return In I2C_MODE_BLOCKING: true for a successful transfer; false for an
 *          error (for example, an I2C bus fault (NACK)).
 *
 *          In I2C_MODE_CALLBACK: always true. The transferCallbackFxn's bool
 *          argument will be true to indicate success, and false to indicate
 *          an error.
 *
 *  @sa     I2C_open
 */
extern bool I2C_transfer(I2C_Handle handle, I2C_Transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_I2C__include */
