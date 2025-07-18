.. _snippet-wifi-ip:

Wi-Fi IPv4 and IPv6 Snippet (wifi-ip)
#####################################

.. code-block:: console

   west build -S wifi-ip [...]

Overview
********

This snippet enables IPv4 and IPv6 Wi-Fi support in supported networking samples.
The sample execution is postponed until Wi-Fi connectivity is established.

Use Wi-Fi shell to connect to the Wi-Fi network:

.. code-block:: console

   wifi connect -s <SSID> -k <key_management> -p <passphrase>

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
