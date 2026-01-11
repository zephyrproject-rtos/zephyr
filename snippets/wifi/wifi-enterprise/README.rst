.. _snippet-wifi-enterprise:

Wi-Fi Enterprise Snippet (wifi-enterprise)
##########################################

.. code-block:: console

   west build  -S wifi-enterprise [...]

Can also be used along with the :ref:`snippet-wifi-ipv4` snippet.

.. code-block:: console

   west build  -S "wifi-enterprise,wifi-ipv4" [...]

Overview
********

This snippet enables enterprise Wi-Fi support in supported networking samples.

See :ref:`wifi_mgmt` for more information on the usage.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE`
