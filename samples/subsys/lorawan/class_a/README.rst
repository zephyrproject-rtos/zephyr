.. zephyr:code-sample:: lorawan-class-a
   :name: LoRaWAN class A device
   :relevant-api: lorawan_api

   Join a LoRaWAN network and send a message periodically.

Overview
********

A simple application to demonstrate the :ref:`LoRaWAN subsystem <lorawan_api>` of Zephyr.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/subsys/lorawan/class_a` in the Zephyr tree.

Before building the sample, make sure to select the correct region in the
``prj.conf`` file.

The following commands build and flash the sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_wl55jc
   :goals: build flash
   :compact:

Important Notes for Multiple Runs
*********************************

By default, this example will only succeed the first time it is run. On subsequent join attempts, the LoRaWAN network server may reject the join request due to a hardcoded ``dev_nonce`` value. According to the LoRaWAN specification, ``dev_nonce`` must increment for every new connection attempt.

To run this sample multiple times, choose one of the following options:

1. **Manually Increment ``dev_nonce``:**
   Modify the sample code to increment ``join_cfg.otaa.dev_nonce`` before each connection attempt and ensure it is preserved across reboots.

2. **Built-in Zephyr Settings Implementation:**
   Enable :kconfig:option:`CONFIG_LORAWAN_NVM_SETTINGS` in the Kconfig. This allows proper storage and reuse of configuration settings, including the ``dev_nonce``, across multiple runs.
