.. _secure_sockets_api:

Secure sockets API
##################

Zephyr provides an extension of standard POSIX socket API, allowing to create
and configure sockets with TLS protocol types, facilitating secure
communication. Secure functions for the implementation are provided by
mbedTLS library. Secure sockets implementation allows use of both TLS and DTLS
protocols with standard socket calls. See :c:type:`net_ip_protocol_secure` for
supported secure protocol versions.

To enable secure sockets, set the
:option:`CONFIG_NET_SOCKETS_SOCKOPT_TLS`
option. To enable DTLS support, use :option:`CONFIG_NET_SOCKETS_ENABLE_DTLS`.

TLS credentials subsystem
*************************

TLS credentials must be registered in the system before they can be used with
secure sockets. See :c:func:`tls_credential_add` for more information.

When a specific TLS credential is registered in the system, it is assigned with
numeric value of type :c:type:`sec_tag_t`, called a tag. This value can be used
later on to reference the credential during secure socket configuration with
socket options.

The following TLS credential types can be registered in the system:

- ``TLS_CREDENTIAL_CA_CERTIFICATE``
- ``TLS_CREDENTIAL_SERVER_CERTIFICATE``
- ``TLS_CREDENTIAL_PRIVATE_KEY``
- ``TLS_CREDENTIAL_PSK``
- ``TLS_CREDENTIAL_PSK_ID``

An example registration of CA certificate (provided in ``ca_certificate``
array) looks like this:

.. code-block:: c

   ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
                            ca_certificate, sizeof(ca_certificate));

By default certificates in DER format are supported. PEM support can be enabled
in mbedTLS settings.

Secure socket creation
**********************

A secure socket can be created by specifying secure protocol type, for instance:

.. code-block:: c

   sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

Once created, it can be configured with socket options. For instance, the
CA certificate and hostname can be set:

.. code-block:: c

   sec_tag_t sec_tag_opt[] = {
      CA_CERTIFICATE_TAG,
   };

   ret = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
                    sec_tag_opt, sizeof(sec_tag_opt));

.. code-block:: c

   char host[] = "google.com";

   ret = setsockopt(sock, SOL_TLS, TLS_HOSTNAME, host, sizeof(host));

Once configured, socket can be used just like a regular TCP socket.

Several samples in Zephyr use secure sockets for communication. For a sample use
see e.g. :ref:`sockets-echo-server-sample` or :ref:`sockets-http-get`.

Secure Sockets options
**********************

Secure sockets offer the following options for socket management:

.. doxygengroup:: secure_sockets_options
