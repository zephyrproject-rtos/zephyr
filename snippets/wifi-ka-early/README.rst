.. _snippet-wifi-ka-early:

Wi-Fi keep-alive schedule early Snippet (wifi-ka-early)
######################################################

.. code-block:: console

   west build  -S wifi-ka-early [...]

Overview
********

This snippet enables Wi-Fi keep-alive scheduling earlier than BSS Max idle period.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_KEEP_ALIVE_EARLY_MS`
