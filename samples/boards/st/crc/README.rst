.. zephyr:code-sample:: stm32_crc
   :name: Cyclic Redundancy Check (CRC)

   Get the CRC code of a data buffer.

Overview
********
Very basic example to calculate the CRC of a data buffer,
using the stm32Cube HAL functions directly in a zephyr application.

Requires a Kconfig to enable the corresponding stm32 HAL driver
 - USE_STM32_HAL_CRC
 - USE_STM32_HAL_CRC_EX

Because there is no CRC node in the stm32 device, the CRC instance is initialized with HAL function.
The sample application, enables the clock of the CRC peripheral and initialize it.
Then the CRC is computed for a given data buffer and result is compared to the know expected value.

This code is given as example, coming from the `STM32CubeMX`_ .


Building and Running
********************

In order to run this sample, just


 .. zephyr-app-commands::
   :zephyr-app: samples/boards/st/crc
   :board: b_u585i_iot02a
   :goals: build
   :compact:

Sample Output
=============
The sample gives the calculated CRC value:

.. code-block:: console

    CRC computation
    ===============

    CRC = 0x379e9f06


.. _STM32CubeMX:
   https://www.st.com/en/development-tools/stm32cubemx.html
