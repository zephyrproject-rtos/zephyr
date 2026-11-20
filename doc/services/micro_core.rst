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
disconnections. From this Zephyr checkout in a west workspace, run all three suites on the
host native simulator::

   export ZEPHYR_BASE="$PWD"
   export ZEPHYR_TOOLCHAIN_VARIANT=host
   unset PYTHONHOME PYTHONPATH
   west twister -p native_sim/native/64 \
     -s mc.comm -s mc.msg -s mc.comm.stress.native \
     -i -O ../build/mc-native-all
   find ../build/mc-native-all -name handler.log -exec cat {} +

For the x86 QEMU stress scenarios, see
:zephyr_file:`tests/subsys/mc/comm_stress/README.rst`. Set ``ZEPHYR_BASE`` to this checkout
when an SDK environment points to a different Zephyr tree. Its direct ``west build`` and
``west build -t run`` commands print each ztest case in the terminal. The x86 board is selected
by ``-b``; the test CMake file uses the in-memory transport when UART is disabled.

To run the same suites on each x86 QEMU board, set ``ZEPHYR_SDK_INSTALL_DIR`` to a Zephyr SDK
with the x86 GNU target and QEMU host tools. Run the 32-bit board separately::

   export ZEPHYR_BASE="$PWD"
   export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
   export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk
   unset PYTHONHOME PYTHONPATH
   west twister -p qemu_x86/atom \
     -s mc.comm -s mc.msg -s mc.comm.stress.qemu_x86 \
     -i -O ../build/mc-qemu-x86-all
   find ../build/mc-qemu-x86-all -name handler.log -exec cat {} +

Run the 64-bit board with its own stress scenario and output directory::

   west twister -p qemu_x86_64/atom \
     -s mc.comm -s mc.msg -s mc.comm.stress.qemu_x86_64 \
     -i -O ../build/mc-qemu-x86-64-all
   find ../build/mc-qemu-x86-64-all -name handler.log -exec cat {} +

Twister prints the full ztest output only for failures with ``-i``. The
``handler.log`` files show each case after a successful run. The direct
``west build -t run`` commands in the stress test README print the cases in the terminal.
For each of the three Twister commands above, a complete passing run reports::

   3 of 3 executed test configurations passed (100.00%)
   25 of 25 executed test cases passed (100.00%)

The three configurations are ``mc.comm``, ``mc.msg``, and the selected
platform's ``mc.comm.stress`` scenario. These are execution results, not
build-only results. The stress suite contains eight cases; its
``handler.log`` shows the per-case names and pass or fail status.

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

The native simulator suites passed 25 of 25 tests again on 2026-11-03. The 2026-11-01 gcovr
XML report measured the following production-source coverage:

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

On 2026-11-01, two nRF54L15 DK boards also ran the UART tests. The communication and message
integration tests each passed one case on both the sender and receiver. The communication stress
suite passed eight cases on both boards. These hardware executions are separate from the
build-only Twister configurations and do not contribute to the native simulator coverage above.

On 2026-11-03, separate runs of the communication, message, and stress suites passed 25 of 25
cases on ``qemu_x86/atom`` and 25 of 25 on ``qemu_x86_64/atom`` using Zephyr SDK 1.0.1. Each
board ran three configurations, including eight stress cases. These QEMU runs are execution
results; they do not add to the native simulator coverage measurement.

Historical Micro Core Linux/UDP evidence
========================================

The original ``micro_core`` repository at commit ``8dce228`` records gcov line results of
approximately 300/302 (99.34%) for five communication source files and 125/131 (95.42%) for
``mc_message.c``. Its communication report calls 99.674% an average of per-file percentages;
that is not aggregate line coverage. The reports also record Linux/UDP fault campaigns and
Valgrind runs. Those results concern different source code and do not measure or certify this
Zephyr implementation.
