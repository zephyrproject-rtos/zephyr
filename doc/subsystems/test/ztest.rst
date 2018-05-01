.. _testing:

Test Framework
###############

The Zephyr Test Framework (Ztest) provides a simple testing framework intended
to be used during development.  It provides basic assertion macros and a generic
test structure.

The framework can be used in two ways, either as a generic framework for
integration testing, or for unit testing specific modules.

Quick start - Integration testing
*********************************

A simple working base is located at :file:`samples/testing/integration`.  Just
copy the files to ``tests/`` and edit them for your needs. The test will then
be automatically built and run by the sanitycheck script. If you are testing
the **bar** component of **foo**, you should copy the sample folder to
``tests/foo/bar``. It can then be tested with::

    ./scripts/sanitycheck -s tests/foo/bar/test

The sample contains the following files:

CMakeLists.txt

.. literalinclude:: ../../../samples/testing/integration/CMakeLists.txt
   :language: CMake
   :linenos:

testcase.yaml

.. literalinclude:: ../../../samples/testing/integration/testcase.yaml
   :language: yaml
   :linenos:

prj.conf

.. literalinclude:: ../../../samples/testing/integration/prj.conf
   :language: text
   :linenos:

src/main.c

.. literalinclude:: ../../../samples/testing/integration/src/main.c
   :language: c
   :linenos:

.. contents::
   :depth: 1
   :local:
   :backlinks: top



A test case project may consist of multiple sub-tests or smaller tests that
either can be testing functionality or APIs. Functions implementing a test
should follow the guidelines below:

* Test cases function names should be prefix with **test_**
* Test cases should be documented using doxygen
* Test function names should be unique within the section or component being
  tested


An example can be seen below::

    /**
     * @brief Test Asserts
     *
     * This test verifies the zassert_true macro.
     */
    static void test_assert(void)
    {
            zassert_true(1, "1 was false");
    }


The above test is then enabled as part of the testsuite using::

    ztest_unit_test(test_assert)


Listing Tests
=============

Tests (test projects) in the Zephyr tree consist of many testcases that run as
part of a project and test similar functionality, for example an API or a
feature. The ``sanitycheck`` script can parse the testcases in all
test projects or a subset of them, and can generate reports on a granular
level, i.e. if cases have passed or failed or if they were blocked or skipped.

Sanitycheck parses the source files looking for test case names, so you
can list all kernel test cases, for example, by entering::

        sanitycheck --list-tests -T tests/kernel

Skipping Tests
==============

Special- or architecture-specific tests cannot run on all
platforms and architectures, however we still want to count those and
report them as being skipped.  Because the test inventory and
the list of tests is extracted from the code, adding
conditionals inside the test suite is sub-optimal.  Tests that need
to be skipped for a certain platform or feature need to explicitly
report a skip using :c:func:`ztest_test_skip()`. If the test runs,
it needs to report either a pass or fail.  For example::

	#ifdef CONFIG_TEST1
	void test_test1(void)
	{
		zassert_true(1, "true");
	}
        #else
	void test_test1(void)
	{
		ztest_test_skip();
	}
	#endif


	void test_main(void)
	{
		ztest_test_suite(common,
				 ztest_unit_test(test_test1),
				 ztest_unit_test(test_test2)
				 );
		ztest_run_test_suite(common);
	}

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

API reference
*************

Running tests
=============

.. doxygengroup:: ztest_test
   :project: Zephyr

Assertions
==========

These macros will instantly fail the test if the related assertion fails.
When an assertion fails, it will print the current file, line and function,
alongside a reason for the failure and an optional message. If the config
option:`CONFIG_ZTEST_ASSERT_VERBOSE` is 0, the assertions will only print the
file and line numbers, reducing the binary size of the test.

Example output for a failed macro from
``zassert_equal(buf->ref, 2, "Invalid refcount")``:

.. code-block:: none

    Assertion failed at main.c:62: test_get_single_buffer: Invalid refcount (buf->ref not equal to 2)
    Aborted at unit test function

.. doxygengroup:: ztest_assert
   :project: Zephyr

Mocking
=======

These functions allow abstracting callbacks and related functions and
controlling them from specific tests. You can enable the mocking framework by
setting :option:`CONFIG_ZTEST_MOCKING` to "y" in the configuration file of the
test.  The amount of concurrent return values and expected parameters is
limited by :option:`CONFIG_ZTEST_PARAMETER_COUNT`.

Here is an example for configuring the function ``expect_two_parameters`` to
expect the values ``a=2`` and ``b=3``, and telling ``returns_int`` to return
``5``:

.. literalinclude:: mocking.c
   :language: c
   :linenos:

.. doxygengroup:: ztest_mock
   :project: Zephyr
