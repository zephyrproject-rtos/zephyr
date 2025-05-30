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


To initiate Wi-Fi connection, the following command can be used:

.. code-block:: console

    uart:~$ wifi connect -s <SSID> -c 149 -k 7 -w 2 -a client1 --key1-pwd whatever --key2-pwd whatever

Server certificate is also provided in the same directory for testing purposes.
Any AAA server can be used for testing purposes, for example, ``FreeRADIUS`` or ``hostapd``.

.. note::

    The certificates are for testing purposes only and should not be used in production.
    They are generated using `FreeRADIUS raddb <https://github.com/FreeRADIUS/freeradius-server/tree/master/raddb/certs>`_ scripts.

.. note::

    When using TLS credentials subsystem, by default the volatile backend i.e., :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE` is chosen. When using the volatile backend, the certificates are stored in RAM and are lost on reboot, so the certificates need to be installed again after reboot. As an alternative, the PS (protected storage) backend i.e., :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE` can be used to store the certificates in the non-volatile storage.

API Reference
*************

.. doxygengroup:: wifi_mgmt
