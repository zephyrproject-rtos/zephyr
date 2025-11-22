.. zephyr:code-sample:: gui_guider
   :name: GUI Guider basic sample
   :relevant-api: display_interface input_interface

   Display a slider widget.

Overview
********

This sample application displays a slider in the top left corner of the screen.

Requirements
************

Display shield and a board which provides a configuration
for corresponding connectors, for example:

a simulated display environment in a :zephyr:board:`native_sim <native_sim>` application:

- :zephyr:board:`native_sim`
- `SDL2`_

or

- :zephyr:board:`mimxrt1050_evk`
- `RK043FN02H-CT`_

or

- :zephyr:board:`mimxrt1064_evk`
- `RK043FN02H-CT`_

Building and Running
********************

Example building for :zephyr:board:`mimxrt1064_evk`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/gui_guider
   :board: mimxrt1064_evk
   :shield: rk043fn02h_ct
   :goals: build flash

Example building for :zephyr:board:`native_sim <native_sim>`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/gui_guider
   :board: native_sim
   :goals: build run

Alternatively, if building from a 64-bit host machine, the previous target
board argument may also be replaced by ``native_sim/native/64``.

References
**********

.. target-notes::

.. _LVGL Web Page: https://lvgl.io/
.. _SDL2: https://www.libsdl.org
.. _RK043FN02H-CT: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/4.3-lcd-panel:RK043FN02H-CT
