.. _bluetooth_mesh_shell:

Bluetooth Mesh Shell
####################

The Bluetooth mesh shell subsystem provides a set of Bluetooth mesh shell commands for the :ref:`shell_api` module.
It allows for testing and exploring the Bluetooth mesh API through an interactive interface, without having to write an application.

The Bluetooth mesh shell interface provides access to most Bluetooth mesh features, including provisioning, configuration, and message sending.

Prerequisites
*************

The Bluetooth mesh shell subsystem depends on the application to create the composition data and do the mesh initialization.

Application
***********

The Bluetooth mesh shell subsystem is most easily used through the Bluetooth mesh shell application under ``tests/bluetooth/mesh_shell``.
See :ref:`shell_api` for information on how to connect and interact with the Bluetooth mesh shell application.

Basic usage
***********

The Bluetooth mesh shell subsystem adds a single ``mesh`` command, which holds a set of sub-commands. Every time the device boots up, make sure to call ``mesh init`` before any of the other Bluetooth mesh shell commands can be called::

	uart:~$ mesh init

This is done to ensure that all available log will be printed to the shell output.

Provisioning
============

The mesh node must be provisioned to become part of the network. This is only necessary the first time the device boots up, as the device will remember its provisioning data between reboots.

The simplest way to provision the device is through self-provisioning. To do this the user must provision the device with the default network key and address ``0x0001``, execute::

	uart:~$ mesh prov local 0 0x0001

Since all mesh nodes use the same values for the default network key, this can be done on multiple devices, as long as they're assigned non-overlapping unicast addresses. Alternatively, to provision the device into an existing network, the unprovisioned beacon can be enabled with ``mesh prov pb-adv on`` or ``mesh prov pb-gatt on``. The beacons can be picked up by an external provisioner, which can provision the node into its network.

Once the mesh node is part of a network, its transmission parameters can be controlled by the general configuration commands:

* To set the destination address, call ``mesh target dst <Addr>``.
* To set the network key index, call ``mesh target net <NetKeyIdx>``.
* To set the application key index, call ``mesh target app <AppKeyIdx>``.

By default, the transmission parameters are set to send messages to the provisioned address and network key.

Configuration
=============

By setting the destination address to the local unicast address (``0x0001`` in the ``mesh prov local`` command above), we can perform self-configuration through any of the :ref:`bluetooth_mesh_shell_cfg_cli` commands.

A good first step is to read out the node's own composition data::

	uart:~$ mesh models cfg get-comp

This prints a list of the composition data of the node, including a list of its model IDs.

Next, since the device has no application keys by default, it's a good idea to add one::

	uart:~$ mesh models cfg appkey add 0 0

Message sending
===============

With an application key added (see above), the mesh node's transition parameters are all valid, and the Bluetooth mesh shell can send raw mesh messages through the network.

For example, to send a Generic OnOff Set message, call::

	uart:~$ mesh test net-send 82020100

.. note::
	All multibyte fields model messages are in little endian, except the opcode.

The message will be sent to the current destination address, using the current network and application key indexes. As the destination address points to the local unicast address by default, the device will only send packets to itself. To change the destination address to the All Nodes broadcast address, call::

	uart:~$ mesh target dst 0xffff

With the destination address set to ``0xffff``, any other mesh nodes in the network with the configured network and application keys will receive and process the messages we send.

.. note::
	To change the configuration of the device, the destination address must be set back to the local unicast address before issuing any configuration commands.

Sending raw mesh packets is a good way to test model message handler implementations during development, as it can be done without having to implement the sending model. By default, only the reception of the model messages can be tested this way, as the Bluetooth mesh shell only includes the foundation models. To receive a packet in the mesh node, you have to add a model with a valid opcode handler list to the composition data in ``subsys/bluetooth/mesh/shell.c``, and print the incoming message to the shell in the handler callback.

Parameter formats
*****************

The Bluetooth mesh shell commands are parsed with a variety of formats:

.. list-table:: Parameter formats
	:widths: 1 4 2
	:header-rows: 1

	* - Type
	  - Description
	  - Example
	* - Integers
	  - The default format unless something else is specified. Can be either decimal or hexadecimal.
	  - ``1234``, ``0xabcd01234``
	* - Hexstrings
	  - For raw byte arrays, like UUIDs, key values and message payloads, the parameters should be formatted as an unbroken string of hexadecimal values without any prefix.
	  - ``deadbeef01234``
	* - Booleans
	  - Boolean values are denoted in the API documentation as ``<val(off, on)>``.
	  - ``on``, ``off``, ``enabled``, ``disabled``, ``1``, ``0``

Commands
********

The Bluetooth mesh shell implements a large set of commands. Some of the commands accept parameters, which are mentioned in brackets after the command name. For example, ``mesh lpn set <value: off, on>``. Mandatory parameters are marked with angle brackets (e.g. ``<NetKeyIdx>``), and optional parameters are marked with square brackets (e.g. ``[DstAddr]``).

The Bluetooth mesh shell commands are divided into the following groups:

.. contents::
	:depth: 1
	:local:

.. note::
	Some commands depend on specific features being enabled in the compile time configuration of the application. Not all features are enabled by default. The list of available Bluetooth mesh shell commands can be shown in the shell by calling ``mesh`` without any arguments.

General configuration
=====================

``mesh init``
-------------

	Initialize the mesh shell. This command must be run before any other mesh command.

``mesh reset-local``
--------------------

	Reset the local mesh node to its initial unprovisioned state. This command will also clear the Configuration Database (CDB) if present.

Target
======

The target commands enables the user to monitor and set the target destination address, network index and application index for the shell. These parameters are used by several commands, like provisioning, Configuration Client, etc.

``mesh target dst [DstAddr]``
-----------------------------

	Get or set the message destination address. The destination address determines where mesh packets are sent with the shell, but has no effect on modules outside the shell's control.

	* ``DstAddr``: If present, sets the new 16-bit mesh destination address. If omitted, the current destination address is printed.


``mesh target net [NetKeyIdx]``
-------------------------------

	Get or set the message network index. The network index determines which network key is used to encrypt mesh packets that are sent with the shell, but has no effect on modules outside the shell's control. The network key must already be added to the device, either through provisioning or by a Configuration Client.

	* ``NetKeyIdx``: If present, sets the new network index. If omitted, the current network index is printed.


``mesh target app [AppKeyIdx]``
-------------------------------

	Get or set the message application index. The application index determines which application key is used to encrypt mesh packets that are sent with the shell, but has no effect on modules outside the shell's control. The application key must already be added to the device by a Configuration Client, and must be bound to the current network index.

	* ``AppKeyIdx``: If present, sets the new application index. If omitted, the current application index is printed.

Low Power Node
==============

``mesh lpn set <Val(off, on)>``
-------------------------------

	Enable or disable Low Power operation. Once enabled, the device will turn off its radio and start polling for friend nodes.

	* ``Val``: Sets whether Low Power operation is enabled.

``mesh lpn poll``
-----------------

	Perform a poll to the friend node, to receive any pending messages. Only available when LPN is enabled.

Testing
=======

``mesh test net-send <HexString>``
-----------------------------------

	Send a raw mesh message with the current destination address, network and application index. The message opcode must be encoded manually.

	* ``HexString`` Raw hexadecimal representation of the message to send.

``mesh test iv-update``
-----------------------

	Force an IV update.


``mesh test iv-update-test <Val(off, on)>``
-------------------------------------------

	Set the IV update test mode. In test mode, the IV update timing requirements are bypassed.

	* ``Val``: Enable or disable the IV update test mode.


``mesh test rpl-clear``
-----------------------

	Clear the replay protection list, forcing the node to forget all received messages.

.. warning::

	Clearing the replay protection list breaks the security mechanisms of the mesh node, making it susceptible to message replay attacks. This should never be performed in a real deployment.

Health Server Test
------------------

``mesh test health-srv add-fault <FaultID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Register a new Health Server Fault for the Linux Foundation Company ID.

	* ``FaultID``: ID of the fault to register (``0x0001`` to ``0xFFFF``)


``mesh test health-srv del-fault [FaultID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Remove registered Health Server faults for the Linux Foundation Company ID.

	* ``FaultID``: If present, the given fault ID will be deleted. If omitted, all registered faults will be cleared.

Provisioning
============

To allow a device to broadcast connectable unprovisioned beacons, the :kconfig:option:`CONFIG_BT_MESH_PROV_DEVICE` configuration option must be enabled, along with the :kconfig:option:`CONFIG_BT_MESH_PB_GATT` option.

``mesh prov pb-gatt <Val(off, on)>``
------------------------------------

	Start or stop advertising a connectable unprovisioned beacon. The connectable unprovisioned beacon allows the mesh node to be discovered by nearby GATT based provisioners, and provisioned through the GATT bearer.

	* ``Val``: Enable or disable provisioning with GATT

To allow a device to broadcast unprovisioned beacons, the :kconfig:option:`CONFIG_BT_MESH_PROV_DEVICE` configuration option must be enabled, along with the :kconfig:option:`CONFIG_BT_MESH_PB_ADV` option.

``mesh prov pb-adv <Val(off, on)>``
-----------------------------------

	Start or stop advertising the unprovisioned beacon. The unprovisioned beacon allows the mesh node to be discovered by nearby advertising-based provisioners, and provisioned through the advertising bearer.

	* ``Val``: Enable or disable provisioning with advertiser

To allow a device to provision devices, the :kconfig:option:`CONFIG_BT_MESH_PROVISIONER` and :kconfig:option:`CONFIG_BT_MESH_PB_ADV` configuration options must be enabled.

``mesh prov remote-adv <UUID(1-16 hex)> <NetKeyIdx> <Addr> <AttDur(s)> [AuthType]``
-----------------------------------------------------------------------------------

	Provision a nearby device into the mesh. The mesh node starts scanning for unprovisioned beacons with the given UUID. Once found, the unprovisioned device will be added to the mesh network with the given unicast address, and given the network key indicated by ``NetKeyIdx``.

	* ``UUID``: UUID of the unprovisioned device. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``NetKeyIdx``: Index of the network key to pass to the device.
	* ``Addr``: First unicast address to assign to the unprovisioned device. The device will occupy as many addresses as it has elements, and all must be available.
	* ``AttDur``: The duration in seconds the unprovisioned device will identify itself for, if supported. See :ref:`bluetooth_mesh_models_health_srv_attention` for details.
	* ``AuthType``: If present, the OOB authentication type used for provisioning.

		* ``no``: No OOB (default).
		* ``static``: Static OOB.
		* ``output``: Output OOB.
		* ``input``: Input OOB.

To allow a device to provision devices over GATT, the :kconfig:option:`CONFIG_BT_MESH_PROVISIONER` and :kconfig:option:`CONFIG_BT_MESH_PB_GATT_CLIENT` configuration options must be enabled.

``mesh prov remote-gatt <UUID(1-16 hex)> <NetKeyIdx> <Addr> <AttDur(s)>``
-------------------------------------------------------------------------

	Provision a nearby device into the mesh. The mesh node starts scanning for connectable advertising for PB-GATT with the given UUID. Once found, the unprovisioned device will be added to the mesh network with the given unicast address, and given the network key indicated by ``NetKeyIdx``.

	* ``UUID``: UUID of the unprovisioned device. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``NetKeyIdx``: Index of the network key to pass to the device.
	* ``Addr``: First unicast address to assign to the unprovisioned device. The device will occupy as many addresses as it has elements, and all must be available.
	* ``AttDur``: The duration in seconds the unprovisioned device will identify itself for, if supported. See :ref:`bluetooth_mesh_models_health_srv_attention` for details.

``mesh prov uuid [UUID(1-16 hex)]``
-----------------------------------

	Get or set the mesh node's UUID, used in the unprovisioned beacons.

	* ``UUID``: If present, new 128-bit UUID value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, the current UUID will be printed. To enable this command, the :kconfig:option:`BT_MESH_SHELL_PROV_CTX_INSTANCE` option must be enabled.


``mesh prov input-num <Number>``
--------------------------------

	Input a numeric OOB authentication value. Only valid when prompted by the shell during provisioning. The input number must match the number presented by the other participant in the provisioning.

	* ``Number``: Decimal authentication number.


``mesh prov input-str <String>``
--------------------------------

	Input an alphanumeric OOB authentication value. Only valid when prompted by the shell during provisioning. The input string must match the string presented by the other participant in the provisioning.

	* ``String``: Unquoted alphanumeric authentication string.


``mesh prov static-oob [Val(1-16 hex)]``
----------------------------------------

	Set or clear the static OOB authentication value. The static OOB authentication value must be set before provisioning starts to have any effect. The static OOB value must be same on both participants in the provisioning. To enable this command, the :kconfig:option:`BT_MESH_SHELL_PROV_CTX_INSTANCE` option must be enabled.

	* ``Val``: If present, indicates the new hexadecimal value of the static OOB. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, the static OOB value is cleared.


``mesh prov local <NetKeyIdx> <Addr> [IVI]``
--------------------------------------------

	Provision the mesh node itself. If the Configuration database is enabled, the network key must be created. Otherwise, the default key value is used.

	* ``NetKeyIdx``: Index of the network key to provision.
	* ``Addr``: First unicast address to assign to the device. The device will occupy as many addresses as it has elements, and all must be available.
	* ``IVI``: Indicates the current network IV index. Defaults to 0 if omitted.


``mesh prov beacon-listen <Val(off, on)>``
------------------------------------------

	Enable or disable printing of incoming unprovisioned beacons. Allows a provisioner device to detect nearby unprovisioned devices and provision them. To enable this command, the :kconfig:option:`BT_MESH_SHELL_PROV_CTX_INSTANCE` option must be enabled.

	* ``Val``: Whether to enable the unprovisioned beacon printing.

``mesh prov remote-pub-key <PubKey>``
-------------------------------------
	Provide Device public key.

	* ``PubKey`` - Device public key in big-endian.

``mesh prov auth-method input <Action> <Size>``
-----------------------------------------------
	From the provisioner device, instruct the unprovisioned device to use the specified Input OOB authentication action.

	* ``Action`` - Input action. Allowed values:

		* ``0`` - No input action.
		* ``1`` - Push action set.
		* ``2`` - Twist action set.
		* ``4`` - Enter number action set.
		* ``8`` - Enter String action set.
	* ``Size`` - Authentication size.

``mesh prov auth-method output <Action> <Size>``
------------------------------------------------
	From the provisioner device, instruct the unprovisioned device to use the specified Output OOB authentication action.

	* ``Action`` - Output action. Allowed values:

		* ``0`` - No output action.
		* ``1`` - Blink action set.
		* ``2`` - Vibrate action set.
		* ``4`` - Display number action set.
		* ``8`` - Display String action set.
	* ``Size`` - Authentication size.

``mesh prov auth-method static <Val(1-16 hex)>``
------------------------------------------------
	From the provisioner device, instruct the unprovisioned device to use static OOB authentication, and use the given static authentication value when provisioning.

	* ``Val`` - Static OOB value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.

``mesh prov auth-method none``
------------------------------
	From the provisioner device, don't use any authentication when provisioning new devices. This is the default behavior.

Proxy
=====

The Proxy Server module is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_GATT_PROXY` configuration option.

``mesh proxy identity-enable``
------------------------------

	Enable the Proxy Node Identity beacon, allowing Proxy devices to connect explicitly to this device. The beacon will run for 60 seconds before the node returns to normal Proxy beacons.

The Proxy Client module is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_PROXY_CLIENT` configuration option.

``mesh proxy connect <NetKeyIdx>``
----------------------------------

	Auto-Connect a nearby proxy server into the mesh.

	* ``NetKeyIdx``: Index of the network key to connect.


``mesh proxy disconnect <NetKeyIdx>``
-------------------------------------

	Disconnect the existing proxy connection.

	* ``NetKeyIdx``: Index of the network key to disconnect.


``mesh proxy solicit <NetKeyIdx>``
----------------------------------

	Begin Proxy Solicitation of a subnet. Support of this feature can be enabled through the :kconfig:option:`CONFIG_BT_MESH_PROXY_SOLICITATION` configuration option.

	* ``NetKeyIdx``: Index of the network key to send Solicitation PDUs to.

.. _bluetooth_mesh_shell_cfg_cli:

Models
======

Configuration Client
--------------------

The Configuration Client model is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_CFG_CLI` configuration option. This is implemented as a separate module (``mesh models cfg``) inside the ``mesh models`` subcommand list. This module will work on any instance of the Configuration Client model if the mentioned shell configuration options is enabled, and as long as the Configuration Client model is present in the model composition of the application. This shell module can be used for configuring itself and other nodes in the mesh network.

The Configuration Client uses general message parameters set by ``mesh target dst`` and ``mesh target net`` to target specific nodes. When the Bluetooth mesh shell node is provisioned, given that the :kconfig:option:`BT_MESH_SHELL_PROV_CTX_INSTANCE` option is enabled with the shell provisioning context initialized, the Configuration Client model targets itself by default. Similarly, when another node has been provisioned by the Bluetooth mesh shell, the Configuration Client model targets the new node. In most common use-cases, the Configuration Client is depending on the provisioning features and the Configuration database to be fully functional. The Configuration Client always sends messages using the Device key bound to the destination address, so it will only be able to configure itself and the mesh nodes it provisioned. The following steps are an example of how you can set up a device to start using the Configuration Client commands:

* Initialize the client node (``mesh init``).
* Create the CDB (``mesh cdb create``).
* Provision the local device (``mesh prov local``).
* The shell module should now target itself.
* Monitor the composition data of the local node (``mesh models cfg get-comp``).
* Configure the local node as desired with the Configuration Client commands.
* Provision other devices (``mesh prov beacon-listen``) (``mesh prov remote-adv``) (``mesh prov remote-gatt``).
* The shell module should now target the newly added node.
* Monitor the newly provisioned nodes and their addresses (``mesh cdb show``).
* Monitor the composition data of the target device (``mesh models cfg get-comp``).
* Configure the node as desired with the Configuration Client commands.

``mesh models cfg target get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the target Configuration server for the Configuration Client model.

``mesh models cfg help``
^^^^^^^^^^^^^^^^^^^^^^^^

	Print information for the Configuration Client shell module.

``mesh models cfg reset``
^^^^^^^^^^^^^^^^^^^^^^^^^

	Reset the target device.

``mesh models cfg timeout [Timeout(s)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get and set the Config Client model timeout used during message sending.

	* ``Timeout``: If present, set the Config Client model timeout in seconds. If omitted, the current timeout is printed.


``mesh models cfg get-comp [Page]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Read a composition data page. The full composition data page will be printed. If the target does not have the given page, it will return the last page before it.

	* ``Page``: The composition data page to request. Defaults to 0 if omitted.


``mesh models cfg beacon [Val(off, on)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the network beacon transmission.

	* ``Val``: If present, enables or disables sending of the network beacon. If omitted, the current network beacon state is printed.


``mesh models cfg ttl [TTL]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the default TTL value.

	* ``TTL``: If present, sets the new default TTL value. Leagal TTL values are 0x00 and 0x02-0x7f. If omitted, the current default TTL value is printed.


``mesh models cfg friend [Val(off, on)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the Friend feature.

	* ``Val``: If present, enables or disables the Friend feature. If omitted, the current Friend feature state is printed:

		* ``0x00``: The feature is supported, but disabled.
		* ``0x01``: The feature is enabled.
		* ``0x02``: The feature is not supported.


``mesh models cfg gatt-proxy [Val(off, on)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the GATT Proxy feature.

	* ``Val``: If present, enables or disables the GATT Proxy feature. If omitted, the current GATT Proxy feature state is printed:

		* ``0x00``: The feature is supported, but disabled.
		* ``0x01``: The feature is enabled.
		* ``0x02``: The feature is not supported.


``mesh models cfg relay [<Val(off, on)> [<Count> [Int(ms)]]]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the Relay feature and its parameters.

	* ``Val``: If present, enables or disables the Relay feature. If omitted, the current Relay feature state is printed:

		* ``0x00``: The feature is supported, but disabled.
		* ``0x01``: The feature is enabled.
		* ``0x02``: The feature is not supported.

	* ``Count``: Sets the new relay retransmit count if ``val`` is ``on``. Ignored if ``val`` is ``off``. Legal retransmit count is 0-7. Defaults to ``2`` if omitted.
	* ``Int``: Sets the new relay retransmit interval in milliseconds if ``val`` is ``on``. Legal interval range is 10-320 milliseconds. Ignored if ``val`` is ``off``. Defaults to ``20`` if omitted.

``mesh models cfg node-id <NetKeyIdx> [Identity]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or Set of current Node Identity state of a subnet.

	* ``NetKeyIdx``: The network key index to Get/Set.
	* ``Identity``: If present, sets the identity of Node Identity state.

``mesh models cfg polltimeout-get <LPNAddr>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get current value of the PollTimeout timer of the LPN within a Friend node.

	* ``LPNAddr`` Address of Low Power node.

``mesh models cfg net-transmit-param [<Count> <Int(ms)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the network transmit parameters.

	* ``Count``: Sets the number of additional network transmits for every sent message. Legal retransmit count is 0-7.
	* ``Int``: Sets the new network retransmit interval in milliseconds. Legal interval range is 10-320 milliseconds.


``mesh models cfg netkey add <NetKeyIdx> [Key(1-16 hex)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add a network key to the target node. Adds the key to the Configuration Database if enabled.

	* ``NetKeyIdx``: The network key index to add.
	* ``Key``: If present, sets the key value as a 128-bit hexadecimal value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. Only valid if the key does not already exist in the Configuration Database. If omitted, the default key value is used.


``mesh models cfg netkey upd <NetKeyIdx> [Key(1-16 hex)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Update a network key to the target node.

	* ``NetKeyIdx``: The network key index to updated.
	* ``Key``: If present, sets the key value as a 128-bit hexadecimal value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, the default key value is used.

``mesh models cfg netkey get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of known network key indexes.


``mesh models cfg netkey del <NetKeyIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete a network key from the target node.

	* ``NetKeyIdx``: The network key index to delete.


``mesh models cfg appkey add <NetKeyIdx> <AppKeyIdx> [Key(1-16 hex)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add an application key to the target node. Adds the key to the Configuration Database if enabled.

	* ``NetKeyIdx``: The network key index the application key is bound to.
	* ``AppKeyIdx``: The application key index to add.
	* ``Key``: If present, sets the key value as a 128-bit hexadecimal value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. Only valid if the key does not already exist in the Configuration Database. If omitted, the default key value is used.

``mesh models cfg appkey upd <NetKeyIdx> <AppKeyIdx> [Key(1-16 hex)]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Update an application key to the target node.

	* ``NetKeyIdx``: The network key index the application key is bound to.
	* ``AppKeyIdx``: The application key index to update.
	* ``Key``: If present, sets the key value as a 128-bit hexadecimal value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, the default key value is used.

``mesh models cfg appkey get <NetKeyIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of known application key indexes bound to the given network key index.

	* ``NetKeyIdx``: Network key indexes to get a list of application key indexes from.


``mesh models cfg appkey del <NetKeyIdx> <AppKeyIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete an application key from the target node.

	* ``NetKeyIdx``: The network key index the application key is bound to.
	* ``AppKeyIdx``: The application key index to delete.


``mesh models cfg model app-bind <Addr> <AppKeyIdx> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Bind an application key to a model. Models can only encrypt and decrypt messages sent with application keys they are bound to.

	* ``Addr``: Address of the element the model is on.
	* ``AppKeyIdx``: The application key to bind to the model.
	* ``MID``: The model ID of the model to bind the key to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model app-unbind <Addr> <AppKeyIdx> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Unbind an application key from a model.

	* ``Addr``: Address of the element the model is on.
	* ``AppKeyIdx``: The application key to unbind from the model.
	* ``MID``: The model ID of the model to unbind the key from.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model app-get <ElemAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of application keys bound to a model.

	* ``ElemAddr``: Address of the element the model is on.
	* ``MID``: The model ID of the model to get the bound keys of.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model pub <Addr> <MID> [CID] [<PubAddr> <AppKeyIdx> <Cred(off, on)> <TTL> <PerRes> <PerSteps> <Count> <Int(ms)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the publication parameters of a model. If all publication parameters are included, they become the new publication parameters of the model.
	If all publication parameters are omitted, print the current publication parameters of the model.

	* ``Addr``: Address of the element the model is on.
	* ``MID``: The model ID of the model to get the bound keys of.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

	Publication parameters:

		* ``PubAddr``: The destination address to publish to.
		* ``AppKeyIdx``: The application key index to publish with.
		* ``Cred``: Whether to publish with Friendship credentials when acting as a Low Power Node.
		* ``TTL``: TTL value to publish with (``0x00`` to ``0x07f``).
		* ``PerRes``: Resolution of the publication period steps:

			* ``0x00``: The Step Resolution is 100 milliseconds
			* ``0x01``: The Step Resolution is 1 second
			* ``0x02``: The Step Resolution is 10 seconds
			* ``0x03``: The Step Resolution is 10 minutes
		* ``PerSteps``: Number of publication period steps, or 0 to disable periodic publication.
		* ``Count``: Number of retransmission for each published message (``0`` to ``7``).
		* ``Int`` The interval between each retransmission, in milliseconds. Must be a multiple of 50.

``mesh models cfg model pub-va <Addr> <UUID(1-16 hex)> <AppKeyIdx> <Cred(off, on)> <TTL> <PerRes> <PerSteps> <Count> <Int(ms)> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the publication parameters of a model.

	* ``Addr``: Address of the element the model is on.
	* ``MID``: The model ID of the model to get the bound keys of.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

	Publication parameters:

		* ``UUID``: The destination virtual address to publish to. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
		* ``AppKeyIdx``: The application key index to publish with.
		* ``Cred``: Whether to publish with Friendship credentials when acting as a Low Power Node.
		* ``TTL``: TTL value to publish with (``0x00`` to ``0x07f``).
		* ``PerRes``: Resolution of the publication period steps:

			* ``0x00``: The Step Resolution is 100 milliseconds
			* ``0x01``: The Step Resolution is 1 second
			* ``0x02``: The Step Resolution is 10 seconds
			* ``0x03``: The Step Resolution is 10 minutes
		* ``PerSteps``: Number of publication period steps, or 0 to disable periodic publication.
		* ``Count``: Number of retransmission for each published message (``0`` to ``7``).
		* ``Int`` The interval between each retransmission, in milliseconds. Must be a multiple of 50.


``mesh models cfg model sub-add <ElemAddr> <SubAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Subscription the model to a group address. Models only receive messages sent to their unicast address or a group or virtual address they subscribe to. Models may subscribe to multiple group and virtual addresses.

	* ``ElemAddr``: Address of the element the model is on.
	* ``SubAddr``: 16-bit group address the model should subscribe to (``0xc000`` to ``0xFEFF``).
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model sub-del <ElemAddr> <SubAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Unsubscribe a model from a group address.

	* ``ElemAddr``: Address of the element the model is on.
	* ``SubAddr``: 16-bit group address the model should remove from its subscription list (``0xc000`` to ``0xFEFF``).
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model sub-add-va <ElemAddr> <LabelUUID(1-16 hex)> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Subscribe the model to a virtual address. Models only receive messages sent to their unicast address or a group or virtual address they subscribe to. Models may subscribe to multiple group and virtual addresses.

	* ``ElemAddr``: Address of the element the model is on.
	* ``LabelUUID``: 128-bit label UUID of the virtual address to subscribe to. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg model sub-del-va <ElemAddr> <LabelUUID(1-16 hex)> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Unsubscribe a model from a virtual address.

	* ``ElemAddr``: Address of the element the model is on.
	* ``LabelUUID``: 128-bit label UUID of the virtual address to remove the subscription of. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

``mesh models cfg model sub-ow <ElemAddr> <SubAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Overwrite all model subscriptions with a single new group address.

	* ``ElemAddr``: Address of the element the model is on.
	* ``SubAddr``: 16-bit group address the model should added to the subscription list (``0xc000`` to ``0xFEFF``).
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

``mesh models cfg model sub-ow-va <ElemAddr> <LabelUUID(1-16 hex)> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Overwrite all model subscriptions with a single new virtual address. Models only receive messages sent to their unicast address or a group or virtual address they subscribe to. Models may subscribe to multiple group and virtual addresses.

	* ``ElemAddr``: Address of the element the model is on.
	* ``LabelUUID``: 128-bit label UUID of the virtual address as the new Address to be added to the subscription list. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``MID``: The model ID of the model to add the subscription to.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

``mesh models cfg model sub-del-all <ElemAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Remove all group and virtual address subscriptions from of a model.

	* ``ElemAddr``: Address of the element the model is on.
	* ``MID``: The model ID of the model to Unsubscribe all.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.

``mesh models cfg model sub-get <ElemAddr> <MID> [CID]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of addresses the model subscribes to.

	* ``ElemAddr``: Address of the element the model is on.
	* ``MID``: The model ID of the model to get the subscription list of.
	* ``CID``: If present, determines the Company ID of the model. If omitted, the model is a Bluetooth SIG defined model.


``mesh models cfg krp <NetKeyIdx> [Phase]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the key refresh phase of a subnet.

	* ``NetKeyIdx``: The identified network key used to Get/Set the current Key Refresh Phase state.
	* ``Phase``: New Key Refresh Phase. Valid phases are:

		* ``0x00``: Normal operation; Key Refresh procedure is not active
		* ``0x01``: First phase of Key Refresh procedure
		* ``0x02``: Second phase of Key Refresh procedure

``mesh models cfg hb-sub [<Src> <Dst> <Per>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the Heartbeat subscription parameters. A node only receives Heartbeat messages matching the Heartbeat subscription parameters. Sets the Heartbeat subscription parameters if present, or prints the current Heartbeat subscription parameters if called with no parameters.

	* ``Src``: Unicast source address to receive Heartbeat messages from.
	* ``Dst``: Destination address to receive Heartbeat messages on.
	* ``Per``: Logarithmic representation of the Heartbeat subscription period:

		* ``0``: Heartbeat subscription will be disabled.
		* ``1`` to ``17``: The node will subscribe to Heartbeat messages for 2\ :sup:`(period - 1)` seconds.


``mesh models cfg hb-pub [<Dst> <Count> <Per> <TTL> <Features> <NetKeyIdx>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get or set the Heartbeat publication parameters. Sets the Heartbeat publication parameters if present, or prints the current Heartbeat publication parameters if called with no parameters.

	* ``Dst``: Destination address to publish Heartbeat messages to.
	* ``Count``: Logarithmic representation of the number of Heartbeat messages to publish periodically:

		* ``0``: Heartbeat messages are not published periodically.
		* ``1`` to ``17``: The node will periodically publish 2\ :sup:`(count - 1)` Heartbeat messages.
		* ``255``: Heartbeat messages will be published periodically indefinitely.

	* ``Per``: Logarithmic representation of the Heartbeat publication period:

		* ``0``: Heartbeat messages are not published periodically.
		* ``1`` to ``17``: The node will publish Heartbeat messages every 2\ :sup:`(period - 1)` seconds.

	* ``TTL``: The TTL value to publish Heartbeat messages with (``0x00`` to ``0x7f``).
	* ``Features``: Bitfield of features that should trigger a Heartbeat publication when changed:

		* ``Bit 0``: Relay feature.
		* ``Bit 1``: Proxy feature.
		* ``Bit 2``: Friend feature.
		* ``Bit 3``: Low Power feature.

	* ``NetKeyIdx``: Index of the network key to publish Heartbeat messages with.


Health Client
-------------

The Health Client model is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_HEALTH_CLI` configuration option. This is implemented as a separate module (``mesh models health``) inside the ``mesh models`` subcommand list. This module will work on any instance of the Health Client model if the mentioned shell configuration options is enabled, and as long as one or more Health Client model(s) is present in the model composition of the application. This shell module can be used to trigger interaction between Health Clients and Servers on devices in a Mesh network.

By default, the module will choose the first Health Client instance in the model composition when using the Health Client commands. To choose a spesific Health Client instance the user can utilize the commands ``mesh models health instance set`` and ``mesh models health instance get-all``.

The Health Client may use the general messages parameters set by ``mesh target dst``, ``mesh target net`` and ``mesh target app`` to target specific nodes. If the shell target destination address is set to zero, the targeted Health Client will attempt to publish messages using its configured publication parameters.

``mesh models health instance set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the Health Client model instance to use.

	* ``ElemIdx``: Element index of Health Client model.

``mesh models health instance get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Prints all available Health Client model instances on the device.

``mesh models health fault-get <CID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of registered faults for a Company ID.

	* ``CID``: Company ID to get faults for.


``mesh models health fault-clear <CID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Clear the list of faults for a Company ID.

	* ``CID``: Company ID to clear the faults for.


``mesh models health fault-clear-unack <CID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Clear the list of faults for a Company ID without requesting a response.

	* ``CID``: Company ID to clear the faults for.


``mesh models health fault-test <CID> <TestID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Invoke a self-test procedure, and show a list of triggered faults.

	* ``CID``: Company ID to perform self-tests for.
	* ``TestID``: Test to perform.


``mesh models health fault-test-unack <CID> <TestID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Invoke a self-test procedure without requesting a response.

	* ``CID``: Company ID to perform self-tests for.
	* ``TestID``: Test to perform.


``mesh models health period-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the current Health Server publish period divisor.


``mesh models health period-set <Divisor>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the current Health Server publish period divisor. When a fault is detected, the Health Server will start publishing is fault status with a reduced interval. The reduced interval is determined by the Health Server publish period divisor: Fault publish period = Publish period / 2\ :sup:`divisor`.

	* ``Divisor``: The new Health Server publish period divisor.


``mesh models health period-set-unack <Divisor>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the current Health Server publish period divisor. When a fault is detected, the Health Server will start publishing is fault status with a reduced interval. The reduced interval is determined by the Health Server publish period divisor: Fault publish period = Publish period / 2\ :sup:`divisor`.

	* ``Divisor``: The new Health Server publish period divisor.


``mesh models health attention-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the current Health Server attention state.


``mesh models health attention-set <Time(s)>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Enable the Health Server attention state for some time.

	* ``Time``: Duration of the attention state, in seconds (``0`` to ``255``)


``mesh models health attention-set-unack <Time(s)>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Enable the Health Server attention state for some time without requesting a response.

	* ``Time``: Duration of the attention state, in seconds (``0`` to ``255``)


Binary Large Object (BLOB) Transfer Client model
------------------------------------------------

The :ref:`bluetooth_mesh_blob_cli` can be added to the mesh shell by enabling the :kconfig:option:`CONFIG_BT_MESH_BLOB_CLI` option, and disabling the :kconfig:option:`CONFIG_BT_MESH_DFU_CLI` option.

``mesh models blob cli target <Addr>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add a Target node for the next BLOB transfer.

	* ``Addr``: Unicast address of the Target node's BLOB Transfer Server model.


``mesh models blob cli bounds [<Group>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the total boundary parameters of all Target nodes.

	* ``Group``: Optional group address to use when communicating with Target nodes. If omitted, the BLOB Transfer Client will address each Target node individually.


``mesh models blob cli tx <Id> <Size> <BlockSizeLog> <ChunkSize> [<Group> [<Mode(push, pull)>]]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Perform a BLOB transfer to Target nodes. The BLOB Transfer Client will send a dummy BLOB to all Target nodes, then post a message when the transfer is completed. Note that all Target nodes must first be configured to receive the transfer using the ``mesh models blob srv rx`` command.

	* ``Id``: 64-bit BLOB transfer ID.
	* ``Size``: Size of the BLOB in bytes.
	* ``BlockSizeLog`` Logarithmic representation of the BLOB's block size. The final block size will be ``1 << block size log`` bytes.
	* ``ChunkSize``: Chunk size in bytes.
	* ``Group``: Optional group address to use when communicating with Target nodes. If omitted or set to 0, the BLOB Transfer Client will address each Target node individually.
	* ``Mode``: BLOB transfer mode to use. Must be either ``push`` (Push BLOB Transfer Mode) or ``pull`` (Pull BLOB Transfer Mode). If omitted, ``push`` will be used by default.


``mesh models blob cli tx-cancel``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Cancel an ongoing BLOB transfer.

``mesh models blob cli tx-get [Group]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Determine the progress of a previously running BLOB transfer. Can be used when not performing a BLOB transfer.

	* ``Group``: Optional group address to use when communicating with Target nodes. If omitted or set to 0, the BLOB Transfer Client will address each Target node individually.


``mesh models blob cli tx-suspend``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Suspend the ongoing BLOB transfer.


``mesh models blob cli tx-resume``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Resume the suspended BLOB transfer.

``mesh models blob cli instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the BLOB Transfer Client model instance on the specified element when using the other BLOB Transfer Client model commands.

	* ``ElemIdx``: The element on which to find the BLOB Transfer Client model instance to use.

``mesh models blob cli instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all BLOB Transfer Client model instances on the node.


BLOB Transfer Server model
--------------------------

The :ref:`bluetooth_mesh_blob_srv` can be added to the mesh shell by enabling the :kconfig:option:`CONFIG_BT_MESH_BLOB_SRV` option. The BLOB Transfer Server model is capable of receiving any BLOB data, but the implementation in the mesh shell will discard the incoming data.


``mesh models blob srv rx <ID> [<TimeoutBase(10s steps)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Prepare to receive a BLOB transfer.

	* ``ID``: 64-bit BLOB transfer ID to receive.
	* ``TimeoutBase``: Optional additional time to wait for client messages, in 10-second increments.


``mesh models blob srv rx-cancel``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Cancel an ongoing BLOB transfer.

``mesh models blob srv instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the BLOB Transfer Server model instance on the specified element when using the other BLOB Transfer Server model commands.

	* ``ElemIdx``: The element on which to find the BLOB Transfer Server model instance to use.

``mesh models blob srv instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all BLOB Transfer Server model instances on the node.


Firmware Update Client model
----------------------------

The Firmware Update Client model can be added to the mesh shell by enabling configuration options :kconfig:option:`CONFIG_BT_MESH_BLOB_CLI` and :kconfig:option:`CONFIG_BT_MESH_DFU_CLI`. The Firmware Update Client demonstrates the firmware update Distributor role by transferring a dummy firmware update to a set of Target nodes.


``mesh models dfu slot add <Size> [<FwID> [<Metadata> [<URI>]]]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add a virtual DFU image slot that can be transferred as a DFU image. The image slot will be assigned an image slot index, which is printed as a response, and can be used to reference the slot in other commands. To update the image slot, remove it using the ``mesh models dfu slot del`` shell command and then add it again.

	* ``Size``: DFU image slot size in bytes.
	* ``FwID``: Optional firmware ID, formatted as a hexstring.
	* ``Metadata``: Optional firmware metadata, formatted as a hexstring.
	* ``URI``: Optional URI for the firmware.


``mesh models dfu slot del <SlotIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete the DFU image slot at the given index.

	* ``SlotIdx``: Index of the slot to delete.


``mesh models dfu slot get <SlotIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get all available information about a DFU image slot.

	* ``SlotIdx``: Index of the slot to get.


``mesh models dfu cli target <Addr> <ImgIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add a Target node.

	* ``Addr``: Unicast address of the Target node.
	* ``ImgIdx``: Image index to address on the Target node.


``mesh models dfu cli target-state``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Check the DFU Target state of the device at the configured destination address.


``mesh models dfu cli target-imgs [<MaxCount>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of DFU images on the device at the configured destination address.

	* ``MaxCount``: Optional maximum number of images to return. If omitted, there's no limit on the number of returned images.


``mesh models dfu cli target-check <SlotIdx> <TargetImgIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Check whether the device at the configured destination address will accept a DFU transfer from the given DFU image slot to the Target node's DFU image at the given index, and what the effect would be.

	* ``SlotIdx``: Index of the local DFU image slot to check.
	* ``TargetImgIdx``: Index of the Target node's DFU image to check.


``mesh models dfu cli send <SlotIdx> [<Group>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start a DFU transfer to all added Target nodes.

	* ``SlotIdx``: Index of the local DFU image slot to send.
	* ``Group``: Optional group address to use when communicating with the Target nodes. If omitted, the Firmware Update Client will address each Target node individually.


``mesh models dfu cli apply``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Apply the most recent DFU transfer on all Target nodes. Can only be called after a DFU transfer is completed.


``mesh models dfu cli confirm``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Confirm that the most recent DFU transfer was successfully applied on all Target nodes. Can only be called after a DFU transfer is completed and applied.


``mesh models dfu cli progress``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Check the progress of the current transfer.


``mesh models dfu cli suspend``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Suspend the ongoing DFU transfer.


``mesh models dfu cli resume``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Resume the suspended DFU transfer.

``mesh models dfu srv progress``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Check the progress of the current transfer.

``mesh models dfu cli instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the Firmware Update Client model instance on the specified element when using the other Firmware Update Client model commands.

	* ``ElemIdx``: The element on which to find the Firmware Update Client model instance to use.

``mesh models dfu cli instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all Firmware Update Client model instances on the node.


Firmware Update Server model
----------------------------

The Firmware Update Server model can be added to the mesh shell by enabling configuration options :kconfig:option:`CONFIG_BT_MESH_BLOB_SRV` and :kconfig:option:`CONFIG_BT_MESH_DFU_SRV`. The Firmware Update Server demonstrates the firmware update Target role by accepting any firmware update. The mesh shell Firmware Update Server will discard the incoming firmware data, but otherwise behave as a proper firmware update Target node.


``mesh models dfu srv applied``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Mark the most recent DFU transfer as applied. Can only be called after a DFU transfer is completed, and the Distributor has requested that the transfer is applied.

	As the mesh shell Firmware Update Server doesn't actually apply the incoming firmware image, this command can be used to emulate an applied status, to notify the Distributor that the transfer was successful.


``mesh models dfu srv progress``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Check the progress of the current transfer.

``mesh models dfu srv rx-cancel``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Cancel incoming DFU transfer.

``mesh models dfu srv instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the Firmware Update Server model instance on the specified element when using the other Firmware Update Server model commands.

	* ``ElemIdx``: The element on which to find the Firmware Update Server model instance to use.

``mesh models dfu srv instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all Firmware Update Server model instances on the node.


.. _bluetooth_mesh_shell_dfd_server:

Firmware Distribution Server model
----------------------------------

The Firmware Distribution Server model commands can be added to the mesh shell by enabling the :kconfig:option:`CONFIG_BT_MESH_DFD_SRV` configuration option.
The shell commands for this model mirror the messages sent to the server by a Firmware Distribution Client model.
To use these commands, a Firmware Distribution Server must be instantiated by the application.

``mesh models dfd receivers-add <Addr>,<FwIdx>[;<Addr>,<FwIdx>]...``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Add receivers to the Firmware Distribution Server.
	Supply receivers as a list of comma-separated addr,fw_idx pairs, separated by semicolons, for example, ``0x0001,0;0x0002,0;0x0004,1``.
	Do not use spaces in the receiver list.
	Repeated calls to this command will continue populating the receivers list until ``mesh models dfd receivers-delete-all`` is called.

	* ``Addr``: Address of the receiving node(s).
	* ``FwIdx``: Index of the firmware slot to send to ``Addr``.

``mesh models dfd receivers-delete-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete all receivers from the server.

``mesh models dfd receivers-get <First> <Count>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of info about firmware receivers.

	* ``First``: Index of the first receiver to get from the receiver list.
	* ``Count``: The number of recievers for which to get info.

``mesh models dfd capabilities-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the capabilities of the server.

``mesh models dfd get``
^^^^^^^^^^^^^^^^^^^^^^^

	Get information about the current distribution state, phase and the transfer parameters.

``mesh models dfd start <AppKeyIdx> <SlotIdx> [<Group> [<PolicyApply> [<TTL> [<TimeoutBase> [<XferMode>]]]]]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start the firmware distribution.

	* ``AppKeyIdx``: Application index to use for sending. The common application key should be bound to the Firmware Update and BLOB Transfer models on the Distributor and Target nodes.
	* ``SlotIdx``: Index of the local image slot to send.
	* ``Group``: Optional group address to use when communicating with the Target nodes. If omitted, the Firmware Distribution Server will address each Target node individually. To keep addressing each Target node individually while changing other arguments, set this argument value to 0.
	* ``PolicyApply``: Optional field that corresponds to the update policy. Setting this to ``true`` will make the Firmware Distribution Server apply the image immediately after the transfer is completed.
	* ``TTL``: Optional. TTL value to use when sending. Defaults to configured default TTL.
	* ``TimeoutBase``: Optional additional value used to calculate timeout values in the firmware distribution process, in 10-second increments.. See :ref:`bluetooth_mesh_blob_timeout` for information about how ``timeout_base`` is used to calculate the transfer timeout. Defaults to 0.
	* ``XferMode``: Optional BLOB transfer mode. 1 = Push mode (Push BLOB Transfer Mode), 2 = Pull mode (Pull BLOB Transfer Mode). Defaults to Push mode.

``mesh models dfd suspend``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Suspends the ongoing distribution.

``mesh models dfd cancel``
^^^^^^^^^^^^^^^^^^^^^^^^^^

	Cancel the ongoing distribution.

``mesh models dfd apply``
^^^^^^^^^^^^^^^^^^^^^^^^^

	Apply the distributed firmware.

``mesh models dfd fw-get <FwID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get information about the firmware image uploaded to the server.

	* ``FwID``: Firmware ID of the image to get.

``mesh models dfd fw-get-by-idx <Idx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get information about the firmware image uploaded to the server in a specific slot.

	* ``Idx``: Index of the slot to get the image from.

``mesh models dfd fw-delete <FwID>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete a firmware image from the server.

	* ``FwID``: Firmware ID of the image to delete.

``mesh models dfd fw-delete-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Delete all firmware images from the server.

``mesh models dfd instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the Firmware Distribution Server model instance on the specified element when using the other Firmware Distribution Server model commands.

	* ``ElemIdx``: The element on which to find the Firmware Distribution Server model instance to use.

``mesh models dfd instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all Firmware Distribution Server model instances on the node.


.. _bluetooth_mesh_shell_dfu_metadata:

DFU metadata
------------

The DFU metadata commands allow generating metadata that can be used by a Target node to check the firmware before accepting it. The commands are enabled through the :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA` configuration option.

``mesh models dfu metadata comp-clear``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Clear the stored composition data to be used for the Target node.

``mesh models dfu metadata comp-add <CID> <ProductID> <VendorID> <Crpl> <Features>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Create a header of the Composition Data Page 0.

	* ``CID``: Company identifier assigned by Bluetooth SIG.
	* ``ProductID``: Vendor-assigned product identifier.
	* ``VendorID``: Vendor-assigned version identifier.
	* ``Crpl``: The size of the replay protection list.
	* ``Features``: Features supported by the node in bit field format:

		* ``0``: Relay.
		* ``1``: Proxy.
		* ``2``: Friend.
		* ``3``: Low Power.

``mesh models dfu metadata comp-elem-add <Loc> <NumS> <NumV> {<SigMID>|<VndCID> <VndMID>}...``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	Add element description of the Target node.

	* ``Loc``: Element location.
	* ``NumS``: Number of SIG models instantiated on the element.
	* ``NumV``: Number of vendor models instantiated on the element.
	* ``SigMID``: SIG Model ID.
	* ``VndCID``: Vendor model company identifier.
	* ``VndMID``: Vendor model identifier.

``mesh models dfu metadata comp-hash-get [<Key(16 hex)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Generate a hash of the stored Composition Data to be used in metadata.

	* ``Key``: Optional 128-bit key to be used to generate the hash. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.

``mesh models dfu metadata encode <Major> <Minor> <Rev> <BuildNum> <Size> <CoreType> <Hash> <Elems> [<UserData>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Encode metadata for the DFU.

	* ``Major``: Major version of the firmware.
	* ``Minor``: Minor version of the firmware.
	* ``Rev``: Revision number of the firmware.
	* ``BuildNum``: Build number.
	* ``Size``: Size of the signed bin file.
	* ``CoreType``: New firmware core type in bit field format:

		* ``0``: Application core.
		* ``1``: Network core.
		* ``2``: Applications specific BLOB.
	* ``Hash``: Hash of the composition data generated using ``mesh models dfu metadata comp-hash-get`` command.
	* ``Elems``: Number of elements on the new firmware.
	* ``UserData``: User data supplied with the metadata.


Segmentation and Reassembly (SAR) Configuration Client
------------------------------------------------------

The SAR Configuration client is an optional mesh model that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_SAR_CFG_CLI` configuration option. The SAR Configuration Client model is used to support the functionality of configuring the behavior of the lower transport layer of a node that supports the SAR Configuration Server model.


``mesh models sar tx-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^

	Send SAR Configuration Transmitter Get message.

``mesh models sar tx-set <SegIntStep> <UniRetransCnt> <UniRetransWithoutProgCnt> <UniRetransIntStep> <UniRetransIntInc> <MultiRetransCnt> <MultiRetransInt>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Send SAR Configuration Transmitter Set message.

	* ``SegIntStep``: SAR Segment Interval Step state.
	* ``UniRetransCnt``: SAR Unicast Retransmissions Count state.
	* ``UniRetransWithoutProgCnt``: SAR Unicast Retransmissions Without Progress Count state.
	* ``UniRetransIntStep``: SAR Unicast Retransmissions Interval Step state.
	* ``UniRetransIntInc``: SAR Unicast Retransmissions Interval Increment state.
	* ``MultiRetransCnt``: SAR Multicast Retransmissions Count state.
	* ``MultiRetransInt``: SAR Multicast Retransmissions Interval state.

``mesh models sar rx-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^

	Send SAR Configuration Receiver Get message.

``mesh models sar rx-set <SegThresh> <AckDelayInc> <DiscardTimeout> <RxSegIntStep> <AckRetransCount>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Send SAR Configuration Receiver Set message.

	* ``SegThresh``: SAR Segments Threshold state.
	* ``AckDelayInc``: SAR Acknowledgment Delay Increment state.
	* ``DiscardTimeout``: SAR Discard Timeout state.
	* ``RxSegIntStep``: SAR Receiver Segment Interval Step state.
	* ``AckRetransCount``: SAR Acknowledgment Retransmissions Count state.


Private Beacon Client
---------------------

The Private Beacon Client model is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_PRIV_BEACON_CLI` configuration option.

``mesh models prb priv-beacon-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the target's Private Beacon state. Possible values:

		* ``0x00``: The node doesn't broadcast Private beacons.
		* ``0x01``: The node broadcasts Private beacons.

``mesh models prb priv-beacon-set <Val(off, on)> <RandInt(10s steps)>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the target's Private Beacon state.

	* ``Val``: Control Private Beacon state.
	* ``RandInt``: Random refresh interval (in 10-second steps), or 0 to keep current value.

``mesh models prb priv-gatt-proxy-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the target's Private GATT Proxy state. Possible values:

		* ``0x00``: The Private Proxy functionality is supported, but disabled.
		* ``0x01``: The Private Proxy functionality is enabled.
		* ``0x02``: The Private Proxy functionality is not supported.

``mesh models prb priv-gatt-proxy-set <Val(off, on)>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the target's Private GATT Proxy state.

	* ``Val``: New Private GATT Proxy value:

		* ``0x00``: Disable the Private Proxy functionality.
		* ``0x01``: Enable the Private Proxy functionality.

``mesh models prb priv-node-id-get <NetKeyIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the target's Private Node Identity state. Possible values:

		* ``0x00``: The node does not adverstise with the Private Node Identity.
		* ``0x01``: The node advertises with the Private Node Identity.
		* ``0x02``: The node doesn't support advertising with the Private Node Identity.

	* ``NetKeyIdx``: Network index to get the Private Node Identity state of.

``mesh models prb priv-node-id-set <NetKeyIdx> <State>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Set the target's Private Node Identity state.

	* ``NetKeyIdx``: Network index to set the Private Node Identity state of.
	* ``State``: New Private Node Identity value:

		* ``0x00``: Stop advertising with the Private Node Identity.
		* ``0x01``: Start advertising with the Private Node Identity.


Opcodes Aggregator Client
-------------------------

The Opcodes Aggregator client is an optional Bluetooth mesh model that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_OP_AGG_CLI` configuration option. The Opcodes Aggregator Client model is used to support the functionality of dispatching a sequence of access layer messages to nodes supporting the Opcodes Aggregator Server model.

``mesh models opagg seq-start <ElemAddr>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start the Opcodes Aggregator Sequence message. This command initiates the context for aggregating messages and sets the destination address for next shell commands to ``elem_addr``.

	* ``ElemAddr``: Element address that will process the aggregated opcodes.

``mesh models opagg seq-send``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Send the Opcodes Aggregator Sequence message. This command completes the procedure, sends the aggregated sequence message to the target node and clears the context.

``mesh models opagg seq-abort``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Abort the Opcodes Aggregator Sequence message. This command clears the Opcodes Aggregator Client context.


Remote Provisioning Client
--------------------------

The Remote Provisioning Client is an optional Bluetooth mesh model enabled through the :kconfig:option:`CONFIG_BT_MESH_RPR_CLI` configuration option. The Remote Provisioning Client model provides support for remote provisioning of devices into a mesh network by using the Remote Provisioning Server model.

This shell module can be used to trigger interaction between Remote Provisioning Clients and Remote Provisioning Servers on devices in a mesh network.

``mesh models rpr scan <Timeout(s)> [<UUID(1-16 hex)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start scanning for unprovisioned devices.

	* ``Timeout``: Scan timeout in seconds. Must be at least 1 second.
	* ``UUID``: Device UUID to scan for. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, all devices will be reported.

``mesh models rpr scan-ext <Timeout(s)> <UUID(1-16 hex)> [<ADType> ... ]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start the extended scanning for unprovisioned devices.

	* ``Timeout``: Scan timeout in seconds. Valid values from :c:macro:`BT_MESH_RPR_EXT_SCAN_TIME_MIN` to :c:macro:`BT_MESH_RPR_EXT_SCAN_TIME_MAX`.
	* ``UUID``: Device UUID to start extended scanning for. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``ADType``: List of AD types to include in the scan report. Must contain 1 to :kconfig:option:`CONFIG_BT_MESH_RPR_AD_TYPES_MAX` entries.

``mesh models rpr scan-srv [<ADType> ... ]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Start the extended scanning for the Remote Provisioning Server.

	* ``ADType``: List of AD types to include in the scan report. Must contain 1 to :kconfig:option:`CONFIG_BT_MESH_RPR_AD_TYPES_MAX` entries.

``mesh models rpr scan-caps``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the scanning capabilities of the Remote Provisioning Server.

``mesh models rpr scan-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the current scanning state of the Remote Provisioning Server.

``mesh models rpr scan-stop``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Stop any ongoing scanning on the Remote Provisioning Server.

``mesh models rpr link-get``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get the current link status of the Remote Provisioning Server.

``mesh models rpr link-close``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Close any open links on the Remote Provisioning Server.

``mesh models rpr provision-remote <UUID(1-16 hex)> <NetKeyIdx> <Addr>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Provision a mesh node using the PB-Remote provisioning bearer.

	* ``UUID``: UUID of the unprovisioned node. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``NetKeyIdx``: Network Key Index to give to the unprovisioned node.
	* ``Addr``: Address to assign to remote device. If ``addr`` is 0, the lowest available address will be chosen.

``mesh models rpr reprovision-remote <Addr> [<CompChanged(false, true)>]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Reprovision a mesh node using the PB-Remote provisioning bearer.

	* ``Addr``: Address to assign to remote device. If ``addr`` is 0, the lowest available address will be chosen.
	* ``CompChanged``: The Target node has indicated that its Composition Data has changed. Defaults to false.

``mesh models rpr instance-set <ElemIdx>``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Use the Remote Provisioning Client model instance on the specified element when using the other Remote Provisioning Client model commands.

	* ``ElemIdx``: The element on which to find the Remote Provisioning Client model instance to use.

``mesh models rpr instance-get-all``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	Get a list of all Remote Provisioning Client model instances on the node.


Configuration database
======================

The Configuration database is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_CDB` configuration option. The Configuration database is only available on provisioner devices, and allows them to store all information about the mesh network. To avoid conflicts, there should only be one mesh node in the network with the Configuration database enabled. This node is the Configurator, and is responsible for adding new nodes to the network and configuring them.

``mesh cdb create [NetKey(1-16 hex)]``
--------------------------------------

	Create a Configuration database.

	* ``NetKey``: Optional network key value of the primary network key (NetKeyIndex=0). Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. Defaults to the default key value if omitted.


``mesh cdb clear``
------------------

	Clear all data from the Configuration database.


``mesh cdb show``
-----------------

	Show all data in the Configuration database.


``mesh cdb node-add <UUID(1-16 hex)> <Addr> <ElemCnt> <NetKeyIdx> [DevKey(1-16 hex)]``
--------------------------------------------------------------------------------------

	Manually add a mesh node to the configuration database. Note that devices provisioned with ``mesh provision`` and ``mesh provision-adv`` will be added automatically if the Configuration Database is enabled and created.

	* ``UUID``: 128-bit hexadecimal UUID of the node. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest.
	* ``Addr``: Unicast address of the node, or 0 to automatically choose the lowest available address.
	* ``ElemCnt``: Number of elements on the node.
	* ``NetKeyIdx``: The network key the node was provisioned with.
	* ``DevKey``: Optional 128-bit device key value for the device. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, a random value will be generated.


``mesh cdb node-del <Addr>``
----------------------------

	Delete a mesh node from the Configuration database. If possible, the node should be reset with ``mesh reset`` before it is deleted from the Configuration database, to avoid unexpected behavior and uncontrolled access to the network.

	* ``Addr`` Address of the node to delete.


``mesh cdb subnet-add <NetKeyIdx> [<NetKey(1-16 hex)>]``
--------------------------------------------------------

	Add a network key to the Configuration database. The network key can later be passed to mesh nodes in the network. Note that adding a key to the Configuration database does not automatically add it to the local node's list of known network keys.

	* ``NetKeyIdx``: Key index of the network key to add.
	* ``NetKey``: Optional 128-bit network key value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, a random value will be generated.


``mesh cdb subnet-del <NetKeyIdx>``
-----------------------------------

	Delete a network key from the Configuration database.

	* ``NetKeyIdx``: Key index of the network key to delete.


``mesh cdb app-key-add <NetKeyIdx> <AppKeyIdx> [<AppKey(1-16 hex)>]``
---------------------------------------------------------------------

	Add an application key to the Configuration database. The application key can later be passed to mesh nodes in the network. Note that adding a key to the Configuration database does not automatically add it to the local node's list of known application keys.

	* ``NetKeyIdx``: Network key index the application key is bound to.
	* ``AppKeyIdx``: Key index of the application key to add.
	* ``AppKey``: Optional 128-bit application key value. Providing a hex-string shorter than 16 bytes will populate the N most significant bytes of the array and zero-pad the rest. If omitted, a random value will be generated.


``mesh cdb app-key-del <AppKeyIdx>``
------------------------------------

	Delete an application key from the Configuration database.

	* ``AppKeyIdx``: Key index of the application key to delete.


On-Demand Private GATT Proxy Client
-----------------------------------

The On-Demand Private GATT Proxy Client model is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_OD_PRIV_PROXY_CLI` configuration option.

``mesh models od_priv_proxy od-priv-gatt-proxy [Dur(s)]``
---------------------------------------------------------

	Set the On-Demand Private GATT Proxy state on active target, or fetch the value of this state from it.

	* ``Dur``: If given, set the state of On-Demand Private GATT Proxy to this value in seconds. Fetch this value otherwise.


Solicitation PDU RPL Client
---------------------------

The Solicitation PDU RPL Client model is an optional mesh subsystem that can be enabled through the :kconfig:option:`CONFIG_BT_MESH_SOL_PDU_RPL_CLI` configuration option.

``mesh models sol_pdu_rpl sol-pdu-rpl-clear <RngStart> <Ackd> [RngLen]``
------------------------------------------------------------------------

	Clear active target's solicitation replay protection list (SRPL) in given range of solicitation source (SSRC) addresses.

	* ``RngStart``: Start address of the SSRC range.
	* ``Ackd``: This argument decides on whether an acknowledged or unacknowledged message will be sent.
	* ``RngLen``: Range length for the SSRC addresses to be cleared from the solicitiation RPL list. This parameter is optional; if absent, only a single SSRC address will be cleared.
