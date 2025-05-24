.. zephyr:board:: curiosity_nano_mc

Overview
********

The PIC32CM1216MC00032 Curiosity Nano Evaluation Kit (EV10N93A) is a hardware
platform used to evaluate the PIC32CM MC microcontroller (MCU).
More information can be found on `Microchip's website`_ and on the
`Users Guide`_.

Hardware
********

- PIC32CM1216MC00032 ARM Cortex-M0+ processor at 48 MHz
- One yellow user LED
- One mechanical user switch
- On-board debugger

Supported Features
==================

.. zephyr:board-supported-hw::

Refer to the `EV10N93A Documentation`_ for a detailed evaluation kit design
details.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

``pyocd`` does not natively support PIC32CM devices. Install the required pack
with this command:

.. code-block:: console

   $ pyocd pack install pic32cm1216mc00032

Applications for the ``curiosity_nano_mc`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

Here is an example for the :zephyr:code-sample:`blinky` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF52840 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: curiosity_nano_mc
   :goals: build flash


References
**********

.. target-notes::

.. _Microchip's website:
    https://www.microchip.com/en-us/development-tool/ev10n93a

.. _Users Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CM_MC00_Curiosity_Nano_Users_Guide_DS70005445A.pdf

.. _EV10N93A Documentation:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/BoardDesignFiles/PIC32CM-MC-Curiosity-Nano-Evaluation-Kit-Design-Documentation.zip
