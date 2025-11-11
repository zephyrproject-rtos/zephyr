.. _arm_scmi:

ARM System Control and Management Interface
###########################################

Overview
********

What is SCMI?
*************

System Control and Management Interface (SCMI) is a specification developed by
ARM, which describes a set of OS-agnostic software interfaces used to perform
system management (e.g: clock control, pinctrl, etc...).


Agent, platform, protocol and transport
***************************************

The SCMI specification defines **four** key terms, which will also be used throughout
this documentation:

	#. Agent
		Entity that performs SCMI requests (e.g: gating a clock or configuring
		a pin). In this context, Zephyr itself is an agent.
	#. Platform
		This refers to a set of hardware components that handle the requests from
		agents and provide the necessary functionality. In some cases, the requests
		are handled by a firmware, running on a core dedicated to performing system
		management tasks.
	#. Protocol
		A protocol is a set of messages grouped by functionality. Intuitively, a message
		can be thought of as a remote procedure call.

		The SCMI specification defines ten standard protocols:

		#. **Base** (0x10)
		#. **Power domain management** (0x11)
		#. **System power management** (0x12)
		#. **Performance domain management** (0x13)
		#. **Clock management** (0x14)
		#. **Sensor management** (0x15)
		#. **Reset domain management** (0x16)
		#. **Voltage domain management** (0x17)
		#. **Power capping and monitoring** (0x18)
		#. **Pin Control** (0x19)

		where each of these protocols is identified by an unique protocol ID
		(listed between brackets).

		Apart from the standard protocols, the SCMI specification reserves the
		**0x80-0xFF** protocol ID range for vendor-specific protocols.


	#. Transport
		This describes how messages are exchanged between agents and the platform.
		The communication itself happens through channels.

.. note::
	A system may have more than one agent.

Channels
********

A **channel** is the medium through which agents and the platform exchange messages.
The structure of a channel and the way it works is solely dependent on the transport.

Each agent has its own distinct set of channels, meaning some channel A cannot be used
by two different agents for example.

Channels are **bidirectional** (exception: FastChannels), and, depending on which entity
initiates the communication, can be one of **two** types:

	#. A2P (agent to platform)
		The agent is the initiator/requester. The messages passed through these
		channels are known as **commands**.
	#. P2A (platform to agent)
		The platform is the initiator/requester.

Messages
********

The SCMI specification defines **four** types of messages:

	#. Synchronous
		These are commands that block until the platform has completed the
		requested work and are sent over A2P channels.
	#. Asynchronous
		For these commands, the platform schedules the requested work to
		be performed at a later time. As such, they return almost immediately.
		These commands are sent over A2P channels.
	#. Delayed response
		These messages indicate the completion of the work associated
		with an asynchronous command. These are sent over P2A channels.

	#. Notification
		These messages are used to notify agents of events that take place on
		the platform. These are sent over P2A channels.

The Zephyr support for SCMI is based on the documentation provided by ARM:
`DEN0056E <https://developer.arm.com/documentation/den0056/latest/>`_. For more details
on the specification, the readers are encouraged to have a look at it.

SCMI support in Zephyr
**********************

Shared memory and doorbell-based transport
******************************************

This form of transport uses shared memory for reading/writing messages
and doorbells for signaling. The interaction with the shared
memory area is performed using a driver (:file:`drivers/firmware/scmi/shmem.c`),
which offers a set of functions for this exact purpose. Furthermore,
signaling is performed using the Zephyr MBOX API (signaling mode only, no message passing).

Interacting with the shared memory area and signaling are abstracted by the
transport API, which is implemented by the shared memory and doorbell-based
transport driver (:file:`drivers/firmware/scmi/mailbox.c`).

The steps below exemplify how the communication between the Zephyr agent
and the platform may happen using this transport:

	#. Write message to the shared memory area.
	#. Zephyr rings request doorbell. If in ``PRE_KERNEL_1`` or ``PRE_KERNEL_2`` phase start polling for reply, otherwise wait for reply doorbell ring.
	#. Platform reads message from shared memory area, processes it, writes the reply back to the same area and rings the reply doorbell.
	#. Zephyr reads reply from the shared memory area.

In the context of this transport, a channel is comprised of a **single** shared
memory area and one or more mailbox channels. This is because users may need/want
to use different mailbox channels for the request/reply doorbells.


Protocols
*********

Currently, Zephyr has support for the following standard protocols:

	#. **Power domain management**
	#. **Clock management**
	#. **Pin Control**

NXP-specific protocols:
	#. **CPU domain management**

Power domain management
***********************

This protocol is intended for management of power states of power domains.
This is done via a set of functions implementing various commands, for
example, ``POWER_STATE_GET`` and ``POWER_STATE_SET``.

.. note::
	This driver is vendor-agnostic. As such, it may be used on any
	system that uses SCMI for power domain management operations.

Clock management protocol
*************************

This protocol is used to perform clock management operations. This is done
via a driver (:file:`drivers/clock_control/clock_control_arm_scmi.c`), which
implements the Zephyr clock control subsystem API. As such, from the user's
perspective, using this driver is no different than using any other clock
management driver.

.. note::
	This driver is vendor-agnostic. As such, it may be used on any
	system that uses SCMI for clock management operations.

Pin Control protocol
********************

This protocol is used to perform pin configuration operations. This is done
via a set of functions implementing various commands. Currently, the only
supported command is ``PINCTRL_SETTINGS_CONFIGURE``.

.. note::
	The support for this protocol **does not** include a definition for
	the :code:`pinctrl_configure_pins` function. Each vendor should use
	their own definition of :code:`pinctrl_configure_pins`, which should
	call into the SCMI pin control protocol function implementing the
	``PINCTRL_SETTINGS_CONFIGURE`` command.

NXP - CPU domain management
***************************

This protocol is intended for management of cpu states.
This is done via a set of functions implementing various commands, for
example, ``CPU_SLEEP_MODE_SET``.

.. note::
	This driver is NXP-specific. As such, it may only be used on NXP
	system that uses SCMI for cpu domain management operations.

Enabling the SCMI support
*************************

To use the SCMI support, each vendor is required to add an ``scmi`` DT
node (used for transport driver binding) and a ``protocol`` node under the ``scmi``
node for each supported protocol.

.. note::
	Zephyr has no support for protocol discovery. As such, if users
	add a DT node for a certain protocol it's assumed the platform
	supports said protocol.

The example below shows how a DT may be configured in order to use the
SCMI support. It's assumed that the only protocol required is the clock
management protocol.

.. code-block:: devicetree

	#include <mem.h>

	#define MY_CLOCK_CONSUMER_CLK_ID 123

	scmi_res0: memory@cafebabe {
		/* mandatory to use shared memory driver */
		compatible = "arm,scmi-shmem";
		reg = <0xcafebabe DT_SIZE_K(1)>;
	};

	scmi {
		/* compatible for shared memory and doorbell-based transport */
		compatible = "arm,scmi";

		/* one SCMI channel => A2P/transmit channel */
		shmem = <&scmi_res0>;

		/* two mailbox channels */
		mboxes = <&my_mbox_ip 0>, <&my_mbox_ip 1>;
		mbox-names = "tx", "tx_reply";

		scmi_clk: protocol@14 {
			compatible = "arm,scmi-clock";

			/* matches the clock management protocol ID */
			reg = <0x14>;

			/* vendor-agnostic - always 1 */
			#clock-cells = <1>;
		};
	};

	my_mbox_ip: mailbox@deadbeef {
		compatible = "vnd,mbox-ip";
		reg = <0xdeadbeef DT_SIZE_K(1)>;
		#mbox-cells = <1>;
	};

	my_clock_consumer_ip: serial@12345678 {
		compatible = "vnd,consumer-ip";
		reg = <0x12345678 DT_SIZE_K(1)>;
		/* clock ID is vendor specific */
		clocks = <&scmi_clk MY_CLOCK_CONSUMER_CLK_ID>;
	};


Finally, all that's left to do is enable :kconfig:option:`CONFIG_ARM_SCMI`.
