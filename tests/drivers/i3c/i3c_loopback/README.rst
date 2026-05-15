.. zephyr:code-sample:: i3c_loopback
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

Two Zephyr applications live in this directory:

``i3c_controller/``
    Runs on **Board A**.  Builds with ``CONFIG_I3C_CONTROLLER_ROLE_ONLY=y``
    and contains the actual ``ZTEST_SUITE``.  Drives the bus, issues
    CCCs and SDR transfers, attaches/detaches devices, enables IBIs.

``i3c_target/``
    Runs on **Board B**.  Builds with ``CONFIG_I3C_TARGET_ROLE_ONLY=y``.
    A reactive harness — registers I3C target callbacks and reacts to a
    UART sync channel from Board A so the controller-side ZTESTs can
    pre-stage read data, request IBIs, verify writes, etc.

A small line-oriented ASCII protocol (``common/sync_proto.h``) is shared
between the two apps over a UART channel that is **not** the I3C bus, so
the test runner on Board A can deterministically coordinate Board B
(``HELLO`` / ``READY`` / ``ACK`` / ``PASS`` / ``FAIL`` / ``IBI`` /
``STAGE_READ`` / ``VERIFY`` / ``RESET``).

Required board configuration
============================

Each board needs a ``boards/<board>.overlay`` (and optionally a
``boards/<board>.conf``) under both ``i3c_controller/`` and
``i3c_target/`` that provides:

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
       -b <board> tests/drivers/i3c/i3c_loopback/i3c_controller

   # Target (Board B)
   west build -p always -d build_loopback_b \
       -b <board> tests/drivers/i3c/i3c_loopback/i3c_target

Twister discovery::

   twister -T tests/drivers/i3c/i3c_loopback -p <board>

Both ``testcase.yaml`` files use ``harness: console`` with
``fixture: i3c_loopback_pair`` — the fixture must be declared on the
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
then ``FAIL`` cleanly via ``I3C_LOOPBACK_REQUIRE_TARGET_DA``.

A successful run prints ``SUITE PASS`` followed by ``PROJECT EXECUTION
SUCCESSFUL``.

Test files
==========

All ZTESTs live under ``i3c_controller/src/``:

* ``test_attach.c``  — ``i3c_attach_*`` / ``i3c_detach_*`` API.
* ``test_ccc.c``     — full CCC set: ``ENEC``/``DISEC``, ``ENTAS``,
  ``GETBCR``/``GETDCR``/``GETPID``/``GETSTATUS``,
  ``GETMRL``/``GETMWL``, ``SETMRL``/``SETMWL``, ``GETMXDS``,
  ``RSTDAA``, ``SETAASA``, ``SETNEWDA``.
* ``test_config.c``  — runtime SCL frequency change, ``i3c_config_get``.
* ``test_daa.c``     — ``i3c_bus_init`` DAA, attach-multiple,
  ``ENTDAA``-only stress (100 iterations).
* ``test_ibi.c``     — TIR (with / without mandatory byte, extended
  payload, back-to-back), enable/disable, hot-join, controller-role
  request.
* ``test_pm.c``      — suspend / resume.
* ``test_recovery.c``— ``i3c_recover_bus`` API, recovery after
  ``ADDRESS_NACK``, recovery after invalid CCC.
* ``test_xfer.c``    — single-byte / max-payload (256 B) / FIFO
  boundary / multi-message / write-then-read repeated-start /
  back-to-back stress.

Test results
============

On the validated bench (both boards, controller suite):
``SUITE PASS`` (45 PASS / 3 SKIP / 0 FAIL out of 48 — the 3 skips are
environment-gated tests like hot-join that need bench-specific wiring).

Future work
===========

* Coverage measurement against ``drivers/i3c/i3c_dw.c``,
  ``drivers/i3c/i3c_common.c`` and ``drivers/i3c/i3c_ccc.c``.

Permanently uncovered (acceptable):

* HDR-DDR / HDR-TS — DW IP feature not present on every SoC variant.
* HDR-BT — not in the mainline DW core.
* Some PM corner cases requiring deep-sleep transitions.
