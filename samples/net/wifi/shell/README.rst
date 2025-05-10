.. zephyr:code-sample:: wifi-shell
   :name: Wi-Fi shell
   :relevant-api: net_stats

   Test Wi-Fi functionality using the Wi-Fi shell module.

Overview
********

This sample allows testing Wi-Fi drivers for various boards by
enabling the Wi-Fi shell module that provides a set of commands:
scan, connect, and disconnect.  It also enables the net_shell module
to verify net_if settings.

Building and Running
********************

Verify the board and chip you are targeting provide Wi-Fi support.

For instance you can use Nordic's nrf7002dk by selecting the nrf7002dk/nrf5340/cpuapp board.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: nrf7002dk/nrf5340/cpuapp
   :goals: build
   :compact:

Sample console interaction
==========================

.. code-block:: console

   shell> wifi scan
   Scan requested
   shell>
   Num  | SSID                             (len) | Chan | RSSI | Sec
   1    | kapoueh!                         8     | 1    | -93  | WPA/WPA2
   2    | mooooooh                         8     | 6    | -89  | WPA/WPA2
   3    | Ap-foo blob..                    13    | 11   | -73  | WPA/WPA2
   4    | gksu                             4     | 1    | -26  | WPA/WPA2
   ----------
   Scan request done

   shell> wifi connect "gksu" 4 SecretStuff
   Connection requested
   shell>
   Connected
   shell>

Building and Running with module as shield
******************************************

If you want to use shield based approach, you can use NXP's MIMXRT1060EVKC by selecting mimxrt1060_evk@C board.
Here IW416 Murata module 1XK is used as shield.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: mimxrt1060_evk@C
   :goals: build
   :shield: nxp_m2_1xk_wifi_bt
   :conf: "nxp/overlay_hosted_mcu.conf nxp/overlay_debug.conf"
   :compact:

Sample console interaction
==========================

.. code-block:: console

   shell> wifi scan
   Scan requested
   shell>
   Num  | SSID                             (len) | Chan | RSSI | Sec
   1    | kapoueh!                         8     | 1    | -93  | WPA/WPA2
   2    | mooooooh                         8     | 6    | -89  | WPA/WPA2
   3    | Ap-foo blob..                    13    | 11   | -73  | WPA/WPA2
   4    | gksu                             4     | 1    | -26  | WPA/WPA2
   ----------
   Scan request done

   shell> wifi connect "gksu" 4 SecretStuff
   Connection requested
   shell>
   Connected
   shell>
