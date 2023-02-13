.. _lvgl-sample:

LVGL Basic Sample
#################

Overview
********

This sample application displays "Hello World" in the center of the screen
and a counter at the bottom which increments every second. If an input driver
is supported, such as the touch panel controller on mimxrt10{50,60,64}_evk
boards, "Hello World" is enclosed in a button that changes to the toggled state
when touched.

Requirements
************

Display shield and a board which provides a configuration
for Arduino connectors, for example:

- :ref:`adafruit_2_8_tft_touch_v2` and :ref:`nrf52840dk_nrf52840`
- :ref:`buydisplay_2_8_tft_touch_arduino` and :ref:`nrf52840dk_nrf52840`
- :ref:`ssd1306_128_shield` and :ref:`frdm_k64f`

or a board with an integrated display:

- :ref:`esp_wrover_kit`

or a simulated display environment in a native Posix application:

- :ref:`native_posix`
- `SDL2`_

or

- :ref:`mimxrt1050_evk`
- `RK043FN02H-CT`_

or

- :ref:`mimxrt1060_evk`
- `RK043FN02H-CT`_

Building and Running
********************

Example building for :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: nrf52840dk_nrf52840
   :shield: adafruit_2_8_tft_touch_v2
   :goals: build flash

Example building for :ref:`native_posix`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: native_posix
   :goals: build run

Alternatively, if building from a 64-bit host machine, the previous target
board argument may also be replaced by ``native_posix_64``.

References
**********

.. target-notes::

.. _LVGL Web Page: https://lvgl.io/
.. _SDL2: https://www.libsdl.org
.. _RK043FN02H-CT: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/4.3-lcd-panel:RK043FN02H-CT
