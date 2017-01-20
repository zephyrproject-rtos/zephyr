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

A simple working base is located at `samples/testing/integration`. Just copy the
files to `tests/` and edit them for your needs. The test will then be
automatically built and run by the sanitycheck script. If you are testing the `bar`
component of `foo`, you should copy the sample folder to tests/foo/bar. It can
then be tested with `./scripts/sanitycheck -s tests/foo/bar/test`.

The sample contains the following files:

Makefile

.. literalinclude:: ../../../samples/testing/integration/Makefile
   :language: Make
   :linenos:

testcase.ini

.. literalinclude:: ../../../samples/testing/integration/testcase.ini
   :language: ini
   :linenos:

prj.conf

.. literalinclude:: ../../../samples/testing/integration/prj.conf
   :language: text
   :linenos:

src/Makefile

.. literalinclude:: ../../../samples/testing/integration/src/Makefile
   :language: Make
   :linenos:

src/main.c

.. literalinclude:: ../../../samples/testing/integration/src/main.c
   :language: c
   :linenos:

.. contents::
   :depth: 1
   :local:
   :backlinks: top

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

The `samples/testing/unit` folder contains an example for testing
the net-buf api of Zephyr.

Makefile

.. literalinclude:: ../../../samples/testing/unit/Makefile
   :language: Make
   :linenos:

testcase.ini

.. literalinclude:: ../../../samples/testing/unit/testcase.ini
   :language: ini
   :linenos:

main.c

.. literalinclude:: ../../../samples/testing/unit/main.c
   :language: c
   :linenos:

API reference
*************

Running tests
=============

.. doxygengroup:: ztest_test
   :project: Zephyr
   :content-only:

Assertions
==========

These macros will instantly fail the test if the related assertion fails.
When an assertion fails, it will print the current file, line and function,
alongside a reason for the failure and an optional message. If the config
option `CONFIG_ZTEST_ASSERT_VERBOSE=0`, the assertions will only print the
file and line numbers, reducing the binary size of the test.

Example output for a failed macro from
`assert_equal(buf->ref, 2, "Invalid refcount")`:

.. code-block:: none

    Assertion failed at main.c:62: test_get_single_buffer: Invalid refcount (buf->ref not equal to 2)
    Aborted at unit test function

.. doxygengroup:: ztest_assert
   :project: Zephyr
   :content-only:

Mocking
=======

These functions allow abstracting callbacks and related functions and
controlling them from specific tests. You can enable the mocking framework by
setting `CONFIG_ZTEST_MOCKING=y` in the configuration file of the test.
The amount of concurrent return values and expected parameters is
limited by `ZTEST_PARAMETER_COUNT`.

Here is an example for configuring the function `expect_two_parameters` to
expect the values `a=2` and `b=3`, and telling `returns_int` to return `5`:

.. literalinclude:: mocking.c
   :language: c
   :linenos:

.. doxygengroup:: ztest_mock
   :project: Zephyr
   :content-only:
