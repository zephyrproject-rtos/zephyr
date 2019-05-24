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
/*!****************************************************************************
 *  @file       NVS.h
 *  @brief      Non-Volatile Storage driver interface
 *
 *  To use the NVS driver, ensure that the correct driver library for your
 *  device is linked in and include this header file as follows:
 *  @code
 *  #include <ti/drivers/NVS.h>
 *  @endcode
 *
 *  This module serves as the main interface for applications. Its purpose
 *  is to redirect the NVS APIs to specific driver implementations
 *  which are specified using a pointer to a #NVS_FxnTable.
 *
 *  # Overview #
 *
 *  The NVS module allows you to manage non-volatile memory.  Using the
 *  NVS APIs, you can read and write data from and to persistent storage.
 *
 *  Each NVS object manages a region of non-volatile memory. The size is
 *  specified in the device specific driver's hardware attributes. A sector
 *  is the smallest unit of non-volatile storage that can be erased at
 *  one time. The size of the sector, or sector size, is hardware specific and
 *  may be meaningless for some non-volatile storage hardware. For flash
 *  memory devices, the region must be aligned with the sector size. That is,
 *  the region must start on a sector boundary. Additionally, the overall size
 *  of the region must be an integer multiple of the sector size.
 *
 *  # Thread Safety #
 *
 *  All NVS APIs are globally thread safe. Consequently, only one write,
 *  erase, or read in the case of SPI flash operation is allowed to be
 *  performed at a time, even for distinct NVS regions. Threads initiating
 *  new NVS writes or erases will block until any current operation completes.
 *
 *  # Interrupt Latency During Flash Operations #
 *
 *  When writing to or erasing internal flash, interrupts must be disabled
 *  to avoid executing code in flash while the flash is being reprogrammed.
 *  This constraint is met internally by the driver. User code does not need
 *  to safeguard against this.
 *
 *  Care must be taken by the user to not perform flash write or erase
 *  operations during latency critical phases of an application. See the
 *  NVS_lock() and NVS_unlock() API descriptions for more information.
 *
 *  # Usage #
 *
 *  The NVS driver interface provides device independent APIs, data types,
 *  and macros.  The following code example opens an NVS region instance,
 *  writes a string into it, then prints the string after reading it back
 *  into a local buffer, and also prints the string from its directly
 *  addressable location in flash memory.
 *
 *  @code
 *    NVS_Handle rHandle;
 *    NVS_Attrs regionAttrs;
 *    NVS_Params nvsParams;
 *    uint_fast16_t status;
 *    char buf[32];
 *
 *    // Initialize the NVS driver
 *    NVS_init();
 *
 *    //
 *    // Open the NVS region specified by the 0 element in the NVS_config[]
 *    // array defined in Board.c.
 *    // Use default NVS_Params to open this memory region, hence NULL
 *    //
 *    rHandle = NVS_open(Board_NVSINTERNAL, NULL);
 *
 *    // confirm that the NVS region opened properly
 *    if (rHandle == NULL) {
 *        // Error opening NVS driver
 *        while (1);
 *    }
 *
 *    // fetch the generic NVS region attributes
 *    NVS_getAttrs(rHandle, &regionAttrs);
 *
 *    // erase the first sector of the NVS region
 *    status = NVS_erase(rHandle, 0, regionAttrs.sectorSize);
 *    if (status != NVS_STATUS_SUCCESS) {
 *        // Error handling code
 *    }
 *
 *    // write "Hello" to the base address of region 0, verify after write
 *    status = NVS_write(rHandle, 0, "Hello", strlen("Hello")+1, NVS_POST_VERIFY);
 *    if (status != NVS_STATUS_SUCCESS) {
 *        // Error handling code
 *    }
 *
 *    // copy "Hello" from region0 into local 'buf'
 *    status = NVS_read(rHandle, 0, buf, strlen("Hello")+1);
 *    if (status != NVS_STATUS_SUCCESS) {
 *        // Error handling code
 *    }
 *
 *    // print string from fetched NVS storage
 *    System_printf("%s\n", buf);
 *
 *    // print string using direct address reference if valid
 *    if (regionAttrs.regionBase != NVS_REGION_NOT_ADDRESSABLE) {
 *       System_printf("%s\n", regionAttrs.regionBase);
 *    }
 *
 *    // close the region
 *    NVS_close(rHandle);
 *
 *  @endcode
 *
 *  Details for the example code above are described in the following
 *  subsections.
 *
 *  ### NVS Driver Configuration #
 *
 *  In order to use the NVS APIs, the application is required
 *  to provide device-specific NVS configuration in the Board.c file.
 *  The NVS driver interface defines a configuration data structure,
 *  #NVS_Config.
 *
 *  The application must declare an array of #NVS_Config elements, named
 *  \p NVS_config[].  Each element of \p NVS_config[] is populated with
 *  pointers to a device specific NVS driver implementation's function
 *  table, driver object, and hardware attributes.  The hardware attributes
 *  define properties such as the NVS region's base address and size,
 *  Each element in \p NVS_config[] corresponds to a NVS instance, and none
 *  of the elements should have NULL pointers.
 *
 *  You will need to check the device-specific NVS driver implementation's
 *  header file for example configuration.  Please also refer to the
 *  Board.c file of any of the provided examples to see the NVS configuration.
 *
 *  ### Initializing the NVS Driver #
 *
 *  NVS_init() must be called before any other NVS APIs.  This function
 *  calls the device implementation's NVS initialization function, for each
 *  element of \p NVS_config[].
 *
 *  ### Opening the NVS Driver #
 *
 *  Opening a NVS requires four steps:
 *  1.  Optionally create and initialize a #NVS_Params structure.
 *  2.  Fill in the desired parameters.
 *  3.  Call NVS_open(), passing the index of the NVS region in the #NVS_Config
 *      structure, and the address of the #NVS_Params structure.
 *  4.  Check that the #NVS_Handle returned by NVS_open() is non-NULL,
 *      and save it.  The handle will be used to read and write to the
 *      NVS you just opened.
 *
 *  \note Each NVS index can only be opened exclusively. Calling NVS_open()
 *  multiple times with the same index will result in an error. The index can
 *  be re-used if NVS_close() is called first.
 *
 *****************************************************************************
 */

#ifndef ti_drivers_NVS__include
#define ti_drivers_NVS__include

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/**
 *  @defgroup NVS_CONTROL NVS_control command and status codes
 *  These NVS macros are reservations for NVS.h
 *  @{
 */

/*!
 *  Common NVS_control command code reservation offset.
 *  NVS driver implementations should offset command codes with NVS_CMD_RESERVED
 *  growing positively
 *
 *  Example implementation specific command codes:
 *  @code
 *  #define NVSXYZ_CMD_COMMAND0     NVS_CMD_RESERVED + 0
 *  #define NVSXYZ_CMD_COMMAND1     NVS_CMD_RESERVED + 1
 *  @endcode
 */
#define NVS_CMD_RESERVED            (32)

/*!
 *  Common NVS_control status code reservation offset.
 *  NVS driver implementations should offset status codes with
 *  NVS_STATUS_RESERVED growing negatively.
 *
 *  Example implementation specific status codes:
 *  @code
 *  #define NVSXYZ_STATUS_ERROR0    NVS_STATUS_RESERVED - 0
 *  #define NVSXYZ_STATUS_ERROR1    NVS_STATUS_RESERVED - 1
 *  #define NVSXYZ_STATUS_ERROR2    NVS_STATUS_RESERVED - 2
 *  @endcode
 */
#define NVS_STATUS_RESERVED         (-32)

/**
 *  @defgroup NVS_STATUS Status Codes
 *  NVS_STATUS_* macros are general status codes returned by NVS_control()
 *  @{
 *  @ingroup NVS_CONTROL
 */

/*!
 *  @brief   Successful status code returned by:
 *  NVS_control(), NVS_read(), NVS_write(), NVS_erase(), or
 *  NVS_lock().
 *
 *  APIs returns NVS_STATUS_SUCCESS if the API was executed
 *  successfully.
 */
#define NVS_STATUS_SUCCESS          (0)

/*!
 *  @brief   Generic error status code returned by:
 *  NVS_control(), NVS_erase(), or NVS_write(),
 *
 *  APIs return NVS_STATUS_ERROR if the API was not executed
 *  successfully.
 */
#define NVS_STATUS_ERROR            (-1)

/*!
 *  @brief   An error status code returned by NVS_control() for undefined
 *  command codes.
 *
 *  NVS_control() returns #NVS_STATUS_UNDEFINEDCMD if the control code is not
 *  recognized by the driver implementation.
 */
#define NVS_STATUS_UNDEFINEDCMD     (-2)

/*!
 *  @brief An error status code returned by NVS_lock()
 *
 *  NVS_lock() will return this value if the \p timeout has expired
 */
#define NVS_STATUS_TIMEOUT          (-3)

/*!
 *  @brief An error status code returned by NVS_read(), NVS_write(), or
 *  NVS_erase()
 *
 *  Error status code returned if the \p offset argument is invalid
 *  (e.g., when offset + bufferSize exceeds the size of the region).
 */
#define NVS_STATUS_INV_OFFSET       (-4)

/*!
 *  @brief An error status code
 *
 *  Error status code returned by NVS_erase() if the \p offset argument is
 *  not aligned on a flash sector address.
 */
#define NVS_STATUS_INV_ALIGNMENT    (-5)

/*!
 *  @brief An error status code returned by NVS_erase() and NVS_write()
 *
 *  Error status code returned by NVS_erase() if the \p size argument is
 *  not a multiple of the flash sector size, or if \p offset + \p size
 *  extends past the end of the region.
 */
#define NVS_STATUS_INV_SIZE         (-6)

/*!
 *  @brief An error status code returned by NVS_write()
 *
 *  NVS_write() will return this value if #NVS_WRITE_PRE_VERIFY is
 *  requested and a flash location can not be changed to the value
 *  desired.
 */
#define NVS_STATUS_INV_WRITE        (-7)

/** @}*/

/**
 *  @defgroup NVS_CMD Command Codes
 *  NVS_CMD_* macros are general command codes for NVS_control(). Not all NVS
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup NVS_CONTROL
 */

/* Add NVS_CMD_<commands> here */

/** @} end NVS commands */

/** @} end NVS_CONTROL group */


/*!
 *  @brief NVS write flags
 *
 *  The following flags can be or'd together and passed as a bit mask
 *  to NVS_write.
 *  @{
 */

/*!
 *  @brief Erase write flag.
 *
 *  If #NVS_WRITE_ERASE is set in the flags passed to NVS_write(), the
 *  affected destination flash sectors will be erased prior to the
 *  start of the write operation.
 */
#define NVS_WRITE_ERASE             (0x1)

/*!
 *  @brief Validate write flag.
 *
 *  If #NVS_WRITE_PRE_VERIFY is set in the flags passed to NVS_write(), the
 *  destination address range will be pre-tested to guarantee that the source
 *  data can be successfully written. If #NVS_WRITE_ERASE is also requested in
 *  the write flags, then the #NVS_WRITE_PRE_VERIFY modifier is ignored.
 */
#define NVS_WRITE_PRE_VERIFY        (0x2)

/*!
 *  @brief Validate write flag.
 *
 *  If #NVS_WRITE_POST_VERIFY is set in the flags passed to NVS_write(), the
 *  destination address range will be tested after the write is finished to
 *  verify that the write operation was completed successfully.
 */
#define NVS_WRITE_POST_VERIFY       (0x4)

/** @} */

/*!
 *  @brief Special NVS_lock() timeout values
 *  @{
 */

 /*!
 *  @brief    NVS_lock() Wait forever define
 */
#define NVS_LOCK_WAIT_FOREVER       (~(0U))

/*!
 *  @brief    NVS_lock() No wait define
 */
#define NVS_LOCK_NO_WAIT            (0U)

/** @} */

/*!
 *  @brief Special NVS_Attrs.regionBase value
 *  @{
 */

 /*!
 *  @brief    This region is not directly addressable (e.g.,: SPI flash region)
 *
 *  The NVS_Attrs.regionBase field returned by NVS_getAttrs() is set to this
 *  value by the NVSSPI driver to indicate that the region is not directly
 *  addressable.
 */
#define NVS_REGION_NOT_ADDRESSABLE  ((void *)(~(0U)))

/** @} */

/*!
 *  @brief    NVS Parameters
 *
 *  NVS parameters are used with the NVS_open() call. Default values for
 *  these parameters are set using NVS_Params_init().
 *
 *  @sa       NVS_Params_init()
 */
typedef struct NVS_Params {
    void *custom;    /*!< Custom argument used by driver implementation */
} NVS_Params;

/*!
 *  @brief      NVS attributes
 *
 *  The address of an NVS_Attrs structure is passed to NVS_getAttrs().
 *
 *  @sa     NVS_getAttrs()
 */
typedef struct NVS_Attrs {
    void   *regionBase;   /*!< Base address of the NVS region. If the NVS
                               region is not directly accessible by the MCU
                               (such as SPI flash), this field will be set to
                               #NVS_REGION_NOT_ADDRESSABLE. */
    size_t  regionSize;   /*!< NVS region size in bytes. */
    size_t  sectorSize;   /*!< Erase sector size in bytes. This attribute is
                               device specific. */
} NVS_Attrs;

/*!
 *  @brief      A handle that is returned from the NVS_open() call.
 */
typedef struct NVS_Config_ *NVS_Handle;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_close().
 */
typedef void (*NVS_CloseFxn) (NVS_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_control().
 */
typedef int_fast16_t (*NVS_ControlFxn) (NVS_Handle handle, uint_fast16_t cmd,
                                        uintptr_t arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_erase().
 */
typedef int_fast16_t (*NVS_EraseFxn) (NVS_Handle handle, size_t offset,
                                      size_t size);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_getAttrs().
 */
typedef void (*NVS_GetAttrsFxn) (NVS_Handle handle, NVS_Attrs *attrs);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_init().
 */
typedef void (*NVS_InitFxn) (void);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_open().
 */
typedef NVS_Handle (*NVS_OpenFxn) (uint_least8_t index, NVS_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_read().
 */
typedef int_fast16_t (*NVS_ReadFxn) (NVS_Handle handle, size_t offset,
                                     void *buffer, size_t bufferSize);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_write().
 */
typedef int_fast16_t (*NVS_WriteFxn) (NVS_Handle handle, size_t offset,
                                      void *buffer, size_t bufferSize,
                                      uint_fast16_t flags);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_lock().
 */
typedef int_fast16_t (*NVS_LockFxn) (NVS_Handle handle, uint32_t timeout);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              NVS_unlock().
 */
typedef void (*NVS_UnlockFxn) (NVS_Handle handle);

/*!
 *  @brief      The definition of an NVS function table that contains the
 *              required set of functions to control a specific NVS driver
 *              implementation.
 */
typedef struct NVS_FxnTable {
    /*! Function to close the specified NVS region */
    NVS_CloseFxn        closeFxn;

    /*! Function to apply control command to the specified NVS region */
    NVS_ControlFxn      controlFxn;

    /*! Function to erase a portion of the specified NVS region */
    NVS_EraseFxn        eraseFxn;

    /*! Function to get the NVS device-specific attributes */
    NVS_GetAttrsFxn     getAttrsFxn;

    /*! Function to initialize the NVS module */
    NVS_InitFxn         initFxn;

    /*! Function to lock the specified NVS flash region */
    NVS_LockFxn         lockFxn;

    /*! Function to open an NVS region */
    NVS_OpenFxn         openFxn;

    /*! Function to read from the specified NVS region */
    NVS_ReadFxn         readFxn;

    /*! Function to unlock the specified NVS flash region */
    NVS_UnlockFxn       unlockFxn;

    /*! Function to write to the specified NVS region */
    NVS_WriteFxn        writeFxn;
} NVS_FxnTable;

/*!
 *  @brief  NVS Global configuration
 *
 *  The NVS_Config structure contains a set of pointers used to characterize
 *  the NVS driver implementation.
 *
 *  This structure needs to be defined before calling NVS_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     NVS_init()
 */
typedef struct NVS_Config_ {
    /*! Pointer to a table of driver-specific implementations of NVS APIs */
    NVS_FxnTable  const *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void                *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void          const *hwAttrs;
} NVS_Config;

/*!
 *  @brief  Function to close an #NVS_Handle.
 *
 *  @param  handle      A handle returned from NVS_open()
 *
 *  @sa     NVS_open()
 */
extern void NVS_close(NVS_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          #NVS_Handle.
 *
 *  @pre    NVS_open() must be called first.
 *
 *  @param  handle      An #NVS_Handle returned from NVS_open()
 *
 *  @param  cmd         A command value defined by the driver specific
 *                      implementation
 *
 *  @param  arg         An optional read or write argument that is
 *                      accompanied with \p cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     NVS_open()
 */
extern int_fast16_t NVS_control(NVS_Handle handle, uint_fast16_t cmd, uintptr_t arg);

/*!
 *  @brief  Erase \p size bytes of the region beginning at \p offset bytes
 *  from the base of the region referenced by the #NVS_Handle.
 *
 *  @warning Erasing internal flash on most devices can introduce
 *  significant interrupt latencies while the erase operation is in
 *  in progress. The user may want to surround certain real-time
 *  critical code sections with NVS_lock() and NVS_unlock() calls in order
 *  to prevent uncoordinated flash erase operations from negatively
 *  impacting performance.
 *
 *  @param   handle     A handle returned from NVS_open()
 *
 *  @param   offset     The byte offset into the NVS region to start
 *                      erasing from (must be erase sector aligned)
 *
 *  @param   size       The number of bytes to erase (must be integer
 *                      multiple of sector size)
 *
 *  @retval  #NVS_STATUS_SUCCESS         Success.
 *  @retval  #NVS_STATUS_INV_ALIGNMENT   If \p offset is not aligned on
 *                                       a sector boundary
 *  @retval  #NVS_STATUS_INV_OFFSET      If \p offset exceeds region size
 *  @retval  #NVS_STATUS_INV_SIZE        If \p size or \p offset + \p size
 *                                       exceeds region size, or if \p size
 *                                       is not an integer multiple of
 *                                       the flash sector size.
 *  @retval  #NVS_STATUS_ERROR           If an internal error occurred
 *                                       erasing the flash.
 */
extern int_fast16_t NVS_erase(NVS_Handle handle, size_t offset, size_t size);

/*!
 *  @brief  Function to get the NVS attributes
 *
 *  This function will populate a #NVS_Attrs structure with attributes
 *  specific to the memory region associated with the #NVS_Handle.
 *
 *  @param  handle      A handle returned from NVS_open()
 *
 *  @param  attrs       Location to store attributes.
 */
extern void NVS_getAttrs(NVS_Handle handle, NVS_Attrs *attrs);

/*!
 *  @brief  Function to initialize the NVS module
 *
 *  @pre    The NVS_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other NVS APIs.
 */
extern void NVS_init(void);

/*!
 *  @brief  Function to lock the NVS driver
 *
 *  This function is provided in the event that the user needs to
 *  perform some flash related operation not provided by the NVS
 *  driver API set or if the user simply needs to block flash operations
 *  for a period of time.
 *
 *  For example, the interrupt latency introduced
 *  by an uncoordinated flash write operation could interfere with some
 *  critical operation being performed by the application.
 *
 *  NVS_lock() prevents any other thread from initiating
 *  read, write, or erase operations while the user is performing an
 *  operation which is incompatible with those functions.
 *
 *  When the application no longer needs to block flash operations by
 *  other threads, NVS_unlock() must be called to allow NVS write or erase
 *  APIs to complete.
 *
 *  @param  handle      A handle returned from NVS_open()
 *
 *  @param  timeout     Timeout (in milliseconds) to wait,
 *                      or #NVS_LOCK_WAIT_FOREVER, #NVS_LOCK_NO_WAIT
 *
 *  @retval  #NVS_STATUS_SUCCESS         Success.
 *  @retval  #NVS_STATUS_TIMEOUT         If \p timeout has expired.
 */
extern int_fast16_t NVS_lock(NVS_Handle handle, uint32_t timeout);

/*!
 *  @brief  Open an NVS region for reading and writing.
 *
 *  @pre    NVS_init() was called.
 *
 *  @param  index         Index in the #NVS_Config table of the region
 *                        to manage.
 *
 *  @param  params        Pointer to a parameter region.  If NULL, default
 *                        parameter values will be used.
 *
 *  @return  A non-zero handle on success, else NULL.
 */
extern NVS_Handle NVS_open(uint_least8_t index, NVS_Params *params);

/*!
 *  @brief  Function to initialize the NVS_Params struct to its defaults
 *
 *  @param  params      A pointer to NVS_Params structure for
 *                      initialization.
 */
extern void NVS_Params_init(NVS_Params *params);

/*!
 *  @brief   Read data from the NVS region associated with the #NVS_Handle.
 *
 *  @param   handle     A handle returned from NVS_open()
 *
 *  @param   offset     The byte offset into the NVS region to start
 *                      reading from.
 *
 *  @param   buffer     A buffer to copy the data to.
 *
 *  @param   bufferSize The size of the buffer (number of bytes to read).
 *
 *  @retval  #NVS_STATUS_SUCCESS     Success.
 *  @retval  #NVS_STATUS_INV_OFFSET  If \p offset + \p size exceed the size
 *                                   of the region.
 */
extern int_fast16_t NVS_read(NVS_Handle handle, size_t offset, void *buffer,
                    size_t bufferSize);

/*!
 *  @brief  Function to unlock the NVS driver
 *
 *  This function allows NVS write and erase operations to proceed after being
 *  temporarily inhibited by a call to NVS_lock().
 *
 *  @param  handle      A handle returned from NVS_open()
 */
extern void NVS_unlock(NVS_Handle handle);

/*!
 *  @brief   Write data to the NVS region associated with the #NVS_Handle.
 *
 *  @warning Writing to internal flash on most devices can introduce
 *  significant interrupt latencies while the write operation is in
 *  in progress. The user may want to surround certain real-time
 *  critical code sections with NVS_lock() and NVS_unlock() calls in order
 *  to prevent uncoordinated flash write operations from negatively
 *  impacting performance.
 *
 *  @param   handle     A handle returned from NVS_open()
 *
 *  @param   offset     The byte offset into the NVS region to start
 *                      writing.
 *
 *  @param   buffer     A buffer containing data to write to
 *                      the NVS region.
 *
 *  @param   bufferSize The size of the buffer (number of bytes to write).
 *
 *  @param   flags      Write flags (#NVS_WRITE_ERASE, #NVS_WRITE_PRE_VERIFY,
 *                      #NVS_WRITE_POST_VERIFY).
 *
 *  @retval  #NVS_STATUS_SUCCESS        Success.
 *  @retval  #NVS_STATUS_ERROR          If the internal flash write operation
 *                                      failed, or if #NVS_WRITE_POST_VERIFY
 *                                      was requested and the destination flash
 *                                      range does not match the source
 *                                      \p buffer data.
 *  @retval  #NVS_STATUS_INV_OFFSET     If \p offset + \p size exceed the size
 *                                      of the region.
 *  @retval  #NVS_STATUS_INV_WRITE      If #NVS_WRITE_PRE_VERIFY is requested
 *                                      and the destination flash address range
 *                                      cannot be change to the values desired.
 *  @retval  #NVS_STATUS_INV_ALIGNMENT  If #NVS_WRITE_ERASE is requested
 *                                      and \p offset is not aligned on
 *                                      a sector boundary
 *
 *  @remark  This call may lock a region to ensure atomic access to the region.
 */
extern int_fast16_t NVS_write(NVS_Handle handle, size_t offset, void *buffer,
                     size_t bufferSize, uint_fast16_t flags);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

/*@}*/
#endif /* ti_drivers_NVS__include */
