.. _esp32_net:

ESP32-NET
#########

Overview
********

ESP32_NET is a board configuration to allow zephyr application building
targeted to ESP32 APP_CPU only, please refer ESP32 board to a more complete
list of features.

System requirements
*******************

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

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32_devkitc_wroom
   :goals: build

The usual ``flash`` target will work with the ``esp32_devkitc_wroom`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32_devkitc_wroom
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! esp32_devkitc_wroom

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
   :board: esp32_devkitc_wroom
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32_devkitc_wroom
   :goals: debug

Using JTAG
======================

On the ESP-WROVER-KIT board, the JTAG pins are connected internally to
a USB serial port on the same device as the console.  These boards
require no external hardware and are debuggable as-is.  The JTAG
signals, however, must be jumpered closed to connect the internal
controller (the default is to leave them disconnected).  The jumper
headers are on the right side of the board as viewed from the power
switch, next to similar headers for SPI and UART.  See
`ESP-WROVER-32 V3 Getting Started Guide`_ for details.

On the ESP-WROOM-32 DevKitC board, the JTAG pins are not run to a
standard connector (e.g. ARM 20-pin) and need to be manually connected
to the external programmer (e.g. a Flyswatter2):

+------------+-----------+
| ESP32 pin  | JTAG pin  |
+============+===========+
| 3V3        | VTRef     |
+------------+-----------+
| EN         | nTRST     |
+------------+-----------+
| IO14       | TMS       |
+------------+-----------+
| IO12       | TDI       |
+------------+-----------+
| GND        | GND       |
+------------+-----------+
| IO13       | TCK       |
+------------+-----------+
| IO15       | TDO       |
+------------+-----------+

Once the device is connected, you should be able to connect with (for
a DevKitC board, replace with esp32-wrover.cfg for WROVER):

.. code-block:: console

    cd ~/esp/openocd-esp32
    src/openocd -f interface/ftdi/flyswatter2.cfg -c 'set ESP32_ONLYCPU 1' -c 'set ESP32_RTOS none' -f board/esp-wroom-32.cfg -s tcl

The ESP32_ONLYCPU setting is critical: without it OpenOCD will present
only the "APP_CPU" via the gdbserver, and not the "PRO_CPU" on which
Zephyr is running.  It's currently unexplored as to whether the CPU
can be switched at runtime or if breakpoints can be set for
either/both.

Now you can connect to openocd with gdb and point it to the OpenOCD
gdbserver running (by default) on localhost port 3333.  Note that you
must use the gdb distributed with the ESP-32 SDK.  Builds off of the
FSF mainline get inexplicable protocol errors when connecting.

.. code-block:: console

    ~/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb outdir/esp32/zephyr.elf
    (gdb) target remote localhost:3333

Further documentation can be obtained from the SoC vendor in `JTAG debugging
for ESP32`_.

Note on Debugging with GDB Stub
===============================

GDB stub is enabled on ESP32.

* When adding breakpoints, please use hardware breakpoints with command
  ``hbreak``. Command ``break`` uses software breakpoints which requires
  modifying memory content to insert break/trap instructions.
  This does not work as the code is on flash which cannot be randomly
  accessed for modification.

References
**********

.. _`ESP32 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32`: http://esp-idf.readthedocs.io/en/latest/api-guides/jtag-debugging/index.html
.. _`Hardware Reference`: https://esp-idf.readthedocs.io/en/latest/hw-reference/index.html
.. _`ESP-WROVER-32 V3 Getting Started Guide`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-wrover-kit.html
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
