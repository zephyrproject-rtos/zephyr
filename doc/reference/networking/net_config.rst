.. _net_config_interface:

Network Configuration Library
#############################

.. contents::
    :local:
    :depth: 2

Overview
********

The network configuration library sets up networking devices in a
semi-automatic way during the system boot, based on user-supplied
Kconfig options.

The following Kconfig options affect how configuration library will
setup the system:

.. csv-table:: Kconfig options for network configuration library
   :header: "Option name", "Description"
   :widths: auto

   ":option:`CONFIG_NET_CONFIG_SETTINGS`", "This option controls whether the
   network system is configured or initialized at all. If not set, then the
   config library is not used for initialization and the application needs to
   do all the network related configuration itself. If this option is set,
   then the user can optionally configure static IP addresses to be set to the
   first network interface in the system. Typically setting static IP addresses
   is only usable in testing and should not be used in production code. See
   the config library Kconfig file :zephyr_file:`subsys/net/lib/config/Kconfig`
   for specific options to set the static IP addresses."
   ":option:`CONFIG_NET_CONFIG_AUTO_INIT`", "The networking system is
   automatically configured when the device is started."
   ":option:`CONFIG_NET_CONFIG_INIT_TIMEOUT`", "This tells how long to wait for
   the networking to be ready and available. If for example IPv4 address from
   DHCPv4 is not received within this limit, then a call to
   ``net_config_init()`` will return error during the device startup."
   ":option:`CONFIG_NET_CONFIG_NEED_IPV4`", "The network application needs IPv4
   support to function properly. This option makes sure the network application
   is initialized properly in order to use IPv4.
   If :option:`CONFIG_NET_IPV4` is not enabled, then setting this option will
   automatically enable IPv4."
   ":option:`CONFIG_NET_CONFIG_NEED_IPV6`", "The network application needs IPv6
   support to function properly. This option makes sure the network application
   is initialized properly in order to use IPv6.
   If :option:`CONFIG_NET_IPV6` is not enabled, then setting this option will
   automatically enable IPv6."
   ":option:`CONFIG_NET_CONFIG_NEED_IPV6_ROUTER`", "If IPv6 is enabled, then
   this option tells that the network application needs IPv6 router to exists
   before continuing. This means in practice that the application wants to wait
   until it receives IPv6 router advertisement message before continuing."
   ":option:`CONFIG_NET_CONFIG_BT_NODE`", "Enables application to operate in
   Bluetooth node mode which requires GATT service to be registered and start
   advertising as peripheral."

Sample usage
************

If :option:`CONFIG_NET_CONFIG_AUTO_INIT` is set, then the configuration library
is automatically enabled and run during the device boot. In this case,
the library will call ``net_config_init()`` automatically and the application
does not need to do any network configuration.

If you want to use the network configuration library but without automatic
initialization, you can call ``net_config_init()`` manually. The ``flags``
parameter can be used to give hints to the library about what kind of
functionality the application wishes to have before the actual application
starts.

API Reference
*************

.. doxygengroup:: net_config
   :project: Zephyr
