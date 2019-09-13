.. _lvgl-sample:

LittlevGL Basic Sample
######################

Overview
********

This sample application displays "Hello World" in the center of the screen
and a counter at the bottom which increments every second.

Requirements
************

Display shield and a board which provides a configuration
for Arduino connectors, for example:

- :ref:`adafruit_2_8_tft_touch_v2` and :ref:`nrf52840_pca10056`
- :ref:`ssd1306_128x64_shield` and :ref:`frdm_k64f`

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

Example building for :ref:`nrf52840_pca10056`:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840_pca10056
   :shield: adafruit_2_8_tft_touch_v2
   :goals: build flash

Example building for :ref:`native_posix`:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: native_posix
   :goals: build flash

References
**********

.. target-notes::

.. _LittlevGL Web Page: https://littlevgl.com/
.. _SDL2: https://www.libsdl.org
.. _RK043FN02H-CT: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/4.3-lcd-panel:RK043FN02H-CT
