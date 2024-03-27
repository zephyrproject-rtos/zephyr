.. zephyr:code-sample:: lorawan-fuota
   :name: LoRaWAN FUOTA
   :relevant-api: lorawan_api

   Perform a LoRaWAN firmware-upgrade over the air (FUOTA) operation.

Overview
********

An application to demonstrate firmware-upgrade over the air (FUOTA) over LoRaWAN.

The following services specified by the LoRa Alliance are used:

- Application Layer Clock Synchronization (`TS003-2.0.0`_)
- Remote Multicast Setup (`TS005-1.0.0`_)
- Fragmented Data Block Transport (`TS004-1.0.0`_)

The FUOTA process is started by the application and afterwards runs in the background in its own
work queue thread. After a firmware upgrade is successfully received, the application is notified
via a callback and can reboot the device into MCUboot to apply the upgrade.

A LoRaWAN Application Server implementing the relevant services is required for this sample to work.

.. _`TS003-2.0.0`: https://resources.lora-alliance.org/technical-specifications/ts003-2-0-0-application-layer-clock-synchronization
.. _`TS005-1.0.0`: https://resources.lora-alliance.org/technical-specifications/lorawan-remote-multicast-setup-specification-v1-0-0
.. _`TS004-1.0.0`: https://resources.lora-alliance.org/technical-specifications/lorawan-fragmented-data-block-transport-specification-v1-0-0

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/lorawan/fuota` in the Zephyr tree.

Before building the sample, make sure to select the correct region in the ``prj.conf`` file.

The following commands build and flash the sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/fuota
   :board: nucleo_wl55jc
   :goals: build flash
   :compact:
