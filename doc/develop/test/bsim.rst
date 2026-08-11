.. _bsim:

BabbleSim
#########

BabbleSim and Zephyr
********************

In the Zephyr project we use the `Babblesim`_ simulator to test some of the Zephyr radio protocols,
including the Bluetooth LE stack, 802.15.4, and some of the networking stack.

BabbleSim_ is a physical layer simulator, which in combination with the Zephyr
:ref:`bsim boards<bsim boards>`
can be used to simulate a network of Bluetooth LE and 15.4 devices.
When we build Zephyr targeting a :ref:`bsim board<bsim boards>` we produce a Linux
executable, which includes the application, Zephyr OS, and models of the HW.

When there is radio activity, this Linux executable will connect to the BabbleSim Phy simulation
to simulate the radio channel.

In the BabbleSim documentation you can find more information on how to
`get <https://babblesim.github.io/fetching.html>`_ and
`build <https://babblesim.github.io/building.html>`_ the simulator.
In the :ref:`nrf52_bsim<nrf52_bsim>`, :ref:`nrf5340bsim<nrf5340bsim>`,
and :ref:`nrf54l15bsim<nrf54l15bsim>` boards documentation
you can find more information about how to build Zephyr targeting these particular boards,
and a few examples.

Types of tests
**************

Tests without radio activity: bsim tests with twister
=====================================================

The :ref:`bsim boards<bsim boards>` can be used without radio activity, and in that case, it is not
necessary to connect them to a physical layer simulation. Thanks to this, these target boards can
be used just like :zephyr:board:`native_sim<native_sim>` with :ref:`twister <twister_script>`,
to run all standard Zephyr twister tests, but with models of a real SOC HW, and their drivers.

Tests with radio activity
=========================

When there is radio activity, BabbleSim tests require at the very least a physical layer simulation
running, and most, more than 1 simulated device. Due to this, these tests are executed by running
a dedicated script for each test which starts both the needed simulated devices, and the physical
layer executable with their required parameters, and any other tool which may be required.

To be able to run them with twister, the :ref:`bsim harness <twister_bsim_harness>` should be used.

These tests are kept in the :zephyr_file:`tests/bsim/` folder.

Check the sub-sections below for more information about how to build and run them, as well as the
conventions they follow.

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

As the :ref:`bsim boards <bsim boards>` are based on the POSIX architecture, you can easily collect
test coverage information.

Check :ref:`the page on coverage generation <coverage_posix>` for more info, and note you can
just pass the ``--coverage`` option to twister to automatically build with
:kconfig:option:`CONFIG_COVERAGE` and generate the coverage report.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _EDTT:
   https://github.com/EDTTool/EDTT

Building and running the tests
******************************

Check the :ref:`nrf52_bsim <nrf52bsim_build_and_run>` page for instructions on how to set up the
simulator.

You can build and run these tests with :ref:`twister <twister_script>`.
To run multidevice tests, you will need to pass twister the option ``--fixture bsim_multi_test``.

For example, from ${ZEPHYR_BASE}, you can build and run one of the BT tests with:

.. code-block:: bash

   twister -p nrf52_bsim/native -T tests/bsim/bluetooth/host/adv/chain/ --fixture bsim_multi_test

If the test binaries have already been built, you can also run a test directly using its individual
test script. For example:

.. code-block:: bash

   BOARD=nrf52_bsim/native tests/bsim/bluetooth/host/adv/chain/tests_scripts/adv_chain.sh

:ref:`Twister command line options <twister_commandline_options>` like ``-n, --no-clean``,
``--aggressive-no-clean``, or ``-b, --build-only`` are likely to be useful when debugging or
fixing issues.

Legacy batch scripts
====================

Until the twister bsim harness was added, building and running multidevice bsim tests relied
on two scripts in the :zephyr_file:`tests/bsim/` folder: ``compile.sh`` and ``run_parallel.sh``.
These scripts were used by the CI system to build the needed images and execute these tests in
batch. They were also meant as utilities for users to build and execute their tests.
These scripts can still be used, but users are recommended to transition to using
:ref:`twister <twister_script>` and ``tests.yaml`` definitions.

These scripts expect a few environment variables to be set.
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

Test code
---------

See the :zephyr_file:`Bluetooth sample test <tests/bsim/bluetooth/host/misc/sample_test/README.rst>` for conventions that apply to test
code.

Test scripts
------------

Please follow the existing conventions and do not design one-off bespoke runners (e.g. a python
script, or another shell abstraction).

The rationale is that it is easier and faster for the maintainers to perform tree-wide updates for
build system or compatibility changes if the tests are run in the same manner, with the same
variables, etc..

If you have a good idea for improving your test script, please make a PR changing *all* the test
scripts in order to benefit everyone and conserve homogeneity. You can of course discuss it first in
an RFC issue or on the babblesim discord channel.

Scripts starting with an underscore (``_``) are not automatically discovered and run. They can serve
as either helper functions for the main script, or can be used for local development utilities, e.g.
building and running tests locally, debugging, etc..

Here are the conventions:

- Each test is defined by a shell script with the extension ``.sh``, in a subfolder called
  ``tests_scripts/``.
- It is recommended to run a single test per script file. It allows for better parallelization of
  the runs in CI.
- Scripts expect that the binaries they require are already built. They should not compile binaries.
- Scripts will spawn the processes for every simulated device and the physical layer simulation.
- Scripts must return 0 to the invoking shell if the test passes, and not 0 if the test fails.
- Each test must have a unique simulation id, to enable running different tests in parallel.
- Neither the scripts nor the images should modify the workstation filesystem content beyond the
  ``${BSIM_OUT_PATH}/results/<simulation_id>/`` or ``/tmp/`` folders.
  That is, they should not leave stray files behind.
- Tests that require several consecutive simulations (e.g, if simulating a device pairing, powering
  off, and powering up after as a new simulation) should use separate simulation ids for each
  simulation segment, ensuring that the radio activity of each segment can be inspected a
  posteriori.
- Avoid overly long tests. If the test takes over 20 seconds of runtime, consider if it is possible
  to split it in several separate tests.
- If the test takes over 5 seconds, set ``EXECUTE_TIMEOUT`` to a value that is at least 5 times
  bigger than the measured run-time.
- Do not set ``EXECUTE_TIMEOUT`` to a value lower than the default.
- Tests should not be overly verbose: less than a hundred lines are expected on the outputs. Do make
  use of ``LOG_DBG()`` extensively, but don't enable the ``DBG`` log level by default.
