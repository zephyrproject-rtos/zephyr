.. zephyr:code-sample:: x-nucleo-nfc04a1
   :name: X-NUCLEO-NFC04A1 shield
   :relevant-api: eeprom_interface gpio_interface

   Interact with the EEPROM and LEDs of an X-NUCLEO-NFC04A1 shield.

Overview
********
This sample is provided as an example to test the X-NUCLEO-NFC04A1 shield.
Please refer to :ref:`x-nucleo-nfc04a1` for more info on this configuration.

This sample increments a counter in the ST25DV EEPROM at boot. It then
loops through all 3 LEDs and toggle each one every one second.

Requirements
************

This sample communicates over I2C with the X-NUCLEO-NFC04A1 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree). See for example the :ref:`nucleo_g0b1re_board` board
source code:

- :file:`$ZEPHYR_BASE/boards/arm/nucleo_g0b1re/nucleo_g0b1re.dts`
- :file:`$ZEPHYR_BASE/boards/arm/nucleo_g0b1re/pinmux.c`

References
**********

- X-NUCLEO-NFC04A1: http://www.st.com/en/ecosystems/x-nucleo-nfc04a1.html

Building and Running
********************

This sample runs with X-NUCLEO-NFC04A1 stacked on any board with a matching
Arduino connector. For this example, we use a :ref:`nucleo_g0b1re_board` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_nfc04a1/standard/
   :host-os: unix
   :board: nucleo_g0b1re
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console


    X-NUCLEO-NFC04A1 sensor dashboard


    *** Booting Zephyr OS build 9ad30bf5846e ***
    Found EEPROM device "eeprom@53"
    UUID = F4 01 13 05.
    REV_IC = 12.
    Using eeprom with size of: 4096.
    Device booted 7 times.
    Reset the MCU to see the increasing boot counter

       <each LED toggles every 1 second>
