.. zephyr:board:: ch582m_dev_board

Overview
********

The `WCH`_ CH582M Dev Board is a generic evaluation target for the CH582M, a
QingKe V4A 32-bit RISC-V SoC with an integrated BLE 5.3 radio. This board
target enables the following peripherals:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`UART (Universal Asynchronous Receiver Transmitter)`
* :abbr:`LED (Light Emitting Diode)`

Hardware
********

The QingKe V4A 32-bit RISC-V core of the CH582M runs at 6.4 MHz out of reset
(32 MHz crystal divided by 5). The CH582M provides 512 KB of flash and 32 KB
of SRAM.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
====================

UART1 is used as the default console. It is available on PA8 (RXD) and PA9
(TXD), which is the peripheral's default pin location after reset, so no pin
remapping is required.

Programming and Debugging
**************************

.. zephyr:board-supported-runners::

Applications for the ``ch582m_dev_board`` board target can be built in the usual way
(see :ref:`build_an_application` and :ref:`application_run` for more
details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ch582m_dev_board
   :goals: build

Before the first flash, the chip must be unprotected. Press the BOOT button on the board, plug in
USB-C, and run the following ``wchisp`` command (see wchisp_):

.. code-block:: console

   wchisp config unprotect

This step is done only once (it can be repeated if the board is switched to protected mode again).

After that one can flash the application using ``wchisp`` - disconnect USB-C, press BOOT
button, reconnect USB_C again and running the following command:

.. code-block:: console

   west flash --runner wchisp

The application can also be flashed and debugged with an external WCH-Link probe. The WCH-Link
enumerates as a USB (WinUSB/libusb) device for flashing and debugging. Any serial port it exposes
is only a pass-through console UART and is not used for flashing.

The WCH-Link debug interface only works on an unprotected chip. If a debug or flash command fails
with ``WCH-Link failed to connect with riscvchip``, unprotect the chip as described above.

``wlink`` (see wlink_) is the default runner:

.. code-block:: console

   west flash

One can also use it explicitly with the following command:

.. code-block:: console

   west flash --runner wlink

To debug the application with the WCH-Link, use a WCH-enabled OpenOCD build that provides the
``wlinke`` adapter driver, such as the ``openocd-wch`` fork (see openocd_wch_). The OpenOCD shipped
with the Zephyr SDK does not include this driver, so pass the fork's binary with ``--openocd``.

Start the debug server in one terminal (replace the path with the location of your OpenOCD build):

.. code-block:: console

   west debugserver -r openocd --openocd C:\path\to\openocd-wch\bin\openocd.exe

The server listens for GDB on port 3333. In another terminal, start the debug session:

.. code-block:: console

   west debug --no-rebuild --runner openocd --openocd C:\path\to\openocd-wch\bin\openocd.exe

If the build directory is not the default ``build``, add ``-d <build-dir>`` to both commands.

It's also possible to flash image using WCHISPStudio using default first stage bootloader through
USB-C port. To do this add image to WCHISPStudio, press BOOT button on the board and plug in USB-C.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _wchisp: https://github.com/ch32-rs/wchisp
.. _wlink: https://github.com/ch32-rs/wlink
.. _openocd_wch: https://github.com/jnk0le/openocd-wch
