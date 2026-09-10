.. zephyr:code-sample:: lorawan-class-a
   :name: LoRaWAN class A device
   :relevant-api: lorawan_api

   Join a LoRaWAN network and send a message periodically.

Overview
********

A simple application to demonstrate the :ref:`LoRaWAN subsystem <lorawan_api>` of Zephyr.
Every fifth uplink requests a link check, logging the demodulation margin and
gateway count reported by the network.

Building and Running
********************

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

Without persistent storage, this sample supplies the same ``dev_nonce`` on every
boot. A network server may reject subsequent join requests. LoRaWAN 1.0.4 requires
DevNonce to increase for each join attempt with the same device identity.

Enable :kconfig:option:`CONFIG_LORAWAN_NVM_SETTINGS` to let the stack manage
DevNonce. The native backend supports Settings with NVS and reserves each nonce
before transmitting, including attempts that do not receive a Join-Accept.
The following configuration uses the board's existing ``storage_partition``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_wl55jc
   :gen-args: -DEXTRA_CONF_FILE=overlay-native-nvs.conf -DCONFIG_LORAWAN_REGION_EU868=y
   :goals: build flash
   :compact:

Preserve the storage partition when updating firmware. Erasing it resets the
counter and can cause nonce reuse. When switching an existing device from
application-managed nonces, provision a new device identity and key or migrate
its next unused nonce before joining. The native backend persists DevNonce only;
it performs a new OTAA join after reboot.

With :kconfig:option:`CONFIG_LORAWAN_NVM_NONE`, the application remains responsible
for persisting the next unused nonce before each join and supplying the reserved
value in ``join_cfg.otaa.dev_nonce``.
