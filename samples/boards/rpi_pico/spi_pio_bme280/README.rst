.. _spi pio:

Raspberry Pi Pico PIO SPI Sample
################################

Overview
********

This sample, adapted from zephyr/samples/sensor/bme280, demonstrates using the
spi_rpi_pico_pio driver for the Raspberry Pi Pico.  This driver makes use
of the RP2040 PIO periperals to accommodate needs that are not met by the built-in
SPI peripherals.

The source for this sample is unchanged from the original.  The only changes are
the addition of boards/rpi_pico.overlay, which defines the configuration for using
the PIO SPI driver, and adding "CONFIG_SPI=y" to prj.conf to select the SPI interface
for the :ref:`bme280` driver.

The GPIO pin assignments are arbitrary:  any GPIO pins may be assigned for the four
SPI lines, MOSI, MISO, CLK, and CS.  The pins used in boards/rpi_pico.overlay just
happened to be the way I connected my breadboard.

Note that the MOSI, MISO, and CLK pins must be assigned to the chosen PIO device in
the pinctrl definition;  the CS pin must *not* be assigned to PIO since it will be
controlled as an ordinary I/O pin by spi_context.

Like the original sample, this sample periodically reads temperature, pressure and
humidity data from the first available BME280 device discovered in the system. The
sample checks the sensor in polling mode (without interrupt trigger).

Building and Running
********************

The pin assignments in the provided boards/rpi_pico.overlay are:

.. list-table:: Pin Assignments
   :widths: auto
   :header-rows: 1

   * - Function
     - Pin
   * - MOSI
     - 15
   * - MISO
     - 12
   * - CLK
     - 14
   * - CS
     - 13

As mentioned above, these assignments are completely arbitrary, and can be
reassigned to suit any arrangement of the four lines.

The sample can be built and executed on a Raspberry Pi Pico as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/rpi_pico/spi_pio_bme280
   :board: rpi_pico
   :goals: build flash
