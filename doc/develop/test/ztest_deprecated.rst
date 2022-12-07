.. _test-framework-deprecated:

ZTest Deprecated APIs
#####################

Ztest is currently being migrated to a new API, this documentation provides information about the
deprecated APIs which will eventually be removed. See :ref:`Test Framework <test-framework>` for the new API.
Similarly, ZTest's mocking framework is also deprecated (see :ref:`Mocking via FFF <mocking-fff>`).

Quick start - Unit testing
**************************

Ztest can be used for unit testing. This means that rather than including the
entire Zephyr OS for testing a single function, you can focus the testing
efforts into the specific module in question. This will speed up testing since
only the module will have to be compiled in, and the tested functions will be
called directly.

Since you won't be including basic kernel data structures that most code
depends on, you have to provide function stubs in the test. Ztest provides
some helpers for mocking functions, as demonstrated below.

In a unit test, mock objects can simulate the behavior of complex real objects
and are used to decide whether a test failed or passed by verifying whether an
interaction with an object occurred, and if required, to assert the order of
that interaction.

.. _main_c_bp:

Best practices for declaring the test suite
===========================================

*twister* and other validation tools need to obtain the list of
subcases that a Zephyr *ztest* test image will expose.

.. admonition:: Rationale

   This all is for the purpose of traceability. It's not enough to
   have only a semaphore test project.  We also need to show that we
   have testpoints for all APIs and functionality, and we trace back
   to documentation of the API, and functional requirements.

   The idea is that test reports show results for every sub-testcase
   as passed, failed, blocked, or skipped.  Reporting on only the
   high-level test project level, particularly when tests do too
   many things, is too vague.

There exist two alternatives to writing tests. The first, and more verbose,
approach is to directly declare and run the test suites.
Here is a generic template for a test showing the expected use of
:c:func:`ztest_test_suite`:

.. code-block:: C

   #include <zephyr/ztest.h>

   extern void test_sometest1(void);
   extern void test_sometest2(void);
   #ifndef CONFIG_WHATEVER		/* Conditionally skip test_sometest3 */
   void test_sometest3(void)
   {
   	ztest_test_skip();
   }
   #else
   extern void test_sometest3(void);
   #endif
   extern void test_sometest4(void);
   ...

   void test_main(void)
   {
   	ztest_test_suite(common,
                            ztest_unit_test(test_sometest1),
                            ztest_unit_test(test_sometest2),
                            ztest_unit_test(test_sometest3),
                            ztest_unit_test(test_sometest4)
                   );
   	ztest_run_test_suite(common);
   }

Alternatively, it is possible to split tests across multiple files using
:c:func:`ztest_register_test_suite` which bypasses the need for ``extern``:

.. code-block:: C

  #include <zephyr/ztest.h>

  void test_sometest1(void) {
  	zassert_true(1, "true");
  }

  ztest_register_test_suite(common, NULL,
  			    ztest_unit_test(test_sometest1)
  			    );

The above sample simple registers the test suite and uses a ``NULL`` pragma
function (more on that later). It is important to note that the test suite isn't
directly run in this file. Instead two alternatives exist for running the suite.
First, if to do nothing. A default ``test_main`` function is provided by
ztest. This is the preferred approach if the test doesn't involve a state and
doesn't require use of the pragma.

In cases of an integration test it is possible that some general state needs to
be set between test suites. This can be thought of as a state diagram in which
``test_main`` simply goes through various actions that modify the board's
state and different test suites need to run. This is achieved in the following:

.. code-block:: C

  #include <zephyr/ztest.h>

  struct state {
  	bool is_hibernating;
  	bool is_usb_connected;
  }

  static bool pragma_always(const void *state)
  {
  	return true;
  }

  static bool pragma_not_hibernating_not_connected(const void *s)
  {
  	struct state *state = s;
  	return !state->is_hibernating && !state->is_usb_connected;
  }

  static bool pragma_usb_connected(const void *s)
  {
  	return ((struct state *)s)->is_usb_connected;
  }

  ztest_register_test_suite(baseline, pragma_always,
  			    ztest_unit_test(test_case0));
  ztest_register_test_suite(before_usb, pragma_not_hibernating_not_connected,
  			    ztest_unit_test(test_case1),
  			    ztest_unit_test(test_case2));
  ztest_register_test_suite(with_usb, pragma_usb_connected,,
  			    ztest_unit_test(test_case3),
  			    ztest_unit_test(test_case4));

  void test_main(void)
  {
  	struct state state;

	/* Should run `baseline` test suite only. */
	ztest_run_registered_test_suites(&state);

  	/* Simulate power on and update state. */
  	emulate_power_on();
  	/* Should run `baseline` and `before_usb` test suites. */
  	ztest_run_registered_test_suites(&state);

  	/* Simulate plugging in a USB device. */
  	emulate_plugging_in_usb();
  	/* Should run `baseline` and `with_usb` test suites. */
  	ztest_run_registered_test_suites(&state);

  	/* Verify that all the registered test suites actually ran. */
  	ztest_verify_all_registered_test_suites_ran();
  }

For *twister* to parse source files and create a list of subcases,
the declarations of :c:func:`ztest_test_suite` and
:c:func:`ztest_register_test_suite` must follow a few rules:

- one declaration per line

- conditional execution by using :c:func:`ztest_test_skip`

What to avoid:

- packing multiple testcases in one source file

  .. code-block:: C

     void test_main(void)
     {
     #ifdef TEST_feature1
             ztest_test_suite(feature1,
                              ztest_unit_test(test_1a),
                              ztest_unit_test(test_1b),
                              ztest_unit_test(test_1c)
                              );
             ztest_run_test_suite(feature1);
     #endif

     #ifdef TEST_feature2
             ztest_test_suite(feature2,
                              ztest_unit_test(test_2a),
                              ztest_unit_test(test_2b)
                              );
             ztest_run_test_suite(feature2);
     #endif
     }


- Do not use ``#if``

  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1),
                              ztest_unit_test(test_sometest2),
     #ifdef CONFIG_WHATEVER
                              ztest_unit_test(test_sometest3),
     #endif
                              ztest_unit_test(test_sometest4),
             ...

- Do not add comments on lines with a call to :c:func:`ztest_unit_test`:

  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1),
                              ztest_unit_test(test_sometest2) /* will fail */,
             /* will fail! */ ztest_unit_test(test_sometest3),
                              ztest_unit_test(test_sometest4),
             ...

- Do not define multiple definitions of unit / user unit test case per
  line


  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1), ztest_unit_test(test_sometest2),
                              ztest_unit_test(test_sometest3),
                              ztest_unit_test(test_sometest4),
             ...


Other questions:

- Why not pre-scan with CPP and then parse? or post scan the ELF file?

  If C pre-processing or building fails because of any issue, then we
  won't be able to tell the subcases.

- Why not declare them in the YAML testcase description?

  A separate testcase description file would be harder to maintain
  than just keeping the information in the test source files
  themselves -- only one file to update when changes are made
  eliminates duplication.

Mocking
*******

These functions allow abstracting callbacks and related functions and
controlling them from specific tests. You can enable the mocking framework by
setting :kconfig:option:`CONFIG_ZTEST_MOCKING` to "y" in the configuration file of the
test.  The amount of concurrent return values and expected parameters is
limited by :kconfig:option:`CONFIG_ZTEST_PARAMETER_COUNT`.

Here is an example for configuring the function ``expect_two_parameters`` to
expect the values ``a=2`` and ``b=3``, and telling ``returns_int`` to return
``5``:

.. literalinclude:: mocking.c
   :language: c
   :linenos:

.. doxygengroup:: ztest_mock
