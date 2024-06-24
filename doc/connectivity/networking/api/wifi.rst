.. _wifi_mgmt:

Wi-Fi Management
################

Overview
========

The Wi-Fi management API is used to manage Wi-Fi networks. It supports below modes:

* IEEE802.11 Station (STA)
* IEEE802.11 Access Point (AP)

Only personal mode security is supported with below types:

* Open
* WPA2-PSK
* WPA3-PSK-256
* WPA3-SAE

The Wi-Fi management API is implemented in the `wifi_mgmt` module as a part of the networking L2 stack.
Currently, two types of Wi-Fi drivers are supported:

* Networking or socket offloaded drivers
* Native L2 Ethernet drivers

Wi-Fi Enterprise test: X.509 Certificate header generation
**********************************************************

Wi-Fi enterprise security requires use of X.509 certificates, test certificates
in PEM format are committed to the repo and the below steps to be followed to convert
them to C header file that can be included in Wi-Fi shell module.

Convert CA format from PEM to DER
=================================
To convert the CA format from PEM to DER, use the following command:

.. code-block:: none

   openssl x509 -outform der -in cas.pem -out ca.der

Convert RSA Key format to DER
=============================
To convert the RSA Key format to DER, use the following command:

.. code-block:: none

   openssl rsa -in wifiuser.key -outform DER -out client_key.der

Convert DER to C header file
============================
To convert the .der file to a header file, use the following command:

.. code-block:: none

   xxd -i ca.der > ca.h

Replace corresponding cert header files
=======================================
Replace the corresponding cert header files at the following path:
subsys/net/l2/wifi/test_certs/

.. note::

   Please ensure that the parameter names in the header are unchanged, only the payload is updated.

API Reference
*************

.. doxygengroup:: wifi_mgmt
