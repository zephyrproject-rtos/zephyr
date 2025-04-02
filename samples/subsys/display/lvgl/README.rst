.. zephyr:code-sample:: lvgl
   :name: LVGL basic sample
   :relevant-api: display_interface input_interface

   Display a "Hello World" and react to user input using LVGL.

Overview
********

This sample application displays "Hello World" in the center of the screen
and a counter at the bottom which increments every second.
Based on the available input devices on the board used to run the sample,
additional widgets may be displayed and additional interactions enabled:

* Pointer
      If your board has a touch panel controller
      (:dtcompatible:`zephyr,lvgl-pointer-input`), a button widget is displayed
      in the center of the screen. Otherwise a label widget is displayed.
* Button
      The button pseudo device (:dtcompatible:`zephyr,lvgl-button-input`) maps
      a press/release action to a specific coordinate on screen. In the case
      of this sample, the coordinates are mapped to the center of the screen.
* Encoder
      The encoder pseudo device (:dtcompatible:`zephyr,lvgl-encoder-input`)
      can be used to navigate between widgets and edit their values. If the
      board contains an encoder, an arc widget is displayed, which can be
      edited.
* Keypad
      The keypad pseudo device (:dtcompatible:`zephyr,lvgl-keypad-input`) can
      be used for focus shifting and also entering characters inside editable
      widgets such as text areas. If the board used with this sample has a
      keypad device, a button matrix is displayed at the bottom of the screen
      to showcase the focus shifting capabilities.

Requirements
************

Display shield and a board which provides a configuration
for corresponding connectors, for example:

- :ref:`adafruit_2_8_tft_touch_v2` and :zephyr:board:`nrf52840dk`
- :ref:`buydisplay_2_8_tft_touch_arduino` and :zephyr:board:`nrf52840dk`
- :ref:`ssd1306_128_shield` and :zephyr:board:`frdm_k64f`
- :ref:`seeed_xiao_round_display` and :zephyr:board:`xiao_ble`

or a board with an integrated display:

- :zephyr:board:`esp_wrover_kit`
- :zephyr:board:`adafruit_feather_esp32s3_tft`

or a simulated display environment in a :ref:`native_sim <native_sim>` application:

- :ref:`native_sim`
- `SDL2`_

or

- :zephyr:board:`mimxrt1050_evk`
- `RK043FN02H-CT`_

or

- :zephyr:board:`mimxrt1060_evk`
- `RK043FN02H-CT`_

Building and Running
********************

Example building for :zephyr:board:`nrf52840dk`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: nrf52840dk/nrf52840
   :shield: adafruit_2_8_tft_touch_v2
   :goals: build flash

Example building for :ref:`native_sim <native_sim>`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
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
