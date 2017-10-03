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
 *  @file       PIN.h
 *  @brief      Generic PIN & GPIO driver
 *
 *  To use the PIN driver ensure that the correct TI-RTOS driver library for your
 *  device is linked in and include this header file:
 *    @code
 *    #include <ti/drivers/PIN.h>
 *    @endcode
 *
 *  In order to use device-specific functionality or to use the size/speed-
 *  optimized versions of some of the PIN driver functions that circumvent error
 *  and resource checking, link in the correct TI-RTOS driver library for your
 *  device and include the device-specific PIN driver header file (which in turn
 *  includes PIN.h). As an example for the CC26xx family of devices:
 *    @code
 *    #include <ti/drivers/pin/PINCC26xx.h>
 *    @endcode
 *
 *  # Overview #
 *  The PIN driver allows clients (applications or other drivers) to allocate
 *  and control the I/O pins on the device. The pins can either be software-
 *  controlled general-purpose I/O (GPIO) or connected to hardware peripherals.
 *  Furthermore, the PIN driver allows clients to configure interrupt
 *  functionality on the pins to receive callbacks (and potentially wake up from
 *  the standby or idle power modes) on configurable signal edges.
 *
 *  Most other drivers rely on functionality in the PIN driver.
 *
 *  ## Structure ##
 *  In order to provide a generic driver interface, this file (PIN.h) only
 *  defines the API and some common data types and macros of the driver. A PIN
 *  client (application or driver) can in most cases only use the generic PIN
 *  API, however, for more advanced usage where device-specific pin
 *  configuration is used or device-specific PIN driver API extensions are
 *  used must use the device-specific PIN driver API.
 *
 *  The device-independent API is implemented as function calls with pin
 *  access control based on the PIN client handle. For time-critical
 *  applications the device-specific API can be used directly, as these
 *  API functions are implemented as inlined functions without access control.
 *
 *  ## Functionality ##
 *  The PIN module provides the following functionality:
 *  - Initialize I/O pins upon boot to a default configuration (possibly
 *    user-generated)
 *  - Provides atomic manipulation of I/O pin hardware registers to allow safe
 *    simultaneous use of I/O pin resources
 *  - I/O pin allocation
 *    - A set of pins can be allocated receiving a pin set handle.
 *      Typically each peripheral driver will allocate a set of pins and an
 *      application must allocate the pins it uses too
 *    - When a pin set is deallocated all the pins in it revert to the state
 *      they were initialized to at boot
 *  - General-purpose I/O (GPIO) services
 *    - Read input buffer value
 *    - Read and set output buffer value
 *    - Read and set output buffer enable
 *    - Access as single pin or port (muliple pins simultaneously)
 *  - Protect pin manipulation
 *    - Pins in an allocated set can only be manipulated using the corresponding
 *      handle.
 *    - No handle is needed to read input and output buffer values
 *  - I/O buffer/driver control
 *    - Input mode (detached, hysteresis, pull-up, pull-down)
 *    - Output mode (tristated, push-pull, open drain, open source)
 *    - Output driver strength control
 *    - Output driver slew rate control
 *  - I/O source/target selection (device-specific driver only)
 *    - Map pin to GPIO, peripheral or HW observation signal
 *  - Configuration of I/O interrupt and wakeup from standby
 *    - Interrupt configuration: signal edge to interrupt on, interrupt mask,
 *      callback function registration
 *    - Pins that have enabled interrupts will also wake up the device from low-
 *      power modes like standby and idle upon events
 *  - Provides data types and enums/defines for use in pin configurations
 *    definitions in board files, drivers and applications
 *
 *  ## Pin Allocation ##
 *  The purpose of being able to allocate pins to a pin set is to:
 *  - Manage pin resources
 *  - Give exclusive, protected access to these pins
 *  - Establish a driver state in connection with these pins that allow
 *    functionality such as I/O interrupt callback and I/O port operations
 *    in a safe manner
 *
 *  | API function       | Description                                          |
 *  |--------------------|------------------------------------------------------|
 *  | PIN_open()         | Allocate pins to a set, returns handle               |
 *  | PIN_add()          | Add pin to pin set for open PIN handle               |
 *  | PIN_remove()       | Removes pin from pin set for open PIN handle         |
 *  | PIN_close()        | Deallocate pin set, revert to original GPIO state    |
 *
 *  ## GPIO ##
 *  Pins that are to be used as software-controlled general-purpose I/O (GPIO)
 *  need to be allocated in the same manner as for pins that will be mapped to
 *  hardware peripheral ports. A pin set requested with a PIN_open() call may
 *  contain a mix of pins to be used for GPIO and hardware-mapped pins.
 *
 *  When a pin is deallocated using PIN_close() it reverts to the GPIO
 *  configuration it was given in the initial call to PIN_init().
 *
 *  | API function         | Description                                       |
 *  |----------------------|---------------------------------------------------|
 *  | PIN_init()           | Initialize I/O pins to a safe GPIO state          |
 *  | PIN_open()           | Allocate pins to a set, returns handle            |
 *  | PIN_close()          | Deallocate pin set, revert to original GPIO state |
 *  | PIN_setConfig()      | Sets parts of or complete pin configuration       |
 *  | PIN_getConfig()      | Returns pin configuration                         |
 *  | PIN_setOutputEnable()| Control output enable of GPIO pin                 |
 *  | PIN_getInputValue()  | Read input value on pin                           |
 *  | PIN_setOutputValue() | Set output value of GPIO pin                      |
 *  | PIN_getOutputValue() | Get current output value of GPIO pin              |
 *
 *  ## GPIO Ports ##
 *  Sometimes it is necessary to be able to read from, write to or control
 *  multiple pins simultaneously (in time). The PIN driver allows a set of
 *  allocated pins, if they reside on the same GPIO port in the underlying
 *  hardware, to be manipulated simultaneously.
 *
 *  | API function             | Description                                       |
 *  |--------------------------|---------------------------------------------------|
 *  | PIN_open()               | Allocate pins to a set, returns handle            |
 *  | PIN_close()              | Deallocate pin set, revert to original GPIO state |
 *  | PIN_getPortMask()        | Returns bitmask for allocated pins in GPIO port   |
 *  | PIN_getPortInputValue()  | Returns input value of whole GPIO port            |
 *  | PIN_setPortOutputValue() | Sets output value of whole GPIO port (masked)     |
 *  | PIN_getPortOutputValue() | Get current output value of whole GPIO port       |
 *  | PIN_setPortOutputValue() | Sets output value of whole GPIO port (masked)     |
 *  | PIN_setPortOutputEnable()| Sets output enable of whole GPIO port (masked)    |
 *
 *  ## I/O Pin Configuration ##
 *  Different devices provide different levels of configurability of I/O pins.
 *  The PIN driver provides a fairly extensive set of @ref PIN_GENERIC_FLAGS
 *  "generic IO configuration options" that are device-independent, all of which
 *  might not be supported by the underlying device-specific PIN driver and
 *  hardware. Likewise, the underlying device-specific PIN driver and hardware
 *  might support additional configuration options not covered by the generic
 *  options.
 *
 *  To allow both independence from and flexibility to use features on the target
 *  device, the #PIN_Config entries used by the PIN driver allows use of either
 *  a set of @ref PIN_GENERIC_FLAGS "generic PIN configuration options" or a
 *  device-specific set of PIN configuration options defined in the underlying
 *  device-specific PIN driver (e.g. PINCC26XX.h)
 *
 *  ### Mapping to GPIO or Peripheral ###
 *  Since the amount of flexibilty in which peripherals can be mapped to which
 *  pins and the manner in which this needs to be set up is highly
 *  device-specific, functions for configuring this is not part of the generic
 *  PIN driver API but is left to be implemented by device-specific PIN drivers.
 *  See the relevant device-specific PIN driver (e.g. PINCC26XX.h) for details.
 *
 *  ### Input Mode ###
 *  The input mode of a pin controls:
 *  - Input buffer enable
 *  - Pull-ups or pull-downs
 *  - Hysteresis of input buffer
 *  - Inversion of logical input level
 *  - Potentially, device-specific options
 *  The input mode is set initially with PIN_init() or at a later stage with
 *  PIN_setConfig() and a bitmask with the relevant options
 *
 *  | API function     | Description                                           |
 *  |------------------|-------------------------------------------------------|
 *  | PIN_init()       | Initialize IOs to a safe GPIO state                   |
 *  | PIN_getConfig()  | Returns pin configuration                             |
 *  | PIN_setConfig()  | Sets parts of or complete pin configuration           |
 *
 *  ### Output Mode ###
 *  The output mode of a pin controls:
 *  - Output buffer enable
 *  - Output driver mode (push-pull, open-drain, open-source)
 *  - Output driver slew control
 *  - Output driver current (drive strength)
 *  - Inversion of logical output level
 *  - Potentially, device-specific options
 *
 *  | API function         | Description                                       |
 *  |----------------------|---------------------------------------------------|
 *  | PIN_init()           | Initialize IOs to a safe GPIO state               |
 *  | PIN_setOutputEnable()| Control output enable of GPIO pins                      |
 *  | PIN_getConfig()      | Returns pin configuration                         |
 *  | PIN_setConfig()      | Sets parts of or complete pin configuration       |
 *
 *  ### Pin Interrupt and Pin Wakeup ###
 *  Pin interrupts are used to process asynchronous signal edge events on pins
 *  and potentially wake the device up from low power sleep modes. To use pin
 *  interrupts the relevant pins must be allocated and a interrupt callback
 *  registered by the client. The callback function will be called in a SWI
 *  context.
 *
 *  | API function        | Description                                        |
 *  |---------------------|----------------------------------------------------|
 *  | PIN_init()          | Initialize IOs to a safe GPIO state                |
 *  | PIN_getConfig()     | Returns pin configuration                          |
 *  | PIN_setConfig()     | Sets parts of or complete pin configuration        |
 *  | PIN_setInterrupt()  | Control interrupt enable and edge for pin          |
 *  | PIN_registerIntCb() | Register callback function for a set of pins       |
 *  | PIN_setUserArg()    | Sets a user argument associated with the handle    |
 *  | PIN_getUserArg()    | Gets a user argument associated with the handle    |
 *
 *  ## PIN Data Types ##
 *  The PIN driver defines the following data types:
 *  - #PIN_Id: identifies a pin in arguments or lists
 *  - #PIN_Config: provides I/O configuration options for a pin and also embeds
 *    a #PIN_Id identifier. See @ref PIN_GENERIC_FLAGS "available flags/fields"
 *
 *  ## PIN Config Flags/Fields and Bitmasks ##
 *  The PIN driver uses the #PIN_Config data type many places and it merits some
 *  additional attention. A #PIN_Config value consists of a collection of flags
 *  and fields that define how an I/O pin and its attached GPIO interface should
 *  behave electrically and logically. In addition a #PIN_Config value also
 *  embeds a #PIN_Id pin ID, identifying which pin it refers to.
 *
 *  A #PIN_Config value can use one of two mutually exclusive sets of flags and
 *  fields: @ref PIN_GENERIC_FLAGS "device-independent options" defined in
 *  PIN.h or device-dependent options defined in the device-specific
 *  implementation of the PIN driver interface. Any function that uses
 *  #PIN_Config will accept both option types, just not at the same time.
 *  PIN_getConfig() always returns device-independent options, an additional
 *  device-specific version (e.g. PINCC26XX_getConfig()) might return
 *  device-specific options.
 *
 *  The bitmask argument for PIN_setConfig() decides which of the options the
 *  call should affect. All other options are kept at their current values in
 *  hardware. Thus PIN_setConfig(hPins, PIN_BM_PULLING, PIN_BM_PULLUP) will only
 *  change the pullup/pulldown configuration of the pin, leaving everything
 *  else, such as for instance output enable, input hysteresis or output value,
 *  untouched. For #PIN_Config lists (as supplied to PIN_init() for instance)
 *  there is no mask, so all options will affect the pin.
 *
 *  Some of the options affect the pin regardless of whether it is mapped to
 *  a hardware peripheral or GPIO and some options only take effect when it is
 *  mapped to GPIO. These latter options have \_GPIO_ in their names.
 *
 *  The default value for a flag/field is indicated with a star (*) in the
 *  description of the options and will be applied if any explicit value is
 *  not supplied for a flag/field that is masked.
 *
 *  The available options can be grouped into categories as follows:
 *
 *  ### Input Mode Options ###
 *  | Option             | Option bitmask        | HW/GPIO | Description                    |
 *  |--------------------|-----------------------|---------|--------------------------------|
 *  |#PIN_INPUT_EN (*)   |#PIN_BM_INPUT_EN       | Both    | Enable pin input buffer        |
 *  |#PIN_INPUT_DIS      |#PIN_BM_INPUT_EN       | Both    | Disable pin input buffer       |
 *  |#PIN_HYSTERESIS     |#PIN_BM_HYSTERESIS     | Both    | Enable hysteresis on input     |
 *  |#PIN_NOPULL (*)     |#PIN_BM_PULLING        | Both    | No pullup/pulldown             |
 *  |#PIN_PULLUP         |#PIN_BM_PULLING        | Both    | Enable pullup                  |
 *  |#PIN_PULLDOWN       |#PIN_BM_PULLING        | Both    | Enable pulldown                |
 *  |                    |#PIN_BM_INPUT_MODE     |         | Mask for all input mode options|
 *
 *  ### Output Mode Options ###
 *  | Option             | Option bitmask             | HW/GPIO | Description                      |
 *  |------------------------|------------------------|---------|----------------------------------|
 *  |#PIN_GPIO_OUTPUT_DIS (*)|#PIN_BM_GPIO_OUTPUT_EN  | GPIO    | Disable GPIO output buffer       |
 *  |#PIN_GPIO_OUTPUT_EN     |#PIN_BM_GPIO_OUTPUT_EN  | GPIO    | Enable GPIO output buffer        |
 *  |#PIN_GPIO_LOW (*)       |#PIN_BM_GPIO_OUTPUT_VAL | GPIO    | Output 0 when GPIO               |
 *  |#PIN_GPIO_HIGH          |#PIN_BM_GPIO_OUTPUT_VAL | GPIO    | Output 1 when GPIO               |
 *  |#PIN_PUSHPULL (*)       |#PIN_BM_OUTPUT_BUF      | Both    | Use push-pull output buffer      |
 *  |#PIN_OPENDRAIN          |#PIN_BM_OUTPUT_BUF      | Both    | Use open drain output buffer     |
 *  |#PIN_OPENSOURCE         |#PIN_BM_OUTPUT_BUF      | Both    | Use open source output buffer    |
 *  |#PIN_SLEWCTRL           |#PIN_BM_SLEWCTRL        | Both    | Enable output buffer slew control|
 *  |#PIN_DRVSTR_MIN (*)     |#PIN_BM_DRVSTR          | Both    | Output buffer uses min drive     |
 *  |#PIN_DRVSTR_MED         |#PIN_BM_DRVSTR          | Both    | Output buffer uses medium drive  |
 *  |#PIN_DRVSTR_MAX         |#PIN_BM_DRVSTR          | Both    | Output buffer uses max drive     |
 *  |                        |#PIN_BM_OUTPUT_MODE     |         | Mask for all output mode options |
 *
  *  ### Misc Options ###
 *  | Option            | Option bitmask   | HW/GPIO | Description                      |
 *  |-------------------|------------------|---------|----------------------------------|
 *  |#PIN_INV_INOUT     |#PIN_BM_INV_INOUT | Both    | Invert input/output              |
 *  |#PIN_IRQ_DIS (*)   |#PIN_BM_IRQ       | Both    | Disable pin interrupts           |
 *  |#PIN_IRQ_NEGEDGE   |#PIN_BM_IRQ       | Both    | Pin interrupts on negative edges |
 *  |#PIN_IRQ_POSEDGE   |#PIN_BM_IRQ       | Both    | Pin interrupts on negative edges |
 *  |#PIN_IRQ_BOTHEDGES |#PIN_BM_IRQ       | Both    | Pin interrupts on both edges     |
 *  |                   |#PIN_BM_ALL       |         | Mask for *all* options           |
 *
 *  ## Initialization ##
 *  The PIN driver must be initialized before any other drivers are initialized.
 *  In order for IO pins to get a safe value as soon as possible PIN_init()
 *  should be called as early as possible in the boot sequence. Typically,
 *  PIN_init() is called at the start of main() before TI-RTOS is started with
 *  BIOS_start().
 *
 *  PIN_init() takes as an argument a #PIN_Config list containing default pin
 *  configurations. Typically the #PIN_Config list defined in the board files
 *  is used:
 *  @code
 *  PIN_init(BoardGpioInitTable);
 *  @endcode
 *  It is possible, however, to use another #PIN_Config list if desired.
 *
 *  ## Power Management Interaction ##
 *  No specific interaction with power management module, as PIN is independent
 *  of power mode.
 *
 *  ## Functionality Not Supported ##
 *  There is no known unsupported functionality.
 *
 *  ## Instrumentation ##
 *  The pin driver does not use any of the instrumentation facilities.
 *
 *  # Usage Examples #
 *
 *  ## Initialization and Pin Allocation ##
 *  Example that illustrates when and how to call PIN_init(), PIN_open(), PIN_add(), PIN_close()
 *    @code
 *    // Default pin configuration. Typically resides in Board.c file.
 *    // IOs not mentioned here configured to default: input/output/pull disabled
 *    PIN_Config BoardGpioInitTable[] = {
 *        // DIO11: LED A (initially off)
 *        PIN_ID(11) | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 *        // DIO10: LED B (initially off)
 *        PIN_ID(10)  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 *        // DIO23: BUTTON A (ensure pull-up as button A is also used by other ICs)
 *        PIN_ID(23) | PIN_INPUT_EN  | PIN_PULLUP | PIN_HYSTERESIS,
 *        // DIO3: LCD controller reset line (make sure LCD is in reset)
 *        PIN_ID(3)  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL,
 *        // Terminate list
 *        PIN_TERMINATE
 *    };
 *
 *    Task_Struct taskStart;
 *    uint8_t     taskStartStack[512];
 *
 *    // PIN_init() should be called as early as possible in boot
 *    void main() {
 *        // Default initialization of IO
 *        PIN_init(BoardGpioInitTable);
 *
 *        // Configure startup task
 *        Task_Params taskParams;
 *        Task_Params_init(&taskParams);
 *        taskParams.stack = taskStartStack;
 *        taskParams.stackSize = sizeof(taskStartStack);
 *        Task_construct(&taskStart, taskStartFxn, &taskParams, NULL);
 *
 *        // Start kernel (never returns)
 *        BIOS_start();
 *    }
 *
 *    // Human user interface PIN state/handle
 *    PIN_State  hStateHui;
 *    #define HUI_LED_A     PIN_ID(11)
 *    #define HUI_LED_B     PIN_ID(10)
 *    #define HUI_LED_C     PIN_ID(9)
 *    #define HUI_BUTTON_A  PIN_ID(23)
 *    #define HUI_BUTTON_B  PIN_ID(24)
 *
 *    static void taskStartFxn(UArg a0, UArg a1) {
 *        // Define pins used by Human user interface and initial configuration
 *        const PIN_Config pinListHui[] = {
 *            HUI_LED_A    | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 *            HUI_LED_B    | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 *            HUI_BUTTON_A | PIN_INPUT_EN  | PIN_PULLUP | PIN_HYSTERESIS,
 *            HUI_BUTTON_B | PIN_INPUT_EN  | PIN_PULLUP | PIN_HYSTERESIS,
 *            PIN_TERMINATE
 *        };
 *
 *        // Get handle to this collection of pins
 *        if (!PIN_open(&hStateHui, pinListHui)) {
 *            // Handle allocation error
 *        }
 *
 *        // ...
 *
 *        // We can also add (and remove) pins to a set at run time
 *        PIN_Status status = PIN_add(
 *          &hStateHui,
 *          HUI_LED_C | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 *        );
 *        if (status != PIN_SUCCESS) {
 *            // Handling allocation error is especially important with PIN_add()
 *        }
 *
 *        // ...
 *        huiDoSomething();
 *
 *        // Before ending task, make sure to deallocate pins. They will return
 *        // to the default configurations provided in PIN_init()
 *        PIN_close(&hStateHui);
 *    }
 *    @endcode
 *
 *  ## Application use of GPIO ##
 *  An example of using GPIO that builds on the previous example. Illustrates how
 *  to read input values, set output values and control output enable
 *    @code
 *    void huiDoSomething() {
 *        // Running lights on LEDs A-B-C (left to right). Button A causes left
 *        // movement, button B causes right movement, both simultaneously aborts
 *        // and disables LED output drivers
 *
 *        // LED initial state (A off, B off, C on). Only our outputs are affected
 *        PIN_setPortOutputValue(&hStateHui, (1<<HUI_LED_C));
 *
 *        int32_t moveDir = -1;    // <0: left, 0: stop, >0 right
 *        while (moveDir) {
 *            // Update LEDs
 *            if (moveDir<0) {
 *                // Left movement
 *                uint32_t t = PIN_getOutputValue(HUI_LED_A);
 *                PIN_setOutputValue(&hStateHui, HUI_LED_A, PIN_getOutputValue(HUI_LED_B));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_B, PIN_getOutputValue(HUI_LED_C));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_C, t);
 *            } else {
 *                // Right movement
 *                uint32_t t = PIN_getOutputValue(HUI_LED_C);
 *                PIN_setOutputValue(&hStateHui, HUI_LED_C, PIN_getOutputValue(HUI_LED_B));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_B, PIN_getOutputValue(HUI_LED_A));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_A, t);
 *            }
 *
 *            // Sleep for 333 ms
 *            Task_sleep(333000/10);
 *
 *            // Read input from both buttons simultaneously
 *            uint32_t buttons = PIN_getPortInputValue(&hStateHui);
 *            if (buttons&(1<<HUI_BUTTON_A) == 0) {
 *                moveDir = -1;
 *            } else if (buttons&(1<<HUI_BUTTON_A) == 0) {
 *                 moveDir = 1;
 *            } else if (buttons&((1<<HUI_BUTTON_A)|(1<<HUI_BUTTON_A))) {
 *                moveDir = 0;
 *            }
 *        }
 *        // Disable output enable for all pins (only our pins affected)
 *        PIN_setPortOutputEnable(&hStateHui, 0);
 *    }
 *    @endcode
 *
 *  ## Pin Interrupt ##
 *  An example that handles pin inputs in the GPIO example above using PIN interrupts
 *  instead:
 *    @code
 *    // volatile variable used to communicate between callback and task
 *    static volatile int32_t moveDir = -1;    // <0: left, 0: stop, >0 right
 *
 *    // Pin interrupt callback
 *    void huiPinIntCb(PIN_Handle handle, PIN_Id pinId) {
 *        // Ignore pinId and read input from both buttons simultaneously
 *        uint32_t buttons = PIN_getPortInputValue(&hStateHui);
 *        if (buttons&(1<<HUI_BUTTON_A) == 0) {
 *            moveDir = -1;
 *        } else if (buttons&(1<<HUI_BUTTON_A) == 0) {
 *             moveDir = 1;
 *        } else if (buttons&((1<<HUI_BUTTON_A)|(1<<HUI_BUTTON_A))) {
 *            moveDir = 0;
 *        }
 *    }
 *
 *    void huiDoSomething() {
 *        // Running lights on LEDs A-B-C (left to right). Button A causes left
 *        // movement, button B causes right movement, both simultaneously aborts
 *        // and disables LED output drivers
 *
 *        // LED initial state (A off, B off, C on). Only our outputs are affected
 *        PIN_setPortOutputValue(&hStateHui, (1<<HUI_LED_C));
 *        moveDir = -1;    // <0: left, 0: stop, >0 right
 *
 *        // Setup pin interrupts and register callback
 *        PIN_registerIntCb(&hStateHui, huiPinIntCb);
 *        PIN_setInterrupt(&hStateHui, HUI_BUTTON_A | PIN_IRQ_NEGEDGE);
 *        PIN_setInterrupt(&hStateHui, HUI_BUTTON_B | PIN_IRQ_NEGEDGE);
 *
 *        while (moveDir) {
 *            // Update LEDs
 *            if (moveDir<0) {
 *                // Left movement
 *                uint32_t t = PIN_getOutputValue(HUI_LED_A);
 *                PIN_setOutputValue(&hStateHui, HUI_LED_A, PIN_getOutputValue(HUI_LED_B));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_B, PIN_getOutputValue(HUI_LED_C));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_C, t);
 *            } else {
 *                // Right movement
 *                uint32_t t = PIN_getOutputValue(HUI_LED_C);
 *                PIN_setOutputValue(&hStateHui, HUI_LED_C, PIN_getOutputValue(HUI_LED_B));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_B, PIN_getOutputValue(HUI_LED_A));
 *                PIN_setOutputValue(&hStateHui, HUI_LED_A, t);
 *            }
 *
 *            // Sleep for 333 ms (we will likely go into standby)
 *            Task_sleep(333000/10);
 *        }
 *        // Disable output enable for all pins (only our pins affected)
 *        PIN_setPortOutputEnable(&hStateHui, 0);
 *        // Disable pin interrupts
 *        PIN_setInterrupt(&hStateHui, HUI_BUTTON_A | PIN_IRQ_DIS);
 *        PIN_setInterrupt(&hStateHui, HUI_BUTTON_B | PIN_IRQ_DIS);
 *    }
 *    @endcode
 *
 *******************************************************************************
 */

#ifndef ti_drivers_PIN__include
#define ti_drivers_PIN__include
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** @brief  Pin identifier data type
 *
 *  Data type used to identify a pin through an index between 0 to 254.
 *  Typically the index does not refer to the physical device pin number but
 *  rather to the index of the subset of pins that are under software-control
 *  (e.g. index 3 refers to DIO3).
 *  This data type is used as arguments in API functions to identify which pin
 *  is affected or used in lists (terminated by #PIN_TERMINATE entry) identifying
 *  multiple pins
 *  @sa PIN_ID
 */
typedef uint8_t PIN_Id;

/// Pin ID used to indicate no pin
#define PIN_UNASSIGNED              0xFF
/// Pin ID used to terminate a list of PIN_Id or PIN_Config entries
#define PIN_TERMINATE               0xFE

/** @brief  Pin configuration data type with embedded pin identifier
 *
 *  A data type used to specify I/O-pin configuration options. The lower 8b
 *  contain an embedded pin ID (see #PIN_Id) and the top 24b contain
 *  flags/fields that affect I/O configuration. #PIN_Config entries can either
 *  use a @ref PIN_GENERIC_FLAGS "set of device-independent options" or
 *  device-specific options defined in PIN driver (e.g. PINCC26XX.h), but cannot
 *  mix the two.
 *
 *  This data type is used as arguments or return values in API functions that
 *  manipulate pin configuration or used in lists (terminated by a
 *  #PIN_TERMINATE entry) for configuring multiple pins at a time.
 */
typedef uint32_t PIN_Config;

/** @brief Macro for inserting or extracting a #PIN_Id in a #PIN_Config entry
 *  @par Usage
 *       @code
 *       PIN_Config pinCfg = PIN_ID(5) | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL |
 *                           PIN_GPIO_HIGH | PIN_IRQ_POSEDGE;
 *       PIN_setConfig(hPins, PIN_BM_OUTPUT_MODE, pinCfg);
 *       // Trigger IRQ
 *       PIN_setOutputValue(hPins, PIN_ID(pinCfg), 1);
 *       @endcode
 */
#define PIN_ID(x)                 ((x)&0xFF)


/** @anchor PIN_GENERIC_FLAGS
 *  @name Generic PIN_Config flags/fields
 *  Generic (i.e. not device-specific) fields/flags for I/O configuration for
 *  use in #PIN_Config entries. All of these generic options may not be
 *  supported by the underlying device-specific PIN driver. A #PIN_Config
 *  entry may use either these generic fields/flags or device-specific ones
 *  defined in the device-specific PIN-driver, but may not mix the two.
 *
 *  The entries starting with PIN_BM_ are bitmasks used to extract individual
 *  fields obtained from PIN_getConfig() or to pass as a parameter to
 *  PIN_setConfig()to define which options it should set.
 *
 *  A star (*) in the descriptions below means the default if no option is
 *  supplied.
 *  \{
 */
#define PIN_GEN             (((uint32_t)1) << 31)   ///< Flags that generic options are used

#define PIN_INPUT_EN        (PIN_GEN | (0 << 29))   ///< (*) Enable input buffer
#define PIN_INPUT_DIS       (PIN_GEN | (1 << 29))   ///< Disable input buffer
#define PIN_HYSTERESIS      (PIN_GEN | (1 << 30))   ///< Enable input buffer hysteresis
#define PIN_NOPULL          (PIN_GEN | (0 << 13))   ///< (*) No pull-up or pull-down resistor
#define PIN_PULLUP          (PIN_GEN | (1 << 13))   ///< Pull-up resistor enabled
#define PIN_PULLDOWN        (PIN_GEN | (2 << 13))   ///< Pull-down resistor enabled
#define PIN_BM_INPUT_EN     (1 << 29)               ///< Bitmask for input enable option
#define PIN_BM_HYSTERESIS   (1 << 30)               ///< Bitmask input hysteresis option
#define PIN_BM_PULLING      (0x3 << 13)             ///< Bitmask for pull-up/pull-down options

/// Bitmask for all input mode options
#define PIN_BM_INPUT_MODE   (PIN_BM_INPUT_EN|PIN_BM_HYSTERESIS|PIN_BM_PULLING)

#define PIN_GPIO_OUTPUT_DIS (PIN_GEN | (0 << 23))   ///< (*) Disable output buffer when GPIO
#define PIN_GPIO_OUTPUT_EN  (PIN_GEN | (1 << 23))   ///< Enable output buffer when GPIO
#define PIN_GPIO_LOW        (PIN_GEN | (0 << 22))   ///< Output buffer drives to VSS when GPIO
#define PIN_GPIO_HIGH       (PIN_GEN | (1 << 22))   ///< Output buffer drives to VDD when GPIO
#define PIN_PUSHPULL        (PIN_GEN | (0 << 25))   ///< (*) Output buffer mode: push/pull
#define PIN_OPENDRAIN       (PIN_GEN | (2 << 25))   ///< Output buffer mode: open drain
#define PIN_OPENSOURCE      (PIN_GEN | (3 << 25))   ///< Output buffer mode: open source
#define PIN_SLEWCTRL        (PIN_GEN | (1 << 12))   ///< Enable output buffer slew control
#define PIN_DRVSTR_MIN      (PIN_GEN | (0x0 << 8))  ///< (*) Lowest drive strength
#define PIN_DRVSTR_MED      (PIN_GEN | (0x4 << 8))  ///< Medium drive strength
#define PIN_DRVSTR_MAX      (PIN_GEN | (0x8 << 8))  ///< Highest drive strength
#define PIN_BM_GPIO_OUTPUT_EN  (1 << 23)            ///< Bitmask for output enable option
#define PIN_BM_GPIO_OUTPUT_VAL (1 << 22)            ///< Bitmask for output value option
#define PIN_BM_OUTPUT_BUF   (0x3 << 25)             ///< Bitmask for output buffer options
#define PIN_BM_SLEWCTRL     (0x1 << 12)             ///< Bitmask for slew control options
#define PIN_BM_DRVSTR       (0xF << 8)              ///< Bitmask for drive strength options

/// Bitmask for all output mode options
#define PIN_BM_OUTPUT_MODE  (PIN_BM_GPIO_OUTPUT_VAL | PIN_BM_GPIO_OUTPUT_EN | \
                             PIN_BM_OUTPUT_BUF | PIN_BM_SLEWCTRL | PIN_BM_DRVSTR)

#define PIN_INV_INOUT       (PIN_GEN | (1 << 24))   ///< Logically invert input and output
#define PIN_BM_INV_INOUT    (1 << 24)               ///< Bitmask for input/output inversion option

#define PIN_IRQ_DIS         (PIN_GEN | (0x0 << 16)) ///< (*) Disable IRQ on pin
#define PIN_IRQ_NEGEDGE     (PIN_GEN | (0x5 << 16)) ///< Enable IRQ on negative edge
#define PIN_IRQ_POSEDGE     (PIN_GEN | (0x6 << 16)) ///< Enable IRQ on positive edge
#define PIN_IRQ_BOTHEDGES   (PIN_GEN | (0x7 << 16)) ///< Enable IRQ on both edges
#define PIN_BM_IRQ          (0x7 << 16)             ///< Bitmask for pin interrupt option

/// Bitmask for all options at once
#define PIN_BM_ALL        (PIN_BM_INPUT_MODE | PIN_BM_OUTPUT_MODE | PIN_BM_INV_INOUT | PIN_BM_IRQ)
/** \} (PIN_GENERIC_FLAGS)
 */


/** @brief Struct used to store PIN client state
 *  Pointer to a PIN_State is used as handles (#PIN_Handle) in interactions with
 *  the I/O driver
 *  @note Must reside in persistent memory
 *  @note Fields must never be modified directly
 */
typedef struct PIN_State_s PIN_State;


/** @brief A handle that is returned from a PIN_open() call
 *  Used for further PIN client interaction with the PIN driver
 */
typedef PIN_State *PIN_Handle;


/** @brief I/O Interrupt callback function pointer type
 *  One PIN Interrupt callback can be registered by each PIN client and it
 *  will be called when one of the pins allocated by the client has an interrupt
 *  event. The callback is called from HWI context with handle and pin ID as
 *  arguments.
 * @remark The callback must, as it runs in HWI context, execute and return
 *         quickly. Any lengthy operations should be performed in SWIs or tasks
 *         triggered by the callback
 */
typedef void (*PIN_IntCb)(PIN_Handle handle, PIN_Id pinId);


/** @brief underlying data structure for type #PIN_State
 */
struct PIN_State_s {
    PIN_IntCb   callbackFxn;        ///< Pointer to interrupt callback function
    uint32_t    portMask;           ///< Bitmask for pins allocated in port
    uintptr_t   userArg;            ///< User argument for whole handle
    // TODO: add driver-specific field for extensions?
};

/// @brief Return value for many functions in the PIN driver interface
typedef enum {
    PIN_SUCCESS              = 0,    ///< Operation succeeded
    PIN_ALREADY_ALLOCATED    = 1,    ///< Operation failed, some pin already allocated
    PIN_NO_ACCESS            = 2,    ///< Operation failed, client does not have access to pin
    PIN_UNSUPPORTED          = 3     ///< Operation not supported
} PIN_Status;


/** @brief  PIN module initialization
 *
 *  Must be called early in the boot sequence to ensure that I/O pins have safe
 *  configurations. This initialization sets up pins as GPIO as defined in an
 *  array (possibly user-generated) that typically resides in a board file. All
 *  pins not mentioned in aPinCfg[] are configured to be input/output/pull
 *  disabled.
 *
 *  @note Function *cannot* be called more than once.
 *
 *  @param aPinCfg[]  Pointer to array of PIN_Config entries, one per pin
 *                    that needs configuration. List terminates when a
 *                    #PIN_TERMINATE entry is encountered.
 *  @return #PIN_SUCCESS if successful, else an error code.
 */
extern PIN_Status PIN_init(const PIN_Config aPinCfg[]);


/** @brief  Allocate one or more pins for a driver or an application
 *
 *  Allows a PIN client (driver or application) to allocate a set of pins, thus
 *  ensuring that they cannot be reconfigured/controlled by anyone else. The
 *  pins are identified by and reconfigured according to the #PIN_Config
 *  entries in pinList.
 *
 *  @param state     Pointer to a PIN_State object that will hold the state for
 *                    this IO client. The object must be in persistent memory
 *  @param pinList[] Pointer to array of #PIN_Config entries, one per pin to
 *                    allocate. List terminates when #PIN_TERMINATE entry is
 *                    encountered.
 *  @return A handle for further PIN driver calls or NULL if an error occurred
 *          (already allocated pin in pinList or non-existent pin in pinList)
 */
extern PIN_Handle PIN_open(PIN_State *state, const PIN_Config pinList[]);


/** @brief  Add pin to pin set for open PIN handle
 *
 *  If the requested pin is unallocated it will be added, else an error code
 *  will be returned.
 *  @param handle   handle retrieved through an earlier call to PIN_open().
 *  @param pinCfg   Pin ID/configuration for pin to add.
 *  @return Error code if unsuccessful, else PIN_SUCCESS
 */
extern PIN_Status PIN_add(PIN_Handle handle, PIN_Config pinCfg);


/** @brief  Removes pin from pin set foropen PIN handle
 *
 *  If the requested pin is allocated to handle it will be removed from the pin
 *  set, else an error code will be returned.
 *  @param handle   handle retrieved through an earlier call to PIN_open().
 *  @param pinId    Pin ID for pin to remove.
 *  @return Error code if unsuccessful, else PIN_SUCCESS
 */
extern PIN_Status PIN_remove(PIN_Handle handle, PIN_Id pinId);


/** @brief  Deallocate all pins previously allocated with a call to PIN_open().
 *
 *  Deallocate pins allocated to handle and restore these pins to the
 *  pool of unallocated pins. Also restores the pin configuration to what it was
 *  set to when PIN_init() was called.
 *  @param handle   handle retrieved through an earlier call to PIN_open().
 */
extern void PIN_close(PIN_Handle handle);


/** @brief  Sets a user argument associated with the handle
 *
 *  Allows the application to store some data, for example a pointer to some
 *  data structure, with each PIN handle
 *  @param handle   handle retrieved through an earlier call to PIN_open().
 *  @param arg      User argument
 */
static inline void PIN_setUserArg(PIN_Handle handle, uintptr_t arg) {
    if (handle) {
        handle->userArg = arg;
    }
}


/** @brief  Gets a user argument associated with the handle
 *
 *  Allows the application to store some data, for example a pointer to some
 *  data structure, with each PIN handle
 *  @param handle   handle retrieved through an earlier call to PIN_open().
 *  @return User argument. Has the value 0 if never initialized
 */
static inline uintptr_t PIN_getUserArg(PIN_Handle handle) {
    return handle->userArg;
}


/** @name Pin Manipulation/Configuration Functions
 *  Functions that are used to manipulate the configuration of I/O pins and to
 *  get input values and set output values.
 *  \{
 */

/** @brief Get pin input value (0/1)
 *
 *  Input values of all pins are available to everyone so no handle required
 *  @param pinId ID of pin to get input value from
 *  @return      Current input buffer value
 *  @remark      This function typically has an inlined sibling function in the
 *               device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       myPin = PIN_getInputValue(PIN_ID(5));
 *       @endcode
 */
extern uint32_t PIN_getInputValue(PIN_Id pinId);


/** @brief Control output enable for GPIO pin
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param pinId   #PIN_Id entry identifying pin
 *  @param outputEnable  Enable output buffer when true, else disable
 *  @return #PIN_SUCCESS if successful, else error code
 *  @remark This function is included for consistency with the corresponding
 *          port function and to provide a more efficient/directed approach.
 *          PIN_setConfig() can be used to achieve same result.
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       PIN_setOutputEnable(hPins, PIN_ID(11), 0);
 *       @endcode
 */
extern PIN_Status PIN_setOutputEnable(PIN_Handle handle, PIN_Id pinId, bool outputEnable);


/** @brief Control output value for GPIO pin
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param pinId   Pin ID
 *  @param val     Output value (0/1)
 *  @return  #PIN_SUCCESS if successful, else error code
 *  @remark  This function typically has an inlined sibling function in the
 *           device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       PIN_setOutputValue(hPins, PIN_ID(4), 1);
 *       @endcode
 */
extern PIN_Status PIN_setOutputValue(PIN_Handle handle, PIN_Id pinId, uint32_t val);


/** @brief Get value of GPIO pin output buffer
 *
 *  Output values of all pins are available to everyone so no handle required
 *  @param pinId    Pin ID
 *  @return  Output value (0/1)
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       PIN_setOutputValue(hpins, PIN_ID(4), PIN_getOutputValue(PIN_ID(6)));
 *       @endcode
 */
extern uint32_t PIN_getOutputValue(PIN_Id pinId);


/** @brief Control interrupt enable and edge for pin
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param pinCfg  #PIN_Config entry identifying pin ID and relevant pin
 *                 configuration as combinations of:
 *                 - #PIN_IRQ_DIS      (default)
 *                 - #PIN_IRQ_POSEDGE
 *                 - #PIN_IRQ_NEGEDGE
 *                 - #PIN_IRQ_BOTHEDGES
 *  @return #PIN_SUCCESS if successful, else error code
 *  @note Any pending interrupts on pins that have not had interrupt enabled
 *        will be cleared when enabling interrupts
 *  @par Usage
 *       @code
 *       PIN_setInterrupt(hPins, PIN_ID(8)|PIN_IRQ_POSEDGE);
 *       @endcode
 */
extern PIN_Status PIN_setInterrupt(PIN_Handle handle, PIN_Config pinCfg);


/** @brief Clear pending interrupt for pin, if any
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param pinId  #PIN_Id for pin to clear pending interrupt for
 *  @return #PIN_SUCCESS if successful, else error code
 *  @par Usage
 *       @code
 *       PIN_ClrPendInterrupt(hPins, PIN_ID(8));
 *       @endcode
 */
extern PIN_Status PIN_clrPendInterrupt(PIN_Handle handle, PIN_Id pinId);


/** @brief Register callback function for a set of pins
 *
 *  Registers a callback function (see #PIN_IntCb for details) for the client
 *  identified by handle that will be called from HWI context upon an interrupt
 *  event on one or more of the allocated pins that have interrupts enabled
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param callbackFxn     Function pointer to a #PIN_IntCb function.
 *  @return #PIN_SUCCESS if successful, else error code
 *  @note Pin interrupts are serviced one at a time in pin order when
 *        simultaneous. Pin hardware interrupt flags are automatically cleared
 *        by PIN driver.
 *  @par Usage
 *       @code
 *       void pinIntHandler(PIN_Handle handle, PIN_Id pinId) {
 *           // Handle pin interrupt
 *       }
 *       ...
 *       PIN_registerIntCb(hPins, pinIntHandler);
 *       @endcode
 */
extern PIN_Status PIN_registerIntCb(PIN_Handle handle, PIN_IntCb callbackFxn);



/** @brief Returns pin configuration
 *
 *  @param pinId    Pin ID
 *  @return  Current pin configuration as a device-independent #PIN_Config value
 *  @note    The pin ID is embedded in return value.
 *  @note    There is usually a device-specific version of this function that
 *           returns device-specific options
 *  @par Usage
 *       @code
 *       // Get config of pin 14 to be able to revert later
 *       myPinConfig = PIN_getConfig(PIN_ID(14));
 *       // ...
 *       // Lots of pin reconfigurations
 *       // ...
 *       // Restore previous configuration
 *       PIN_setConfig(hPins, PIN_BM_ALL, myPinConfig);
 *       @endcode
 */
extern PIN_Config PIN_getConfig(PIN_Id pinId);


/** @brief Sets complete pin configuration
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @param updateMask  Bitmask specifying which fields in cfg that should take
 *                 effect, the rest keep their current value.
 *  @param pinCfg  #PIN_Config entry with pin ID and pin configuration
 *  @return #PIN_SUCCESS if successful, else error code
 *  @par Usage
 *       @code
 *       // Set drive strength on pin 15
 *       PIN_setConfig(hPins, PIN_BM_DRVSTR, PIN_ID(15)|PIN_DRVSTR_MAX);
 *       @endcode
 */
extern PIN_Status PIN_setConfig(PIN_Handle handle, PIN_Config updateMask, PIN_Config pinCfg);


/** \} (IO Manipulation/Configuration Functions)
 */


/** @name IO Port Functions
 *  Functions used to get input values for, set ouput values for and set output
 *  enables for multiple pins at a time. The size of so-called I/O ports that
 *  allows such multiple-pin operations are highly device dependent. In order to
 *  use the I/O port functions a set of pins that reside in the same I/O port
 *  must have been allocated previously with PIN_open().
 *  \{
 */


/** @brief Returns bitmask indicating pins allocated to client in GPIO port
 *
 *  @param handle  Handle provided by previous call to PIN_open()
 *  @return A bitmask indicating which bit positions in an I/O port the
 *          allocated I/O pins lie on, or zero if I/O port operations are not
 *          supported or the allocated pins span multiple I/O ports. The bitmask
 *          maps lowest pin index to the rightmost mask bit
 */
extern uint32_t PIN_getPortMask(PIN_Handle handle);


/** @brief Read input value of whole GPIO port
 *
 *  @param  handle  Handle provided by previous call to PIN_open()
 *  @return The simultaneous input value for the whole I/O port masked by the
 *          bit mask for the client's allocated pins
 *  @sa     PIN_getPortMask()
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 */
extern uint32_t PIN_getPortInputValue(PIN_Handle handle);


/** @brief Returns value of whole GPIO port's output buffers
 *
 *  The I/O port is identified by the pins allocated by client in a previous
 *  call to PIN_open()
 *  @param  handle  Handle provided by previous call to PIN_open()
 *  @return The current output value for whole I/O port
 *  @sa     PIN_getPortMask()
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 */
extern uint32_t PIN_getPortOutputValue(PIN_Handle handle);


/** @brief Simultaneous write output buffer values of all allocated pins in GPIO port
 *
 *  @param handle    Handle provided by previous call to PIN_open()
 *  @param outputValueMask  Bitmask indicating the desired output value for the whole
 *                   port, only the pins allocated to the client will be
 *                   affected
 *  @return #PIN_SUCCESS if successful, else error code
 *  @sa     PIN_getPortMask()
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       // Invert all pins allocated to client
 *       PIN_setPortOutputVal(hPins, ~PIN_getPortOutputVals(hPins));
 *       @endcode
 */
extern PIN_Status PIN_setPortOutputValue(PIN_Handle handle, uint32_t outputValueMask);


/** @brief Set output enable for all pins allocated to client in GPIO port
 *
 *  @param handle   Handle provided by previous call to PIN_open()
 *  @param outputEnableMask  Bitmask indicating the desired output enable configuration
 *                  for the whole port, only the pins allocated to the client
 *                  will be affected
 *  @return #PIN_SUCCESS if successful, else error code
 *  @sa      PIN_getPortMask()
 *  @remark This function typically has an inlined sibling function in the
 *          device-specific driver that may be used for higher efficiency
 *  @par Usage
 *       @code
 *       // Set output to 0 on all allocated pins, then enable the output drivers
 *       pin_setPortOutputVal(hPins, 0);
 *       pin_setPortOutputEnable(hPins, PIN_getPortMask());
 *       @endcode
 */
extern PIN_Status PIN_setPortOutputEnable(PIN_Handle handle, uint32_t outputEnableMask);


/** \} (IO Port Functions)
 */

#ifdef __cplusplus
}
#endif
#endif /* ti_drivers_PIN__include */
