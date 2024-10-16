.. zephyr:code-sample:: stm32_crc_poly
   :name: Cyclic Redundancy Check Polynomial (CRC)

   Get the 8-bit CRC code of a data buffer.

Overview
********
Very basic example to calculate the CRC of a data buffer, using the STM32 CRC LL API.
This sample describes how to use CRC peripheral for generating 8-bit CRC value
for an input data Buffer, based on a user defined polynomial value,
Peripheral initialization done using LL unitary services functions.

The CRC clock enable on the RCC register might differ depending on the stm32 serie.

This code is given as example, coming from the `STM32CubeMX`_ .


Building and Running
********************

In order to run this sample, just


 .. zephyr-app-commands::
   :zephyr-app: samples/boards/st/crc_poly
   :board: b_u585i_iot02a
   :goals: build
   :compact:

Sample Output
=============
The sample gives the calculated CRC value:

.. code-block:: console

    CRC computation
    ===============

    CRC = 0xc


.. _STM32CubeMX:
   https://www.st.com/en/development-tools/stm32cubemx.html
