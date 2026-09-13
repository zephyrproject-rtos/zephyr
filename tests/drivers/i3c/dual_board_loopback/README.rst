.. zephyr:code-sample:: dual_board_loopback
   :name: I3C Loopback (DW driver, controller + target)

   Two-board I3C loopback regression for the Synopsys DW I3C driver
   (``drivers/i3c/i3c_dw.c``).

Overview
========

This test exercises the DesignWare I3C driver end-to-end on real
hardware using two physical boards wired as a controller and a target
on a single I3C bus.  The suite covers attach/detach, ENTDAA / RSTDAA,
the full CCC set, SDR transfers, IBI (TIR, hot-join,
controller-role-request), recovery from NACK / abort, runtime SCL
configuration, and basic PM.

Two Zephyr applications live under ``controller_driven/``, which holds
the suites whose ZTESTs run on the controller and drive the target
through the sync channel:

``controller_driven/i3c_controller/``
    Runs on **Board A**.  Builds with ``CONFIG_I3C_CONTROLLER_ROLE_ONLY=y``
    and contains the actual ``ZTEST_SUITE``.  Drives the bus, issues
    CCCs and SDR transfers, attaches/detaches devices, enables IBIs.

``controller_driven/i3c_target/``
    Runs on **Board B**.  Builds with ``CONFIG_I3C_TARGET_ROLE_ONLY=y``.
    A reactive harness — registers I3C target callbacks and reacts to a
    UART sync channel from Board A so the controller-side ZTESTs can
    pre-stage read data, request IBIs, verify writes, etc.

The ``common/`` directory sits beside ``controller_driven/`` rather than
inside it, so that suites added later can reuse the same sync protocol
and target identity.

A small line-oriented ASCII protocol (``common/sync_proto.h``) is shared
between the two apps over a UART channel that is **not** the I3C bus, so
the test runner on Board A can deterministically coordinate Board B.
Board A requests (``READY`` / ``STAGE_READ`` / ``RESET`` / ``SOFT_RST`` /
``DUMP`` / ``RAISE_TIR`` / ``RAISE_HJ`` / ``RAISE_MR``) and Board B answers
(``HELLO`` / ``ACK`` / ``READY`` / ``DUMP`` / ``PASS`` / ``FAIL`` / ``IBI``).
The ``RAISE_*`` requests and the ``IBI`` notification exist only when both
sides are built with ``CONFIG_I3C_USE_IBI=y``.  See the header for the full
wire format.

Required board configuration
============================

Each board needs a ``boards/<board>.overlay`` (and optionally a
``boards/<board>.conf``) under both ``controller_driven/i3c_controller/``
and ``controller_driven/i3c_target/`` that provides:

* An ``snps,designware-i3c``-compatible (or compatible-derived)
  controller node aliased as ``test-i3c``.
* The ``zephyr,sync-uart`` chosen alias pointing at a UART that is
  cross-wired to the peer board (TX↔RX, RX↔TX, GND).
* Pull-ups on SCL/SDA — either external or, where the SoC supports
  configurable I/O pads, internal.
* Optional: ``la-marker-gpios`` (target) pointing at a free GPIO so a
  logic analyzer can trigger on target-callback entry.

See the existing per-board overlay files for a worked example,
including any vendor-specific pinctrl that the SoC requires for the
controller's ACK detector to function and any vendor-specific notes
about pull-ups, bus speed, and silicon quirks.

Bench configuration options
===========================

Both apps carry a Kconfig menu for bench and IP differences.  The
defaults suit a bench where Board A and Board B are the only devices on
the bus and the SoC is a stock DW integration; a different bench needs
these reviewed.

Controller (``controller_driven/i3c_controller/Kconfig``):

``I3C_DUAL_BOARD_LOOPBACK_BENCH_HAS_PERMANENT_TARGET``
    Enable when the controller's bus carries a target chip besides
    Board B — a sensor soldered to the SOM with no power-enable, for
    example.  The suite then attaches a placeholder descriptor for it so
    its participation in ENTDAA does not take Board B's address slot.
    Its identity is set by ``..._PERMANENT_TARGET_PID_HI``,
    ``..._PERMANENT_TARGET_PID_LO`` and ``..._PERMANENT_TARGET_INIT_DA``.

``I3C_DUAL_BOARD_LOOPBACK_META_TESTS``
    See `Meta tests`_.

Target (``controller_driven/i3c_target/Kconfig``):

``I3C_DUAL_BOARD_LOOPBACK_TARGET_VENDOR_WRAPPER_GATE``
    Enable on integrations that gate the protocol clock behind a vendor
    wrapper register which must be re-opened before ``SOFT_RST`` can
    self-clear.  The register is addressed by
    ``..._WRAPPER_GATE_OFFSET`` (from the controller base) and written
    with ``..._WRAPPER_GATE_VALUE``.

``I3C_DUAL_BOARD_LOOPBACK_TARGET_SOFT_RST_CLEARS_DEVICE_ADDR``
    On by default, matching DW variants where ``SOFT_RST`` clears
    ``DEVICE_ADDR``.  Turn it off where the dynamic address survives the
    reset; the target then zeroes ``DEVICE_ADDR`` itself so it does not
    enter the next enumeration believing it already holds a DA.

Wiring (generic)
================

I3C bus::

    A.I3C_SCL ── B.I3C_SCL
    A.I3C_SDA ── B.I3C_SDA
    GND_A   ──── GND_B

UART sync channel (separate from the I3C bus)::

    A.UART_TX ── B.UART_RX
    A.UART_RX ── B.UART_TX
    GND_A   ──── GND_B

Build
=====

Each app builds independently for the chosen board.

.. code-block:: bash

   # Controller (Board A)
   west build -p always -d build_loopback_a \
       -b <board> tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_controller

   # Target (Board B)
   west build -p always -d build_loopback_b \
       -b <board> tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_target

Twister discovery::

   twister -T tests/drivers/i3c/dual_board_loopback -p <board>

Both ``tests.yaml`` files use ``harness: console`` with
``fixture: dual_board_loopback_pair`` — the fixture must be declared on the
test bench for Twister to schedule them.

Flash and run
=============

The two boards are distinguished by their debugger serial numbers and
COM ports (capture controller output on Board A's COM port; Board B is
silent except for diagnostic ``printk`` lines on its own COM):

.. code-block:: bash

   # Flash target FIRST (Board B is reactive and must be live before A boots)
   west flash -d build_loopback_b --dev-id <SN_B>
   sleep 8

   # Then flash controller and capture its UART
   west flash -d build_loopback_a --dev-id <SN_A>

The controller's ``suite_setup`` runs a 3-way ``HELLO`` / ``READY`` /
``ACK`` handshake over the sync UART before calling ``i3c_bus_init``.
If Board B is absent the handshake times out and the controller
gracefully skips bus traffic instead of crashing — bus-traffic ZTESTs
then ``FAIL`` cleanly via ``I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA``.

A successful run prints ``SUITE PASS`` followed by ``PROJECT EXECUTION
SUCCESSFUL``.

Test files
==========

All ZTESTs live under ``controller_driven/i3c_controller/src/``:

* ``test_attach.c``  — ``i3c_attach_*`` / ``i3c_detach_*`` API, including
  attaching multiple descriptors and duplicate-address rejection.
* ``test_ccc.c``     — full CCC set: ``ENEC``/``DISEC``, ``ENTAS``,
  ``GETBCR``/``GETDCR``/``GETPID``/``GETSTATUS``,
  ``GETMRL``/``GETMWL``, ``SETMRL``/``SETMWL``, ``GETMXDS``,
  ``RSTDAA``, ``SETAASA``, ``SETNEWDA``.
* ``test_config.c``  — runtime SCL frequency change, ``i3c_config_get``,
  device lookup by PID, secondary-controller query.
* ``test_daa.c``     — ``i3c_bus_init`` DAA, ``ENTDAA``-only stress (100
  iterations), ``RSTDAA``/``ENTDAA``/transfer stress (200 iterations),
  BCR/DCR/PID cross-check against the overlay, tolerance of a declared
  but absent device.
* ``test_harness.c`` — ``meta_*`` tests covering the fixture itself, not
  the driver.  They exist because a silently broken recovery path once
  turned a single target hiccup into a suite-wide failure cascade.  Not
  built unless ``CONFIG_I3C_DUAL_BOARD_LOOPBACK_META_TESTS=y`` — see `Meta tests`_.
* ``test_ibi.c``     — TIR (with / without mandatory byte, extended
  payload, back-to-back), enable/disable, hot-join, controller-role
  request.
* ``test_pm.c``      — suspend / resume.
* ``test_recovery.c``— ``i3c_recover_bus`` API, recovery after
  ``ADDRESS_NACK``, recovery after a halt caused by a queued NACK,
  recovery after invalid CCC.
* ``test_xfer.c``    — single-byte / max-payload (256 B) / FIFO
  boundary / multi-message / write-then-read repeated-start /
  back-to-back stress / the ``i3c_write_read``-style helpers.

Meta tests
==========

The ``meta_*`` tests in ``test_harness.c`` are guarded by
``CONFIG_I3C_DUAL_BOARD_LOOPBACK_META_TESTS`` and are **off by default**:

.. code-block:: bash

   west build -p always -d build_loopback_a \
       -b <board> tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_controller \
       -- -DCONFIG_I3C_DUAL_BOARD_LOOPBACK_META_TESTS=y

They verify the fixture rather than the driver.  They target frustrating-to-debug
failures that were encountered while developing the harness itself - e.g. a 1-in-200
run occurrence that can cause half the suite to fail.

Enable these when working on the harness or the recovery path; leave them off
for routine driver runs.

IBI configuration
=================

Both apps default to ``CONFIG_I3C_USE_IBI=y``.  The suite also builds and runs
with IBI disabled, which is what a target that never raises in-band interrupts
ships with:

.. code-block:: bash

   west build -p always -d build_ibi_n_a -b <board> \
       tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_controller \
       -- -DCONFIG_I3C_USE_IBI=n

   west build -p always -d build_ibi_n_b -b <board> \
       tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_target \
       -- -DCONFIG_I3C_USE_IBI=n

Set the symbol on both boards.  A mixed pair does run, but it is not a
configuration worth reporting results for: every IBI test is controller-side,
so an IBI=n controller against an IBI=y target simply drops them.

With IBI disabled the eight ``test_ibi_*`` cases are compiled out of
``test_ibi.c`` and replaced by a single ``test_ibi_not_compiled`` placeholder
that skips, and the target drops its TIR / HJ / MR raise handlers along with
the ``RAISE_*`` sync commands.  Everything else — DAA, CCCs, transfers,
recovery, runtime config and PM — is unchanged.

Run both configurations when touching the driver's interrupt masks or its ISR.
A bit left enabled in ``INTR_MASTER_MASK`` or ``INTR_SLAVE_MASK`` whose handler
sits behind ``#ifdef CONFIG_I3C_USE_IBI`` is never cleared in the build that
compiles the handler out, and the level-sensitive status then re-enters the ISR
indefinitely — a failure mode neither configuration can show on its own.

Test results
============

On the validated bench (both boards, controller suite, meta tests
disabled):

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Configuration
     - Result
   * - ``CONFIG_I3C_USE_IBI=y``
     - ``SUITE PASS`` — 59 PASS / 2 SKIP / 0 FAIL out of 61
   * - ``CONFIG_I3C_USE_IBI=n``
     - ``SUITE PASS`` — 53 PASS / 1 SKIP / 0 FAIL out of 54

The two IBI=y skips are the hot-join tests, which are currently
environment-gated; the IBI=n skip is the ``test_ibi_not_compiled``
placeholder.  ``CONFIG_I3C_DUAL_BOARD_LOOPBACK_META_TESTS=y`` adds the two ``meta_*``
tests from ``test_harness.c`` to either configuration.

Because both boards run the same driver, a ``SUITE PASS`` is necessary
but not sufficient evidence for anything wire-format related: a
symmetric encode/decode bug passes loopback happily.

Future work
===========

* Coverage measurement against ``drivers/i3c/i3c_dw.c``,
  ``drivers/i3c/i3c_common.c`` and ``drivers/i3c/i3c_ccc.c``.

Known gaps (should be covered, currently are not):

* **DAT exhaustion.** Attaching more devices than the hardware Device
  Address Table can hold must return ``-ENOSPC`` from
  ``i3c_attach_i3c_device``.  Nothing here exercises that path, so the
  bounds checks on ``get_free_pos()`` are untested.  It needs no bus
  traffic — attach is software-only, and a descriptor with no address
  still consumes a slot — but it dirties real DAT entries, so it is
  better suited to a separate mini-suite where imperfect cleanup cannot
  cascade into the other tests.
* **Hot join.** ``test_ibi_hot_join`` and ``test_ibi_hot_join_stress``
  are skipped pending a root cause, so the HJ path is unverified.
* **GETMXDS against the descriptor cache.** Every other getter here
  cross-checks the wire against the field ``i3c_device_adv_info_get``
  cached (``bcr``, ``dcr``, ``data_length``).  ``GETMXDS`` cannot:
  ``i3c_device_adv_info_get`` reads the CCC into a local and never
  copies it into ``desc->data_speed``, which is only ever written by
  ``i3c_shell.c``.  The same function then gates its ``CRHDLY`` fetch on
  ``target->data_speed.maxwr``, so that branch is unreachable.  Until
  that is fixed upstream, ``test_ccc_getmxds`` can only check the two
  wire formats against each other.  The bench target also reports
  ``0x00``/``0x00``, which is spec-legal but leaves the speed-code
  bounds checks passing trivially.

Permanently uncovered (acceptable):

* HDR-DDR / HDR-TS — DW IP feature not present on every SoC variant.
* HDR-BT — not in the mainline DW core.
* Some PM corner cases requiring deep-sleep transitions.
