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
prefixed with ``coap_resource_`` and added to a linker file:

.. code-block:: c
    :caption: ``sections-ram.ld``

    #include <zephyr/linker/iterable_sections.h>

    ITERABLE_SECTION_RAM(coap_resource_my_service, Z_LINK_ITERABLE_SUBALIGN)

Add this linker file to your application using CMake:

.. code-block:: cmake
    :caption: ``CMakeLists.txt``

    # Support LD linker template
    zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)

    # Support CMake linker generator
    zephyr_iterable_section(NAME coap_resource_my_service
                            GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
                            SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

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
        return coap_resource_send(resource, &response, addr, addr_len, NULL);
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
of CoAP services. An example using a temperature sensor can look like:

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zephyr/drivers/sensor.h>
    #include <zephyr/net/coap_service.h>

    static void notify_observers(struct k_work *work);
    K_WORK_DELAYABLE_DEFINE(temp_work, notify_observers);

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

        /* Get the sensor data */
        sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
        temp = sensor_value_to_double(&value);

        snprintk(payload, sizeof(payload), "%0.2f°C", temp);

        /* Append payload */
        coap_packet_append_payload_marker(&response);
        coap_packet_append_payload(&response, (uint8_t *)payload, strlen(payload));

        return coap_resource_send(resource, &response, addr, addr_len, NULL);
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
    });

    static void notify_observers(struct k_work *work)
    {
        if (sys_slist_is_empty(&temp_resource.observers)) {
            return;
        }

        coap_resource_notify(&temp_resource);
        k_work_reschedule(&temp_work, K_SECONDS(1));
    }

CoAP Events
***********

By enabling :kconfig:option:`CONFIG_NET_MGMT_EVENT` the user can register for CoAP events. The
following example simply prints when an event occurs.

.. code-block:: c

    #include <zephyr/sys/printk.h>
    #include <zephyr/net/coap_mgmt.h>
    #include <zephyr/net/coap_service.h>

    #define COAP_EVENTS_SET (NET_EVENT_COAP_OBSERVER_ADDED | NET_EVENT_COAP_OBSERVER_REMOVED | \
                             NET_EVENT_COAP_SERVICE_STARTED | NET_EVENT_COAP_SERVICE_STOPPED)

    void coap_event_handler(uint32_t mgmt_event, struct net_if *iface,
                            void *info, size_t info_length, void *user_data)
    {
        switch (mgmt_event) {
        case NET_EVENT_COAP_OBSERVER_ADDED:
            printk("CoAP observer added");
            break;
        case NET_EVENT_COAP_OBSERVER_REMOVED:
            printk("CoAP observer removed");
            break;
        case NET_EVENT_COAP_SERVICE_STARTED:
            if (info != NULL && info_length == sizeof(struct net_event_coap_service)) {
                struct net_event_coap_service *net_event = info;

                printk("CoAP service %s started", net_event->service->name);
            } else {
                printk("CoAP service started");
            }
            break;
        case NET_EVENT_COAP_SERVICE_STOPPED:
            if (info != NULL && info_length == sizeof(struct net_event_coap_service)) {
                struct net_event_coap_service *net_event = info;

                printk("CoAP service %s stopped", net_event->service->name);
            } else {
                printk("CoAP service stopped");
            }
            break;
        }
    }

    NET_MGMT_REGISTER_EVENT_HANDLER(coap_events, COAP_EVENTS_SET, coap_event_handler, NULL);

CoRE Link Format
****************

The :kconfig:option:`CONFIG_COAP_SERVER_WELL_KNOWN_CORE` option enables handling the
``.well-known/core`` GET requests by the server. This allows clients to get a list of hypermedia
links to other resources hosted in that server.

API Reference
*************

.. doxygengroup:: coap_service
.. doxygengroup:: coap_mgmt
