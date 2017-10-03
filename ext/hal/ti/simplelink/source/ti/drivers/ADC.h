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
 *  @file       ADC.h
 *
 *  @brief      ADC driver interface
 *
 *  The ADC header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/ADC.h>
 *  @endcode
 *
 *  # Operation #
 *  The ADC driver operates as a simplified ADC module with only single channel
 *  sampling support. It also operates on blocking only mode which means users
 *  have to wait the current sampling finished before starting another sampling.
 *  The sampling channel needs to be specified in the ADC_open() before calling
 *  ADC_convert().
 *
 *  The APIs in this driver serve as an interface to a typical TI-RTOS
 *  application. The specific peripheral implementations are responsible to
 *  create all the SYS/BIOS specific primitives to allow for thread-safe
 *  operation.
 *  User can use the ADC driver or the ADCBuf driver that has more features.
 *  But both ADC and ADCBuf cannot be used together in an application.
 *
 *  ## Opening the driver #
 *
 *  @code
 *  ADC_Handle adc;
 *  ADC_Params params;
 *
 *  ADC_Params_init(&params);
 *  adc = ADC_open(Board_ADCCHANNEL_A0, &params);
 *  if (adc == NULL) {
 *      // ADC_open() failed
 *      while (1);
 *  }
 *  @endcode
 *
 *  ## Converting #
 *  An ADC conversion with a ADC peripheral is started by calling ADC_convert().
 *  The result value is returned by ADC_convert() once the conversion is
 *  finished.
 *
 *  @code
 *  int_fast16_t res;
 *  uint_fast16_t adcValue;
 *
 *  res = ADC_convert(adc, &adcValue);
 *  if (res == ADC_STATUS_SUCCESS) {
 *      //use adcValue
 *  }
 *  @endcode
 *
 *  # Implementation #
 *
 *  This module serves as the main interface for TI-RTOS
 *  applications. Its purpose is to redirect the module's APIs to specific
 *  peripheral implementations which are specified using a pointer to a
 *  ADC_FxnTable.
 *
 *  The ADC driver interface module is joined (at link time) to a
 *  NULL-terminated array of ADC_Config data structures named *ADC_config*.
 *  *ADC_config* is implemented in the application with each entry being an
 *  instance of a ADC peripheral. Each entry in *ADC_config* contains a:
 *  - (ADC_FxnTable *) to a set of functions that implement a ADC peripheral
 *  - (void *) data object that is associated with the ADC_FxnTable
 *  - (void *) hardware attributes that are associated to the ADC_FxnTable
 *
 *  # Instrumentation #
 *  The ADC driver interface produces log statements if instrumentation is
 *  enabled.
 *
 *  Diagnostics Mask | Log details |
 *  ---------------- | ----------- |
 *  Diags_USER1      | basic operations performed |
 *  Diags_USER2      | detailed operations performed |
 *
 *  ============================================================================
 */

#ifndef ti_drivers_ADC__include
#define ti_drivers_ADC__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 *  @brief Define to support deprecated API ADC_convertRawToMicroVolts.
 *
 *  It is succeeded by the generic ADC_convertToMicroVolts.
 */
#define ADC_convertRawToMicroVolts ADC_convertToMicroVolts

/**
 *  @defgroup ADC_CONTROL ADC_control command and status codes
 *  These ADC macros are reservations for ADC.h
 *  @{
 */

/*!
 * Common ADC_control command code reservation offset.
 * ADC driver implementations should offset command codes with ADC_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define ADCXYZ_CMD_COMMAND0      ADC_CMD_RESERVED + 0
 * #define ADCXYZ_CMD_COMMAND1      ADC_CMD_RESERVED + 1
 * @endcode
 */
#define ADC_CMD_RESERVED           (32)

/*!
 * Common ADC_control status code reservation offset.
 * ADC driver implementations should offset status codes with
 * ADC_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define ADCXYZ_STATUS_ERROR0     ADC_STATUS_RESERVED - 0
 * #define ADCXYZ_STATUS_ERROR1     ADC_STATUS_RESERVED - 1
 * #define ADCXYZ_STATUS_ERROR2     ADC_STATUS_RESERVED - 2
 * @endcode
 */
#define ADC_STATUS_RESERVED        (-32)

/*!
 * @brief   Successful status code returned by ADC_control().
 *
 * ADC_control() returns ADC_STATUS_SUCCESS if the control code was executed
 * successfully.
 *  @{
 *  @ingroup ADC_CONTROL
 */
#define ADC_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code returned by ADC_control().
 *
 * ADC_control() returns ADC_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define ADC_STATUS_ERROR           (-1)

/*!
 * @brief   An error status code returned by ADC_control() for undefined
 * command codes.
 *
 * ADC_control() returns ADC_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define ADC_STATUS_UNDEFINEDCMD    (-2)
/** @}*/

/**
 *  @defgroup ADC_CMD Command Codes
 *  ADC_CMD_* macros are general command codes for ADC_control(). Not all ADC
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup ADC_CONTROL
 */

/* Add ADC_CMD_<commands> here */

/** @}*/

/** @}*/

/*!
 *  @brief      A handle that is returned from a ADC_open() call.
 */
typedef struct ADC_Config_    *ADC_Handle;

/*!
 *  @brief  ADC Parameters
 *
 *  ADC parameters are used to with the ADC_open() call. Only custom argument
 *  is supported in the parameters. Default values for these parameters are
 *  set using ADC_Params_init().
 *
 *  @sa     ADC_Params_init()
 */
typedef struct ADC_Params_ {
    void    *custom;        /*!< Custom argument used by driver
                                implementation */
    bool    isProtected;    /*!< By default ADC uses a semaphore
                                to guarantee thread safety. Setting
                                this parameter to 'false' will eliminate
                                the usage of a semaphore for thread
                                safety. The user is then responsible
                                for ensuring that parallel invocations
                                of ADC_convert() are thread safe. */
} ADC_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_close().
 */
typedef void (*ADC_CloseFxn) (ADC_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_control().
 */
typedef int_fast16_t (*ADC_ControlFxn) (ADC_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_ConvertFxn().
 */
typedef int_fast16_t (*ADC_ConvertFxn) (ADC_Handle handle, uint16_t *value);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_convertToMicroVolts().
 */
typedef uint32_t (*ADC_ConvertToMicroVoltsFxn) (ADC_Handle handle,
    uint16_t adcValue);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_init().
 */
typedef void (*ADC_InitFxn) (ADC_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              ADC_open().
 */
typedef ADC_Handle (*ADC_OpenFxn) (ADC_Handle handle, ADC_Params *params);

/*!
 *  @brief      The definition of a ADC function table that contains the
 *              required set of functions to control a specific ADC driver
 *              implementation.
 */
typedef struct ADC_FxnTable_ {
    /*! Function to close the specified peripheral */
    ADC_CloseFxn      closeFxn;

    /*! Function to perform implementation specific features */
    ADC_ControlFxn    controlFxn;

    /*! Function to initiate a ADC single channel conversion */
    ADC_ConvertFxn    convertFxn;

    /*! Function to convert ADC result to microvolts */
    ADC_ConvertToMicroVoltsFxn convertToMicroVolts;

    /*! Function to initialize the given data object */
    ADC_InitFxn       initFxn;

    /*! Function to open the specified peripheral */
    ADC_OpenFxn       openFxn;
} ADC_FxnTable;

/*!
 *  @brief ADC Global configuration
 *
 *  The ADC_Config structure contains a set of pointers used to characterize
 *  the ADC driver implementation.
 *
 *  This structure needs to be defined before calling ADC_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     ADC_init()
 */
typedef struct ADC_Config_ {
    /*! Pointer to a table of driver-specific implementations of ADC APIs */
    ADC_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void               *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void         const *hwAttrs;
} ADC_Config;

/*!
 *  @brief  Function to close a ADC driver
 *
 *  @pre    ADC_open() has to be called first.
 *
 *  @param  handle An ADC handle returned from ADC_open()
 *
 *  @sa     ADC_open()
 */
extern void ADC_close(ADC_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          ADC_Handle.
 *
 *  @pre    ADC_open() has to be called first.
 *
 *  @param  handle      A ADC handle returned from ADC_open()
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
 *  @sa     ADC_open()
 */
extern int_fast16_t ADC_control(ADC_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief  Function to perform ADC conversion
 *
 *  Function to perform ADC single channel single sample conversion.
 *
 *  @pre    ADC_open() has been called
 *
 *  @param  handle        An ADC_Handle
 *  @param  value         A pointer to the conversion result
 *
 *  @return The return value indicates the conversion is succeeded or
 *          failed. The value could be ADC_STATUS_SUCCESS or
 *          ADC_STATUS_ERROR.
 *
 *  @sa     ADC_open()
 *  @sa     ADC_close()
 */
extern int_fast16_t ADC_convert(ADC_Handle handle, uint16_t *value);

/*!
 *  @brief  Function performs conversion from ADC result to actual value in
 *          microvolts.
 *
 *  @pre    ADC_open() and ADC_convert() has to be called first.
 *
 *  @param  handle      A ADC handle returned from ADC_open()
 *
 *  @param  adcValue A sampling result return from ADC_convert()
 *
 *  @return The actual sampling result in micro volts unit.
 *
 *  @sa     ADC_open()
 */
extern uint32_t ADC_convertToMicroVolts(ADC_Handle handle,
    uint16_t adcValue);

/*!
 *  @brief  Function to initializes the ADC driver
 *
 *  @pre    The ADC_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other ADC driver APIs.
 */
extern void ADC_init(void);

/*!
 *  @brief  Function to initialize the ADC peripheral
 *
 *  Function to initialize the ADC peripheral specified by the
 *  particular index value.
 *
 *  @pre    ADC_init() has been called
 *
 *  @param  index         Logical peripheral number for the ADC indexed into
 *                        the ADC_config table
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A ADC_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     ADC_init()
 *  @sa     ADC_close()
 */
extern ADC_Handle ADC_open(uint_least8_t index, ADC_Params *params);

/*!
 *  @brief  Function to initialize the ADC_Params struct to its defaults
 *
 *  @param  params      An pointer to ADC_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      custom = NULL
 */
extern void ADC_Params_init(ADC_Params *params);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_ADC__include */
