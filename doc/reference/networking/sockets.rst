.. _bsd_sockets_interface:

BSD Sockets
###########

Overview
********

Zephyr offers an implementation of a subset of the BSD Sockets API (a part
of the POSIX standard). This API allows to reuse existing programming experience
and port existing simple networking applications to Zephyr.

Here are the key requirements and concepts which governed BSD Sockets
compatible API implementation for Zephyr:

* Should have minimal overhead, similar to the requirement for other
  Zephyr subsystems.
* Should be implemented on top of
  :ref:`native networking API <networking_api_usage>` to keep modular
  design.
* Should be namespaced by default, to avoid name conflicts with well-known
  names like ``close()``, which may be part of libc or other POSIX
  compatibility libraries. If enabled, should also expose native POSIX
  names.

BSD Sockets compatible API is enabled using :option:`CONFIG_NET_SOCKETS`
config option and implements the following operations: ``socket()``, ``close()``,
``recv()``, ``recvfrom()``, ``send()``, ``sendto()``, ``connect()``, ``bind()``,
``listen()``, ``fcntl()`` (to set non-blocking mode), ``poll()``.

Based on the namespacing requirements above, these operations are by
default exposed as functions with ``zsock_`` prefix, e.g.
:c:func:`zsock_socket()` and :c:func:`zsock_close()`. If the config option
:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined, all the functions
will be also exposed as aliases without the prefix. This includes the
functions like ``close()`` and ``fcntl()`` (which may conflict with
functions in libc or other libraries, for example, with the filesystem
libraries).

The native BSD Sockets API uses file descriptors to represent sockets. File descriptors
are small integers, consecutively assigned from zero. Internally, there is usually a table
mapping file descriptors to internal object pointers. For memory efficiency reasons, the
Zephyr BSD Sockets compatible API is devoid of such a table. Instead, ``net_context``
pointers, cast to an int, are used to represent sockets. Thus, socket identifiers aren't
really small integers, so the ``select()`` operation is not available, as it depends on the
"small int" property of file descriptors. Instead of using ``select()`` then, use the ``poll()``
operation, which is generally more efficient.

The BSD Sockets API (and the POSIX API in general) also treat negative file
descriptors values in a special way (such values usually mean an
error). As the Zephyr API uses a pointer value cast to an int for file descriptors, it means
that the pointer should not have the highest bit set, in other words,
pointers should not refer to the second (highest) part of the address space.
For many CPU architectures and SoCs Zephyr supports, user RAM is
located in the lower half, so the above condition is satisfied. If
you face an issue with some SoC because of this, please report it to the Zephyr bug
tracker or mailing list. The decision to use pointers to represent
sockets might be reworked in the future.

The final entailment of the design requirements above is that the Zephyr
API aggressively employs the short-read/short-write property of the POSIX API
whenever possible (to minimize complexity and overheads). POSIX allows
for calls like ``recv()`` and ``send()`` to actually process (receive
or send) less data than requested by the user (on STREAM type sockets).
For example, a call ``recv(sock, 1000, 0)`` may return 100,
meaning that only 100 bytes were read (short read), and the application
needs to retry call(s) to read the remaining 900 bytes.

.. _secure_sockets_interface:

Secure Sockets
**************

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
=========================

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

Secure Socket Creation
======================

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
======================

Secure sockets offer the following options for socket management:

.. doxygengroup:: secure_sockets_options

API Reference
*************

BSD Sockets
===========

.. doxygengroup:: bsd_sockets
   :project: Zephyr

TLS Credentials
===============

.. doxygengroup:: tls_credentials
   :project: Zephyr
