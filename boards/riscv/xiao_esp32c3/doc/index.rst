.. _xiao_esp32c3:

XIAO ESP32C3
############

Overview
********

Seeed Studio XIAO ESP32C3 is an IoT mini development board based on the
Espressif ESP32-C3 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio XIAO ESP32C3`_ wiki page.

.. figure:: img/xiao_esp32c.jpg
   :align: center
   :alt: XIAO ESP32C3

   XIAO ESP32C3

Hardware
********

This board is based on the ESP32-C3 with 4MB of flash, WiFi and BLE support. It
has an USB-C port for programming and debugging, integrated battery charging
and an U.FL external antenna connector. It is based on a standard XIAO 14 pin
pinout.

Supported Features
==================

The XIAO ESP32C3 board configuration supports the following hardware features:

+-----------+------------+------------------+
| Interface | Controller | Driver/Component |
+===========+============+==================+
| PMP       | on-chip    | arch/riscv       |
+-----------+------------+------------------+
| INTMTRX   | on-chip    | intc_esp32c3     |
+-----------+------------+------------------+
| PINMUX    | on-chip    | pinctrl_esp32    |
+-----------+------------+------------------+
| USB UART  | on-chip    | serial_esp32_usb |
+-----------+------------+------------------+
| GPIO      | on-chip    | gpio_esp32       |
+-----------+------------+------------------+
| UART      | on-chip    | uart_esp32       |
+-----------+------------+------------------+
| I2C       | on-chip    | i2c_esp32        |
+-----------+------------+------------------+
| SPI       | on-chip    | spi_esp32_spim   |
+-----------+------------+------------------+
| TWAI      | on-chip    | can_esp32_twai   |
+-----------+------------+------------------+

Connections and IOs
===================

The board uses a standard XIAO pinout, the default pin mapping is the following:

.. figure:: img/xiao_esp32c3_pinout.jpg
   :align: center
   :alt: XIAO ESP32C3 Pinout

   XIAO ESP32C3 Pinout

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
-------------------

For the :code:`Hello, world!` application, follow the instructions below.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_esp32c3
   :goals: build flash

Since the Zephyr console is by default on the `usb_serial` device, we use
the espressif monitor to view.

.. code-block:: console

   $ west espressif monitor

Debugging
---------

As with much custom hardware, the ESP32 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_esp32c3
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_esp32c3
   :goals: debug

References
**********

.. target-notes::

.. _`Seeed Studio XIAO ESP32C3`: https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
