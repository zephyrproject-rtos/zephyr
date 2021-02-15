.. _lwm2m_interface:

Lightweight M2M (LWM2M)
#######################

.. contents::
    :local:
    :depth: 2

Overview
********

Lightweight Machine to Machine (LwM2M) is an application layer protocol
designed with device management, data reporting and device actuation in mind.
Based on CoAP/UDP, `LwM2M`_ is a
`standard <http://openmobilealliance.org/release/LightweightM2M/>`_ defined by
the Open Mobile Alliance and suitable for constrained devices by its use of
CoAP packet-size optimization and a simple, stateless flow that supports a
REST API.

One of the key differences between LwM2M and CoAP is that an LwM2M client
initiates the connection to an LwM2M server.  The server can then use the
REST API to manage various interfaces with the client.

LwM2M uses a simple resource model with the core set of objects and resources
defined in the specification.

Example LwM2M object and resources: Device
******************************************

*Object definition*

.. list-table::
   :header-rows: 1

   * - Object ID
     - Name
     - Instance
     - Mandatory

   * - 3
     - Device
     - Single
     - Mandatory

*Resource definitions*

``* R=Read, W=Write, E=Execute``

.. list-table::
   :header-rows: 1

   * - ID
     - Name
     - OP\*
     - Instance
     - Mandatory
     - Type

   * - 0
     - Manufacturer
     - R
     - Single
     - Optional
     - String

   * - 1
     - Model
     - R
     - Single
     - Optional
     - String

   * - 2
     - Serial number
     - R
     - Single
     - Optional
     - String

   * - 3
     - Firmware version
     - R
     - Single
     - Optional
     - String

   * - 4
     - Reboot
     - E
     - Single
     - Mandatory
     -

   * - 5
     - Factory Reset
     - E
     - Single
     - Optional
     -

   * - 6
     - Available Power Sources
     - R
     - Multiple
     - Optional
     - Integer 0-7

   * - 7
     - Power Source Voltage (mV)
     - R
     - Multiple
     - Optional
     - Integer

   * - 8
     - Power Source Current (mA)
     - R
     - Multiple
     - Optional
     - Integer

   * - 9
     - Battery Level %
     - R
     - Single
     - Optional
     - Integer

   * - 10
     - Memory Free (Kb)
     - R
     - Single
     - Optional
     - Integer

   * - 11
     - Error Code
     - R
     - Multiple
     - Optional
     - Integer 0-8

   * - 12
     - Reset Error
     - E
     - Single
     - Optional
     -

   * - 13
     - Current Time
     - RW
     - Single
     - Optional
     - Time

   * - 14
     - UTC Offset
     - RW
     - Single
     - Optional
     - String

   * - 15
     - Timezone
     - RW
     - Single
     - Optional
     - String

   * - 16
     - Supported Binding
     - R
     - Single
     - Mandatory
     - String

   * - 17
     - Device Type
     - R
     - Single
     - Optional
     - String

   * - 18
     - Hardware Version
     - R
     - Single
     - Optional
     - String

   * - 19
     - Software Version
     - R
     - Single
     - Optional
     - String

   * - 20
     - Battery Status
     - R
     - Single
     - Optional
     - Integer 0-6

   * - 21
     - Memory Total (Kb)
     - R
     - Single
     - Optional
     - Integer

   * - 22
     - ExtDevInfo
     - R
     - Multiple
     - Optional
     - ObjLnk

The server could query the ``Manufacturer`` resource for ``Device`` object
instance 0 (the default and only instance) by sending a ``READ 3/0/0``
operation to the client.

The full list of registered objects and resource IDs can be found in the
`LwM2M registry`_.

Zephyr's LwM2M library lives in the :zephyr_file:`subsys/net/lib/lwm2m`, with a
client sample in :zephyr_file:`samples/net/lwm2m_client`.  For more information
about the provided sample see: :ref:`lwm2m-client-sample`  The sample can be
configured to use normal unsecure network sockets or sockets secured via DTLS.

The Zephyr LwM2M library implements the following items:

* engine to process networking events and core functions
* RD client which performs BOOTSTRAP and REGISTRATION functions
* TLV, JSON, and plain text formatting functions
* LwM2M Technical Specification Enabler objects such as Security, Server,
  Device, Firmware Update, etc.
* Extended IPSO objects such as Light Control, Temperature Sensor, and Timer

The library currently implements up to `LwM2M specification 1.0.2`_.

For more information about LwM2M visit `OMA Specworks LwM2M`_.

Sample usage
************

To use the LwM2M library, start by creating an LwM2M client context
:c:struct:`lwm2m_ctx` structure:

.. code-block:: c

	/* LwM2M client context */
	static struct lwm2m_ctx client;

Create callback functions for LwM2M resource exuctions:

.. code-block:: c

	static int device_reboot_cb(uint16_t obj_inst_id, uint8_t *args,
				    uint16_t args_len)
	{
		LOG_INF("Device rebooting.");
		LOG_PANIC();
		sys_reboot(0);
		return 0; /* wont reach this */
	}

The LwM2M RD client can send events back to the sample.  To receive those
events, setup a callback function:

.. code-block:: c

	static void rd_client_event(struct lwm2m_ctx *client,
				    enum lwm2m_rd_client_event client_event)
	{
		switch (client_event) {

		case LWM2M_RD_CLIENT_EVENT_NONE:
			/* do nothing */
			break;

		case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
			LOG_DBG("Bootstrap registration failure!");
			break;

		case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
			LOG_DBG("Bootstrap registration complete");
			break;

		case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
			LOG_DBG("Bootstrap transfer complete");
			break;

		case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
			LOG_DBG("Registration failure!");
			break;

		case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
			LOG_DBG("Registration complete");
			break;

		case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
			LOG_DBG("Registration update failure!");
			break;

		case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
			LOG_DBG("Registration update complete");
			break;

		case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
			LOG_DBG("Deregister failure!");
			break;

		case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
			LOG_DBG("Disconnected");
			break;

		}
	}

Next we assign ``Security`` resource values to let the client know where and how
to connect as well as set the ``Manufacturer`` and ``Reboot`` resources in the
``Device`` object with some data and the callback we defined above:

.. code-block:: c

	/*
	 * Server URL of default Security object = 0/0/0
	 * Use leshan.eclipse.org server IP (5.39.83.206) for connection
	 */
	lwm2m_engine_set_string("0/0/0", "coap://5.39.83.206");

	/*
	 * Security Mode of default Security object = 0/0/2
	 * 3 = NoSec mode (no security beware!)
	 */
	lwm2m_engine_set_u8("0/0/2", 3);

	#define CLIENT_MANUFACTURER "Zephyr Manufacturer"

	/*
	 * Manufacturer resource of Device object = 3/0/0
	 * We use lwm2m_engine_set_res_data() function to set a pointer to the
	 * CLIENT_MANUFACTURER string.
	 * Note the LWM2M_RES_DATA_FLAG_RO flag which stops the engine from
	 * trying to assign a new value to the buffer.
	 */
	lwm2m_engine_set_res_data("3/0/0", CLIENT_MANUFACTURER,
				  sizeof(CLIENT_MANUFACTURER),
				  LWM2M_RES_DATA_FLAG_RO);

	/* Reboot resource of Device object = 3/0/4 */
	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);

Lastly, we start the LwM2M RD client (which in turn starts the LwM2M engine).
The second parameter of :c:func:`lwm2m_rd_client_start` is the client
endpoint name.  This is important as it needs to be unique per LwM2M server:

.. code-block:: c

	(void)memset(&client, 0x0, sizeof(client));
	lwm2m_rd_client_start(&client, "unique-endpoint-name", 0, rd_client_event);

Using LwM2M library with DTLS
*****************************

The Zephyr LwM2M library can be used with DTLS transport for secure
communication by selecting :option:`CONFIG_LWM2M_DTLS_SUPPORT`.  In the client
initialization we need to create a PSK and identity.  These need to match
the security information loaded onto the LwM2M server.  Normally, the
endpoint name is used to lookup the related security information:

.. code-block:: c

	/* "000102030405060708090a0b0c0d0e0f" */
	static unsigned char client_psk[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};

	static const char client_identity[] = "Client_identity";

Next we alter the ``Security`` object resources to include DTLS security
information.  The server URL should begin with ``coaps://`` to indicate security
is required.  Assign a 0 value (Pre-shared Key mode) to the ``Security Mode``
resource.  Lastly, set the client identity and PSK resources.

.. code-block:: c

	/* Use coaps:// for server URL protocol */
	lwm2m_engine_set_string("0/0/0", "coaps://5.39.83.206");
	/* 0 = Pre-Shared Key mode */
	lwm2m_engine_set_u8("0/0/2", 0);
	/* Set the client identity */
	lwm2m_engine_set_string("0/0/3", (char *)client_identity);
	/* Set the client pre-shared key (PSK) */
	lwm2m_engine_set_opaque("0/0/5", (void *)client_psk, sizeof(client_psk));

Before calling :c:func:`lwm2m_rd_client_start` assign the tls_tag # where the
LwM2M library should store the DTLS information prior to connection (normally a
value of 1 is ok here).

.. code-block:: c

	(void)memset(&client, 0x0, sizeof(client));
	client.tls_tag = 1; /* <---- */
	lwm2m_rd_client_start(&client, "endpoint-name", 0, rd_client_event);

For a more detailed LwM2M client sample see: :ref:`lwm2m-client-sample`.

.. _lwm2m_api_reference:

API Reference
*************

.. doxygengroup:: lwm2m_api
   :project: Zephyr

.. _LwM2M:
   https://www.omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/

.. _LwM2M registry:
   http://www.openmobilealliance.org/wp/OMNA/LwM2M/LwM2MRegistry.html

.. _OMA Specworks LwM2M:
   https://www.omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/

.. _LwM2M specification 1.0.2:
   http://openmobilealliance.org/release/LightweightM2M/V1_0_2-20180209-A/OMA-TS-LightweightM2M-V1_0_2-20180209-A.pdf
