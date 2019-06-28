.. _ili9340-sample:

ILI9340 Display driver
######################

Overview
********

Every half-second, this sample application draws a color-filled rectangle in a
corner of the LCD display.  The rectangle fill color cycles through red, green,
and blue on each update.

Requirements
************

- `ST NUCLEO-L476RG development board`_
- `Adafruit 2.2 inch TFT Display`_

or

- :ref:`nrf52_pca10040`
- `Seeed 2.8 inch TFT Touch Shield V2.0`_

or

- :ref:`nrf52840_pca10056`
- `Adafruit 2.8 inch TFT Touch Shield`_

Wiring
******

The NUCLEO-L476RG should be connected as follows to the Adafruit TFT display.

+------------------+-----------------+----------------+
| | NUCLEO-L476RG  | | NUCLEO-L476RG | | Adafruit TFT |
| | Arduino Header | | Pin           | | Pin          |
+==================+=================+================+
| D3               | PB3             | SCK            |
+------------------+-----------------+----------------+
| D7               | PA8             | D/C            |
+------------------+-----------------+----------------+
| D8               | PA9             | RST            |
+------------------+-----------------+----------------+
| D11              | PA7             | MOSI           |
+------------------+-----------------+----------------+
| D12              | PA6             | MISO           |
+------------------+-----------------+----------------+
| A2               | PA4             | NSS            |
+------------------+-----------------+----------------+

The Seeed 2.8 inch TFT Touch Shield V2.0 should be plugged in the Arduino
header on :ref:`nrf52_pca10040`. The following pins will be connected except
the TFT reset pin. A separate wire should connect P0.21 pin to RESET pin on
the :ref:`nrf52_pca10040`.

+-------------+-------------+
| | nRF52832  | | Seeed TFT |
| | Pin       | | Pin       |
+=============+=============+
| P0.25       | SPI_SCK     |
+-------------+-------------+
| P0.23       | SPI_MOSI    |
+-------------+-------------+
| P0.24       | SPI_MISO    |
+-------------+-------------+
| P0.16       | TFT_CS      |
+-------------+-------------+
| P0.17       | TFT_DC      |
+-------------+-------------+
| P0.21       | RESET       |
+-------------+-------------+

The Adafruit 2.8 inch TFT Touch Shield should be plugged in the Arduino header
on :ref:`nrf52840_pca10056`. The following pins will be connected.

+-------------+----------------+
| | nRF52840  | | Adafruit TFT |
| | Pin       | | Pin          |
+=============+================+
| P1.15       | SCLK           |
+-------------+----------------+
| P1.13       | MOSI           |
+-------------+----------------+
| P1.14       | MISO           |
+-------------+----------------+
| P1.12       | TFT_CS         |
+-------------+----------------+
| P1.11       | TFT_DC         |
+-------------+----------------+

Building and Running
********************

For NUCLEO-L476RG, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/display/ili9340
   :board: nucleo_l476rg
   :goals: build
   :compact:

See :ref:`nucleo_l476rg_board` on how to flash the build.

For :ref:`nrf52_pca10040`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/display/ili9340
   :board: nrf52_pca10040
   :goals: build
   :compact:

See :ref:`nrf52_pca10040` on how to flash the build.

For :ref:`nrf52840_pca10056`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/display/ili9340
   :board: nrf52840_pca10056
   :goals: build
   :compact:

See :ref:`nrf52840_pca10056` on how to flash the build.

References
**********

- `ILI9340 datasheet`_
- `ILI9341 datasheet`_

.. _Adafruit 2.2 inch TFT Display: https://www.adafruit.com/product/1480
.. _ST NUCLEO-L476RG development board: http://www.st.com/en/evaluation-tools/nucleo-l476rg.html
.. _Seeed 2.8 inch TFT Touch Shield V2.0: https://www.seeedstudio.com/2-8-TFT-Touch-Shield-V2-0-p-1286.html
.. _Adafruit 2.8 inch TFT Touch Shield: https://www.adafruit.com/product/1947
.. _ILI9340 datasheet: https://cdn-shop.adafruit.com/datasheets/ILI9340.pdf
.. _ILI9341 datasheet: https://www.newhavendisplay.com/app_notes/ILI9341.pdf
