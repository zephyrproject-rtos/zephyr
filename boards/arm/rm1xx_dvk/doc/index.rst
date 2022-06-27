.. _rm1xx_dvk:

Laird Connectivity RM1xx DVK
############################

Overview
********

Laird Connectivity's RM1xx is a module which integrates both LoRa and
BLE communications, powered by a Nordic Semiconductor nRF51822 ARM
Cortex-M0 CPU and on-board Semtech SX1272 LoRa RF chip. This board
supports the RM1xx on the RM1xx development board - RM191 for the
915MHz version and RM186 for the 868MHz version.

This development kit has the following features:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

.. figure:: img/RM186-DVK.png
     :width: 500px
     :align: center
     :alt: RM1xx development kit (DVK)

     RM1xx development kit (DVK) (Credit: Laird Connectivity)

.. figure:: img/RM186-SM.jpg
     :width: 185px
     :align: center
     :alt: RM1xx module

     RM1xx module (Credit: Laird Connectivity)

More information about the module can be found on the
`RM1xx homepage`_.

The `Nordic Semiconductor Infocenter`_
contains the processor's information and the datasheet.

Hardware
********

The RM1xx has two internal oscillators. The frequency of
the slow clock is 32.768KHz. The frequency of the main clock
is 16MHz.


Supported Features
==================

The rm1xx_dvk board configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| SPU       | on-chip    | system protection    |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.
See `Nordic Semiconductor Infocenter`_
for a complete list of hardware features.

Connections and IOs
===================

The development board features a Microchip MCP23S08 SPI port expander -
note that this is not currently supported in Zephyr.

Refer to the `Microchip MCP23S08 datasheet`_ for further details.

Push buttons
------------

* BUTTON2 = SW0 = P0.05


Internal Memory
===============

EEPROM Memory
-------------

A 512KB (4Mb) Adesto AT25DF041B EEPROM is available via SPI for storage
of infrequently updated data and small datasets and can be used with
the spi-nor driver. Note that the EEPROM shares the same SPI bus as the
SX1272 LoRa transceiver so priority access should be given to the LoRa
radio.

Refer to the `Adesto AT25DF041B datasheet`_ for further details.

LoRa
====

A Semtech SX1272 transceiver chip is present in the module which can be
used in 915MHz LoRa frequency ranges if using an RM191 module or 868MHz
LoRa frequency ranges if uses an RM186 module

Refer to the `Semtech SX1272 datasheet`_ for further details.

Programming and Debugging
*************************

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF51 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rm1xx_dvk
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging boards
with a Segger IC.

References
**********

.. target-notes::

.. _RM1xx homepage: https://www.lairdconnect.com/wireless-modules/lorawan-solutions/sentrius-rm1xx-lora-ble-module
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
.. _Adesto AT25DF041B datasheet: https://www.dialog-semiconductor.com/sites/default/files/ds-at25df041b_040.pdf
.. _Semtech SX1272 datasheet: https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/440000001NCE/v_VBhk1IolDgxwwnOpcS_vTFxPfSEPQbuneK3mWsXlU
.. _Microchip MCP23S08 datasheet: https://www.microchip.com/content/dam/mchp/documents/OTH/ProductDocuments/DataSheets/MCP23008-MCP23S08-Data-Sheet-20001919F.pdf
