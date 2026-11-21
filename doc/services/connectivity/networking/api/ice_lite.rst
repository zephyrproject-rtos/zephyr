.. _ice_lite_interface:

ICE-lite
########

.. contents::
    :local:
    :depth: 2

Overview
********

Interactive Connectivity Establishment (ICE, :rfc:`8445`) finds a path for UDP
data between two peers that may sit behind NATs. A full ICE agent gathers
candidate addresses and probes pairs of them with connectivity checks. A lite
agent (:rfc:`8445` section 2.5) only answers the checks of a full agent: it is
for a device the peer can reach directly, such as one with a public address or
one in the same network.

The library is a lite agent built on :ref:`stun_interface`. It offers host
candidates only, is always the controlled agent and serves one component of one
data stream. It does no I/O: the application owns the UDP socket, which it
shares with the data protocol, and exchanges the credentials and the candidates
with the peer over its signaling channel.

.. note::

   This library is marked :ref:`experimental <api_lifecycle_experimental>`. The
   API and the Kconfig options may change between releases.

Configuration
*************

Enable the agent with :kconfig:option:`CONFIG_ICE_LITE`. It selects
:kconfig:option:`CONFIG_STUN` and needs a cryptographically secure random number
generator for its credentials.

Credentials
   :kconfig:option:`CONFIG_ICE_LITE_UFRAG_LEN` and
   :kconfig:option:`CONFIG_ICE_LITE_PWD_LEN` set the lengths of the username
   fragment and the password, 8 and 24 characters by default. The lower
   bounds, 4 and 22, are those of :rfc:`8445` section 5.3. The password is
   capped at 32 characters, what a PSA key slot holds when the key material
   is stored statically.

Usage
*****

:c:func:`ice_lite_init` gives the agent a username fragment and a password. The
application sends them to the peer, with the address and port of its socket as
a host candidate of the priority :c:macro:`ICE_LITE_HOST_PRIORITY` and the
statement that the agent is lite.

Every datagram the socket receives goes to :c:func:`ice_lite_input` first. It
answers the connectivity checks and tells the application which datagrams are
not STUN and belong to the data protocol (:rfc:`7983`):

.. code-block:: c

   #include <zephyr/net/ice_lite.h>

   static struct ice_lite_agent agent;

   void on_datagram(int sock, const uint8_t *buf, size_t len,
                    const struct net_sockaddr *src, net_socklen_t src_len)
   {
           uint8_t resp[ICE_LITE_RESPONSE_SIZE];
           int ret;

           ret = ice_lite_input(&agent, buf, len, src, resp, sizeof(resp));
           if (ret > 0) {
                   (void)zsock_sendto(sock, resp, ret, 0, src, src_len);
           } else if (ret == -ENOMSG && agent.nominated &&
                      memcmp(src, &agent.remote, src_len) == 0) {
                   /* data of the peer */
           }
   }

A datagram that is not STUN is data only if it comes from the nominated
address: anyone can send to the socket.

Once the peer has nominated a pair, ``agent.nominated`` is set and
``agent.remote`` is the address to send the data to. The peer may nominate
another address later, so the application reads the address each time it sends.
When it has sent nothing for 15 seconds, it sends the message of
:c:func:`ice_lite_keepalive` instead.

The :zephyr:code-sample:`ice-lite` sample shows the agent in an application.

API Reference
*************

.. doxygengroup:: ice_lite
