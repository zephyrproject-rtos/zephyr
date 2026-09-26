.. _knx:

KNX
###

`KNX <https://www.knx.org/>`_ (ISO/IEC 14543-3, also standardized as
EN 50090) is a field-bus protocol for home and building automation. This
subsystem implements a KNX device stack for Zephyr, currently covering:

* **TP1** (twisted pair, 9600 bit/s) as the bus medium.
* **S-Mode**, **System B** (mask version ``07B0h``): a device whose
  parameters and Group Object bindings are configured by the ETS
  (`Engineering Tool Software <https://www.knx.org/knx-en/for-professionals/software/>`_)
  through direct memory access, rather than compiled in.
* An **end device** role only — no line/backbone coupler, no IP or RF
  medium, no KNX Data Secure.

Within that scope the stack implements the physical layer up through the
application layer (see *Architecture* below), the Interface Object /
Property model ETS uses to configure a device, and DPT (Datapoint Type)
encoding for the most common families.

Source layout
**************

.. list-table::
   :header-rows: 1

   * - Location
     - Contents
   * - :zephyr_file:`subsys/knx`
     - The protocol stack itself: one translation unit per OSI layer, plus
       the Interface Object / Property and DPT support.
   * - :zephyr_file:`drivers/knx`
     - Physical-layer (L1) drivers: :ref:`the NCN5130 transceiver driver <knx_ncn5130>`
       and a software loopback driver (:kconfig:option:`CONFIG_KNX_LOOPBACK`)
       for testing without a bus.
   * - :zephyr_file:`include/zephyr/knx`
     - Public headers: packet buffer (``knx_pkt.h``), core types
       (``knx_core.h``), the L1/L2 contract (``knx_l1.h``), DPT encode/decode
       (``dpt.h``), and the application-data size-check macros
       (``knx_app_data.h``, see :ref:`knx_application`).
   * - :zephyr_file:`tests/subsys/knx`
     - Host-side ``ztest`` suites (``unit_testing`` and ``native_sim``
       targets) covering the wire encoding, the Transport Layer state
       machine, the Load/Run State Machines and the Property layer.

Architecture
************

A single ``struct knx_pkt`` (allocated from a ``k_mem_slab``,
:kconfig:option:`CONFIG_KNX_PKT_POOL_SIZE`) carries a frame through every
layer. Ownership is refcounted: **every function taking a**
``struct knx_pkt *`` **consumes it** — the callee either frees it
(``knx_pkt_unref()``) or hands it to the next layer. A caller that needs to
keep the pointer afterwards must ``knx_pkt_ref()`` it first.

.. list-table:: OSI layer mapping
   :header-rows: 1

   * - Layer
     - File
     - Notes
   * - L1 Physical
     - :zephyr_file:`subsys/knx/layer1_physical.c`
     - Talks to an L1 driver through ``knx_l1.h``. Frame accumulation, FCS
       and length validation happen above the driver, in L2.
   * - L2 Data Link
     - :zephyr_file:`subsys/knx/layer2_data_link.c`
     - Standard and extended frame formats, FCS, CSMA/CA retry, RX
       repetition filter, priority-ordered TX queues.
   * - L3 Network
     - :zephyr_file:`subsys/knx/layer3_network.c`
     - Hop-count handling; complete for an end device.
   * - L4 Transport
     - :zephyr_file:`subsys/knx/layer4_transport.c`
     - The 28x4 Transport Layer Style 3 state table (with the AN210
       amendment), connection-oriented (``T_Data_Connected``) and
       point-to-point connectionless (``T_Data_Individual``) management.
   * - L7 Application
     - :zephyr_file:`subsys/knx/layer7_application.c`
     - APDU services: property and memory access, device descriptor,
       restart, authorize, and Group Object read/write/response.
   * - Interface Objects
     - :zephyr_file:`subsys/knx/object_interface.c` and ``object_*.c``
     - Declarative property tables (``KNX_PROP_*`` macros), the Load and
       Run State Machines, and property access control.
   * - DPT
     - :zephyr_file:`subsys/knx/dpt.c`
     - Datapoint Type encode/decode, gated by :kconfig:option:`CONFIG_KNX_DPT`.
   * - Application-facing API
     - :zephyr_file:`subsys/knx/knx_group_object.c`
     - Typed Group Object accessors and update/read callbacks — see
       :ref:`knx_application`.

Threads and queues
===================

.. list-table::
   :header-rows: 1

   * - Context
     - Created in
     - Role
   * - UART async callback (ISR)
     - the L1 driver
     - Byte-level transceiver service, TX echo filtering, frame
       accumulation.
   * - ``knx_l2_rx`` thread
     - ``L_Init()``
     - FCS/length validation, header decode, submits work to the queue
       below.
   * - Application work queue
     - ``knx_init()``
     - L3 through L7 RX dispatch, and the L4 connection/ACK timers.
   * - ``knx_l2_tx`` thread
     - ``L_Init()``
     - FCS append, burst TX via the L1 driver, CSMA retry, ``L_Data.con``.

Configuration
*************

The subsystem is enabled with :kconfig:option:`CONFIG_KNX_STACK`, which pulls
in :kconfig:option:`CONFIG_POLL` (needed by the L2 TX thread, which polls
across two priority queues instead of blocking on a single message queue).
A physical-layer driver (:ref:`CONFIG_NCN5130 <knx_ncn5130>` or
:kconfig:option:`CONFIG_KNX_LOOPBACK`) is required alongside it.

Some of the more commonly-tuned options:

.. list-table::
   :header-rows: 1

   * - Option
     - Purpose
   * - :kconfig:option:`CONFIG_KNX_PKT_POOL_SIZE`
     - Number of ``knx_pkt`` slab slots (each ~300 bytes).
   * - :kconfig:option:`CONFIG_KNX_DPT`
     - Build the DPT 1-20 / 232 encode/decode library and the typed Group
       Object accessors.
   * - :kconfig:option:`CONFIG_KNX_GROUP_OBJECT_COUNT`
     - Number of Group Object Table entries.
   * - :kconfig:option:`CONFIG_KNX_MAX_ADDRESS_GROUP`,
       :kconfig:option:`CONFIG_KNX_MAX_ASSOCIATIONS`
     - Group Address / Association Table sizes.
   * - :kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE`,
       :kconfig:option:`CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE`
     - Bytes reserved for the application's own parameter and Group Object
       value storage — see :ref:`knx_application`.
   * - :kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_ID`,
       :kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_VERSION`
     - The ETS application program identity (under
       :kconfig:option:`CONFIG_KNX_DECLARE_APPLICATION_PROGRAM`).
   * - :kconfig:option:`CONFIG_KNX_AUTO_LOAD_TABLES`
     - Start with every table's Load State already ``LOADED`` — useful on
       the bench, before a real ETS commissioning flow exists.
   * - :kconfig:option:`CONFIG_KNX_DEBUG_VERBOSE_L1` ... ``L7``,
       ``SYSTEM``
     - Per-layer frame tracing (requires :kconfig:option:`CONFIG_LOG`).

See :zephyr_file:`subsys/knx/Kconfig` for the full list.

Testing
*******

The host-side suites under :zephyr_file:`tests/subsys/knx` don't need any
hardware. Pure encoding/logic suites build for ``unit_testing``; suites that
need a real kernel (``k_timer``, ``k_work``, ``k_mem_slab``) build for
``native_sim``:

.. code-block:: bash

   west build -p always -b unit_testing tests/subsys/knx/encoding
   ./build/testbinary

   west build -p always -b native_sim tests/subsys/knx/l2_framing
   ./build/zephyr/zephyr.exe

API Reference
**************

.. doxygengroup:: knx_pkt

.. doxygengroup:: knx_core

The :ref:`Group Object accessors <knx_application_go_api>` and the DPT
encode/decode functions and ``DPT_*``/``Type_DPT_*`` names are documented
in :ref:`knx_application`, since applications are the only intended caller
of that surface.

.. toctree::
   :maxdepth: 1

   application.rst
   ncn5130.rst
