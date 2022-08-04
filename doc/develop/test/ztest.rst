.. _test-framework:

Test Framework
###############

The Zephyr Test Framework (Ztest) provides a simple testing framework intended
to be used during development.  It provides basic assertion macros and a generic
test structure.

The framework can be used in two ways, either as a generic framework for
integration testing, or for unit testing specific modules.

To enable the latest APIs of Ztest simply set :kconfig:option:`CONFIG_ZTEST_NEW_API=y`. The legacy
APIs will soon be deprecated and eventually removed.

Creating a test suite
*********************

Using Ztest to create a test suite is as easy as calling the :c:macro:`ZTEST_SUITE`. The macro
accepts the following arguments:

* ``suite_name`` - The name of the suite. This name must be unique within a single binary.
* :c:type:`ztest_suite_predicate_t` - An optional predicate function to allow choosing when the
  test will run. The predicate will get a pointer to the global state passed in through
  :c:func:`ztest_run_all` and should return a boolean to decide if the suite should run.
* :c:type:`ztest_suite_setup_t` - An optional setup function which returns a test fixture. This
  will be called and run once per test suite run.
* :c:type:`ztest_suite_before_t` - An optional before function which will run before every single
  test in this suite.
* :c:type:`ztest_suite_after_t` - An optional after function which will run after every single
  test in this suite.
* :c:type:`ztest_suite_teardown_t` - An optional teardown function which will run at the end of
  all the tests in the suite.

Below is an example of a test suite using a predicate::

    #include <zephyr/ztest.h>
    #include "test_state.h"

    static bool predicate(const void *global_state)
    {
      return ((const struct test_state*)global_state)->x == 5;
    }

    ZTEST_SUITE(alternating_suite, predicate, NULL, NULL, NULL, NULL);

Adding tests to a suite
***********************

There are 4 macros used to add a test to a suite, they are:

* :c:macro:`ZTEST` ``(suite_name, test_name)`` - Which can be used to add a test by ``test_name`` to a
  given suite by ``suite_name``.
* :c:macro:`ZTEST_USER` ``(suite_name, test_name)`` - Which behaves the same as :c:macro:`ZTEST`, only
  that when :kconfig:option:`CONFIG_USERSPACE` is enabled, then the test will be run in a userspace
  thread.
* :c:macro:`ZTEST_F` ``(suite_name, test_name)`` - Which behaves the same as :c:macro:`ZTEST`, only
  that the test function will already include a variable named ``fixture`` with the type
  ``<suite_name>_fixture``.
* :c:macro:`ZTEST_USER_F` ``(suite_name, test_name)`` - Which combines the fixture feature of
  :c:macro:`ZTEST_F` with the userspace threading for the test.

Test fixtures
=============

Test fixtures can be used to help simplify repeated test setup operations. In many cases, tests in
the same suite will require some initial setup followed by some form of reset between each test.
This is achieved via fixtures in the following way::

    #include <zephyr/ztest.h>

    struct my_suite_fixture {
      size_t max_size;
      size_t size;
      uint8_t buff[1];
    };

    static void *my_suite_setup(void)
    {
      /* Allocate the fixture with 256 byte buffer */
      struct my_suite_fixture *fixture = k_malloc(sizeof(struct my_suite_fixture) + 255);

      zassume_not_null(fixture, NULL);
      fixture->max_size = 256;

      return fixture;
    }

    static void my_suite_before(void *f)
    {
      struct my_suite_fixture *fixture = (struct my_suite_fixture *)f;
      memset(fixture->buff, 0, fixture->max_size);
      fixture->size = 0;
    }

    static void my_suite_teardown(void *f)
    {
      k_free(f);
    }

    ZTEST_SUITE(my_suite, NULL, my_suite_setup, my_suite_before, NULL, my_suite_teardown);

    ZTEST_F(my_suite, test_feature_x)
    {
      zassert_equal(0, fixture->size);
      zassert_equal(256, fixture->max_size);
    }


Advanced features
*****************

Test rules
==========

Test rules are a way to run the same logic for every test and every suite. There are a lot of cases
where you might want to reset some state for every test in the binary (regardless of which suite is
currently running). As an example, this could be to reset mocks, reset emulators, flush the UART,
etc.

  .. code-block:: C

    #include <zephyr/fff.h>
    #include <zephyr/ztest.h>

    #include "test_mocks.h"

    DEFINE_FFF_GLOBALS;

    DEFINE_FAKE_VOID_FUN(my_weak_func);

    static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
    {
      ARG_UNUSED(test);
      ARG_UNUSED(fixture);

      RESET_FAKE(my_weak_func);
    }

    ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

A custom ``test_main``
======================

While the Ztest framework provides a default :c:func:`test_main` function, it's possible that some
applications will want to provide custom behavior. This is particularly true if there's some global
state that the tests depend on and that state either cannot be replicated or is difficult to
replicate without starting the process over. For example, one such state could be a power sequence.
Assuming there's a board with several steps in the power-on sequence a test suite can be written
using the ``predicate`` to control when it would run. In that case, the :c:func:`test_main`
function can be written as following::

    #include <zephyr/ztest.h>

    #include "my_test.h"

    void test_main(void)
    {
      struct power_sequence_state state;

      /* Only suites that use a predicate checking for phase == PWR_PHASE_0 will run. */
      state.phase = PWR_PHASE_0;
      ztest_run_all(&state);

      /* Only suites that use a predicate checking for phase == PWR_PHASE_1 will run. */
      state.phase = PWR_PHASE_1;
      ztest_run_all(&state);

      /* Only suites that use a predicate checking for phase == PWR_PHASE_2 will run. */
      state.phase = PWR_PHASE_2;
      ztest_run_all(&state);

      /* Check that all the suites in this binary ran at least once. */
      ztest_verify_all_test_suites_ran();
    }


Quick start - Integration testing
*********************************

A simple working base is located at :zephyr_file:`samples/subsys/testsuite/integration`.  Just
copy the files to ``tests/`` and edit them for your needs. The test will then
be automatically built and run by the twister script. If you are testing
the **bar** component of **foo**, you should copy the sample folder to
``tests/foo/bar``. It can then be tested with::

    ./scripts/twister -s tests/foo/bar/test-identifier


In the example above ``tests/foo/bar`` signifies the path to the test and the
``test-identifier`` references a test defined in the :file:`testcase.yaml` file.

To run all tests defined in a test project, run::

    ./scripts/twister -T tests/foo/bar/

The sample contains the following files:

CMakeLists.txt

.. literalinclude:: ../../../samples/subsys/testsuite/integration/CMakeLists.txt
   :language: CMake
   :linenos:

testcase.yaml

.. literalinclude:: ../../../samples/subsys/testsuite/integration/testcase.yaml
   :language: yaml
   :linenos:

prj.conf

.. literalinclude:: ../../../samples/subsys/testsuite/integration/prj.conf
   :language: text
   :linenos:

src/main.c (see :ref:`best practices <main_c_bp>`)

.. literalinclude:: ../../../samples/subsys/testsuite/integration/src/main.c
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
    ZTEST(my_suite, test_assert)
    {
            zassert_true(1, "1 was false");
    }


Listing Tests
=============

Tests (test projects) in the Zephyr tree consist of many testcases that run as
part of a project and test similar functionality, for example an API or a
feature. The ``twister`` script can parse the testcases in all
test projects or a subset of them, and can generate reports on a granular
level, i.e. if cases have passed or failed or if they were blocked or skipped.

Twister parses the source files looking for test case names, so you
can list all kernel test cases, for example, by entering::

        twister --list-tests -T tests/kernel


Skipping Tests
==============

Special- or architecture-specific tests cannot run on all
platforms and architectures, however we still want to count those and
report them as being skipped.  Because the test inventory and
the list of tests is extracted from the code, adding
conditionals inside the test suite is sub-optimal.  Tests that need
to be skipped for a certain platform or feature need to explicitly
report a skip using :c:func:`ztest_test_skip` or :c:macro:`Z_TEST_SKIP_IFDEF`. If the test runs,
it needs to report either a pass or fail.  For example::

        #ifdef CONFIG_TEST1
	ZTEST(common, test_test1)
	{
	  zassert_true(1, "true");
	}
        #else
	ZTEST(common, test_test1)
	{
		ztest_test_skip();
	}
	#endif

        ZTEST(common, test_test2)
        {
          Z_TEST_SKIP_IFDEF(CONFIG_BUGxxxxx);
          zassert_equal(1, 0, NULL);
        }

        ZTEST_SUITE(common, NULL, NULL, NULL, NULL, NULL);

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

Other questions:

- Why not pre-scan with CPP and then parse? or post scan the ELF file?

  If C pre-processing or building fails because of any issue, then we
  won't be able to tell the subcases.

- Why not declare them in the YAML testcase description?

  A separate testcase description file would be harder to maintain
  than just keeping the information in the test source files
  themselves -- only one file to update when changes are made
  eliminates duplication.

Stress test framework
*********************

Zephyr stress test framework (Ztress) provides an environment for executing user
functions in multiple priority contexts. It can be used to validate that code is
resilient to preemptions. The framework tracks the number of executions and preemptions
for each context. Execution can have various completion conditions like timeout,
number of executions or number of preemptions.

The framework is setting up the environment by creating the requested number of threads
(each on different priority), optionally starting a timer. For each context, a user
function (different for each context) is called and then the context sleeps for
a randomized amount of system ticks. The framework is tracking CPU load and adjusts sleeping
periods to achieve higher CPU load. In order to increase the probability of preemptions,
the system clock frequency should be relatively high. The default 100 Hz on QEMU x86
is much too low and it is recommended to increase it to 100 kHz.

The stress test environment is setup and executed using :c:macro:`ZTRESS_EXECUTE` which
accepts a variable number of arguments. Each argument is a context that is
specified by :c:macro:`ZTRESS_TIMER` or :c:macro:`ZTRESS_THREAD` macros. Contexts
are specified in priority descending order. Each context specifies completion
conditions by providing the minimum number of executions and preemptions. When all
conditions are met and the execution has completed, an execution report is printed
and the macro returns. Note that while the test is executing, a progress report is
periodically printed.

Execution can be prematurely completed by specifying a test timeout (:c:func:`ztress_set_timeout`)
or an explicit abort (:c:func:`ztress_abort`).

User function parameters contains an execution counter and a flag indicating if it is
the last execution.

The example below presents how to setup and run 3 contexts (one of which is k_timer
interrupt handler context). Completion criteria is set to at least 10000 executions
of each context and 1000 preemptions of the lowest priority context. Additionally,
the timeout is configured to complete after 10 seconds if those conditions are not met.
The last argument of each context is the initial sleep time which will be adjusted throughout
the test to achieve the highest CPU load.

  .. code-block:: C

             ztress_set_timeout(K_MSEC(10000));
             ZTRESS_EXECUTE(ZTRESS_TIMER(foo_0, user_data_0, 10000, Z_TIMEOUT_TICKS(20)),
                            ZTRESS_THREAD(foo_1, user_data_1, 10000, 0, Z_TIMEOUT_TICKS(20)),
                            ZTRESS_THREAD(foo_2, user_data_2, 10000, 1000, Z_TIMEOUT_TICKS(20)));

Configuration
=============

Static configuration of Ztress contains:

 - :c:macro:`ZTRESS_MAX_THREADS` - number of supported threads.
 - :c:macro:`ZTRESS_STACK_SIZE` - Stack size of created threads.
 - :c:macro:`ZTRESS_REPORT_PROGRESS_MS` - Test progress report interval.

API reference
*************

Running tests
=============

.. doxygengroup:: ztest_test

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

Assumptions
===========

These macros will instantly skip the test or suite if the related assumption fails.
When an assumption fails, it will print the current file, line, and function,
alongside a reason for the failure and an optional message. If the config
option:`CONFIG_ZTEST_ASSERT_VERBOSE` is 0, the assumptions will only print the
file and line numbers, reducing the binary size of the test.

Example output for a failed macro from
``zassume_equal(buf->ref, 2, "Invalid refcount")``:

.. code-block::none

    START - test_get_single_buffer
        Assumption failed at main.c:62: test_get_single_buffer: Invalid refcount (buf->ref not equal to 2)
     SKIP - test_get_single_buffer in 0.0 seconds

.. doxygengroup:: ztest_assume


.. _mocking-fff:

Mocking via FFF
===============

Zephyr has integrated with FFF for mocking. See `FFF`_ for documentation. To use it, use the
following in your source::

    #include <zephyr/fff.h>


Customizing Test Output
***********************
The way output is presented when running tests can be customized.
An example can be found in :zephyr_file:`tests/ztest/custom_output`.

Customization is enabled by setting :kconfig:option:`CONFIG_ZTEST_TC_UTIL_USER_OVERRIDE` to "y"
and adding a file :file:`tc_util_user_override.h` with your overrides.

Add the line ``zephyr_include_directories(my_folder)`` to
your project's :file:`CMakeLists.txt` to let Zephyr find your header file during builds.

See the file :zephyr_file:`subsys/testsuite/include/tc_util.h` to see which macros and/or defines can be overridden.
These will be surrounded by blocks such as::

        #ifndef SOMETHING
        #define SOMETHING <default implementation>
        #endif /* SOMETHING */

.. _ztest_shuffle:

Shuffling Test Sequence
***********************
By default the tests are sorted and ran in alphanumerical order.  Test cases may
be dependent on this sequence. Enable `ZTEST_SHUFFLE` to randomize the order. The
output from the test will display the seed for failed tests.  For native posix
builds you can provide the seed as an argument to twister with `--seed`

Static configuration of ZTEST_SHUFFLE contains:

 - :c:macro:`ZTEST_SHUFFLE_SUITE_REPEAT_COUNT` - Number of iterations the test suite will run.
 - :c:macro:`ZTEST_SHUFFLE_TEST_REPEAT_COUNT` - Number of iterations the test will run.


Test Selection
**************
For POSIX enabled builds with ZTEST_NEW_API use command line arguments to list
or select tests to run. The test argument expects a comma separated list
of ``suite::test`` .  You can substitute the test name with an ``*`` to run all
tests within a suite.

For example

.. code-block:: bash

    $ zephyr.exe -list
    $ zephyr.exe -test="fixture_tests::test_fixture_pointer,framework_tests::test_assert_mem_equal"
    $ zephyr.exe -test="framework_tests::*"

.. _FFF: https://github.com/meekrosoft/fff
