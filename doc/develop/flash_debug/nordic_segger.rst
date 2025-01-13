:orphan:

.. _nordic_segger:

Nordic nRF5x Segger J-Link
##########################

Overview
********

All Nordic nRF5x Development Kits, Preview Development Kits and Dongles are equipped
with a Debug IC (Atmel ATSAM3U2C) which provides the following functionality:

* Segger J-Link firmware and desktop tools
* SWD debug for the nRF5x IC
* Mass Storage device for drag-and-drop image flashing
* USB CDC ACM Serial Port bridged to the nRF5x UART peripheral
* Segger RTT Console
* Segger Ozone Debugger

Segger J-Link Software Installation
***********************************

To install the J-Link Software and documentation pack, follow the steps below:

#. Download the appropriate package from the `J-Link Software and documentation pack`_ website
#. Depending on your platform, install the package or run the installer
#. When connecting a J-Link-enabled board such as an nRF5x DK, PDK or dongle, a
   drive corresponding to a USB Mass Storage device as well as a serial port should come up

nRF5x Command-Line Tools Installation
*************************************

The nRF5x command-line Tools allow you to control your nRF5x device from the command line,
including resetting it, erasing or programming the flash memory and more.

To install them, visit `nRF5x Command-Line Tools`_ and select your operating
system.

After installing, make sure that ``nrfjprog`` is somewhere in your executable path
to be able to invoke it from anywhere.

.. _nordic_segger_flashing:

Flashing
********

To program the flash with a compiled Zephyr image after having followed the instructions
to install the Segger J-Link Software and the nRF5x Command-Line Tools, follow the steps below:

* Connect the micro-USB cable to the nRF5x board and to your computer
* Erase the flash memory in the nRF5x IC:

.. code-block:: console

   nrfjprog --eraseall -f nrf5<x>

Where ``<x>`` is either 1 for nRF51-based boards or 2 for nRF52-based boards

* Flash the Zephyr image from the sample folder of your choice:

.. code-block:: console

   nrfjprog --program outdir/<board>/zephyr.hex -f nrf5<x>

Where: ``<board>`` is the board name you used in the BOARD directive when building (for example nrf52dk/nrf52832)
and ``<x>`` is either 1 for nRF51-based boards or 2 for nRF52-based boards

* Reset and start Zephyr:

.. code-block:: console

   nrfjprog --reset -f nrf5<x>

Where ``<x>`` is either 1 for nRF51-based boards or 2 for nRF52-based boards

USB CDC ACM Serial Port Setup
*****************************

**Important note**: An issue with Segger J-Link firmware on the nRF5x boards might cause
data loss and/or corruption on the USB CDC ACM Serial Port on some machines.
To work around this disable the Mass Storage Device on your board as described in :ref:`nordic_segger_msd`.

Windows
=======

The serial port will appear as ``COMxx``. Simply check the "Ports (COM & LPT)" section
in the Device Manager.

GNU/Linux
=========

The serial port will appear as ``/dev/ttyACMx``. By default the port is not accessible to all users.
Type the command below to add your user to the dialout group to give it access to the serial port.
Note that re-login is required for this to take effect.

.. code-block:: bash

   sudo usermod -a -G dialout `whoami`

Recent versions of `ModemManager send AT commands to TTY-like devices`_; this
includes Nordic development kits. This will prevent you from using the serial
port for a few seconds, and can make your application misbehave if it reads
data from the UART. Before running your application, you might want to
temporarily disable ModemManager by running these commands:

.. code-block:: bash

   systemctl stop ModemManager.service
   systemctl disable ModemManager.service

You can also `blocklist Segger devices by editing udev rules`_ so ModemManager
ignores them, by running:

.. code-block:: bash

   sudo sh -c 'echo "ATTRS{idVendor}==\"1366\", ENV{ID_MM_DEVICE_IGNORE}=\"1\" " \
     >> /etc/udev/rules.d/99-segger-modemmanager-blocklist.rules'
   sudo service udev restart

A fix for this is expected in ModemManager 1.8 and new firmware for the Segger IMCUs.

Apple macOS (OS X)
==================

The serial port will appear as ``/dev/tty.usbmodemXXXX``.

.. _nordic_segger_msd:

Disabling the Mass Storage Device functionality
***********************************************

Due to a known issue in Segger's J-Link firmware, depending on your operating system
and version you might experience data corruption or drops if you use the USB CDC
ACM Serial Port with packets larger than 64 bytes.
This has been observed on both GNU/Linux and macOS (OS X).

To avoid this, you can simply disable the Mass Storage Device by opening:

* On GNU/Linux or macOS (OS X) JLinkExe from a terminal
* On Microsoft Windows the "JLink Commander" application

And then typing the following:

.. code-block:: bat

   MSDDisable

And finally unplugging and replugging the board. The Mass Storage Device should
not appear anymore and you should now be able to send long packets over the virtual Serial Port.
Further information from Segger can be found in the `Segger SAM3U Wiki`_.

RTT Console
***********

Segger's J-Link supports `Real-Time Tracing (RTT)`_, a technology that allows a terminal
connection (both input and output) to be established between the target (nRF5x board)
and the development computer for logging and input. Zephyr supports RTT on nRF5x targets,
which can be very useful if the UART (through USB CDC ACM) is already being used for
a purpose different than logging (such as HCI traffic in the hci_uart application).
To use RTT, you will first need to enable it by adding the following lines in your ``.conf`` file:

.. code-block:: cfg

   CONFIG_USE_SEGGER_RTT=y
   CONFIG_RTT_CONSOLE=y

.. warning::

   There is also a ``HAS_SEGGER_RTT`` symbol that indicates that the platform
   supports SEGGER J-Link RTT. This symbol is set automatically by the SoC
   Kconfig files. Do not confuse it with ``USE_SEGGER_RTT``.

   ``USE_SEGGER_RTT`` depends on ``HAS_SEGGER_RTT``.

If you get no RTT output you might need to disable other consoles which conflict
with the RTT one if they are enabled by default in the particular sample or
application you are running. For example, to disable the UART console,
add this to your ``.conf`` file:

.. code-block:: cfg

   CONFIG_UART_CONSOLE=n

Once compiled and flashed with RTT enabled, you will be able to display RTT console
messages by doing the following:

Windows
=======

* Open the "J-Link RTT Viewer" application
* Select the following options:

  * Connection: USB
  * Target Device: Select your IC from the list
  * Target Interface and Speed: SWD, 4000 KHz
  * RTT Control Block: Auto Detection

GNU/Linux and macOS (OS X)
==========================

* Open ``JLinkRTTLogger`` from a terminal
* Select the following options:

  * Device Name: Use the fully qualified device name for your IC
  * Target Interface: SWD
  * Interface Speed: 4000 KHz
  * RTT Control Block address: auto-detection
  * RTT Channel name or index: 0
  * Output file: filename or ``/dev/stdout`` to display on the terminal directly

Python viewer
=============

A Python RTT viewer tool can be found in the `pyrtt-viewer`_ GitHub repository.

Segger Ozone
************

Segger J-Link is compatible with `Segger Ozone`_, a visual debugger that can be obtained here:

* `Segger Ozone Download`_

Once downloaded you can install it and configure it like so:

* Target Device: Select your IC from the list
* Target Interface: SWD
* Target Interface Speed: 4 MHz
* Host Interface: USB

Once configured, you can then use the File->Open menu to open the ``zephyr.elf``
file that you can find in your build folder.

References
**********

.. target-notes::

.. _nRF5x Command-Line Tools: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools

.. _Segger SAM3U Wiki: https://wiki.segger.com/index.php?title=J-Link-OB_SAM3U
.. _Real-Time Tracing (RTT): https://www.segger.com/jlink-rtt.html
.. _pyrtt-viewer: https://github.com/thomasstenersen/pyrtt-viewer
.. _Segger Ozone: https://www.segger.com/ozone.html
.. _Segger Ozone Download: https://www.segger.com/downloads/jlink#Ozone

.. _ModemManager send AT commands to TTY-like devices: https://bugs.freedesktop.org/show_bug.cgi?id=85007
.. _blocklist Segger devices by editing udev rules: http://www.at91.com/linux4sam/bin/view/Linux4SAM/SoftwareTools#Device_or_resource_busy_dev_ttyA

.. _J-Link Software and documentation pack: https://www.segger.com/jlink-software.html
