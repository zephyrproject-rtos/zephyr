.. _modbus-rtu-client-sample:

Modbus RTU Client Sample
########################

Overview
********

This is a simple application demonstrating a Modbus RTU client implementation
in Zephyr RTOS.

Requirements
************

This sample has been tested with the nRF52840-DK and FRDM-K64F boards,
but it should work with any board that has a free UART interface.

RTU client example is running on an evaluation board and communicates
with another board that has been prepared according to the description
`modbus-rtu-server-sample`.

In addition to the evaluation board a RS-485 shield may be used.
The shield converts UART TX, RX signals to RS-485.
An Arduino header compatible shield like `joy-it RS-485 shield for Arduino`_
can be used. This example uses DE signal, which is controlled by pin D9
on the JOY-IT shield. For other shields, ``de-gpios`` property must be adapted
or removed in the application overlay file
:zephyr_file:`samples/subsys/modbus/rtu_client/app.overlay`

The A+, B- lines of the RS-485 shields should be connected together.

Alternatively UART RX,TX signals of two boards can be connected crosswise.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/subsys/modbus/rtu_client` in the Zephyr tree.

The following commands build and flash RTU client sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/modbus/rtu_client
   :board: frdm_k64f
   :goals: build flash
   :compact:

The example communicates with the RTU server and lets the LEDs light up
one after the other.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.3.0-1993-g07e8d80ae028  ***
   [00:00:00.005,000] <inf> mb_rtu: RTU timeout 2005 us
   [00:00:00.050,000] <inf> mbc_sample: WR|RD holding register:
   48 00 65 00 6c 00 6c 00  6f 00 00 00 00 00 00 00 |H.e.l.l. o.......
   [00:00:00.062,000] <inf> mbc_sample: Coils state 0x00
   [00:00:00.864,000] <inf> mbc_sample: Coils state 0x07


.. _`joy-it RS-485 shield for Arduino`: https://joy-it.net/en/products/ARD-RS485
