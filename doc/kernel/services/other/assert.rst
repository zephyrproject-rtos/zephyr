.. _assert:

Assertions
##########

Zephyr provides several assertion facilities for catching programming errors:

- **Runtime assertions** check a condition while the code is running and, if it
  fails, induce a :ref:`fatal error <fatal>`. The recommended API is the
  module-aware ``ZASSERT()`` macro; the older ``__ASSERT()`` macro is now a thin
  compatibility layer on top of it and is deprecated.
- **Build assertions** (``BUILD_ASSERT()``) are evaluated entirely at
  compile-time and always checked.

.. note::

   The runtime ``ZASSERT()`` macro documented here is unrelated to the lowercase ``zassert_*``
   macros (``zassert_true()``, ``zassert_equal()``, ...) provided by the
   :ref:`Ztest <test-framework>` framework.
   The ``zassert_*`` macros report test failures, the ``ZASSERT()``
   macro raises a fatal error when a programming error is detected.

Runtime Assertions
******************

ZASSERT()
=========

The module-aware assertion API is declared in :zephyr_file:`include/zephyr/sys/zassert.h`.
Each source file opts into an assertion *module* whose level is a compile-time constant. Because the
level is known at compile time, the compiler can optimize the footprint of the assertion code based
on the associated assertion level of the module the assert belongs to.

Assertion Levels
----------------

Every module resolves to one of three levels from :c:enum:`zassert_level`:

- ``ZASSERT_OFF`` -- assertions are compiled out entirely.
- ``ZASSERT_ON`` -- assertions are checked; on failure only the location is
  reported (``ASSERTION FAIL @ file:line``). The condition, message and
  arguments are not compiled in.
- ``ZASSERT_VERBOSE`` -- assertions are checked; on failure the stringified
  condition, location and optional message are reported.

:kconfig:option:`CONFIG_ASSERT` is the master switch.
When it is disabled, every module is forced to ``ZASSERT_OFF`` and all ``ZASSERT()`` /
``ZASSERT_MODULE()`` usage compiles to nothing, regardless of any module's configured level.

Selecting a Module
------------------

Place :c:macro:`ZASSERT_MODULE` once at file scope, before any use of
:c:macro:`ZASSERT` in the translation unit:

.. code-block:: c

   #include <zephyr/sys/zassert.h>

   ZASSERT_MODULE(MYMODULE);

The module name is an UPPERCASE identifier. Its default level is taken from the
Kconfig symbol ``CONFIG_ASSERT_MODULE_<module>_LEVEL`` (here ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL``).
A file may override the module default by passing an explicit level as a second argument, for example
``ZASSERT_MODULE(MYMODULE, ZASSERT_VERBOSE)``.

Once a module is selected, use :c:macro:`ZASSERT` like a conditional check with an optional
:c:func:`printf`-like message:

.. code-block:: c

   ZASSERT(x == 3, "x was %d, expected 3", x);

If the condition is false and the module's level is at least ``ZASSERT_ON``, a fatal error is raised.
The message and its arguments are only compiled in and printed at ``ZASSERT_VERBOSE``.

For headers and inline functions, avoid ``ZASSERT_MODULE()`` at file scope, as the
selection would leak into every file that includes the header. Use one of the
following instead.

Place :c:macro:`ZASSERT_MODULE` inside the function body. The selection is then
block-scoped and does not escape to the includer, and plain :c:macro:`ZASSERT`
works within that function:

.. code-block:: c

   static inline void f(void *ptr)
   {
           ZASSERT_MODULE(MYMODULE);

           ZASSERT(ptr != NULL, "ptr must not be NULL");
   }

Alternatively, use one of the stateless forms, which select the level at the
call site and declare nothing in scope. :c:macro:`ZASSERT_M` takes a Kconfig
module:

.. code-block:: c

   ZASSERT_M(MYMODULE, ptr != NULL, "ptr must not be NULL");

while :c:macro:`ZASSERT_L` takes an explicit level (``ZASSERT_ON`` or
``ZASSERT_VERBOSE``) for a single assertion that does not belong to a module:

.. code-block:: c

   ZASSERT_L(ZASSERT_VERBOSE, ptr != NULL, "ptr must not be NULL");

.. note::

   A few rules apply to ``ZASSERT()`` and its file-scope module:

   - ``ZASSERT_MODULE()`` must appear before the first ``ZASSERT()`` in the
     translation unit, and only one module may be selected per file.
   - Using ``ZASSERT()`` with no module in scope is a compile error. Select a
     module first, or use the stateless ``ZASSERT_M()`` / ``ZASSERT_L()`` forms.
   - :kconfig:option:`CONFIG_ASSERT` remains the master switch: when it is
     disabled the module level is forced to ``ZASSERT_OFF`` regardless of the
     configured level.


Defining a Module's Kconfig Level
---------------------------------

The ``CONFIG_ASSERT_MODULE_<module>_LEVEL`` symbol is generated from the template
:zephyr_file:`lib/os/zassert/Kconfig.template.assert`. Source it from a Kconfig
file, setting the module name and a human-readable description first:

.. code-block:: kconfig

   module = MYMODULE
   module-str = the MYMODULE assert module
   source "lib/os/zassert/Kconfig.template.assert"

This produces a user-facing ``Off`` / ``On`` / ``Verbose`` choice and the derived, non-assignable
integer symbol ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL`` consumed by ``ZASSERT_MODULE(MYMODULE)``.
The choice defaults to ``On`` and stays overridable from :file:`prj.conf`.

Example
-------

The :zephyr:code-sample:`assert` sample demonstrates enabling
verbose assertions for a single file while the master assertion switch is enabled.
A condensed version:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/sys/zassert.h>

   ZASSERT_MODULE(MYMODULE);

   int main(void)
   {
           int x = 2;

           ZASSERT(x == 3, "x was %d, expected 3", x);

           return 0;
   }

With ``CONFIG_ASSERT_MODULE_MYMODULE_LEVEL_VERBOSE=y`` the failing check produces:

.. code-block:: none

   ASSERTION FAIL [x == 3] @ .../src/main.c:...
   x was 2, expected 3

Customizing the Failure Behavior
--------------------------------

The entire assertion cold path is consolidated into a small set of weak,
overridable functions declared in :zephyr_file:`include/zephyr/sys/zassert.h`
and implemented in :zephyr_file:`lib/os/zassert/zassert.c`:

- :c:func:`zassert_fail` reports a failed assertion (location, and when a
  message is present the message and its arguments) and then invokes
  :c:func:`zassert_post_action`. Overriding it is the single surface for
  capturing or redirecting the whole assertion output.
- :c:func:`zassert_post_action` takes the terminal action. The default
  implementation invokes :c:func:`k_oops` if the failing thread was running in
  user mode, and :c:func:`k_panic` otherwise.
- :c:func:`zassert_vprint` is the single primitive through which all assertion
  text flows. Override it to capture or redirect every assertion message from
  one place. :c:func:`zassert_print` is a variadic convenience wrapper around
  it, used by the legacy ``__ASSERT_PRINT()`` compatibility shims.

When :kconfig:option:`CONFIG_ASSERT_TEST` is enabled, the post action handler is
allowed to return (rather than abort) so that tests can validate assertion
behavior by installing a custom hook.


Build Assertions
****************

Zephyr provides a macro for performing build-time assertion checks.
It is evaluated completely at compile-time and always checked.

BUILD_ASSERT()
==============

This has the same semantics as C's ``_Static_assert`` or C++'s
``static_assert``. If the evaluation fails, a build error will be generated by
the compiler. If the compiler supports it, the provided message will be printed
to provide further context.

Unlike ``__ASSERT()``, the message must be a static string, without
:c:func:`printf()`-like format codes or extra arguments.

For example, suppose this check fails:

.. code-block:: c

	BUILD_ASSERT(FOO == 2000, "Invalid value of FOO");

With GCC, the output resembles:

.. code-block:: none

	tests/kernel/fatal/src/main.c: In function 'test_main':
	include/zephyr/toolchain/gcc.h:28:37: error: static assertion failed: "Invalid value of FOO"
	 #define BUILD_ASSERT(EXPR, MSG) _Static_assert(EXPR, "" MSG)
					 ^~~~~~~~~~~~~~
	tests/kernel/fatal/src/main.c:370:2: note: in expansion of macro 'BUILD_ASSERT'
	  BUILD_ASSERT(FOO == 2000,
	  ^~~~~~~~~~~~~~~~


Legacy __ASSERT()
=================

The ``__ASSERT()`` family, declared in
:zephyr_file:`include/zephyr/sys/__assert.h`, predates ``ZASSERT()`` and is now
a compatibility shim: ``__ASSERT()`` maps directly to ``ZASSERT_M(DEFAULT, ...)``,
using the built-in ``DEFAULT`` assertion module. New code should prefer
``ZASSERT()`` with a dedicated module.

.. note::

   ``__ASSERT()`` and the ``CONFIG_ASSERT*`` Kconfig options are deprecated.
   They continue to work through the ``DEFAULT`` module, but the underlying
   assert API may change in future releases.

The ``DEFAULT`` module is enabled by :kconfig:option:`CONFIG_ASSERT`. Its level is controlled by
:kconfig:option:`CONFIG_ASSERT_MODULE_DEFAULT_LEVEL`, configured through the
``Off`` / ``On`` / ``Verbose`` choice
(:kconfig:option:`CONFIG_ASSERT_MODULE_DEFAULT_LEVEL_OFF` /
:kconfig:option:`CONFIG_ASSERT_MODULE_DEFAULT_LEVEL_ON` /
:kconfig:option:`CONFIG_ASSERT_MODULE_DEFAULT_LEVEL_VERBOSE`). Assertions are enabled
by default when running Zephyr test cases, as configured by the
:kconfig:option:`CONFIG_TEST` option.

The deprecated legacy symbols are still honored and derive the ``DEFAULT`` module
level: :kconfig:option:`CONFIG_ASSERT_VERBOSE` maps to ``Verbose``,
:kconfig:option:`CONFIG_ASSERT_NO_COND_INFO` and
:kconfig:option:`CONFIG_ASSERT_NO_MSG_INFO` map to ``On``, and a disabled or
level-0 :kconfig:option:`CONFIG_ASSERT` leaves it ``Off``.
To disable all assertions regardless of how the level was configured, set
``CONFIG_ASSERT=n`` (the replacement for the deprecated
:kconfig:option:`CONFIG_FORCE_NO_ASSERT`).

API Reference
*************

.. doxygengroup:: zassert
