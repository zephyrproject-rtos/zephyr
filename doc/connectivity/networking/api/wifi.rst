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
in PEM format are committed to the repo at :zephyr_file:`samples/net/wifi/test_certs` and the during the
build process the certificates are converted to a `C` header file that is included by the Wi-Fi shell
module.

.. code-block:: bash

    $ cp client.pem samples/net/wifi/test_certs/
    $ cp client-key.pem samples/net/wifi/test_certs/
    $ cp ca.pem samples/net/wifi/test_certs/
    $ west build -p -b <board> samples/net/wifi

API Reference
*************

.. doxygengroup:: wifi_mgmt
