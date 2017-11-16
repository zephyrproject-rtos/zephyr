:orphan:

.. _nxp_opensda:

NXP OpenSDA
###########

Overview
********

`OpenSDA`_ is a serial and debug adapter that is built into several NXP
evaluation boards. It provides a bridge between your computer (or other USB
host) and the embedded target processor, which can be used for debugging, flash
programming, and serial communication, all over a simple USB cable.

The OpenSDA hardware features a Kinetis K2x microcontroller with an integrated
USB controller. On the software side, it implements a mass storage device
bootloader which offers a quick and easy way to load OpenSDA applications such
as flash programmers, run-control debug interfaces, serial to USB converters,
and more.

Zephyr supports the following debug tools through OpenSDA:

* :ref:`nxp_opensda_pyocd`
* :ref:`nxp_opensda_jlink`

.. _nxp_opensda_firmware:

Program the Firmware
====================

Once you've selected which debug tool you wish to use, you need to program the
associated OpenSDA firmware application to the OpenSDA adapter.

Put the OpenSDA adapter into bootloader mode by holding the reset button while
you power on the board. After you power on the board, release the reset button
and a USB mass storage device called **BOOTLOADER** or **MAINTENANCE** will
enumerate. Copy the OpenSDA firmware application binary to the USB mass storage
device. Power cycle the board, this time without holding the reset button.


.. _nxp_opensda_pyocd:

pyOCD
*****

pyOCD is an Open Source python 2.7 based library for programming and debugging
ARM Cortex-M microcontrollers using CMSIS-DAP.

Host Tools and Firmware
=======================

Follow the instructions in `pyOCD Installation`_ to install the pyOCD flash
tool and GDB server for your host computer.

Select your board in `OpenSDA`_ and download the latest DAPLink firmware
application binary. :ref:`nxp_opensda_firmware` with this application.

Flashing
========

Use the CMake ``flash`` target with the argument ``OPENSDA_FW=daplink`` to
build your Zephyr application, invoke the pyOCD flash tool and program your
Zephyr application to flash. Some boards set the argument by default, so it is
not always necessary to set explicitly.

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :gen-args: -DOPENSDA_FW=daplink
     :goals: flash

Debugging
=========

Use the CMake ``debug`` target with the argument ``OPENSDA_FW=daplink`` to
build your Zephyr application, invoke the pyOCD GDB server, attach a GDB
client, and program your Zephyr application to flash. It will leave you at a
GDB prompt. Some boards set the argument by default, so it is not always
necessary to set explicitly.

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :gen-args: -DOPENSDA_FW=daplink
     :goals: debug


.. _nxp_opensda_jlink:

Segger J-Link
*************

Segger offers firmware running on the OpenSDA platform which makes OpenSDA
compatible to J-Link Lite, allowing users to take advantage of most J-Link
features like the ultra fast flash download and debugging speed or the
free-to-use GDB Server, by using a low-cost OpenSDA platform for developing on
evaluation boards.

Host Tools and Firmware
=======================

Download and install the `Segger J-Link Software and Documentation Pack`_ to
get the J-Link GDB server for your host computer.

Select your board in `OpenSDA`_ and download the Segger J-Link firmware
application binary. :ref:`nxp_opensda_firmware` with this application.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the CMake ``flash`` target is not supported.

Debugging
=========

Use the CMake ``debug`` target with the argument ``OPENSDA_FW=jlink`` to build
your Zephyr application, invoke the J-Link GDB server, attach a GDB client, and
program your Zephyr application to flash. It will leave you at a GDB prompt.
Some boards set the argument by default, so it is not always necessary to set
explicitly.

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :gen-args: -DOPENSDA_FW=jlink
     :goals: debug

Console
=======

If you configured your Zephyr application to use a UART console (most boards
enable this by default), open a serial terminal (minicom, putty, etc.) with the
following settings:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

If you configured your Zephyr application to use `Segger RTT`_ console instead,
open telnet:

  .. code-block:: console

     $ telnet localhost 19021
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.14b - Real time terminal output
     J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57 V1.0, SN=621000000
     Process: JLinkGDBServer


.. _OpenSDA:
   http://www.nxp.com/opensda

.. _Segger J-Link OpenSDA:
   https://www.segger.com/opensda.html

.. _Segger J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink

.. _Segger RTT:
    https://www.segger.com/jlink-rtt.html

.. _pyOCD Installation:
   https://github.com/mbedmicro/pyOCD#installation
