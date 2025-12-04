.. _mac_address_config:

MAC Address Configuration
*************************

Ethernet drivers can delegate most MAC address handling during initialization to
:c:struct:`net_eth_mac_config` and :c:func:`net_eth_mac_load`. The structure
is typically stored in the driver configuration and initialized with
:c:macro:`NET_ETH_MAC_DT_CONFIG_INIT` or :c:macro:`NET_ETH_MAC_DT_INST_CONFIG_INIT`,
which translate the devicetree properties into one of the following behaviors:

* :c:enumerator:`NET_ETH_MAC_STATIC` – use the full ``local-mac-address`` property.
* :c:enumerator:`NET_ETH_MAC_RANDOM` – generate a random locally administered MAC address,
  optionally using the bytes provided in ``zephyr,mac-address-prefix`` as the first octets.
* :c:enumerator:`NET_ETH_MAC_NVMEM` – read the remaining bytes from the ``"mac-address"``
  :ref:`NVMEM<nvmem>` cell, again optionally prefixed by ``zephyr,mac-address-prefix``.
* :c:enumerator:`NET_ETH_MAC_DEFAULT` – fall back to the driver's default logic (for
  example, a factory-programmed MAC address stored in peripheral registers).

Driver integration
==================

Embed the :c:struct:`net_eth_mac_config` structure inside the driver's configuration
and a static buffer inside the driver's data:

.. code-block:: c

   struct my_eth_config {
       struct net_eth_mac_config mac_cfg;
       /* more config fields */
   };

   struct my_eth_data {
       uint8_t mac_addr[NET_ETH_ADDR_LEN];
       /* more data fields */
   };

   static const struct my_eth_config my_eth_config_0 = {
       .mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
   };
   static struct my_eth_data my_eth_data_0;

During initialization, call :c:func:`net_eth_mac_load` before registering
the address with the network interface. The helper copies any statically provided
bytes, fills the remaining octets, and performs the necessary validation.
Drivers can still fall back to SoC-specific storage when no configuration was provided:

.. code-block:: c

   static int my_eth_init(const struct device *dev)
   {
       const struct my_eth_config *cfg = dev->config;
       struct my_eth_data *data = dev->data;
       int ret;

       ret = net_eth_mac_load(&cfg->mac_cfg, data->mac_addr);
       if (ret == -ENODATA) {
           ret = my_eth_hw_read_mac(dev, data->mac_addr);
       }

       return ret;
   }

   static void my_eth_iface_init(struct net_if *iface)
   {
       const struct device *dev = net_if_get_device(iface);
       struct my_eth_data *data = dev->data;

       net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);
   }

Devicetree examples
===================

The examples below show how to pick a MAC address configuration for an ethernet controller node
such as ``&eth0``.

Static MAC address
------------------

.. code-block:: devicetree

   &eth0 {
       local-mac-address = [00 11 22 33 44 55];
   };

Random MAC address with prefix
------------------------------

.. code-block:: devicetree

   &eth0 {
       zephyr,mac-address-prefix = [00 04 25];
       zephyr,random-mac-address;
   };

NVMEM-provided MAC address with prefix
--------------------------------------

.. code-block:: devicetree

   &eth0 {
       zephyr,mac-address-prefix = [00 12 34];
       nvmem-cells = <&macaddr_cell>;
       nvmem-cell-names = "mac-address";
   };

   &eeprom0 {
       nvmem-layout {
           compatible = "fixed-layout";
           #address-cells = <1>;
           #size-cells = <1>;

           macaddr_cell: cell@0 {
               reg = <0x0 0x6>;
               #nvmem-cell-cells = <0>;
           };
       };
   };

When no MAC-related properties are present, :c:func:`net_eth_mac_load` returns ``-ENODATA`` and
the driver is expected to use its existing mechanism (for example, reading the hardware registers or
using a build-time constant).

Changing the MAC address at runtime
===================================

Applications that need to set MAC addresses dynamically (for example to adopt an address obtained
from a management interface) need to enable the networking management API with
:kconfig:option:`CONFIG_NET_MGMT` and use :c:macro:`net_mgmt` with
:c:macro:`NET_REQUEST_ETHERNET_SET_MAC_ADDRESS`.
The request internally calls the driver's :c:func:`ethernet_api.set_config` implementation.

.. code-block:: c

   static int app_set_mac_address(const struct device *dev)
   {
       struct net_if *iface = net_if_lookup_by_dev(dev);
       struct ethernet_req_params params = {
           .mac_address = { { 0x02, 0x00, 0x5E, 0x01, 0x02, 0x03 } },
       };

       /* Make sure the iface is down */

       return net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
                       &params, sizeof(params));
   }
