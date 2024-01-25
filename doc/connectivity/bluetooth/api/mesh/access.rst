.. _bluetooth_mesh_access:

Access layer
############

The access layer is the application's interface to the Bluetooth Mesh network.
The access layer provides mechanisms for compartmentalizing the node behavior
into elements and models, which are implemented by the application.

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

Mesh models have several parameters that can be configured either through
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
configured with the :kconfig:option:`CONFIG_BT_MESH_MODEL_KEY_COUNT` configuration
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
can hold is configured with the :kconfig:option:`CONFIG_BT_MESH_MODEL_GROUP_COUNT`
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

By setting :c:member:`bt_mesh_model_pub.retr_update` to 1, the model can
configure the :c:member:`bt_mesh_model_pub.update` callback to be triggered
on every retransmission. This can, for example, be used by models that make
use of a Delay parameter, which can be adjusted for every retransmission.
The :c:func:`bt_mesh_model_pub_is_retransmission` function can be
used to differentiate a first publication and a retransmission.
The :c:macro:`BT_MESH_PUB_MSG_TOTAL` and :c:macro:`BT_MESH_PUB_MSG_NUM` macros
can be used to return total number of transmissions and the retransmission
number within one publication interval.

Extended models
===============

The Bluetooth Mesh specification allows the mesh models to extend each other.
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
:kconfig:option:`CONFIG_BT_MESH_MODEL_EXTENSIONS` to have any effect.

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

When model data changes frequently, storing it on every change may lead to
increased wear of flash. To reduce the wear, the model can postpone storing of
data by calling :c:func:`bt_mesh_model_data_store_schedule`. The stack will
schedule a work item with delay defined by the
:kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT` option. When the work item is
running, the stack will call the :c:member:`bt_mesh_model_cb.pending_store`
callback for every model that has requested storing of data. The model can
then call :c:func:`bt_mesh_model_data_store` to store the data.

If :kconfig:option:`CONFIG_BT_MESH_SETTINGS_WORKQ` is enabled, the
:c:member:`bt_mesh_model_cb.pending_store` callback is called from a dedicated
thread. This allows the stack to process other incoming and outgoing messages
while model data is being stored. It is recommended to use this option and the
:c:func:`bt_mesh_model_data_store_schedule` function when large amount of data
needs to be stored.

Composition Data
================

The Composition Data provides information about a mesh device.
A device's Composition Data holds information about the elements on the
device, the models that it supports, and other features. The Composition
Data is split into different pages, where each page contains specific feature
information about the device. In order to access this information, the user
may use the :ref:`bluetooth_mesh_models_cfg_srv` model or, if supported,
the :ref:`bluetooth_mesh_lcd_srv` model.

Composition Data Page 0
-----------------------

Composition Data Page 0 provides the fundamental information about a device, and
is mandatory for all mesh devices. It contains the element and model composition,
the supported features, and manufacturer information.

Composition Data Page 1
-----------------------

Composition Data Page 1 provides information about the relationships between models,
and is mandatory for all mesh devices. A model may extend and/or correspond to one
or more models. A model can extend another model by calling :c:func:`bt_mesh_model_extend`,
or correspond to another model by calling :c:func:`bt_mesh_model_correspond`.
:kconfig:option:`CONFIG_BT_MESH_MODEL_EXTENSION_LIST_SIZE` specifies how many model
relations can be stored in the composition on a device, and this number should reflect the
number of :c:func:`bt_mesh_model_extend` and :c:func:`bt_mesh_model_correspond` calls.

Composition Data Page 2
-----------------------

Composition Data Page 2 provides information for supported mesh profiles. Mesh profile
specifications define product requirements for devices that want to support a specific
Bluetooth SIG defined profile. Currently supported profiles can be found in section
3.12 in `Bluetooth SIG Assigned Numbers
<https://www.bluetooth.com/specifications/assigned-numbers/uri-scheme-name-string-mapping/>`_.
Composition Data Page 2 is only mandatory for devices that claim support for one or more
mesh profile(s).

Composition Data Pages 128, 129 and 130
---------------------------------------

Composition Data Pages 128, 129 and 130 mirror Composition Data Pages 0, 1 and 2 respectively.
They are used to represent the new content of the mirrored pages when the Composition Data will
change after a firmware update. See :ref:`bluetooth_mesh_dfu_srv_comp_data_and_models_metadata`
for details.

Delayable messages
==================

The delayable message functionality is enabled with Kconfig option
:kconfig:option:`CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`.
This is an optional functionality that implements specification recommendations for
messages that are transmitted by a model in a response to a received message, also called
response messages.

Response messages should be sent with the following random delays:

* Between 20 and 50 milliseconds if the received message was sent
  to a unicast address
* Between 20 and 500 milliseconds if the received message was sent
  to a group or virtual address

The delayable message functionality is triggered if the :c:member:`bt_mesh_msg_ctx.rnd_delay`
flag is set.
The delayable message functionality stores messages in the local memory while they are
waiting for the random delay expiration.

If the transport layer doesn't have sufficient memory to send a message at the moment
the random delay expires, the message is postponed for another 10 milliseconds.
If the transport layer cannot send a message for any other reason, the delayable message
functionality raises the :c:member:`bt_mesh_send_cb.start` callback with a transport layer
error code.

If the delayable message functionality cannot find enough free memory to store an incoming
message, it will send messages with delay close to expiration to free memory.

When the mesh stack is suspended or reset, messages not yet sent are removed and
the :c:member:`bt_mesh_send_cb.start` callback is raised with an error code.

Delayable publications
======================

The delayable publication functionality implements the specification recommendations for message
publication delays in the following cases:

* Between 20 to 500 milliseconds when the Bluetooth Mesh stack starts or when the publication is
  triggered by the :c:func:`bt_mesh_model_publish` function
* Between 20 to 50 milliseconds for periodically published messages

This feature is optional and enabled with the :kconfig:option:`CONFIG_BT_MESH_DELAYABLE_PUBLICATION`
Kconfig option. When enabled, each model can enable or disable the delayable publication by setting
the :c:member:`bt_mesh_model_pub.delayable` bit field to ``1`` or ``0`` correspondingly. This bit
field can be changed at any time.

API reference
*************

.. doxygengroup:: bt_mesh_access
