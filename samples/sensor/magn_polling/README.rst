.. zephyr:code-sample:: magn_polling
   :name: Magnetometer Sensor
   :relevant-api: sensor_interface

   Get magnetometer data from a magnetometer sensor (polling mode).

Overview
********

Sample application that periodically reads magnetometer (X, Y, Z) data from
the first available device that implements SENSOR_CHAN_MAGN_* (predefined array
of device names).

Board-specific overlays
***********************

TMAG5170 via Raspberry Pi Pico
==============================

The Zephyr driver for the :dtcompatible:`ti,tmag5170`` requires an SPI driver
that supports 32-bit SPI_WORD_SIZE.  On the :zephyr:board:`rpi_pico`, the
:dtcompatible:`raspberrypi,pico-spi-pio` SPI driver provides this support,
demonstrated with the
:zephyr_file:`samples/sensor/magn_polling/boards/rpi_pico.overlay`.

The GPIO pin assignments in the overlay file are arbitrary.  The PIO SPI
driver allows using any four GPIO pins for the SPI bus.  Just keep in mind
that the pin assignments in the pinctrl block and the pio0_spi0 block
must match.

With the sensor wired to the desired pins, build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/magn_polling
   :goals: build flash
   :board: rpi_pico
