.. _snippet-nxp-wifi-hostap:

NXP Wi-Fi Hostap Snippet (nxp-wifi-hostap)
############################################

.. code-block:: console

   west build  -S nxp-wifi-hostap [...]

Can also be used along with the :ref:`snippet-nxp-wifi` snippet.

.. code-block:: console

   west build  -S "nxp-wifi,nxp-wifi-hostap" [...]

Overview
********

This snippet enables NXP Wi-Fi hostap support.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
