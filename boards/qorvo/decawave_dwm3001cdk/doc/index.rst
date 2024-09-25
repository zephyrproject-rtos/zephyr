.. _decawave_dwm3001cdk:

Decawave DWM3001CDK
###################

Overview
********

The DWM3001CDK development board includes the DWM3001C module, battery connector
and charging circuit, LEDs, buttons, Raspberry Pi connector, and USB connector.
In addition, the board comes with J-Link OB adding debugging and Virtual COM
Port capabilities.

See `Qorvo (Decawave) DWM3001CDK website`_ for more information about the
development board, `Qorvo (Decawave) DWM3001C website`_ about the module
itself, and `nRF52833 website`_ for the official reference on the IC itself.

Programming and Debugging
*************************

Applications for the ``decawave_dwm3001cdk`` board target can be built, flashed,
and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

There are two USB ports, the one farthest from the DWM3001C is connected to the
J-Link debugger and the closer one is connected to the nRF52833, though you need
to use CDC ACM USB to get output over it

Here is an example for the :zephyr:code-sample:`usb-cdc-acm` application.

Connect to the bottom USB port, and flash the sample

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/console
   :board: decawave_dwm3001cdk
   :goals: build flash


Then, connect the top USB port and open run your favorite terminal program to
listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF52 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.


References
**********
.. target-notes::

.. _nRF52833 website: https://www.nordicsemi.com/products/nrf52833
.. _Qorvo (Decawave) DWM3001C website: https://www.qorvo.com/products/p/DWM3001C
.. _Qorvo (Decawave) DWM3001CDK website: https://www.qorvo.com/products/p/DWM3001CDK
