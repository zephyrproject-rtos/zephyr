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
- :rfc:`8768` - Constrained Application Protocol (CoAP) Hop-Limit Option
- :rfc:`9175` - CoAP: Echo, Request-Tag, and Token Processing
- :rfc:`9177` - Constrained Application Protocol (CoAP) Block-Wise Transfer Options Supporting Robust Transmission
- :rfc:`8613` - Object Security for Constrained RESTful Environments (OSCORE)
- :rfc:`9668` - EDHOC and OSCORE profile of ACE (EDHOC with CoAP and OSCORE)
- :rfc:`9528` - Ephemeral Diffie-Hellman Over COSE (EDHOC)

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

Request-Tag Option Support (RFC 9175)
======================================

The Zephyr CoAP client implements RFC 9175 Request-Tag option support for
blockwise transfers. The Request-Tag option (Option 292) allows the server to
match block-wise message fragments belonging to the same request operation.

**Key Features**:

- **Automatic Request-Tag generation**: When a Block1 (upload) transfer is
  initiated, the client automatically generates a Request-Tag using the same
  secure token generation mechanism.

- **Block1 + Block2 continuity**: Per RFC 9175 §3.4, when Block1 and Block2
  are combined in an operation (e.g., a large PUT request followed by a large
  response), the Request-Tag from the Block1 phase is automatically carried
  into the Block2 phase. This ensures the server can recognize all messages
  as part of the same operation.

- **Request-only**: The Request-Tag option is never sent in response messages,
  as required by RFC 9175 §3.4.

- **Format compliance**: Request-Tag values are 0-8 bytes of opaque data, as
  specified in RFC 9175 §3.2.1.

Applications using the :ref:`coap_client_interface` benefit from Request-Tag
support automatically without code changes, as the client library handles
Request-Tag generation and propagation internally.

Block-wise Transfer ETag Validation (RFC 7959 §2.4)
====================================================

The Zephyr CoAP client implements RFC 7959 §2.4 ETag validation for Block2
(download) transfers to prevent silent reassembly of mixed representations.

**Overview**:

When a server sends a Block2 response with an ETag option, the client tracks
that ETag and enforces that all subsequent blocks of the same transfer use
the same ETag value. This ensures the client doesn't accidentally reassemble
blocks from different versions of a resource.

**Key Features**:

- **Automatic ETag tracking**: When the first block (block 0) of a Block2
  transfer contains an ETag option, the client stores the ETag value.

- **Mandatory comparison**: Per RFC 7959 §2.4, if an ETag is present in the
  first block, all subsequent blocks MUST include the same ETag. Mismatches
  or missing ETags abort the transfer.

- **Optional enforcement**: If no ETag is present in the first block, the
  client does not enforce ETag comparison for that transfer (per RFC 7959,
  comparison is only mandatory "if an ETag Option is available").

- **RFC 7252 compliance**: The client enforces RFC 7252 §5.10.6.1 which
  requires that "The ETag Option MUST NOT occur more than once in a response."
  Responses with multiple ETags are rejected.

- **OSCORE integration**: ETag validation operates on the decrypted inner
  response for OSCORE-protected transfers, ensuring it works correctly with
  inner Block2 transfers.

**Behavior**:

1. **First block (block 0)**:

   - If ETag present and valid (1-8 bytes, single option): Store ETag
   - If ETag absent: No ETag enforcement for this transfer
   - If ETag invalid (multiple options or wrong length): Abort transfer with
     error callback

2. **Subsequent blocks (block 1+)**:

   - If ETag was stored from block 0:

     - Current block MUST have matching ETag (same length and value)
     - Missing or mismatched ETag: Abort transfer, clear state, report error
       (-EBADMSG) to application callback

   - If no ETag was stored: No enforcement

3. **Last block**:

   - After successful delivery of the last block, ETag state is cleared

**Server Requirements**:

Per RFC 7959 §2.4 and RFC 9175 §3.8, servers SHOULD include an ETag option
in each Block2 response:

- Use a stable ETag for a given representation
- Change the ETag when the representation changes
- Include the same ETag in all blocks of a transfer

**Example Server Implementation**:

.. code-block:: c

   /* In Block2 response handler */
   static const uint8_t etag_value[] = {0x01, 0x02, 0x03, 0x04};

   /* ETag must come before Content-Format and Block2 (option ordering) */
   ret = coap_packet_append_option(&response, COAP_OPTION_ETAG,
                                   etag_value, sizeof(etag_value));
   if (ret < 0) {
       return ret;
   }

   ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
                                COAP_CONTENT_FORMAT_TEXT_PLAIN);
   /* ... */

   ret = coap_append_block2_option(&response, &ctx);
   /* ... */

**Security Considerations**:

- **Fail-closed**: Invalid ETags or mismatches cause the transfer to abort
  rather than silently accepting potentially inconsistent data.

- **No partial delivery**: When an ETag mismatch is detected, the application
  callback receives an error (-EBADMSG) and does not receive the mismatched
  block payload.

Q-Block: Robust Block-wise Transfer (RFC 9177)
===============================================

The Zephyr CoAP library provides support for Q-Block1 and Q-Block2 options per
:rfc:`9177`, which enable robust block-wise transfers with recovery from missing
blocks. This is particularly useful for unreliable transports like UDP where
packets may be lost.

**Overview**:

Q-Block extends the basic Block1/Block2 mechanism from RFC 7959 with:

- **Missing block recovery**: Servers can respond with 4.08 (Request Entity Incomplete)
  containing a CBOR Sequence of missing block numbers
- **Multiple block requests**: Clients can request multiple missing blocks in a single
  request using multiple Q-Block2 options
- **Non-confirmable (NON) support**: Optimized for NON messages where ACKs are not used

**Key Differences from Block1/Block2**:

+-------------------+---------------------------+--------------------------------+
| Feature           | Block1/Block2 (RFC 7959)  | Q-Block1/Q-Block2 (RFC 9177)   |
+===================+===========================+================================+
| Recovery          | Restart from beginning    | Request only missing blocks    |
+-------------------+---------------------------+--------------------------------+
| Required options  | None                      | Request-Tag + Size1 (Q-Block1) |
|                   |                           | Size2 + ETag (Q-Block2)        |
+-------------------+---------------------------+--------------------------------+
| Multiple blocks   | One per request           | Multiple Q-Block2 in request   |
+-------------------+---------------------------+--------------------------------+
| Missing blocks    | Not specified             | 4.08 with CBOR Sequence        |
+-------------------+---------------------------+--------------------------------+

**Configuration**:

Enable Q-Block support with:

.. code-block:: kconfig

   CONFIG_COAP_Q_BLOCK=y
   CONFIG_ZCBOR=y  # Required for CBOR encoding/decoding

**API Functions**:

The following functions are available when ``CONFIG_COAP_Q_BLOCK`` is enabled:

.. code-block:: c

   /* Append Q-Block options */
   int coap_append_q_block1_option(struct coap_packet *cpkt,
                                   uint32_t block_number,
                                   bool has_more,
                                   enum coap_block_size block_size);

   int coap_append_q_block2_option(struct coap_packet *cpkt,
                                   uint32_t block_number,
                                   bool has_more,
                                   enum coap_block_size block_size);

   /* Parse Q-Block options */
   int coap_get_q_block1_option(const struct coap_packet *cpkt,
                                bool *has_more,
                                uint32_t *block_number);

   int coap_get_q_block2_option(const struct coap_packet *cpkt,
                                bool *has_more,
                                uint32_t *block_number);

   /* Validate Block/Q-Block mixing */
   int coap_validate_block_q_block_mixing(const struct coap_packet *cpkt);

   /* CBOR Sequence encoding for missing blocks */
   int coap_encode_missing_blocks_cbor_seq(uint8_t *payload,
                                           size_t payload_len,
                                           const uint32_t *missing_blocks,
                                           size_t missing_blocks_count,
                                           size_t *encoded_len);

   int coap_decode_missing_blocks_cbor_seq(const uint8_t *payload,
                                           size_t payload_len,
                                           uint32_t *missing_blocks,
                                           size_t max_blocks,
                                           size_t *decoded_count);

**Important Constraints**:

Per RFC 9177 §4.1:

- **Cannot mix Block and Q-Block**: A packet MUST NOT contain both Block1/Block2
  and Q-Block1/Q-Block2 options. Use ``coap_validate_block_q_block_mixing()`` to
  check for violations.

- **OSCORE separation**: When using OSCORE, the mixing constraint applies separately
  to outer and inner packets. Both layers must be validated independently.

**Q-Block1 Requirements** (RFC 9177 §4.3):

When using Q-Block1 for request payloads:

- **Request-Tag MUST be present** in all payloads with the same value
- **Size1 MUST be present** in all payloads with the same value
- Missing either option results in 4.00 (Bad Request)

**Q-Block2 Requirements** (RFC 9177 §4.4, §4.6):

When using Q-Block2 for response payloads:

- **Size2 MUST be present** in all blocks with the same value
- **ETag MUST be constant** across all blocks of the same body
- **Multiple Q-Block2 options** in missing-block requests must be strictly increasing
  with no duplicates

**Missing Blocks Payload** (RFC 9177 §5):

The 4.08 (Request Entity Incomplete) response uses:

- **Content-Format**: 272 (application/missing-blocks+cbor-seq)
- **Payload**: CBOR Sequence (not array) of unsigned integers
- **Order**: Missing block numbers in ascending order, no duplicates
- **Client behavior**: Ignores duplicates if present

**Example: Encoding Missing Blocks**:

.. code-block:: c

   /* Server detected missing blocks 2, 5, 7 */
   uint32_t missing[] = {2, 5, 7};
   uint8_t payload[64];
   size_t encoded_len;

   ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
                                             missing, ARRAY_SIZE(missing),
                                             &encoded_len);
   if (ret == 0) {
       /* Add to 4.08 response */
       coap_packet_append_option_int(&response,
                                     COAP_OPTION_CONTENT_FORMAT,
                                     COAP_CONTENT_FORMAT_APP_MISSING_BLOCKS_CBOR_SEQ);
       coap_packet_append_payload_marker(&response);
       coap_packet_append_payload(&response, payload, encoded_len);
   }

**Example: Decoding Missing Blocks**:

.. code-block:: c

   /* Client received 4.08 response */
   const uint8_t *payload;
   uint16_t payload_len;
   uint32_t missing[16];
   size_t decoded_count;

   payload = coap_packet_get_payload(&response, &payload_len);
   ret = coap_decode_missing_blocks_cbor_seq(payload, payload_len,
                                             missing, ARRAY_SIZE(missing),
                                             &decoded_count);
   if (ret == 0) {
       /* Retransmit missing blocks */
       for (size_t i = 0; i < decoded_count; i++) {
           /* Send block missing[i] */
       }
   }

**Congestion Control** (RFC 9177 §7.2):

The ``CONFIG_COAP_Q_BLOCK_MAX_PAYLOADS`` option (default 10) controls the maximum
number of Q-Block payloads in flight for NON transfers. This prevents network
congestion when multiple blocks are sent without waiting for acknowledgments.

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

OSCORE Support (RFC 8613)
**************************

The Zephyr CoAP library provides support for Object Security for Constrained RESTful
Environments (OSCORE) as specified in :rfc:`8613`. OSCORE provides end-to-end protection
of CoAP messages using COSE (CBOR Object Signing and Encryption).

Overview
========

OSCORE protects CoAP messages at the application layer, providing:

1. **Confidentiality**: Message payloads and sensitive options are encrypted
2. **Integrity**: Messages are authenticated with a MAC
3. **Replay protection**: Sequence numbers prevent replay attacks
4. **Proxy-friendly**: Outer options remain visible for routing

Unlike DTLS, OSCORE provides end-to-end security that survives proxy translation
between different transport protocols (UDP, TCP, HTTP).

Configuration
=============

Enable OSCORE support with :kconfig:option:`CONFIG_COAP_OSCORE`. This option depends
on the uoscore-uedhoc module and PSA Crypto support:

.. code-block:: kconfig

   CONFIG_COAP_OSCORE=y
   CONFIG_UOSCORE=y
   CONFIG_PSA_CRYPTO=y

The uoscore module automatically selects required PSA crypto algorithms (AES-CCM,
HKDF-SHA256, etc.).

Additional OSCORE configuration options:

- :kconfig:option:`CONFIG_COAP_OSCORE_MAX_PLAINTEXT_LEN`: Maximum OSCORE plaintext length (default 1024)
- :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE`: Number of OSCORE exchanges to track per service (default 8)
- :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`: Lifetime of non-Observe OSCORE exchanges (default 60000ms)

OSCORE Option
=============

Per RFC 8613 Section 2, OSCORE uses option number **9** to indicate protected messages:

- **Option Number**: 9
- **Properties**: Critical, not safe-to-forward, part of cache key, **not repeatable**
- **Value**: Compressed COSE object (0-255 bytes)

**Important**: A CoAP message with the OSCORE option but no payload is malformed and
MUST be rejected (RFC 8613 Section 2). The Zephyr implementation enforces this rule.

**Non-Repeatable Constraint** (RFC 8613 Section 2 + RFC 7252 Section 5.4.5):

The OSCORE option is **not repeatable** and MUST NOT appear more than once in a message.
Per RFC 7252 Section 5.4.5, each supernumerary occurrence of a non-repeatable critical
option MUST be treated like an unrecognized critical option, causing the message to be
rejected:

- **Server behavior**: CON requests with multiple OSCORE options are rejected with
  **4.02 (Bad Option)** response. NON requests are silently dropped.
- **Client behavior**: Responses with multiple OSCORE options are rejected. For CON
  responses, the client sends **RST**. For NON/ACK responses, the message is silently
  dropped and not delivered to the application.

The Zephyr implementation enforces this constraint automatically in both client and
server, ensuring RFC compliance and fail-closed security behavior.

Handling OSCORE When Not Supported
-----------------------------------

When OSCORE support is not enabled (:kconfig:option:`CONFIG_COAP_OSCORE` is not set),
the Zephyr CoAP stack implements fail-closed behavior for the OSCORE option per
RFC 7252 Section 5.4.1:

**Server behavior** (when ``CONFIG_COAP_OSCORE=n``):

- **CON requests** with OSCORE option: Returns **4.02 (Bad Option)** response
- **NON requests** with OSCORE option: Silently rejects (drops) the message
- **Responses** with OSCORE option: Sends RST for CON, silently drops NON/ACK

**Client behavior** (when ``CONFIG_COAP_OSCORE=n``):

- **Responses** with OSCORE option: Rejects the response, sends RST for CON responses,
  does not deliver to application callback

This ensures that CoAP messages containing the OSCORE option are never processed as
plaintext when OSCORE support is unavailable, preventing security violations.

Inner vs Outer Options
======================

OSCORE divides CoAP options into two classes (RFC 8613 Section 4.1):

**Class E (Encrypted)**: Protected inside OSCORE payload

- Uri-Path, Uri-Query (used for routing after decryption)
- Content-Format, Accept
- If-Match, If-None-Match, ETag
- Observe (when present)

**Class U (Unprotected)**: Remain visible in outer message

- Uri-Host, Uri-Port (for proxy routing)
- Proxy-Uri, Proxy-Scheme
- Max-Age, Size1, Size2

The Zephyr OSCORE implementation uses the uoscore library which handles option
classification automatically.

Server Usage
============

To enable OSCORE on a CoAP service, initialize an OSCORE security context and
attach it to the service:

.. code-block:: c

   #include <oscore.h>
   #include <oscore/security_context.h>

   /* Initialize OSCORE context */
   struct context oscore_ctx;
   struct oscore_init_params params = {
       .master_secret.ptr = master_secret,
       .master_secret.len = 16,
       .sender_id.ptr = sender_id,
       .sender_id.len = sender_id_len,
       .recipient_id.ptr = recipient_id,
       .recipient_id.len = recipient_id_len,
       .master_salt.ptr = master_salt,
       .master_salt.len = 8,
       .aead_alg = OSCORE_AES_CCM_16_64_128,
       .hkdf = OSCORE_SHA_256,
       .fresh_master_secret_salt = false,
   };

   int ret = oscore_context_init(&params, &oscore_ctx);
   if (ret != 0) {
       /* Handle error */
   }

   /* Attach to service */
   my_service_data.oscore_ctx = &oscore_ctx;

   /* Optionally require OSCORE for all requests */
   my_service_data.require_oscore = true;

When a service has an OSCORE context attached:

1. **Incoming requests**: The server automatically verifies and decrypts OSCORE-protected
   requests (RFC 8613 Section 8.2). Resource handlers receive decrypted CoAP messages
   with Inner options visible.

2. **Outgoing responses**: The server automatically OSCORE-protects responses and
   notifications for OSCORE exchanges (RFC 8613 Section 8.3). This is done transparently:

   - When an OSCORE request is successfully verified, the exchange is tracked
   - All responses for that exchange (including Observe notifications) are OSCORE-protected
   - Non-Observe exchanges are removed after sending the response
   - Observe exchanges remain tracked until the observation is cancelled

3. **Error handling**: OSCORE verification errors are sent as simple CoAP responses
   **without** OSCORE processing (RFC 8613 Section 8.2):

   - COSE decode failure → 4.02 Bad Option
   - Security context not found → 4.01 Unauthorized
   - Decryption failure → 4.00 Bad Request

4. **Required OSCORE**: If ``require_oscore`` is true, unprotected requests are rejected
   with 4.01 Unauthorized.

5. **Fail-closed behavior**: If OSCORE protection of a response fails for an OSCORE
   exchange, the server will not fall back to sending a plaintext response. This ensures
   security-first behavior per RFC 8613 Section 2.

Client Usage
============

To enable OSCORE on a CoAP client, initialize an OSCORE security context and attach
it to the client:

.. code-block:: c

   #include <oscore.h>
   #include <oscore/security_context.h>

   /* Initialize OSCORE context (similar to server) */
   struct context oscore_ctx;
   struct oscore_init_params params = {
       .master_secret.ptr = master_secret,
       .master_secret.len = 16,
       .sender_id.ptr = sender_id,
       .sender_id.len = sender_id_len,
       .recipient_id.ptr = recipient_id,
       .recipient_id.len = recipient_id_len,
       .master_salt.ptr = master_salt,
       .master_salt.len = 8,
       .aead_alg = OSCORE_AES_CCM_16_64_128,
       .hkdf = OSCORE_SHA_256,
       .fresh_master_secret_salt = false,
   };

   int ret = oscore_context_init(&params, &oscore_ctx);
   if (ret != 0) {
       /* Handle error */
   }

   /* Attach to client */
   my_client.oscore_ctx = &oscore_ctx;

When a client has an OSCORE context attached:

1. **Outgoing requests**: The client automatically OSCORE-protects all requests
   (RFC 8613 Section 8.1). This is done transparently before sending.

2. **Incoming responses**: The client automatically verifies and decrypts OSCORE-protected
   responses (RFC 8613 Section 8.4). The application callback receives decrypted Inner
   options and payload.

3. **Fail-closed behavior**: The client enforces security-first guarantees:

   - If OSCORE protection of a request fails, the request is not sent
   - If a response to an OSCORE request is not OSCORE-protected, it is rejected
   - If OSCORE verification of a response fails, the response is not delivered to the
     application callback

4. **Block-wise transfers**: When using Block-wise transfers (RFC 7959) with OSCORE,
   the client follows RFC 8613 Section 8.4.1 ordering requirements:

   - For Block2 (download): The client buffers outer block payloads and performs OSCORE
     verification after receiving the last block. Note: Current implementation verifies
     only the last block's OSCORE message, which works with most servers but may not
     handle all Block2 + OSCORE combinations per RFC 8613 Section 8.4.1.
   - For Block1 (upload): Block-wise upload with OSCORE is handled by the existing
     blockwise logic operating on OSCORE-protected messages

5. **Observe notifications**: For Observe (RFC 7641) with OSCORE, verification failures
   on notifications do not cancel the observation (RFC 8613 Section 8.4.2). The client
   continues waiting for the next notification.

The client-side OSCORE implementation is fully functional and interoperates with
OSCORE-enabled servers.

Security Context Derivation
============================

OSCORE security contexts are derived from a small set of parameters (RFC 8613 Section 3):

**Required parameters**:

- **Master Secret**: Shared secret (typically 16 bytes for AES-CCM-16-64-128)
- **Sender ID**: Unique identifier for the sender
- **Recipient ID**: Unique identifier for the recipient

**Optional parameters**:

- **Master Salt**: Additional entropy (recommended, typically 8 bytes)
- **ID Context**: Additional context identifier
- **AEAD Algorithm**: Defaults to AES-CCM-16-64-128
- **KDF**: Defaults to HKDF-SHA-256

These parameters are typically established through:

1. **Pre-shared keys**: Configured at device provisioning
2. **EDHOC**: Ephemeral Diffie-Hellman Over COSE (see uoscore-uedhoc module)

Message Processing
==================

**Request Protection (RFC 8613 Section 8.1)**:

1. Client builds normal CoAP request
2. OSCORE library encrypts payload and Class E options
3. Adds OSCORE option (number 9) to outer message
4. Sends OSCORE-protected message

**Request Verification (RFC 8613 Section 8.2)**:

1. Server receives message with OSCORE option
2. Validates message format (must have payload)
3. Decrypts and verifies using security context
4. Extracts Inner options for routing
5. Passes decrypted message to resource handlers

**Response Protection (RFC 8613 Section 8.3)**:

1. Server builds normal CoAP response
2. OSCORE library encrypts response
3. Adds OSCORE option to response
4. Sends OSCORE-protected response

**Response Verification (RFC 8613 Section 8.4)**:

1. Client receives response with OSCORE option
2. Verifies and decrypts using security context
3. Passes decrypted response to application

Block-wise Transfer with OSCORE
================================

When using block-wise transfers (RFC 7959) with OSCORE:

- **Block options are Outer (Class U)**: Block1, Block2, Size1, Size2 remain visible
  to proxies for fragmentation handling
- **OSCORE processes the complete message**: Protection/verification happens before
  block-wise processing (RFC 8613 Section 8.2.1 and 8.4.1)

Observe with OSCORE
===================

When using Observe (RFC 7641) with OSCORE:

- **Observe option is Inner (Class E)**: Protected inside OSCORE payload
- **Notification numbers**: Each notification has a fresh Partial IV for replay protection
- **Re-registration**: Clients must re-register observations after security context changes

Security Considerations
=======================

1. **Sequence number overflow**: The sender sequence number (SSN) must not exceed 2^23-1
   for AES-CCM-16-64-128. The uoscore library enforces this limit.

2. **Master secret protection**: Master secrets must be stored securely (e.g., in
   secure storage or derived from EDHOC).

3. **Replay window**: The server maintains a replay window to detect and reject
   replayed requests.

4. **Fresh master secrets**: If master secrets are not re-derived after reboot (e.g.,
   using EDHOC), the sender sequence number must be persisted to non-volatile memory
   to prevent reuse.

5. **Token binding**: OSCORE maintains request-response binding through tokens and
   security context association (RFC 8613 Section 8).

EDHOC Support (RFC 9668)
*************************

The Zephyr CoAP library provides support for Ephemeral Diffie-Hellman Over COSE (EDHOC)
as specified in :rfc:`9528` and its integration with CoAP and OSCORE as specified in
:rfc:`9668`. EDHOC provides lightweight authenticated key exchange for constrained devices,
enabling secure session establishment with minimal overhead.

Overview
========

EDHOC is a key exchange protocol that:

1. **Establishes shared secrets**: Uses Diffie-Hellman key exchange with authentication
2. **Derives OSCORE contexts**: Produces master secret and salt for OSCORE
3. **Minimizes round trips**: Can complete in 3 messages (1.5 round trips)
4. **Supports various authentication methods**: Pre-shared keys, raw public keys, certificates

When combined with OSCORE, EDHOC provides a complete security solution for CoAP:

- **EDHOC**: Establishes the security context (key exchange + authentication)
- **OSCORE**: Protects subsequent application messages (encryption + integrity)

Configuration
=============

Enable EDHOC support with :kconfig:option:`CONFIG_COAP_EDHOC`. This option depends on
CBOR support:

.. code-block:: kconfig

   CONFIG_COAP_EDHOC=y
   CONFIG_ZCBOR=y

Full EDHOC handshake processing requires the uoscore-uedhoc module:

.. code-block:: kconfig

   CONFIG_UEDHOC=y

For EDHOC+OSCORE combined request support (RFC 9668 Section 3), also enable:

.. code-block:: kconfig

   CONFIG_COAP_EDHOC_COMBINED_REQUEST=y
   CONFIG_COAP_OSCORE=y

Additional EDHOC configuration options:

- :kconfig:option:`CONFIG_COAP_EDHOC_MAX_COMBINED_PAYLOAD_LEN`: Maximum combined payload length (default 1024)
- :kconfig:option:`CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE`: Number of EDHOC sessions to cache per service (default 4)
- :kconfig:option:`CONFIG_COAP_EDHOC_SESSION_LIFETIME_MS`: EDHOC session lifetime in milliseconds (default 60000)
- :kconfig:option:`CONFIG_COAP_OSCORE_CTX_CACHE_SIZE`: OSCORE context cache size for EDHOC-derived contexts (default 4)
- :kconfig:option:`CONFIG_COAP_OSCORE_CTX_LIFETIME_MS`: OSCORE derived context lifetime in milliseconds (default 3600000)

EDHOC Option
============

Per RFC 9668 Section 3.1, EDHOC uses option number **21** to indicate EDHOC+OSCORE
combined requests:

- **Option Number**: 21
- **Properties**: Critical, safe-to-forward, part of cache key, Class U for OSCORE
- **Value**: MUST be empty (0 bytes)

**Important**: The EDHOC option MUST occur at most once and MUST be empty. If any value
is sent, the recipient MUST ignore it (RFC 9668 Section 3.1).

EDHOC Content-Formats
======================

EDHOC messages use specific content-formats (RFC 9528 Section 10.9):

- **64** (``application/edhoc+cbor-seq``): EDHOC messages and error responses
- **65** (``application/cid-edhoc+cbor-seq``): EDHOC messages with connection identifiers

Handling EDHOC When Not Supported
----------------------------------

When EDHOC support is not enabled (:kconfig:option:`CONFIG_COAP_EDHOC` is not set),
the Zephyr CoAP stack implements fail-closed behavior for the EDHOC option per
RFC 7252 Section 5.4.1:

**Server behavior** (when ``CONFIG_COAP_EDHOC=n``):

- **CON requests** with EDHOC option: Returns **4.02 (Bad Option)** response
- **NON requests** with EDHOC option: Silently rejects (drops) the message

This ensures that CoAP messages containing the EDHOC option are never processed
incorrectly when EDHOC support is unavailable.

EDHOC over CoAP (``/.well-known/edhoc``) (RFC 9528 Appendix A.2)
==================================================================

The Zephyr CoAP library provides support for EDHOC message transfer over CoAP
as specified in RFC 9528 Appendix A.2 ("Transferring EDHOC over CoAP"). This
implements the forward flow for EDHOC handshakes using POST requests to the
``/.well-known/edhoc`` resource.

Configuration
-------------

Enable EDHOC-over-CoAP transport support with:

.. code-block:: kconfig

   CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC=y
   CONFIG_COAP_EDHOC=y
   CONFIG_COAP_SERVER=y

This enables the server to handle POST requests to ``/.well-known/edhoc`` for
EDHOC message_1 and message_3 processing.

Protocol Flow
-------------

Per RFC 9528 Appendix A.2.1, EDHOC messages are transferred via POST requests:

**Message_1 Request** (Initiator → Responder):

- **Method**: POST
- **Uri-Path**: ``/.well-known/edhoc``
- **Content-Format**: 65 (``application/cid-edhoc+cbor-seq``)
- **Payload**: CBOR ``true`` (0xF5) followed by EDHOC message_1

**Message_2 Response** (Responder → Initiator):

- **Code**: 2.04 (Changed)
- **Content-Format**: 64 (``application/edhoc+cbor-seq``)
- **Payload**: EDHOC message_2

**Message_3 Request** (Initiator → Responder):

- **Method**: POST
- **Uri-Path**: ``/.well-known/edhoc``
- **Content-Format**: 65 (``application/cid-edhoc+cbor-seq``)
- **Payload**: C_R (connection identifier) followed by EDHOC message_3

**Message_4 Response** (Responder → Initiator):

- **Code**: 2.04 (Changed)
- **Content-Format**: 64 (``application/edhoc+cbor-seq``)
- **Payload**: EDHOC message_4 (if required) or empty

Connection Identifier Encoding
-------------------------------

Per RFC 9528 Section 3.3.2, connection identifiers (C_R, C_I) use special encoding:

- **One-byte CBOR integers** (major type 0 or 1, values 0-23): Encoded as a single byte
- **Other values**: Encoded as CBOR byte strings (major type 2)

This optimization reduces overhead for common connection identifier values.

OSCORE Context Derivation
--------------------------

When ``CONFIG_COAP_EDHOC_COMBINED_REQUEST=y`` is enabled, after successful EDHOC
message_3 processing, the server automatically derives an OSCORE security context
per RFC 9528 Appendix A.1:

1. **Derive PRK_out**: From EDHOC handshake
2. **Derive master secret**: Using EDHOC_Exporter with label 0
3. **Derive master salt**: Using EDHOC_Exporter with label 1
4. **Map connection identifiers** (RFC 9528 Table 14):

   - Responder OSCORE Sender ID = C_I (initiator's connection identifier)
   - Responder OSCORE Recipient ID = C_R (responder's connection identifier)

5. **Initialize OSCORE context**: With derived keying material
6. **Cache context**: Keyed by C_R for subsequent OSCORE requests

Note: OSCORE context caching requires ``CONFIG_COAP_EDHOC_COMBINED_REQUEST=y``.

Error Handling
--------------

Per RFC 9528 Appendix A.2.3, EDHOC errors over CoAP are carried in the payload:

**Error Response Format**:

- **Code**: 4.00 (Bad Request) or 5.00 (Internal Server Error)
- **Content-Format**: 64 (``application/edhoc+cbor-seq``)
- **Payload**: CBOR Sequence ``(ERR_CODE, ERR_INFO)``

**Common Error Cases**:

- Invalid message_1 prefix → 4.00 with ERR_CODE=1
- Missing Content-Format → 4.00 with ERR_CODE=1
- Wrong HTTP method → 4.05 (Method Not Allowed)
- Session not found → 4.00 with ERR_CODE=1
- EDHOC processing failure → 5.00 with ERR_CODE=1

Security Considerations
-----------------------

**DoS Mitigation**: Enable Echo option support to mitigate denial-of-service attacks
on the EDHOC endpoint:

.. code-block:: kconfig

   CONFIG_COAP_SERVER_ECHO=y
   CONFIG_COAP_SERVER_ECHO_REQUIRE_FOR_UNSAFE=y

Per RFC 9528 Appendix A.2 and RFC 9175, the Echo option provides:

- **Request freshness verification**: Ensures requests are not replayed
- **Amplification mitigation**: Prevents using the server to amplify attacks
- **Client verification**: Confirms client can receive responses at claimed address

**Fail-closed behavior**: If EDHOC processing fails, no OSCORE context is established.

**Session management**: EDHOC sessions are cached with configurable lifetime and
automatic eviction of expired sessions.

Resource Discovery (RFC 9668 Section 6)
----------------------------------------

When :kconfig:option:`CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC` is enabled, the server
automatically advertises the ``/.well-known/edhoc`` resource in ``/.well-known/core``
responses using CoRE Link Format (RFC 6690).

**Link-Value Format**:

.. code-block:: text

   </.well-known/edhoc>;rt=core.edhoc;ed-r[;ed-comb-req]

**Attributes**:

- ``rt=core.edhoc``: Resource type registered in RFC 9528 Section 10.10
- ``ed-r``: Valueless hint indicating EDHOC Responder role support (forward flow)
- ``ed-comb-req``: Valueless hint indicating EDHOC+OSCORE combined request support
  (only present when :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_REQUEST` is enabled)

Per RFC 9668 Section 6, the ``ed-r`` and ``ed-comb-req`` attributes are valueless
(no ``=`` sign) and any present value MUST be ignored by clients.

**Example Discovery Request**:

.. code-block:: text

   GET /.well-known/core

   Response:
   </test>;if=sensor,
   </.well-known/edhoc>;rt=core.edhoc;ed-r;ed-comb-req

**Filtered Discovery**:

Clients can filter for EDHOC resources using query parameters:

.. code-block:: text

   GET /.well-known/core?rt=core.edhoc

   Response:
   </.well-known/edhoc>;rt=core.edhoc;ed-r;ed-comb-req

**Duplicate Prevention**:

If an application defines its own ``/.well-known/edhoc`` resource in the resource
array, the synthetic EDHOC link-value is not added, preventing duplication.

Limitations
-----------

The current implementation:

- Supports the forward flow (message_1 → message_2, message_3 → message_4)
- Does not support the reverse flow (message_2 → message_1, message_4 → message_3)
- Assumes message_4 is not required (most common case)
- Requires uoscore-uedhoc module for full EDHOC processing

EDHOC+OSCORE Combined Request (RFC 9668 Section 3)
===================================================

The EDHOC+OSCORE combined request optimization allows a client to send EDHOC message_3
and the first OSCORE-protected application request in a single CoAP message, reducing
round trips from 3 to 2.

**Combined Payload Format** (RFC 9668 Section 3.2.1):

.. code-block:: text

   COMB_PAYLOAD = EDHOC_MSG_3 / OSCORE_PAYLOAD

Where:

- ``EDHOC_MSG_3``: CBOR byte string containing the final EDHOC message
- ``OSCORE_PAYLOAD``: OSCORE-protected CoAP request

**Message Structure**:

.. code-block:: text

   POST coap://server/resource
   EDHOC option: (empty)
   OSCORE option: (kid = C_R)
   Payload: EDHOC_MSG_3 / OSCORE_PAYLOAD

**Processing Flow** (RFC 9668 Section 3.3.1):

1. Server receives request with EDHOC option and OSCORE option
2. Validates combined payload format (must contain both EDHOC_MSG_3 and OSCORE_PAYLOAD)
3. Extracts C_R (connection identifier) from OSCORE option kid field
4. Retrieves EDHOC session by C_R
5. Processes EDHOC message_3 and derives OSCORE security context
6. Removes EDHOC option from request
7. Replaces payload with OSCORE_PAYLOAD
8. Verifies OSCORE using derived context
9. Dispatches decrypted request to resource handlers

**Error Handling**:

- **Malformed combined payload**: 4.00 Bad Request (not 4.02 Bad Option)
- **EDHOC processing failure**: Unprotected EDHOC error message (see below)
- **OSCORE verification failure**: Standard OSCORE error handling

EDHOC Error Messages (RFC 9668 Section 3.3.1 & RFC 9528 Section 6)
-------------------------------------------------------------------

When EDHOC processing fails in a combined request, the server responds with an
**unprotected** CoAP error response containing an EDHOC error message per RFC 9528
Section 6 and Appendix A.2.3.

**Error Response Format**:

- **Response Code**: 4.00 (Bad Request) for client errors, 5.00 (Internal Server Error) for server errors
- **Content-Format**: application/edhoc+cbor-seq (64)
- **Payload**: CBOR Sequence ``(ERR_CODE : int, ERR_INFO : any)``
- **MUST NOT** be protected with OSCORE (RFC 9668 Section 3.3.1)

**EDHOC Error Codes** (RFC 9528 Section 6.2):

- **ERR_CODE = 1**: Unspecified Error (used for combined request failures)
- **ERR_INFO**: MUST be a text string (tstr) for ERR_CODE = 1

**Example CBOR Sequence** (ERR_CODE=1, ERR_INFO="EDHOC error"):

.. code-block:: text

   0x01 0x6B 45 44 48 4F 43 20 65 72 72 6F 72
   |    |    |-- "EDHOC error" (11 bytes UTF-8)
   |    |-- CBOR tstr header (major type 3, length 11)
   |-- CBOR unsigned int 1

**Security Considerations**:

Per RFC 9528 Section 9.5 and RFC 9668 Section 3.3.1, diagnostic messages in
ERR_INFO SHOULD be generic to avoid leaking sensitive information about the
EDHOC session or server state. The Zephyr implementation uses "EDHOC error"
as a deliberately non-specific diagnostic message.

**When EDHOC Error Messages Are Sent**:

1. **EDHOC message_3 processing failure**: When the EDHOC library rejects message_3
   (e.g., authentication failure, invalid MAC, protocol violation)
2. **message_4 required**: When the EDHOC session requires message_4 but the client
   sent a combined request (RFC 9668 Section 3.3.1 Step 4)

**Response Type**:

- For CON requests: ACK with error payload
- For NON requests: NON with error payload

This ensures the error response follows standard CoAP message type rules while
remaining unprotected as required by RFC 9668.

**Security Considerations**:

1. **Fail-closed**: If EDHOC fails, no OSCORE context is established
2. **Payload size limit**: Enforced via :kconfig:option:`CONFIG_COAP_EDHOC_MAX_COMBINED_PAYLOAD_LEN`
   to prevent resource exhaustion
3. **Session tracking**: EDHOC sessions are cached per C_R with LRU eviction

Current Implementation Status
==============================

The current implementation provides:

1. **EDHOC option definition**: Option number 21 with proper classification
2. **Content-format IDs**: 64 and 65 for EDHOC messages
3. **Combined payload parsing**: Splits EDHOC_MSG_3 and OSCORE_PAYLOAD
4. **Server-side detection**: Detects and validates EDHOC+OSCORE combined requests
5. **Fail-closed behavior**: Rejects EDHOC messages when support is disabled

**Server-side EDHOC+OSCORE Combined Request Processing**:

When :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_REQUEST` is enabled, the server implements
RFC 9668 Section 3.3.1 processing with full RFC 9528 Appendix A.1 compliance:

1. **Step 1**: Validates EDHOC option and OSCORE option presence
2. **Step 2**: Extracts EDHOC message_3 from combined payload (CBOR parsing)
3. **Step 3**: Extracts C_R from OSCORE option 'kid' field
4. **Step 4**: Retrieves EDHOC session by C_R and processes message_3 (derives PRK_out, extracts C_I)
5. **Step 5**: Derives OSCORE Security Context per RFC 9528 Appendix A.1:

   - Uses EDHOC_Exporter with labels 0 (master secret) and 1 (master salt)
   - Context parameter: h'' (empty CBOR byte string)
   - **Sender ID = C_I** (connection identifier for initiator, extracted from EDHOC context)
   - **Recipient ID = C_R** (connection identifier for responder, from OSCORE option)
   - AEAD and HKDF algorithms from EDHOC suite

6. **Step 6-7**: Rebuilds OSCORE request without EDHOC option
7. **Step 8**: Verifies and decrypts OSCORE message using derived context
8. **Step 9**: Dispatches decrypted request to resource handlers

**Response Protection**:

Per RFC 9668 Section 3.3.1, responses to EDHOC+OSCORE combined requests **MUST** be
OSCORE-protected using the derived context. The server automatically:

- Tracks the per-exchange OSCORE context
- Protects all responses (including Observe notifications) with the derived context
- Fails closed if OSCORE protection fails (does not send plaintext)

**OSCORE Context Management**:

The server manages OSCORE contexts internally:

- Allocates contexts from an internal fixed pool (sized by :kconfig:option:`CONFIG_COAP_OSCORE_CTX_CACHE_SIZE`)
- Caches contexts keyed by C_R for subsequent requests
- Evicts contexts based on age (lifetime: :kconfig:option:`CONFIG_COAP_OSCORE_CTX_LIFETIME_MS`)
- Zeroizes contexts on eviction (security best practice)

**Requirements for combined request processing**:

- EDHOC session must be pre-provisioned in the service's ``edhoc_session_cache``
- Application profile must NOT require EDHOC message_4
- OSCORE context cache must have available entries
- When :kconfig:option:`CONFIG_UEDHOC` is enabled: Full uoscore-uedhoc module integration
- When :kconfig:option:`CONFIG_UEDHOC` is disabled: Test wrappers must be provided

**Fail-closed behavior**:

- Malformed combined payload → 4.00 Bad Request
- Unknown C_R session → 4.00 Bad Request
- EDHOC message_3 processing failure → EDHOC error message (ERR_CODE=1, Content-Format 64, EDHOC session aborted)
- message_4 required → EDHOC error message (ERR_CODE=1, Content-Format 64, EDHOC session aborted)
- OSCORE verification failure → appropriate OSCORE error code
- Context allocation failure → 5.00 Internal Server Error

**RFC 9528 Table 14 Compliance**:

The implementation correctly maps connection identifiers to OSCORE IDs per RFC 9528 Appendix A.1 Table 14:

- **EDHOC Responder (Server)**: OSCORE Sender ID = C_I, OSCORE Recipient ID = C_R

This ensures proper bidirectional OSCORE communication after EDHOC key exchange.

**Outer Block-wise Transfer Support (RFC 9668 Section 3.3.2)**:

The server now supports outer Block1 transfers for EDHOC+OSCORE combined requests per
RFC 9668 Section 3.3.2. This allows clients to send large combined payloads using
block-wise transfers before OSCORE/EDHOC processing.

When a combined request uses outer Block1 (RFC 7959), the server implements **Step 0**
processing:

1. **Detection**: Block1 option present AND (EDHOC option present OR matching cache entry)
2. **Reassembly**: Server buffers blocks until all received, then reconstructs full request
3. **Intermediate responses**: Server sends **2.31 Continue** for non-final blocks
4. **Final processing**: After last block, removes outer Block1/Size1 options and proceeds
   with normal EDHOC+OSCORE processing (RFC 9668 Section 3.3.1)

**Configuration Options**:

- :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE`: Number of concurrent
  reassemblies per service (default 4)
- :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_LIFETIME_MS`: Reassembly timeout
  in milliseconds (default 60000)
- :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN`: Maximum reassembled
  payload length (default: same as ``CONFIG_COAP_EDHOC_MAX_COMBINED_PAYLOAD_LEN``)

**Security Considerations**:

- **DoS resistance**: Limited concurrent reassemblies, lifetime timeout, size limit
- **Fail-closed**: Malformed blocks or size violations abort reassembly and clear state
- **Token requirement**: Block1 requires token for tracking (per RFC 7959)
- **Zeroization**: Reassembly buffers are zeroized on eviction/completion

**Error Handling**:

- Missing token → 4.00 Bad Request
- Out-of-order blocks → 4.00 Bad Request
- Inconsistent block size → 4.00 Bad Request
- Size limit exceeded → 4.13 Request Entity Too Large (with Size1 option)
- Expired reassembly → Entry evicted, client must restart

**Example Flow**:

1. Client sends Block1 NUM=0, M=1 with EDHOC option → Server responds 2.31 Continue
2. Client sends Block1 NUM=1, M=1 (no EDHOC option) → Server responds 2.31 Continue
3. Client sends Block1 NUM=2, M=0 (last block) → Server reassembles and processes combined request
4. Server proceeds with EDHOC+OSCORE processing on full payload

**Request-Tag Processing (RFC 9175)**:

Per RFC 9175 Section 3.3, the server-side Block1 reassembly for EDHOC+OSCORE combined
requests treats **Request-Tag as part of the blockwise operation key**. This ensures that
blocks from different operations are not incorrectly associated:

- **Operation key**: Blocks are keyed by (client address, token, **Request-Tag list**)
- **Absent vs present**: Absent Request-Tag is distinct from present with 0-length
- **Repeatable**: Multiple Request-Tag options must match exactly (same values, same order)
- **Opaque**: Request-Tag values are treated as opaque byte strings (per RFC 9175 Section 3.3)
- **Responses**: Request-Tag is **never present in responses** (per RFC 9175 Section 3.4)

If a continuation block has a different Request-Tag list than the first block, the server
treats it as a different operation and responds with **4.00 Bad Request**, clearing the
original reassembly state (fail-closed policy).

**Client-side EDHOC+OSCORE Combined Request (RFC 9668 Section 3.2)**:

The CoAP client library now supports generating EDHOC+OSCORE combined requests per RFC 9668
Section 3.2.1, allowing clients to send EDHOC message_3 and the first OSCORE-protected
application request in a single CoAP message.

**API Usage**:

.. code-block:: c

   #include <zephyr/net/coap_client.h>

   /* Prepare EDHOC message_3 (CBOR bstr encoding per RFC 9528 Section 5.4.2) */
   uint8_t edhoc_msg3[256];
   size_t edhoc_msg3_len = sizeof(edhoc_msg3);
   /* ... generate EDHOC message_3 using EDHOC library ... */

   /* Configure combined request parameters */
   struct coap_client_edhoc_params edhoc_params = {
       .edhoc_msg3 = edhoc_msg3,
       .edhoc_msg3_len = edhoc_msg3_len,
       .combined_request_enabled = true,
   };

   /* Send combined request */
   ret = coap_client_req_edhoc_oscore_combined(&client, sock, &server_addr,
                                                &request, &edhoc_params, NULL);
   if (ret == -EMSGSIZE) {
       /* Combined payload exceeds MAX_UNFRAGMENTED_SIZE */
       /* Fall back to sequential workflow (send message_3 separately) */
   }

**Combined Request Construction** (RFC 9668 Section 3.2.1):

1. **Step 1**: Client builds normal CoAP request
2. **Step 2**: OSCORE protects the request (encryption + OSCORE option)
3. **Step 3**: Builds ``COMB_PAYLOAD = EDHOC_MSG_3 || OSCORE_PAYLOAD``
4. **Step 4**: Constructs outer message with:

   - Same header fields (version/type/token/code/MID)
   - All outer options from OSCORE-protected message
   - **EDHOC option (21) with empty value** (added in correct numeric order)
   - Combined payload

5. **Step 5**: Sends combined request

**Block-wise Constraints** (RFC 9668 Section 3.2.2):

Per RFC 9668 Section 3.2.2, the EDHOC option is only included for the **first inner Block1**
(NUM == 0):

- **First block (NUM=0)**: EDHOC option present, combined payload
- **Continuation blocks (NUM>0)**: Normal OSCORE-protected request (no EDHOC option)

**MAX_UNFRAGMENTED_SIZE Constraint** (RFC 9668 Section 3.2.2 Step 3.1):

If ``edhoc_msg3_len + oscore_payload_len > CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE``,
the function returns ``-EMSGSIZE`` and does not send anything. The application must fall
back to the sequential workflow (send EDHOC message_3 separately via ``/.well-known/edhoc``).

**Error Handling**:

- ``-EMSGSIZE``: Combined payload exceeds MAX_UNFRAGMENTED_SIZE (per RFC 8613 Section 4.1.3.4.2)
- ``-EINVAL``: Invalid parameters (NULL pointers, missing EDHOC_MSG_3, etc.)
- Other negative values: OSCORE protection failure, socket errors, etc.

**Security Considerations**:

1. **Fail-closed**: If combined request construction fails, nothing is sent
2. **OSCORE kid = C_R**: Per RFC 9668 Section 3.2.1 Step 4, the OSCORE option 'kid' field
   carries C_R (connection identifier for responder)
3. **EDHOC_MSG_3 not protected**: Per RFC 9668 Section 3.2.1 Step 2 note, EDHOC message_3
   is not encrypted by OSCORE (only the original CoAP request is protected)

**Requirements**:

- :kconfig:option:`CONFIG_COAP_CLIENT` must be enabled
- :kconfig:option:`CONFIG_COAP_OSCORE` must be enabled
- :kconfig:option:`CONFIG_COAP_EDHOC_COMBINED_REQUEST` must be enabled
- Client must have an OSCORE context attached (``client.oscore_ctx``)
- EDHOC message_3 must be provided as CBOR bstr encoding (per RFC 9528 Section 5.4.2)

**Not yet implemented**:

- Full EDHOC handshake processing (message_1, message_2) - requires application integration
- Client-side outer Block1 generation for combined requests (automatic fragmentation)

The current implementation provides RFC-compliant client-side generation of EDHOC+OSCORE
combined requests with automatic Block1 detection and MAX_UNFRAGMENTED_SIZE enforcement.
Server-side processing is fully implemented with automatic OSCORE context derivation,
response protection, and outer Block1 reassembly support.

Hop-Limit Option (RFC 8768)
============================

The Hop-Limit option indicates the maximum number of hops a request may traverse. It is used
to prevent infinite request loops in proxy chains and to limit the scope of requests.

**Option Details**:

- **Option Number**: 16 (``COAP_OPTION_HOP_LIMIT``)
- **Format**: uint (unsigned integer)
- **Length**: 1 byte
- **Valid Range**: 1-255
- **Default**: 16 (when inserted by proxy)

**Response Code**:

- **5.08 Hop Limit Reached** (``COAP_RESPONSE_CODE_HOP_LIMIT_REACHED``): Returned by a proxy
  when the Hop-Limit reaches 0 and the request cannot be forwarded further.

**Validation Rules** (RFC 8768 Section 3):

1. Requests with Hop-Limit value 0 **MUST** be rejected with 4.00 (Bad Request)
2. Requests with Hop-Limit length != 1 byte **MUST** be rejected with 4.00 (Bad Request)
3. If multiple Hop-Limit options are present, only the first is processed (supernumerary
   options are treated as unrecognized per RFC 7252 Section 5.4.5)

**Endpoint Behavior**:

Endpoints (clients and servers) that are not proxies:

- **MAY** insert a Hop-Limit option in requests
- **MUST** validate Hop-Limit if present (reject invalid values with 4.00)
- Do not need to decrement or modify the Hop-Limit

**Proxy Behavior**:

Proxies that understand Hop-Limit:

- **MUST** decrement Hop-Limit by 1 before forwarding
- **MUST NOT** forward if Hop-Limit becomes 0 (return 5.08 instead)
- **MAY** insert Hop-Limit with default value 16 if absent and policy requires

**API Functions**:

.. code-block:: c

   /* Append Hop-Limit option to a request */
   int coap_append_hop_limit(struct coap_packet *cpkt, uint8_t hop_limit);

   /* Get Hop-Limit value from a request */
   int coap_get_hop_limit(const struct coap_packet *cpkt, uint8_t *hop_limit);

   /* Update Hop-Limit for proxy forwarding (decrement or insert) */
   int coap_hop_limit_proxy_update(struct coap_packet *cpkt, uint8_t default_initial);

**Example Usage - Endpoint**:

.. code-block:: c

   /* Client: Add Hop-Limit to limit request scope */
   struct coap_packet request;
   /* ... initialize request ... */
   ret = coap_append_hop_limit(&request, 10);  /* Max 10 hops */

   /* Server: Validate Hop-Limit in request */
   uint8_t hop_limit;
   ret = coap_get_hop_limit(&request, &hop_limit);
   if (ret == 0) {
       /* Hop-Limit present and valid */
       LOG_INF("Request has Hop-Limit: %u", hop_limit);
   } else if (ret == -EINVAL) {
       /* Invalid Hop-Limit - reject with 4.00 */
       /* (Server implementation does this automatically) */
   }
   /* ret == -ENOENT means Hop-Limit absent, which is valid */

**Example Usage - Proxy**:

.. code-block:: c

   /* Proxy: Update Hop-Limit before forwarding */
   ret = coap_hop_limit_proxy_update(&request, 16);  /* Use default 16 if absent */
   if (ret == -EHOSTUNREACH) {
       /* Hop-Limit reached 0 - send 5.08 response, do not forward */
       send_hop_limit_reached_response(&request);
       return;
   } else if (ret == 0) {
       /* Hop-Limit decremented successfully - forward request */
       forward_request(&request);
   }

**Server-Side Automatic Validation**:

The CoAP server automatically validates Hop-Limit options in all requests:

- Invalid Hop-Limit (length != 1 or value == 0) → 4.00 (Bad Request)
- Valid or absent Hop-Limit → request processed normally

This validation occurs on the outer message before OSCORE/EDHOC processing,
ensuring fail-closed behavior for all requests.

**Security Considerations**:

- Hop-Limit is an **elective** option (even option number), so unrecognized instances
  are silently ignored per RFC 7252 Section 5.4.1
- Malicious clients cannot cause infinite loops by omitting Hop-Limit (proxies insert it)
- Malicious clients cannot bypass Hop-Limit by setting high values (proxies decrement it)

API Reference
*************

.. doxygengroup:: coap
