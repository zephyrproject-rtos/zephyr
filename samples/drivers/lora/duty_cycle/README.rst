.. zephyr:code-sample:: lora-duty-cycle
   :name: LoRa duty cycle
   :relevant-api: lora_interface

   Receive LoRa packets using hardware RX duty cycling (wake-on-radio).

Overview
********

This sample demonstrates how to use ``lora_recv_duty_cycle_async()`` to let the
radio autonomously alternate between short RX windows and sleep, waking
the MCU only when a packet is received.

Two boards are needed.  By default the sample starts in **receiver** mode.
Hold **button 0** during boot to select **sender** mode.

The sender transmits with a long preamble (100 symbols) so that the
duty-cycling receiver is guaranteed to detect it during one of its RX
windows.

Building and Running
********************

Build and flash on two boards with a ``lora0`` alias, for example
``nucleo_wl55jc`` or ``nrf52840dk/nrf52840`` with the SX1261 shield:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lora/duty_cycle
   :host-os: unix
   :board: nucleo_wl55jc
   :goals: build flash
   :compact:

Sample Output (receiver)
========================

.. code-block:: console

   [00:00:00.235,000] <inf> lora_duty_cycle: RX duty cycle started (rx=10 ms, sleep=490 ms)
   [00:00:01.456,000] <inf> lora_duty_cycle: RX 12 bytes, RSSI: -55 dBm, SNR: 9 dB
   [00:00:01.456,000] <inf> lora_duty_cycle: payload
                                             64 75 74 79 63 79 63 6c  65 20 20 30  |dutycycle  0

Sample Output (sender)
======================

.. code-block:: console

   [00:00:00.235,000] <inf> lora_duty_cycle: Sender mode (preamble=100 symbols)
   [00:00:00.235,000] <inf> lora_duty_cycle: Expected packet airtime: 1565 ms
   [00:00:01.800,000] <inf> lora_duty_cycle: Sent packet 0
