.. _bluetooth_mesh_models_op_agg_cli:

Opcodes Aggregator Client
#########################

The Opcodes Aggregator Client model is a foundation model defined by the Bluetooth
mesh specification. It is an optional model, enabled with the :kconfig:option:`CONFIG_BT_MESH_OP_AGG_CLI` option.

The Opcodes Aggregator Client model is introduced in the Bluetooth Mesh Profile
Specification version 1.1, and is used to support the functionality of dispatching
a sequence of access layer messages to nodes supporting the :ref:`bluetooth_mesh_models_op_agg_srv` model.

The Opcodes Aggregator Client model communicates with an Opcodes Aggregator Server model
using the device key of the target node or the application keys configured by the Configuration Client.

The Opcodes Aggregator Client model must only be instantiated on the primary
element, and it is implicitly bound to the device key on initialization.

The Opcodes Aggregator Client model should be bound to the same application keys that the client models,
used to produce the sequence of messages, are bound to.

To be able to aggregate a message from a client model, it should support an asynchronous
API, for example through callbacks.

API reference
*************

.. doxygengroup:: bt_mesh_op_agg_cli
   :project: Zephyr
   :members:
