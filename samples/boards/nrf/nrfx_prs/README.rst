.. _nrfx_prs_sample:

nrfx peripheral resource sharing example
########################################

Overview
********

This sample shows how to use in Zephyr nRF peripherals that share the same ID
and base address. Such peripherals cannot be used simultaneously because they
share certain hardware resources, but it is possible to switch between them and
use one or the other alternately. Because of the current driver model in Zephyr
and the lack of possibility to deinitialize a peripheral that is initialized by
a driver at boot, such switching cannot be achieved with Zephyr APIs. Therefore,
this sample uses the nrfx drivers directly for those peripheral instances that
are to be switched (SPIM2 and UARTE2) while the standard Zephyr drivers are used
for other instances of the same peripheral types (UARTE0 is used by the standard
Zephyr console and SPIM1 is used for performing additional sample transfers).

The sample uses two buttons:
  - by pressing Button 1 user can request a transfer to be performed using the
    currently initialized peripheral (SPIM2 or UARTE2)
  - by pressing Button 2 user can switch between the two peripherals

When no button is pressed, every 5 seconds a background transfer using SPIM1
is performed.

The sample outputs on the standard console the hex codes of all sent and
received bytes, so when the proper loopback wiring is provided on the used
board (between the MOSI and MISO pins for SPIMs and between the TX and RX pins
for the UARTE), it can be checked that what is sent by a given peripheral
is also received back. Without such wiring, no data is received by UARTE and
all zeros are received by SPIMs. Refer to the overlay files provided in the
:zephyr_file:`samples/boards/nrf/nrfx_prs/boards` directory to check which pins
on the boards supported by the sample are assigned as MOSI/MISO and TX/RX pins.

Requirements
************

This sample has been tested on the Nordic Semiconductor nRF9160 DK
(nrf9160dk_nrf9160) and nRF5340 DK (nrf5340dk_nrf5340_cpuapp) boards.

Building and Running
********************

The code can be found in :zephyr_file:`samples/boards/nrf/nrfx_prs`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/nrfx_prs
   :board: nrf9160dk_nrf9160
   :goals: build flash
   :compact:

Press Button 1 to trigger a sample transfer on SPIM2 or UARTE2.
Press Button 2 to switch the type of peripheral to be used for the transfer.

When no button is pressed, a background transfer on SPIM1 is performed.
