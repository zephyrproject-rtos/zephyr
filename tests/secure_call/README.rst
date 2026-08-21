Secure Call (``__secure_call``) Tests
#####################################

Overview
********

``__secure_call`` is a build-time code-generation pattern for crossing the
ARMv8-M TrustZone Non-Secure (NS) / Secure (S) boundary.  It mirrors Zephyr's
``__syscall`` mechanism: a developer decorates a function *declaration*, and the
build generates the boilerplate needed to call that function safely from the
Non-Secure world into the Secure world.

A developer writes::

    __secure_call int sc_add(int a, int b);

and the build generates, for each decorated declaration:

``z_secure_mrsh_<name>`` (Secure side)
    A ``cmse_nonsecure_entry`` veneer — the actual S-world entry point that NS
    code branches to across the boundary.

NS-side inline wrapper (``<zephyr/secure_calls/...>``)
    Calls the veneer, wrapped in ``z_secure_call_lock()`` /
    ``z_secure_call_unlock()`` so the NS scheduler cannot preempt while
    execution is in the Secure world.

The developer supplies two Secure-side functions per call:

``z_secure_vrfy_<name>``
    Validates any NS-supplied pointers (via ``Z_SECURE_MEMORY_READ`` /
    ``Z_SECURE_MEMORY_WRITE`` / ``Z_SECURE_VERIFY``) before they are touched,
    then calls the implementation.

``z_secure_impl_<name>``
    The actual Trusted-Execution-Environment (TEE) business logic.

Two Python scripts drive the generation:
``scripts/build/parse_secure_calls.py`` scans headers for ``__secure_call``
declarations, and ``scripts/build/gen_secure_calls.py`` emits the NS wrappers,
the Secure veneers, and the ID list.  ``CONFIG_APPLICATION_DEFINED_SYSCALL=y``
adds the application's source directory to the scan path so a project can
declare its own secure calls (see ``src/test_calls.h``).

The generated NS wrapper has two branches:

* **Dual-image (TrustZone)** — when the NS firmware is built against a Secure
  image (``CONFIG_ARM_FIRMWARE_USES_SECURE_CALLS=y``), the wrapper branches
  across the S/NS boundary through the generated veneer.
* **Single-image** — otherwise the ``#else`` branch simply calls
  ``z_secure_impl_<name>()`` directly.  This makes the same source portable to
  non-TrustZone targets and lets the wrappers be unit-tested on the host.

Three sample calls exercise the three argument/return shapes the generator must
handle:

.. list-table::
   :header-rows: 1
   :widths: 15 40 45

   * - Call
     - Signature
     - Shape
   * - ``sc_add``
     - ``int sc_add(int a, int b)``
     - scalar args, non-void return
   * - ``sc_fill``
     - ``void sc_fill(uint8_t *buf, size_t len)``
     - pointer + scalar args, void return
   * - ``sc_nop``
     - ``int sc_nop(void)``
     - zero args, non-void return

Test structure
**************

This directory holds three test scenarios plus a standalone code-generation
unit test.

``secure_call.gen`` (``./``)
    A single-image Ztest suite (``src/main.c``) built
    with the ``#else`` branch active.  Host-side ``z_secure_impl_*`` functions
    record their arguments so the tests can assert that arguments are forwarded
    unchanged and return values are propagated.  It also checks that
    ``z_secure_call_lock()``/``unlock()`` and the ``Z_SECURE_MEMORY_*`` /
    ``Z_SECURE_VERIFY`` handler macros compile and are no-ops in a non-Secure
    build.  Runs on ``native_sim`` and the QEMU Cortex-M targets.

``secure_call.tz`` (``tz/`` + ``tz_s/``)
    The real dual-image TrustZone integration test on **QEMU** (mps2/an521,
    Cortex-M33).  A sysbuild run builds the Secure image
    (``tz_s``) first — producing ``libentryveneers.a`` — then the Non-Secure
    image (``tz``) links against it.  The Secure image configures the SAU and
    boots NS; the NS image calls ``sc_add`` / ``sc_nop`` / ``sc_fill`` through
    the veneers and prints the results.  A ``pytest`` harness
    (``tz/pytest/test_tz_calls.py``) launches QEMU with both ELFs and checks the
    output.

``secure_call.tz.hw`` (``tz/`` + ``tz_s/``)
    The same dual-image test run on **hardware** (Infineon PSOC Edge E84,
    ``kit_pse84_*/…/m33/ns``), validated with a ``console`` harness.  See
    `Platform abstraction`_ below.

The code-generation scripts themselves are covered by
``scripts/tests/build/test_parse_gen.py`` (a plain ``pytest`` module), which
feeds sample headers through ``parse_secure_calls.py`` / ``gen_secure_calls.py``
and checks the emitted wrappers and marshalling code.

Directory layout
================

::

    tests/secure_call/
    ├── src/                 single-image Ztest suite (secure_call.gen)
    │   ├── main.c           host-side z_secure_impl_* + ZTEST cases
    │   └── test_calls.h     the three __secure_call declarations
    ├── tz/                  Non-Secure image (default sysbuild domain)
    │   ├── src/main.c       calls sc_add / sc_nop / sc_fill, prints results
    │   ├── sysbuild.cmake   builds tz_s first, derives the Secure board
    │   ├── pytest/          QEMU harness (secure_call.tz)
    │   └── boards/          per-board NS overlays / Kconfig fragments
    └── tz_s/                Secure image (secure_s sysbuild domain)
        ├── src/main.c       MPC/SAU config; enters NS via arch_secure_domain_swap()
        ├── src/secure_impl.c   z_secure_vrfy_* + z_secure_impl_*
        ├── Kconfig          NS-image-base symbols (set per board fragment)
        └── boards/          per-board Secure overlays / Kconfig fragments

Building and running
********************

Single-image functional test (host / QEMU):

.. code-block:: shell

    west twister -T tests/secure_call -p native_sim -v

Code-generation unit test (standalone pytest):

.. code-block:: shell

    pytest scripts/tests/build/test_parse_gen.py

TrustZone dual-image test on QEMU (mps2/an521):

.. code-block:: shell

    west twister -T tests/secure_call/tz -p mps2/an521/cpu0/ns -v

TrustZone dual-image test on hardware (PSOC Edge E84 AI kit):

.. code-block:: shell

    west twister -T tests/secure_call/tz \
        -p kit_pse84_ai/pse846gps2dbzc4a/m33/ns \
        --device-testing --hardware-map <hardware-map.yml> -v

A passing run prints, from the Non-Secure image::

    NS: starting secure call integration test
    SC_ADD: 7
    SC_NOP: 42
    SC_FILL: OK
    NS: test complete

Platform abstraction
********************

The Secure and Non-Secure test sources (``tz_s/src/main.c``, ``tz/src/main.c``)
are platform-agnostic: no per-SoC ``#if``, no magic addresses, no raw register
pokes.  Everything hardware-specific lives in reusable arch, SoC, DT, and driver
layers, so the same test source runs on QEMU (mps2/an521) and PSOC Edge E84:

* **Address attribution (SAU)** — configured by ``arch_security_partition_*``
  (``<zephyr/arch/security_partition.h>``) from a ``zephyr,security-partition``
  devicetree node in each Secure board overlay.
* **Bus-level protection (MPC/PPC)** — configured by the ``mpc``/``ppc`` driver
  classes (``mpc_configure_all()`` / ``ppc_configure_ns_all()``) from devicetree.
* **Secure→Non-Secure entry** — ``arch_secure_domain_swap()``
  (``<zephyr/arch/secure_domain.h>``), with a weak backend a SoC can override.

Hardware notes (PSOC Edge E84)
==============================

The PSOC Edge E84 silicon/topology quirks are handled entirely in those layers:

* **S→NS entry (BXNS INVTRAN deviation).** ``BXNS`` raises ``INVTRAN`` on this
  silicon even when every ARM-architectural precondition is met.  The arch
  provides two ``arch_secure_domain_swap()`` entry flows selected by
  :kconfig:option:`CONFIG_ARM_SECURE_DOMAIN_ENTRY_SVC`: the default ``BXNS``
  branch (QEMU mps2/an521) and, for this silicon, an SVC exception-return flow
  — a synthesised NS exception frame + ``EXC_RETURN``.  The PSE84 SoC selects
  the SVC flow and additionally overrides only the vector-table write to shadow
  the MXCM33 base register.

* **Shared console SCB.** The console UART is shared between the Secure and
  Non-Secure images.  The Secure image owns and fully configures it (pins,
  peripheral clock, ``Cy_SCB_UART_Init``) and, as its *last* action before the
  hand-off, grants the Non-Secure protection context access to the console SCB
  via the PPC driver (``ppc_configure_ns_all()``).  Because the hardware is
  already brought up, the Non-Secure image must **not** re-initialise it: the
  ``clock_control`` and ``uart`` drivers skip their hardware bring-up when
  ``CONFIG_ARM_FIRMWARE_USES_SECURE_CALLS=y`` and use the Secure-configured SCB
  directly.  Re-initialising it from NS either faults on a Secure-owned register
  or resets the live console.
