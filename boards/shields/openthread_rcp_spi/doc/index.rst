.. _openthread_rcp_spi_shield:

OpenThread RCP over SPI Shield
##############################

Overview
********

This (virtual) shield can be used to connect a board with an Arduino R3 compatible header to an
external `OpenThread RCP`_ device. The RCP device would function as the Thread radio, while another
board can function as the OpenThread host.

Requirements
************

An RCP over SPI radio device is needed for this shield to work. As an example, the reference from
OpenThread using the :zephyr:board:`nrf52840dk` is chosen as a demonstration. Refer to the
`OpenThread on nRF52840 Example website`_.

The following was executed on Ubuntu 24.04 to build and flash the RCP firmware:

.. code-block:: bash

   git clone https://github.com/openthread/ot-nrf528xx.git --recurse-submodules
   cd ot-nrf528xx
   python3 -m venv .venv
   source .venv/bin/activate
   ./script/bootstrap
   ./script/build nrf52840 SPI_trans_NCP
   arm-none-eabi-objcopy -O ihex build/bin/ot-rcp build/bin/ot-rcp.hex
   nrfjprog -f nrf52 --chiperase --program build/bin/ot-rcp.hex --reset

Pins Assignments
================

The RCP firmware comes with default pins assigned, the following table lists both the Arduino header
pins and the nRF52840DK pins.

+-----------------------+-----------------------+-----------------------+
| Arduino Header Pin    | Function              | nRF52840 DK Pin       |
+=======================+=======================+=======================+
| D8                    | RSTn                  | P0.18/RESET           |
+-----------------------+-----------------------+-----------------------+
| D9                    | INTn                  | P0.30                 |
+-----------------------+-----------------------+-----------------------+
| D10                   | SPI CSn               | P0.29                 |
+-----------------------+-----------------------+-----------------------+
| D11                   | SPI MOSI              | P0.04                 |
+-----------------------+-----------------------+-----------------------+
| D12                   | SPI MISO              | P0.28                 |
+-----------------------+-----------------------+-----------------------+
| D13                   | SPI SCK               | P0.03                 |
+-----------------------+-----------------------+-----------------------+

Programming
***********

Include ``--shield openthread_rcp_spi`` when you invoke ``west build``
for projects utilizing this shield. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: stm32h573i_dk/stm32h573xx
   :shield: openthread_rcp_spi
   :conf: "prj.conf overlay-ot-rcp-host-uart.conf"
   :goals: build

References
**********

.. target-notes::

.. _OpenThread RCP:
   https://openthread.io/platforms/co-processor

.. _OpenThread on nRF52840 Example website:
   https://github.com/openthread/ot-nrf528xx/blob/main/src/nrf52840/README.md
