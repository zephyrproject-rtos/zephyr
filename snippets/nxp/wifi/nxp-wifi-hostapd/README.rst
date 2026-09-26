.. _snippet-nxp-wifi-hostapd:

NXP Wi-Fi Hostapd Snippet (nxp-wifi-hostapd)
############################################

.. code-block:: console

   west build  -S nxp-wifi-hostapd [...]

Can also be used along with the :ref:`snippet-nxp-wifi` snippet.

.. code-block:: console

   west build  -S "nxp-wifi,nxp-wifi-hostapd" [...]

Overview
********

This snippet enables NXP Wi-Fi hostapd support.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
