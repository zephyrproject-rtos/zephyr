.. _fanstel_ev832x:

Fanstel EV-BT832X
#################

Overview
********

The Fanstel EV-BT832X board is an evaluation board for the BluNor BT832X
Bluetooth Low Energy module. This board uses Nordic Semiconductor nRF52832 SoC,
with an ARM Cortex (TM) M4F MCU, available 512KB flash, 64KB RAM, embedded
2.4GHz multi-protocol transceiver, power amplifier, and an integrated PCB trace
antenna, or an u.FL connector for external antenna.

See `fanstel ev-bt832x website`_ for more information about the development
board and `nRF52832 website`_ for the official reference on the IC itself.

Programming and Debugging
*************************

Flashing
========

Flashing Zephyr onto the ``fanstel_ev832x`` board requires an external
J-Link programmer. The programmer is attached to the JS1 SWD header.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application.

#. Build the Zephyr kernel and the :ref:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: fanstel_ev832x
      :goals: build
      :compact:

#. Connect the Fanstel EV-BT832X board to your host computer using USB

#. Run your favorite terminal program to listen for output.

   .. code-block:: console

      $ minicom -D <tty_device> -b 115200

   Replace :code:`<tty_device>` with the port where the Fanstel EV-BT832X
   board can be found. For example, under Linux, :code:`/dev/ttyUSB0`.

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: fanstel_ev832x
      :goals: flash
      :compact:

   You should see "Hello World! fanstel_ev832x" in your terminal.

Debugging
=========

The ``fanstel_ev832x`` board does not have an on-board J-Link debug IC
as some nRF5x development boards, however, instructions from the
:ref:`nordic_segger` page also apply to this board, with the additional step
of connecting an external debugger.

Testing the LEDs and buttons on the Fanstel EV-BT832X
**********************************************************

There are several samples that allow you to test that the buttons (switches) and
LEDs on the board are working properly with Zephyr:

- :ref:`blinky-sample`
- :ref:`button-sample`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/arm/fanstel_ev832x/board.h`.

References
**********
.. target-notes::

.. _nRF52832 website: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52832
.. _fanstel ev-bt832x website: https://www.fanstel.com/bt832x-bluetooth-5-module/
