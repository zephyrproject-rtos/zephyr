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

For in-depth description of each shell command, you can run ``wifi --help`` on the shell.

In this shell example, we will scan for available access points and connect to one of them.
``--key-mgmt`` indicates the security type, where 0 is open, 1 is WPA/WPA2 Personal.

More information can be obtained by running ``wifi connect --help``.


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

   shell> wifi connect --ssid "gksu" --key-mgmt 1 --passphrase SecretStuff
   Connection requested
   shell>
   Connected
   shell>
