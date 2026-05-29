.. zephyr:board:: decawave_dwm1001_dev

Overview
********

The DWM1001 development board includes the DWM1001 module, battery
connector and charging circuit, LEDs, buttons, Raspberry-Pi and USB
connector. In addition, the board comes with J-Link OB adding
debugging and Virtual COM Port capabilities.

See `Qorvo (Decawave) DWM1001-DEV website`_ for more information about the development
board, `Qorvo (Decawave) DWM1001 website`_ about the board itself, and `nRF52832 website`_ for the
official reference on the IC itself.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``decawave_dwm1001_dev`` board configuration can be built,
flashed, and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF52 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: decawave_dwm1001_dev
   :goals: build flash

References
**********
.. target-notes::

.. _nRF52832 website: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52832
.. _Qorvo (Decawave) DWM1001 website: https://www.qorvo.com/products/p/DWM1001C
.. _Qorvo (Decawave) DWM1001-DEV website: https://www.qorvo.com/products/p/DWM1001-DEV
