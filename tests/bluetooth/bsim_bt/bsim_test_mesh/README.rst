Bluetooth Mesh BabbleSim tests
##############################

This directory contains a set of high level system tests for the Bluetooth Mesh
subsystem. The tests run on the BabbleSim simulator, using the BabbleSim test
framework.

There's only a single test application for all the Bluetooth Mesh BabbleSim
tests. The test application is built from this directory, and includes all test
test casees in every build. Twister can select the test casees to use at runtime,
using the test identifiers passed in the ``harness_config.bsim_tests`` entry.

Framework
*********

The Bluetooth Mesh BabbleSim tests mainly operate on the test framework for the
BabbleSim platform, but with some Mesh specific additions in mesh_test.c:

Common target side mesh behavior is collected in mesh_test.c and mesh_test.h.
This includes ``PASS``/``FAIL`` macros and some common mesh actions, such as
mesh initialization, as well as message sending and receiving.

The mesh_test module is never called from the framework, so each test test case
is free to call the parts that are appropriate for its implementation.
All utility functions can be replaced by custom behavior in each test case if the
generic implementation does not fit with the test case requirements.

Adding tests
************

Each test needs a separate test script and one or more test test casees that
execute the test on target.

Test scripts should be organized by submodules under the test_scripts
directory, and each test case needs a separate test script with a single test
run.

Each test test case is defined by a ``struct bst_test_instance` structure, that
is presented to the test runner through an ``install`` function called from the
test application's main function. The test case contains a set of callback
functions that allows starting the test behavior.

Each mesh test test case should call the ``bt_mesh_test_cfg`` function from its
``test_init_f`` callback to set up the test timeout and mesh metadata. The test
will run until the mesh_test module's ``FAIL`` macro is called, or until the
test times out. At the test timeout, the test will pass if the ``PASS()`` macro
has been called - otherwise, it will fail.

..note::

   The Bluetooth stack must be initialized in the ``test_main_f`` function of
   the test case, as the previous callbacks are all executed in hardware threads.

The Bluetooth Mesh tests include the entire Bluetooth host and controller
subsystems, so timing requirements should generally be kept fairly liberal to
avoid regressions on changes in the underlying layers.
