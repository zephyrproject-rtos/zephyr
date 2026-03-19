.. zephyr:code-sample:: spis-async
   :name: SPI slave async callback
   :relevant-api: spi_interface

   Use spi_transceive_cb() to handle SPI slave transfers via a completion
   callback.

Overview
********

This sample demonstrates how to use the Zephyr SPI slave API with an
asynchronous completion callback via :c:func:`spi_transceive_cb`. This
is the Zephyr equivalent of registering an ``spis_event_handler`` and
checking ``NRF_DRV_SPIS_XFER_DONE`` in the legacy nrfx API (see
:github:`30375`).

The device arms the SPIS peripheral and returns immediately. When the SPI
master completes a transfer, the registered callback fires, signals a
semaphore, and the main thread wakes, logs the received data, and re-arms
for the next transfer. Zero-byte spurious completions are silently ignored.

Building and Running
********************

The sample requires a board overlay that defines a ``spis`` alias
pointing to a :dtcompatible:`nordic,nrf-spis` compatible node with pinctrl
configured for the target board. An overlay for the Seeed XIAO nRF52840
Sense is provided as a reference under ``boards/``.

To add support for another board, create
``boards/<board>.overlay`` with a ``nordic,nrf-spis`` node and
a ``spis`` alias, following the provided overlay as a template.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/spis_async
   :board: <your_board>
   :goals: build flash
   :compact:

Testing
*******

After flashing, open a serial terminal (115200 8N1). The sample logs confirm
the SPIS peripheral is initialized and armed:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   [00:00:00.301] <inf> spis_async: SPIS async sample started on <board>
   [00:00:00.301] <inf> spis_async: Waiting for SPI master transfer...

The device blocks in this state until a SPI master initiates a transfer.
Zero-byte spurious completions from floating pins are silently ignored and
the peripheral re-arms automatically.

Once a master sends data, the callback fires and the received bytes are logged:

.. code-block:: console

   [00:00:01.234] <inf> spis_async: Transfer complete, received 32 bytes: Hello SPIS!
   [00:00:01.234] <inf> spis_async: Waiting for SPI master transfer...

For a self-contained loopback test using both a SPIM and SPIS on the same
board, see ``tests/drivers/spi/spi_controller_peripheral``.
