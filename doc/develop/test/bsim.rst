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
When we build Zephyr targeting an :ref:`nrf52_bsim<nrf52_bsim>` board we produce a Linux
executable, which includes the application, Zephyr OS, and models of the HW.

When there is radio activity, this Linux executable will connect to the BabbleSim Phy simulation
to simulate the radio channel.

In the BabbleSim documentation you can find more information on how to
`get <https://babblesim.github.io/fetching.html>`_ and
`build <https://babblesim.github.io/building.html>`_ the simulator.
In the :ref:`nrf52_bsim<nrf52_bsim>` board documentation you can find more information about how
to build Zephyr targeting that particular board, and a few examples.

Types of tests
**************

Tests without radio activity: bsim tests with twister
-----------------------------------------------------

The :ref:`bsim boards<bsim boards>` can be used without radio activity, and in that case, it is not
necessary to connect them to a phyisical layer simulation. Thanks to this, this target boards can
be used just like :ref:`native_posix<native_posix>` with :ref:`twister <twister_script>`,
to run all standard Zephyr twister tests, but with models of a real SOC HW, and their drivers.

Tests with radio activity
-------------------------

When there is radio activity, BabbleSim tests require at the very least a physical layer simulation
running, and most, more than 1 simulated device. Due to this, these tests are not build and run
with twister, but with a dedicated set of tests scripts.

These tests are kept in the :code:`tests/bsim/` folder. There you can find a README with more
information about how to build and run them, as well as the convention they follow.

There are two main sets of tests of these type:

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

As the :ref:`nrf52_bsim<nrf52_bsim>` is based on the POSIX architecture, you can easily collect test
coverage information.

You can use the script :code:`tests/bsim/generate_coverage_report.sh` to generate an html
coverage report from tests.

Check :ref:`the page on coverage generation <coverage_posix>` for more info.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _EDTT:
   https://github.com/EDTTool/EDTT
