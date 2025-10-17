.. zephyr:code-sample:: eeprom
   :name: EEPROM
   :relevant-api: eeprom_interface

   Store a boot count value in EEPROM.

Overview
********

This sample demonstrates the :ref:`EEPROM driver API <eeprom_api>` in a simple boot counter
application.

Building and Running
********************

In case the target board has defined an EEPROM with alias ``eeprom-0`` the
sample can be built without further ado. This applies for example to the
:zephyr:board:`native_sim` board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/eeprom
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Otherwise either a board specific overlay needs to be defined, or a shield must
be activated. Any board with Arduino headers can for example build the sample
as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/eeprom
   :board: nrf52840dk/nrf52840
   :goals: build
   :shield: x_nucleo_eeprma2
   :compact:

For :zephyr:board:`gd32f450i_eval` board. First bridge the JP5 to USART with the jumper cap,
Then the sample can be built and executed for the  as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/eeprom
   :board: gd32f450i_eval
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    Found EEPROM device "EEPROM_M24C02"
    Using eeprom with size of: 256.
    Device booted 7 times.
    Reset the MCU to see the increasing boot counter.
