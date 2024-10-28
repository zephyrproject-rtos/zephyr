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

The LwM2M library can be enabled with :kconfig:option:`CONFIG_LWM2M` Kconfig option.

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
about the provided sample see: :zephyr:code-sample:`lwm2m-client`. The sample can be
configured to use normal unsecure network sockets or sockets secured via DTLS.

The Zephyr LwM2M library implements the following items:

* engine to process networking events and core functions
* RD client which performs BOOTSTRAP and REGISTRATION functions
* SenML CBOR, SenML JSON, CBOR, TLV, JSON, and plain text formatting functions
* LwM2M Technical Specification Enabler objects such as Security, Server,
  Device, Firmware Update, etc.
* Extended IPSO objects such as Light Control, Temperature Sensor, and Timer

By default, the library implements `LwM2M specification 1.0.2`_ and can be set to
`LwM2M specification 1.1.1`_ with a Kconfig option.

For more information about LwM2M visit `OMA Specworks LwM2M`_.

Sample usage
************

To use the LwM2M library, start by creating an LwM2M client context
:c:struct:`lwm2m_ctx` structure:

.. code-block:: c

	/* LwM2M client context */
	static struct lwm2m_ctx client;

Create callback functions for LwM2M resource executions:

.. code-block:: c

	static int device_reboot_cb(uint16_t obj_inst_id, uint8_t *args,
				    uint16_t args_len)
	{
		LOG_INF("Device rebooting.");
		LOG_PANIC();
		sys_reboot(0);
		return 0; /* won't reach this */
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

		case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
			LOG_DBG("Registration timeout!");
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

		case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
			LOG_DBG("Registration update");
			break;

		case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
			LOG_DBG("Deregistration client");
			break;

		case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
			LOG_DBG("LwM2M server disabled");
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
	lwm2m_set_string(&LWM2M_OBJ(0, 0, 0), "coap://5.39.83.206");

	/*
	 * Security Mode of default Security object = 0/0/2
	 * 3 = NoSec mode (no security beware!)
	 */
	lwm2m_set_u8(&LWM2M_OBJ(0, 0, 2), 3);

	#define CLIENT_MANUFACTURER "Zephyr Manufacturer"

	/*
	 * Manufacturer resource of Device object = 3/0/0
	 * We use lwm2m_set_res_data() function to set a pointer to the
	 * CLIENT_MANUFACTURER string.
	 * Note the LWM2M_RES_DATA_FLAG_RO flag which stops the engine from
	 * trying to assign a new value to the buffer.
	 */
	lwm2m_set_res_data(&LWM2M_OBJ(3, 0, 0), CLIENT_MANUFACTURER,
			   sizeof(CLIENT_MANUFACTURER),
			   LWM2M_RES_DATA_FLAG_RO);

	/* Reboot resource of Device object = 3/0/4 */
	lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 4), device_reboot_cb);

Lastly, we start the LwM2M RD client (which in turn starts the LwM2M engine).
The second parameter of :c:func:`lwm2m_rd_client_start` is the client
endpoint name.  This is important as it needs to be unique per LwM2M server:

.. code-block:: c

	(void)memset(&client, 0x0, sizeof(client));
	lwm2m_rd_client_start(&client, "unique-endpoint-name", 0, rd_client_event);

.. _lwm2m_security:

LwM2M security modes
********************

The Zephyr LwM2M library can be used either without security or use DTLS to secure the communication channel.
When using DTLS with the LwM2M engine, PSK (Pre-Shared Key) and X.509 certificates are the security modes that can be used to secure the communication.
The engine uses LwM2M Security object (Id 0) to read the stored credentials and feed keys from the security object into
the TLS credential subsystem, see :ref:`secure sockets documentation <secure_sockets_interface>`.
Enable the :kconfig:option:`CONFIG_LWM2M_DTLS_SUPPORT` Kconfig option to use the security.

Depending on the selected mode, the security object must contain following data:

PSK
  Security Mode (Resource ID 2) set to zero (Pre-Shared Key mode).
  Identity (Resource ID 3) contains PSK ID in binary form.
  Secret key (Resource ID 5) contains the PSK key in binary form.
  If the key or identity is provided as a hex string, it must be converted to binary before storing into the security object.

X509
  When X509 certificates are used, set Security Mode (ID 2) to ``2`` (Certificate mode).
  Identity (ID 3) is used to store the client certificate and Secret key (ID 5) must have a private key associated with the certificate.
  Server Public Key resource (ID 4) must contain a server certificate or CA certificate used to sign the certificate chain.
  If the :kconfig:option:`CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT` Kconfig option is enabled, certificates and private key can be entered in PEM format.
  Otherwise, they must be in binary DER format.

NoSec
  When no security is used, set Security Mode (Resource ID 2) to ``3`` (NoSec).

In all modes, Server URI resource (ID 0) must contain the full URI for the target server.
When DNS names are used, the DNS resolver must be enabled.

When DTLS is used, following options are recommended to reduce DTLS handshake traffic when connection is re-established:

* :kconfig:option:`CONFIG_LWM2M_DTLS_CID` enables DTLS Connection Identifier support. When server supports it, this completely removes the handshake when device resumes operation after long idle period. Greatly helps when NAT mappings have timed out.
* :kconfig:option:`CONFIG_LWM2M_TLS_SESSION_CACHING` uses session cache when before falling back to full DTLS handshake. Reduces few packets from handshake, when session is still cached on server side. Most significant effect is to avoid full registration.

LwM2M stack provides callbacks in the :c:struct:`lwm2m_ctx` structure.
They are used to feed keys from the LwM2M security object into the TLS credential subsystem.
By default, these callbacks can be left as NULL pointers, in which case default callbacks are used.
When an external TLS stack, or non-default socket options are required, you can overwrite the :c:func:`lwm2m_ctx.load_credentials` or :c:func:`lwm2m_ctx.set_socketoptions` callbacks.

An example of setting up the security object for PSK mode:

.. code-block:: c

	/* "000102030405060708090a0b0c0d0e0f" */
	static unsigned char client_psk[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};

	static const char client_identity[] = "Client_identity";

	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 0), "coaps://lwm2m.example.com");
	lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 2), LWM2M_SECURITY_PSK);
	/* Set the client identity as a string, but this could be binary as well */
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 3), client_identity);
	/* Set the client pre-shared key (PSK) */
	lwm2m_set_opaque(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 5), client_psk, sizeof(client_psk));

An example of setting up the security object for X509 certificate mode:

.. code-block:: c

	static const char certificate[] = "-----BEGIN CERTIFICATE-----\nMIIB6jCCAY+gAw...";
	static const char key[] = "-----BEGIN EC PRIVATE KEY-----\nMHcCAQ...";
	static const char root_ca[] = "-----BEGIN CERTIFICATE-----\nMIIBaz...";

	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 0), "coaps://lwm2m.example.com");
	lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 2), LWM2M_SECURITY_CERT);
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 3), certificate);
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 5), key);
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, 4), root_ca);

Before calling :c:func:`lwm2m_rd_client_start` assign the tls_tag # where the
LwM2M library should store the DTLS information prior to connection (normally a
value of 1 is ok here).

.. code-block:: c

	(void)memset(&client, 0x0, sizeof(client));
	client.tls_tag = 1; /* <---- */
	lwm2m_rd_client_start(&client, "endpoint-name", 0, rd_client_event);

For a more detailed LwM2M client sample see: :zephyr:code-sample:`lwm2m-client`.

Multi-thread usage
******************
Writing a value to a resource can be done using functions like lwm2m_set_u8. When writing
to multiple resources, the function lwm2m_registry_lock will ensure that the
client halts until all writing operations are finished:

.. code-block:: c

  lwm2m_registry_lock();
  lwm2m_set_u32(&LWM2M_OBJ(1, 0, 1), 60);
  lwm2m_set_u8(&LWM2M_OBJ(5, 0, 3), 0);
  lwm2m_set_f64(&LWM2M_OBJ(3303, 0, 5700), value);
  lwm2m_registry_unlock();

This is especially useful if the server is composite-observing the resources being
written to. Locking will then ensure that the client only updates and sends notifications
to the server after all operations are done, resulting in fewer messages in general.

Support for time series data
****************************

LwM2M version 1.1 adds support for SenML CBOR and SenML JSON data formats. These data formats add
support for time series data. Time series formats can be used for READ, NOTIFY and SEND operations.
When data cache is enabled for a resource, each write will create a timestamped entry in a cache,
and its content is then returned as a content in READ, NOTIFY or SEND operation for a given
resource.

Data cache is only supported for resources with a fixed data size.

Supported resource types:

* Signed and unsigned 8-64-bit integers
* Float
* Boolean

Enabling and configuring
========================

Enable data cache by selecting :kconfig:option:`CONFIG_LWM2M_RESOURCE_DATA_CACHE_SUPPORT`.
Application needs to allocate an array of :c:struct:`lwm2m_time_series_elem` structures and then
enable the cache by calling :c:func:`lwm2m_engine_enable_cache` for a given resource. Each resource
must be enabled separately and each resource needs their own storage.

.. code-block:: c

  /* Allocate data cache storage */
  static struct lwm2m_time_series_elem temperature_cache[10];
  /* Enable data cache */
  lwm2m_engine_enable_cache(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
          temperature_cache, ARRAY_SIZE(temperature_cache));

LwM2M engine have room for four resources that have cache enabled. Limit can be increased by
changing :kconfig:option:`CONFIG_LWM2M_MAX_CACHED_RESOURCES`. This affects a static memory usage of
engine.

Data caches depends on one of the SenML data formats
:kconfig:option:`CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT` or
:kconfig:option:`CONFIG_LWM2M_RW_SENML_JSON_SUPPORT` and needs :kconfig:option:`CONFIG_POSIX_TIMERS`
so it can request a timestamp from the system and :kconfig:option:`CONFIG_RING_BUFFER` for ring
buffer.

Read and Write operations
=========================

Full content of data cache is written into a payload when any READ, SEND or NOTIFY operation
internally reads the content of a given resource. This has a side effect that any read callbacks
registered for a that resource are ignored when cache is enabled.
Data is written into a cache when any of the ``lwm2m_set_*`` functions are called. To filter
the data entering the cache, application may register a validation callback using
:c:func:`lwm2m_register_validate_callback`.

Limitations
===========

Cache size should be manually set so small that the content can fit normal packets sizes.
When cache is full, new values are dropped.

LwM2M engine and application events
***********************************

The Zephyr LwM2M engine defines events that can be sent back to the application through callback
functions.
The engine state machine shows when the events are spawned.
Events depicted in the diagram are listed in the table.
The events are prefixed with ``LWM2M_RD_CLIENT_EVENT_``.

.. figure:: images/lwm2m_engine_state_machine.svg
    :alt: LwM2M engine state machine

    State machine for the LwM2M engine

.. list-table:: LwM2M RD Client events
   :widths: auto
   :header-rows: 1

   * - Event ID
     - Event Name
     - Description
   * - 0
     - NONE
     - No event
   * - 1
     - BOOTSTRAP_REG_FAILURE
     - Bootstrap registration failed.
       Occurs if there is a timeout or failure in bootstrap registration.
   * - 2
     - BOOTSTRAP_REG_COMPLETE
     - Bootstrap registration complete.
       Occurs after successful bootstrap registration.
   * - 3
     - BOOTSTRAP_TRANSFER_COMPLETE
     - Bootstrap finish command received from the server.
   * - 4
     - REGISTRATION_FAILURE
     - Registration to LwM2M server failed.
       Occurs if server rejects the registration attempt.
   * - 5
     - REGISTRATION_COMPLETE
     - Registration to LwM2M server successful.
       Occurs after a successful registration reply from the LwM2M server
       or when session resumption is used.
   * - 6
     - REG_TIMEOUT
     - Registration status lost.
       Occurs if there is socket errors or message timeouts. Client have lost connection to the server.
   * - 7
     - REG_UPDATE_COMPLETE
     - Registration update completed.
       Occurs after successful registration update reply from the LwM2M server.
   * - 8
     - DEREGISTER_FAILURE
     - Deregistration to LwM2M server failed.
       Occurs if there is a timeout or failure in the deregistration.
   * - 9
     - DISCONNECT
     - LwM2M client have de-registered from server and is now stopped.
       Triggered only if the application have requested the client to stop.
   * - 10
     - QUEUE_MODE_RX_OFF
     - Used only in queue mode, not actively listening for incoming packets.
       In queue mode the client is not required to actively listen for the incoming packets
       after a configured time period.
   * - 11
     - ENGINE_SUSPENDED
     - Indicate that client has now paused as a result of calling :c:func:`lwm2m_engine_pause`.
       State machine is no longer running and the handler thread is suspended.
       All timers are stopped so notifications are not triggered.
   * - 12
     - SERVER_DISABLED
     - Server have executed the disable command.
       Client will deregister and stay idle for the disable period.
   * - 13
     - NETWORK_ERROR
     - Sending messages to the network failed too many times.
       Client cannot reach any servers or fallback to bootstrap.
       LwM2M engine cannot recover and have stopped.

The LwM2M client engine handles most of the state transitions automatically. The application
needs to handle only the events that indicate that the client have stopped or is in a state
where it cannot recover.

.. list-table:: How application should react to events
   :widths: auto
   :header-rows: 1

   * - Event Name
     - How application should react
   * - NONE
     - Ignore the event.
   * - BOOTSTRAP_REG_FAILURE
     - Try to recover network connection. Then restart the client by calling :c:func:`lwm2m_rd_client_start`.
       This might also indicate configuration issue.
   * - BOOTSTRAP_REG_COMPLETE
     - No actions needed
   * - BOOTSTRAP_TRANSFER_COMPLETE
     - No actions needed
   * - REGISTRATION_FAILURE
     - No actions needed.
       Client proceeds re-registration automatically. Might need a bootstrap or configuration fix. Cannot send or receive data.
   * - REGISTRATION_COMPLETE
     - No actions needed.
       Application can send or receive data.
   * - REG_TIMEOUT
     - No actions needed.
       Client proceeds to re-registration automatically. Cannot send or receive data.
   * - REG_UPDATE_COMPLETE
     - No actions needed
       Application can send or receive data.
   * - DEREGISTER_FAILURE
     - No actions needed, client proceeds to idle state automatically. Cannot send or receive data.
   * - DISCONNECT
     - Engine have stopped as a result of calling :c:func:`lwm2m_rd_client_stop`.
       If connection is required, the application should restart the client by calling :c:func:`lwm2m_rd_client_start`.
   * - QUEUE_MODE_RX_OFF
     - No actions needed.
       Application can send but cannot receive data.
       Any data transmission will trigger a registration update.
   * - ENGINE_SUSPENDED
     - Engine can be resumed by calling :c:func:`lwm2m_engine_resume`.
       Cannot send or receive data.
   * - SERVER_DISABLED
     - No actions needed, client will re-register once the disable period is over.
       Cannot send or receive data.
   * - NETWORK_ERROR
     - Try to recover network connection. Then restart the client by calling :c:func:`lwm2m_rd_client_start`.
       This might also indicate configuration issue.

Sending of data in the table above refers to calling :c:func:`lwm2m_send_cb` or by writing into one of the observed resources where observation would trigger a notify message.
Receiving of data refers to receiving read, write or execute operations from the server. Application can register callbacks for these operations.

Configuring lifetime and activity period
****************************************

In LwM2M engine, there are three Kconfig options and one runtime value that configures how often the
client will send LwM2M Update message.

.. list-table:: Update period variables
   :widths: auto
   :header-rows: 1

   * - Variable
     - Effect
   * - LwM2M registration lifetime
     - The lifetime parameter in LwM2M specifies how long a device's registration with an LwM2M server remains valid.
       Device is expected to send LwM2M Update message before the lifetime exprires.
   * - :kconfig:option:`CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME`
     - Default lifetime value, unless set by the bootstrap server.
       Also defines lower limit that client accepts as a lifetime.
   * - :kconfig:option:`CONFIG_LWM2M_UPDATE_PERIOD`
     - How long the client can stay idle before sending a next update.
   * - :kconfig:option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY`
     - Minimum time margin to send the update message before the registration lifetime expires.

.. figure:: images/lwm2m_lifetime_seconds_early.png
    :alt: LwM2M seconds to update early

    Default way of calculating when to update registration.

By default, the client uses :kconfig:option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` to calculate how
many seconds before the expiration of lifetime it is going to send the registration update.
The problem with default mode is when the server changes the lifetime of the registration.
This is then affecting the period of updates the client is doing.
If this is used with the QUEUE mode, which is typical in IPv4 networks, it is also affecting the
period of when the device is reachable from the server.

.. figure:: images/lwm2m_lifetime_both.png
    :alt: LwM2M update time when both values are set

    Update time is controlled by UPDATE_PERIOD.

When also the :kconfig:option:`CONFIG_LWM2M_UPDATE_PERIOD` is set, time to send the update message
is the earliest when any of these values expire. This allows setting long lifetime for the
registration and configure the period accurately, even if server changes the lifetime parameter.

In runtime, the update frequency is limited to once in 15 seconds to avoid flooding.

.. _lwm2m_shell:

LwM2M shell
***********
For testing the client it is possible to enable Zephyr's shell and LwM2M specific commands which
support changing the state of the client. Operations supported are read, write and execute
resources. Client start, stop, pause and resume are also available. The feature is enabled by
selecting :kconfig:option:`CONFIG_LWM2M_SHELL`. The shell is meant for testing so productions
systems should not enable it.

One imaginable scenario, where to use the shell, would be executing client side actions over UART
when a server side tests would require those. It is assumed that not all tests are able to trigger
required actions from the server side.

.. code-block:: console

  uart:~$ lwm2m
  lwm2m - LwM2M commands
  Subcommands:
    send    :send PATHS
            LwM2M SEND operation

    exec    :exec PATH [PARAM]
            Execute a resource

    read    :read PATH [OPTIONS]
            Read value from LwM2M resource
            -x   Read value as hex stream (default)
            -s   Read value as string
            -b   Read value as bool (1/0)
            -uX  Read value as uintX_t
            -sX  Read value as intX_t
            -f   Read value as float
            -t   Read value as time_t

    write   :write PATH [OPTIONS] VALUE
            Write into LwM2M resource
            -s   Write value as string (default)
            -b   Write value as bool
            -uX  Write value as uintX_t
            -sX  Write value as intX_t
            -f   Write value as float
            -t   Write value as time_t

    create  :create PATH
            Create object or resource instance

    delete  :delete PATH
            Delete object or resource instance

    cache   :cache PATH NUM
            Enable data cache for resource
            PATH is LwM2M path
            NUM how many elements to cache

    start   :start EP_NAME [BOOTSTRAP FLAG]
            Start the LwM2M RD (Registration / Discovery) Client
            -b   Set the bootstrap flag (default 0)

    stop    :stop [OPTIONS]
            Stop the LwM2M RD (De-register) Client
            -f   Force close the connection

    update  :Trigger Registration Update of the LwM2M RD Client

    pause   :LwM2M engine thread pause
    resume  :LwM2M engine thread resume
    lock    :Lock the LwM2M registry
    unlock  :Unlock the LwM2M registry
    obs     : List observations
    ls      : ls [PATH]
            List objects, instances, resources




.. _lwm2m_api_reference:

API Reference
*************

.. doxygengroup:: lwm2m_api

.. _LwM2M:
   https://www.omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/

.. _LwM2M registry:
   http://www.openmobilealliance.org/wp/OMNA/LwM2M/LwM2MRegistry.html

.. _OMA Specworks LwM2M:
   https://www.omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/

.. _LwM2M specification 1.0.2:
   http://openmobilealliance.org/release/LightweightM2M/V1_0_2-20180209-A/OMA-TS-LightweightM2M-V1_0_2-20180209-A.pdf

.. _LwM2M specification 1.1.1:
   http://openmobilealliance.org/release/LightweightM2M/V1_1_1-20190617-A/
