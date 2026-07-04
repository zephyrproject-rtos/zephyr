.. zephyr:code-sample:: assert
   :name: Assertions

   Enable verbose assertions for a single source file via an assert module while
   the master assertion switch is enabled.

Overview
********

This sample demonstrates configuring and using a named assertion module with
the API declared in :zephyr_file:`include/zephyr/sys/zassert.h`.

The sample's :file:`Kconfig` defines the ``MYMODULE`` assertion module by
sourcing :zephyr_file:`subsys/debug/zassert/Kconfig.template.assert`.
The template creates one Kconfig choice with these mutually exclusive Kconfig options:

* ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL_OFF=y`` selects ``ZASSERT_LEVEL_OFF``. Calls
  to :c:macro:`ZASSERT` for the module are compiled out.
* ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL_TERSE=y`` selects ``ZASSERT_LEVEL_TERSE``.
  Conditions are checked, but the location, message and arguments are not
  compiled in. A failure reports only a fixed ``ASSERTION FAIL`` banner.
* ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL_NORMAL=y`` selects ``ZASSERT_LEVEL_NORMAL``.
  Conditions are checked, but its formatting string, and arguments are not
  compiled in. A failure reports only its source location.
* ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL_VERBOSE=y`` selects
  ``ZASSERT_LEVEL_VERBOSE``. Conditions are checked and its formatting string and arguments are
  compiled in. A failure reports its source location and the optional formatted string.

The choice produces the non-assignable integer symbol
:kconfig:option:`CONFIG_ASSERT_MODULE_MYMODULE_LEVEL`, whose value is
``ZASSERT_LEVEL_OFF`` (0), ``ZASSERT_LEVEL_TERSE`` (1),
``ZASSERT_LEVEL_NORMAL`` (2), or ``ZASSERT_LEVEL_VERBOSE`` (3).
The choice defaults to ``ZASSERT_LEVEL_VERBOSE``.
This sample keeps that default in :file:`prj.conf` by selecting the verbose setting.
The master :kconfig:option:`CONFIG_ASSERT` setting must also be enabled, when it is
disabled, all module assertions are compiled out regardless of their configured
levels.

At file scope, ``ZASSERT_MODULE(MYMODULE)`` makes the derived ``MYMODULE`` level
the level used by :c:macro:`ZASSERT` calls in :file:`src/main.c`. The first call
has a true condition and produces no assertion output. The second has a false
condition, so the verbose failure output includes ``x == 3``, its source
location, and the formatted value of ``x``.
The default assertion post action then raises a fatal error, so execution does
not continue past the failing assertion.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/asserts
   :board: native_sim
   :goals: build run
   :compact:

Expected output:

.. code-block:: console

   Granular assert sample
   MYMODULE assert level = 3
   Passed module ZASSERT(1 == 1)
   Triggering a failing module ZASSERT() ...
   ASSERTION FAIL [x == 3] @ .../src/main.c:...
   x was 2, expected 3
