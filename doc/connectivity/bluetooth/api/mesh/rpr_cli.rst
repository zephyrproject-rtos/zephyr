.. _bluetooth_mesh_models_rpr_cli:

Remote Provisioning Client
##########################

The Remote Provisioning Client model is a foundation model defined by the Bluetooth
mesh specification. It is enabled with the
:kconfig:option:`CONFIG_BT_MESH_RPR_CLI` option.

The Remote Provisioning Client model is introduced in the Bluetooth Mesh Protocol
Specification version 1.1.
This model provides functionality to remotely provision devices into a mesh network, and perform
Node Provisioning Protocol Interface procedures by interacting with mesh nodes that support the
:ref:`bluetooth_mesh_models_rpr_srv` model.

The Remote Provisioning Client model communicates with a Remote Provisioning Server model
using the device key of the node containing the target Remote Provisioning Server model instance.

If present, the Remote Provisioning Client model must be instantiated on the primary
element.

Scanning
********

The scanning procedure is used to scan for unprovisioned devices located nearby the Remote
Provisioning Server. The Remote Provisioning Client starts a scan procedure by using the
:c:func:`bt_mesh_rpr_scan_start` call:

.. code-block:: C

      static void rpr_scan_report(struct bt_mesh_rpr_cli *cli,
                  const struct bt_mesh_rpr_node *srv,
                  struct bt_mesh_rpr_unprov *unprov,
                  struct net_buf_simple *adv_data)
      {

      }

      struct bt_mesh_rpr_cli rpr_cli = {
         .scan_report = rpr_scan_report,
      };

      const struct bt_mesh_rpr_node srv = {
         .addr = 0x0004,
         .net_idx = 0,
         .ttl = BT_MESH_TTL_DEFAULT,
      };

      struct bt_mesh_rpr_scan_status status;
      uint8_t *uuid = NULL;
      uint8_t timeout = 10;
      uint8_t max_devs = 3;

      bt_mesh_rpr_scan_start(&rpr_cli, &srv, uuid, timeout, max_devs, &status);

The above example shows pseudo code for starting a scan procedure on the target Remote Provisioning
Server node. This procedure will start a ten-second, multiple-device scanning where the generated
scan report will contain a maximum of three unprovisioned devices. If the UUID argument was
specified, the same procedure would only scan for the device with the corresponding UUID. After the
procedure completes, the server sends the scan report that will be handled in the client's
:c:member:`bt_mesh_rpr_cli.scan_report` callback.

Additionally, the Remote Provisioning Client model also supports extended scanning with the
:c:func:`bt_mesh_rpr_scan_start_ext` call. Extended scanning supplements regular scanning by
allowing the Remote Provisioning Server to report additional data for a specific device. The Remote
Provisioning Server will use active scanning to request a scan response from the unprovisioned
device if it is supported by the unprovisioned device.

Provisioning
************

The Remote Provisioning Client starts a provisioning procedure by using the
:c:func:`bt_mesh_provision_remote` call:

.. code-block:: C

      struct bt_mesh_rpr_cli rpr_cli;

      const struct bt_mesh_rpr_node srv = {
         .addr = 0x0004,
         .net_idx = 0,
         .ttl = BT_MESH_TTL_DEFAULT,
      };

      uint8_t uuid[16] = { 0xaa };
      uint16_t addr = 0x0006;
      uint16_t net_idx = 0;

      bt_mesh_provision_remote(&rpr_cli, &srv, uuid, net_idx, addr);

The above example shows pseudo code for remotely provisioning a device through a Remote Provisioning
Server node. This procedure will attempt to provision the device with the corresponding UUID, and
assign the address 0x0006 to its primary element using the network key located at index zero.

.. note::
   During the remote provisioning, the same :c:struct:`bt_mesh_prov` callbacks are triggered as for
   ordinary provisioning. See section :ref:`bluetooth_mesh_provisioning` for further details.

Re-provisioning
***************

In addition to scanning and provisioning functionality, the Remote Provisioning Client also provides
means to reconfigure node addresses, device keys and Composition Data on devices that support the
:ref:`bluetooth_mesh_models_rpr_srv` model. This is provided through the Node Provisioning Protocol
Interface (NPPI) which supports the following three procedures:

* Device Key Refresh procedure: Used to change the device key of the Target node without a need to
  reconfigure the node.
* Node Address Refresh procedure: Used to change the node’s device key and unicast address.
* Node Composition Refresh procedure: Used to change the device key of the node, and to add or
  delete models or features of the node.

The three NPPI procedures can be initiated with the :c:func:`bt_mesh_reprovision_remote` call:

.. code-block:: C

      struct bt_mesh_rpr_cli rpr_cli;
      struct bt_mesh_rpr_node srv = {
         .addr = 0x0006,
         .net_idx = 0,
         .ttl = BT_MESH_TTL_DEFAULT,
      };

      bool composition_changed = false;
      uint16_t new_addr = 0x0009;

      bt_mesh_reprovision_remote(&rpr_cli, &srv, new_addr, composition_changed);

The above example shows pseudo code for triggering a Node Address Refresh procedure on the Target
node. The specific procedure is not chosen directly, but rather through the other parameters that
are inputted. In the example we can see that the current unicast address of the Target is 0x0006,
while the new address is set to 0x0009. If the two addresses were the same, and the
``composition_changed`` flag was set to true, this code would instead trigger a Node Composition
Refresh procedure. If the two addresses were the same, and the ``composition_changed`` flag was set
to false, this code would trigger a Device Key Refresh procedure.

API reference
*************

.. doxygengroup:: bt_mesh_rpr_cli
   :project: Zephyr
   :members:
