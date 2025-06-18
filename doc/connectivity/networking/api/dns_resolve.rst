.. _dns_resolve_interface:

DNS Resolve
###########

.. contents::
    :local:
    :depth: 2

Overview
********

The DNS resolver implements a basic DNS resolver according
to `IETF RFC1035 on Domain Implementation and Specification <https://tools.ietf.org/html/rfc1035>`_.
Supported DNS answers are IPv4/IPv6 addresses and CNAME.

If a CNAME is received, the DNS resolver will create another DNS query.
The number of additional queries is controlled by the
:kconfig:option:`CONFIG_DNS_RESOLVER_ADDITIONAL_QUERIES` Kconfig variable.

The multicast DNS (mDNS) client resolver support can be enabled by setting
:kconfig:option:`CONFIG_MDNS_RESOLVER` Kconfig option.
See `IETF RFC6762 <https://tools.ietf.org/html/rfc6762>`_ for more details
about mDNS.

The link-local multicast name resolution (LLMNR) client resolver support can be
enabled by setting the :kconfig:option:`CONFIG_LLMNR_RESOLVER` Kconfig variable.
See `IETF RFC4795 <https://tools.ietf.org/html/rfc4795>`_ for more details
about LLMNR.

For more information about DNS configuration variables, see:
:zephyr_file:`subsys/net/lib/dns/Kconfig`. The DNS resolver API can be found at
:zephyr_file:`include/zephyr/net/dns_resolve.h`.

DNS-based service discovery queries described in
`IETF RFC6763 <https://datatracker.ietf.org/doc/html/rfc6763>`_
can be done by :c:func:`dns_resolve_service` API.
The returned service descriptions are passed to user supplied callback
and the API sets the address family to :c:macro:`AF_LOCAL` to indicate that
the value is not an IPv4 or IPv6 address but a service description.

Example:

.. code-block:: c

   #include <zephyr/net/dns_resolve.h>

   #define MAX_STR_LEN CONFIG_DNS_RESOLVER_MAX_NAME_LEN

   static void dns_result_cb(enum dns_resolve_status status,
                             struct dns_addrinfo *info,
                             void *user_data)
   {
        if (status == DNS_EAI_CANCELED) {
                /* dns: Timeout while resolving name */
                return;
	}

        if (status == DNS_EAI_INPROGRESS && info) {
                char str[MAX_STR_LEN + 1];

                if (info->ai_family == AF_INET) {
                        net_addr_ntop(AF_INET,
                                      &net_sin(&info->ai_addr)->sin_addr,
                                      str, NET_IPV4_ADDR_LEN);
                } else if (info->ai_family == AF_INET6) {
                        net_addr_ntop(AF_INET6,
                                      &net_sin6(&info->ai_addr)->sin6_addr,
                                      str, NET_IPV6_ADDR_LEN);
                } else if (info->ai_family == AF_LOCAL) {
                        /* service discovery */
                        memset(str, 0, MAX_STR_LEN);
                        memcpy(str, info->ai_canonname,
                               MIN(info->ai_addrlen, MAX_STR_LEN));
                } else {
                        strncpy(str, "Invalid proto family", MAX_STR_LEN + 1);
                }

                str[MAX_STR_LEN] = '\0';

                printk("dns: %s\n", str);
                return;
        }

        if (status == DNS_EAI_ALLDONE) {
                printk("dns: All results received\n");
                return;
        }

        if (status == DNS_EAI_FAIL) {
                printk("dns: No such name found.\n");
                return;
        }

        printk("dns: Unhandled status %d received (errno %d)\n", status, errno);
   }

   #define DNS_TIMEOUT (MSEC_PER_SEC * 5) /* in ms */

   static void discover_service(void)
   {
        int ret = dns_resolve_service(dns_resolve_get_default(),
                                      "_http._tcp.dns-sd.org",
                                      NULL, dns_result_cb,
                                      NULL, DNS_TIMEOUT);
        ...
   }

The above query would return output like this:

.. code-block: console

    Query for '_http._tcp.dns-sd.org' sent.
    dns: . * cnn, world news._http._tcp.dns-sd.org
    dns: .source de télévision, département de langues._http._tcp.dns-sd.org
    dns: . * multicast dns._http._tcp.dns-sd.org
    dns: . * amazon.com, on-line shopping._http._tcp.dns-sd.org
    dns: . * google, searching the web._http._tcp.dns-sd.org
    dns: . * ebay, online auctions._http._tcp.dns-sd.org
    dns: . * apple, makers of the ipod._http._tcp.dns-sd.org
    dns: . * yahoo, maps, weather, and stock quotes._http._tcp.dns-sd.org
    dns: .about bonjour in web browsers._http._tcp.dns-sd.org
    dns: .π._http._tcp.•bullets•.dns-sd.org
    dns: . * dns service discovery._http._tcp.dns-sd.org
    dns: . * wired, technology, culture, business, politics._http._tcp.dns-sd.org
    dns: . * slashdot, news for nerds, stuff that matters._http._tcp.dns-sd.org
    dns: . * bbc, world news._http._tcp.dns-sd.org
    dns: .stuart’s printer._http._tcp.dns-sd.org
    dns: . * zeroconf._http._tcp.dns-sd.org
    dns: All results received

As the service discovery query could return long strings and the packet size could
be large, you might need to adjust following Kconfig options:

- :kconfig:option:`CONFIG_DNS_RESOLVER_MAX_ANSWER_SIZE`. This tells the maximum size of the
  answer, typical value for this option could be 1024. The default size for this option is
  512 bytes.

- :kconfig:option:`CONFIG_DNS_RESOLVER_MAX_NAME_LEN`. This tells the maximum length of the
  returned name. The value depends on your expected data size, typical value might be 128 bytes.

Sample usage
************

See :zephyr:code-sample:`dns-resolve` sample application for details.

API Reference
*************

.. doxygengroup:: dns_resolve
