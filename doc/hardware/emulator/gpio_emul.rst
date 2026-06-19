.. _gpio_emul:

GPIO Emulator
#############

Overview
========

The GPIO emulator is a software emulated GPIO controller in Zephyr that allows
developers to test GPIO-dependent code without requiring real hardware. It
implements the standard :ref:`GPIO driver API <gpio_api>` and can be used on
any target with sufficient RAM and flash, including :zephyr:board:`native_sim`.

The GPIO emulator supports:

* Configuring pins as input or output
* Reading and writing pin values from application code
* Triggering interrupts on rising edge, falling edge, high level, and low level
* Testing GPIO interrupt handlers without physical signals

This makes it particularly useful for unit testing drivers and applications that
depend on GPIO signals, such as button inputs, sensor data-ready pins, or
hardware control lines.

Configuration
=============

To enable the GPIO emulator, add the following to your ``prj.conf``:

.. code-block:: kconfig

   CONFIG_GPIO=y
   CONFIG_GPIO_EMUL=y

The GPIO emulator is defined in devicetree using the
:dtcompatible:`zephyr,gpio-emul` compatible string. Add the following to your
board's devicetree overlay:

.. code-block:: devicetree

   / {
       gpio_emul0: gpio_emul@0 {
           compatible = "zephyr,gpio-emul";
           reg = <0x0 0x4>;
           rising-edge;
           falling-edge;
           high-level;
           low-level;
           gpio-controller;
           #gpio-cells = <2>;
           ngpios = <8>;
       };
   };

The ``ngpios`` property sets the number of emulated GPIO pins (maximum 32).
The interrupt capability properties (``rising-edge``, ``falling-edge``,
``high-level``, ``low-level``) define which interrupt trigger types the
emulated controller supports.

Usage
=====

The GPIO emulator provides a backend API in
:file:`include/zephyr/drivers/gpio/gpio_emul.h` that allows tests to drive
pin values directly from software.

To set an input pin value from a test:

.. code-block:: c

   #include <zephyr/drivers/gpio.h>
   #include <zephyr/drivers/gpio/gpio_emul.h>

   const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio_emul0));

   /* Set pin 3 high (simulates an external signal going high) */
   gpio_emul_input_set(gpio_dev, 3, 1);

   /* Set pin 3 low */
   gpio_emul_input_set(gpio_dev, 3, 0);

To read the current output value that application code has written to a pin:

.. code-block:: c

   gpio_port_value_t value;

   gpio_emul_output_get(gpio_dev, &value);

Interrupt Testing
=================

The GPIO emulator supports interrupt testing by allowing tests to drive input
pin transitions, which trigger registered interrupt callbacks.

The interrupt capabilities are configured via devicetree properties:

* ``rising-edge`` — supports :c:macro:`GPIO_INT_EDGE_RISING`
* ``falling-edge`` — supports :c:macro:`GPIO_INT_EDGE_FALLING`
* ``high-level`` — supports :c:macro:`GPIO_INT_LEVEL_HIGH`
* ``low-level`` — supports :c:macro:`GPIO_INT_LEVEL_LOW`

Example test flow:

1. Register a GPIO interrupt callback using :c:func:`gpio_add_callback`.
2. Configure the pin interrupt using :c:func:`gpio_pin_interrupt_configure`.
3. Drive the pin using :c:func:`gpio_emul_input_set` to trigger the interrupt.
4. Verify the callback was invoked with the expected pin state.

API Reference
=============

.. doxygengroup:: gpio_emul_interface
