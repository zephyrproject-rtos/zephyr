.. _bluetooth_mesh_provisioning:

Provisioning
############

Provisioning is the process of adding devices to a mesh network. It requires
two devices operating in the following roles:

* The *provisioner* represents the network owner, and is responsible for
  adding new nodes to the mesh network.
* The *provisionee* is the device that gets added to the network through the
  Provisioning process. Before the provisioning process starts, the
  provisionee is an *unprovisioned device*.

The Provisioning module in the Zephyr Bluetooth Mesh stack supports both the
Advertising and GATT Provisioning bearers for the provisionee role, as well as
the Advertising Provisioning bearer for the provisioner role.

The Provisioning process
************************

All Bluetooth Mesh nodes must be provisioned before they can participate in a
Bluetooth Mesh network. The Provisioning API provides all the functionality
necessary for a device to become a provisioned mesh node.

Beaconing
=========

To start the provisioning process, the unprovisioned device must first start
broadcasting the Unprovisioned Beacon. This makes it visible to nearby
provisioners, which can initiate the provisioning. To indicate that the device
needs to be provisioned, call :c:func:`bt_mesh_prov_enable`. The device
starts broadcasting the Unprovisioned Beacon with the device UUID and the
``OOB information`` field, as specified in the ``prov`` parameter passed to
:c:func:`bt_mesh_init`. Additionally, a Uniform Resource Identifier (URI)
may be specified, which can point the provisioner to the location of some Out
Of Band information, such as the device's public key or an authentication
value database. The URI is advertised in a separate beacon, with a URI hash
included in the unprovisioned beacon, to tie the two together.


Uniform Resource Identifier
---------------------------

The Uniform Resource Identifier shall follow the format specified in the
Bluetooth Core Specification Supplement. The URI must start with a URI scheme,
encoded as a single utf-8 data point, or the special ``none`` scheme, encoded
as ``0x01``. The available schemes are listed on the `Bluetooth website
<https://www.bluetooth.com/specifications/assigned-numbers/uri-scheme-name-string-mapping/>`_.

Examples of encoded URIs:

.. list-table:: URI encoding examples

  * - URI
    - Encoded
  * - ``http://example.com``
    - ``\x16//example.com``
  * - ``https://www.zephyrproject.org/``
    - ``\x17//www.zephyrproject.org/``
  * - ``just a string``
    - ``\x01just a string``

Provisioning invitation
=======================

The provisioner initiates the Provisioning process by sending a Provisioning
invitation. The invitations prompts the provisionee to call attention to
itself using the Health Server
:ref:`bluetooth_mesh_models_health_srv_attention`, if available.

The Unprovisioned device automatically responds to the invite by presenting a
list of its capabilities, including the supported Out of Band Authentication
methods.

Authentication
==============

After the initial exchange, the provisioner selects an Out of Band (OOB)
Authentication method. This allows the user to confirm that the device the
provisioner connected to is actually the device they intended, and not a
malicious third party.

The Provisioning API supports the following authentication methods for the
provisionee:

* **Static OOB:** An authentication value is assigned to the device in
  production, which the provisioner can query in some application specific
  way.
* **Input OOB:** The user inputs the authentication value. The available input
  actions are listed in :c:enum:`bt_mesh_input_action_t`.
* **Output OOB:** Show the user the authentication value. The available output
  actions are listed in :c:enum:`bt_mesh_output_action_t`.

The application must provide callbacks for the supported authentication
methods in :c:struct:`bt_mesh_prov`, as well as enabling the supported actions
in :c:member:`bt_mesh_prov.output_actions` and
:c:member:`bt_mesh_prov.input_actions`.

When an Output OOB action is selected, the authentication value should be
presented to the user when the output callback is called, and remain until the
:c:member:`bt_mesh_prov.input_complete` or :c:member:`bt_mesh_prov.complete`
callback is called. If the action is ``blink``, ``beep`` or ``vibrate``, the
sequence should be repeated after a delay of three seconds or more.

When an Input OOB action is selected, the user should be prompted when the
application receives the :c:member:`bt_mesh_prov.input` callback. The user
response should be fed back to the Provisioning API through
:c:func:`bt_mesh_input_string` or :c:func:`bt_mesh_input_number`. If
no user response is recorded within 60 seconds, the Provisioning process is
aborted.

Data transfer
=============

After the device has been successfully authenticated, the provisioner
transfers the Provisioning data:

* Unicast address
* A network key
* IV index
* Network flags

  * Key refresh
  * IV update

Additionally, a device key is generated for the node. All this data is stored
by the mesh stack, and the provisioning :c:member:`bt_mesh_prov.complete`
callback gets called.

API reference
*************

.. doxygengroup:: bt_mesh_prov
   :project: Zephyr
   :members:
