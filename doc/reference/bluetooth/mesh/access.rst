.. _bluetooth_mesh_access:

Access
######

The Bluetooth Mesh access layer is the application's interface to the mesh
network. The access layer provides mechanisms for compartmentalizing the node
behavior into elements and models, which are implemented by the application.

Mesh models
***********

The functionality of a mesh node is represented by models. A model implements
a single behavior the node supports, like being a light, a sensor or a
thermostat. The mesh models are grouped into *elements*. Each element is
assigned its own unicast address, and may only contain one of each type of
model. Conventionally, each element represents a single aspect of the mesh
node behavior. For instance, a node that contains a sensor, two lights and a
power outlet would spread this functionality across four elements, with each
element instantiating all the models required for a single aspect of the
supported behavior.

The node's element and model structure is specified in the node composition
data, which is passed to :c:func:`bt_mesh_init` during initialization. The
Bluetooth SIG have defined a set of foundation models (see
:ref:`bluetooth_mesh_models`) and a set of models for implementing common
behavior in the `Bluetooth Mesh Model Specification
<https://www.bluetooth.com/specifications/mesh-specifications/>`_. All models
not specified by the Bluetooth SIG are vendor models, and must be tied to a
Company ID.

Mesh Models have several parameters that can be configured either through
initialization of the mesh stack or with the
:ref:`bluetooth_mesh_models_cfg_srv`:

Opcode list
===========

The opcode list contains all message opcodes the model can receive, as well as
the minimum acceptable payload length and the callback to pass them to. Models
can support any number of opcodes, but each opcode can only be listed by one
model in each element.

The full opcode list must be passed to the model structure in the composition
data, and cannot be changed at runtime. The end of the opcode list is
determined by the special :c:macro:`BT_MESH_MODEL_OP_END` entry. This entry
must always be present in the opcode list, unless the list is empty. In that
case, :c:macro:`BT_MESH_MODEL_NO_OPS` should be used in place of a proper
opcode list definition.

AppKey list
===========

The AppKey list contains all the application keys the model can receive
messages on. Only messages encrypted with application keys in the AppKey list
will be passed to the model.

The maximum number of supported application keys each model can hold is
configured with the :option:`CONFIG_BT_MESH_MODEL_KEY_COUNT` configuration
option. The contents of the AppKey list is managed by the
:ref:`bluetooth_mesh_models_cfg_srv`.

Subscription list
=================

A model will process all messages addressed to the unicast address of their
element (given that the utilized application key is present in the AppKey
list). Additionally, the model will process packets addressed to any group or
virtual address in its subscription list. This allows nodes to address
multiple nodes throughout the mesh network with a single message.

The maximum number of supported addresses in the Subscription list each model
can hold is configured with the :option:`CONFIG_BT_MESH_MODEL_GROUP_COUNT`
configuration option. The contents of the subscription list is managed by the
:ref:`bluetooth_mesh_models_cfg_srv`.

Model publication
=================

The models may send messages in two ways:

* By specifying a set of message parameters in a :c:struct:`bt_mesh_msg_ctx`,
  and calling :c:func:`bt_mesh_model_send`.
* By setting up a :c:struct:`bt_mesh_model_pub` structure and calling
  :c:func:`bt_mesh_model_publish`.

When publishing messages with :c:func:`bt_mesh_model_publish`, the model
will use the publication parameters configured by the
:ref:`bluetooth_mesh_models_cfg_srv`. This is the recommended way to send
unprompted model messages, as it passes the responsibility of selecting
message parameters to the network administrator, which likely knows more about
the mesh network than the individual nodes will.

To support publishing with the publication parameters, the model must allocate
a packet buffer for publishing, and pass it to
:c:member:`bt_mesh_model_pub.msg`. The Config Server may also set up period
publication for the publication message. To support this, the model must
populate the :c:member:`bt_mesh_model_pub.update` callback. The
:c:member:`bt_mesh_model_pub.update` callback will be called right before the
message is published, allowing the model to change the payload to reflect its
current state.

Extended models
===============

The Bluetooth Mesh specification allows the Mesh models to extend each other.
When a model extends another, it inherits that model's functionality, and
extension can be used to construct complex models out of simple ones,
leveraging the existing model functionality to avoid defining new opcodes.
Models may extend any number of models, from any element. When one model
extends another in the same element, the two models will share subscription
lists. The mesh stack implements this by merging the subscription lists of the
two models into one, combining the number of subscriptions the models can have
in total. Models may extend models that extend others, creating an "extension
tree". All models in an extension tree share a single subscription list per
element it spans.

Model extensions are done by calling :c:func:`bt_mesh_model_extend` during
initialization. A model can only be extended by one other model, and
extensions cannot be circular. Note that binding of node states and other
relationships between the models must be defined by the model implementations.

The model extension concept adds some overhead in the access layer packet
processing, and must be explicitly enabled with
:option:`CONFIG_BT_MESH_MODEL_EXTENSIONS` to have any effect.

Model data storage
==================

Mesh models may have data associated with each model instance that needs to be
stored persistently. The access API provides a mechanism for storing this
data, leveraging the internal model instance encoding scheme. Models can store
one user defined data entry per instance by calling
:c:func:`bt_mesh_model_data_store`. To be able to read out the data the
next time the device reboots, the model's
:c:member:`bt_mesh_model_cb.settings_set` callback must be populated. This
callback gets called when model specific data is found in the persistent
storage. The model can retrieve the data by calling the ``read_cb`` passed as
a parameter to the callback. See the :ref:`settings_api` module documentation for
details.

API reference
*************

.. doxygengroup:: bt_mesh_access
   :project: Zephyr
   :members:
