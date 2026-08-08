.. _snippet-nxp-wifi-hostap:

NXP Wi-Fi Hostap (nxp-wifi-hostap)
##################################

Overview
********

This snippet enables NXP WPA supplicant/hostapd support including:

- WPA supplicant with CLI
- Enterprise authentication (EAP-TLS, EAP-TTLS, EAP-PEAP)
- DPP (Device Provisioning Protocol)
- WPS (Wi-Fi Protected Setup)
- EAP-TLSv1.3
- mbedTLS PSA crypto integration

Usage
*****

.. code-block:: console

   west build -b rd_rw612_bga <sample> -S nxp-wifi -S nxp-wifi-hostap
