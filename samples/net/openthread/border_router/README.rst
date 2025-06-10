.. zephyr:code-sample:: openthread-border-router
   :name: OpenThread BorderRouter
   :relevant-api:

   Test Wi-Fi and IEEE 802.15.4 using the OpenThread Border Router.

Overview
********

This sample allows testing the Thread protocol and the Thread Border Router functionality for various
boards using the OpenThread and Wi-Fi shell.

Building and Running
********************

Verify that the board and chip you are targeting provide IEEE 802.15.4 and Wi-Fi support.

There are configuration files for different boards and setups in the shell directory:

- :file:`prj.conf`
  Generic config file.

- :file:`overlay-ot-rcp-host-wifi-nxp.conf`
  This overlay config enables support of OpenThread RCP host running on NXP chips over IMU interface. It also has Wi-Fi support enabled.

Build OpenThread Border Router application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/border_router
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for NXP's RW612 FRDM (RCP host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/border_router
   :board: frdm_rw612
   :conf: "prj.conf overlay-ot-rcp-host-wifi-nxp.conf"
   :goals: build
   :compact:
