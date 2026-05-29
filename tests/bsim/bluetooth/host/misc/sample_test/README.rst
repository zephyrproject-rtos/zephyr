.. _bluetooth_bsim_test_sample:

Bluetooth: Example BabbleSim test
#################################

Abstract
********

This test's purpose is to serve as template for implementing a new BabbleSim Bluetooth test.

BabbleSim_ is :ref:`integrated in zephyr <bsim>` and used for testing the Bluetooth stack.
The tests are implemented in ``tests/bsim/bluetooth``.
They can only be run on Linux.

This sample test uses the ``testlib`` (:zephyr_file:`tests/bluetooth/common/testlib/CMakeLists.txt`)
test library, in order to de-duplicate code that is not relevant to the test in question. Things
like setting up a connection, getting the GATT handle of a characteristic, etc..

Please don't use the ``bs_`` prefix in files or identifiers. It's meant to
namespace the babblesim simulator code.

Reading guide
*************

Read in order:

1. The :ref:`Bsim test documentation <bsim>`.
#. ``test_scripts/run.sh``
#. ``CMakeLists.txt``
#. ``src/dut.c`` and ``src/peer.c``
#. ``src/main.c``

.. _BabbleSim:
   https://BabbleSim.github.io
