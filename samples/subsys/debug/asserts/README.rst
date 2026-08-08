.. zephyr:code-sample:: assert
   :name: Assertions

   Enable assertions for a single source file via an assert module while global
   assertions are disabled.

Overview
********

This sample demonstrates the module-aware assertion API declared in
:zephyr_file:`include/zephyr/sys/zassert.h`:

* :c:macro:`ZASSERT_MODULE` selects the assertion *module* used by a source
  file. The module's default level comes from the ``CONFIG_ASSERT_MODULE_<MODULE>_LEVEL``
  Kconfig symbol (here :kconfig:option:`CONFIG_ASSERT_MODULE_MYMODULE_LEVEL`), or from an
  optional explicit level argument (``ZASSERT_OFF``, ``ZASSERT_ON`` or
  ``ZASSERT_VERBOSE``).
* :c:macro:`ZASSERT` behaves like ``__ASSERT`` but is gated by the file's
  registered module level instead of the global :kconfig:option:`CONFIG_ASSERT_LEVEL`.
  At ``ZASSERT_ON`` the message and its arguments are not compiled in; at
  ``ZASSERT_VERBOSE`` they are.

The sample raises ``MYMODULE`` to ``2`` (``ZASSERT_VERBOSE``), so the file's
``ZASSERT()`` is active and reports a failing check together with its message.
The failure path is marked unreachable, so the sample installs a custom
:c:func:`zassert_post_action` that ``longjmp()``\ s back into ``main()`` instead
of returning, letting the sample continue after the failing assertion.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/asserts
   :board: native_sim
   :goals: build run
   :compact:

Expected output:

.. code-block:: console

   assert sample
   MYMODULE assert level        = 2
   Passed module ZASSERT(1 == 1)
   Triggering a failing module ZASSERT() ...
   ASSERTION FAIL [x == 3] @ .../src/main.c:...
   x was 2, expected 3
   (assertion caught[.../src/main.c:...]; recovering)
   Continued after the failing ZASSERT()
