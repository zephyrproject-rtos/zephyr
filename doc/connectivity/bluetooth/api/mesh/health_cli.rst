.. _bluetooth_mesh_models_health_cli:

Health Client
#############

The Health Client model interacts with a Health Server model to read out
diagnostics and control the node's attention state.

All message passing functions in the Health Client API have ``net_idx`` and
``addr`` as their first parameters. These should be set to the network index
and primary unicast address that the target node was provisioned with.

The Health Client model is optional, and may be instantiated in any element.
However, if a Health Client model is instantiated in an element other than the
first, an instance must also be present in the first element.

See :ref:`bluetooth_mesh_health_faults` for a list of specification defined
fault values.

API reference
*************

.. doxygengroup:: bt_mesh_health_cli
