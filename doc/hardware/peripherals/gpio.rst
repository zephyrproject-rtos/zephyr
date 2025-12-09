.. _gpio_api:

General-Purpose Input/Output (GPIO)
###################################

Overview
********

A General-Purpose Input/Output (GPIO) is a digital signal pin that does not have a specific function
but can be controlled by software to function as an input or an output.

The GPIO API provides a generic method to interact with General Purpose Input/Output (GPIO)
pins. It allows applications to configure pins as inputs or outputs, read and write
their state, and manage interrupts. Key features include:

**Pin Configuration**
  Configure pins as input, output, or disconnected.
  Support for internal pull-up/pull-down resistors and drive strength configuration.

**Data Access**
  Read input values and write output values.

**Interrupts**
  Configure interrupts on pin state changes (rising edge, falling edge,
  level low, level high) and register callbacks to handle these interrupts.

**Devicetree Integration**
  GPIOs are typically defined in the Devicetree, allowing
  drivers and applications to reference them in a hardware-agnostic way using
  :c:struct:`gpio_dt_spec`.

Devicetree Configuration
************************

GPIO controllers are defined in the Devicetree as nodes with the ``gpio-controller`` property.
The ``#gpio-cells`` property typically specifies that 2 cells are used to describe a GPIO: the pin
number and the flags.

Example of a GPIO controller definition:

.. code-block:: devicetree

   gpio0: gpio@40022000 {
       compatible = "ti,cc13xx-cc26xx-gpio";
       reg = <0x40022000 0x400>;
       interrupts = <0 0>;
       gpio-controller;
       #gpio-cells = <2>;
   };

Example of referencing a GPIO:

.. code-block:: devicetree

   leds {
       compatible = "gpio-leds";
       led0: led_0 {
           gpios = <&gpio0 25 GPIO_ACTIVE_HIGH>;
           label = "Green LED";
       };
   };

Basic Operation
***************

GPIO operations are usually performed on a :c:struct:`gpio_dt_spec` structure, which is a container
for the GPIO pin information specified in the Devicetree.

This structure is typically populated using :c:macro:`GPIO_DT_SPEC_GET` macro (or any of its
variants).

.. code-block:: c
   :caption: Populating a gpio_dt_spec structure for a GPIO pin defined as alias ``led0``

   #define LED0_NODE DT_ALIAS(led0)
   static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

The :c:struct:`gpio_dt_spec` structure can then be used to perform GPIO operations.

.. code-block:: c
   :caption: Configure a GPIO pin (``led`` from the previous snippet) as output, initially inactive,
             and then set it to its active level.

   int ret;

   ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
   if (ret < 0) {
       return ret;
   }

   ret = gpio_pin_set_dt(&led, 1);
   if (ret < 0) {
       return ret;
   }

Refer to :zephyr:code-sample:`blinky` for a complete example of basic operations on GPIOs using the
:c:struct:`gpio_dt_spec` structure.

GPIO operations can also be performed directly on a GPIO controller device, in which case you will
use the GPIO API functions that take a device pointer as an argument. For example
:c:func:`gpio_pin_configure` instead of :c:func:`gpio_pin_configure_dt`.

GPIO Hogs
*********

GPIO hogs provide a mechanism to automatically configure GPIO pins during system initialization.
This is useful for pins that need to be set to a specific state (e.g., reset lines, power enables)
and don't require runtime control by an application.

Hogs are defined in the Devicetree as children of a GPIO controller node.

- The ``gpio-hog`` property marks the node as a hog.
- The ``gpios`` property specifies the pin and its active state.
- One of ``input``, ``output-low``, or ``output-high`` properties specifies the configuration.

Example Devicetree overlay:

.. code-block:: devicetree

   &gpio0 {
       hog1 {
           gpio-hog;
           gpios = <1 GPIO_ACTIVE_LOW>;
           output-high;
       };

       hog2 {
           gpio-hog;
           gpios = <2 GPIO_ACTIVE_HIGH>;
           output-low;
       };
   };

If you need runtime control of a pin configured as a hog beyond the initial state being
automatically set during system initialization, you may consider using the :ref:`regulator_api`
instead, with a :dtcompatible:`regulator-fixed` Devicetree node.

Configuration Options
*********************

Main configuration options:

* :kconfig:option:`CONFIG_GPIO`
* :kconfig:option:`CONFIG_GPIO_SHELL`
* :kconfig:option:`CONFIG_GPIO_GET_DIRECTION`
* :kconfig:option:`CONFIG_GPIO_GET_CONFIG`
* :kconfig:option:`CONFIG_GPIO_HOGS`
* :kconfig:option:`CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT`

API Reference
*************

.. doxygengroup:: gpio_interface
