.. _http_client_multiprotocol_interface:

Multiprotocol HTTP Client
#########################

.. contents::
    :local:
    :depth: 2

Overview
********

The multiprotocol HTTP client sends HTTP requests and delivers the responses to
the application through a callback, with the HTTP version chosen when the client
is initialized. It is meant to cover HTTP/1.1, HTTP/2 and HTTP/3 behind one API
and to replace the :ref:`HTTP client <http_client_interface>` eventually. Only
HTTP/2 is implemented so far.

Unlike the HTTP/1.1-only client, this client owns the connection: a session is
opened once with :c:func:`http_client_connect` and carries any number of
requests, several at a time over HTTP/2. The bytes are carried by a transport
layer; TCP and TLS transports are provided, and an application may supply its
own by implementing :c:struct:`http_client_transport_ops`.

It is enabled with the :kconfig:option:`CONFIG_HTTP_CLIENT_MULTIPROTOCOL`
Kconfig option. The library is experimental.

Sessions
********

A session is either synchronous or asynchronous, chosen in
:c:struct:`http_client_session_config`:

* In a synchronous session, :c:func:`http_client_send_request` sends the request
  and receives the response on the calling thread, returning once the response
  is complete or the request timed out. One request is in flight at a time. The
  socket is only read while a request is in flight, so nothing the server sends
  in between, such as a GOAWAY frame, is noticed before the next request.
* In an asynchronous session, :c:func:`http_client_send_request` returns as
  soon as the request is sent, and the response arrives through the callback
  from the receive context of the transport, the socket service thread for the
  TCP and TLS transports, or from the client's own work queue when a request
  times out. Up to :kconfig:option:`CONFIG_HTTP_CLIENT_MAX_STREAMS` requests
  can be in flight on one session.

In both modes the response is delivered to :c:type:`http_client_response_cb_t`
in pieces, each call carrying a :c:enum:`http_client_transaction_status`. Each
header block is reported in a call with no body data: the response headers, any
1xx interim response before them and any trailers after the body. The body
follows the response headers in chunks that carry no headers. The callback may
abort a transaction by returning a negative value.

Buffers and limits
******************

The client works with fixed buffers, sized by Kconfig options, and a response
need not fit any of them as a whole: the body is delivered as it arrives, and a
header block larger than :kconfig:option:`CONFIG_HTTP_CLIENT_HEADERS_BUFFER_SIZE`
is reported in several callbacks. A single header field, however, must fit
:kconfig:option:`CONFIG_HTTP_CLIENT_RX_BUFFER_SIZE`, once decoded
:kconfig:option:`CONFIG_HTTP_HUFFMAN_DECODE_BUFFER_SIZE`, and the headers buffer
on its own. A field exceeding the receive buffer ends the session, one exceeding
either of the other two only the response, all with ``-ENOBUFS``. Servers behind
content delivery networks tend to send header fields of a few kilobytes, so raise
these from their defaults when talking to such servers.

Over HTTP/2 the server never has more response data in flight than the
:kconfig:option:`CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE` window the client grants
it, so on a fast link the window should be at least as large as the TCP receive
window.

The request target is a path, with the query if any. Absolute URLs and the
CONNECT method are not supported.

Sample usage
************

The client is initialized for a transport and an HTTP version, then connected
to a server address. A synchronous GET looks as follows:

.. code-block:: c

    static int response_cb(struct http_client *client,
                           const struct http_client_request *request,
                           struct http_client_response *response,
                           enum http_client_transaction_status status,
                           void *user_data)
    {
            if (status == HTTP_CLIENT_TRANSACTION_ABORTED) {
                    return 0;
            }

            if (response->header_count > 0) {
                    /* A header block, with the status code decoded. The
                     * fields are only valid during the callback.
                     */
                    printk("status %u\n", response->status_code);

                    for (size_t i = 0; i < response->header_count; i++) {
                            printk("%s: %s\n", response->headers[i].name,
                                   response->headers[i].value);
                    }
            }

            if (response->body_len > 0) {
                    /* A chunk of the body at response->body. */
            }

            return 0;
    }

    struct http_client_transport_tcp_ctx tcp_ctx = { 0 };
    struct http_client_session_config cfg = { 0 };
    struct http_client_request request = { 0 };
    struct http_client client;

    http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &tcp_ctx);

    /* cfg.server_addr and cfg.addrlen hold the server address;
     * cfg.host names the server for the :authority pseudo-header.
     */
    http_client_connect(&client, &cfg);

    request.method = HTTP_GET;
    request.url = "/index.html";
    request.response_cb = response_cb;

    http_client_send_request(&client, &request, 5000, NULL);

    http_client_disconnect(&client);

For HTTPS, the client is initialized with :c:func:`http_client_init_tls` and a
:c:struct:`http_client_transport_tls_ctx` carrying the TLS credential tags and,
optionally, the host name to send as the TLS server name and verify the
certificate against; by default it is the session host, when that is a domain
name. The transport negotiates the HTTP version through ALPN.

A request body is given either in one piece, in which case the client sends a
Content-Length header, or through a :c:type:`http_client_payload_cb_t` that
hands out the body chunk by chunk. Additional request headers are given as an
array, or through a :c:type:`http_client_headers_cb_t`.

See :zephyr:code-sample:`sockets-http-client-multiprotocol` for a complete
application.

API Reference
*************

.. doxygengroup:: http_client_multiprotocol
