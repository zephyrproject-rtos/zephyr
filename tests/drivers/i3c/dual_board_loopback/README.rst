.. SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
.. SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
..
.. SPDX-License-Identifier: Apache-2.0

I3C two-board loopback tests
############################

A hardware regression suite for the I3C subsystem and the Synopsys DesignWare I3C driver
(``drivers/i3c/i3c_dw.c``), run across two physical boards wired as a single I3C bus.

These tests need real hardware. There is no emulated backend: the I3C bus behaviour that
matters here (arbitration, in-band interrupts, dynamic address assignment, NACK and abort
recovery) is only observable between two independent devices. A single board cannot drive
and observe the same bus transaction, so the suite puts the assertions on one board and a
stimulus responder on the other.

Layout
******

.. list-table::
   :widths: 34 12 26 28
   :header-rows: 1

   * - Application
     - Runs on
     - Role Kconfig
     - Contains the ZTESTs
   * - ``controller_driven/i3c_controller``
     - Board A
     - ``I3C_CONTROLLER_ROLE_ONLY``
     - yes
   * - ``controller_driven/i3c_target``
     - Board B
     - ``I3C_TARGET_ROLE_ONLY``
     - no, responder

``controller_driven/`` asserts on what a controller can observe: CCC round trips, DAA, SDR
transfers, IBI reception, bus recovery, runtime reconfiguration. Target-side state that a
controller cannot see, such as which callbacks fired in which order, is a separate axis and
is not covered here.

Both apps are built role-only on purpose. A dual-role build lets the controller code paths
mask a target-side defect, and the reverse, so each side is compiled with only the half under
test.

``common/`` holds the pieces the two applications share: ``sync_proto.h`` (the out-of-band
coordination protocol) and ``test_identity.h`` (the agreed PID, BCR, DCR and addresses).

Coordination channel
********************

The boards coordinate over a UART that is **not** the I3C bus, using a line-oriented ASCII
protocol in ``common/sync_proto.h``.

The board running the ZTESTs is the one that sends requests; its peer answers. Requests cover
staging read data, forcing resets, dumping target registers and asking for an IBI; replies
carry acknowledgements, register dumps and IBI notifications.

Specification basis and portability
***********************************

Every case cites the clause it is derived from, in the comment directly above it:

.. code-block:: c

   /*
    * A target with no dynamic address can request one by hot join.
    *
    * Source: MIPI I3C Basic v1.0 5.1.9.3.1 (ENHJ/DISHJ).
    */

The suite was written against **MIPI I3C Basic v1.0** for protocol behaviour, and against the
**Synopsys DesignWare MIPI I3C databook** where an expectation depends on how the IP behaves
rather than on what the bus requires. Where the two disagree, the comment says which one it is
following.

Most of what is asserted is protocol-level and should hold for any conforming I3C controller
or target driver. Some of it will not, and it is worth knowing which parts before pointing
this suite at different hardware:

Resource-derived boundaries
   Several transfer cases are sized from the DW FIFO depths. On different hardware they still
   pass or fail honestly, but they stop sitting exactly on the boundary they were written to
   probe.

Reset semantics
   Whether ``SOFT_RST`` clears ``DEVICE_ADDR`` is a DW variant difference, and is a Kconfig
   option here for that reason.

Integration wrappers
   Clock or register gating around the IP is vendor-specific and is likewise behind a Kconfig
   option.

The TIR payload cap
   A mandatory byte plus four data bytes, which reflects what this IP accepts.

None of this is hidden: because each case names its source, re-basing on a later revision of
either document is a matter of re-reading the cited clause and adjusting that one case. That
is the main reason the citations are there.

Hardware requirements
*********************

Two boards of the same type, wired::

   I3C bus                       Coordination UART
     A.SCL  ----  B.SCL            A.TX  ----  B.RX
     A.SDA  ----  B.SDA            A.RX  ----  B.TX
     A.GND  ----  B.GND            A.GND ----  B.GND

SCL and SDA need pull-ups, either external or from configurable SoC pads.

The suite assumes the two boards are alone on the bus. A third device wired to the same pads
will join dynamic address assignment and take an address, which shifts the addresses the
tests expect. Where such a device cannot be removed or isolated, be aware that it may also
change behaviour between runs.

Each application needs a ``boards/<board>.overlay`` (and optionally a ``boards/<board>.conf``)
providing:

* an ``snps,designware-i3c``-compatible controller node aliased as ``test-i3c``;
* a ``zephyr,sync-uart`` chosen node pointing at the cross-wired UART;
* optionally ``la-marker-gpios`` on the target, toggled on callback entry so a logic analyser
  can trigger on target-side events.

Both ``tests.yaml`` files filter on exactly those two things::

   filter: dt_alias_exists("test-i3c") and dt_chosen_enabled("zephyr,sync-uart")

Building and running
********************

Build each side independently:

.. code-block:: bash

   west build -p always -d build_cd_a -b <board> \
       tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_controller

   west build -p always -d build_cd_b -b <board> \
       tests/drivers/i3c/dual_board_loopback/controller_driven/i3c_target

Flashing order
==============

.. code-block:: bash

   west flash -d build_cd_b --dev-id <serial-B>
   west flash -d build_cd_a --dev-id <serial-A>

The flashing order does not matter. Whichever board is flashed first runs against the stale
image still on its peer, so the start of its console is expected to contain a failed run.

Board A holds the ZTESTs, so it is Board A that waits. It greets for up to a minute, and then
waits for Board B's ``BOOT`` announcements to stop before enumerating, which keeps it from
binding to an image that is about to be reset again. Configurations whose flashing resets a
board twice, such as the sysbuild CM55 ones, are covered by the same wait.

Reading the result
==================

Capture Board A's console; it is the side that holds the ZTESTs. A clean run ends with the
ztest summary and ``PROJECT EXECUTION SUCCESSFUL``.

One ``<err> i3c_dw: Invalid slave device`` during the run is expected. It is the negative
control in the unaddressed-traffic case, which deliberately writes to an address no device
holds.

Kconfig
*******

Required
========

Set by each ``prj.conf``; listed here because changing them changes what is under test.

.. list-table::
   :widths: 38 24 38
   :header-rows: 1

   * - Symbol
     - Where
     - Why
   * - ``CONFIG_I3C=y``
     - both apps
     - the subsystem under test
   * - ``CONFIG_I3C_CONTROLLER_ROLE_ONLY=y``
     - ``i3c_controller``
     - keeps target paths out of the build
   * - ``CONFIG_I3C_TARGET_ROLE_ONLY=y``
     - ``i3c_target``
     - keeps controller paths out of the build
   * - ``CONFIG_I3C_USE_IBI=y``
     - both apps
     - default; see the ``no_ibi`` scenario for the exception
   * - ``CONFIG_SERIAL=y``
     - both apps
     - the coordination UART
   * - ``CONFIG_ZTEST=y``
     - asserting side only
     - the responder is not a test application

Do not set
==========

``CONFIG_UART_INTERRUPT_DRIVEN``
   The coordination UART is poll-mode only. With the interrupt-driven API enabled, the UART
   interrupt handler races ``poll_in``/``poll_out`` and wedges subsequent I3C traffic. The
   failure appears later and elsewhere than the cause, so this is the single most expensive
   configuration mistake available here.

Both role symbols at once
   A dual-role build is a valid driver configuration but it is not what this suite measures;
   enabling both lets one role's code path compensate for the other's defect.

Scenario-specific
=================

* ``no_ibi`` sets ``CONFIG_I3C_USE_IBI=n`` on the controller. Controller-role-only with IBI
  disabled is a supported driver configuration that nothing else in the tree builds, which is
  why it is a scenario rather than a manual experiment. The IBI cases compile out and are
  reported as a single ``test_ibi_not_compiled`` placeholder.

Suite options
=============

Bench and integration differences. Each option is declared by the application that consumes
it, named in parentheses below. The defaults describe one particular bench, two boards alone
on the bus on a stock DW integration, so treat them as an example to be matched to your
hardware rather than as values that are correct everywhere.

``I3C_DUAL_BOARD_LOOPBACK_META_TESTS`` (``controller_driven/i3c_controller``)
   Builds tests that exercise the fixture itself rather than the driver. They exist because a
   silently broken recovery path once turned one target hiccup into a suite-wide cascade.

``I3C_DUAL_BOARD_LOOPBACK_TARGET_VENDOR_WRAPPER_GATE`` (``controller_driven/i3c_target``)
   For integrations that gate the protocol clock behind a vendor register which must be
   reopened before ``SOFT_RST`` can self-clear. The register is addressed by
   ``..._WRAPPER_GATE_OFFSET`` from the controller base and written with
   ``..._WRAPPER_GATE_VALUE``; both are specific to the integration.

``I3C_DUAL_BOARD_LOOPBACK_TARGET_SOFT_RST_CLEARS_DEVICE_ADDR`` (``controller_driven/i3c_target``)
   Defaults on, matching DW variants where ``SOFT_RST`` clears ``DEVICE_ADDR``. Turn it off
   where the dynamic address survives the reset; the target then clears it itself rather than
   entering enumeration believing it already holds an address.

Scenarios and expected results
******************************

Skips are expected and meaningful; they are not failures waiting to happen. Cases that cannot
apply to a configuration skip at runtime rather than being compiled out, so twister still
reports them and a case cannot silently disappear from a build.

.. list-table::
   :widths: 40 8 8 8 36
   :header-rows: 1

   * - Scenario
     - Pass
     - Fail
     - Skip
     - Skipped cases
   * - ``controller_driven.i3c_controller``
     - 61
     - 0
     - 2
     - the two hot-join cases
   * - ``controller_driven.i3c_controller.no_ibi``
     - 55
     - 0
     - 1
     - ``test_ibi_not_compiled``

CCC coverage
************

Exercised, through the ``i3c_ccc_do_*`` API:

.. list-table::
   :widths: 44 56
   :header-rows: 1

   * - CCC
     - Form
   * - ``ENEC`` / ``DISEC``
     - directed and broadcast
   * - ``ENTDAA``
     - via ``i3c_bus_init()`` / ``i3c_do_daa()``
   * - ``ENTAS0``-``ENTAS3``
     - directed; ``ENTAS0`` also broadcast
   * - ``GETBCR``, ``GETDCR``, ``GETPID``
     - directed
   * - ``GETSTATUS``
     - format 1
   * - ``GETMXDS``
     - formats 1 and 2, when the target advertises ``BCR[0]``
   * - ``GETMRL`` / ``GETMWL``, ``SETMRL`` / ``SETMWL``
     - directed
   * - ``RSTDAA``
     - broadcast
   * - ``SETAASA``
     - broadcast
   * - ``SETDASA``, ``SETNEWDA``
     - directed

Known gaps, and why:

``RSTACT``, directed and broadcast
   The DW driver implements no target-reset action, so the target side has nothing to assert
   against. Needs driver support first.

   The target block this suite was written against does not implement ``RSTACT`` either: a
   directed ``RSTACT`` (``0x9A``) is NACKed, and a broadcast one (``0x2A``) cannot be NACKed
   by a single target at all, so its defining byte is clocked in and left in the target's RX
   FIFO with no response descriptor. The suite therefore leaves ``CONFIG_I3C_INIT_RSTACT``
   at its default ``y`` on the controller side: ``i3c_bus_init()`` sends that broadcast, and
   it is what exercises the driver's RX-residue drain at dynamic address assignment. Setting
   the symbol to ``n`` makes the residue disappear and silently drops that coverage.

``GETACCCR``, ``DEFTGTS``, ``SETBUSCON``
   Controller handoff and secondary-controller territory. Both apps are built role-only, so
   there is no second controller to hand off to. This is a separate test axis, not an
   oversight.

``GETCAPS``
   Optional, and the target's advertised capabilities are already cross-checked through
   ``GETBCR``/``GETDCR`` and the target-side register dump.

``GETVENDOR`` / ``SETVENDOR``, and the defining-byte forms
   Vendor-defined by construction, so there is no portable oracle for a pass.

``ENTTM``
   Puts the bus into a test mode with no defined exit on this bench.

``SETMRL_ALL`` / ``SETMWL_ALL``
   The broadcast forms reach the same driver code as the directed forms already covered, minus
   the target index.

Directed ``RSTDAA``
   Only the broadcast form is exercised. Directed ``RSTDAA`` should not be used, per MIPI spec.

``GETSTATUS`` format 2, ``GETMXDS`` format 3
   The covered formats already exercise the driver's CCC length handling. The remaining
   formats take a defining byte that the target harness does not stage, so a pass would only
   show that the controller transmitted it.

Why the hot-join cases are skipped
**********************************

Two cases are implemented and then skipped immediately: ``test_ibi_hot_join`` and
``test_ibi_hot_join_stress``.

These are all the hot join tests. I haven't been able to verify hot join on the DW IP,
so I can't say for certain if the tests written are correct / can pass.

Other expected skips
********************

* The ``GETMXDS`` case skips unless the target advertises the maximum-data-speed-limit bit in
  its BCR, because the CCC is optional otherwise.
* With ``CONFIG_I3C_USE_IBI=n``, the IBI cases cannot be built at all, and a single
  ``test_ibi_not_compiled`` placeholder is reported in their place.

Runtime ``ztest_test_skip()`` is preferred over ``#ifdef`` throughout. A compiled-out test is
invisible in the report; a skipped one is counted, named, and still has to compile, so it
cannot rot unnoticed while the configuration that excludes it is the one being built.
