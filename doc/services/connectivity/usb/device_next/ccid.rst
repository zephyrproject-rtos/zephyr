.. _usb_device_ccid:

USB CCID instances
##################

Enable ``CONFIG_USBD_CCID_CLASS``. The number of CCID instances is derived
directly from enabled ``zephyr,ccid`` devicetree nodes. No Kconfig instance
count or limit is needed. Buffers and worker stacks are allocated only for
these nodes, independently of the logical IDs assigned to them.

Each enabled ``zephyr,ccid`` node creates one independent single-slot CCID
interface. Define nodes below the USB device controller, for example:

.. code-block:: devicetree

   &zephyr_udc0 {
       card_reader: ccid-reader {
           compatible = "zephyr,ccid";
           instance-id = <2>;
           label = "Smart card reader";
       };
   };

This node creates USB class ``ccid_2``. Register its backend with
``usbd_ccid_register_backend(2, &backend)`` before calling
``usbd_init()``. Initialization fails with ``-ENODEV`` if an instance has no
registered backend. The application can obtain the ID without repeating a literal:
``DT_PROP(DT_NODELABEL(card_reader), instance_id)``. Use the same ID with
``usbd_ccid_card_changed()`` to report presence changes.

``struct usbd_ccid_backend`` bundles the operation table and reader configuration.
All card callbacks receive a pointer to this registered object. Embed it in an
application-specific structure and use ``CONTAINER_OF()`` to recover private
state; no untyped callback context is needed. Each object can be bound to one
interface only. Complete registrations sequentially before ``usbd_init()``.
The object, its operation table and referenced capability tables must remain
valid for the class lifetime. Do not change ``ops`` or ``config`` after binding.

``instance-id`` is an explicit logical ID. It does not depend on the order of
nodes or Zephyr's devicetree instance enumeration. It is neither the USB
``bInterfaceNumber`` assigned by the USB stack nor the CCID slot number, which
is always zero for these single-slot interfaces. ``reg`` is not used because
the ID is not an address on the USB controller's bus.

The build fails if enabled nodes have duplicate IDs. Gaps are allowed and
consume no additional storage; the backend API returns ``-ENODEV`` for IDs
without an enabled node. Disabled nodes do not create classes or consume
CCID buffers and stacks. The USB controller must provide enough endpoints
for the selected interfaces.

``label`` supplies an ASCII7 USB interface string and defaults to ``CCID``
when omitted. It does not select an APDU protocol or a card application.
The registered backend interprets APDUs and may expose any card application,
including multiple applications behind the same interface.

USB speed
=========

CCID follows the global USB stack speed selection. With
``CONFIG_USBD_MAX_SPEED_FULL=y``, high-speed endpoint descriptors, descriptor
lists and their references are omitted. On a high-speed-capable controller,
also disable ``CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED`` to select
full-speed-only operation. High-speed builds retain the full-speed descriptors
for fallback operation.

For a high-speed-capable controller, select high-speed operation with
the following settings in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED=y
   CONFIG_USBD_MAX_SPEED_HIGH=y

For full-speed-only operation, replace these two settings with:

.. code-block:: kconfig

   CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED=n
   CONFIG_USBD_MAX_SPEED_FULL=y

Card parameters
===============

The class uses APDU exchange and does not select a card protocol or interpret
protocol parameter bytes. Every backend must
implement ``get_parameters``, ``set_parameters`` and ``reset_parameters`` in
addition to the card state, power and APDU operations. These callbacks run
in the instance's worker thread and return ``struct usbd_ccid_parameters``:
the selected protocol number, a pointer to parameter bytes and their length.
T=0 has five parameter bytes, T=1 has seven. The optional
``usbd_ccid_t0_parameters`` and ``usbd_ccid_t1_parameters`` structures are
layout helpers for backends, not restrictions imposed by the class.
Returned data must remain valid until the next backend callback on that
instance; the class checks bounds and copies it into its USB response.
Never return a pointer to callback-local stack storage.

The header provides ``USBD_CCID_T0_PARAMETERS_DEFAULT`` and
``USBD_CCID_T1_PARAMETERS_DEFAULT`` as reusable initializers. Both use initial
ISO/IEC 7816-3 parameters, direct convention and no clock-stop support; the
T=1 initializer uses LRC, BWI=4, CWI=13 and IFSC=32. For example:

.. code-block:: c

   static const struct usbd_ccid_t1_parameters parameters =
       USBD_CCID_T1_PARAMETERS_DEFAULT;

These are starting values, not universal card or application profiles. The
backend must adapt them to its ATR and negotiated settings. Named constants
cover convention, checksum and clock-stop support; ``set_clock`` reports a
separate ``enum usbd_ccid_clock_status``. A GIDS or PIV application does not
determine a complete set of transport parameters.

Set each backend object's ``config`` to a ``struct usbd_ccid_reader_config``. Capabilities
are per instance: protocols, voltage support, maximum IFSD, automation and
APDU features, and supported clocks and data rates. The class has no
application-specific defaults for these fields. The class references the backend
configuration directly, including its clock/rate tables.
Tables contain nonzero, strictly ascending values; their first entries are
the defaults and their last entries the maxima reported in the descriptor.

.. code-block:: c

   static const uint32_t clocks_khz[] = { 4000, 8000 };
   static const uint32_t rates_bps[] = { 9600, 19200 };
   static const struct usbd_ccid_reader_config reader_config = {
       .protocols = USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T0) |
                    USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T1),
       .default_protocol = USBD_CCID_PROTOCOL_T0,
       .features = USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_NEGOTIATION |
                   USBD_CCID_AUTO_VOLTAGE | USBD_CCID_EXTENDED_APDU,
       .voltage_support = USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_3V),
       .max_ifsd = USBD_CCID_T1_IFS_MAX,
       .clock_frequencies = clocks_khz,
       .clock_count = ARRAY_SIZE(clocks_khz),
       .data_rates = rates_bps,
       .data_rate_count = ARRAY_SIZE(rates_bps),
   };

   struct application_card {
       struct usbd_ccid_backend backend;
       /* Application-specific state. */
   };
   static struct application_card card;

   /* Before usbd_init(); ops is the application's callback table. */
   card.backend.ops = &ops;
   card.backend.config = reader_config;
   usbd_ccid_register_backend(2, &card.backend);

Only advertise protocols defined by the applicable CCID specification and
implemented by the backend. CCID 1.1 defines parameter layouts for T=0 and
T=1; its synchronous protocol numbers are reserved and have no defined
parameter layouts. Opaque parameter data does not make reserved protocols
interoperable with standard host drivers.

The backend owns current/default parameters and must keep them consistent with
the ATR returned by ``power_on``. Power-on/reset must select the appropriate
protocol and parameters, including any negotiation. The configured APDU
exchange level requires automatic ATR parameter configuration and either
automatic negotiation or automatic PPS.
After power-on it queries ``get_parameters`` to track the selected protocol,
including in subsequent failure responses.

``set_parameters`` applies supported changes atomically and returns the
effective protocol and parameters. The backend validates both the requested
data length and contents. Unsupported changes return a positive CCID
``bError`` value (1..255), for example an offending field offset from
``enum usbd_ccid_parameter_error``. The class then queries the unchanged
current settings and includes them in the failure response (CCID 6.2.3).
Operation failures return negative errno values;
``-ETIMEDOUT`` reports an absent or unresponsive card. Failed changes must
leave all current parameters unchanged. The convention bit in the request
is ignored for T=0/T=1; the returned convention is determined by the ATR.

``power_on`` receives the host's voltage selection. The class rejects voltages
outside the advertised mask, and automatic selection requires
``USBD_CCID_AUTO_VOLTAGE``. The backend is responsible for actually selecting
the voltage and returning a consistent ATR and card state.

``get_data_rate_and_clock`` reports actual backend-selected settings after
power-on and on a host rate/clock query. ``set_data_rate_and_clock`` is optional
for fixed/automatically selected settings. It applies a supported pair
atomically and returns the effective pair. Forced values for automatically
selected fields are ignored, as required by CCID 6.1.14. The class validates
returned values against the advertised capability tables.

``set_clock`` is required if ``USBD_CCID_CLOCK_STOP`` is advertised. It starts
or stops the card clock and reports its actual CCID clock status. Unsupported
hardware functions are rejected instead of silently acknowledged.

For a fixed virtual card, keep the profile as a constant in the application
backend. Get and reset return that profile; set accepts matching values and
rejects unsupported changes. No profile bytes are configured in devicetree.

APDU buffers and USB transfers
=============================

``CONFIG_USBD_CCID_MAX_MESSAGE_SIZE`` bounds a single CCID USB message,
including its ten-byte header. ``CONFIG_USBD_CCID_MAX_APDU_SIZE`` bounds each
assembled command and response. Extended APDU readers can receive commands
in multiple CCID messages and return responses in multiple messages. The
backend receives one assembled command and produces one complete response.
An APDU larger than the configured capacity fails with ``XFR_OVERRUN``.
Short APDU readers reject chaining and enforce the short APDU size limits.

Runtime state is grouped into USB, backend, worker, receive, transmit, ABORT
and APDU state. Fixed instance bindings are stored separately as constants.
Each instance has a separate zero-initialized buffer store in BSS, requiring
``CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + 2 * CONFIG_USBD_CCID_MAX_APDU_SIZE``
bytes. This keeps buffer zero bytes out of the initialized flash image;
moving them out of the context does not remove their RAM requirement.
One USB message workspace is reused for command reception and the resulting
reply. Independent command and response APDU buffers preserve the backend's
input while it writes its output and support extended APDU chaining.

The class adds a zero-length USB packet when an IN message fills complete
USB packets, as required by CCID 3.1.3.

Bulk-OUT is received one USB packet at a time and assembled according to the
CCID header's ``dwLength``. Messages ending on an exact packet boundary do
not require an OUT ZLP. An optional trailing ZLP is accepted even before
the host reads the response. Premature short transfers fail length validation;
oversized messages are drained with bounded storage. While the worker or its
reply owns the message workspace, reception keeps only the ten-byte header
and drains payloads. ABORT needs no additional message-sized buffer. A frame
that started while busy remains rejected even if the old reply completes
between its packets. The worker keeps an independent request header for
time extensions and completion handling after the workspace becomes a reply.

Long operations and cancellation
================================

Each interface has its own backend worker. USB control and endpoint handling
continue while a worker callback waits. Configure
``time_extension_interval_ms`` and ``time_extension_multiplier`` for callbacks
that can exceed the host's waiting time. The class periodically sends a CCID
time-extension response using the outstanding command's type, slot and sequence.
Choose an interval shorter than both the host's initial waiting time and the
extended waiting time, taking the backend's T=0 WWT or T=1 BWT into account.
An interval of zero disables automatic extensions and is suitable only for
callbacks that finish within the host's waiting time. These messages do not
replace the backend's card-side timeout and protocol handling.

ABORT is a pair of control and bulk requests with matching slot/sequence
values. Its sequence need not equal the sequence of the interrupted command.
An optional, nonblocking ``abort`` callback requests backend cancellation;
the class supports the handshake even without this callback. It waits until
the old backend operation has returned before acknowledging the pair. While
waiting for the matching bulk request, other commands fail with
``CMD_ABORTED``. The backend must eventually return, either after cancellation
or its own bounded timeout. Already queued IN packets may finish during
ABORT; their completion cannot consume a new pending reply. As specified by
CCID 5.3.1, the host discards old replies until it receives the matching
ABORT acknowledgment.

``get_state`` and ``abort`` may run concurrently with worker callbacks and
must not wait for a lock held by a blocking callback. Publish card state
atomically. ``usbd_ccid_card_changed()`` is safe to call from an ISR; notification
allocation and enqueueing run in class work context, with transient failures
retried automatically.

Disable, reset and suspend invalidate pending USB work. A stale transfer
completion cannot clear a newly queued transfer, and an old backend result
cannot be sent after reconfiguration. The worker calls ``power_off`` after
any running callback has quiesced. A new session requires power-on; backend
buffers are not reused before the previous callback returns. Suspend/resume
uses the same invalidation and deferred power-off behavior.

See :ref:`usbd_ccid_api` and :zephyr:code-sample:`usb-ccid`.
