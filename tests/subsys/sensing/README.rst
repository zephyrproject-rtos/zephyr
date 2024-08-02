.. _sensing_test:

Sensing Subsystem Test Sample
############################

Overview
********

This test sample is used to test the Sensing Subsystem APIs. It is based
on bmi160 emulator on native_sim platform.

Building and Testing
********************

This application can be built and executed on native_sim as follows:

.. code-block:: console
   $ ./scripts/twister -p native_sim -T tests/subsys/sensing
To build for another board, change "native_sim" above to that board's name.

At the current stage, it only supports native_sim.
