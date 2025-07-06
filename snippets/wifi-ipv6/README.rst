.. _snippet-wifi-ipv6:

Wi-Fi IPv6 Snippet (wifi-ipv6)
##############################

.. code-block:: console

   west build -S wifi-ipv6 [...]

Overview
********

This snippet enables IPv6 Wi-Fi support in supported networking samples.
The sample execution is postponed until Wi-Fi connectivity is established.

Use Wi-Fi shell to connect to the Wi-Fi network:

.. code-block:: console

   wifi connect -s <SSID> -k <key_management> -p <passphrase>

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
