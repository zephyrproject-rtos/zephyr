..
    Copyright (c) 2017 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _device_driver_pinctrl_stm32:

STM32 PINCTRL (Pin Controller) Device Driver
############################################

.. contents::
   :depth: 1
   :local:
   :backlinks: top

STMicroelectronics's STM32 MCUs intregrate a GPIO and Pin mux/config
hardware controller. It controls the input/output settings on the
available pins and also provides the ability to multiplex and configure
the output of various on-chip controllers onto these pads.

Control Properties
******************

Applications and other device drivers may access the control properties of the
STM32 PINCTRL device driver by the following files:

  - Pins: :file:`include/dt-bindings/pinctrl/pinctrl_stm32.h`
  - Groups: :file:`include/dt-bindings/pinctrl/pinctrl_stm32.h`
  - Functions: :file:`include/dt-bindings/pinctrl/pinctrl_stm32.h`

There are no groups, and functions for groups defined.

There are pin defines available for all port pins [PINCTRL_STM32_PINA0 ...
PINCTRL_STM32_PINA15] to [PINCTRL_STM32_PINH0 ... PINCTRL_STM32_PINH15].

Pin Configuration
*****************

The pin configuration options supported by the STM32 PINCTRL device driver are
a subset of the generic configuration options given in
:file:`include/pinctrl_common.h`.

+---------------------------------------+---------+--------------------------+
| Property                              | Support | Comment                  |
+=======================================+=========+==========================+
| PINCTRL_CONFIG_BIAS_DISABLE           | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_BIAS_HIGH_IMPEDANCE    | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_BIAS_BUS_HOLD          | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_BIAS_PULL_UP           | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_BIAS_PULL_DOWN         | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT  | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_PUSH_PULL        | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_OPEN_DRAIN       | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_OPEN_SOURCE      | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_1       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_2       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_3       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_4       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_5       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_6       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_DRIVE_STRENGTH_7       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_ENABLE           | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_DISABLE          | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE   | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE  | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_DEBOUNCE_NONE    | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_DEBOUNCE_SHORT   | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_DEBOUNCE_MEDIUM  | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_INPUT_DEBOUNCE_LONG    | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_POWER_SOURCE_DEFAULT   | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_POWER_SOURCE_1         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_POWER_SOURCE_2         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_POWER_SOURCE_3         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_LOW_POWER_ENABLE       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_LOW_POWER_DISABLE      | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_OUTPUT_ENABLE          | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_OUTPUT_DISABLE         | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_OUTPUT_LOW             | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_OUTPUT_HIGH            | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SLEW_RATE_SLOW         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SLEW_RATE_MEDIUM       | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SLEW_RATE_FAST         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SLEW_RATE_HIGH         | no      |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SPEED_SLOW             | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SPEED_MEDIUM           | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SPEED_FAST             | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_CONFIG_SPEED_HIGH             | **yes** |                          |
+---------------------------------------+---------+--------------------------+

Pin Multiplexing
****************

The STM32 PINCTRL driver supports pin multiplex control by device function and
by hardware pinmux function. The hardware pinmux functions are defined in
:file:`include/dt-bindings/pinctrl/pinctrl_stm32.h`.

+---------------------------------------+---------+--------------------------+
| Pinmux Function                       | Support | Comment                  |
+=======================================+=========+==========================+
| PINCTRL_STM32_FUNCTION_ALT_0          | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| ...                                   |         |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_STM32_FUNCTION_ALT_15         | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_STM32_FUNCTION_INPUT          | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_STM32_FUNCTION_OUTPUT         | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| PINCTRL_STM32_FUNCTION_ANALOG         | **yes** |                          |
+---------------------------------------+---------+--------------------------+

Pin Initialization
******************

The STM32 PINCTRL driver supports pin initialization by pinctrls named
"default".

Device Tree Support
*******************

Nodes
=====

The STM32 PINCTRL device driver is based on and supports the following
device tree nodes:

Pin controller node
-------------------

Required properies:
    - compatible: value must be: "st,stm32-pinctrl"
    - #address-cells: The value of this property must be 1
    - #size-cells: The value of this property must be 1
    - label: Should be a name string

.. code-block:: DTS

	pinctrl: pin-controller@40020000 {
		pin-controller;
		compatible = "st,stm32-pinctrl";
		label = "PINCTRL";
		#address-cells = <1>;
		#size-cells = <1>;
		reg = <0x40020000 0x3000>;

		/* pin configuration nodes follow... */
	};

GPIO controller node
--------------------

Required properties:

    - gpio-ranges: Defines a dedicated mapping between a pin-controller and
                   a gpio controller. Format is <&phandle a b c> with:

        - (phandle): phandle of pin-controller.
        - (a): gpio port pin base in range.
        - (b): pin-controller pin base in range.
        - (c): gpio port pin count in range

.. code-block:: DTS

    pinctrl: pin-controller@40020000 {
        pin-controller;
        /* ... */
    };

    gpioa: gpio@40020000 {
        gpio-controller;
        /* ... */
        gpio-ranges = <&pinctrl GPIO_PORT_PIN0  PINCTRL_STM32_PINA0 16>;
    };


Device nodes
------------

Required properties:

    - pinctrl-0: Pin control definition for pinctrl state 0.
    - pinctrl-names: Names of the pinctrl states.
    - label: Should be the name string used in DEVICE_INIT().

Optional properties:

    - pinctrl-1,2,...: Pin control definition for pinctrl state 1 or 2 or ....

Pinctrl state nodes
-------------------

The STM32 PINCTRL device driver recognizes the following pin configuration
properties given in the device tree:

+---------------------------------------+---------+--------------------------+
| Property                              | Support | Comment                  |
+=======================================+=========+==========================+
| pins                                  | no      |                          |
+---------------------------------------+---------+--------------------------+
| group                                 | no      |                          |
+---------------------------------------+---------+--------------------------+
| pinmux                                | **yes** | see _`pinmux`            |
+---------------------------------------+---------+--------------------------+
| bias-disable                          | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| bias-high-impedance                   | no      |                          |
+---------------------------------------+---------+--------------------------+
| bias-pull-up                          | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| bias-pull-down                        | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| bias-pull-pin-default                 | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| drive-push-pull                       | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| drive-open-drain                      | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| drive-open-source                     | no      |                          |
+---------------------------------------+---------+--------------------------+
| drive-strength                        | no      |                          |
+---------------------------------------+---------+--------------------------+
| input-enable                          | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| input-disable                         | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| input-schmitt-enable                  | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| input-schmitt-disable                 | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| input-debounce                        | no      |                          |
+---------------------------------------+---------+--------------------------+
| low-power-enable                      | no      |                          |
+---------------------------------------+---------+--------------------------+
| low-power-disable                     | no      |                          |
+---------------------------------------+---------+--------------------------+
| output-disable                        | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| output-enable                         | partial | set by mux function only |
+---------------------------------------+---------+--------------------------+
| output-low                            | no      |                          |
+---------------------------------------+---------+--------------------------+
| output-high                           | no      |                          |
+---------------------------------------+---------+--------------------------+
| power-source                          | no      |                          |
+---------------------------------------+---------+--------------------------+
| slew-rate                             | no      |                          |
+---------------------------------------+---------+--------------------------+
| speed                                 | **yes** |                          |
+---------------------------------------+---------+--------------------------+

pinmux
------

The pinmux property value must be given by the tuple pin-number and
pin-function.

.. code-block:: text

    [pin-configuration-label] {
        pinmux = <[pin-number] [pin-function]>;
        ...
    };

Example of pin 6 and 7 of GPIO port B multiplexed to alternate function 0:

.. code-block:: DTS

    pin-controller {
        /* ... */
        usart1_pins_a: usart1@0 {
            rx {
                pinmux = <PINCTRL_STM32_PINB7 PINCTRL_STM32_FUNCTION_ALT_0>;
                bias-disable;
            };
            tx {
                pinmux = <PINCTRL_STM32_PINB6 PINCTRL_STM32_FUNCTION_ALT_0>;
                drive-push-pull;
                bias-disable;
            };
        };
    };

    &usart1 {
        pinctrl-0 = <&usart1_pins_a>;
        pinctrl-names = "default";
        status = "okay";
    };

STM32 PINCTRL Implementation
****************************

Pins
====

.. doxygengroup:: pinctrl_interface_stm32_pin_defs
   :project: Zephyr

Groups
======

.. doxygengroup:: pinctrl_interface_stm32_group_defs
   :project: Zephyr

Functions
=========

.. doxygengroup:: pinctrl_interface_stm32_func_defs
   :project: Zephyr

