.. zephyr:code-sample:: openthread-shell
   :name: OpenThread shell
   :relevant-api: net_stats

   Test Thread and IEEE 802.15.4 using the OpenThread shell.

Overview
********

This sample allows testing the Thread protocol and the underlying IEEE 802.15.4 drivers for various
boards using the OpenThread shell.

Building and Running
********************

Verify that the board and chip you are targeting provide IEEE 802.15.4 support.

There are configuration files for different boards and setups in the shell directory:

- :file:`prj.conf`
  Generic config file.

- :file:`prj-ot-host.conf`
  This config file enables support of OpenThread RCP host running on NXP chips over IMU interface.

Build shell application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for Nordic's nRF52840 DK.

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: nrf52840dk/nrf52840
   :conf: "prj.conf"
   :goals: build
   :compact:

Example building for NXP's RW612 FRDM (RCP host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_rw612
   :gen-args: -DEXTRA_CONF_FILE="prj-ot-host.conf"
   :goals: build
   :compact:

Example building for NXP's MIMXRT1060-EVK Rev-C with an IW612 M.2 module (RCP host).
The IW612 802.15.4 radio is used as an RCP over SPI. The NXP Wi-Fi driver is
required to load the module firmware; the Wi-Fi interface is present but left
down. Use ``iw610.conf`` instead of ``iw612.conf`` for an IW610 module. See
:ref:`nxp_m2_ieee802154` for the hardware setup.

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: mimxrt1060_evk@C/mimxrt1062/qspi
   :shield: nxp_m2_2ll_wifi_bt,nxp_m2_2el_2ll_ieee802154
   :gen-args: -DEXTRA_CONF_FILE="prj-ot-host.conf;boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf;boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw612.conf"
   :goals: build
   :compact:

Example building for NXP's MCXW72 FRDM (host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_mcxw72/mcxw727c
   :gen-args: -DEXTRA_CONF_FILE="prj-ot-host.conf"
   :goals: build
   :compact:

Example building for NXP's MCXW72 FRDM (host) with vendor specific radio implementation.

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_mcxw72/mcxw727c
   :gen-args: -DEXTRA_CONF_FILE="prj-ot-host.conf;prj-ot-host-no-l2-nxp-mcxw.conf"
   :goals: build
   :compact:

Example building for NXP's MCXW72 FRDM (host) with the RGB status LED enabled.
The board RGB LED blinks with a colour that reflects the Thread device role:
white (disabled), red (detached), yellow (child), green (router) and blue (leader).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_mcxw72/mcxw727c
   :gen-args: -DEXTRA_CONF_FILE="prj-ot-host.conf" -DCONFIG_OT_SHELL_SAMPLE_RGB_STATUS_LED=y
   :goals: build
   :compact:

The LED is controlled with the ``rgb`` shell command:

.. code-block:: console

   uart:~$ rgb off             # stop blinking
   uart:~$ rgb on              # start blinking
   uart:~$ rgb rate 200        # blink period in milliseconds
   uart:~$ rgb color 255 0 255 # fixed colour, stops following the role
   uart:~$ rgb role            # follow the Thread device role again

Example building for Silicon Labs EFR32MG24 4187c radio board:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: xg24_rb4187c
   :conf: "prj.conf"
   :goals: build
   :compact:


Sample console interaction
==========================

.. code-block:: console

   uart:~$ ot scan
   | PAN  | MAC Address      | Ch | dBm | LQI |
   +------+------------------+----+-----+-----+
   | fe09 | abcdef1234567890 | 15 | -78 |  60 |
   Done
