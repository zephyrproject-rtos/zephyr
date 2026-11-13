.. _snippet-nxp-wifi:

NXP Native Wi-Fi Snippet (nxp-wifi)
#####################################

.. code-block:: console

   west build -S nxp-wifi [...]

Overview
********

This snippet enables NXP native Wi-Fi support.

Use Wi-Fi shell to connect to the Wi-Fi network:

.. code-block:: console

   wifi connect -s <SSID> -k <key_management> -p <passphrase>

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
