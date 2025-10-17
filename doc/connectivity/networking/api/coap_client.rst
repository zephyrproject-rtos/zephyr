.. _coap_client_interface:

CoAP client
###########

.. contents::
    :local:
    :depth: 2

Overview
********

The CoAP client library allows application to send CoAP requests and parse CoAP responses.
The library can be enabled with :kconfig:option:`CONFIG_COAP_CLIENT` Kconfig option.
The application is notified about the response via a callback that is provided to the API
in the request. The CoAP client handles the communication over sockets.
As the CoAP client doesn't create socket it is using, the application is responsible for creating
the socket. Plain UDP or DTLS sockets are supported.

Sample Usage
************

The following is an example of a CoAP client initialization and request sending:

.. code-block:: c

    static struct coap_client;
    struct coap_client_request req = { 0 };

    coap_client_init(&client, NULL);

    req.method = COAP_METHOD_GET;
    req.confirmable = true;
    strcpy(req.path, "test");
    req.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;
    req.cb = response_cb;
    req.payload = NULL;
    req.len = 0;

    /* Sock is a file descriptor referencing a socket, address is the sockaddr struct for the
     * destination address of the request or NULL if the socket is already connected.
     */
    ret = coap_client_req(&client, sock, &address, &req, -1);

Before any requests can be sent, the CoAP client needs to be initialized.
After initialization, the application can send a CoAP request and wait for the response.
Currently only one request can be sent for a single CoAP client at a time. There can be multiple
CoAP clients.

The callback provided in the callback will be called in following cases:

- There is a response for the request
- The request failed for some reason

The callback contains a flag ``last_block``, which indicates if there is more data to come in the
response and means that the current response is part of a blockwise transfer. When the
``last_block`` is set to true, the response is finished and the client is ready for the next request
after returning from the callback.

If the server responds to the request, the library provides the response to the
application through the response callback registered in the request structure.
As the response can be a blockwise transfer and the client calls the callback once per each
block, the application should be to process all of the blocks to be able to process the response.

The following is an example of a very simple response handling function:

.. code-block:: c

    void response_cb(const struct coap_client_response_data *data, void *user_data)
    {
        if (data->result_code >= 0) {
	        LOG_INF("CoAP response from server %d", data->result_code);
                if (data->last_block) {
                        LOG_INF("Last packet received");
                }
        } else {
                LOG_ERR("Error in sending request %d", data->result_code);
        }
    }

CoAP options may also be added to the request by the application. The following is an example of
the application adding a Block2 option to the initial request, to suggest a maximum block size to
the server for a resource that it expects to be large enough to require a blockwise transfer (see
RFC7959 Figure 3: Block-Wise GET with Early Negotiation).

.. code-block:: c

    static struct coap_client;
    struct coap_client_request req = { 0 };

    coap_client_init(&client, NULL);

    req.method = COAP_METHOD_GET;
    req.confirmable = true;
    strcpy(req.path, "test");
    req.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;
    req.cb = response_cb;
    req.options[0] = coap_client_option_initial_block2();
    req.num_options = 1;
    req.payload = NULL;
    req.len = 0;

    ret = coap_client_req(&client, sock, &address, &req, -1);

Optionally, the application can register a payload callback instead of providing a payload pointer
for the CoAP upload. In such cases, the CoAP client library will call this callback when preparing
a PUT/POST request, so that the application can provide the payload in blocks, instead of having to
provide a single contiguous buffer with the entire payload. An example callback, providing the
content of the Lorem Ipsum string can look like this:

.. code-block:: c

    static int lorem_ipsum_cb(size_t offset, const uint8_t **payload, size_t *len,
                              bool *last_block, void *user_data)
    {
        size_t data_left;

        if (offset > LOREM_IPSUM_STRLEN) {
            return -EINVAL;
        }

        *payload = LOREM_IPSUM + offset;

        data_left = LOREM_IPSUM_STRLEN - offset;
        if (data_left <= *len) {
            *len = data_left;
            *last_block = true;
        } else {
            *last_block = false;
        }

        return 0;
    }

The callback can then be registered for the PUT/POST request instead of a payload pointer:

.. code-block:: c

    struct coap_client_request req = { 0 };

    req.method = COAP_METHOD_PUT;
    req.confirmable = true;
    strcpy(req.path, "lorem-ipsum");
    req.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;
    req.cb = response_cb;
    req.payload_cb = lore_ipsum_cb,

    ret = coap_client_req(&client, sock, &address, &req, -1);


API Reference
*************

.. doxygengroup:: coap_client
