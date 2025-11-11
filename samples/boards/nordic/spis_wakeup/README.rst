.. zephyr:code-sample:: spis-wakeup
   :name: SPIS wake up
   :relevant-api: spi_interface

   Reduce current consumption by handling the wake line while using an SPIS.

Overview
********

Sample showing how to use the additional wake line in nrf-spis driver. The application, configured
as a SPIS, is put to sleep while waiting for an SPI transmission. An external device (other core in
this sample) working as a SPIM and also handling the wake line wakes the application up just before
the transmission starts. To simulate two separate devices the Nordic Semiconductor nRF54H20 DK has
been used and SPIS / SPIM drivers are controlled by two independent cores:

- nrf54h20dk/nrf54h20/cpuapp works as a SPIS controller device
- nrf54h20dk/nrf54h20/cpurad works as a SPIM controller device


Requirements
************

This sample can be run on multicore SoCs like nRF54H20 and requires additional wiring as below.

Wiring
******

Wires (jumpers) for connecting SPIS and SPIM devices are needed:

- SPIS WAKE <-> WAKE SPIM
- SPIS CS <-----> CS SPIM
- SPIS SCK <---> SCK SPIM
- SPIS MOSI <-> MOSI SPIM
- SPIS MISO <-> MISO SPIM

Building and Running
********************

The code can be found in :zephyr_file:`samples/boards/nordic/spis_wakeup`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/spis_wakeup
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: build flash
   :compact:

Sample Output
=============

nrf54h20/cpuapp:

.. code-block:: console

    [00:00:00.272,217] <inf> spi_wakeup: Hello world from nrf54h20dk@0.9.0/nrf54h20/cpuapp
    [00:00:00.272,241] <inf> spi_wakeup: SPIS: waiting for SPI transfer; going to sleep...
    [00:00:02.274,192] <inf> spi_wakeup: SPIS: woken up by nrf54h20/cpurad
    [00:00:02.274,215] <inf> spi_wakeup: SPIS: will be active for 500ms after transfer

nrf54h20/cpurad:

.. code-block:: console

    [00:00:00.272,195] <inf> spi_wakeup: Hello world from nrf54h20dk@0.9.0/nrf54h20/cpurad
    [00:00:00.272,219] <inf> spi_wakeup: SPIM: going to sleep for 1.5s...
    [00:00:01.772,352] <inf> spi_wakeup: SPIM: will be active for 500ms before transfer
    [00:00:02.273,456] <inf> spi_wakeup: SPIM: transferring the CONFIG_BOARD_QUALIFIERS: 'nrf54h20/cpurad'

References
**********

:zephyr_file:`dts/bindings/spi/nordic,nrf-spi-common.yaml`
