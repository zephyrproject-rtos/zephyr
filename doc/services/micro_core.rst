.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _micro_core:

Micro Core communication
########################

Micro Core provides two optional communication services. :kconfig:option:`CONFIG_MC_COMM`
transfers complete payloads over application-provided frame callbacks using CRC-16, sequence
numbers, acknowledgments, and bounded retransmission. :kconfig:option:`CONFIG_MC_MSG` adds
16-bit message IDs, subscriptions, and zero-length signals. The transport can be UART, radio,
or another frame-oriented link. Its callbacks must read and write one complete frame or return
a negative error code.

Memory ownership
****************

All **persistent instance state and internal buffers** are embedded in the caller-provided
storage. This includes the communication and message objects, transmit and receive windows,
reassembly space, message space, and subscription table. The subsystem does not call a heap
allocator. Query :c:func:`mc_comm_req_size` or :c:func:`mc_msg_req_size` after filling the
configuration, then provide at least that many bytes aligned to 8 bytes. The storage must remain
valid and exclusive to the instance until the application stops using it. The size functions
return zero for invalid configurations; initialization rejects insufficient or misaligned
storage. The message size includes its embedded communication instance.

Temporary local variables, including a small acknowledgment frame, use the thread stack. The
application also owns transport callback context, input and output buffers, and any memory used
by its transport driver. These are outside the instance storage guarantee. A message callback's
payload pointer refers to instance storage and remains valid only during that callback.

For example, initialize a communication instance with the following configuration::

   static uint8_t storage[512] __aligned(8);
   struct mc_comm *comm;
   struct mc_comm_cfg config = {
       .io.receive = read_frame,
       .io.transmit = write_frame,
       .io.context = transport_context,
       .recv.size = 64U,
       .send.size = 64U,
       .receive_capacity = 64U,
   };

   size_t required = mc_comm_req_size(&config);
   if (required == 0U || required > sizeof(storage)) {
       /* Select a larger, aligned storage buffer or revise the configuration. */
   } else {
       int err = mc_comm_init(&comm, storage, sizeof(storage), &config);
       /* Check err before using comm. */
   }

Each window ``size`` includes the 12-byte protocol header; each ``capacity`` is the number of
buffered frames and defaults to one when zero. ``receive_capacity`` limits the size of a
complete payload. For ``mc_msg_cfg``, ``pool_size`` limits the message payload and
``ids_capacity`` limits active subscriptions. Both limits are fixed at initialization.

The ``samples/subsys/micro_core/uart`` sample shows a two-board UART transport adapter and
the communication API.

Behavior and limits
*******************

The receiver checks frame length, header fields, and CRC before processing data. Sequence
numbers suppress duplicate delivery and allow limited frame reordering. A sender retransmits
unacknowledged frames until a configured retry or time limit is reached. These mechanisms can
improve behavior on noisy links; CRC detects errors but is not authentication or a guarantee
against every possible corruption pattern. The implementation does not provide encryption.

Calls to :c:func:`mc_comm_send` and :c:func:`mc_msg_send` wait for acknowledgments. A timeout
means delivery is uncertain: the peer may already have received the payload. Applications
must choose a retry and deduplication policy suitable for their messages. The current wire
format is Zephyr-specific and is not interoperable with the original Linux ``micro_core``
implementation. The instances are not internally synchronized; applications must serialize
calls that share an instance. Callbacks execute in the context of the caller processing input.

The fixed, caller-owned memory budget avoids subsystem heap fragmentation and makes instance
RAM predictable. Bounds checks, explicit errors, and targeted fault tests support review for
safety-sensitive applications. They do not establish functional safety certification, a
worst-case execution-time bound, or suitability for a particular safety integrity level.

Verification
************

The Zephyr tests under ``tests/subsys/mc`` cover configuration errors, transfers, message
dispatch, signals, malformed frames, loss, corruption, reordered and duplicate frames,
timeouts, and guarded storage. The native stress suite also exercises repeated transfers and
disconnections. Build and run the communication and message suites in a west workspace with the
host toolchain::

   ZEPHYR_BASE=$PWD ZEPHYR_TOOLCHAIN_VARIANT=host west twister \
     -p native_sim/native/64 -s mc.comm -s mc.msg -s mc.comm.stress.native -i

The original Linux/UDP scenario categories have these Zephyr equivalents:

.. list-table:: Scenario mapping
   :header-rows: 1

   * - Original category
     - Zephyr verification
   * - Invalid and valid creation
     - ``mc.comm`` and ``mc.msg`` configuration and guarded-storage tests
   * - Clean and duplicate transfer
     - ``mc.comm.stress.native`` and ``mc.msg`` repeated-message tests
   * - Moderate loss, corruption, and reordering
     - ``mc.comm.stress.native`` fault phases, ``mc.comm`` reordering, and ``mc.msg`` noisy link
   * - Extreme loss, disconnect, and timeout
     - ``mc.comm.stress.native`` high-loss and timeout phases; ``mc.msg`` timeout and recovery
   * - Message dispatch and signal
     - ``mc.msg`` subscription, payload, signal, and malformed-message tests

Run coverage with ``gcovr`` installed::

   ZEPHYR_BASE=$PWD ZEPHYR_TOOLCHAIN_VARIANT=host west twister \
     -p native_sim/native/64 -s mc.comm -s mc.msg -s mc.comm.stress.native \
     --coverage --coverage-tool gcovr --coverage-formats xml,txt -i \
     --outdir ../build/mc-coverage
   python3 tests/subsys/mc/coverage_report.py \
     ../build/mc-coverage/coverage/coverage.xml --min-line-percent 97

On 2026-11-01, the native simulator run passed 25 of 25 tests. The gcovr XML report measured
the following production-source coverage:

.. list-table:: Zephyr Micro Core coverage
   :header-rows: 1

   * - Source
     - Executable lines covered
     - Line coverage
     - Branch rate
   * - ``comm.c``
     - 277/278
     - 99.64%
     - 90.38%
   * - ``msg.c``
     - 108/109
     - 99.08%
     - 87.50%

These results use the host GNU toolchain and the three listed Twister scenarios. The line
coverage target is at least 97% for each file. The branch rates are reported separately and
are not MC/DC coverage. The build output contains the full XML and text reports; other target
boards and test selections require separate measurements.

The uncovered communication line is a defensive check against an already-full reassembly
state. The uncovered message line propagates a nested initialization error after the
configuration and storage size have already been validated. Both paths remain in place for
defense; neither percentage proves that all error conditions have been tested.

Historical Micro Core Linux/UDP evidence
========================================

The original ``micro_core`` repository at commit ``8dce228`` records gcov line results of
approximately 300/302 (99.34%) for five communication source files and 125/131 (95.42%) for
``mc_message.c``. Its communication report calls 99.674% an average of per-file percentages;
that is not aggregate line coverage. The reports also record Linux/UDP fault campaigns and
Valgrind runs. Those results concern different source code and do not measure or certify this
Zephyr implementation.
