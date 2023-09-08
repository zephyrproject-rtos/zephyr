.. _socks5_interface:

SOCKS5 Proxy Support
####################

.. contents::
    :local:
    :depth: 2

Overview
********

The SOCKS library implements SOCKS5 support, which allows Zephyr to connect
to peer devices via a network proxy.

See this
`SOCKS5 Wikipedia article <https://en.wikipedia.org/wiki/SOCKS#SOCKS5>`_
for a detailed overview of how SOCKS5 works.

For more information about the protocol itself, see
`IETF RFC1928 SOCKS Protocol Version 5 <https://tools.ietf.org/html/rfc1928>`_.

SOCKS5 API
**********

The SOCKS5 support is enabled by :kconfig:option:`CONFIG_SOCKS` Kconfig variable.
Application wanting to use the SOCKS5 must set the SOCKS5 proxy host address
by calling :c:func:`setsockopt()` like this:

.. code-block:: c

    static int set_proxy(int sock, const struct sockaddr *proxy_addr,
                         socklen_t proxy_addrlen)
    {
        int ret;

        ret = setsockopt(sock, SOL_SOCKET, SO_SOCKS5,
                         proxy_addr, proxy_addrlen);
        if (ret < 0) {
                return -errno;
        }

        return 0;
    }

SOCKS5 Proxy Usage in MQTT
**************************

For MQTT client, there is :c:func:`mqtt_client_set_proxy()` API that the
application can call to setup SOCKS5 proxy. See :zephyr:code-sample:`mqtt-publisher`
sample application for usage example.
