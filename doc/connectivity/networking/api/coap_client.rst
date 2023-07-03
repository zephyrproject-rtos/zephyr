.. _coap_client_interface:

CoAP client
###########

.. contents::
    :local:
    :depth: 2

Overview
********

The CoAP client library allows application to send CoAP requests and parse CoAP responses.
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
    req.path = "test";
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

The callback contains a flag `last_block`, which indicates if there is more data to come in the
response and means that the current response is part of a blockwise transfer. When the `last_block`
is set to true, the response is finished and the client is ready for the next request after
returning from the callback.

If the server responds to the request, the library provides the response to the
application through the response callback registered in the request structure.
As the response can be a blockwise transfer and the client calls the callback once per each
block, the application should be to process all of the blocks to be able to process the response.

The following is an example of a very simple response handling function:

.. code-block:: c

    void response_cb(int16_t code, size_t offset, const uint8_t *payload, size_t len,
                     bool last_block, void *user_data)
    {
        if (code >= 0) {
	        LOG_INF("CoAP response from server %d", code);
                if (last_block) {
                        LOG_INF("Last packet received");
                }
        } else {
                LOG_ERR("Error in sending request %d", code);
        }
    }

API Reference
*************

.. doxygengroup:: coap_client
