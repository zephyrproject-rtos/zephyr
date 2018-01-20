..
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _device_driver_gpio_pinctrl_stm32:

STM32 GPIO Device Driver
########################

.. contents::
   :depth: 1
   :local:
   :backlinks: top

STMicroelectronics's STM32 MCUs integrate several GPIO hardware controllers.

Control Properties
******************

Pins
====

There are no specific STM32 pin definitions. Use the generic pin definitions from
:file:`include/gpio_common.h`.

Signalling Configuration
************************

The STM32 GPIO PINCTRL device driver recognizes the following pin signalling
properties:

+---------------------------------------+---------+--------------------------+
| Property                              | Support | Comment                  |
+=======================================+=========+==========================+
| GPIO_INT                              | **yes** |                          |
+---------------------------------------+---------+--------------------------+
| TBD                                   | no      |                          |
+---------------------------------------+---------+--------------------------+

Device Tree Support
*******************

Nodes
=====

The STM32 GPIO device driver is based on and supports the following
device tree nodes:

GPIO controller node
--------------------

Required properies:

    - gpio-controller: Indicates this device is a GPIO controller
    - compatible: value must be: "st,stm32-gpio-pinctrl"
    - label: Should be a name string
    - st,bank-name: shall be one of "GPIOA", "GPIOB", ...
    - gpio-ranges:
        Defines a dedicated mapping between a pin controller and
        a gpio controller. Format is <&phandle a b c> with:

        - (phandle): phandle of pin-controller.
        - (a): gpio port pin base in range.
        - (b): pin controller pin base in range.
        - (c): gpio port pin count in range

    - clocks: clock that drives this bank

