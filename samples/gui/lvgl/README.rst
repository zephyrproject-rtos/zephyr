.. _lvgl-sample:

LittlevGL Basic Sample
######################

Overview
********

This sample application displays "Hello World" in the center of the screen
and a counter at the bottom which increments every second.

Requirements
************

- `nRF52840 Preview development kit`_
- `Adafruit 2.2 inch TFT Display`_

or a simulated display environment in a native Posix application:

- :ref:`native_posix`
- `SDL2`_

or

- :ref:`mimxrt1050_evk`
- `RK043FN02H-CT`_

or

- :ref:`mimxrt1060_evk`
- `RK043FN02H-CT`_

Wiring
******

The nrf52840 Preview development kit should be connected as follows to the
Adafruit TFT display.

+-------------+----------------+
| | nrf52840  | | Adafruit TFT |
| | Pin       | | Pin          |
+=============+================+
| P0.27       | SCK            |
+-------------+----------------+
| P0.31       | D/C            |
+-------------+----------------+
| P0.30       | RST            |
+-------------+----------------+
| P0.26       | MOSI           |
+-------------+----------------+
| P0.29       | MISO           |
+-------------+----------------+
| P0.4        | NSS            |
+-------------+----------------+

Building and Running
********************

Build this sample using the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840_pca10056
   :goals: build
   :compact:

See :ref:`nrf52840_pca10056` on how to flash the build.

or

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: native_posix
   :goals: build
   :compact:

References
**********

.. target-notes::

.. _LittlevGL Web Page: https://littlevgl.com/
.. _Adafruit 2.2 inch TFT Display: https://www.adafruit.com/product/1480
.. _nRF52840 Preview development kit: http://www.nordicsemi.com/eng/Products/nRF52840-Preview-DK
.. _SDL2: https://www.libsdl.org
.. _RK043FN02H-CT: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/4.3-lcd-panel:RK043FN02H-CT
