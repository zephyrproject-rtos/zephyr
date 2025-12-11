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
* WPA2-PSK-256
* WPA3-SAE

The Wi-Fi management API is implemented in the ``wifi_mgmt`` module as a part of the networking L2
stack.
Currently, two types of Wi-Fi drivers are supported:

* Networking or socket offloaded drivers
* Native L2 Ethernet drivers

Compiled Features
*****************

To support applications that only require a certain subset of Wi-Fi features, :kconfig:option:`CONFIG_WIFI_USAGE_MODE` can be used
as a hint for drivers to limit the functionality that needs to be compiled in. The following usage hints are available:

 * :kconfig:option:`CONFIG_WIFI_USAGE_MODE_STA` (Connecting to an access point)
 * :kconfig:option:`CONFIG_WIFI_USAGE_MODE_AP` (Being an access point)
 * :kconfig:option:`CONFIG_WIFI_USAGE_MODE_STA_AP` (Both being and connecting to an access point)
 * :kconfig:option:`CONFIG_WIFI_USAGE_MODE_SCAN_ONLY` (Access point SSID scanning only)

.. note::

    Support for a requested usage mode is hardware dependent.

Wi-Fi PSA crypto supported build
********************************

To enable PSA crypto API supported Wi-Fi build, the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT` and the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA` need to be set.

Wi-Fi Enterprise test: X.509 Certificate management
***************************************************

Wi-Fi enterprise security requires use of X.509 certificates, two methods of installing certificates are supported:

Compile time certificates
-------------------------

Test certificates in PEM format are committed to the repo at :zephyr_file:`samples/net/wifi/test_certs` and the during the
build process the certificates are converted to a C header file that is included by the Wi-Fi shell
module.

If you want to use your own certificates, you can replace the existing certificates with your own certificates in the same directory.

.. code-block:: bash

    $ export WIFI_TEST_CERTS_DIR=samples/net/wifi/test_certs/rsa3k
    $ cp client.pem $WIFI_TEST_CERTS_DIR
    $ cp client-key.pem $WIFI_TEST_CERTS_DIR
    $ cp ca.pem $WIFI_TEST_CERTS_DIR
    $ cp client2.pem $WIFI_TEST_CERTS_DIR
    $ cp client-key2.pem $WIFI_TEST_CERTS_DIR
    $ cp ca2.pem $WIFI_TEST_CERTS_DIR
    $ west build -p -b <board> samples/net/wifi -S wifi-enterprise

or alternatively copy ``rsa2k`` certificates by changing the ``WIFI_TEST_CERTS_DIR`` environment variable.

.. code-block:: bash

    $ export WIFI_TEST_CERTS_DIR=samples/net/wifi/test_certs/rsa2k

or you can set the :envvar:`WIFI_TEST_CERTS_DIR` environment variable to point to the directory containing your certificates.

.. code-block:: bash

    $ west build -p -b <board> samples/net/wifi -S wifi-enterprise -- -DWIFI_TEST_CERTS_DIR=<path_to_your_certificates>

Run time certificates
---------------------

The Wi-Fi shell module uses TLS credentials subsystem to store and manage the certificates. The certificates can be added at runtime using the shell commands, see :ref:`tls_credentials_shell` for more details.
The sample or application need to enable the :kconfig:option:`CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES` option to use this feature.

To facilitate installation of the certificates, a helper script is provided, see below for usage.

.. code-block:: bash

    $ ./scripts/utils/wifi_ent_cert_installer.py -p samples/net/wifi/test_certs/rsa2k

The script will install the certificates in the ``rsa2k`` directory to the TLS credentials store in the device over UART and using TLS credentials shell commands.


To initiate a Wi-Fi connection using enterprise security, use one of the following commands depending on the EAP method:

* EAP-TLS

  .. code-block:: console

     uart:~$ wifi connect -s <SSID> -c <channel> -k 7 -w 2 -a <Anonymous identity> --key1-pwd <Password EAP phase1> --key2-pwd <Password EAP phase2>

* EAP-TTLS-MSCHAPV2

  .. code-block:: console

     uart:~$ wifi connect -s <SSID> -c <channel> -k 14 -K <Private key Password> --eap-id1 <Client Identity> --eap-pwd1 <Client Password> -a <Anonymous identity>

* EAP-PEAP-MSCHAPV2

  .. code-block:: console

     uart:~$ wifi connect -s <SSID> -c <channel> -k 12 -K <Private key Password> --eap-id1 <Client Identity> --eap-pwd1 <Client Password> -a <Anonymous identity>

Server certificate is also provided in the same directory for testing purposes.
Any AAA server can be used for testing purposes, for example, ``FreeRADIUS`` or ``hostapd``.

Server certificate domain name verification
-------------------------------------------

The authentication server’s identity is verified by validating the domain name in the X.509 certificate received from the server, using the ``Common Name`` (CN) field.

* Exact domain match — Verifies that the certificate’s CN exactly matches the specified domain.

* Domain suffix match — Allows a certificate whose CN ends with the specified domain suffix.

To initiate a Wi-Fi connection using enterprise security with server certificate validation, use one of the following commands, depending on the desired validation mode:

* Exact domain match

  .. code-block:: console

     wifi connect -s <SSID> -c <channel> -k 12 -K <Private key Password> -e <Domain match>

* Domain suffix match

  .. code-block:: console

     wifi connect -s <SSID> -c <channel> -k 12 -K <Private key Password> -x <Domain suffix name>

Certificate requirements for EAP methods
----------------------------------------

Different EAP methods have varying client-side certificate requirements, as outlined below:

* EAP-TLS - Requires both a client certificate (and its private key) and a CA certificate on the client.
            The client authenticates itself to the server using its certificate.

* EAP-TTLS-MSCHAPV2 - Requires only the CA certificate on the client.
                      The client authenticates to the server using a username and password <MSCHAPV2> inside the TLS tunnel.
                      No client certificate is needed.

* EAP-PEAP-MSCHAPV2 - Requires only the CA certificate on the client.
                      Like TTLS, the client uses a username and password <MSCHAPV2> inside the TLS tunnel and does not require a client certificate.

.. note::

    The certificates are for testing purposes only and should not be used in production.
    They are generated using `FreeRADIUS raddb <https://github.com/FreeRADIUS/freeradius-server/tree/master/raddb/certs>`_ scripts.

.. note::

    When using TLS credentials subsystem, by default the volatile backend i.e., :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE` is chosen. When using the volatile backend, the certificates are stored in RAM and are lost on reboot, so the certificates need to be installed again after reboot. As an alternative, the PS (protected storage) backend i.e., :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE` can be used to store the certificates in the non-volatile storage.

How to Generate Test Certificates Using FreeRADIUS
--------------------------------------------------

The test certificates in ``samples/net/wifi/test_certs/rsa2k`` are generated using the `FreeRADIUS raddb/certs scripts <https://github.com/FreeRADIUS/freeradius-server/tree/master/raddb/certs>`_. You can generate your own certificates for testing as follows:

1. **Prerequisites**
   - Install OpenSSL and GNU Make.
   - Download the `FreeRADIUS raddb/certs directory <https://github.com/FreeRADIUS/freeradius-server/tree/master/raddb/certs>`_.

2. **Edit the Makefile**
   In the ``raddb/certs`` directory, edit the ``Makefile`` to add ``-nodes`` to the OpenSSL commands for server and client keys. This ensures the private keys are not password-protected (Zephyr Wi-Fi shell does not support private key passwords):

   ::

     $(OPENSSL) req -new -out server.csr -keyout server.key -nodes -config ./server.cnf
     $(OPENSSL) req -new -out client.csr -keyout client.key -nodes -config ./client.cnf

3. **(Optional) Edit the .cnf files**
   Customize ``server.cnf`` and ``client.cnf`` as needed for your environment.

4. **Generate Certificates**
   Run the following commands in the ``raddb/certs`` directory:

   ::

     make destroycerts
     make server
     make client

5. **Rename Files for Zephyr**
   Match the filenames used in Zephyr samples:

   +-------------------+---------------------+
   | FreeRADIUS Output | Zephyr Sample Name  |
   +===================+=====================+
   | ca.pem            | ca.pem              |
   | server.key        | server-key.pem      |
   | server.pem        | server.pem          |
   | client.key        | client-key.pem      |
   | client.pem        | client.pem          |
   +-------------------+---------------------+

6. **Copy the files**
   Place the renamed files in your Zephyr project's certificate directory (e.g., ``samples/net/wifi/test_certs/rsa2k``).

.. note::
   These certificates are for testing only and should not be used in production.

API Reference
*************

.. doxygengroup:: wifi_mgmt
