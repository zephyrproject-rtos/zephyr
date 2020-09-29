.. _lvgl-sample:

LittlevGL Basic Sample
######################

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

.. note::
   When deferred logging is enabled you will likely need to increase
   :option:`CONFIG_LOG_STRDUP_BUF_COUNT` and/or
   :option:`CONFIG_LOG_STRDUP_MAX_STRING` to make sure no messages are lost or
   truncated.

Example building for :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840dk_nrf52840
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
