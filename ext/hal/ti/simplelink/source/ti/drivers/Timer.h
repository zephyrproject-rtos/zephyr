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
 *  @file       Timer.h
 *  @brief      Timer driver interface
 *
 *  The timer header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/Timer.h>
 *  @endcode
 *
 *  # Overview #
 *  The timer driver serves as the main interface for a typical RTOS
 *  application. Its purpose is to redirect the timer APIs to device specific
 *  implementations which are specified using a pointer to a #Timer_FxnTable.
 *  The device specific implementations are responsible for creating all the
 *  RTOS specific primitives to allow for thead-safe operation. This driver
 *  does not have PWM or capture functionalities. These functionalities are
 *  addressed in both the capture and PWM driver.
 *
 *  The timer driver also handles the general purpose timer resource allocation.
 *  For each driver that requires use of a general purpose timer, it calls
 *  Timer_open() to occupy the specified timer, and calls Timer_close() to
 *  release the occupied timer resource.
 *
 *  # Usage #
 *  The following example code opens a timer in continuous callback mode. The
 *  period is set to 1000 Hz.
 *
 *  @code
 *  Timer_Handle    handle;
 *  Timer_Params    params;
 *
 *  Timer_Params_init(&params);
 *  params.periodUnits = Timer_PERIOD_HZ;
 *  params.period = 1000;
 *  params.timerMode  = Timer_CONTINUOUS_CALLBACK;
 *  params.timerCallback = someTimerCallbackFunction;
 *
 *  handle = Timer_open(Board_TIMER0, &params);
 *
 *  if (handle == NULL) {
 *      // Timer_open() failed
 *      while (1);
 *  }
 *
 *  status = Timer_start(handle);
 *
 *  if (status == Timer_STATUS_ERROR) {
 *      //Timer_start() failed
 *      while (1);
 *  }
 *
 *  sleep(10000);
 *
 *  Timer_stop(handle);
 *  @endcode
 *
 *  ### Timer Driver Configuration #
 *
 *  In order to use the timer APIs, the application is required to provide
 *  device specific timer configuration in the Board.c file. The timer driver
 *  interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct Timer_Config_ {
 *      Timer_FxnTable const *fxnTablePtr;
 *      void                 *object;
 *      void           const *hwAttrs;
 *  } Timer_Config;
 *  @endcode
 *
 *  The application must declare an array of Timer_Config elements, named
 *  Timer_config[]. Each element of Timer_config[] are populated with
 *  pointers to a device specific timer driver implementation's function
 *  table, driver object, and hardware attributes. The hardware attributes
 *  define properties such as the timer peripheral's base address, interrupt
 *  number and interrupt priority. Each element in Timer_config[] corresponds
 *  to a timer instance, and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the peripheral designation
 *  (such as TIMER0 or TIMER1). For example, it is possible to use
 *  Timer_config[0] for TIMER1.
 *
 *  You will need to check the device specific timer driver implementation's
 *  header file for example configuration.
 *
 *  ### Initializing the Timer Driver #
 *
 *  Timer_init() must be called before any other timer APIs. This function
 *  calls the device implementation's timer initialization function, for each
 *  element of Timer_config[].
 *
 *  ### Modes of Operation #
 *
 *  The timer driver supports four modes of operation which may be specified in
 *  the Timer_Params. The device specific implementation may configure the timer
 *  peripheral as an up or down counter. In any case, Timer_getCount() will
 *  return a value characteristic of an up counter.
 *
 *  #Timer_ONESHOT_CALLBACK is non-blocking. After Timer_start() is called,
 *  the calling thread will continue execution. When the timer interrupt
 *  is triggered, the specified callback function will be called. The timer
 *  will not generate another interrupt unless Timer_start() is called again.
 *  Calling Timer_stop() or Timer_close() after Timer_start() but, before the
 *  timer interrupt, will prevent the specified callback from ever being
 *  invoked.
 *
 *  #Timer_ONESHOT_BLOCKING is a blocking call. A semaphore is used to block
 *  the calling thread's execution until the timer generates an interrupt. If
 *  Timer_stop() is called, the calling thread will become unblocked
 *  immediately. The behavior of the timer in this mode is similar to a sleep
 *  function.
 *
 *  #Timer_CONTINUOUS_CALLBACK is non-blocking. After Timer_start() is called,
 *  the calling thread will continue execution. When the timer interrupt is
 *  treiggered, the specified callback function will be called. The timer is
 *  automatically restarted and will continue to periodically generate
 *  interrupts until Timer_stop() is called.
 *
 *  #Timer_FREE_RUNNING is non-blocking. After Timer_start() is called,
 *  the calling thread will continue execution. The timer will not
 *  generate an interrupt in this mode. The timer hardware will run until
 *  Timer_stop() is called.
 *
 *  # Implementation #
 *
 *  The timer driver interface module is joined (at link time) to an
 *  array of Timer_Config data structures named *Timer_config*.
 *  Timer_config is implemented in the application with each entry being an
 *  instance of a timer peripheral. Each entry in *Timer_config* contains a:
 *  - (Timer_FxnTable *) to a set of functions that implement a timer peripheral
 *  - (void *) data object that is associated with the Timer_FxnTable
 *  - (void *) hardware attributes that are associated with the Timer_FxnTable
 *
 *  The timer APIs are redirected to the device specific implementations
 *  using the Timer_FxnTable pointer of the Timer_config entry.
 *  In order to use device specific functions of the timer driver directly,
 *  link in the correct driver library for your device and include the
 *  device specific timer driver header file (which in turn includes Timer.h).
 *  For example, for the MSP432 family of devices, you would include the
 *  following header file:
 *
 *  @code
 *  #include <ti/drivers/timer/TimerMSP432.h>
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef ti_drivers_Timer__include
#define ti_drivers_Timer__include

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/*!
 * Common Timer_control command code reservation offset.
 * Timer driver implementations should offset command codes with Timer_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define TimerXYZ_CMD_COMMAND0      Timer_CMD_RESERVED + 0
 * #define TimerXYZ_CMD_COMMAND1      Timer_CMD_RESERVED + 1
 * @endcode
 */
#define Timer_CMD_RESERVED                (32)

/*!
 * Common Timer_control status code reservation offset.
 * Timer driver implementations should offset status codes with
 * Timer_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define TimerXYZ_STATUS_ERROR0     Timer_STATUS_RESERVED - 0
 * #define TimerXYZ_STATUS_ERROR1     Timer_STATUS_RESERVED - 1
 * @endcode
 */
#define Timer_STATUS_RESERVED            (-32)

/*!
 * @brief   Successful status code.
 */
#define Timer_STATUS_SUCCESS               (0)

/*!
 * @brief   Generic error status code.
 */
#define Timer_STATUS_ERROR                (-1)

/*!
 * @brief   An error status code returned by Timer_control() for undefined
 *          command codes.
 *
 * Timer_control() returns Timer_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define Timer_STATUS_UNDEFINEDCMD         (-2)

/*!
 *  @brief      A handle that is returned from a Timer_open() call.
 */
typedef struct Timer_Config_ *Timer_Handle;

/*!
 *  @brief Timer mode settings
 *
 *  This enum defines the timer modes that may be specified in #Timer_Params.
 */
typedef enum Timer_Mode_ {
    Timer_ONESHOT_CALLBACK,       /*!< User routine doesn't get blocked and
                                       user-specified callback function is
                                       invoked once the timer interrupt happens
                                       for only one time */
    Timer_ONESHOT_BLOCKING,       /*!< User routine gets blocked until timer
                                       interrupt happens for only one time. */
    Timer_CONTINUOUS_CALLBACK,    /*!< User routine doesn't get blocked and
                                       user-specified callback function is
                                       invoked with every timer interrupt. */
    Timer_FREE_RUNNING
} Timer_Mode;

/*!
 *  @brief Timer period unit enum
 *
 *  This enum defines the units that may be specified for the period
 *  in #Timer_Params. This unit has no effect with Timer_getCounts.
 */
typedef enum Timer_PeriodUnits_ {
    Timer_PERIOD_US,      /*!< Period specified in micro seconds. */
    Timer_PERIOD_HZ,      /*!< Period specified in hertz; interrupts per
                                  second. */
    Timer_PERIOD_COUNTS   /*!< Period specified in ticks or counts. Varies
                                  from board to board. */
} Timer_PeriodUnits;

/*!
 *  @brief  Timer callback function
 *
 *  User definable callback function prototype. The timer driver will call the
 *  defined function and pass in the timer driver's handle and the pointer to the
 *  user-specified the argument.
 *
 *  @param  handle         Timer_Handle
 */
typedef void (*Timer_CallBackFxn)(Timer_Handle handle);

/*!
 *  @brief Timer Parameters
 *
 *  Timer parameters are used to with the Timer_open() call. Default values for
 *  these parameters are set using Timer_Params_init().
 *
 */
typedef struct Timer_Params_ {
    /*!< Mode to be used by the timer driver. */
    Timer_Mode           timerMode;

    /*!< Units used to specify the period. */
    Timer_PeriodUnits    periodUnits;

    /*!< Callback function called when timerMode is Timer_ONESHOT_CALLBACK or
         Timer_CONTINUOUS_CALLBACK. */
    Timer_CallBackFxn    timerCallback;

    /*!< Period in units of periodUnits. */
    uint32_t             period;
} Timer_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_control().
 */
typedef int_fast16_t (*Timer_ControlFxn)(Timer_Handle handle,
    uint_fast16_t cmd, void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_close().
 */
typedef void (*Timer_CloseFxn)(Timer_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_getCount().
 */
typedef uint32_t (*Timer_GetCountFxn)(Timer_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_init().
 */
typedef void (*Timer_InitFxn)(Timer_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_open().
 */
typedef Timer_Handle (*Timer_OpenFxn)(Timer_Handle handle,
    Timer_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_start().
 */
typedef int32_t (*Timer_StartFxn)(Timer_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Timer_stop().
 */
typedef void (*Timer_StopFxn)(Timer_Handle handle);

/*!
 *  @brief      The definition of a timer function table that contains the
 *              required set of functions to control a specific timer driver
 *              implementation.
 */
typedef struct Timer_FxnTable_ {
    /*!< Function to close the specified peripheral. */
    Timer_CloseFxn closeFxn;

    /*!< Function to implementation specific control function. */
    Timer_ControlFxn controlFxn;

    /*!< Function to get the count of the specified peripheral. */
    Timer_GetCountFxn getCountFxn;

    /*!< Function to initialize the given data object. */
    Timer_InitFxn initFxn;

    /*!< Function to open the specified peripheral. */
    Timer_OpenFxn openFxn;

    /*!< Function to start the specified peripheral. */
    Timer_StartFxn startFxn;

    /*!< Function to stop the specified peripheral. */
    Timer_StopFxn stopFxn;
} Timer_FxnTable;

/*!
 *  @brief  Timer Global configuration
 *
 *  The Timer_Config structure contains a set of pointers used to characterize
 *  the timer driver implementation.
 *
 *  This structure needs to be defined before calling Timer_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     Timer_init()
 */
typedef struct Timer_Config_ {
    /*! Pointer to a table of driver-specific implementations of timer APIs. */
    Timer_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object. */
    void                 *object;

    /*! Pointer to a driver specific hardware attributes structure. */
    void           const *hwAttrs;
} Timer_Config;

/*!
 *  @brief  Function to close a timer. The corresponding timer to the
 *          Timer_Handle becomes an available timer resource.
 *
 *  @pre    Timer_open() has been called.
 *
 *  @param  handle  A Timer_Handle returned from Timer_open().
 *
 *  @sa     Timer_open()
 */
extern void Timer_close(Timer_Handle handle);

/*!
 *  @brief  Function performs device specific features on a given
 *          Timer_Handle.
 *
 *  @pre    Timer_open() has been called.
 *
 *  @param  handle      A Timer_Handle returned from Timer_open().
 *
 *  @param  cmd         A command value defined by the driver specific
 *                      implementation.
 *
 *  @param  arg         A pointer to an optional R/W (read/write) argument that
 *                      is accompanied with cmd.
 *
 *  @return A Timer_Status describing an error or success state. Negative values
 *          indicate an error occurred.
 *
 *  @sa     Timer_open()
 */
extern int_fast16_t Timer_control(Timer_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief  Function to get the current count of a timer. The value returned
 *          represents timer counts. The value returned is always
 *          characteristic of an up counter. This is true even if the timer
 *          peripheral is counting down. Some device specific implementations
 *          may employ a prescaler in addition to this timer count.
 *
 *  @pre    Timer_open() has been called.
 *
 *  @param  handle  A Timer_Handle returned from Timer_open().
 *
 *  @sa     Timer_open()
 *
 *  @return The current count of the timer in timer ticks.
 *
 */
extern uint32_t Timer_getCount(Timer_Handle handle);


/*!
 *  @brief  Function to initialize a timer module. This function will go through
 *          all available hardware resources and mark them as "available".
 *
 *  @pre    The Timer_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other timer driver APIs.
 *
 *  @sa     Timer_open()
 */
extern void Timer_init(void);

/*!
 *  @brief  Function to initialize a given timer peripheral specified by the
 *          index argument. The Timer_Params specifies which mode the timer
 *          will operate. The accuracy of the desired period is limited by the
 *          the clock. For example, a 100 MHz clock will have a tick resolution
 *          of 10 nanoseconds. This function takes care of timer resource
 *          allocation. If the particular timer is available to use, the timer
 *          driver owns it and returns a Timer_Handle.
 *
 *  @pre    Timer_init() has been called.
 *
 *  @param  index         Logical peripheral number for the timer indexed into
 *                        the Timer_config table.
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values.
 *
 *  @return A Timer_Handle upon success or NULL. If the desired period results
 *          in overflow, or saturation, of the timer, NULL is returned. If the
 *          timer resource is already in use, NULL is returned.
 *
 *  @sa     Timer_init()
 *  @sa     Timer_close()
 */
extern Timer_Handle Timer_open(uint_least8_t index, Timer_Params *params);

/*!
 *  @brief  Function to initialize the Timer_Params struct to its defaults.
 *
 *  @param  params      A pointer to Timer_Params structure for
 *                      initialization.
 *
 *  Defaults values are:
 *      timerMode = Timer_ONESHOT_BLOCKING
 *      periodUnit = Timer_PERIOD_COUNTS
 *      timerCallback = NULL
 *      period = (uint16_t) ~0
 */
extern void Timer_Params_init(Timer_Params *params);

/*!
 *  @brief  Function to start the timer.
 *
 *  @pre    Timer_open() has been called.
 *
 *  @param  handle  A Timer_Handle returned from Timer_open().
 *
 *  @return Timer_STATUS_SUCCESS or Timer_STATUS_ERROR.
 *
 *  @sa     Timer_stop()
 */
extern int32_t Timer_start(Timer_Handle handle);

/*!
 *  @brief  Function to stop timer. If the timer is already stopped, this
 *          function has no effect.
 *
 *  @pre    Timer_open() has been called.
 *
 *  @param  handle  A Timer_Handle returned from Timer_open().
 *
 *  @sa     Timer_start()
 */
extern void Timer_stop(Timer_Handle handle);

/* The following are included for backwards compatibility. These should not be
 * used by the application.
 */
#define TIMER_CMD_RESERVED           Timer_CMD_RESERVED
#define TIMER_STATUS_RESERVED        Timer_STATUS_RESERVED
#define TIMER_STATUS_SUCCESS         Timer_STATUS_SUCCESS
#define TIMER_STATUS_ERROR           Timer_STATUS_ERROR
#define TIMER_STATUS_UNDEFINEDCMD    Timer_STATUS_UNDEFINEDCMD
#define TIMER_ONESHOT_CB             Timer_ONESHOT_CALLBACK
#define TIMER_ONESHOT_BLOCK          Timer_ONESHOT_BLOCKING
#define TIMER_CONTINUOUS_CB          Timer_CONTINUOUS_CALLBACK
#define TIMER_MODE_FREE_RUNNING      Timer_FREE_RUNNING
#define TIMER_PERIOD_US              Timer_PERIOD_US
#define TIMER_PERIOD_HZ              Timer_PERIOD_HZ
#define TIMER_PERIOD_COUNTS          Timer_PERIOD_COUNTS
#define Timer_Period_Units           Timer_PeriodUnits

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_Timer__include */
