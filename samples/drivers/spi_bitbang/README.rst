.. zephyr:code-sample:: spi-bitbang
   :name: SPI bitbang
   :relevant-api: spi_interface

   Use the bitbang SPI driver for communicating with a slave.

Overview
********

This sample demonstrates using the bitbang SPI driver. The bitbang driver can
be useful for devices which use a non multiple of 8 word size, for example some
LCDs which have an extra cmd/data bit.

This sample loops through some different spi transfer configurations.


Building and Running
********************

The application will build only for a target that has a :ref:`devicetree
<dt-guide>` entry with :dtcompatible:`zephyr,spi-bitbang` as a compatible.

You can connect the MISO and MOSI pins with a wire to provide a basic loopback
test for receive data.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_bitbang
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

  *** Booting Zephyr OS build zephyr-v2.6.0-2939-g1882b95b42e2  ***
  basic_write_9bit_words; ret: 0
    wrote 0101 00ff 00a5 0000 0102
  9bit_loopback_partial; ret: 0
   tx (i)  : 0101 0102
   tx (ii) : 0003 0004 0105
   rx (ii) : 0003 0004 0105
  basic_write_9bit_words; ret: 0
   wrote 0101 00ff 00a5 0000 0102
  9bit_loopback_partial; ret: 0
   tx (i)  : 0101 0102
   tx (ii) : 0003 0004 0105
   rx (ii) : 0003 0004 0105
  basic_write_9bit_words; ret: 0
   wrote 0101 00ff 00a5 0000 0102
  9bit_loopback_partial; ret: 0
   tx (i)  : 0101 0102
   tx (ii) : 0003 0004 0105
   rx (ii) : 0003 0004 0105
