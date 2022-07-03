.. _led_max7219_sample:

MAX7219 Sample Application
##########################

Overview
********

This sample application demonstrates basic usage of the MAX7219 LED
driver. The first "maxim,max7219" compatible device instance found in
DT is used.

Building and Running
********************

Build the application for the :ref:`stm32_min_dev` board, and connect
a MAX7219 LED driver on the bus SPI1.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_max7219
   :board: stm32_min_dev_black
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`stm32_min_dev` board documentation.

Sample output
=============

You should get a similar output as below.

.. code-block:: console

  *** Booting Zephyr OS build zephyr-v3.1.0-1518-g8ed358cfd685  ***
  [00:00:00.006,000] <inf> app: Displaying the pattern

References
**********

- `MAX7219 datasheet <https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf>`_
