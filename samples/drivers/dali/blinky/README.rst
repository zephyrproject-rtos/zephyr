.. zephyr:code-sample:: dali_blinky
   :name: Digital Addressable Lighting Interface (DALI)

   Blink LED controllers connected to a DALI bus.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to blink DALI enabled LED drivers.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree.

The devicetree must have a ``dali`` alias that provides the access to the DALI bus. The
:ref:`mikroe_dali_2_click_shield` defines such a node. See the board overlays in
:zephyr_file:`boards/shields/mikroe_dali_2_click/boards` for examples.

.. note:: For proper operation a DALI specific physical interface is required.

Building and Running for ST Nucleo F091RC
=========================================
The :ref:`mikroe_dali_2_click_shield` is used as physical interface to the DALI bus. The board
specific settings are provided by the shield in
:zephyr_file:`boards/shields/mikroe_dali_2_click/boards/nucleo_f091rc.overlay`. The click board
uses negative logic for signal transmission (Tx Low <-> DALI Bus Idle).
The sample can be build and executed for the :zephyr:board:`nucleo_f091rc` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/blinky
   :board: nucleo_f091rc
   :goals: build flash
   :shield: arduino_uno_click,mikroe_dali_2_click
   :compact:

Building and Running for Nordic nRF52840
========================================
The :ref:`mikroe_dali_2_click_shield` is used as physical interface to the DALI bus. The board
specific settings are provided by the shield in
:zephyr_file:`boards/shields/mikroe_dali_2_click/boards/nrf52840dk_nrf52840.overlay`. The pin
assignment supports the use of an Arduino UNO click shield. The click board uses negative logic
for signal transmission (Tx Low <-> DALI Bus Idle).
The sample can be build and executed for the :zephyr:board:`nrf52840dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/blinky
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :shield: arduino_uno_click,mikroe_dali_2_click
   :compact:

Sample output
=============

You should see DALI frames transferred every 2 seconds to the DALI bus.
The frames are alternating. One frame is a DALI OFF command broadcasted to
all control gears. The other frame is a DALI RECALL MAX command broadcasted
to all control gears. When a control gear is connected it will alternate
between no light output from the attached LED and maximum output of the LED.
