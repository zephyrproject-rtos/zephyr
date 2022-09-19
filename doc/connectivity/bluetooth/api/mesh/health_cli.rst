.. _bluetooth_mesh_models_health_cli:

Health Client
#############

The Health Client model interacts with a Health Server model to read out
diagnostics and control the node's attention state.

All message passing functions in the Health Client API have ``cli`` as
their first parameter. This is a pointer to the client model instance to be
used in this function call. The second parameter is the ``ctx`` or message
context. Message context contains netkey index, appkey index and unicast
address that the target node uses.

The Health Client model is optional, and may be instantiated in any element.
However, if a Health Client model is instantiated in an element other than the
first, an instance must also be present in the first element.

See :ref:`bluetooth_mesh_health_faults` for a list of specification defined
fault values.

API reference
*************

.. doxygengroup:: bt_mesh_health_cli
