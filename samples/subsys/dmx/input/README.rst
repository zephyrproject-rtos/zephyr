.. zephyr:code-sample:: dmx-input
   :name: DMX512 Input
   :relevant-api: dmx

   Receive and display DMX512 frames from all supported packet types.

Overview
********

This sample demonstrates DMX512-A reception using Zephyr's DMX subsystem. It receives frames from
a DMX universe and logs their contents, including:

* **NULL START code** (``0x00``): dimmer/channel data with change detection
* **Test packets** (``0x55``): per ANSI E1.11 Annex D3, with pass/fail validation
* **ASCII text** (``0x17``): with page number, characters per line, and text content
* **UTF-8 text** (``0x90``): same layout as ASCII but UTF-8 encoded
* **Manufacturer-specific** (``0x91``): manufacturer ID and raw data
* **System Information Packets** (``0xCF``): with checksum validation
* **Raw packets**: any other start code, displayed as hex dump

An LED (if available via ``led0`` alias) toggles on each received frame and turns off when no
packets are received.

Requirements
************

* A board with a free UART peripheral
* An RS-485 transceiver connecting the UART to the DMX bus
* A DMX512 source (e.g., lighting console or computer with USB DMX adapter)

Wiring
******

Connect the RS-485 transceiver to the UART and DMX bus:

.. code-block:: text

   Board UART TX  -->  DI  (Driver Input)
   Board UART RX  <--  RO  (Receiver Output)
   Board GPIO DE  -->  DE  (Driver Enable, active high)
   Board GPIO RE  -->  ~RE (Receiver Enable, active low)
   DMX Bus A (+)  <->  A
   DMX Bus B (-)  <->  B
   GND            <->  GND

The specific pins for each supported board are documented in the board overlay files under
``boards/``.

Building and Running
********************

.. code-block:: console

   west build -b <board> samples/subsys/dmx/input
   west flash

Supported Boards
================

Pre-configured board overlays are provided for:

* ``adafruit_kb2040``: RP2040, UART1 (pins D0/D1), DE=D2, RE=D3
* ``adafruit_itsybitsy``: nRF52840, UART0 (D0/D1), DE=D10, RE=D11
* ``nucleo_l432kc``: STM32L4, USART1 (D0/D1), DE=D5, RE=D6
* ``teensy40``: i.MX RT1060, LPUART2 (14/15), DE=16, RE=17

For other boards, create a board overlay defining the ``dmx0`` node.

Sample Output
*************

.. code-block:: console

   [00:00:00.001,000] <inf> dmx_sample: DMX input started, capturing full universe
   [00:00:03.705,000] <dbg> dmx.dmx_uart_handle_break: sync: first break, discarding frame
   [00:00:03.730,000] <dbg> dmx.dmx_uart_handle_break: sync: locked after 25 ms
   [00:00:03.730,000] <dbg> dmx.dmx_uart_process_byte: BREAK: sc=0x00 buf=0x202826ac
   [00:00:03.753,000] <dbg> dmx.dmx_uart_process_byte: commit: idx=512
   [00:00:03.753,000] <inf> dmx_sample: NULL SC @ 3730 ms, slots 1-512 [changed]
   [00:00:03.753,000] <inf> dmx_sample: slot data
                                        ff 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
                                        00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........
   [00:00:03.755,000] <dbg> dmx.dmx_uart_handle_break: sync: locked after 25 ms
   [00:00:03.755,000] <dbg> dmx.dmx_uart_process_byte: BREAK: sc=0x00 buf=0x202828bc
   [00:00:03.778,000] <dbg> dmx.dmx_uart_process_byte: commit: idx=512
   [00:00:03.778,000] <inf> dmx_sample: NULL SC @ 3755 ms, slots 1-512
