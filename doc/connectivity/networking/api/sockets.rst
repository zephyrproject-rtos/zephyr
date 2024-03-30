.. _bsd_sockets_interface:

BSD Sockets
###########

.. contents::
    :local:
    :depth: 2

Overview
********

Zephyr offers an implementation of a subset of the BSD Sockets API (a part
of the POSIX standard). This API allows to reuse existing programming experience
and port existing simple networking applications to Zephyr.

Here are the key requirements and concepts which governed BSD Sockets
compatible API implementation for Zephyr:

* Has minimal overhead, similar to the requirement for other
  Zephyr subsystems.
* Is namespaced by default, to avoid name conflicts with well-known
  names like ``close()``, which may be part of libc or other POSIX
  compatibility libraries.
  If enabled by :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES`, it will also
  expose native POSIX names.

BSD Sockets compatible API is enabled using :kconfig:option:`CONFIG_NET_SOCKETS`
config option and implements the following operations: ``socket()``, ``close()``,
``recv()``, ``recvfrom()``, ``send()``, ``sendto()``, ``connect()``, ``bind()``,
``listen()``, ``accept()``, ``fcntl()`` (to set non-blocking mode),
``getsockopt()``, ``setsockopt()``, ``poll()``, ``select()``,
``getaddrinfo()``, ``getnameinfo()``.

Based on the namespacing requirements above, these operations are by
default exposed as functions with ``zsock_`` prefix, e.g.
:c:func:`zsock_socket` and :c:func:`zsock_close`. If the config option
:kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined, all the functions
will be also exposed as aliases without the prefix. This includes the
functions like ``close()`` and ``fcntl()`` (which may conflict with
functions in libc or other libraries, for example, with the filesystem
libraries).

Another entailment of the design requirements above is that the Zephyr
API aggressively employs the short-read/short-write property of the POSIX API
whenever possible (to minimize complexity and overheads). POSIX allows
for calls like ``recv()`` and ``send()`` to actually process (receive
or send) less data than requested by the user (on ``SOCK_STREAM`` type
sockets). For example, a call ``recv(sock, 1000, 0)`` may return 100,
meaning that only 100 bytes were read (short read), and the application
needs to retry call(s) to receive the remaining 900 bytes.

The BSD Sockets API uses file descriptors to represent sockets. File
descriptors are small integers, consecutively assigned from zero, shared
among sockets, files, special devices (like stdin/stdout), etc. Internally,
there is a table mapping file descriptors to internal object pointers.
The file descriptor table is used by the BSD Sockets API even if the rest
of the POSIX subsystem (filesystem, stdin/stdout) is not enabled.

See :zephyr:code-sample:`sockets-echo-server` and :zephyr:code-sample:`sockets-echo-client`
sample applications to learn how to create a simple server or client BSD socket based
application.

.. _secure_sockets_interface:

Secure Sockets
**************

Zephyr provides an extension of standard POSIX socket API, allowing to create
and configure sockets with TLS protocol types, facilitating secure
communication. Secure functions for the implementation are provided by
mbedTLS library. Secure sockets implementation allows use of both TLS and DTLS
protocols with standard socket calls. See :c:enum:`net_ip_protocol_secure` type
for supported secure protocol versions.

To enable secure sockets, set the :kconfig:option:`CONFIG_NET_SOCKETS_SOCKOPT_TLS`
option. To enable DTLS support, use :kconfig:option:`CONFIG_NET_SOCKETS_ENABLE_DTLS`
option.

.. _sockets_tls_credentials_subsys:

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
see e.g. :zephyr:code-sample:`echo-server sample application <sockets-echo-server>` or
:zephyr:code-sample:`HTTP GET sample application <sockets-http-get>`.

Secure Sockets options
======================

Secure sockets offer the following options for socket management:

.. doxygengroup:: secure_sockets_options

Socket offloading
*****************

Zephyr allows to register custom socket implementations (called offloaded
sockets). This allows for seamless integration for devices which provide an
external IP stack and expose socket-like API.

Socket offloading can be enabled with :kconfig:option:`CONFIG_NET_SOCKETS_OFFLOAD`
option. A network driver that wants to register a new socket implementation
should use :c:macro:`NET_SOCKET_OFFLOAD_REGISTER` macro. The macro accepts the
following parameters:

 * ``socket_name``
     An arbitrary name for the socket implementation.

 * ``prio``
     Socket implementation's priority. The higher the priority, the earlier this
     particular implementation will be processed when creating a new socket.
     Lower numeric value indicates higher priority.

 * ``_family``
     Socket family implemented by the offloaded socket. ``AF_UNSPEC`` indicates
     any family.

 * ``_is_supported``
     A filtering function, used to verify whether a particular socket family,
     type and protocol are supported by the offloaded socket implementation.

 * ``_handler``
     A function compatible with :c:func:`socket` API, used to create an
     offloaded socket.

Every offloaded socket implementation should also implement a set of socket
APIs, specified in :c:struct:`socket_op_vtable` struct.

The function registered for socket creation should allocate a new file
descriptor using :c:func:`z_reserve_fd` function. Any additional actions,
specific to the creation of a particular offloaded socket implementation should
take place after the file descriptor is allocated. As a final step, if the
offloaded socket was created successfully, the file descriptor should be
finalized with :c:func:`z_finalize_fd` function. The finalize function allows
to register a :c:struct:`socket_op_vtable` structure implementing socket APIs
for an offloaded socket along with an optional socket context data pointer.

Finally, when an offloaded network interface is initialized, it should indicate
that the interface is offloaded with :c:func:`net_if_socket_offload_set`
function. The function registers the function used to create an offloaded socket
(the same as the one provided in :c:macro:`NET_SOCKET_OFFLOAD_REGISTER`) at the
network interface.

Offloaded socket creation
=========================

When application creates a new socket with :c:func:`socket` function, the
network stack iterates over all registered socket implementations (native and
offloaded). Higher priority socket implementations are processed first.
For each registered socket implementation, an address family is verified, and if
it matches (or the socket was registered as ``AF_UNSPEC``), the corresponding
``_is_supported`` function is called to verify the remaining socket parameters.
The first implementation that fulfills the socket requirements (i. e.
``_is_supported`` returns true) will create a new socket with its ``_handler``
function.

The above indicates the importance of the socket priority. If multiple socket
implementations support the same set of socket family/type/protocol, the first
implementation processed by the system will create a socket. Therefore it's
important to give the highest priority to the implementation that should be the
system default.

The socket priority for native socket implementation is configured with Kconfig.
Use :kconfig:option:`CONFIG_NET_SOCKETS_TLS_PRIORITY` to set the priority for
the native TLS sockets.
Use :kconfig:option:`CONFIG_NET_SOCKETS_PRIORITY_DEFAULT` to set the priority
for the remaining native sockets.

Dealing with multiple offloaded interfaces
==========================================

As the :c:func:`socket` function does not allow to specify which network
interface should be used by a socket, it's not possible to choose a specific
implementation in case multiple offloaded socket implementations, supporting the
same type of sockets, are available. The same problem arises when both native
and offloaded sockets are available in the system.

To address this problem, a special socket implementation (called socket
dispatcher) was introduced. The sole reason for this module is to postpone the
socket creation for until the first operation on a socket is performed. This
leaves an opening to use ``SO_BINDTODEVICE`` socket option, to bind a socket to
a particular network interface (and thus offloaded socket implementation).
The socket dispatcher can be enabled with :kconfig:option:`CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER`
Kconfig option.

When enabled, the application can specify the network interface to use with
:c:func:`setsockopt` function:

.. code-block:: c

   /* A "dispatcher" socket is created */
   sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

   struct ifreq ifreq = {
      .ifr_name = "SimpleLink"
   };

   /* The socket is "dispatched" to a particular network interface
    * (offloaded or not).
    */
   setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq, sizeof(ifreq));

Similarly, if TLS is supported by both native and offloaded sockets,
``TLS_NATIVE`` socket option can be used to indicate that a native TLS socket
should be created. The underlying socket can then be bound to a particular
network interface:

.. code-block:: c

   /* A "dispatcher" socket is created */
   sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

   int tls_native = 1;

   /* The socket is "dispatched" to a native TLS socket implmeentation.
    * The underlying socket is a "dispatcher" socket now.
    */
   setsockopt(sock, SOL_TLS, TLS_NATIVE, &tls_native, sizeof(tls_native));

   struct ifreq ifreq = {
      .ifr_name = "SimpleLink"
   };

   /* The underlying socket is "dispatched" to a particular network interface
    * (offloaded or not).
    */
   setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq, sizeof(ifreq));

In case no ``SO_BINDTODEVICE`` socket option is used on a socket, the socket
will be dispatched according to the default priority and filtering rules on a
first socket API call.

API Reference
*************

BSD Sockets
===========

.. doxygengroup:: bsd_sockets

TLS Credentials
===============

.. doxygengroup:: tls_credentials
