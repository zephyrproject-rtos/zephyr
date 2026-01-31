.. _coap_sock_interface:

CoAP
#####

.. contents::
    :local:
    :depth: 2

Overview
********

The Constrained Application Protocol (CoAP) is a specialized web transfer
protocol for use with constrained nodes and constrained (e.g., low-power,
lossy) networks. It provides a convenient API for RESTful Web services
that support CoAP's features. For more information about the protocol
itself, see :rfc:`7252`.

Zephyr provides a CoAP library which supports client and server roles.
The library can be enabled with :kconfig:option:`CONFIG_COAP` Kconfig option and
is configurable as per user needs. The Zephyr CoAP library
is implemented using plain buffers. Users of the API create sockets
for communication and pass the buffer to the library for parsing and other
purposes. The library itself doesn't create any sockets for users.

On top of CoAP, Zephyr has support for LWM2M "Lightweight Machine 2 Machine"
protocol, a simple, low-cost remote management and service enablement mechanism.
See :ref:`lwm2m_interface` for more information.

Supported RFCs:

- :rfc:`7252` - The Constrained Application Protocol (CoAP)
- :rfc:`6690` - Constrained RESTful Environments (CoRE) Link Format
- :rfc:`7959` - Block-Wise Transfers in the Constrained Application Protocol (CoAP)
- :rfc:`7641` - Observing Resources in the Constrained Application Protocol (CoAP)
- :rfc:`7967` - Constrained Application Protocol (CoAP) Option for No Server Response
- :rfc:`9175` - CoAP: Echo, Request-Tag, and Token Processing

.. note:: Not all parts of these RFCs are supported. Features are supported based on Zephyr requirements.

Sample Usage
************

CoAP Server
===========

.. note::

   A :ref:`coap_server_interface` subsystem is available, the following is for creating a custom
   server implementation.

To create a CoAP server, resources for the server need to be defined.
The ``.well-known/core`` resource should be added before all other
resources that should be included in the responses of the ``.well-known/core``
resource.

.. code-block:: c

    static struct coap_resource resources[] = {
        { .get = well_known_core_get,
          .path = COAP_WELL_KNOWN_CORE_PATH,
        },
        { .get  = sample_get,
          .post = sample_post,
          .del  = sample_del,
          .put  = sample_put,
          .path = sample_path
        },
        { },
    };

An application reads data from the socket and passes the buffer to the CoAP library
to parse the message. If the CoAP message is proper, the library uses the buffer
along with resources defined above to call the correct callback function
to handle the CoAP request from the client. It's the callback function's
responsibility to either reply or act according to CoAP request.

.. code-block:: c

    coap_packet_parse(&request, data, data_len, options, opt_num);
    ...
    coap_handle_request(&request, resources, options, opt_num,
                        client_addr, client_addr_len);

If :kconfig:option:`CONFIG_COAP_URI_WILDCARD` enabled, server may accept multiple resources
using MQTT-like wildcard style:

- the plus symbol represents a single-level wild card in the path;
- the hash symbol represents the multi-level wild card in the path.

.. code-block:: c

    static const char * const led_set[] = { "led","+","set", NULL };
    static const char * const btn_get[] = { "button","#", NULL };
    static const char * const no_wc[] = { "test","+1", NULL };

It accepts /led/0/set, led/1234/set, led/any/set, /button/door/1, /test/+1,
but returns -ENOENT for /led/1, /test/21, /test/1.

This option is enabled by default, disable it to avoid unexpected behaviour
with resource path like '/some_resource/+/#'.

No-Response Option (RFC 7967)
==============================

The No-Response option allows CoAP clients to express disinterest in receiving
responses from the server for certain response classes (2.xx, 4.xx, 5.xx). This
can be useful for reducing network traffic in scenarios such as frequent sensor
updates where the client doesn't need confirmation of successful operations.

When a server receives a request with the No-Response option, it should check
whether the response it would send should be suppressed based on the option value.
The Zephyr CoAP library provides the :c:func:`coap_no_response_check` function
to help with this decision.

Server applications should use this function before sending responses:

.. code-block:: c

    bool suppress = false;
    int ret;

    /* Check if response should be suppressed */
    ret = coap_no_response_check(request, response_code, &suppress);
    if (ret < 0 && ret != -ENOENT) {
        /* Invalid No-Response option - do not suppress */
        suppress = false;
    }

    if (suppress) {
        /* Response should be suppressed */
        if (request_type == COAP_TYPE_CON) {
            /* For CON requests, send an empty ACK */
            ret = coap_ack_init(&response, request, data, sizeof(data), COAP_CODE_EMPTY);
            if (ret < 0) {
                return ret;
            }
            return coap_resource_send(resource, &response, addr, addr_len, NULL);
        }
        /* For NON requests, send nothing */
        return 0;
    }

    /* Response not suppressed, send normal response */
    /* ... build and send response ... */

The built-in CoAP server in Zephyr automatically handles the No-Response option
for responses generated internally (such as error responses and .well-known/core
responses). Applications using the :ref:`coap_server_interface` subsystem or
implementing custom resource handlers should check for No-Response suppression
before sending responses.

CoAP Client
===========

.. note::

   A :ref:`coap_client_interface` subsystem is available, the following is for creating a custom
   client implementation.

If the CoAP client knows about resources in the CoAP server, the client can start
prepare CoAP requests and wait for responses. If the client doesn't know
about resources in the CoAP server, it can request resources through
the ``.well-known/core`` CoAP message.

.. code-block:: c

    /* Initialize the CoAP message */
    char *path = "test";
    struct coap_packet request;
    uint8_t data[100];
    uint8_t payload[20];

    coap_packet_init(&request, data, sizeof(data),
                     1, COAP_TYPE_CON, 8, coap_next_token(),
                     COAP_METHOD_GET, coap_next_id());

    /* Append options */
    coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                              path, strlen(path));

    /* Append Payload marker if you are going to add payload */
    coap_packet_append_payload_marker(&request);

    /* Append payload */
    coap_packet_append_payload(&request, (uint8_t *)payload,
                               sizeof(payload) - 1);

    /* send over sockets */

Token Generation (RFC 9175)
============================

The Zephyr CoAP library implements RFC 9175 compliant token generation using a
sequence-based approach. This ensures that tokens are never reused within a
connection lifetime, preventing response-to-request binding issues.

The :c:func:`coap_next_token` function generates 8-byte tokens consisting of:

- A 4-byte random prefix (initialized at startup)
- A 4-byte monotonically increasing sequence number

This design guarantees token uniqueness and is thread-safe for concurrent use.

For applications using secure connections (DTLS, TLS, or OSCORE), the token
generator should be reset when connections are established or rekeyed:

.. code-block:: c

    /* When establishing a new secure connection or rekeying */
    coap_token_generator_rekey();

    /* Or with a specific prefix */
    uint32_t my_prefix = get_connection_specific_prefix();
    coap_token_generator_reset(my_prefix);

The sequence-based token generation also applies to Request-Tag options used in
blockwise transfers, ensuring that Request-Tags are never recycled as required
by RFC 9175 §3.4.

Testing
*******

There are various ways to test Zephyr CoAP library.

libcoap
=======
libcoap implements a lightweight application-protocol for devices that are
resource constrained, such as by computing power, RF range, memory, bandwidth,
or network packet sizes. Sources can be found here `libcoap <https://github.com/obgm/libcoap>`_.
libcoap has a script (``examples/etsi_coaptest.sh``) to test coap-server functionality
in Zephyr.

See the `net-tools <https://github.com/zephyrproject-rtos/net-tools>`_ project for more details

The :zephyr:code-sample:`coap-server` sample can be built and executed on QEMU as described
in :ref:`networking_with_qemu`.

Use this command on the host to run the libcoap implementation of
the ETSI test cases:

.. code-block:: console

   sudo ./libcoap/examples/etsi_coaptest.sh -i tap0 2001:db8::1

TTCN3
=====
Eclipse has TTCN3 based tests to run against CoAP implementations.

Install eclipse-titan and set symbolic links for titan tools

.. code-block:: console

    sudo apt-get install eclipse-titan

    cd /usr/share/titan

    sudo ln -s /usr/bin bin
    sudo ln /usr/bin/titanver bin
    sudo ln -s /usr/bin/mctr_cli bin
    sudo ln -s /usr/include/titan include
    sudo ln -s /usr/lib/titan lib

    export TTCN3_DIR=/usr/share/titan

    git clone https://gitlab.eclipse.org/eclipse/titan/titan.misc.git

    cd titan.misc

Follow the instruction to setup CoAP test suite from here:

- https://gitlab.eclipse.org/eclipse/titan/titan.misc
- https://gitlab.eclipse.org/eclipse/titan/titan.misc/-/tree/master/CoAP_Conf

After the build is complete, the :zephyr:code-sample:`coap-server` sample can be built
and executed on QEMU as described in :ref:`networking_with_qemu`.

Change the client (test suite) and server (Zephyr coap-server sample) addresses
in coap.cfg file as per your setup.

Execute the test cases with following command.

.. code-block:: console

   ttcn3_start coaptests coap.cfg

Sample output of ttcn3 tests looks like this.

.. code-block:: console

   Verdict statistics: 0 none (0.00 %), 10 pass (100.00 %), 0 inconc (0.00 %), 0 fail (0.00 %), 0 error (0.00 %).
   Test execution summary: 10 test cases were executed. Overall verdict: pass

CoAP Server Echo Option Support (RFC 9175)
******************************************

The Zephyr CoAP server implementation supports the Echo option as specified in
:rfc:`9175` for request freshness verification and amplification attack mitigation.

Overview
========

The Echo option is a lightweight challenge-response mechanism that enables a CoAP
server to:

1. Verify the freshness of requests (particularly for unsafe methods like PUT, POST, DELETE)
2. Mitigate amplification attacks by verifying client reachability
3. Synchronize state between client and server

When enabled, the server can challenge clients with a 4.01 (Unauthorized) response
containing an Echo option. The client must then retry the request with the received
Echo value to prove request freshness or address reachability.

Configuration
=============

Enable Echo support with :kconfig:option:`CONFIG_COAP_SERVER_ECHO`. Additional
configuration options control the behavior:

- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_MAX_LEN`: Maximum Echo value length (1-40 bytes per RFC 9175)
- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_LIFETIME_MS`: How long Echo values remain valid
- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_CACHE_SIZE`: Number of clients to track per service
- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_REQUIRE_FOR_UNSAFE`: Require Echo for unsafe methods
- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_AMPLIFICATION_MITIGATION`: Enable amplification mitigation

Echo Option Format
==================

Per RFC 9175 Section 2.2.1:

- **Option Number**: 252
- **Length**: 1-40 bytes
- **Properties**: Elective, safe-to-forward, not repeatable, not part of cache key
- **Value**: Opaque bytes generated by the server

The server generates random Echo values and caches them per client address. Clients
must treat Echo values as opaque and echo them back without modification.

Request Freshness (RFC 9175 Section 2.3)
=========================================

When :kconfig:option:`CONFIG_COAP_SERVER_ECHO_REQUIRE_FOR_UNSAFE` is enabled, the
server requires Echo verification for unsafe methods (PUT, POST, DELETE, PATCH, IPATCH).

**Flow**:

1. Client sends unsafe request without Echo
2. Server responds with 4.01 (Unauthorized) + Echo option
3. Client retries request with Echo option
4. Server verifies Echo and processes request

This ensures the request is fresh and not a delayed replay attack.

Amplification Mitigation (RFC 9175 Section 2.4 & 2.6)
======================================================

When :kconfig:option:`CONFIG_COAP_SERVER_ECHO_AMPLIFICATION_MITIGATION` is enabled,
the server challenges unauthenticated clients before sending large responses.

**Rationale**: An attacker could spoof a victim's address and send small CoAP requests
to resources with large responses (e.g., ``/.well-known/core``), causing the server
to amplify the attack by sending large responses to the victim.

**Mitigation**: The server first sends a small 4.01 (Unauthorized) response with an
Echo option. Only after the client proves reachability by echoing the value does the
server send the large response.

**Configuration**:

- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_AMPLIFICATION_FACTOR`: Maximum response/request size ratio (default 3)
- :kconfig:option:`CONFIG_COAP_SERVER_ECHO_MAX_INITIAL_RESPONSE_BYTES`: Maximum response size without verification (default 136)

The default of 136 bytes follows RFC 9175 guidance for Ethernet MTU (1500 bytes) with
IP/UDP/CoAP headers, allowing a 3x amplification factor.

**Important**: Per RFC 9175 Section 2.4 item 3, Echo challenge responses MUST be sent
as piggybacked (ACK) or Non-confirmable responses, never as separate responses, to
avoid amplification through retransmissions.

Integration with CoAP Client
=============================

The Zephyr CoAP client (``subsys/net/lib/coap/coap_client.c``) already handles Echo
challenges automatically. When a client receives a 4.01 response with an Echo option,
it retries the request with the Echo value included.

This means applications using the CoAP client API benefit from Echo support without
code changes, as the retry logic is handled internally by the client library.

Security Considerations
=======================

1. **Echo values must be unpredictable**: The server uses cryptographically secure
   random number generation (``sys_csrand_get``) to prevent forgery.

2. **Echo values expire**: Configured via :kconfig:option:`CONFIG_COAP_SERVER_ECHO_LIFETIME_MS`
   to prevent indefinite reuse.

3. **Per-client tracking**: The server maintains a cache of Echo values per client
   address, with LRU eviction when the cache is full.

4. **Length validation**: The server rejects Echo options with invalid lengths
   (0 or >40 bytes) per RFC 9175 Section 2.2.1.

5. **No separate responses**: Challenge responses are never sent as separate CoAP
   messages to avoid amplification through retransmissions.

API Reference
*************

.. doxygengroup:: coap
