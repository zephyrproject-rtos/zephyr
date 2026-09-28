.. _snippet-nxp-wifi-hostap-debug:

NXP Wi-Fi Hostap Debug Snippet (nxp-wifi-hostap-debug)
######################################################

.. code-block:: console

   west build -S "nxp-wifi,nxp-wifi-hostap,nxp-wifi-hostap-debug" [...]

Overview
********

This snippet enables verbose wpa_supplicant/hostap and mbedTLS debug
logging, including key/PMK printing for offline packet decryption.

Meant to be used together with the :ref:`snippet-nxp-wifi-hostap` snippet.
