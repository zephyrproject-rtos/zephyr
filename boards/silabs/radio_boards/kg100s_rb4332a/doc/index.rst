.. zephyr:board:: kg100s_rb4332a

Overview
********

The KG100S Radio Board (BRD4332A) is designed to work with the Wireless Pro Kit
Mainboard (BRD4002A, not included) to support the development of wireless IoT
devices based on Amazon Sidewalk.
The kit provides a complete reference design featuring the Quectel KG100S module,
which integrates a Silicon Labs EFR32BG21 2.4 GHz Bluetooth SoC together with a
Semtech SX1262 LoRa transceiver for Sub-GHz operation.
See :ref:`silabs_radio_boards` for more information about the Wireless Mainboard platform.

Hardware
********

- Quectel KG100S module based on EFR32BG21B010F1024IM32
- CPU core: ARM Cortex®-M33 with FPU, DSP and TrustZone
- Memory: 1024 kB Flash, 96 kB RAM
- Transmit power: up to +10 dBm
- Operation frequency: 2.4 GHz (Bluetooth) and Sub-GHz (LoRa via SX1262)
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz)
- Semtech SX1262 LoRa transceiver
- 2 user buttons

For more information about the KG100S, EFR32BG21 SoC and the LoRa radio SX1262, refer to these documents:

- `KG100S Website`_
- `SX1262 Website`_
- `SX1262 Datasheet`_
- `EFR32BG21 Website`_
- `EFR32BG21 Datasheet`_
- `EFR32BG21 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The EFR32BG21 SoC is configured to use the HFRCODPLL oscillator at 76.8 MHz as the system
clock, locked to the 38.4 MHz external crystal oscillator on the board.

Serial Port
===========

The EFR32BG21 SoC has three USARTs (USART0–USART2).
USART0 is connected to the board controller and is used for the console.
USART2 is used for SPI communication with the SX1262 LoRa transceiver.

Programming and Debugging
*************************

.. zephyr:board-supported-runners ::

Connect the BRD4002A mainboard with a mounted BRD4332A radio board to your host
computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kg100s_rb4332a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! kg100s_rb4332a

Bluetooth
=========

To use Bluetooth functionality, run the command below to retrieve necessary binary
blobs from the Silicon Labs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: kg100s_rb4332a
   :goals: build

LoRa
====

The board includes a Semtech SX1262 LoRa transceiver connected via USART2 (SPI mode).
To build a LoRa sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lora/send
   :board: kg100s_rb4332a
   :goals: build

.. _KG100S Website:
   https://www.quectel.com/product/kg100s-amazon-sidewalk-module/

.. _SX1262 Website:
   https://www.semtech.com/products/wireless-rf/lora-connect/sx1262

.. _SX1262 Datasheet:
   https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/RQ000008nKCH/hp2iKwMDKWl34g1D3LBf_zC7TGBRIo2ff5LMnS8r19s

.. _EFR32BG21 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg21-series-2-socs

.. _EFR32BG21 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg21-datasheet.pdf

.. _EFR32BG21 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg21-rm.pdf
