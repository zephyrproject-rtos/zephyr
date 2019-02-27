.. _net_hostname_interface:

Hostname Configuration
######################

.. contents::
    :local:
    :depth: 2

Overview
********

A networked device might need a hostname, for example, if the device
is configured to be a mDNS responder (see :ref:`dns_resolve_interface` for
details) and needs to respond to ``<hostname>.local`` DNS queries.

The :option:`CONFIG_NET_HOSTNAME_ENABLE` must be set in order
to store the hostname and enable the relevant APIs. If the option is enabled,
then the default hostname is set to be ``zephyr`` by
:option:`CONFIG_NET_HOSTNAME` option.

If the same firmware image is used to flash multiple boards, then it is not
practical to use the same hostname in all of the boards. In that case, one
can enable :option:`CONFIG_NET_HOSTNAME_UNIQUE` which will add a unique
postfix to the hostname. By default the link local address of the first network
interface is used as a postfix. In Ethernet networks, the link local address
refers to MAC address. For example, if the link local address is
``01:02:03:04:05:06``, then the unique hostname could be
``zephyr010203040506``. If you want to set the prefix yourself, then call
``net_hostname_set_postfix()`` before the network interfaces are created.
For example for the Ethernet networks, the initialization priority is set by
:option:`CONFIG_ETH_INIT_PRIORITY` so you would need to set the postfix before
that. The postfix can be set only once.

API Reference
*************

.. doxygengroup:: net_hostname
   :project: Zephyr
