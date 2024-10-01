Bluetooth Mesh BabbleSim tests
##############################

This directory contains a set of high level system tests for the Bluetooth Mesh
subsystem. The tests run on the BabbleSim simulator, using the BabbleSim test
framework.

Tests are organized into bash scripts under the test_scripts folder. Each
subfolder of test_scripts contains tests for a specific module in the Bluetooth
mesh subsystem, and each folder has a corresponding test_<subfolder>.c under the
src/ directory containing the necessary test harnesses to execute the tests.

There's only a single test application for all the Bluetooth Mesh BabbleSim
tests. The test application is built from this directory, and includes all test
harnesses in every build. The overlying bsim test framework selects the harness
to use at runtime, using the test identifiers passed in the test scripts.

Running the tests
*****************

The Bluetooth Mesh tests have no extra requirements, and can be run using the
procedure described in the parent folder.

To only run the mesh tests, set ``SEARCH_PATH`` to point to this folder before
calling ``run_parallel.sh``

Debugging the tests
===================

Individual test harnesses can be run outside of ``RunTest()`` by setting the
``$SKIP`` array environment variable before calling ``RunTest``. This causes
``RunTest`` to skip starting processes for each application in the ``$SKIPPED``
array, so that they can be started independently in gdb, valgrind or some other
external tool.

When running test applications independently, make sure to assign the original
device number with -dXXX to the process.

Example: To debug the ``transport_tx_seg_block`` test harness in the transport
seg_block test, change transport/seg_block.sh to

..code-block::

   SKIP=(transport_tx_seg_block)
   RunTest mesh_transport_seg_block \
      transport_tx_seg_block \
      transport_rx_seg_block

Then separately, call

...code-block::

   gdb bs_nrf52_bsim_tests_bsim_bluetooth_mesh_prj_conf \
       -s=mesh_transport_seg_block -d=0 -RealEncryption=1 \
       -testid=transport_tx_seg_block

Framework
*********

The Bluetooth Mesh BabbleSim tests mainly operate on the test framework for the
BabbleSim platform, but with some mesh specific additions:

mesh_test.sh
=============

All test scripts in the Bluetooth Mesh BabbleSim test suite follow a common
pattern for running a single test across N devices with different test
harnesses. ``mesh_test.sh`` is sourced in each test script, and its ``RunTest``
function is called once in each script with the following parameters:

..code-block::

   RunTest <test name> <harness_name_1> <harness_name_2>...

``RunTest`` starts one mesh application for each selected harness, before
starting the BabbleSim phy process.

mesh_test.c
===========

Common target side mesh behavior is collected in mesh_test.c and mesh_test.h.
This includes ``PASS``/``FAIL`` macros and some common mesh actions, such as
mesh initialization, as well as message sending and receiving.

The mesh_test module is never called from the framework, so each test harness
is free to call the parts that are appropriate for its implementation.
All utility functions can be replaced by custom behavior in each harness if the
generic implementation does not fit with the harness requirements.

Adding tests
************

Each test needs a separate test script and one or more test harnesses that
execute the test on target.

Test scripts should be organized by submodules under the test_scripts
directory, and each test case needs a separate test script with a single test
run.

Each test harness is defined by a ``struct bst_test_instance`` structure, that
is presented to the test runner through an ``install`` function called from the
test application's main function. The harness contains a set of callback
functions that allows starting the test behavior.

Each mesh test harness should call the ``bt_mesh_test_cfg`` function from its
``test_init_f`` callback to set up the test timeout and mesh metadata. The test
will run until the mesh_test module's ``FAIL`` macro is called, or until the
test times out. At the test timeout, the test will pass if the ``PASS()`` macro
has been called - otherwise, it will fail.

..note::

   The Bluetooth stack must be initialized in the ``test_main_f`` function of
   the harness, as the previous callbacks are all executed in hardware threads.

The Bluetooth Mesh tests include the entire Bluetooth host and controller
subsystems, so timing requirements should generally be kept fairly liberal to
avoid regressions on changes in the underlying layers.
