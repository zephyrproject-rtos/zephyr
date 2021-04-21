.. _nrfx_sample:

nrfx use example
################

Overview
********

This sample demonstrates the usage of nrfx library in Zephyr.
GPIOTE and DPPI/PPI are used as examples of nrfx drivers.

The code shows how to initialize the GPIOTE interrupt in Zephyr
so that the nrfx_gpiote driver can use it. Additionally, it shows
how the DPPI/PPI subsystem can be used to connect tasks and events of
nRF peripherals, enabling those peripherals to interact with each
other without any intervention from the CPU.

Zephyr GPIO driver is disabled to prevent it from registering its own handler
for the GPIOTE interrupt. Button 1 is configured to create an event when pushed.
This calls the ``button_handler()`` callback and triggers the LED 1 pin OUT task.
LED is then toggled.

Requirements
************

This sample has been tested on the NordicSemiconductor nRF9160 DK
(nrf9160dk_nrf9160) and nRF52840 DK (nrf52840dk_nrf52840) boards.

Building and Running
********************

The code can be found in :zephyr_file:`samples/boards/nrf/nrfx`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/nrfx
   :board: nrf9160dk_nrf9160
   :goals: build flash
   :compact:

Push Button 1 to toggle the LED 1.

Connect to the serial port - notice that each time the button is pressed,
the ``button_handler()`` function is called.

See nrfx_repository_ for more information about nrfx.

.. _nrfx_repository: https://github.com/NordicSemiconductor/nrfx
