.. zephyr:code-sample:: tmc6460
   :name: TMC6460 Voltage Mode
   :relevant-api: tmc6460_interface

   Drive a BLDC motor in open-loop voltage mode with a Trinamic TMC6460.

Overview
********

This sample performs the full TMC6460 hardware bring-up (clock FSM commit,
ADC reset, gate-driver enable) and then drives a BLDC motor in open-loop
voltage mode, performing the explicit initialization that an evaluation-board
firmware would normally handle.

Requirements
************

- A board with an available SPI (or UART) bus.
- A TMC6460 evaluation board with a BLDC motor connected and a suitable
  motor power supply.
- Two GPIOs wired to the TMC6460 ``DRV_EN`` and ``SLEEPN`` pins, exposed via
  the ``tmc6460-drv-en`` and ``tmc6460-sleepn`` devicetree aliases.

The motor's pole-pair count is read from the ``pole-pairs`` devicetree
property, so a different motor is supported by editing only the board overlay.

Building and Running
********************

.. code-block:: console

   west build -b nucleo_f413zh samples/drivers/misc/tmc6460
   west flash

Replace ``nucleo_f413zh`` with your target board. If your board does not
already have an overlay under ``boards/``, create one that places the
``tmc6460`` node on the correct SPI bus and defines the ``tmc6460-drv-en`` and
``tmc6460-sleepn`` GPIO aliases.

UART transport
==============

To build the UART variant, supply the UART devicetree overlay and the
transport configuration overlay:

.. code-block:: console

   west build -b nucleo_f413zh samples/drivers/misc/tmc6460 \
     -- -DDTC_OVERLAY_FILE=uart.overlay -DEXTRA_CONF_FILE=overlay-uart.conf
   west flash

Without these arguments the sample builds for SPI (the default). The
``uart.overlay`` places the ``tmc6460`` node on the UART bus and
``overlay-uart.conf`` switches the driver transport from SPI to UART.

.. warning::

   The motor turns during this sample. Ensure it is mechanically safe to spin
   before running.
