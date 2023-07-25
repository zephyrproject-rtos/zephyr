.. _wifi_sample:

Wi-Fi sample
############

Overview
********

This sample allows testing Wi-Fi drivers for various boards by
enabling the Wi-Fi shell module that provides a set of commands:
scan, connect, and disconnect.  It also enables the net_shell module
to verify net_if settings.

Building and Running
********************

Verify the board and chip you are targeting provide Wi-Fi support.

For instance you can use TI's CC3220 by selecting the cc3220sf_launchxl board.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi
   :board: cc3220sf_launchxl
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
