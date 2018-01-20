..
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _device_drivers_pinctrl:

PINCTRL (pin controller) device driver
######################################

The **pin controller** device driver:

- enumerates controllable **pins**,
- multiplexes **pins**, and
- configures **pins**.

The Zephyr *pin controller* follows the concepts of the Linux PINCTRL
(PIN CONTROL) subsystem linux_pinctrl_. The following documentation
provides a summary of the concepts and definitions available in detail
in the Linux documentation ( linux_pinctrl_, linux_pinctrl_bindings_ ).
The Zephyr *pin controller* does not provide the full scope of the Linux
PINCTRL subsystem. Also the PINCTRL interface is specific to Zephyr and
not compatible to the Linux PINCTRL interface. However the device tree
bindings are kept compatible to the linux_pinctrl_bindings_ as much as
possible.

.. contents::
   :depth: 2
   :local:
   :backlinks: top

Pin controller concepts
***********************

A **PINCTRL device driver**
    provides access to the *pin controller*.

A **pin controller**
    is a set of hardware resources, usually accessed via registers, that
    can control *pins*. It may be able to multiplex, bias, set load capacitance,
    set drive strength, etc. for individual *pins* or *groups* of pins.
    There may be several pin controllers in a system which control only a set of
    the available *pins*.

A **pin**
    is the physical input/output line of a chip. Pins are denoted by u16_t
    integer numbers. The pin numberspace is local [#pin_number_space]_ to
    a *pin controller*. This pin space may be sparse - i.e. there may be gaps
    in the space with numbers where no pin exists.

A pins **group**
    is a set of *pins* that can be multiplexed to a
    common *function*. Groups are denoted by u16_t integer numbers. The group
    numberspace is local to each *pin controller*. The group number space may
    be sparse or even non-existing.

A pinctrl **state**
    defines the *pin configuration* and the *pin multiplexing* to a *function*
    of a *group* of *pins*. Pinctrl states are denoted by u16_t integer numbers.
    The pinctrl state number space is local to a *pin controller*.

A **function**
    is a logical or physical function block of a chip. Common functions are
    for example GPIO, USART, SPI, I2C, USB, ADC. Functions are
    denoted by u16_t integer numbers. The function number space is local to a
    *pin controller*. It may be sparse - a *pin controller* may not be able to
    multiplex (aka. connect) a *pin* or a *group* of pins to a function block.

**Pin configuration**
    allows to control the pins properties, mostly the electrical ones, by software.
    For example you may be able to make an output pin high impedance,
    or "tristate" meaning it is effectively disconnected. You may be able to connect an
    input pin to VDD or GND using a certain resistor value - pull up and pull down - so
    that the pin has a stable value when nothing is driving the rail it is connected to,
    or when it's unconnected.

**Pin multiplexing**
    allows to control the connection between *pins* and *functions* by software.

**Pin state setting**
    is the combination of *pin configuration* and *pin multiplexing* for a
    *group* of *pins*.

PINCTRL device driver
*********************

The PINCTRL device driver provides applications and other drivers access:

- to the drivers control properties sich as pins, groups, pinctrl states,
  and functions,
- to pin configuration,
- to pin multiplexing, and
- to pin state setting.

The driver also provides initial setup of pin configuration and pin multiplexing
controlled by the device tree pinctrl state information.

Application programming interface
=================================

The PINCTRL device driver API is provided by :file:`pinctrl.h`.

The application programming interface is available in two extension stages:

**Basic interface**

Pin control, and initial pin setup at startup using device tree information.

.. [PINCTRL_BASIC] Basic interface

**Extended interface with run time access to device tree information**

Extended pin control and pin setup at run time using device tree information.
The extended functionality is enabled by ``CONFIG_PINCTRL_RUNTIME_DTS``.

Implementations of the extended interface usually need more FLASH and SRAM
memory than needed by the basic interface.

.. [PINCTRL_RUNTIME_DTS] Extended interface

Control properties
==================

Pins
----

The pins are defined by the specific device driver in the associated pinctrl
device tree bindings file (e.g. :file:`include/dt-bindings/pinctrl/pinctrl_stm32.h`
for the STM32 PINCTRL device driver).

The number of pins managed by a PINCTRL device driver may be retrieved by:

:c:func:`pinctrl_get_pins_count()`
    Get the number of pins selectable by this pin controller.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

Groups
------

The groups are defined by the device tree. Every pinctrl state of an active
node sets up a group with all the pins this pinctrl state controls. There may
be additional groups not covered by an active node's pinctrl state.

The PINCTRL device driver has a mechanism for enumerating groups of pins and
retrieving the actual enumerated pins that are part of a certain group.

:c:func:`pinctrl_get_groups_count()`
    Get the number of groups selectable by this pin controller.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_group_pins()`
    Get the pins that are in a pin group.
    [PINCTRL_RUNTIME_DTS]_

The pin group associated with a certain device can be retrieved also. Example
for a group named "reset" of the device device_dev:

.. code-block:: C

    u16_t function;
    u16_t group;

    pinctrl_get_device_function(pinctrl_dev, device_dev, &function);
    pinctrl_get_function_group(pinctrl_dev, function, "reset", &group);

States
------

The states are defined by the device tree. Every pinctrl-x directive of an
active node defines a pinctrl state.

The PINCTRL device driver has a mechanism for enumerating pinctrl states and
retrieving the pin group that is associated to a certain pinctrl state.

:c:func:`pinctrl_get_states_count()`
    Get the number of states selectable by this pin controller.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_state_group()`
    Get the group of pins controlled by the pinctrl state.
    [PINCTRL_RUNTIME_DTS]_

A pinctrl state associated with a certain device can be retrieved also.
Example for a state named "reset" of the device device_dev:

.. code-block:: C

    u32_t function;
    u16_t state;

    pinctrl_get_device_function(pinctrl_dev, device_dev, &function);
    pinctrl_get_function_state(pinctrl_dev, function, "reset", &state);

Functions
---------

Functions may either denote a device or a low level hardware pinmux control.
The device functions have function numbers in the range
[0 ... (:c:macro:`PINCTRL_FUNCTION_PINMUX_BASE` - 1)]. Hardware pinmux function
numbers start at :c:macro:`PINCTRL_FUNCTION_PINMUX_BASE`.

There are two convenience functions to evaluate the type of a function:

:c:func:`pinctrl_is_device_function()`
    Is the function a client device multiplex control.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_is_pinmux_function()`
    Is the function a hardware pinmux control.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

The function number that denotes a device can be retrieved by
:c:func:`pinctrl_get_device_function()`.

The function numbers to be used for hardware pinmux control are usually defined
by the specific device driver in the associated pinctrl device tree bindings
(e.g. :file:`include/dt-bindings/pinctrl/pinctrl_{driver}.h`).

The PINCTRL device driver allows to retrieve the number of functions supported
and the states that are associated to a function. Also the groups of pins that
can be muxed to a function can be fetched.

:c:func:`pinctrl_get_functions_count()`
    Get the number of selectable functions of this pin controller.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_function_group()`
    Get the pin group with given name that can be muxed to the function.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_function_groups()`
    Get the pin groups that can be muxed to the function.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_function_state()`
    Get the pinctrl state with given name for the function.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_function_states()`
    Get the pinctrl states that can be set for the function.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_get_device_function()`
    Get the function that is associated to a device.
    [PINCTRL_RUNTIME_DTS]_

GPIO ranges
-----------

The PINCTRL device driver allows to map between the pin controller's
pin number space and the GPIO device pin number space.

:c:func:`pinctrl_get_gpio_range()`
    Get the pin controller pin number and pin mapping for a GPIO pin.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

Pin Configuration
=================

:c:func:`pinctrl_config_get()`
    Get the configuration of a pin.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_config_set()`
    Configure a pin.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_config_group_get()`
    Get the configuration of a group of pins.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_config_group_set()`
    Configure a group of pins.
    [PINCTRL_RUNTIME_DTS]_

A generic set of pin configuration properties is defined in
:file:`include/pinctrl.h`.
A pinctrl device driver may only support a subset of these
pin configuration properties.

:c:macro:`PINCTRL_CONFIG_BIAS_DISABLE`
    Disable any pin bias.

:c:macro:`PINCTRL_CONFIG_BIAS_HIGH_IMPEDANCE`
    High impedance mode ("third-state", "floating").

:c:macro:`PINCTRL_CONFIG_BIAS_BUS_HOLD`
    Latch weakly.

:c:macro:`PINCTRL_CONFIG_BIAS_PULL_UP`
    Pull up the pin.

:c:macro:`PINCTRL_CONFIG_BIAS_PULL_DOWN`
    Pull down the pin.

:c:macro:`PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT`
    Use pin default pull state.

:c:macro:`PINCTRL_CONFIG_DRIVE_PUSH_PULL`
    Drive actively high and low.

:c:macro:`PINCTRL_CONFIG_DRIVE_OPEN_DRAIN`
    Drive with open drain (open collector).
    Output can only sink current.

:c:macro:`PINCTRL_CONFIG_DRIVE_OPEN_SOURCE`
    Drive with open source (open emitter).
    Output can only source current.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT`
    Drive with default drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_1`
    Drive with minimum drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_2`
    Drive with minimum to medium drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_3`
    Drive with minimum to medium drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_4`
    Drive with medium drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_5`
    Drive with medium to maximum drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_6`
    Drive with medium to maximum drive strength.

:c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_7`
    Drive with maximum drive strength.

:c:macro:`PINCTRL_CONFIG_INPUT_ENABLE`
    Enable the pins input.
    Does not affect the pin's ability to drive output.

:c:macro:`PINCTRL_CONFIG_INPUT_DISABLE`
    Disable the pins input.
    Does not affect the pin's ability to drive output.

:c:macro:`PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE`
    Enable schmitt trigger mode for input.

:c:macro:`PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE`
    Disable schmitt trigger mode for input.

:c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_NONE`
    Do not debounce input.

:c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_SHORT`
    Debounce input with short debounce time.

:c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_MEDIUM`
    Debounce input with medium debounce time.

:c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_LONG`
    Debounce input with long debounce time.

:c:macro:`PINCTRL_CONFIG_POWER_SOURCE_DEFAULT`
    Select default power source for pin.

:c:macro:`PINCTRL_CONFIG_POWER_SOURCE_1`
    Select power source #1 for pin.

:c:macro:`PINCTRL_CONFIG_POWER_SOURCE_2`
    Select power source #2 for pin.

:c:macro:`PINCTRL_CONFIG_POWER_SOURCE_3`
    Select power source #3 for pin.

:c:macro:`PINCTRL_CONFIG_LOW_POWER_ENABLE`
    Enable low power mode.

:c:macro:`PINCTRL_CONFIG_LOW_POWER_DISABLE`
    Disable low power mode.

:c:macro:`PINCTRL_CONFIG_OUTPUT_ENABLE`
    Enable output on pin.
    Such as connecting the output buffer to the drive stage.
    Does not set the output drive.

:c:macro:`PINCTRL_CONFIG_OUTPUT_DISABLE`
    Disable output on pin.
    Such as disconnecting the output buffer from the drive stage.
    Does not reset the output drive.

:c:macro:`PINCTRL_CONFIG_OUTPUT_LOW`
    Set output to active low.
    A "1" in the output buffer drives the output to low level.

:c:macro:`PINCTRL_CONFIG_OUTPUT_HIGH`
    Set output to active high.
    A "1" in the output buffer drives the output to high level.

:c:macro:`PINCTRL_CONFIG_SLEW_RATE_SLOW`
    Select slow slew rate.

:c:macro:`PINCTRL_CONFIG_SLEW_RATE_MEDIUM`
    Select medium slew rate.

:c:macro:`PINCTRL_CONFIG_SLEW_RATE_FAST`
    Select fast slew rate.

:c:macro:`PINCTRL_CONFIG_SLEW_RATE_HIGH`
    Select high slew rate.

:c:macro:`PINCTRL_CONFIG_SPEED_SLOW`
    Select low toggle speed.

:c:macro:`PINCTRL_CONFIG_SPEED_MEDIUM`
    Select medium toggle speed.

:c:macro:`PINCTRL_CONFIG_SPEED_FAST`
    Select fast toggle speed.

:c:macro:`PINCTRL_CONFIG_SPEED_HIGH`
    Select high toggle speed.

Pin Multiplexing
================

Access to the pin multiplexer may be managed by
:c:func:`pinctrl_mux_request()` and :c:func:`pinctrl_mux_free()`.

:c:func:`pinctrl_mux_request()`
    Request a pin for muxing.
    [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_mux_free()`
    Release a pin for muxing.
    [PINCTRL_RUNTIME_DTS]_

The following multiplexing control functions do not adhere to any access
control. Use the above access management functions if access management is
necessary.

:c:func:`pinctrl_mux_get()`
    Get muxing function at pin.
    [PINCTRL_BASIC]_ [PINCTRL_RUNTIME_DTS]_

:c:func:`pinctrl_mux_set()`
    Set muxing function at pin.
    If the [PINCTRL_BASIC]_ interface is selected the muxing function is
    restricted to hardware multiplex control. In the [PINCTRL_RUNTIME_DTS]_
    interface also client device multiplex control is available.

:c:func:`pinctrl_mux_group_set()`
    Set muxing function for a group of pins.
    [PINCTRL_RUNTIME_DTS]_


Pin State Setting and Initialization
====================================

:c:func:`pinctrl_state_set()`
    Set pinctrl state.
    In the [PINCTRL_BASIC]_ interface the state is
    restricted to the "default" state used for pin setup at startup.
    If the [PINCTRL_RUNTIME_DTS]_ interface is selected all states
    known in the device tree information can be set.

The pinctrl device driver initializes pins based on device tree information.
Pins that are controlled by client devices that are

    - activated nodes in the device tree (status != "disabled")
    - and where a "default" pin control is given

are initialized during driver initialization.

.. code-block:: DTS

    pinctrl-0 = <&xxx_pins_a &xxxx_pins_b>;
    pinctrl-names = "default";
    status = "ok";


Pin Concurrent Use by a Function and GPIO device driver
=======================================================

The GPIO device driver is just another client to the PINCTRL device driver.


Interaction with GPIO device driver
===================================

The GPIO device driver uses the PINCTRL device driver as a backend.


Interaction with PINMUX device driver
=====================================

The PINCTRL device driver also provides the PINMUX interface. No
extra pinmux device is needed.

The PINMUX interface of the PINCTRL driver is for transition. The
PINCTRL multiplexing and configuration interfaces shall be used
instead.


PINCTRL device tree support
***************************

Nodes
=====

Pin Controller Node
-------------------

The pin controller device is defined by a pin controller node.

Required properties are:

    - pin-controller:
    - compatible:
    - label:


Example:

.. code-block:: DTS

    pinctrl: pin-controller@48000000 {
        pin-controller;
        compatible = "st,stm32-pinctrl";
        label = "PINCTRL";

        /* pinctrl state nodes... */
    };

Pinctrl State Node
------------------

Pinctrl states are defined by pinctrl state nodes. The pinctrl state nodes are
sub-nodes of the pin controller node. A pinctrl state node represents a group
of pins and how they should be configured.

A pinctrl state node is of the following format:

.. code-block:: text

    [pinctrl-state-label]: [device-name]@[device-configuration-index] {
        [pin-configuration-label] {
                [property] = [value];
                ...
        };
        ...
    };

The device tree information is used to generate C defines:

.. code-block:: C

    #define [NODE-LABEL]_PINCTRL_[PINCTRL-NAME]_[PIN-CONFIGURATION-LABEL]_[PROPERTY] [value]

The following pin configuration properties are supported by the device tree.
*Pin controller* device drivers usually only support a subset of these
pin configuration properties.

**pins**:
    The **pins** property denotes to which pins the configuration applies to.
    The *pins* are specified as a list of pin numbers from the *pin controller*
    [#pin_number_space]_.
    Either the *pins* or the *group* or the *pinmux* property shall be given
    for a pin configuration.

**group**:
    The **group** property denotes to which *group* of *pins* the configuration
    applies to. The *group* is specified as a group number from the
    *pin controller* group number space.
    Either the *pins* or the *group* or the *pinmux* property shall be given
    for a pin configuration.

**pinmux**:
    The **pinmux** property denotes to which *pins* the configuration applies to
    and defines the *pin multiplexing* of these *pins* at the same time. The
    *pinmux* is specified as a list of integer values. The interpretation of the
    integer value(s) is specific to the *pin controller* device driver.
    Either the *pins* or the *group* or the *pinmux* property shall be given
    for a pin configuration.

**bias-disable**
    Disable any pin bias.

**bias-high-impedance**
    High impedance mode ("third-state", "floating").

**bias-bus-hold**
    Latch weakly.

**bias-pull-up**
    Pull up the pin.

**bias-pull-down**
    Pull down the pin.

**bias-pull-pin-default**
    Use pin default pull state.

**drive-push-pull**
    Drive actively high and low.

**drive-open-drain**
    Drive with open drain (open collector).

**drive-open-source**
    Drive with open source (open emitter).

**drive-strength**
    Drive strength. Generic drive strength values are:

    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_1`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_2`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_3`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_4`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_5`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_6`,
    - :c:macro:`PINCTRL_CONFIG_DRIVE_STRENGTH_7`.

**input-enable**
    Enable the pins input. Does not affect the pin's ability to drive output.

**input-disable**
    Disable the pins input. Does not affect the pin's ability to drive output.

**input-schmitt-enable**
    Enable schmitt trigger mode for input.

**input-schmitt-disable**
    Disable schmitt trigger mode for input.

**input-debounce**
    Debounce input. Generic debounce values are:

    - :c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_NONE`
    - :c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_SHORT`
    - :c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_MEDIUM`
    - :c:macro:`PINCTRL_CONFIG_INPUT_DEBOUNCE_LONG`

**low-power-enable**
    Enable low power mode.

**low-power-disable**
    Disable low power mode.

**output-disable**
    Disable output on pin. Such as disconnecting the output buffer from the drive stage.
    Does not reset the output drive.

**output-enable**
    Enable output on pin. Such as connecting the output buffer to the drive stage.
    Does not set the output drive.

**output-low**
    Set output to active low. A "1" in the output buffer drives the output to low level.

**output-high**
    Set output to active high. A "1" in the output buffer drives the output to high level.

**power-source**
    Power source. Generic power source values are:

    - :c:macro:`PINCTRL_CONFIG_POWER_SOURCE_DEFAULT`
    - :c:macro:`PINCTRL_CONFIG_POWER_SOURCE_1`
    - :c:macro:`PINCTRL_CONFIG_POWER_SOURCE_2`
    - :c:macro:`PINCTRL_CONFIG_POWER_SOURCE_3`

**slew-rate**
    Slew rate. Generic slew rate values are:

    - :c:macro:`PINCTRL_CONFIG_SLEW_RATE_SLOW`
    - :c:macro:`PINCTRL_CONFIG_SLEW_RATE_MEDIUM`
    - :c:macro:`PINCTRL_CONFIG_SLEW_RATE_FAST`
    - :c:macro:`PINCTRL_CONFIG_SLEW_RATE_HIGH`

**speed**
    Toggle speed. Slew rate may be unaffected.
    Generic toggle speed values are:

    - :c:macro:`PINCTRL_CONFIG_SPEED_SLOW`
    - :c:macro:`PINCTRL_CONFIG_SPEED_MEDIUM`
    - :c:macro:`PINCTRL_CONFIG_SPEED_FAST`
    - :c:macro:`PINCTRL_CONFIG_SPEED_HIGH`

The following example device tree describes a pin controller with a pin
configuration node and a SPI.

.. code-block:: DTS

    / {
        /* ... */
        soc {
            /* ... */
            pinctrl: pin-controller@48000000 {
                compatible = "st,stm32-pinctrl";
                pin-controller;
                #address-cells = <1>;
                #size-cells = <1>;
                reg = <0x48000000 0x1800>;
                label = "PINCTRL";
                /* ... */
                spi1_master_a: spi1@0 {
                    sck {
                        pinmux = <PINCTRL_STM32_PINB3 PINCTRL_STM32_FUNCTION_ALT_0>;
                        drive-push-pull;
                        bias-disable;
                    };
                    miso {
                        pinmux = <PINCTRL_STM32_PINB4 PINCTRL_STM32_FUNCTION_ALT_0>;
                        bias-disable;
                    };
                    mosi {
                        pinmux = <PINCTRL_STM32_PINB5 PINCTRL_STM32_FUNCTION_ALT_0>;
                        drive-push-pull;
                        bias-disable;
                    };
                };
            /* ... */
            };
            /* ... */
            spi1: spi@40013000 {
                spi-controller;
                compatible = "st,stm32-spi-fifo";
                #address-cells = <1>;
                #size-cells = <0>;
                reg = <0x40013000 0x400>;
                interrupts = <25 5>;
                status = "disabled";
                label = "SPI_1";
            };
        };
    };
    /* ... */
    &pinctrl {
        status = "ok";
    };

    &spi1 {
        pinctrl-0 = <&spi1_master_a>;
        pinctrl-names = "default";
        status = "ok";
    };

The device tree information is extracted to :file:`generated_dts_board.h`.

The SPI_1 is registered as FUNCTION_2 at the pin controller.
The "default" state of the SPI is registered as STATE_2. Three pin controls
are associated to this state: PINCTRL_4, PINCTRL_5 and PINCTRL_6.
PINCTRL_4 e.g. configures the pin sck.

.. code-block:: C
    :caption: Example generated_dts_board.h - pin controller node

    /* pin-controller@48000000 */
    #define ST_STM32_PINCTRL_48000000_BASE_ADDRESS_0                    0x48000000
    ...
    #define ST_STM32_PINCTRL_48000000_FUNCTION_2_CLIENT                 ST_STM32_SPI_FIFO_40013000
    ...
    #define ST_STM32_PINCTRL_48000000_FUNCTION_COUNT                    4
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_BUS_HOLD           0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_DISABLE            1
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_HIGH_IMPEDANCE     0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_PULL_DOWN          0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_PULL_PIN_DEFAULT   0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_BIAS_PULL_UP            0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_DRIVE_OPEN_DRAIN        0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_DRIVE_OPEN_SOURCE       0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_DRIVE_PUSH_PULL         1
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_DRIVE_STRENGTH          0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_GROUP                   0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_INPUT_DEBOUNCE          0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_INPUT_DISABLE           0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_INPUT_ENABLE            0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_INPUT_SCHMITT_DISABLE   0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_INPUT_SCHMITT_ENABLE    0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_LOW_POWER_DISABLE       0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_LOW_POWER_ENABLE        0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_OUTPUT_DISABLE          0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_OUTPUT_ENABLE           0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_OUTPUT_HIGH             0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_OUTPUT_LOW              0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_PINMUX                  19,32768
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_PINS                    0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_POWER_SOURCE            0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_SLEW_RATE               0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_SPEED                   0
    #define ST_STM32_PINCTRL_48000000_PINCTRL_4_STATE_ID                2
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_5_STATE_ID                2
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_6_STATE_ID                2
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_COUNT                     10
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_STATE_2_FUNCTION_ID       2
    #define ST_STM32_PINCTRL_48000000_PINCTRL_STATE_2_NAME_ID           0
    ...
    #define ST_STM32_PINCTRL_48000000_PINCTRL_STATE_COUNT               4
    #define ST_STM32_PINCTRL_48000000_SIZE_0                            6144
    #define ST_STM32_PINCTRL_48000000_STATE_NAME_0                      default
    #define ST_STM32_PINCTRL_48000000_STATE_NAME_COUNT                  1
    #define ST_STM32_PINCTRL_48000000_BASE_ADDRESS                      ST_STM32_PINCTRL_48000000_BASE_ADDRESS_0
    #define ST_STM32_PINCTRL_48000000_SIZE                              ST_STM32_PINCTRL_48000000_SIZE_0

The relevant pin control information is also given for the SPI node.

.. code-block:: C
    :caption: Example generated_dts_board.h - SPI node

    /* spi@40013000 */
    #define ST_STM32_SPI_FIFO_40013000_BASE_ADDRESS_0                       0x40013000
    #define ST_STM32_SPI_FIFO_40013000_BASE_ADDRESS_1                       0x400
    #define ST_STM32_SPI_FIFO_40013000_IRQ_0                                25
    #define ST_STM32_SPI_FIFO_40013000_IRQ_0_PRIORITY                       5
    #define ST_STM32_SPI_FIFO_40013000_LABEL                                "SPI_1"
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_CONTROLLER_0_CONTROLLER      ST_STM32_PINCTRL_48000000
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_CONTROLLER_0_FUNCTION_ID     2
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_CONTROLLER_COUNT             1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_CONTROLLER_ID        0
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_MISO_BIAS_DISABLE    1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_MISO_PINMUX          20,32768
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_MOSI_BIAS_DISABLE    1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_MOSI_DRIVE_PUSH_PULL 1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_MOSI_PINMUX          21,32768
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_SCK_BIAS_DISABLE     1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_SCK_DRIVE_PUSH_PULL  1
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_SCK_PINMUX           19,32768
    #define ST_STM32_SPI_FIFO_40013000_PINCTRL_DEFAULT_STATE_ID             2
    #define ST_STM32_SPI_FIFO_40013000_BASE_ADDRESS                         ST_STM32_SPI_FIFO_40013000_BASE_ADDRESS_0

Also generic device information is available for the soc node.

.. code-block:: C
    :caption: Example generated_dts_board.h - soc node

    /* soc */
    #define SOC_PIN_CONTROLLER_COUNT    1
    #define SOC_PIN_CONTROLLER_0        ST_STM32_PINCTRL_48000000
    #define SOC_SPI_CONTROLLER_COUNT    1
    #define SOC_SPI_CONTROLLER_0        ST_STM32_SPI_FIFO_40013000

Pin Initialization
==================

Pin initialization is defined by the pinctrl named "default".

In the following example pinctrl-0 is named "default". All pin states
requested by pinctrl-0 (pina_state0_node_other and pinb_state0_node_other)
will be set at initialization time.

.. code-block:: DTS

    pinctrl: pinctrl@48000000 {
        pin-controller;
        /* ... */

        pina_state0_node_other {
            /* ... */
        };
        pina_state1_node_other {
            /* ... */
        };
        pinb_state0_node_other {
            /* ... */
        };
        pinb_state1_node_other {
            /* ... */
        };
    };

    other: other@50000000 {
        /* ... */
        pinctrl-0 = <&pina_state0_node_other, &pinb_state0_node_other>;
        pinctrl-1 = <&pina_state1_node_other, &pinb_state1_node_other>;
        pinctrl-names = "default", "xxxx";
        status = "ok";
    };

Interaction with the GPIO device driver
=======================================

GPIO to PINCTRL pin mapping by pinctrl-x
----------------------------------------

The GPIO device as a client device to the pin-controller may have at least
one pinctrl-x directive that references the pin-controller and sets all port
pins to GPIO input. By this all pin-controller pins that are associated to
the GPIO port can be retrieved by :c:func:`pinctrl_get_group_pins`. The name
of the group to retrieve corresponds to the pinctrl name of the pinctrl-x
directive.

.. code-block:: DTS

    pinctrl: pin-controller@48000000 {
        pin-controller;
        /* ... */

        gpioa_in: gpioa@0 {
            pin0: {
                pinmux = <...>;
                /* ... */
            }
            /* ... */
        }
    };

    gpioa: gpio@40020000 {
        gpio-controller;
        /* ... */
        pinctrl-0 = <&gpioa_in>;
        pinctrl-names = <"reset", ...>;
    };

A gpio driver may retrieve the pin-controller pins associate to it´s port:

.. code-block:: C

    u32_t function;
    u32_t group;
    u16_t pins[16];
    u16_t num_pins = 16;

    pinctrl_get_device_function(pinctrl_dev, gpio_dev, &function);
    pinctrl_get_function_group(pinctrl_dev, function, "reset", &group);
    pinctrl_get_group_pins(pinctrl_dev, group, &pins[0], &num_pins);

GPIO to PINCTRL pin mapping by gpio-ranges
------------------------------------------

To enable a pin controller and/ or a GPIO port to retrieve the pin mapping
between them the gpio-ranges property may be set in the device tree. Also
the label property shall be set and used as device name in this case.

The gpio-ranges property is of the form:

.. code-block:: text

    gpio-ranges = <&[pin controller] [pin number in GPIO pin number space]
                                     [pin number in pin controller number space]
                                     [number of pins]>,
                                     /* ... */
                                     ;

A GPIO port definition in the device tree may look like:

.. code-block:: DTS

    pinctrl: pin-controller@48000000 {
        pin-controller;
        /* ... */
    };

    gpioa: gpio@48001000 {
        gpio-controller;
        compatible = "st,stm32-gpio-pinctrl";
        #gpio-cells = <2>;
        reg = <0x480010000 0x400>;
        label = "GPIOE";
        gpio-ranges = <&pinctrl GPIO_PORT_PIN0 PINCTRL_STM32_PINE0 16>;
    };

The GPIO port pin 0 is mapped to the pinctrl pin E0. In total 16 pins are
mapped starting at pinctrl pin EO and continuing with the consecutively
following pin numbers.

GPIO_RANGE defines are generated for the GPIO node and for the PINCTRL
node.

.. code-block:: C

    /* gpio@48001000 */
    #define ST_STM32_GPIO_PINCTRL_48001000_BASE_ADDRESS_0               0x48001000
    #define ST_STM32_GPIO_PINCTRL_48001000_GPIO_RANGE_0_BASE            1
    #define ST_STM32_GPIO_PINCTRL_48001000_GPIO_RANGE_0_CONTROLLER      ST_STM32_PINCTRL_48000000
    #define ST_STM32_GPIO_PINCTRL_48001000_GPIO_RANGE_0_CONTROLLER_BASE 64
    #define ST_STM32_GPIO_PINCTRL_48001000_GPIO_RANGE_0_NPINS           16
    #define ST_STM32_GPIO_PINCTRL_48001000_GPIO_RANGE_COUNT             1
    #define ST_STM32_GPIO_PINCTRL_48001000_LABEL                        "GPIOE"
    #define ST_STM32_GPIO_PINCTRL_48001000_SIZE_0                       1024
    #define ST_STM32_GPIO_PINCTRL_48001000_BASE_ADDRESS                 ST_STM32_GPIO_PINCTRL_48001000_BASE_ADDRESS_0
    #define ST_STM32_GPIO_PINCTRL_48001000_SIZE                         ST_STM32_GPIO_PINCTRL_48001000_SIZE_0

.. code-block:: C

    /* pin-controller@48000000 */
    #define ST_STM32_PINCTRL_48000000_BASE_ADDRESS_0            0x48000000
    ...
    #define ST_STM32_PINCTRL_48000000_GPIO_RANGE_4_BASE         64
    #define ST_STM32_PINCTRL_48000000_GPIO_RANGE_4_CLIENT       ST_STM32_GPIO_PINCTRL_48001000
    #define ST_STM32_PINCTRL_48000000_GPIO_RANGE_4_CLIENT_BASE  1
    #define ST_STM32_PINCTRL_48000000_GPIO_RANGE_4_NPINS        16
    ...
    #define ST_STM32_PINCTRL_48000000_GPIO_RANGE_COUNT          6
    ...

Interaction with the PINMUX device driver
=========================================

The pinmux device shall be disabled if a pin controller is used.

.. code-block:: DTS

    pinctrl: pin-controller@48000000 {
        pin-controller;
        /* ... */
        status = "ok";
    };

    pinmux: pinmux@48000400 {
        /* ... */
        status = "disabled";
    };


PINCTRL device driver implementation
************************************

Driver Configuration
====================

Driver configuration information comes from three sources:
    - autoconf.h (Kconfig)
    - generated_dts_board.h (device tree)
    - soc.h

Driver configuration by autoconf.h
----------------------------------

See :ref:`configuration` for the the full set of Kconfig symbols that are available.
Specific to the pin controller driver are:

:c:macro:`CONFIG_PINCTRL`
    Enable pinctrl driver.

:c:macro:`CONFIG_PINCTRL_RUNTIME_DTS`
    Enable run time pin configuration control based on device tree
    information. When enabled the [PINCTRL_RUNTIME_DTS]_ API is available.

:c:macro:`CONFIG_PINCTRL_PINMUX`
    Enable the generic shim that maps from the pinmux interface to the pinctrl
    interface.

:c:macro:`CONFIG_PINCTRL_INIT_PRIORITY`
    Initialization priority for the pinctrl driver.

:c:macro:`CONFIG_PINCTRL_NAME`
    Pinctrl driver name. May be used if no label information is given in the device
    tree.

Driver configuration by device tree
-----------------------------------

For a pin controller node the following information is extracted from the device tree
and provided in :file:`generated_dts_board.h`.

The general device information for pin controllers:

:c:macro:`SOC_PIN_CONTROLLER_COUNT`
    Number of pin controllers that are activated. All nodes that contain a
    pin-controller directive are counted.

:c:macro:`SOC_PIN_CONTROLLER_0` ...
    The label prefix of pin controller 0 ... (e.g. 'ST_STM32_PINCTRL_48000000').
    The define is generated if a pin-controller directive is given in the node.

General information that is extracted for every device type:

**LABEL**

**BASE_ADDRESS**

**SIZE**

**COMPATIBLE_COUNT**

**COMPATIBLE_0_ID**

The specific device information for pin controllers:

**FUNCTION**
    For all active client nodes that reference a pin-controller by
    pinctrl-x(&pinctrl, ...) FUNCTION defines are generated.
    The function equals the client node.

**STATE_NAME**
    For all pinctrl names of pinctrl-x directives of active client nodes
    STATE_NAME defines are generated. This is mostly to allow deduplication of
    commonly used state names such as e.g. "default".

**PINCTRL_STATE**
    For all pinctrl-x directives of active client nodes PINCTRL_STATE defines
    are generated.
    The pinctrl state references the *FUNCTION* (aka. node) it is given in.
    The pinctrl state also references the *STATE_NAME* given in the
    pinctrl-names directive associated to the pinctrl-x directive.

    All the pins that a pinctrl state controls are regarded to be a pin group.
    The name of the pin group is the same as the name of the pinctrl state.

**PINCTRL**
    For all subnodes of pin configurations referenced by active client nodes -
    pinctrl-x(&xxx, pinconf_a, ...) - PINCTRL defines are generated.
    The pinctrl references the *FUNCTION* (aka. node) it is referenced from.
    The pinctrl also references the *PINCTRL_STATE* it is referenced from.

**GPIO_RANGE**
    For all gpio-ranges directives of active GPIO nodes GPIO_RANGE defines are
    generated.

The datasets for the clients of pin controllers are:

**PINCTRL_CONTROLLER**
    For all pin controllers that are referenced in a pinctrl-x directive
    PINCTRL_CONTROLLER defines are generated.

**PINCTRL**
    For all subnodes of pin configurations referenced by active client nodes
    - pinctrl-x(&xxx, pinconf_a, ...) - PINCTRL defines are generated.
    The pinctrl references the *PINCTRL_CONTROLLER* given in the pinctrl-x
    directive. The pinctrl also references the *PINCTRL_STATE* the pin
    controller associates to this pinctrl.

Driver configuration by soc.h
-----------------------------

:file:`soc.h` may contain driver relevant information such as HAL header files.

Driver structures and content generation
========================================

A driver implementation may either use a specific (DTS) script or the
inline code generation to generate driver structures and content.

Using the pin controller driver template
----------------------------------------

The pin controller template :file:`pinctrl_tmpl.c` is a template for pin
controller driver implementation. The template parameters are given by
global variables in the inline code generation snippet.
The template is expanded by inline code generation with Python.
The template generates the appropriate data structures
and fills them using the information provided in :file:`generated_dts_board.conf`.

A pin controller driver implementer has to write five hardware/ device specific
functions to implement:

    - :c:func:`pinctrl_config_get()`,
    - :c:func:`pinctrl_config_set()`,
    - :c:func:`pinctrl_mux_get()`,
    - :c:func:`pinctrl_mux_set()`,
    - and device initialization.

Additionally two Python functions have to be provided that extract the pin number
and the multiplexer value from the pinmux value(s) given in the device tree:

    - :py:func:`pinmux_pin(pinmux)`
    - :py:func:`pinmux_mux(pinmux)`

The template expects the following globals to be set:

    - compatible
      The compatible string of the driver (e.g. 'st,stm32-pinctrl')
    - data_info
      device data type definition (e.g. 'struct pinctrl_stm32_data')
    - config_get
      C function name of device config_get function.
    - mux_free
      C function name of device mux_free function.
    - mux_get
      C function name of device mux_get function.
    - mux_set
      C function name of device mux_set function.
    - device_init
      C function name of device init function
    - :py:func:`pinmux_pin(pinmux)`
      Python function that extracts the pin number from the pinmux property.
    - :py:func:`pinmux_mux(pinmux)`
      Python function that extracts the mux value from the pinmux property.

The structure of a pin controller driver may look like this example
of the STM32 driver (:file:`drivers/pinctrl/pinctrl_stm32.c`):

.. code-block:: C
    :caption: Example pinctrl_stm32.c

    /**
     * @code{.codegen}
     * compatible = 'st,stm32-pinctrl'
     * codegen.if_device_tree_controller_compatible('PIN', compatible)
     * @endcode{.codegen}
     */
    /** @code{.codeins}@endcode */
    ...
    struct pinctrl_stm32_data {
       ...
    }
    ...
    /* --- API (using the PINCTRL template code) --- */

    /* forward declarations */
    static int pinctrl_stm32_config_get(struct device *dev, u16_t pin,
                                        u32_t *config);
    static int pinctrl_stm32_config_set(struct device *dev, u16_t pin,
                                        u32_t config);
    static int pinctrl_stm32_mux_get(struct device *dev, u16_t pin, u16_t *func);
    static int pinctrl_stm32_mux_set(struct device *dev, u16_t pin, u16_t func);
    static int pinctrl_stm32_device_init(struct device *dev);

    /**
     * @code{.codegen}
     * # compatible already set
     * data_info = 'struct pinctrl_stm32_data'
     * config_get = 'pinctrl_stm32_config_get'
     * config_set = 'pinctrl_stm32_config_set'
     * mux_get = 'pinctrl_stm32_mux_get'
     * mux_set = 'pinctrl_stm32_mux_set'
     * device_init = 'pinctrl_stm32_device_init'
     * def pinmux_pin(pinmux):
     *     return pinmux.split(',')[0]
     * def pinmux_mux(pinmux):
     *     return pinmux.split(',')[1]
     * codegen.out_include('pinctrl_tmpl.c')
     * @endcode{.codegen}
     */
    /** @code{.codeins}@endcode */

    static int pinctrl_stm32_config_get(struct device *dev, u16_t pin,
                                        u32_t *config)
    { ... }

    static int pinctrl_stm32_config_set(struct device *dev, u16_t pin,
                                        u32_t config)
    { ... }

    static int pinctrl_stm32_mux_get(struct device *dev, u16_t pin, u16_t *func)
    { ... }

    static int pinctrl_stm32_mux_set(struct device *dev, u16_t pin, u16_t func)
    { ... }

    static int pinctrl_stm32_device_init(struct device *dev)
    { ... }

Design Rationales
*****************

.. [#pin_number_space] PINCTRL uses a **local pin number space** instead of
   a global space to allow for object encapsulation.

References
**********

.. target-notes::

.. _linux_pinctrl: https://www.kernel.org/doc/Documentation/pinctrl.txt
.. _linux_pinctrl_bindings: https://www.kernel.org/doc/Documentation/devicetree/bindings/pinctrl/pinctrl-bindings.txt
