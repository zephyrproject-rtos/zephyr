/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
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
 *  @file       ADCBuf.h
 *
 *  @brief      ADCBuf driver interface
 *
 *  The ADCBuf header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/ADCBuf.h>
 *  @endcode
 *
 *  # Operation #
 *  The ADCBuf driver in TI-RTOS samples an analogue waveform at a specified
 *  frequency. The resulting samples are transferred to a buffer provided by
 *  the application. The driver can either take n samples once, or continuously
 *  sample by double-buffering and providing a callback to process each finished
 *  buffer.
 *
 *  The APIs in this driver serve as an interface to a typical TI-RTOS
 *  application. The specific peripheral implementations are responsible to
 *  create all the SYS/BIOS specific primitives to allow for thread-safe
 *  operation.

 *  User can use the ADC driver or the ADCBuf driver. But both ADC and ADCBuf
 *  cannot be used together in an application.
 *
 *  ## Opening the driver #
 *
 *  @code
 *      ADCBuf_Handle adcBufHandle;
 *      ADCBuf_Params adcBufParams;
 *
 *      ADCBuf_Params_init(&adcBufParams);
 *      adcBufHandle = ADCBuf_open(Board_ADCBuf0, &adcBufParams);
 *  @endcode
 *
 *  ## Making a conversion #
 *  In this context, a conversion refers to taking multiple ADC samples and
 *  transferring them to an application-provided buffer.
 *  To start a conversion, the application must configure an ADCBuf_Conversion struct
 *  and call ADCBuf_convert(). In blocking mode, ADCBuf_convert() will return
 *  when the conversion is finished and the desired number of samples have been made.
 *  In callback mode, ADCBuf_convert() will return immediately and the application will
 *  get a callback when the conversion is done.
 *
 *  @code
 *      ADCBuf_Conversion blockingConversion;
 *
 *      blockingConversion.arg = NULL;
 *      blockingConversion.adcChannel = Board_ADCCHANNEL_A1;
 *      blockingConversion.sampleBuffer = sampleBufferOnePtr;
 *      blockingConversion.sampleBufferTwo = NULL;
 *      blockingConversion.samplesRequestedCount = ADCBUFFERSIZE;
 *
 *      if (!ADCBuf_convert(adcBuf, &continuousConversion, 1)) {
 *          // handle error
 *      }
 *  @endcode
 *
 *  ## Canceling a conversion #
 *  ADCBuf_convertCancel() is used to cancel an ADCBuf conversion when the driver is
 *  used in ::ADCBuf_RETURN_MODE_CALLBACK.
 *
 *  Calling this API while no conversion is in progress has no effect. If a
 *  conversion is in progress, it is canceled and the provided callback function
 *  is called.
 *
 *  In ::ADCBuf_RECURRENCE_MODE_CONTINUOUS, this function must be called to stop the
 *  conversion. The driver will continue providing callbacks with fresh samples
 *  until thie ADCBuf_convertCancel() function is called. The callback function is not
 *  called after ADCBuf_convertCancel() while in ::ADCBuf_RECURRENCE_MODE_CONTINUOUS.
 *
 *  # Implementation #
 *
 *  This module serves as the main interface for TI-RTOS applications. Its
 *  purpose is to redirect the module's APIs to specific peripheral
 *  implementations which are specified using a pointer to an ADCBuf_FxnTable.
 *
 *  The ADCBuf driver interface module is joined (at link time) to a
 *  NULL-terminated array of ADCBuf_Config data structures named *ADCBuf_config*.
 *  *ADCBuf_config* is implemented in the application with each entry being an
 *  instance of an ADCBuf peripheral. Each entry in *ADCBuf_config* contains a:
 *  - (ADCBuf_FxnTable *) to a set of functions that implement an ADCBuf peripheral
 *  - (void *) data object that is associated with the ADCBuf_FxnTable
 *  - (void *) hardware attributes that are associated to the ADCBuf_FxnTable
 *
 *  # Instrumentation #
 *
 *  The ADCBuf driver interface produces log statements if instrumentation is
 *  enabled.
 *
 *  Diagnostics Mask | Log details                      |
 *  ---------------- | ---------------------------------|
 *  Diags_USER1      | basic operations performed       |
 *  Diags_USER2      | detailed operations performed    |
 *
 *  ============================================================================
 */

#ifndef ti_drivers_adcbuf__include
#define ti_drivers_adcbuf__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 *  @defgroup ADCBUF_CONTROL ADCBuf_control command and status codes
 *  These ADCBuf macros are reservations for ADCBuf.h
 *  @{
 */

/*!
 * Common ADCBuf_control command code reservation offset.
 * ADC driver implementations should offset command codes with ADCBuf_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define ADCXYZ_COMMAND0         ADCBuf_CMD_RESERVED + 0
 * #define ADCXYZ_COMMAND1         ADCBuf_CMD_RESERVED + 1
 * @endcode
 */
#define ADCBuf_CMD_RESERVED             (32)

/*!
 * Common ADCBuf_control status code reservation offset.
 * ADC driver implementations should offset status codes with
 * ADCBuf_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define ADCXYZ_STATUS_ERROR0    ADCBuf_STATUS_RESERVED - 0
 * #define ADCXYZ_STATUS_ERROR1    ADCBuf_STATUS_RESERVED - 1
 * #define ADCXYZ_STATUS_ERROR2    ADCBuf_STATUS_RESERVED - 2
 * @endcode
 */
#define ADCBuf_STATUS_RESERVED          (-32)

/*!
 * \brief  Success status code returned by:
 * ADCBuf_control()
 *
 * Functions return ADCBuf_STATUS_SUCCESS if the call was executed
 * successfully.
 *  @{
 *  @ingroup ADCBUF_CONTROL
 */
#define ADCBuf_STATUS_SUCCESS           (0)

/*!
 * \brief   Generic error status code returned by ADCBuf_control().
 *
 * ADCBuf_control() returns ADCBuf_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define ADCBuf_STATUS_ERROR             (-1)

/*!
 * \brief   An error status code returned by ADCBuf_control() for undefined
 * command codes.
 *
 * ADCBuf_control() returns ADCBuf_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define ADCBuf_STATUS_UNDEFINEDCMD      (-2)

/*!
 * \brief   An error status code returned by ADCBuf_adjustRawValues() if the
 * function is not supported by a particular driver implementation.
 *
 * ADCBuf_adjustRawValues() returns ADCBuf_STATUS_UNSUPPORTED if the function is
 * not supported by the driver implementation.
 */
#define ADCBuf_STATUS_UNSUPPORTED      (-3)
/** @}*/

/**
 *  @defgroup ADCBUF_CMD Command Codes
 *  ADCBUF_CMD_* macros are general command codes for I2C_control(). Not all ADCBuf
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup ADCBUF_CONTROL
 */

/* Add ADCBUF_CMD_<commands> here */

/** @}*/

/** @}*/


/*!
 *  @brief      A handle that is returned from an ADCBuf_open() call.
 */
typedef struct ADCBuf_Config_              *ADCBuf_Handle;

/*!
 *  @brief
 *  An ::ADCBuf_Conversion data structure is used with ADCBuf_convert(). It indicates
 *  which channel to perform the ADC conversion on, how many conversions to make, and where to put them.
 *  The arg variable is an user-definable argument which gets passed to the
 *  ::ADCBuf_Callback when the ADC driver is in ::ADCBuf_RETURN_MODE_CALLBACK.
 */
typedef struct ADCBuf_Conversion_ {
    uint16_t            samplesRequestedCount;      /*!< Number of samples to convert and return */
    void                *sampleBuffer;              /*!< Buffer the results of the conversions are stored in */
    void                *sampleBufferTwo;           /*!< A second buffer that is filled in ::ADCBuf_RECURRENCE_MODE_CONTINUOUS mode while
                                                        the first buffer is processed by the application. The value is not used in
                                                        ::ADCBuf_RECURRENCE_MODE_ONE_SHOT mode. */
    void                *arg;                       /*!< Argument to be passed to the callback function in ::ADCBuf_RETURN_MODE_CALLBACK */
    uint32_t            adcChannel;                 /*!< Channel to perform the ADC conversion on. Mapping of channel to pin or internal signal is device specific. */
} ADCBuf_Conversion;

/*!
 *  @brief      The definition of a callback function used by the ADC driver
 *              when used in ::ADCBuf_RETURN_MODE_CALLBACK. It is called in a HWI or SWI context depending on the device specific implementation.
 */
typedef void (*ADCBuf_Callback)            (ADCBuf_Handle handle,
                                            ADCBuf_Conversion *conversion,
                                            void *completedADCBuffer,
                                            uint32_t completedChannel);
/*!
 *  @brief     ADC trigger mode settings
 *
 *  This enum defines if the driver should make n conversions and return
 *  or run indefinitely and run a callback function every n conversions.
 */
typedef enum ADCBuf_Recurrence_Mode_ {
    /*!
     *  The driver makes n measurements and returns or runs a callback function depending
     *  on the ::ADCBuf_Return_Mode setting.
     */
    ADCBuf_RECURRENCE_MODE_ONE_SHOT,
    /*!
     *  The driver makes n measurements and then runs a callback function. This process happens
     *  until the application calls ::ADCBuf_ConvertCancelFxn(). This setting can only be used in
     *  ::ADCBuf_RETURN_MODE_CALLBACK.
     */
    ADCBuf_RECURRENCE_MODE_CONTINUOUS
} ADCBuf_Recurrence_Mode;

/*!
 *  @brief      ADC return mode settings
 *
 *  This enum defines how the ADCBuf_convert() function returns.
 *  It either blocks or returns immediately and calls a callback function when the provided buffer has been filled.
 */
typedef enum ADCBuf_Return_Mode_ {
    /*!
      *  Uses a semaphore to block while ADC conversions are performed.  Context of the call
      *  must be a Task.
      *
      *  @note   Blocking return mode cannot be used in combination with ::ADCBuf_RECURRENCE_MODE_CONTINUOUS
      */
    ADCBuf_RETURN_MODE_BLOCKING,

    /*!
     *  Non-blocking and will return immediately.  When the conversion
     *  is finished the configured callback function is called.
     */
    ADCBuf_RETURN_MODE_CALLBACK
} ADCBuf_Return_Mode;


/*!
 *  @brief ADC Parameters
 *
 *  ADC Parameters are used to with the ADCBuf_open() call. Default values for
 *  these parameters are set using ADCBuf_Params_init().
 *
 *  @sa     ADCBuf_Params_init()
 */
typedef struct ADCBuf_Params_ {
    uint32_t                    blockingTimeout;    /*!< Timeout for semaphore in ::ADCBuf_RETURN_MODE_BLOCKING */
    uint32_t                    samplingFrequency;  /*!< The frequency at which the ADC will produce a sample */
    ADCBuf_Return_Mode          returnMode;         /*!< Return mode for all conversions */
    ADCBuf_Callback             callbackFxn;        /*!< Pointer to callback function */
    ADCBuf_Recurrence_Mode      recurrenceMode;     /*!< One-shot or continuous conversion  */
    void                        *custom;            /*!< Pointer to a device specific extension of the ADCBuf_Params */
} ADCBuf_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_close().
 */
typedef void (*ADCBuf_CloseFxn)                  (ADCBuf_Handle handle);


/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_open().
 */
typedef ADCBuf_Handle (*ADCBuf_OpenFxn)          (ADCBuf_Handle handle,
                                                  const ADCBuf_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_control().
 */
typedef int_fast16_t (*ADCBuf_ControlFxn)        (ADCBuf_Handle handle,
                                                  uint_fast8_t cmd,
                                                  void *arg);
/*
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_init().
 */
typedef void (*ADCBuf_InitFxn)                   (ADCBuf_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_convert().
 */
typedef int_fast16_t (*ADCBuf_ConvertFxn)     (ADCBuf_Handle handle,
                                               ADCBuf_Conversion conversions[],
                                               uint_fast8_t channelCount);
/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_convertCancel().
 */
typedef int_fast16_t (*ADCBuf_ConvertCancelFxn)(ADCBuf_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_GetResolution();
 */
typedef uint_fast8_t (*ADCBuf_GetResolutionFxn)     (ADCBuf_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_adjustRawValues();
 */
typedef int_fast16_t (*ADCBuf_adjustRawValuesFxn)(ADCBuf_Handle handle,
                                                void *sampleBuffer,
                                                uint_fast16_t sampleCount,
                                                uint32_t adcChannel);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADCBuf_convertAdjustedToMicroVolts();
 */
typedef int_fast16_t (*ADCBuf_convertAdjustedToMicroVoltsFxn)  (ADCBuf_Handle handle,
                                                            uint32_t  adcChannel,
                                                            void *adjustedSampleBuffer,
                                                            uint32_t outputMicroVoltBuffer[],
                                                            uint_fast16_t sampleCount);

/*!
 *  @brief      The definition of an ADCBuf function table that contains the
 *              required set of functions to control a specific ADC driver
 *              implementation.
 */
typedef struct ADCBuf_FxnTable_ {
    /*! Function to close the specified peripheral */
    ADCBuf_CloseFxn            closeFxn;
    /*! Function to driver implementation specific control function */
    ADCBuf_ControlFxn          controlFxn;
    /*! Function to initialize the given data object */
    ADCBuf_InitFxn             initFxn;
    /*! Function to open the specified peripheral */
    ADCBuf_OpenFxn             openFxn;
    /*! Function to start an ADC conversion with the specified peripheral */
    ADCBuf_ConvertFxn          convertFxn;
    /*! Function to abort a conversion being carried out by the specified peripheral */
    ADCBuf_ConvertCancelFxn    convertCancelFxn;
    /*! Function to get the resolution in bits of the ADC */
    ADCBuf_GetResolutionFxn    getResolutionFxn;
    /*! Function to adjust raw ADC return bit values to values comparable between devices of the same type */
    ADCBuf_adjustRawValuesFxn   adjustRawValuesFxn;
    /*! Function to convert adjusted ADC values to microvolts */
    ADCBuf_convertAdjustedToMicroVoltsFxn   convertAdjustedToMicroVoltsFxn;
} ADCBuf_FxnTable;

/*!
 *  @brief ADCBuf Global configuration
 *
 *  The ADCBuf_Config structure contains a set of pointers used to characterise
 *  the ADC driver implementation.
 *
 *  This structure needs to be defined before calling ADCBuf_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     ADCBuf_init()
 */
typedef struct ADCBuf_Config_ {
    /*! Pointer to a table of driver-specific implementations of ADC APIs */
    const ADCBuf_FxnTable   *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void                    *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void const              *hwAttrs;
} ADCBuf_Config;

/*!
 *  @brief  Function to close an ADC peripheral specified by the ADC handle
 *
 *  @pre    ADCBuf_open() has to be called first.
 *
 *  @pre    In ADCBuf_RECURRENCE_MODE_CONTINUOUS, the application must call ADCBuf_convertCancel() first.
 *
 *  @param  handle      An ADCBuf handle returned from ADCBuf_open()
 *
 *  @sa     ADCBuf_open()
 */
extern void ADCBuf_close(ADCBuf_Handle handle);


/*!
 *  @brief  Function performs implementation specific features on a given
 *          ADCBuf_Handle.
 *
 *  @pre    ADCBuf_open() has to be called first.
 *
 *  @param  handle      An ADCBuf handle returned from ADCBuf_open()
 *
 *  @param  cmd         A command value defined by the driver specific
 *                      implementation
 *
 *  @param  cmdArg      A pointer to an optional R/W (read/write) argument that
 *                      is accompanied with cmd
 *
 *  @return An ADCBuf_Status describing an error or success state. Negative values
 *          indicates an error.
 *
 *  @sa     ADCBuf_open()
 */
extern int_fast16_t ADCBuf_control(ADCBuf_Handle handle, uint_fast16_t cmd, void *cmdArg);

/*!
 *  @brief  This function initializes the ADC module. This function must
 *
 *  @pre    The ADCBuf_Config structure must exist and be persistent before this
 *          function can be called.
 *          This function call does not modify any peripheral registers.
 *          Function should only be called once.
 */
extern void ADCBuf_init(void);

/*!
 *  @brief  This function sets all fields of a specified ADCBuf_Params structure to their
 *          default values.
 *
 *  @param  params      A pointer to ADCBuf_Params structure for initialization
 *
 *  Default values are:
 *                      returnMode         = ADCBuf_RETURN_MODE_BLOCKING,
 *                      blockingTimeout    = 25000,
 *                      callbackFxn        = NULL,
 *                      recurrenceMode     = ADCBuf_RECURRENCE_MODE_ONE_SHOT,
 *                      samplingFrequency  = 10000,
 *                      custom             = NULL
 *
 *  ADCBuf_Params::blockingTimeout should be set large enough to allow for the desired number of samples to be
 *  collected with the specified frequency.
 */
extern void ADCBuf_Params_init(ADCBuf_Params *params);

/*!
 *  @brief  This function opens a given ADCBuf peripheral.
 *
 *  @param  index       Logical peripheral number for the ADCBuf indexed into
 *                      the ADCBuf_config table
 *
 *  @param  params      Pointer to an parameter block, if NULL it will use
 *                      default values.
 *
 *  @return An ADCBuf_Handle on success or a NULL on an error or if it has been
 *          opened already. If NULL is returned further ADC API calls will
 *          result in undefined behaviour.
 *
 *  @sa     ADCBuf_close()
 */
extern ADCBuf_Handle ADCBuf_open(uint_least8_t index, ADCBuf_Params *params);

/*!
 *  @brief  This function starts a set of conversions on one or more channels.
 *
 *  @param  handle          An ADCBuf handle returned from ADCBuf_open()
 *
 *  @param  conversions     A pointer to an array of ADCBuf_Conversion structs with the specific parameters
 *                          for each channel. Only use one ADCBuf_Conversion struct per channel.
 *
 *  @param  channelCount    The number of channels to convert on in this call. Should be the length of the conversions array.
 *                          Depending on the device, multiple simultaneous conversions may not be supported. See device
 *                          specific implementation.
 *
 *  @return ADCBuf_STATUS_SUCCESS if the operation was successful. ADCBuf_STATUS_ERROR or a device specific status is returned otherwise.
 *
 *  @pre    ADCBuf_open() must have been called prior.
 *
 *  @sa     ADCBuf_convertCancel()
 */
extern int_fast16_t ADCBuf_convert(ADCBuf_Handle handle, ADCBuf_Conversion conversions[],  uint_fast8_t channelCount);

/*!
 *  @brief  This function cancels an ADC conversion that is in progress.
 *
 *  This function must be called before calling ADCBuf_close().
 *
 *  @param  handle      An ADCBuf handle returned from ADCBuf_open()
 *
 *  @return ADCBuf_STATUS_SUCCESS if the operation was successful. ADCBuf_STATUS_ERROR or a device specific status is returned otherwise.
 *
 *  @sa     ADCBuf_convert()
 */
extern int_fast16_t ADCBuf_convertCancel(ADCBuf_Handle handle);

/*!
 *  @brief  This function returns the resolution in bits of the specified ADC.
 *
 *  @param  handle      An ADCBuf handle returned from ADCBuf_open().
 *
 *  @return The resolution in bits of the specified ADC.
 *
 *  @pre    ADCBuf_open() must have been called prior.
 */
extern uint_fast8_t ADCBuf_getResolution(ADCBuf_Handle handle);

 /*!
 *  @brief  This function adjusts a raw ADC output buffer such that the result is comparable between devices of the same make.
 *          The function does the adjustment in-place.
 *
 *  @param  handle          An ADCBuf handle returned from ADCBuf_open().
 *
 *  @param  sampleBuf       A buffer full of raw sample values.
 *
 *  @param  sampleCount     The number of samples to adjust.
 *
 *  @param  adcChan         The channel the buffer was sampled on.
 *
 *  @return A buffer full of adjusted samples contained in sampleBuffer.
 *
 *  @return ADCBuf_STATUS_SUCCESS if the operation was successful. ADCBuf_STATUS_ERROR or a device specific status is returned otherwise.
 *
 *  @pre    ADCBuf_open() must have been called prior.
 */
extern int_fast16_t ADCBuf_adjustRawValues(ADCBuf_Handle handle, void *sampleBuf, uint_fast16_t sampleCount, uint32_t adcChan);

 /*!
 *  @brief  This function converts a raw ADC output value to a value scaled in micro volts.
 *
 *  @param  handle      An ADCBuf handle returned from ADCBuf_open()
 *
 *  @param  adcChan     The ADC channel the samples stem from. This parameter is only necessary for certain devices.
 *                      See device specific implementation for details.
 *
 *  @param  adjustedSampleBuffer    A buffer full of adjusted samples.
 *
 *  @param  outputMicroVoltBuffer   The output buffer. The conversion does not occur in place due to the differing data type sizes.
 *
 *  @param  sampleCount             The number of samples to convert.
 *
 *  @return A number of measurements scaled in micro volts inside outputMicroVoltBuffer.
 *
 *  @return ADCBuf_STATUS_SUCCESS if the operation was successful. ADCBuf_STATUS_ERROR or a device specific status is returned otherwise.
 *
 *  @pre    ADCBuf_open() must have been called prior.
 *
 *  @pre    ADCBuf_adjustRawValues() must be called on adjustedSampleBuffer prior.
 */
extern int_fast16_t ADCBuf_convertAdjustedToMicroVolts(ADCBuf_Handle handle, uint32_t  adcChan, void *adjustedSampleBuffer, uint32_t outputMicroVoltBuffer[], uint_fast16_t sampleCount);

#ifdef __cplusplus
}
#endif
#endif /* ti_drivers_adcbuf__include */
