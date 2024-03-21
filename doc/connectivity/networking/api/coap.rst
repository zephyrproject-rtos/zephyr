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
itself, see `IETF RFC7252 The Constrained Application Protocol <https://tools.ietf.org/html/rfc7252>`_.

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

- `RFC7252: The Constrained Application Protocol (CoAP) <https://tools.ietf.org/html/rfc7252>`_
- `RFC6690: Constrained RESTful Environments (CoRE) Link Format <https://tools.ietf.org/html/rfc6690>`_
- `RFC7959: Block-Wise Transfers in the Constrained Application Protocol (CoAP) <https://tools.ietf.org/html/rfc7959>`_
- `RFC7641: Observing Resources in the Constrained Application Protocol (CoAP) <https://tools.ietf.org/html/rfc7641>`_

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

API Reference
*************

.. doxygengroup:: coap
