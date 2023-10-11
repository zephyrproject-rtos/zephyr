.. _qorvo_dwm3001cdk:

Qorvo DWM3001CDK
#################

Overview
********

The DWM3001CDK development board includes the DWM3001C module, battery
connector and charging circuit, LEDs, buttons, Raspberry-Pi an IMU, and USB
connector. In addition, the board comes with J-Link OB adding
debugging and Virtual COM Port capabilities.

Programming and Debugging
*************************

Applications for the ``qorvo_dwm3001cdk`` board configuration can be built,
flashed, and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

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

Replace :code:`<tty_device>` with the port where the board nRF52 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: qorvo_dwm3001cdk
   :goals: build flash

References
**********
.. target-notes::

.. _nRF52833 website: https://www.nordicsemi.com/products/nrf52833
.. _Qorvo DWM3000 website: https://www.qorvo.com/products/p/DWM3000
.. _Qorvo DWM3000C website: https://www.qorvo.com/products/p/DWM3000C
.. _Qorvo DWM3001CDK website: https://www.qorvo.com/products/p/DWM3001CDK
