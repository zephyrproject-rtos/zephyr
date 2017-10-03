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
/*!*****************************************************************************
 *  @file       Capture.h
 *  @brief      Capture driver interface
 *
 *  The capture header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/Capture.h>
 *  @endcode
 *
 *  # Overview #
 *  The capture driver serves as the main interface for a typical RTOS
 *  application. Its purpose is to redirect the capture APIs to device specific
 *  implementations which are specified using a pointer to a #Capture_FxnTable.
 *  The device specific implementations are responsible for creating all the
 *  RTOS specific primitives to allow for thead-safe operation. The capture
 *  driver utilizes the general purpose timer hardware.
 *
 *  The capture driver internally handles the general purpose timer resource
 *  allocation. For each capture driver instance, Capture_open() occupies the
 *  specified timer, and Capture_close() releases the occupied timer resource.
 *
 *  # Usage#
 *  The capture driver is used to detect and time edge triggered events on a
 *  GPIO pin. The following example code opens a capture instance in falling
 *  edge mode. The interval returned in the callback function is in
 *  microseconds.
 *
 *  @code
 *  Capture_Handle    handle;
 *  Capture_Params    params;
 *
 *  Capture_Params_init(&params);
 *  params.mode  = Capture_FALLING_EDGE;
 *  params.callbackFxn = someCaptureCallbackFunction;
 *  params.periodUnit = Capture_PERIOD_US;
 *
 *  handle = Capture_open(someCapture_configIndexValue, &params);
 *
 *  if (handle == NULL) {
 *      //Capture_open() failed
 *      while(1);
 *  }
 *
 *  status = Capture_start(handle);
 *
 *  if (status == Capture_STATUS_ERROR) {
 *      //Capture_start() failed
 *      while(1);
 *  }
 *
 *  sleep(10000);
 *
 *  Capture_stop(handle);
 *  @endcode

 *  ### Capture Driver Configuration #
 *
 *  In order to use the capture APIs, the application is required to provide
 *  device specific capture configuration in the Board.c file. The capture
 *  driver interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct Capture_Config_ {
 *      Capture_FxnTable const *fxnTablePtr;
 *      void                   *object;
 *      void             const *hwAttrs;
 *  } Capture_Config;
 *  @endcode
 *
 *  The application must declare an array of Capture_Config elements, named
 *  Capture_config[]. Each element of Capture_config[] is populated with
 *  pointers to a device specific capture driver implementation's function
 *  table, driver object, and hardware attributes. The hardware attributes
 *  define properties such as the timer peripheral's base address, interrupt
 *  number and interrupt priority. Each element in Capture_config[] corresponds
 *  to a capture instance, and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the peripheral designation.
 *
 *  You will need to check the device specific capture driver implementation's
 *  header file for example configuration.
 *
 *  ### Initializing the Capture Driver #
 *
 *  Capture_init() must be called before any other capture APIs. This function
 *  calls the device implementation's capture initialization function, for each
 *  element of Capture_config[].
 *
 *  ### Modes of Operation #
 *
 *  The capture driver supports four modes of operation which may be specified
 *  in the Capture_Params.
 *
 *  #Capture_RISING_EDGE will capture rising edge triggers. After
 *  Capture_start() is called, the callback function specified in
 *  Capture_Params will be called after each rising edge is detected on the
 *  GPIO pin. This behavior will continue until Capture_stop() is called.
 *
 *  #Capture_FALLING_EDGE will capture falling edge triggers. After
 *  Capture_start() is called, the callback function specified in
 *  Capture_Params will be called after each falling edge is detected on the
 *  GPIO pin. This behavior will continue until Capture_stop() is called.
 *
 *  #Capture_ANY_EDGE will capture both rising and falling edge triggers. After
 *  Capture_start() is called, the callback function specified in
 *  Capture_Params will be called after each rising or falling edge is detected
 *  on the GPIO pin. This behavior will continue until Capture_stop() is
 *  called.
 *
 *  # Implementation #
 *
 *  The capture driver interface module is joined (at link time) to an
 *  array of Capture_Config data structures named *Capture_config*.
 *  Capture_config is implemented in the application with each entry being an
 *  instance of a capture peripheral. Each entry in *Capture_config* contains a:
 *  - (Capture_FxnTable *) to a set of functions that implement a capture
 *     peripheral
 *  - (void *) data object that is associated with the Capture_FxnTable
 *  - (void *) hardware attributes that are associated with the Capture_FxnTable
 *
 *  The capture APIs are redirected to the device specific implementations
 *  using the Capture_FxnTable pointer of the Capture_config entry.
 *  In order to use device specific functions of the capture driver directly,
 *  link in the correct driver library for your device and include the
 *  device specific capture driver header file (which in turn includes
 *  Capture.h). For example, for the MSP432 family of devices, you would
 *  include the following header file:
 *
 *  @code
 *  #include <ti/drivers/capture/CaptureMSP432.h>
 *  @endcode
 *
 *******************************************************************************
 */

#ifndef ti_drivers_Capture__include
#define ti_drivers_Capture__include

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/*!
 * Common Capture_control command code reservation offset.
 * Capture driver implementations should offset command codes with
 * Capture_CMD_RESERVED growing positively.
 *
 * Example implementation specific command codes:
 * @code
 * #define CaptureXYZ_CMD_COMMAND0      Capture_CMD_RESERVED + 0
 * #define CaptureXYZ_CMD_COMMAND1      Capture_CMD_RESERVED + 1
 * @endcode
 */
#define Capture_CMD_RESERVED             (32)

/*!
 * Common Capture_control status code reservation offset.
 * Capture driver implementations should offset status codes with
 * Capture_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define CaptureXYZ_STATUS_ERROR0     Capture_STATUS_RESERVED - 0
 * #define CaptureXYZ_STATUS_ERROR1     Capture_STATUS_RESERVED - 1
 * @endcode
 */
#define Capture_STATUS_RESERVED         (-32)

/*!
 * @brief   Successful status code.
 */
#define Capture_STATUS_SUCCESS          (0)

/*!
 * @brief   Generic error status code.
 */
#define Capture_STATUS_ERROR            (-1)

/*!
 * @brief   An error status code returned by Capture_control() for undefined
 * command codes.
 *
 * Capture_control() returns Capture_STATUS_UNDEFINEDCMD if the control code is
 * not recognized by the driver implementation.
 */
#define Capture_STATUS_UNDEFINEDCMD    (-2)

/*!
 *  @brief      A handle that is returned from a Capture_open() call.
 */
typedef struct Capture_Config_ *Capture_Handle;


/*!
 *  @brief Capture mode settings
 *
 *  This enum defines the capture modes that may be specified in
 *  #Capture_Params.
 */
typedef enum Capture_Mode_ {
    Capture_RISING_EDGE,     /*!< Capture is triggered on rising edges. */
    Capture_FALLING_EDGE,    /*!< Capture is triggered on falling edges. */
    Capture_ANY_EDGE         /*!< Capture is triggered on both rising and
                                  falling edges. */
} Capture_Mode;

/*!
 *  @brief Capture period unit enum
 *
 *  This enum defines the units that may be specified for the period
 *  in #Capture_Params.
 */
typedef enum Capture_PeriodUnits_ {
    Capture_PERIOD_US,       /*!< Period specified in micro seconds. */
    Capture_PERIOD_HZ,       /*!< Period specified in hertz; interrupts per
                                  second. */
    Capture_PERIOD_COUNTS    /*!< Period specified in timer ticks. Varies
                                  by board. */
} Capture_PeriodUnits;


/*!
 *  @brief  Capture callback function
 *
 *  User definable callback function prototype. The capture driver will call
 *  the defined function and pass in the capture driver's handle and the
 *  pointer to the user-specified the argument.
 *
 *  @param  handle         Capture_Handle
 *
 *  @param  interval       Interval of two triggering edges in
 *                         #Capture_PeriodUnits
 */
typedef void (*Capture_CallBackFxn)(Capture_Handle handle, uint32_t interval);

/*!
 *  @brief Capture Parameters
 *
 *  Capture parameters are used by the Capture_open() call. Default values for
 *  these parameters are set using Capture_Params_init().
 *
 */
typedef struct Capture_Params_ {
    /*!< Mode to be used by the timer driver. */
    Capture_Mode           mode;

    /*!< Callback function called when a trigger event occurs. */
    Capture_CallBackFxn    callbackFxn;

    /*!< Units used to specify the interval. */
    Capture_PeriodUnits    periodUnit;
} Capture_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_close().
 */
typedef void (*Capture_CloseFxn)(Capture_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_control().
 */
typedef int_fast16_t (*Capture_ControlFxn)(Capture_Handle handle,
    uint_fast16_t cmd, void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_init().
 */
typedef void (*Capture_InitFxn)(Capture_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_open().
 */
typedef Capture_Handle (*Capture_OpenFxn)(Capture_Handle handle,
    Capture_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_start().
 */
typedef int32_t (*Capture_StartFxn)(Capture_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Capture_stop().
 */
typedef void (*Capture_StopFxn)(Capture_Handle handle);

/*!
 *  @brief      The definition of a capture function table that contains the
 *              required set of functions to control a specific capture driver
 *              implementation.
 */
typedef struct Capture_FxnTable_ {
    /*!< Function to close the specified peripheral. */
    Capture_CloseFxn closeFxn;

    /*!< Function to implementation specific control function. */
    Capture_ControlFxn controlFxn;

    /*!< Function to initialize the given data object. */
    Capture_InitFxn initFxn;

    /*!< Function to open the specified peripheral. */
    Capture_OpenFxn openFxn;

    /*!< Function to start the specified peripheral. */
    Capture_StartFxn startFxn;

    /*!< Function to stop the specified peripheral. */
    Capture_StopFxn stopFxn;
} Capture_FxnTable;

/*!
 *  @brief  Capture Global configuration
 *
 *  The Capture_Config structure contains a set of pointers used to
 *  characterize the capture driver implementation.
 *
 *  This structure needs to be defined before calling Capture_init() and it
 *  must not be changed thereafter.
 *
 *  @sa     Capture_init()
 */
typedef struct Capture_Config_ {
    /*! Pointer to a table of driver-specific implementations of capture
        APIs. */
    Capture_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object. */
    void                   *object;

    /*! Pointer to a driver specific hardware attributes structure. */
    void             const *hwAttrs;
} Capture_Config;

/*!
 *  @brief  Function to close a capture driver instance. The corresponding
 *          timer peripheral to Capture_handle becomes an available resource.
 *
 *  @pre    Capture_open() has been called.
 *
 *  @param  handle  A Capture_Handle returned from Capture_open().
 *
 *  @sa     Capture_open()
 */
extern void Capture_close(Capture_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          Capture_Handle.
 *
 *  @pre    Capture_open() has been called.
 *
 *  @param  handle      A Capture_Handle returned from Capture_open().
 *
 *  @param  cmd         A command value defined by the driver specific
 *                      implementation.
 *
 *  @param  arg         A pointer to an optional R/W (read/write) argument that
 *                      is accompanied with cmd.
 *
 *  @return A Capture_Status describing an error or success state. Negative values
 *          indicate an error occurred.
 *
 *  @sa     Capture_open()
 */
extern int_fast16_t Capture_control(Capture_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief  Function to initialize the capture driver. This function will go
 *          through all available hardware resources and mark them as
 *          "available".
 *
 *  @pre    The Capture_config structure must exist and be persistent before
 *          this function can be called. This function must also be called
 *          before any other capture driver APIs.
 *
 *  @sa     Capture_open()
 */
extern void Capture_init(void);

/*!
 *  @brief  Function to open a given capture instance specified by the
 *          index argument. The Capture_Params specifies which mode the capture
 *          instance will operate. This function takes care of capture resource
 *          allocation. If the particular timer hardware is available to use,
 *          the capture driver acquires it and returns a Capture_Handle.
 *
 *  @pre    Capture_init() has been called.
 *
 *  @param  index         Logical instance number for the capture indexed into
 *                        the Capture_config table.
 *
 *  @param  params        Pointer to a parameter block. Cannot be NULL.
 *
 *  @return A Capture_Handle on success, or NULL if the timer peripheral is
 *          already in use.
 *
 *  @sa     Capture_init()
 *  @sa     Capture_close()
 */
extern Capture_Handle Capture_open(uint_least8_t index, Capture_Params *params);

/*!
 *  @brief  Function to initialize the Capture_Params struct to its defaults.
 *
 *  @param  params      An pointer to Capture_Params structure for
 *                      initialization.
 *
 *  Defaults values are:
 *      callbackFxn = NULL
 *      mode = Capture_RISING_EDGE
 *      periodUnit = Capture_PERIOD_COUNTS
 */
extern void Capture_Params_init(Capture_Params *params);

/*!
 *  @brief  Function to start the capture instance.
 *
 *  @pre    Capture_open() has been called.
 *
 *  @param  handle  A Capture_Handle returned from Capture_open().
 *
 *  @return Capture_STATUS_SUCCESS or Capture_STATUS_ERROR.
 *
 *  @sa     Capture_stop().
 *
 */
extern int32_t Capture_start(Capture_Handle handle);

/*!
 *  @brief  Function to stop a capture instance. If the capture instance is
 *          already stopped, this function has no effect.
 *
 *  @pre    Capture_open() has been called.
 *
 *  @param  handle  A Capture_Handle returned from Capture_open().
 *
 *  @sa     Capture_start()
 */
extern void Capture_stop(Capture_Handle handle);

/* The following are included for backwards compatibility. These should not be
 * used by the application.
 */
#define CAPTURE_CMD_RESERVED           Capture_CMD_RESERVED
#define CAPTURE_STATUS_RESERVED        Capture_STATUS_RESERVED
#define CAPTURE_STATUS_SUCCESS         Capture_STATUS_SUCCESS
#define CAPTURE_STATUS_ERROR           Capture_STATUS_ERROR
#define CAPTURE_STATUS_UNDEFINEDCMD    Capture_STATUS_UNDEFINEDCMD
#define CAPTURE_MODE_RISING_RISING     Capture_RISING_EDGE
#define CAPTURE_MODE_FALLING_FALLING   Capture_FALLING_EDGE
#define CAPTURE_MODE_ANY_EDGE          Capture_ANY_EDGE
#define CAPTURE_PERIOD_US              Capture_PERIOD_US
#define CAPTURE_PERIOD_HZ              Capture_PERIOD_HZ
#define CAPTURE_PERIOD_COUNTS          Capture_PERIOD_COUNTS
#define Capture_Period_Unit            Capture_PeriodUnits

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_Capture__include */
