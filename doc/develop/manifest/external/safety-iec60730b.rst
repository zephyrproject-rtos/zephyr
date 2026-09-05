.. _external_module_safety_iec60730b:

IEC 60730 Class B Safety Test Subsystem
#######################################

Introduction
************

`safety_iec60730b`_ is a Zephyr :ref:`module <modules>` providing a test subsystem that runs
the IEC 60730 Class B safety self-tests from a Zephyr application.

`IEC 60730-1`_ is an international standard defining safety requirements for automatic electrical
controls used in household appliances and similar equipment. Annex H Class B describes the
software fault/error detection measures a control has to implement to detect random hardware
faults that could otherwise lead to a dangerous malfunction.

The subsystem exposes those "continuously performed" self-tests through a single, vendor-neutral
API, integrates them with Zephyr through Kconfig and the device driver model, and delegates
the actual test execution to a selectable backend. The default backend on NXP SoCs is the
`NXP IEC 60730 Class B Safety Library`_.

The following Class B test routines are exposed, each individually selectable through Kconfig:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Test
     - Kconfig symbol
     - Fault detected
   * - CPU registers
     - ``CONFIG_IEC60730B_TEST_CPU``
     - Stuck-at faults in the core registers
   * - FPU registers
     - ``CONFIG_IEC60730B_TEST_FPU``
     - Stuck-at faults in the floating-point registers
   * - Program counter
     - ``CONFIG_IEC60730B_TEST_PC``
     - Program counter corruption, invalid execution flow
   * - RAM
     - ``CONFIG_IEC60730B_TEST_RAM``
     - RAM cell faults, using March C / March X algorithms
   * - Flash / ROM
     - ``CONFIG_IEC60730B_TEST_FLASH``
     - Invariable memory corruption, using CRC16 / CRC32
   * - Stack
     - ``CONFIG_IEC60730B_TEST_STACK``
     - Stack overflow and underflow
   * - Clock
     - ``CONFIG_IEC60730B_TEST_CLOCK``
     - System clock drift, measured against an independent counter
   * - Analog I/O
     - ``CONFIG_IEC60730B_TEST_AIO``
     - ADC signal path faults, verified against known internal references
   * - Digital I/O
     - ``CONFIG_IEC60730B_TEST_DIO``
     - GPIO stuck-at faults and short circuits
   * - Watchdog
     - ``CONFIG_IEC60730B_TEST_WDOG``
     - Watchdog timeout and reset generation

Behind that API, the test routines come from one of three interchangeable Hardware Abstraction
Layer (HAL) backends:

* **NXP HAL** (``CONFIG_IEC60730B_HAL_NXP``) calls the pre-certified NXP IEC 60730 Class B
  bare-metal safety library shipped in the same repository. It is selected by default on NXP SoCs
  and is backed by a certified implementation.
* **Zephyr HAL** (``CONFIG_IEC60730B_HAL_ZEPHYR``) implements the same tests on top of standard
  Zephyr driver APIs, which makes the subsystem usable on any Zephyr-supported platform. It is
  the default on non-NXP SoCs.
* **No HAL** (``CONFIG_IEC60730B_HAL_NONE``) builds only the weak stubs, so every test returns
  ``IEC60730B_TEST_NOT_SUPPORTED``. Use it as a starting point for a custom backend: implement the
  functions declared in the public header and they override the weak ones at link time.

.. warning::

   The Zephyr HAL is EXPERIMENTAL. It does not use any vendor-certified library, and none
   of its code has been certified against IEC 60730 Class B. Users
   are solely responsible for evaluating, validating and certifying all code for their specific
   application and target platform before use in any safety-critical product.

The test subsystem, its HAL backends, the sample and the module metadata (everything under
``zephyr/``) are licensed under the Apache-2.0 license. The NXP safety library used as a backend
(everything under ``source/``) is distributed under the
*LA_OPT_Online Code Hosting NXP_Software_License*; see ``IEC60730-LICENSE.txt`` in the repository
for the full text.

Usage with Zephyr
*****************

Adding the module to an existing workspace
==========================================

Add the module as a West project in your ``west.yml`` manifest, or pull it in with a submanifest
(for example :file:`zephyr/submanifests/iec60730b.yaml`) with the following content:

.. code-block:: yaml

   manifest:
     projects:
       - name: safety_iec60730b
         url: https://github.com/nxp-mcuxpresso/mcux-safety-iec60730b
         revision: main
         path: modules/safety/iec60730b # adjust the path as needed

Then fetch it:

.. code-block:: console

   west update safety_iec60730b

Creating a freestanding workspace
=================================

The repository also ships its own manifest, which pulls in Zephyr itself and only the HALs the
module needs:

.. code-block:: console

   west init -m https://github.com/nxp-mcuxpresso/mcux-safety-iec60730b <workspace>
   cd <workspace>
   west update

Configuration
=============

Enable the test subsystem in your application :file:`prj.conf`:

.. code-block:: cfg

   CONFIG_IEC60730B=y

All test routines are enabled by default. Disable the ones you do not need, and override the
automatic HAL selection if required.

Application interface
=====================

The public API is declared in ``iec60730b_test.h``, which the module adds to the application
include path. Include it and call the test functions directly, typically split into start-up
tests executed once before the application starts, and run-time tests executed periodically from
a dedicated safety thread or interrupt. Each function returns ``0`` on success, a negative error code on a
detected fault, or ``IEC60730B_TEST_NOT_SUPPORTED`` when the test is disabled or unavailable on
the target.

Sample application
==================

The ``safety`` sample under ``zephyr/samples/safety/`` is the reference integration. It runs the
complete set of start-up and run-time tests, reports each result on the console, feeds a Zephyr
task watchdog channel and blinks an LED while the safety thread is alive.

Build and flash it for one of the supported boards from the workspace root:

.. code-block:: console

   west build -p -b frdm_mcxa266 modules/safety/iec60730b/zephyr/samples/safety
   west flash

The sample deliberately lets the watchdog expire as part of the watchdog test, so the board resets
once and the boot banner and start-up test results appear twice.

References
**********

.. target-notes::

.. _safety_iec60730b:
   https://github.com/nxp-mcuxpresso/mcux-safety-iec60730b

.. _IEC 60730-1:
   https://webstore.iec.ch/publication/66089

.. _NXP IEC 60730 Class B Safety Library:
   https://www.nxp.com/applications/technologies/functional-safety/iec-60730-safety-standard-for-household-appliances:APIEC60730
