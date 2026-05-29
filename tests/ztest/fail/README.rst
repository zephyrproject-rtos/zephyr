.. _ztest_framework_failure_tests:

Ztest framework failure tests
#############################

Overview
********

In order to test the actual framework's failure cases, this test suite has to do something unique.
There's a subdirectory to this test called 'core'. This project builds a sample as a
:zephyr:board:`native_sim <native_sim>` or :ref:`unit_testing <ztest_unit_testing>`
binary which is expected to fail by calling one of the following:
- ``ztest_test_fail()`` during either the ``after`` or ``teardown`` phase of the test suite
- ``ztest_test_skip()`` during either the ``after`` or ``teardown`` phase of the test suite
- ``ztest_test_pass()`` during either the ``after`` or ``teardown`` phase of the test suite

Note that these can be called indirectly through failed asserts or assumptions.

The binary by itself, when executed, will fail to run and return a code of ``1``. The main test
binary will use ``popen()`` to run the failing test binary and will assert both the return code and
the output. The output itself cannot be printed to the log as it will confuse ``twister`` by
reporting a failure.
