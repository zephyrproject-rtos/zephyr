.. zephyr:code-sample:: sc18is60x
   :name: SC18IS60x I2C-to-SPI bridge
   :relevant-api: spi_interface

   Talk to an SPI slave through the NXP SC18IS60x I2C-to-SPI bridge
   (I2C to SPI Click).

Overview
********

This sample waits until the SC18IS60x MFD parent and SPI child are ready,
then performs one full-duplex SPI transaction through the bridge: it writes a
known 8-byte pattern and reads back the same number of bytes. The data is
exchanged on the Click's extra SPI header, not on the host MCU's SPI
controller.

If the board provides a ``sc18is60x_target_spi`` nodelabel that points to an
on-chip SPI controller configured as a serial-target (peripheral), the sample also
starts a second thread on that controller and sends the same pattern from the
bridge master. This lets you test the bridge master path with the MCU itself as
the SPI target. Wire the Click extra SPI header to the MCU SPI pins.

If the MFD node has an ``int-gpios`` property, the driver waits for the
bridge's completion interrupt before the I2C read.

Requirements
************

* A board with a mikroBUS™ socket that defines ``mikrobus_i2c`` and
  ``mikrobus_header``
* :ref:`mikroe_i2c_to_spi_click_shield` (SC18IS602B, default I2C address
  ``0x28`` — confirm the address jumpers on the Click)
* An SPI slave connected to the Click extra SPI header, or a test jumper
  from MOSI to MISO for an echo check

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi/sc18is60x
   :board: <board>
   :shield: mikroe_i2c_to_spi_click
   :goals: build flash

Expected console output includes the MFD and SPI child names, hex dumps
of the transmitted and received data, and a line indicating whether the
received data matches the transmitted pattern.

Shell bring-up
**************

To probe the Click without this sample, build the shell with I2C and SPI
shell commands:

.. code-block:: console

   west build -b <your-board> \
     --shield mikroe_i2c_to_spi_click samples/subsys/shell/shell_module \
     -- -DCONFIG_I2C_SHELL=y -DCONFIG_SPI_SHELL=y

On the shell, ``i2c scan`` should list ``0x28``. Then ``device list``
should include the ``sc18is60x`` SPI child (``spi_click``).
