.. zephyr:code-sample:: sensortile_box_pro_sample_ble_fwupg
   :name: ST SensorTile.box Pro BLE Firmware Upgrade

   Upgrade the BLE firmware on STEVAL-MKBOXPRO board

Overview
********
This sample provides a simple method to upgrade the firmware for
the BLE chip present on :ref:`sensortile_box_pro_board` board.
It is based on the BlueNRG-LPS UART bootloader protocol explained in `AN5471`_ .

Requirements
************

The application requires a SensorTile.box Pro board connected to the PC
through USB.

To run this sample the console is not strictly needed, but might be useful.
Following is an example for minicom.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the correct device path automatically created on
the host after the SensorTile.box Pro board gets connected to it,
usually :samp:`/dev/ttyUSB{x}` or :samp:`/dev/ttyACM{x}` (with x being 0, 1, 2, ...).
The ``-b`` option sets baud rate ignoring the value from config.

Building and Running
********************

To build and flash the sample, it is required fetching the controller-only BLE f/w
image for sensortile_box_pro board, which is hosted by external repo
(see `stsw-mkbox-bleco`_ for more information):

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch stm32

Then, build the sample:

.. zephyr-app-commands::
    :zephyr-app: samples/boards/sensortile_box_pro/ble-fw-upgrade
    :board: sensortile_box_pro
    :goals: build flash

Please note that flashing the board requires a few preliminary steps described
in :ref:`sensortile_box_pro_board`.

After flashing, the sample starts and asks the user to acknowledge the f/w update.
User should press :kbd:`y` or :kbd:`Y` to proceed in upgrading the BLE f/w.

 .. code-block:: console

    Start BLE f/w update (press Y to proceed):

Nevertheless it is not strictly necessary to use the console, as the user may also acknowledge
the f/w update pressing the User BT2 button (see :ref:`sensortile_box_pro_board` board and
check on user manual and/or schematic to see where the button is located and how
it works).

LEDs status
-----------

The blue LED blinks three times with 200ms interval to indicate the procedure is starting.
Then blue LED start blinking very fast to indicate the BLE flash activity is on going.

After BLE flashing is complete:

- If status is OK the green LED blinks three times with 200ms interval and remains on.
- If flashing failed the red LED blinks three times with 200ms interval and remains on.

Console messages
----------------

To properly see messages on your terminale emulatore you may also need to set lineWrap to on.
In case of minicom just enter the menu with :kbd:`Ctrl-A Z` an then press :kbd:`W`.

The sample outputs following messages.

 .. code-block:: console

    SensorTile.box Pro BLE f/w upgrade
    bootloader activated!
    ble bootloader version is 4.0
    MASS ERASE ok
    ..............................................................................................
    ..............................................................................................
    ..............................................................................................
    ..............................................................................................
    ..............................................................................................
    ..............................................................................................
    ..............................................................................................
    ............................................
    BLE f/w upgrade ok

References
**********

.. target-notes::

.. _AN5471:
   https://www.st.com/resource/en/application_note/an5471-the-bluenrglp-bluenrglps-uart-bootloader-protocol-stmicroelectronics.pdf

.. _stsw-mkbox-bleco:
   https://www.st.com/en/embedded-software/stsw-mkbox-bleco.html
