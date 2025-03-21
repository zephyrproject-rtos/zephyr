.. zephyr:board:: sam4l_wm400_cape

Overview
********

The SAM4L WM-400 Cape is a full featured design to enable IEEE 802.15.4 low
power nodes. It is a Beaglebone Black cape concept with an Atmel AT86RF233
radio transceiver. User can develop Touch interface and have access to many
sensors and conectivity buses.

Hardware
********

- ATSAM4LC4B ARM Cortex-M4 Processor
- 12 MHz crystal oscillator
- 32.768 kHz crystal oscillator
- 1 RS-232 interface
- 1 RS-485 full duplex interface
- Micro-AB USB OTG host/device
- 1 user touch button and One user pushbutton
- 4 user LEDs
- 1 AT86RF233 IEEE 802.15.4 transceiver
- 1 MPL115A2 I²C Barometric Pressure/Temperature Sensor
- 1 VCNL4010 Proximity/Light Sensor
- 1 CC2D33S Advanced Humidity Temperature Sensor
- 1 NCP18WF104J03RB NTC Temperature Sensor
- 1 TEMT6000X01 Ambient Light Sensor

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

For detailed information see `SAM4L WM-400 Cape`_ Information.

System Clock
============

The SAM4L MCU is configured to use the 12 MHz internal oscillator on the board
with the on-chip PLL to generate an 48 MHz system clock.

Serial Port
===========

The ATSAM4LC4B MCU has 4 USARTs. One of the USARTs (USART3) is shared between
RS-232 and RS-485 interfaces. The default console terminal is available at
RS-232 onboard port or via USB device.

Programming and Debugging
*************************

The SAM4L WM-400 Cape board has a 10-pin header to connect to a Segger JLink.
Using the JLink is possible to program and debug the SAM4LC4B chip. The board
came with a SAM-BA bootloader that only can be used to flash the software.

Flashing
========

#. For JLink instructions, see :ref:`jlink-debug-host-tools`.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Connect the SAM4L WM-400 Cape board to your host computer using the
   USB debug port. Then build and flash the :zephyr:code-sample:`hello_world`
   application.

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: sam4l_wm400_cape
      :goals: build flash

   You should see ``Hello World! sam4l_wm400_cape`` in your terminal.

#. For SAM-BA bootloader instructions, see :ref:`atmel_sam_ba_bootloader`.

#. Connect the SAM4L WM-400 Cape board to your host computer using the
   USB debug port pressing the S1 button. Then build and flash the
   :zephyr:code-sample:`hello_world` application. After programming the board
   the application will start automatically.

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: sam4l_wm400_cape
      :goals: build flash
      :flash-args: -r bossac


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sam4l_wm400_cape
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _SAM4L WM-400 Cape:
    https://gfbudke.wordpress.com/2014/04/30/modulo-wireless-ieee-802-15-4zigbee-wm-400-e-wm-400l-bbbs
