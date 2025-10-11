.. _network_initial_configuration:

Network Stack Initial Configuration
###################################

.. contents::
    :local:
    :depth: 2

This document describes how the network config library can be used to do initial
network stack configuration at runtime when the device boots. The network config
library was mainly developed for testing purposes when a pre-defined setup is
applied to the device when it is starting up. The configuration data is static
and stored in ROM so the device is applied same initial network configuration at
boot.

The configuration library can be used for example to enable DHCPv4 client at boot,
or setup VLAN tags etc.

Network Configuration Options
*****************************

The :kconfig:option:`CONFIG_NET_CONFIG_SETTINGS` enables the network configuration
library. If it is not set, then no network setup is done and the application must
setup network settings itself.

If the above setting is enabled, then two configuration options can be used to setup
the network stack:

* Normal Kconfig options from ``CONFIG_NET_CONFIG_*`` branch.
  This is the legacy way and only suitable for simpler setups where there is only
  one network interface that needs to be setup at boot.
  See available Kconfig options in the network API documentation.

* A yaml configuration file ``net-init-config.yaml``.
  This allows user to describe the device hardware setup and what options to set
  even when the device have multiple network interfaces.

The net-init-config.yaml Syntax
*******************************

The ``net-init-config.yaml`` can be placed to the application directory. When the
application is compiled, this file is used to generate configuration that is then
applied at runtime. The yaml file contents is verified using a schema located in
``scripts/schemas/net-configuration-schema.yaml`` file.

User can use a different configuration yaml file by setting ``NET_INIT_CONFIG_FILE``
when calling cmake or west.

.. code-block:: console

   west build -p -b native_sim myapp -d build -- -DNET_INIT_CONFIG_FILE=my-net-init-config.yaml

The yaml file contains configuration sections for each network interface in the
system, IEEE 802.15.4 configuration or SNTP configuration.

Example:

.. code-block:: yaml

  net_init_config:
    network_interfaces:
      - &main-interface
        name: eth0
        set_name: main-eth0
        set_default: true
	flags:
	  - NET_IF_NO_AUTO_START
        ipv6:
          status: true
          ipv6_addresses:
            - 2001:db8:110::1
          ipv6_multicast_addresses:
            - ff05::114
            - ff15::115
          prefixes:
            - address: "2001:db8::"
              len: 64
              lifetime: 1024
          hop_limit: 48
          multicast_hop_limit: 2
          dhcpv6:
            status: true
            do_request_address: true
            do_request_prefix: false
        ipv4:
          status: true
          ipv4_addresses:
            - 192.0.2.10/24
          ipv4_multicast_addresses:
            - 234.0.0.10
          gateway: 192.0.2.1
          time_to_live: 128
          multicast_time_to_live: 3
          dhcpv4_enabled: true
          ipv4_autoconf_enabled: true

      - &vlan-interface
        set_name: vlan0
        bind_to: *main-interface
	flags:
	  - ^NET_IF_PROMISC
        vlan:
          status: true
          tag: 2432
        ipv4:
          status: true
          dhcpv4_enabled: true

    sntp:
      status: true
      server: sntp.example.com
      timeout: 30
      bind_to: *main-interface

In the above example, there are two network interfaces. One with name ``eth0`` which
is changed to ``main-eth0`` and which is made the default interface. It has both
IPv6 and IPv4 supported. There is also a VLAN interface that is bound to the first
one. Its name is set to ``vlan0`` and it is enabled with tag ``2432``. The VLAN
interface does not have IPv6 enabled, but IPv4 is together with DHCPv4 client support.
Also SNTP is enabled and is using ``sntp.example.com`` server address. The SNTP is
configured to use the first network interface.

The yaml File Configuration Options
***********************************

These options are available for each network configuration domain.

.. table:: The ``network_interface`` options
    :align: left

    +-------------+-------------+-----------------------------------------------------------------+
    | Option name | Type        | Description                                                     |
    +=============+=============+=================================================================+
    | bind_to     | reference   | Bind this object to another network interface.                  |
    |             |             | This is useful for example for VLANs or other types of virtual  |
    |             |             | interfaces.                                                     |
    +-------------+-------------+-----------------------------------------------------------------+
    | name        | string      | Existing name of the network interface.                         |
    |             |             | This is used to find the interface so that we can apply the     |
    |             |             | subsequent configuration to it.                                 |
    |             |             | Either this option or the ``device_name`` option must be given. |
    +-------------+-------------+-----------------------------------------------------------------+
    | device_name | string      | Name of the device of the network interface.                    |
    |             |             | Either this or the ``name`` option must be set in the yaml file.|
    +-------------+-------------+-----------------------------------------------------------------+
    | set_name    | string      | New name of the network interface.                              |
    |             |             | This can be used to change the name of the interface if         |
    |             |             | the default name is not suitable.                               |
    +-------------+-------------+-----------------------------------------------------------------+
    | set_default | bool        | Set this network interface as default one which will be returned|
    |             |             | by the :c:func:`net_if_get_default` function call.              |
    +-------------+-------------+-----------------------------------------------------------------+
    | flags       | string list | Array of network interface flags that should be applied to this |
    |             |             | interface. See ``net_if_flag`` documentation for descriptions of|
    |             |             | the flags. If the flag starts with ``^`` then the flag value is |
    |             |             | cleared.                                                        |
    |             |             | Following flags can be set/cleared:                             |
    |             |             | ``NET_IF_POINTOPOINT``, ``NET_IF_PROMISC``,                     |
    |             |             | ``NET_IF_NO_AUTO_START``, ``NET_IF_FORWARD_MULTICASTS``,        |
    |             |             | ``NET_IF_IPV6_NO_ND``, ``NET_IF_IPV6_NO_MLD``                   |
    +-------------+-------------+-----------------------------------------------------------------+
    | ipv6        | struct      | IPv6 configuration options.                                     |
    +-------------+-------------+-----------------------------------------------------------------+
    | ipv4        | struct      | IPv4 configuration options.                                     |
    +-------------+-------------+-----------------------------------------------------------------+
    | vlan        | struct      | VLAN configuration options.                                     |
    |             |             | Only applicable for Ethernet based interfaces.                  |
    +-------------+-------------+-----------------------------------------------------------------+

.. table:: The ``ipv6`` options
    :align: left

    +--------------------------+-------------+----------------------------------------------------+
    | Option name              | Type        | Description                                        |
    +==========================+=============+====================================================+
    | status                   | bool        | Is the IPv6 enabled for this interface.            |
    |                          |             | If set to ``false``, then these options are no-op. |
    +--------------------------+-------------+----------------------------------------------------+
    | ipv6_addresses           | string list | IPv6 addresses applied to this interface.          |
    |                          |             | The value can contain prefix length.               |
    |                          |             | Example: ``2001:db8::1/64``                        |
    +--------------------------+-------------+----------------------------------------------------+
    | ipv6_multicast_addresses | string list | IPv6 multicast addresses applied to this interface.|
    +--------------------------+-------------+----------------------------------------------------+
    | hop_limit                | int         | Hop limit for the interface.                       |
    +--------------------------+-------------+----------------------------------------------------+
    | multicast_hop_limit      | int         | Multicast hop limit for the interface.             |
    +--------------------------+-------------+----------------------------------------------------+
    | dhcpv6                   | struct      | DHCPv6 client options.                             |
    +--------------------------+-------------+----------------------------------------------------+
    | prefixes                 | list of     | IPv6 prefixes.                                     |
    |                          | structs     |                                                    |
    +--------------------------+-------------+----------------------------------------------------+

.. table:: The ``dhcpv6`` options
    :align: left

    +--------------------+------+-----------------------------------------------------------------+
    | Option name        | Type | Description                                                     |
    +====================+======+=================================================================+
    | status             | bool | Is DHCPv6 client enabled for this interface.                    |
    +--------------------+------+-----------------------------------------------------------------+
    | do_request_address | bool | Request IPv6 address.                                           |
    +--------------------+------+-----------------------------------------------------------------+
    | do_request_prefix  | bool | Requeest IPv6 prefix.                                           |
    +--------------------+------+-----------------------------------------------------------------+

.. table:: The ``prefixes`` options
    :align: left

    +-------------+--------+----------------------------------------------------------------------+
    | Option name | Type   | Description                                                          |
    +=============+========+======================================================================+
    | address     | string | IPv6 address.                                                        |
    +-------------+--------+----------------------------------------------------------------------+
    | len         | int    | Prefix length.                                                       |
    +-------------+--------+----------------------------------------------------------------------+
    | lifetime    | int    | Prefix lifetime.                                                     |
    +-------------+--------+----------------------------------------------------------------------+

.. table:: The ``ipv4`` options
    :align: left

    +--------------------------+-------------+----------------------------------------------------+
    | Option name              | Type        | Description                                        |
    +==========================+=============+====================================================+
    | status                   | bool        | Is the IPv4 enabled for this interface.            |
    |                          |             | If set to ``false``, then these options are no-op. |
    +--------------------------+-------------+----------------------------------------------------+
    | ipv4_addresses           | string list | IPv4 addresses applied to this interface.          |
    |                          |             | The value can contain netmask length.              |
    |                          |             | Example: ``192.0.2.1/24``                          |
    +--------------------------+-------------+----------------------------------------------------+
    | ipv4_multicast_addresses | string list | IPv4 multicast addresses applied to this interface.|
    +--------------------------+-------------+----------------------------------------------------+
    | time_to_live             | int         | Time-to-live value for this interface.             |
    +--------------------------+-------------+----------------------------------------------------+
    | multicast_time_to_live   | int         | Multicast time-to-live value for this interface.   |
    +--------------------------+-------------+----------------------------------------------------+
    | gateway                  | string      | Gateway IPv4 address.                              |
    +--------------------------+-------------+----------------------------------------------------+
    | ipv4_autoconf_enabled    | bool        | Is IPv4 auto-conf enabled to this interface.       |
    +--------------------------+-------------+----------------------------------------------------+
    | dhcpv4_enabled           | bool        | Is DHCPv4 client enabled for this interface.       |
    +--------------------------+-------------+----------------------------------------------------+
    | dhcpv4_server            | struct      | DHCPv4 server options.                             |
    +--------------------------+-------------+----------------------------------------------------+

.. table:: The ``dhcpv4_server`` options
    :align: left

    +--------------------+--------+---------------------------------------------------------------+
    | Option name        | Type   | Description                                                   |
    +====================+========+===============================================================+
    | status             | bool   | Is DHCPv4 server enabled for this interface.                  |
    +--------------------+--------+---------------------------------------------------------------+
    | base_address       | string | Request IPv6 address.                                         |
    +--------------------+--------+---------------------------------------------------------------+

.. table:: The ``vlan`` options
    :align: left

    +-------------+--------+----------------------------------------------------------------------+
    | Option name | Type   | Description                                                          |
    +=============+========+======================================================================+
    | status      | bool   | Is VLAN enabled for this interface.                                  |
    +-------------+--------+----------------------------------------------------------------------+
    | tag         | int    | VLAN tag applied to this interface.                                  |
    +-------------+--------+----------------------------------------------------------------------+

.. table:: The ``sntp`` options
    :align: left

    +-------------+-------------+-----------------------------------------------------------------+
    | Option name | Type        | Description                                                     |
    +=============+=============+=================================================================+
    | status      | bool        | Is SNTP enabled.                                                |
    +-------------+-------------+-----------------------------------------------------------------+
    | server      | string      | SNTP server address.                                            |
    +-------------+-------------+-----------------------------------------------------------------+
    | timeout     | int         | SNTP server connection timeout.                                 |
    +-------------+-------------+-----------------------------------------------------------------+
    | bind_to     | reference   | Connect to server using this network interface.                 |
    +-------------+-------------+-----------------------------------------------------------------+

.. table:: The ``ieee_802_15_4`` options
    :align: left

    +-------------------+-----------+-------------------------------------------------------------+
    | Option name       | Type      | Description                                                 |
    +===================+===========+=============================================================+
    | status            | bool      | Is IEEE 802.15.4 enabled.                                   |
    +-------------------+-----------+-------------------------------------------------------------+
    | bind_to           | reference | Apply the options to this network interface.                |
    +-------------------+-------------+-----------------------------------------------------------+
    | pan_id            | int       | PAN identifier.                                             |
    +-------------------+-----------+-------------------------------------------------------------+
    | channel           | int       | Channel number.                                             |
    +-------------------+-----------+-------------------------------------------------------------+
    | tx_power          | int       | Transmit power.                                             |
    +-------------------+-----------+-------------------------------------------------------------+
    | ack_required      | bool      | Require acknowledgment.                                     |
    +-------------------+-----------+-------------------------------------------------------------+
    | security_key      | int array | IEEE 802.15.4 security key. Maximum length is 16.           |
    +-------------------+-----------+-------------------------------------------------------------+
    | security_key_mode | int       | IEEE 802.15.4 security key mode.                            |
    +-------------------+-----------+-------------------------------------------------------------+
    | security_level    | int       | IEEE 802.15.4 security level.                               |
    +-------------------+-----------+-------------------------------------------------------------+
