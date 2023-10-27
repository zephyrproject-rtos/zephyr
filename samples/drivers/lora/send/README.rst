.. zephyr:code-sample:: lora-send
   :name: LoRa send
   :relevant-api: lora_api

   Transmit a preconfigured payload every second using the LoRa radio.

Overview
********

This sample demonstrates how to use the LoRa radio driver to configure
the encoding settings and send data over the radio.

Transmitted messages can be received by building and flashing the accompanying
LoRa receive sample :zephyr:code-sample:`lora-receive` on another board within
range.

Building and Running
********************

Build and flash the sample as follows, changing ``b_l072z_lrwan1`` for
your board, where your board has a ``lora0`` alias in the devicetree.

.. zephyr-app-commands::
   :zephyr-app: zephyr/samples/drivers/lora/send
   :host-os: unix
   :board: b_l072z_lrwan1
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    [00:00:00.531,000] <inf> lora_send: Data sent!
    [00:00:01.828,000] <inf> lora_send: Data sent!
    [00:00:03.125,000] <inf> lora_send: Data sent!
