.. _gpio_emul:

GPIO Emulator
#############

Overview
========

Zephyr's GPIO (General Purpose Input-Output) emulator has several functions.

1. It serves as a model for the basic GPIO Driver API; if a new GPIO driver is added for a new
    controller, and some aspect of the new driver fails Zephyr's GPIO Driver API Testsuite, while
    the emulated GPIO driver (and others) pass, it may be an indicator that something in the new
    driver does not follow the same behaviour required by Zephyr's GPIO Driver Model, and the new
    driver requires improvement.

2. The ``gpio_emul`` device allows us to perform fast functional testing to verify the expected
    behaviour of various libraries and applications. Why? In Zephyr, a GPIO is a GPIO is a GPIO
    (from a functional perspective) because there is a common GPIO Driver API and model. How can
    emulated GPIO be used in an application? Well, the GPIO Emulator also has programmable
    behaviour via the backend API. So the application can inject custom input events, create
    custom "wiring" via the existing GPIO Driver API.

3. Lastly, ``gpio_emul`` is extensible, which means it can be repurposed where other, virtual
    GPIO might be used. For example, the SDL Button driver.


Usage
=====

Use the emulated GPIO driver exactly the same as one would use any other GPIO driver.

Insert an instance into Devicetree, ensuring that the named compatible is ``zephyr,gpio-emul``
and the node has ``status = "okay"``. The ``rising-edge``, ``falling-edge``, ``high-level``, and
``low-level`` events may be specified, enabling the emulated GPIO controller
instance to generate interrupts for each event.

.. code-block:: dts

    / {
        gpio0: gpio@800 {
            status = "okay";
            compatible = "zephyr,gpio-emul";
            reg = <0x800 0x4>;
            rising-edge;
            falling-edge;
            high-level;
            low-level;
            gpio-controller;
            #gpio-cells = <2>;
        };
    };

.. note::
    The ``zephyr,gpio-emul`` Devicetree bindings inherit from ``gpio-controller`` and ``base``
    Devicetree bindings.

It may be necessary to add the :kconfig:option:`CONFIG_GPIO_EMUL=y` option to your project
configuration, although the driver is enabled by default.

Build the application in question.

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/gpio/gpio_basic_api
   :board: native_sim_64
   :goals: build run
   :compact:

Without anything specified via the backend API, the emulated GPIO behaves as if each pin is
disconnected.

In order to provide custom functionality, create a :c:type:`gpio_callback_handler_t`,
instantiate a :c:struct:`gpio_callback`, and register the callback via
:c:func:`gpio_add_callback`, as shown below.

.. code-block:: C

    #include <zephyr/sys/util.h>
    #include <zephyr/drivers/gpio.h>
    /* For gpio_emul helpers, if needed */
    #include <zephyr/drivers/gpio/gpio_emul.h>

    void gpio_emul_handler(const struct device *port,
				      struct gpio_callback *cb,
				      gpio_port_pins_t pins)
    {
        /*
         * Insert custom logic here based on timing or app state to simulate "wiring".
         * The current GPIO pin state can also be queried to simulate "feedback".
         */
    }

    void mysetup(struct device *dev)
    {
        struct gpio_callback cb;

        /* only register callback for pins 2 and 0 */
        gpio_init_callback(&cb, gpio_emul_handler, BIT(2) | BIT(0));
        gpio_add_callback(dev, &cb);
    }

Samples
=======

Here are some samples that use ``gpio_emul`` in Zephyr:

#. Lightweight Vector Graphics Library (LVGL) sample with a "soft" button

   .. zephyr-app-commands::
      :app: tests/drivers/eeprom/api
      :board: native_sim
      :goals: build
      :gen-args: -DDTC_OVERLAY_FILE=at2x_emul.overlay -DOVERLAY_CONFIG=at2x_emul.conf

Tests (for API usage and reference)
===================================

Here are some tests that use ``gpio_emul`` in Zephyr:

#. GPIO Emulator in the Basic API test suite

  .. zephyr-app-commands::
     :zephyr-app: tests/drivers/gpio/gpio_basic_api
     :board: native_sim_64
     :goals: build run
     :compact:

#. Matrix keyboard driver test suite

  .. zephyr-app-commands::
     :zephyr-app: tests/drivers/input/gpio_kbd_matrix
     :board: native_sim
     :goals: build run
     :compact:

API Reference
*************

.. doxygengroup:: gpio_emul
