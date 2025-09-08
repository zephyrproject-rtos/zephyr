.. zephyr:code-sample:: dali
   :name: Digital Addressable Lighting Interface (DALI)

   Blink LED controllers connected to a DALI bus.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to blink DALI enabled LED drivers.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree.

The board's devictree must have a ``dali0`` node that provides the
access to the DALI bus. See the predefined overlays in
:zephyr_file:`samples/drivers/dali/boards` for examples.

.. note:: For proper operation a DALI specific physical interface is required.

Building and Running for ST Nucleo F091RC
=========================================
The :zephyr_file:`samples/drivers/dali/boards/nucleo_f091rc.overlay`
is specifically for the Mikroe-2672 DALI2 click development board
used as physical interface to the DALI bus. This board uses negative
logic for signal transmission (Tx Low <-> DALI Bus Idle).
The sample can be build and executed for the
:zephyr:board:`nucleo_f091rc` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali
   :board: nucleo_f091rc
   :goals: build flash
   :compact:

Building and Running for Nordic nRF52840
============================================
The :zephyr_file:`samples/drivers/dali/boards/nrf52840dk_nrf52840.overlay`
is specifically for the Mikroe-2672 DALI2 click development board
used as physical interface to the DALI bus. The pin assignment
supports the use of an Arduino UNO click shield.
The click board uses negative logic for signal transmission
(Tx Low <-> DALI Bus Idle).
The sample can be build and executed for the
:zephyr:board:`nrf52840dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali
   :board: nrf52840dk
   :goals: build flash
   :compact:

Sample outout
=============

You should see DALI frames transferred every 2 seconds to the DALI bus.
The frames are alternating. One frame is a DALI OFF command broadcasted to
all control gears. The other frame is a DALI RECALL MAX command broadcasted
to all control gears. When a control gear is connected it will alternate
between no light output from the attached LED and maximum output of the LED.
