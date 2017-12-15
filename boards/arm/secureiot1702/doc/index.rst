.. _secureiot1702:

Microchip SecureIoT1702
#######################

Overview
********

This demo board features a Microchip CEC1702 cryptographic
embedded controlled based on an ARM Cortex-M4.

Highlights of the board:

- CEC1702 32-bit ARM® Cortex®-M4F Controller with Integrated Crypto
- Compact, high-contrast, serial graphic LCD Display Module with back-light
- 2x4 matrix of push buttons inputs
- USB-UART Converter as debug interface
- Potentiometer to ADC channel
- Serial Quad I/O (SQI) flash
- OTP programmable in CEC1702
- Two expansion headers compatible with MikroElektronika mikroBUS™ Expansion interface

More information can be found on the `SecureIoT1702 website`_ and
`CEC1702 website`_, and SoC programming information is available
in the `CEC1702 datasheet`_.

Supported Features
==================

The following devices are supported:

- Nested Vectored Interrupt Controller (NVIC)
- System Tick System Clock (SYSTICK)
- Serial Ports (NS16550)


Connections and IOs
===================

Please refer to the `SecureIoT1702 schematics`_ for the pin routings.
Additional devices can be connected via mikroBUS expansion interface.

Programming and Debugging
*************************

This board comes with a 10-pin Cortext Debug port and a separate SPI
flash programming header.

Applications for the ``secureiot1702`` board configuration can be
built the usual way (see :ref:`build_an_application` for more details) which
is then programmed directly to the external SPI flash chip.

Flashing
========

#  Add extra configuration :code:`CONFIG_BOOT_DELAY=4000` in :code:`prj.conf`.
   This is needed for the host USB drivers to be ready to see the boot messages.
   Build :ref:`hello_world` application. The build will result
   in :code:`zephyr_spi_image.bin`.

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: secureiot1702
      :goals: build

#. Connect your SPI programmer to SecureIoT1702 connector X12 in order to flash.
   Then proceed to flash using flashrom v1.1 or a similar tool for flashing
   SPI chip with :code:`zephyr_spi_image.bin`.

   .. code-block:: console

      $ flashrom -w zephyr_spi_image.bin

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o -b 115200

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Connect the SecureIoT1702 to your host computer using the USB connector.

   You should see "Hello World! secureiot1702" in your terminal.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mec15xxevb_assy6853
   :maybe-skip-config:
   :goals: debug


References
**********

.. target-notes::

.. _CEC1702 website:
   http://www.microchip.com/CEC1702

.. _CEC1702 datasheet:
   http://www.microchip.com/p/207/

.. _CEC1702 quick start guide:
   http://ww1.microchip.com/downloads/en/DeviceDoc/50002665A.pdf

.. _SecureIoT1702 website:
   http://www.microchip.com/Developmenttools/ProductDetails.aspx?PartNO=DM990012

.. _SecureIoT1702 schematics:
   http://microchipdeveloper.com/secureiot1702:schematic
