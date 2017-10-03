/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
 *  @file       Camera.h
 *
 *  @brief      Camera driver interface
 *
 *  The Camera header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/Camera.h>
 *  @endcode
 *
 *  # Overview #
 *  The Camera driver is used to retrieve the data being transferred by the
 *  Camera sensor.
 *  This driver provides an API for capturing the image from the Camera sensor.
 *  The camera sensor control and implementation are the responsibility of the
 *  application using the interface.
 *
 *  The Camera driver has been designed to operate in an RTOS environment.  It
 *  protects its transactions with OS primitives supplied by the underlying
 *  RTOS.
 *
 *  # Usage #
 *
 *  The Camera driver includes the following APIs:
 *    - Camera_init(): Initialize the Camera driver.
 *    - Camera_Params_init():  Initialize a #Camera_Params structure with default
 *      vaules.
 *    - Camera_open():  Open an instance of the Camera driver.
 *    - Camera_control():  Performs implemenation-specific features on a given
 *      Camera peripheral.
 *    - Camera_capture():  Capture a frame.
 *    - Camera_close():  De-initialize a given Camera instance.
 *
 *
 *  ### Camera Driver Configuration #
 *
 *  In order to use the Camera APIs, the application is required
 *  to provide device-specific Camera configuration in the Board.c file.
 *  The Camera driver interface defines a configuration data structure:
 *
 *  @code
 *  typedef struct Camera_Config_ {
 *      Camera_FxnTable const  *fxnTablePtr;
 *      void                   *object;
 *      void            const  *hwAttrs;
 *  } Camera_Config;
 *  @endcode
 *
 *  The application must declare an array of Camera_Config elements, named
 *  Camera_config[].  Each element of Camera_config[] must be populated with
 *  pointers to a device specific Camera driver implementation's function
 *  table, driver object, and hardware attributes.  The hardware attributes
 *  define properties such as the Camera peripheral's base address.
 *  Each element in Camera_config[] corresponds to
 *  a Camera instance, and none of the elements should have NULL pointers.
 *  There is no correlation between the index and the
 *  peripheral designation (such as Camera0 or Camera1).  For example, it
 *  is possible to use Camera_config[0] for Camera1.
 *
 *  Because the Camera configuration is very device dependent, you will need to
 *  check the doxygen for the device specific Camera implementation.  There you
 *  will find a description of the Camera hardware attributes.  Please also
 *  refer to the Board.c file of any of your examples to see the Camera
 *  configuration.
 *
 *  ### Initializing the Camear Driver #
 *  The application initializes the Camera driver by calling Camera_init().
 *  This function must be called before any other Camera API.  Camera_init()
 *  iterates through the elements of the Camera_config[] array, calling
 *  the element's device implementation Camera initialization function.
 *  ### Camera Parameters
 *
 *  The #Camera_Params structure is passed to Camera_open().  If NULL
 *  is passed for the parameters, Camera_open() uses default parameters.
 *  A #Camera_Params structure is initialized with default values by passing
 *  it to Camera_Params_init().
 *  Some of the Camera parameters are described below.  To see brief descriptions
 *  of all the parameters, see #Camera_Params.
 *
 *  #### Camera Modes
 *  The Camera driver operates in either blocking mode or callback mode:
 *  - #Camera_MODE_BLOCKING: The call to Camera_capture() blocks until the
 *    capture has completed.
 *  - #Camera_MODE_CALLBACK: The call to Camera_capture() returns immediately.
 *    When the capture completes, the Camera driver will call a user-
 *    specified callback function.
 *
 *  The capture mode is determined by the #Camera_Params.captureMode parameter
 *  passed to Camera_open().  The Camera driver defaults to blocking mode, if the
 *  application does not set it.
 *
 *  Once a Camera driver instance is opened, the only way
 *  to change the capture mode is to close and re-open the Camera
 *  instance with the new capture mode.
 *
 *  ### Opening the driver #
 *  The following example opens a Camera driver instance in blocking mode:
 *  @code
 *  Camera_Handle      handle;
 *  Camera_Params      params;
 *
 *  Camera_Params_init(&params);
 *  params.captureMode       =  Camera_MODE_BLOCKING;
 *  < Change any other params as required >
 *
 *  handle = Camera_open(someCamera_configIndexValue, &params);
 *  if (!handle) {
 *      // Error opening the Camera driver
 *  }
 *  @endcode
 *
 *  ### Capturing an Image #
 *
 *  The following code example captures a frame.
 *
 *  @code
 *  unsigned char captureBuffer[1920];
 *
 *  ret = Camera_capture(handle, &captureBuffer, sizeof(captureBuffer));
 *  @endcode
 *
 *  # Implementation #
 *
 *  This module serves as the main interface for RTOS
 *  applications. Its purpose is to redirect the module's APIs to specific
 *  peripheral implementations which are specified using a pointer to a
 *  #Camera_FxnTable.
 *
 *  The Camera driver interface module is joined (at link time) to an
 *  array of #Camera_Config data structures named *Camera_config*.
 *  *Camera_config* is implemented in the application with each entry being an
 *  instance of a Camera peripheral. Each entry in *Camera_config* contains a:
 *  - (Camera_FxnTable *) to a set of functions that implement a Camera
 *     peripheral
 *  - (void *) data object that is associated with the Camera_FxnTable
 *  - (void *) hardware attributes that are associated to the Camera_FxnTable
 *
 *******************************************************************************
 */

#ifndef ti_drivers_Camera__include
#define ti_drivers_Camera__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 *  @defgroup CAMERA_CONTROL Camera_control command and status codes
 *  These Camera macros are reservations for Camera.h
 *  @{
 */

/*!
 * Common Camera_control command code reservation offset.
 * Camera driver implementations should offset command codes with
 * CAMERA_CMD_RESERVED growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define CAMERAXYZ_CMD_COMMAND0   CAMERA_CMD_RESERVED + 0
 * #define CAMERAXYZ_CMD_COMMAND1   CAMERA_CMD_RESERVED + 1
 * @endcode
 */
#define CAMERA_CMD_RESERVED          (32)

/*!
 * Common Camera_control status code reservation offset.
 * Camera driver implementations should offset status codes with
 * CAMERA_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define CAMERAXYZ_STATUS_ERROR0  CAMERA_STATUS_RESERVED - 0
 * #define CAMERAXYZ_STATUS_ERROR1  CAMERA_STATUS_RESERVED - 1
 * #define CAMERAXYZ_STATUS_ERROR2  CAMERA_STATUS_RESERVED - 2
 * @endcode
 */
#define CAMERA_STATUS_RESERVED      (-32)

/**
 *  @defgroup Camera_STATUS Status Codes
 *  Camera_STATUS_* macros are general status codes returned by Camera_control()
 *  @{
 *  @ingroup Camera_CONTROL
 */

/*!
 * @brief   Successful status code returned by Camera_control().
 *
 * Camera_control() returns CAMERA_STATUS_SUCCESS if the control code was
 * executed successfully.
 */
#define CAMERA_STATUS_SUCCESS       (0)

/*!
 * @brief   Generic error status code returned by Camera_control().
 *
 * Camera_control() returns CAMERA_STATUS_ERROR if the control code was not
 * executed successfully.
 */
#define CAMERA_STATUS_ERROR        (-1)

/*!
 * @brief   An error status code returned by Camera_control() for undefined
 * command codes.
 *
 * Camera_control() returns CAMERA_STATUS_UNDEFINEDCMD if the control code is
 * not recognized by the driver implementation.
 */
#define CAMERA_STATUS_UNDEFINEDCMD (-2)
/** @}*/

/**
 *  @defgroup Camera_CMD Command Codes
 *  Camera_CMD_* macros are general command codes for Camera_control(). Not all
 *  Camera driver implementations support these command codes.
 *  @{
 *  @ingroup Camera_CONTROL
 */

/* Add Camera_CMD_<commands> here */

/** @}*/

/** @}*/

/*!
 *  @brief    Wait forever define
 */
#define Camera_WAIT_FOREVER (~(0U))

/*!
 *  @brief      A handle that is returned from a Camera_open() call.
 */
typedef struct Camera_Config_      *Camera_Handle;

/*!
 *  @brief      The definition of a callback function used by the Camera driver
 *              when used in ::Camera_MODE_CALLBACK
 *
 *  @param      Camera_Handle             Camera_Handle
 *
 *  @param      buf                       Pointer to capture buffer
 *
 *  @param      frameLength               length of frame
 *
 */
typedef void (*Camera_Callback) (Camera_Handle handle, void *buf,
    size_t frameLength);

/*!
 *  @brief      Camera capture mode settings
 *
 *  This enum defines the capture mode for the
 *  configured Camera.
 */
typedef enum Camera_CaptureMode_ {
    /*!
      *  Uses a semaphore to block while data is being sent.  Context of
      *  the call must be a Task.
      */
    Camera_MODE_BLOCKING,

    /*!
      *  Non-blocking and will return immediately.  When the capture
      *  by the interrupt is finished the configured callback function
      *  is called.
      */
    Camera_MODE_CALLBACK
} Camera_CaptureMode;

/*!
 *  @brief  Camera HSync polarity
 *
 *  This enum defines the polarity of the HSync signal.
 */
typedef enum Camera_HSyncPolarity_ {
    Camera_HSYNC_POLARITY_HIGH = 0,
    Camera_HSYNC_POLARITY_LOW
} Camera_HSyncPolarity;

/*!
 *  @brief  Camera VSync polarity
 *
 *  This enum defines the polarity of the VSync signal.
 */
typedef enum Camera_VSyncPolarity_ {
    Camera_VSYNC_POLARITY_HIGH = 0,
    Camera_VSYNC_POLARITY_LOW
} Camera_VSyncPolarity;

/*!
 *  @brief  Camera pixel clock configuration
 *
 *  This enum defines the pixel clock configuration.
 */
typedef enum Camera_PixelClkConfig_ {
    Camera_PCLK_CONFIG_RISING_EDGE = 0,
    Camera_PCLK_CONFIG_FALLING_EDGE
} Camera_PixelClkConfig;

/*!
 *  @brief  Camera byte order
 *
 *  This enum defines the byte order of camera capture.
 *
 *  In normal mode, the byte order is:
 *  | byte3 | byte2 | byte1 | byte0 |
 *
 *  In swap mode, the bytes are ordered as:
 *  | byte2 | byte3 | byte0 | byte1 |
 */
typedef enum Camera_ByteOrder_ {
    Camera_BYTE_ORDER_NORMAL = 0,
    Camera_BYTE_ORDER_SWAP
} Camera_ByteOrder;

/*!
 *  @brief  Camera interface synchronization
 *
 *  This enum defines the sensor to camera interface synchronization
 *  configuration.
 */
typedef enum Camera_IfSynchoronisation_ {
    Camera_INTERFACE_SYNC_OFF = 0,
    Camera_INTERFACE_SYNC_ON
} Camera_IfSynchoronisation;

/*!
 *  @brief  Camera stop capture configuration
 *
 *  This enum defines the stop capture configuration.
 */
typedef enum Camera_StopCaptureConfig_ {
    Camera_STOP_CAPTURE_IMMEDIATE = 0,
    Camera_STOP_CAPTURE_FRAME_END
} Camera_StopCaptureConfig;

/*!
 *  @brief  Camera start capture configuration
 *
 *  This enum defines the start capture configuration.
 */
typedef enum Camera_StartCaptureConfig_ {
    Camera_START_CAPTURE_IMMEDIATE = 0,
    Camera_START_CAPTURE_FRAME_START
} Camera_StartCaptureConfig;

/*!
 *  @brief  Camera Parameters
 *
 *  Camera parameters are used to with the Camera_open() call.
 *  Default values for these parameters are set using Camera_Params_init().
 *
 *  If Camera_CaptureMode is set to Camera_MODE_BLOCKING then Camera_capture
 *  function calls will block thread execution until the capture has completed.
 *
 *  If Camera_CaptureMode is set to Camera_MODE_CALLBACK then Camera_capture
 *  will not block thread execution and it will call the function specified by
 *  captureCallbackFxn.
 *
 *  @sa     Camera_Params_init()
 */
typedef struct Camera_Params_ {
    /*!< Mode for camera capture */
    Camera_CaptureMode         captureMode;

    /*!< Output clock to set divider */
    uint32_t                    outputClock;

    /*!< Polarity of Hsync  */
    Camera_HSyncPolarity       hsyncPolarity;

    /*!< Polarity of VSync */
    Camera_VSyncPolarity       vsyncPolarity;

    /*!< Pixel clock configuration */
    Camera_PixelClkConfig      pixelClkConfig;

    /*!< camera capture byte order */
    Camera_ByteOrder           byteOrder;

    /*!< Camera-Sensor synchronization */
    Camera_IfSynchoronisation  interfaceSync;

     /*!< Camera stop configuration */
    Camera_StopCaptureConfig   stopConfig;

    /*!< Camera start configuration */
    Camera_StartCaptureConfig  startConfig;

    /*!< Timeout for capture semaphore */
    uint32_t                   captureTimeout;

    /*!< Pointer to capture callback */
    Camera_Callback            captureCallback;

    /*!< Custom argument used by driver implementation */
    void                      *custom;
} Camera_Params;

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Camera_close().
 */
typedef void        (*Camera_CloseFxn)    (Camera_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Camera_control().
 */
typedef int_fast16_t (*Camera_ControlFxn)  (Camera_Handle handle,
                                            uint_fast16_t cmd,
                                            void *arg);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Camera_init().
 */
typedef void        (*Camera_InitFxn)     (Camera_Handle handle);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Camera_open().
 */
typedef Camera_Handle (*Camera_OpenFxn) (Camera_Handle handle,
    Camera_Params *params);

/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              Camera_capture().
 */
typedef int_fast16_t (*Camera_CaptureFxn) (Camera_Handle handle, void *buffer,
    size_t bufferlen, size_t *frameLen);

/*!
 *  @brief      The definition of a Camera function table that contains the
 *              required set of functions to control a specific Camera driver
 *              implementation.
 */
typedef struct Camera_FxnTable_ {
    /*! Function to close the specified peripheral */
    Camera_CloseFxn        closeFxn;

    /*! Function to implementation specific control function */
    Camera_ControlFxn      controlFxn;

    /*! Function to initialize the given data object */
    Camera_InitFxn         initFxn;

    /*! Function to open the specified peripheral */
    Camera_OpenFxn         openFxn;

    /*! Function to initiate a Camera capture */
    Camera_CaptureFxn     captureFxn;
} Camera_FxnTable;

/*!
 *  @brief  Camera Global configuration
 *
 *  The Camera_Config structure contains a set of pointers used to characterize
 *  the Camera driver implementation.
 *
 *  This structure needs to be defined before calling Camera_init() and it must
 *  not be changed thereafter.
 *
*  @sa     Camera_init()
 */
typedef struct Camera_Config_ {
    /*! Pointer to a table of driver-specific implementations of Camera APIs */
    Camera_FxnTable const *fxnTablePtr;

    /*! Pointer to a driver specific data object */
    void                  *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void            const *hwAttrs;
} Camera_Config;

/*!
 *  @brief  Function to close a Camera peripheral specified by the Camera handle
 *
 *  @pre    Camera_open() had to be called first.
 *
 *  @param  handle  A Camera_Handle returned from Camera_open
 *
 *  @sa     Camera_open()
 */
extern void Camera_close(Camera_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          Camera_Handle.
 *
 *  Commands for Camera_control can originate from Camera.h or from
 *  implementation specific Camera*.h (_CameraCC32XX.h_, etc.. ) files.
 *  While commands from Camera.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific Camera*.h files add
 *  unique driver capabilities but are not API portable across all Camera driver
 *  implementations.
 *
 *  Commands supported by Camera.h follow a Camera_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by Camera*.h follow a Camera*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref Camera_CMD "Camera_control command codes" for command codes.
 *
 *  See @ref Camera_STATUS "Camera_control return status codes" for status codes.
 *
 *  @pre    Camera_open() has to be called first.
 *
 *  @param  handle      A Camera handle returned from Camera_open()
 *
 *  @param  cmd         Camera.h or Camera*.h commands.
 *
 *  @param  arg         An optional R/W (read/write) command argument
 *                      accompanied with cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     Camera_open()
 */
extern int_fast16_t Camera_control(Camera_Handle handle, uint_fast16_t cmd,
    void *arg);

/*!
 *  @brief  Function to initializes the Camera module
 *
 *  @pre    The Camera_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other Camera driver APIs. This function call does not modify any
 *          peripheral registers.
 */
extern void Camera_init(void);

/*!
 *  @brief  Function to initialize a given Camera peripheral specified by the
 *          particular index value. The parameter specifies which mode the
 *          Camera will operate.
 *
 *  @pre    Camera controller has been initialized
 *
 *  @param  index         Logical peripheral number for the Camera indexed into
 *                        the Camera_config table
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A Camera_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     Camera_init()
 *  @sa     Camera_close()
 */
extern Camera_Handle Camera_open(uint_least8_t index, Camera_Params *params);

/*!
 *  @brief  Function to initialize the Camera_Params structure to its defaults
 *
 *  @param  params      An pointer to Camera_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      captureMode       =  Camera_MODE_BLOCKING;
 *      outputClock       =  24000000;
 *      hsyncPolarity     =  Camera_HSYNC_POLARITY_HIGH;
 *      vsyncPolarity     =  Camera_VSYNC_POLARITY_HIGH;
 *      pixelClkConfig    =  Camera_PCLK_CONFIG_RISING_EDGE;
 *      byteOrder         =  Camera_BYTE_ORDER_NORMAL;
 *      interfaceSync     =  Camera_INTERFACE_SYNC_ON;
 *      stopConfig        =  Camera_STOP_CAPTURE_FRAME_END;
 *      startConfig       =  Camera_START_CAPTURE_FRAME_START;
 *      captureTimeout    =  Camera_WAIT_FOREVER;
 *      captureCallback   =  NULL;
 */
extern void Camera_Params_init(Camera_Params *params);

/*!
 *  @brief  Function that handles the Camera capture of a frame.
 *
 *  In Camera_MODE_BLOCKING, Camera_capture will block task execution until
 *  the capture is complete.
 *
 *  In Camera_MODE_CALLBACK, Camera_capture does not block task execution
 *  and calls a callback function specified by captureCallbackFxn.
 *  The Camera buffer must stay persistent until the Camera_capture
 *  function has completed!
 *
 *  @param  handle      A Camera_Handle
 *
 *  @param  buffer      A pointer to a WO (write-only) buffer into which the
 *                      captured frame is placed
 *
 *  @param  bufferlen   Length (in bytes) of the capture buffer
 *
 *  @param  frameLen    Pointer to return number of bytes captured.
 *
 *  @return CAMERA_STATUS_SUCCESS on successful capture, CAMERA_STATUS_ERROR if
 *          if otherwise.
 *
 *  @sa     Camera_open
 */
extern int_fast16_t Camera_capture(Camera_Handle handle, void *buffer,
    size_t bufferlen, size_t *frameLen);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_Camera__include */
