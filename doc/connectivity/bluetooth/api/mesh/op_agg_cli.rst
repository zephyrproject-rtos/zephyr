.. _bluetooth_mesh_models_op_agg_cli:

Opcodes Aggregator Client
#########################

The Opcodes Aggregator Client model is a foundation model defined by the Bluetooth Mesh
specification. It is an optional model, enabled with the :kconfig:option:`CONFIG_BT_MESH_OP_AGG_CLI`
option.

The Opcodes Aggregator Client model is introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and is used to support the functionality of dispatching a sequence of access layer
messages to nodes supporting the :ref:`bluetooth_mesh_models_op_agg_srv` model.

The Opcodes Aggregator Client model communicates with an Opcodes Aggregator Server model using the
device key of the target node or the application keys configured by the Configuration Client.

If present, the Opcodes Aggregator Client model must only be instantiated on the primary element.

The Opcodes Aggregator Client model is implicitly bound to the device key on initialization. It
should be bound to the same application keys as the client models that are used to produce the
sequence of messages.

To be able to aggregate a message from a client model, it should support an asynchronous API, for
example through callbacks.

API reference
*************

.. doxygengroup:: bt_mesh_op_agg_cli
   :project: Zephyr
   :members:
