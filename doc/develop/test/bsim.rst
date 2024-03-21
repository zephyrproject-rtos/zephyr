.. _bsim:

BabbleSim
#########

BabbleSim and Zephyr
********************

In the Zephyr project we use the `Babblesim`_ simulator to test some of the Zephyr radio protocols,
including the BLE stack, 802.15.4, and some of the networking stack.

BabbleSim_ is a physical layer simulator, which in combination with the Zephyr
:ref:`bsim boards<bsim boards>`
can be used to simulate a network of BLE and 15.4 devices.
When we build Zephyr targeting a :ref:`bsim board<bsim boards>` we produce a Linux
executable, which includes the application, Zephyr OS, and models of the HW.

When there is radio activity, this Linux executable will connect to the BabbleSim Phy simulation
to simulate the radio channel.

In the BabbleSim documentation you can find more information on how to
`get <https://babblesim.github.io/fetching.html>`_ and
`build <https://babblesim.github.io/building.html>`_ the simulator.
In the :ref:`nrf52_bsim<nrf52_bsim>` and :ref:`nrf5340bsim<nrf5340bsim>` boards documentation
you can find more information about how to build Zephyr targeting these particular boards,
and a few examples.

Types of tests
**************

Tests without radio activity: bsim tests with twister
=====================================================

The :ref:`bsim boards<bsim boards>` can be used without radio activity, and in that case, it is not
necessary to connect them to a physical layer simulation. Thanks to this, these target boards can
be used just like :ref:`native_sim<native_sim>` with :ref:`twister <twister_script>`,
to run all standard Zephyr twister tests, but with models of a real SOC HW, and their drivers.

Tests with radio activity
=========================

When there is radio activity, BabbleSim tests require at the very least a physical layer simulation
running, and most, more than 1 simulated device. Due to this, these tests are not build and run
with twister, but with a dedicated set of tests scripts.

These tests are kept in the :code:`tests/bsim/` folder. The ``compile.sh`` and ``run_parallel.sh``
scripts contained in that folder are used by the CI system to build the needed images and execute
these tests in batch.

See sections below for more information about how to build and run them, as well as the conventions
they follow.

There are two main sets of tests:

* Self checking embedded application/tests: In which some of the simulated devices applications are
  built with some checks which decide if the test is passing or failing. These embedded
  applications tests use the :ref:`bs_tests<bsim_boards_bs_tests>` system to report the pass or
  failure, and in many cases to build several tests into the same binary.

* Test using the EDTT_ tool, in which a EDTT (python) test controls the embedded applications over
  an RPC mechanism, and decides if the test passes or not.
  Today these tests include a very significant subset of the BT qualification test suite.

More information about how different tests types relate to BabbleSim and the bsim boards can be
found in the :ref:`bsim boards tests section<bsim_boards_tests>`.

Test coverage and BabbleSim
***************************

As the :ref:`nrf52_bsim<nrf52_bsim>` and :ref:`nrf5340bsim<nrf5340bsim>` boards are based on the
POSIX architecture, you can easily collect test coverage information.

You can use the script :zephyr_file:`tests/bsim/generate_coverage_report.sh` to generate an html
coverage report from tests.

Check :ref:`the page on coverage generation <coverage_posix>` for more info.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _EDTT:
   https://github.com/EDTTool/EDTT

Building and running the tests
******************************

See the :ref:`nrf52_bsim` page for setting up the simulator.

The scripts also expect a few environment variables to be set.
For example, from Zephyr's root folder, you can run:

.. code-block:: bash

   # Build all the tests
   ${ZEPHYR_BASE}/tests/bsim/compile.sh

   # Run them (in parallel)
   RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml \
      SEARCH_PATH=${ZEPHYR_BASE}/tests/bsim \
         ${ZEPHYR_BASE}/tests/bsim/run_parallel.sh

Or to build and run only a specific subset, e.g. host advertising tests:

.. code-block:: bash

   # Build the Bluetooth host advertising tests
   ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/adv/compile.sh

   # Run them (in parallel)
   RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml \
      SEARCH_PATH=${ZEPHYR_BASE}/tests/bsim/bluetooth/host/adv \
         ${ZEPHYR_BASE}/tests/bsim/run_parallel.sh

Check the ``run_parallel.sh`` help for more options and examples on how to use this script to run
the tests in batch.

After building the tests' required binaries you can run a test directly using its individual test
script.

For example you can build the required binaries for the networking tests with

.. code-block:: bash

   WORK_DIR=${ZEPHYR_BASE}/bsim_out ${ZEPHYR_BASE}/tests/bsim/net/compile.sh

and then directly run one of the tests:

.. code-block:: bash

   ${ZEPHYR_BASE}/tests/bsim/net/sockets/echo_test/tests_scripts/echo_test_802154.sh

Conventions
===========

Build scripts
-------------

The build scripts ``compile.sh`` simply build all the required test and sample applications
for the tests' scripts placed in the subfolders below.

This build scripts use the common compile.source which provide a function (compile) which calls
cmake and ninja with the provided application, configuration and overlay files.

To speed up compilation for users interested only in a subset of tests, several compile scripts
exist in several subfolders, where the upper ones call into the lower ones.

Note that cmake and ninja are used directly instead of the ``west build`` wrapper as west is not
required, and some Zephyr users do not use or have west, but still use the build and tests scripts.

Test scripts
------------

- Each test is defined by a shell script with the extension ``.sh``.
- Scripts starting with an underscore (``_``) are ignored.
- Test scripts expect that the binaries they require are already built, and will spawn the processes
  for the simulated devices and physical layer simulation with the necessary command line options.
- Tests must return 0 to the invoking shell if the test passes, and not 0 if the test fails.
- It is recommended to have a single test for each test script.
- Each test must have a unique simulation id, to enable running different tests in parallel.
- The test scripts should not compile the images on their own.
- Neither the scripts nor the images should modify the workstation filesystem content beyond the
  ``${BSIM_OUT_PATH}/results/<simulation_id>/`` or ``/tmp/`` folders.
  That is, they should not leave stray files behind.
- If the test scripts or the test binaries create temporary files, they should preferably do so by
  placing them in the ``${BSIM_OUT_PATH}/results/<simulation_id>/`` folder.
  Otherwise they should be named as to avoid conflicts with other test scripts which may be running
  in parallel.
- When running tests that require several consecutive simulations, for ex. if simulating a device
  pairing, powering off, and powering up after as a new simulation,
  they should strive for using separate simulation ids for each simulation part,
  in that way ensuring that the simulation radio activity of each segment can be inspected a
  posteriori.
