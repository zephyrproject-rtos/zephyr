.. _snippet-wifi-credentials:

Wi-Fi Credentials Snippet (wifi-credential)
###########################################

.. code-block:: console

   west build  -S wifi-credentials [...]

Can also be used along with the :ref:`snippet-wifi-enterprise` snippet.

.. code-block:: console

   west build  -S "wifi-enterprise,wifi-credentials" [...]

Overview
********

This snippet enables Wi-Fi credentials support.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE`
