.. _bluetooth_mesh_models_health_srv:

Health Server
#############

The Health Server model provides attention callbacks and node diagnostics for
:ref:`bluetooth_mesh_models_health_cli` models. It is primarily used to report
faults in the mesh node and map the mesh nodes to their physical location.

Faults
******

The Health Server model may report a list of faults that have occurred in the
device's lifetime. Typically, the faults are events or conditions that may
alter the behavior of the node, like power outages or faulty peripherals.
Faults are split into warnings and errors. Warnings indicate conditions that
are close to the limits of what the node is designed to withstand, but not
necessarily damaging to the device. Errors indicate conditions that are
outside of the node's design limits, and may have caused invalid behavior or
permanent damage to the device.

Fault values ``0x01`` to ``0x7f`` are reserved for the Bluetooth Mesh
specification, and the full list of specification defined faults are available
in :ref:`bluetooth_mesh_health_faults`. Fault values ``0x80`` to ``0xff`` are
vendor specific. The list of faults are always reported with a company ID to
help interpreting the vendor specific faults.

.. _bluetooth_mesh_models_health_srv_attention:

Attention state
***************

The attention state is used to make the device call attention to itself
through some physical behavior like blinking, playing a sound or vibrating.
The attention state may be used during provisioning to let the user know which
device they're provisioning, as well as through the Health models at runtime.

The attention state is always assigned a timeout in the range of one to 255
seconds when enabled. The Health Server API provides two callbacks for the
application to run their attention calling behavior:
:c:member:`bt_mesh_health_srv_cb.attn_on` is called at the beginning of the
attention period, :c:member:`bt_mesh_health_srv_cb.attn_off` is called at
the end.

The remaining time for the attention period may be queried through
:c:member:`bt_mesh_health_srv.attn_timer`.

API reference
*************

.. doxygengroup:: bt_mesh_health_srv
   :project: Zephyr
   :members:

.. _bluetooth_mesh_health_faults:

Bluetooth Mesh Health Faults
============================

Fault values defined by the Bluetooth Mesh specification.

.. doxygengroup:: bt_mesh_health_faults
   :project: Zephyr
   :members:
