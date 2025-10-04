.. _snippet-wifi-ipv4:

Wi-Fi IPv4 Snippet (wifi-ipv4)
##############################

.. code-block:: console

   west build -S wifi-ipv4 [...]

Overview
********

This snippet enables IPv4 Wi-Fi support in supported networking samples.
The sample execution is postponed until Wi-Fi connectivity is established.

Use Wi-Fi shell to connect to the Wi-Fi network:

.. code-block:: console

   wifi connect -s <SSID> -k <key_management> -p <passphrase>

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
