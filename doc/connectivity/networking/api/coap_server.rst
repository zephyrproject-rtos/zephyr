.. _coap_server_interface:

CoAP server
###########

.. contents::
    :local:
    :depth: 2

Overview
********

Zephyr comes with a batteries-included CoAP server, which uses services to listen for CoAP
requests. The CoAP services handle communication over sockets and pass requests to registered
CoAP resources.

Setup
*****

Some configuration is required to make sure services can be started using the CoAP server. The
:kconfig:option:`CONFIG_COAP_SERVER` option should be enabled in your project:

.. code-block:: cfg
    :caption: ``prj.conf``

    CONFIG_COAP_SERVER=y

All services are added to a predefined linker section and all resources for each service also get
their respective linker sections. If you would have a service ``my_service`` it has to be
prefixed wth ``coap_resource_`` and added to a linker file:

.. code-block:: c
    :caption: ``sections-ram.ld``

    #include <zephyr/linker/iterable_sections.h>

    ITERABLE_SECTION_RAM(coap_resource_my_service, 4)

Add this linker file to your application using CMake:

.. code-block:: cmake
    :caption: ``CMakeLists.txt``

    zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)

You can now define your service as part of the application:

.. code-block:: c

    #include <zephyr/net/coap_service.h>

    static const uint16_t my_service_port = 5683;

    COAP_SERVICE_DEFINE(my_service, "0.0.0.0", &my_service_port, COAP_SERVICE_AUTOSTART);

.. note::

    Services defined with the ``COAP_SERVICE_AUTOSTART`` flag will be started together with the CoAP
    server thread. Services can be manually started and stopped with ``coap_service_start`` and
    ``coap_service_stop`` respectively.

Sample Usage
************

The following is an example of a CoAP resource registered with our service:

.. code-block:: c

    #include <zephyr/net/coap_service.h>

    static int my_get(struct coap_resource *resource, struct coap_packet *request,
                      struct sockaddr *addr, socklen_t addr_len)
    {
        static const char *msg = "Hello, world!";
        uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
        struct coap_packet response;
        uint16_t id;
        uint8_t token[COAP_TOKEN_MAX_LEN];
        uint8_t tkl, type;

        type = coap_header_get_type(request);
        id = coap_header_get_id(request);
        tkl = coap_header_get_token(request, token);

        /* Determine response type */
        type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

        coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl, token,
                         COAP_RESPONSE_CODE_CONTENT, id);

        /* Set content format */
        coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
                               COAP_CONTENT_FORMAT_TEXT_PLAIN);

        /* Append payload */
        coap_packet_append_payload_marker(&response);
        coap_packet_append_payload(&response, (uint8_t *)msg, sizeof(msg));

        /* Send to response back to the client */
        return coap_resource_send(resource, &response, addr, addr_len);
    }

    static int my_put(struct coap_resource *resource, struct coap_packet *request,
                      struct sockaddr *addr, socklen_t addr_len)
    {
        /* ... Handle the incoming request ... */

        /* Return a CoAP response code as a shortcut for an empty ACK message */
        return COAP_RESPONSE_CODE_CHANGED;
    }

    static const char * const my_resource_path[] = { "test", NULL };
    COAP_RESOURCE_DEFINE(my_resource, my_service, {
        .path = my_resource_path,
        .get = my_get,
        .put = my_put,
    });

.. note::

    As demonstrated in the example above, a CoAP resource handler can return response codes to let
    the server respond with an empty ACK response.

Observable resources
********************

The CoAP server provides logic for parsing observe requests and stores these using the runtime data
of CoAP services. Together with observer events, enabled with
:kconfig:option:`CONFIG_COAP_OBSERVER_EVENTS`, the application can easily keep track of clients
and send state updates. An example using a temperature sensor can look like:

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zephyr/drivers/sensor.h>
    #include <zephyr/net/coap_service.h>

    static void notify_observers(struct k_work *work);
    K_WORK_DELAYABLE_DEFINE(temp_work, notify_observers);

    static void temp_observer_event(struct coap_resource *resource, struct coap_observer *observer,
                                    enum coap_observer_event event)
    {
        /* Only track the sensor temperature if an observer is active */
        if (event == COAP_OBSERVER_ADDED) {
            k_work_schedule(&temp_work, K_SECONDS(1));
        }
    }

    static int send_temperature(struct coap_resource *resource,
                                const struct sockaddr *addr, socklen_t addr_len,
                                uint16_t age, uint16_t id, const uint8_t *token, uint8_t tkl,
                                bool is_response)
    {
        const struct device *dev = DEVICE_DT_GET(DT_ALIAS(ambient_temp0));
        uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
        struct coap_packet response;
        char payload[14];
        struct sensor_value value;
        double temp;
        uint8_t type;

        /* Determine response type */
        type = is_response ? COAP_TYPE_ACK : COAP_TYPE_CON;

        if (!is_response) {
            id = coap_next_id();
        }

        coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl, token,
                         COAP_RESPONSE_CODE_CONTENT, id);

        if (age >= 2U) {
            coap_append_option_int(&response, COAP_OPTION_OBSERVE, age);
        }

        /* Set content format */
        coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
                               COAP_CONTENT_FORMAT_TEXT_PLAIN);

        /* Get the sensor date */
        sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
        temp = sensor_value_to_double(&value);

        snprintk(payload, sizeof(payload), "%0.2f°C", temp);

        /* Append payload */
        coap_packet_append_payload_marker(&response);
        coap_packet_append_payload(&response, (uint8_t *)payload, strlen(payload));

        return coap_resource_send(resource, &response, addr, addr_len);
    }

    static int temp_get(struct coap_resource *resource, struct coap_packet *request,
                        struct sockaddr *addr, socklen_t addr_len)
    {
        uint8_t token[COAP_TOKEN_MAX_LEN];
        uint16_t id;
        uint8_t tkl;
        int r;

        /* Let the CoAP server parse the request and add/remove observers if needed */
        r = coap_resource_parse_observe(resource, request, addr);

        id = coap_header_get_id(request);
        tkl = coap_header_get_token(request, token);

        return send_temperature(resource, addr, addr_len, r == 0 ? resource->age : 0,
                                id, token, tkl, true);
    }

    static void temp_notify(struct coap_resource *resource, struct coap_observer *observer)
    {
        send_temperature(resource, &observer->addr, sizeof(observer->addr), resource->age, 0,
                         observer->token, observer->tkl, false);
    }

    static const char * const temp_resource_path[] = { "sensors", "temp1", NULL };
    COAP_RESOURCE_DEFINE(temp_resource, my_service, {
        .path = temp_resource_path,
        .get = temp_get,
        .notify = temp_notify,
        .observer_event_handler = temp_observer_event,
    });

    static void notify_observers(struct k_work *work)
    {
        if (sys_slist_is_empty(&temp_resource.observers)) {
            return;
        }

        coap_resource_notify(&temp_resource);
        k_work_reschedule(&temp_work, K_SECONDS(1));
    }

CoRE Link Format
****************

The :kconfig:option:`CONFIG_COAP_SERVER_WELL_KNOWN_CORE` option enables handling the
``.well-known/core`` GET requests by the server. This allows clients to get a list of hypermedia
links to other resources hosted in that server.

API Reference
*************

.. doxygengroup:: coap_service
