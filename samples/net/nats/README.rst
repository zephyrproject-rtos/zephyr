.. _NATS_Client_Sample:


NATS Client Implementation Sample
#################################


Overview
********

`NATS <http://nats.io/documentation/internals/nats-protocol/>`__ is a
publisher/subscriber protocol implemented on top of TCP.  It is specified in
`NATS Protocol documentation <http://nats.io/documentation/internals/nats-protocol/>`__,
and this is a sample implementation for Zephyr using the new IP stack.
The API is loosely based off of the `Golang API
<https://github.com/nats-io/go-nats>`__.

With this sample, it's possible to subscribe/unsubscribe to a given subject,
and be notified of changes asynchronously.  In order to conserve resources,
the implementation does not keep track of subscribed subjects; that
must be performed by the application itself, so it can ignore unknown/undesired
subjects.

TLS is not supported yet, although basic authentication is. The client will indicate
if it supports username/password if a certain callback is set in the ``struct
nats``.  This callback will then be called, and the user must copy the
username/password to the supplied user/pass buffers.

Content might be also published for a given subject.

The sample application lets one observe the subject "led0", and turn it
"on", "off", or "toggle" its value.  Changing the value will, if supported,
act on a status LED on the development board.  The new status will be
published.

Also worth noting is that most of the networking and GPIO boilerplate has
been shamelessly copied from the IRC bot example.  (Curiously, both
protocols are similar.)

Requirements
************

To test the sample, build the Zephyr application for your platform.  This
has only been tested with the QEMU emulator as provided by the Zephyr SDK,
but it should work with other supported hardware as long as they have enough
memory, the network stack has TCP enabled, and the connectivity hardware is
supported.

As far as the software goes, this has been tested with the official `gnatsd
<https://github.com/nats-io/gnatsd>`__ for the server, and the official
`go-nats <https://github.com/nats-io/go-nats>`__ client library.  Both the
server and clients were set up as per instructions found in their respective
``README.md`` files.

The client was a one-off test that is basically the same code provided in
the `Basic Usage
<https://github.com/nats-io/go-nats/blob/e6bb81b5a5f37ef7bf364bb6276e13813086c6ee/README.md#basic-usage>`__
section as found in the ``go-nats`` README file, however, subscribing to the
topic used in this sample: ``led0``, and publishing values as described
above (``on``, ``off``, and ``toggle``).

Library Usage
*************

Allocate enough space for a ``struct nats``, setting a few callbacks so
that you're notified as events happen:

::

    struct nats nats_ctx = {
        .on_auth_required = on_auth_required,
        .on_message = on_message
    };

The ``on_auth_required()`` and ``on_message()`` functions are part of
your application, and each must have these signatures:

::

    int on_auth_required(struct nats *nats, char **user, char **pass);
    int on_message(struct nats *nats, struct nats_msg *msg);

Both functions should return 0 to signal that they could successfully
handle their role, and a negative value, if they couldn't for any
reason. It's recommended to use a negative integer as provided by
errno.h in order to ease debugging.

The first function, ``on_auth_required()``, is called if the server
notifies that it requires authentication. It's not going to be called if
that's not the case, so it is optional. However, if the server asks for
credentials and this function is not provided, the connection will be
closed and an error will be returned by ``nats_connect()``.

The second function, ``on_message()``, will be called whenever the
server has been notified of a value change. The ``struct nats_msg`` has the
following fields:

::

    struct nats_msg {
        const char *subject;
        const char *sid;
        const char *reply_to;
    };

The field ``reply_to`` may be passed directly to ``nats_publish()``,
in order to publish a reply to this message. If it's ``NULL`` (no
reply-to field in the message from the server), the
``nats_publish()`` function will not reply to a specific mailbox and
will just update the topic value.

In order to manage topic subscription, these functions can be used:

::

    int nats_subscribe(struct nats *nats, const char *subject,
        const char *queue_group, const char *sid);

``subject`` and ``sid`` are validated so that they're actually valid
per the protocol rules. ``-EINVAL`` is returned if they're not.

If ``queue_group`` is NULL, it's not sent to the server.

::

    int nats_unsubscribe(struct nats *nats, const char *sid,
        size_t max_msgs);

``sid`` is validated so it's actually valid per the protocol rules.
-EINVAL is returned if it's not.

``max_msgs`` specifies the number of messages that the server will
send before actually unsubscribing the message. Can be 0 to
immediately unsubscribe.  (See note below.)

Both of these functions will return ``-ENOMEM`` if they couldn't build
the message to transmit to the server. They can also return any error
that ``net_context_send()`` can return.

Note:  In order to conserve resources, the library implementation will not
make note of subscribed topics.  Both ``nats_subscribe()`` and
``nats_unsubscribe()`` functions will only notify the server that the client
is either interested or uninterested in a particular topic.  The
``on_message()`` callback may be called to notify of changes in topics that
have not been subscribed to (or have been recently unsubscribed).  It's up
to the application to decide to ignore the message.

Topics can be published by using the following function:

::

    int nats_publish(struct nats *nats, const char *subject,
        const char *reply_to, const char *payload,
        size_t payload_len);

As usual, ``subject`` is validated and ``-EINVAL`` will be returned if
it's in an invalid format. The ``reply_to`` field can be ``NULL``, in
which case, subscribers to this topic won't receive this information as
well.

As ``net_subscribe()`` and ``net_unsubscribe()``, this function can
return ``-ENOMEM`` or any other errors that ``net_context_send()``
returns.
