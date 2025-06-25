.. _openthread_rcp_arduino_shield:

OpenThread RCP over Arduino header
##################################

Overview
********

This (virtual) shield can be used to connect a board with an Arduino R3 compatible header to an
external `OpenThread RCP`_ device. The RCP device would function as the Thread radio, while another
board can function as the OpenThread host.

Requirements
************

An RCP radio device is needed for this shield to work. As an example, the reference from
OpenThread using the :zephyr:board:`nrf52840dk` is chosen as a demonstration. Refer to the
`OpenThread on nRF52840 Example website`_.

Both UART and SPI can be used as the transport, depending on the board connections.

The following was executed on Ubuntu 24.04 to build and flash the RCP firmware:

Preparation
===========

.. code-block:: bash

   git clone https://github.com/openthread/ot-nrf528xx.git --recurse-submodules
   cd ot-nrf528xx
   python3 -m venv .venv
   source .venv/bin/activate
   ./script/bootstrap

Building
========

.. tabs::

   .. group-tab:: UART

      .. code-block:: bash

         # Set -DOT_PLATFORM_DEFINES="UART_HWFC_ENABLED=1" to enable flow control
         ./script/build nrf52840 UART_trans -DOT_PLATFORM_DEFINES="UART_HWFC_ENABLED=0"

   .. group-tab:: SPI

      .. code-block:: bash

         ./script/build nrf52840 SPI_trans_NCP

Flashing
========

.. code-block:: bash

   arm-none-eabi-objcopy -O ihex build/bin/ot-rcp build/bin/ot-rcp.hex
   nrfjprog -f nrf52 --chiperase --program build/bin/ot-rcp.hex --reset

Pins Assignments
================

The RCP firmware comes with default pins assigned, the following table lists both the Arduino header
pins and the nRF52840DK pins.

.. tabs::

   .. group-tab:: UART

      +-----------------------+-----------------------+-----------------------+
      | Arduino Header Pin    | Function (host)       | nRF52840 DK Pin       |
      +=======================+=======================+=======================+
      | D0                    | UART RX               | P0.06                 |
      +-----------------------+-----------------------+-----------------------+
      | D1                    | UART TX               | P0.08                 |
      +-----------------------+-----------------------+-----------------------+
      | Host specific         | UART CTS              | P0.05 (flow control)  |
      +-----------------------+-----------------------+-----------------------+
      | Host specific         | UART RTS              | P0.07 (flow control)  |
      +-----------------------+-----------------------+-----------------------+

   .. group-tab:: SPI

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

Include ``--shield openthread_rcp_arduino_serial`` or ``--shield openthread_rcp_arduino_spi``
when you invoke ``west build`` for projects utilizing this shield. For example:

.. tabs::

   .. group-tab:: UART

      .. zephyr-app-commands::
         :zephyr-app: samples/net/sockets/echo_client
         :board: stm32h573i_dk/stm32h573xx
         :shield: openthread_rcp_arduino_serial
         :conf: "prj.conf overlay-ot-rcp-host-uart.conf"
         :goals: build

   .. group-tab:: SPI

      .. zephyr-app-commands::
         :zephyr-app: samples/net/sockets/echo_client
         :board: stm32h573i_dk/stm32h573xx
         :shield: openthread_rcp_aduino_spi
         :conf: "prj.conf overlay-ot-rcp-host-uart.conf"
         :goals: build

References
**********

.. target-notes::

.. _OpenThread RCP:
   https://openthread.io/platforms/co-processor

.. _OpenThread on nRF52840 Example website:
   https://github.com/openthread/ot-nrf528xx/blob/main/src/nrf52840/README.md
