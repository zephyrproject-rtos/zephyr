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
:option:`CONFIG_DNS_RESOLVER_ADDITIONAL_QUERIES` Kconfig variable.

The multicast DNS (mDNS) client resolver support can be enabled by setting
:option:`CONFIG_MDNS_RESOLVER` Kconfig option.
See `IETF RFC6762 <https://tools.ietf.org/html/rfc6762>`_ for more details
about mDNS.

The link-local multicast name resolution (LLMNR) client resolver support can be
enabled by setting the :option:`CONFIG_LLMNR_RESOLVER` Kconfig variable.
See `IETF RFC4795 <https://tools.ietf.org/html/rfc4795>`_ for more details
about LLMNR.

For more information about DNS configuration variables, see:
:zephyr_file:`subsys/net/lib/dns/Kconfig`. The DNS resolver API can be found at
:zephyr_file:`include/net/dns_resolve.h`.

Sample usage
************

See :ref:`DNS resolve sample application <dns-resolve-sample>` for details.

API Reference
*************

.. doxygengroup:: dns_resolve
   :project: Zephyr
