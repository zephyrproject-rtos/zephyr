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
 *  @file       I2CSlave.h
 *
 *  @brief      I2CSlave driver interface
 *
 *  The I2CSlave header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/I2CSlave.h>
 *  @endcode
 *
 *  # Operation #
 *  The I2CSlave driver operates as a slave on an I2C bus in either
 *  I2CSLAVE_MODE_BLOCKING or I2CSLAVE_MODE_CALLBACK.
 *  In blocking mode, the task's execution is blocked during the I2CSlave
 *  read/write transfer. When the transfer has completed, code execution will
 *  resume. In callback mode, the task's execution is not blocked, allowing
 *  for other transactions to be queued up or to process some other code. When
 *  the transfer has completed, the I2CSlave driver will call a user-specified
 *  callback function (from a HWI context).
 *
 *  The APIs in this driver serve as an interface to a typical TI-RTOS
 *  application. The specific peripheral implementations are responsible to
 *  create all the SYS/BIOS specific primitives to allow for thread-safe
 *  operation.
 *
 *  ## Opening the driver #
 *
 *  @code
 *  I2CSlave_Handle      handle;
 *  I2CSlave_Params      params;
 *
 *  I2CSlave_Params_init(&params);
 *  params.transferMode  = I2CSLAVE_MODE_CALLBACK;
 *  params.transferCallbackFxn = someI2CSlaveCallbackFunction;
 *  handle = I2CSlave_open(someI2CSlave_configIndexValue, &params);
 *  if (!handle) {
 *      System_printf("I2CSlave did not open");
 *  }
 *  @endcode
 *
 *  ## Transferring data #
 *  A I2CSlave transaction with a I2CSlave peripheral is started by calling
 *  I2CSlave_read() or I2CSlave_write().
 *  Each transfer is performed atomically with the I2CSlave peripheral.
 *
 *  @code
 *  ret = I2CSlave_read(i2cSlave, buffer, 5)
 *  if (!ret) {
 *      System_printf("Unsuccessful I2CSlave read");
 *  }
 *
 *  I2CSlave_write(i2cSlave, buffer, 3);
 *  if (!ret) {
 *      System_printf("Unsuccessful I2CSlave write");
 *  }

 *  @endcode
 *
 *  # Implementation #
 *
 *  This module serves as the main interface for TI-RTOS
 *  applications. Its purpose is to redirect the module's APIs to specific
 *  peripheral implementations which are specified using a pointer to a
 *  I2CSlave_FxnTable.
 *
 *  The I2CSlave driver interface module is joined (at link time) to a
 *  NULL-terminated array of I2CSlave_Config data structures named
 *  *I2CSlave_config*. *I2CSlave_config* is implemented in the application
 *  with each entry being an instance of a I2CSlave peripheral. Each entry in
 *  *I2CSlave_config* contains a:
 *  - (I2CSlave_FxnTable *) to a set of functions that implement an I2CSlave
 *  - (void *) data object that is associated with the I2CSlave_FxnTable
 *  - (void *) hardware attributes that are associated to the I2CSlave_FxnTable
 *
 *  # Instrumentation #
 *  The I2CSlave driver interface produces log statements if instrumentation is
 *  enabled.
 *
 *  Diagnostics Mask | Log details |
 *  ---------------- | ----------- |
 *  Diags_USER1      | basic operations performed |
 *  Diags_USER2      | detailed operations performed |
 *
 *  ============================================================================
 */

#ifndef ti_drivers_I2CSLAVE__include
#define ti_drivers_I2CSLAVE__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 *  @defgroup I2CSLAVE_CONTROL I2CSlave_control command and status codes
 *  These I2CSlave macros are reservations for I2CSlave.h
 *  @{
 */
/*!
 * Common I2CSlave_control command code reservation offset.
 * I2CSlave driver implementations should offset command codes with
 * I2CSLAVE_CMD_RESERVED growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define I2CSLAVEXYZ_COMMAND0          I2CSLAVE_CMD_RESERVED + 0
 * #define I2CSLAVEXYZ_COMMAND1          I2CSLAVE_CMD_RESERVED + 1
 * @endcode
 */
#define I2CSLAVE_CMD_RESERVED             (32)

/*!
 * Common I2CSlave_control status code reservation offset.
 * I2CSlave driver implementations should offset status codes with
 * I2CSLAVE_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define I2CSLAVEXYZ_STATUS_ERROR0     I2CSLAVE_STATUS_RESERVED - 0
 * #define I2CSLAVEXYZ_STATUS_ERROR1     I2CSLAVE_STATUS_RESERVED - 1
 * #define I2CSLAVEXYZ_STATUS_ERROR2     I2CSLAVE_STATUS_RESERVED - 2
 * @endcode
 */
#define I2CSLAVE_STATUS_RESERVED         (-32)

/**
 *  @defgroup I2CSLAVE_STATUS Status Codes
 *  I2CSLAVE_STATUS_SUCCESS_* macros are general status codes returned by I2CSlave_control()
 *  @{
 *  @ingroup I2CSLAVE_CONTROL
 */
/*!
 * @brief    Successful status code returned by I2CSlave_control().
 *
 * I2CSlave_control() returns I2CSLAVE_STATUS_SUCCESS if the control code was
 * executed successfully.
 */
#define I2CSLAVE_STATUS_SUCCESS          (0)

/*!
 * @brief    Generic error status code returned by I2CSlave_control().
 *
 * I2CSlave_control() returns I2CSLAVE_STATUS_ERROR if the control code was not
 * executed successfully.
 */
#define I2CSLAVE_STATUS_ERROR           (-1)

/*!
 * @brief    An error status code returned by I2CSlave_control() for undefined
 * command codes.
 *
 * I2CSlave_control() returns I2CSLAVE_STATUS_UNDEFINEDCMD if the control code
 * is not recognized by the driver implementation.
 */
#define I2CSLAVE_STATUS_UNDEFINEDCMD   (-2)
/** @}*/

/**
 *  @defgroup I2CSLAVE_CMD Command Codes
 *  I2C_CMD_* macros are general command codes for I2CSlave_control(). Not all I2CSlave
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup I2CSLAVE_CONTROL
 */

/* Add I2CSLAVE_CMD_<commands> here */

/** @}*/

/** @}*/

/*!
 *  @brief      A handle that is returned from a I2CSlave_open() call.
 */
typedef struct I2CSlave_Config_      *I2CSlave_Handle;

/*!
 *  @brief  I2CSlave mode
 *
 *  This enum defines the state of the I2CSlave driver's state-machine. Do not
 *  modify.
 */
typedef enum I2CSlave_Mode_ {
    I2CSLAVE_IDLE_MODE = 0,  /*!< I2CSlave is not performing a transaction */
    I2CSLAVE_WRITE_MODE = 1, /*!< I2CSlave is currently performing write */
    I2CSLAVE_READ_MODE = 2,  /*!< I2CSlave is currently performing read */
    I2CSLAVE_START_MODE = 3, /*!< I2CSlave received a START from a master */
    I2CSLAVE_ERROR = 0xFF    /*!< I2CSlave error has occurred, exit gracefully */
} I2CSlave_Mode;

/*!
 *  @brief  I2CSlave transfer mode
 *
 *  I2CSLAVE_MODE_BLOCKING block task execution      a I2CSlave transfer is in
 *  progress. I2CSLAVE_MODE_CALLBACK does not block task execution; but calls a
 *  callback function when the I2CSlave transfer has completed
 */
typedef enum I2CSlave_TransferMode_ {
    I2CSLAVE_MODE_BLOCKING,  /*!< I2CSlave read/write blocks execution*/
    I2CSLAVE_MODE_CALLBACK   /*!< I2CSlave read/wrire queues transactions and
                                  does not block */
} I2CSlave_TransferMode;

/*!
 *  @brief  I2CSlave callback function
 *
 *  User definable callback function prototype. The I2CSlave driver will call
 *  the defined function and pass in the I2CSlave driver's handle, and the
 *  return value of I2CSlave_read/I2CSlave_write.
 *
 *  @param  I2CSlave_Handle          I2CSlave_Handle

 *  @param  bool                     Results of the I2CSlave transaction
 */
typedef void (*I2CSlave_CallbackFxn)(I2CSlave_Handle handle, bool status);

/*!
 *  @brief  I2CSlave Parameters
 *
 *  I2CSlave parameters are used to with the I2CSlave_open() call. Default
 *  values for
 *  these parameters are set using I2CSlave_Params_init().
 *
 *  If I2CSlave_TransferMode is set to I2CSLAVE_MODE_BLOCKING then I2CSlave_read
 *  or I2CSlave_write function calls will block thread execution until the
 *  transaction has completed.
 *
 *  If I2CSlave_TransferMode is set to I2CSLAVE_MODE_CALLBACK then
 *  I2CSlave read/write will not block thread execution and it will call the
 *  function specified by transferCallbackFxn.
 *  (regardless of error state).
 *
 *
 *  @sa     I2CSlave_Params_init()
 */
typedef struct I2CSlave_Params_ {
    /*!< Blocking or Callback mode */
    I2CSlave_TransferMode   transferMode;
    /*!< Callback function pointer */
    I2CSlave_CallbackFxn    transferCallbackFxn;
    /*!< Custom argument used by driver implementation */
    void                   *custom;
} I2CSlave_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_close().
 */
typedef void        (*I2CSlave_CloseFxn)    (I2CSlave_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_control().
 */
typedef int_fast16_t (*I2CSlave_ControlFxn)  (I2CSlave_Handle handle,
                                        uint_fast16_t cmd,
                                        void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_init().
 */
typedef void        (*I2CSlave_InitFxn)     (I2CSlave_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_open().
 */
typedef I2CSlave_Handle  (*I2CSlave_OpenFxn)     (I2CSlave_Handle handle,
                                        I2CSlave_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_WriteTransaction().
 */
typedef bool        (*I2CSlave_WriteFxn) (I2CSlave_Handle handle,
        const void *buffer, size_t size);


/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2CSlave_ReadFxn().
 */
typedef bool        (*I2CSlave_ReadFxn) (I2CSlave_Handle handle, void *buffer,
        size_t size);


/*!
 *  @brief      The definition of a I2CSlave function table that contains the
 *              required set of functions to control a specific I2CSlave
 *              driver implementation.
 */
typedef struct I2CSlave_FxnTable_ {
    /*! Function to close the specified peripheral */
    I2CSlave_CloseFxn        closeFxn;

    /*! Function to implementation specific control function */
    I2CSlave_ControlFxn      controlFxn;

    /*! Function to initialize the given data object */
    I2CSlave_InitFxn         initFxn;

    /*! Function to open the specified peripheral */
    I2CSlave_OpenFxn         openFxn;

    /*! Function to initiate a I2CSlave data read */
    I2CSlave_ReadFxn     readFxn;

    /*! Function to initiate a I2CSlave data write */
    I2CSlave_WriteFxn  writeFxn;
} I2CSlave_FxnTable;

/*!
 *  @brief  I2CSlave Global configuration
 *
 *  The I2CSlave_Config structure contains a set of pointers used to
 *  characterize the I2CSlave driver implementation.
 *
 *  This structure needs to be defined before calling I2CSlave_init() and it
 *  must not be changed thereafter.
 *
 *  @sa     I2CSlave_init()
 */
typedef struct I2CSlave_Config_ {
    /*! Pointer to a table of driver-specific implementations of I2CSlave APIs*/
    I2CSlave_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void               *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void         const *hwAttrs;
} I2CSlave_Config;


/*!
 *  @brief  Function to close a I2CSlave peripheral specified by the I2CSlave
 *  handle
 *  @pre    I2CSlave_open() had to be called first.
 *
 *  @param  handle  A I2CSlave_Handle returned from I2CSlave_open
 *
 *  @sa     I2CSlave_open()
 */
extern void I2CSlave_close(I2CSlave_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          I2CSlave_Handle.
 *
 *  Commands for I2CSlave_control can originate from I2CSlave.h or from implementation
 *  specific I2CSlave*.h (_I2CMSP432.h_, etc.. ) files.
 *  While commands from I2CSlave.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific I2CSlave*.h files add
 *  unique driver capabilities but are not API portable across all I2CSlave driver
 *  implementations.
 *
 *  Commands supported by I2CSlave.h follow a I2CSLAVE_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by I2CSlave*.h follow a I2CSLAVE*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref I2CSLAVE_CMD "I2CSlave_control command codes" for command codes.
 *
 *  See @ref I2CSLAVE_STATUS "I2CSlave_control return status codes" for status codes.
 *
 *  @pre    I2CSlave_open() has to be called first.
 *
 *  @param  handle      A I2CSlave handle returned from I2CSlave_open()
 *
 *  @param  cmd         A command value defined by the driver specific
 *                      implementation
 *
 *  @param  arg         An optional R/W (read/write) argument that is
 *                      accompanied with cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     I2CSlave_open()
 */
extern int_fast16_t I2CSlave_control(I2CSlave_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief  Function to initializes the I2CSlave module
 *
 *  @pre    The I2CSlave_config structure must exist and be persistent before
 *          this function can be called. This function must also be called
 *          before any other I2CSlave driver APIs. This function call does not
 *          modify any peripheral registers.
 */
extern void I2CSlave_init(void);

/*!
 *  @brief  Function to initialize a given I2CSlave peripheral specified by the
 *          particular index value. The parameter specifies which mode the
 *          I2CSlave will operate.
 *
 *  @pre    I2CSlave controller has been initialized
 *
 *  @param  index         Logical peripheral number for the I2CSlave indexed
 *                        into the I2CSlave_config table
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A I2CSlave_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     I2CSlave_init()
 *  @sa     I2CSlave_close()
 */
extern I2CSlave_Handle I2CSlave_open(uint_least8_t index,
    I2CSlave_Params *params);

/*!
 *  @brief  Function to initialize the I2CSlave_Params struct to its defaults
 *
 *  @param  params      An pointer to I2CSlave_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      transferMode = I2CSLAVE_MODE_BLOCKING
 *      transferCallbackFxn = NULL
 */
extern void I2CSlave_Params_init(I2CSlave_Params *params);

/*!
 *  @brief  Function that handles the I2CSlave read for SYS/BIOS
 *
 *  This function will start a I2CSlave read and can only be called from a
 *  Task context when in I2CSLAVE_MODE_BLOCKING.
 *  The I2CSlave read procedure starts with evaluating how many bytes are to be
 *  readby the I2CSlave peripheral.
 *
 *  The data written by the I2CSlave is synchronized with the START and STOP
 *  from the master.
 *
 *  In I2CSLAVE_MODE_BLOCKING, I2CSlave read/write will block task execution until
 *  the transaction has completed.
 *
 *  In I2CSLAVE_MODE_CALLBACK, I2CSlave read/write does not block task execution
 *  and calls a callback function specified by transferCallbackFxn. If a
 *  transfer is already taking place, the transaction is put on an internal
 *  queue. The queue is serviced in a first come first served basis.
 *
 *  @param  handle      A I2CSlave_Handle
 *
 *  @param  buffer      A RO (read-only) pointer to an empty buffer in which
 *                      received data should be written to.
 *
 *  @param  size        The number of bytes to be written into buffer
 *
 *  @return true on successful transfer
 *          false on an error
 *
 *  @sa     I2CSlave_open
 */

extern bool I2CSlave_read(I2CSlave_Handle handle, void *buffer,
        size_t size);
/*!
 *  @brief  Function that handles the I2CSlave write for SYS/BIOS
 *
 *  This function will start a I2CSlave write and can only be called from a
 *  Task context when in I2CSLAVE_MODE_BLOCKING.
 *  The I2CSlave transfer procedure starts with evaluating how many bytes are
 *  to be written.
 *
 *  The data written by the I2CSlave is synchronized with the START and STOP
 *  from the master. If slave does not have as many bytes requested by master
 *  it writes 0xFF. I2CSlave keeps sending 0xFF till master sends a STOP.
 *
 *  In I2CSLAVE_MODE_BLOCKING, I2CSlave read/write will block task execution
 *  until the transaction has completed.
 *
 *  In I2CSLAVE_MODE_CALLBACK, I2CSlave read/write does not block task execution
 *  and calls a callback function specified by transferCallbackFxn. If a
 *  transfer is already taking place, the transaction is put on an internal
 *  queue. The queue is serviced in a first come first served basis.
 *  The I2CSlave_Transaction structure must stay persistent until the
 *   I2CSlave read/write function has completed!
 *
 *  @param  handle      A I2CSlave_Handle
 *
 *  @param  buffer      A WO (write-only) pointer to buffer containing data to
 *                      be written to the master.
 *
 *  @param  size        The number of bytes in buffer that should be written
 *                      onto the master.
 *
 *  @return true on successful write
 *          false on an error
 *
 *  @sa     I2CSlave_open
 */
extern bool I2CSlave_write(I2CSlave_Handle handle, const void *buffer,
        size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_I2CSLAVE__include */
