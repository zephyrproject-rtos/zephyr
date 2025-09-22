.. zephyr:code-sample:: npm1300_ek
   :name: nPM1300 EK

   Interact with the nPM1300 PMIC using the shell interface.

Overview
********

This sample is provided for evaluation of the :ref:`npm1300_ek`.
The sample provides a shell interface to support the features of the
nPM1300 PMIC, including:

- Regulators (BUCK1/2, LDO1/2)
- GPIO

Requirements
************

The shield needs to be wired to a host board supporting the Arduino connector.

Examples and images to follow

Building and Running
********************

The sample is designed so that it can run on any platform. For example, when
building for the nRF52 DK, the following command can be used:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/npm1300_ek
   :board: nrf52dk/nrf52832
   :goals: build
   :compact:

Note that this sample automatically sets ``SHIELD`` to ``npm1300_ek``. Once
flashed, you should boot into the shell interface. The ``regulator`` command is
provided to test the PMIC. Below you can find details for each subcommand.

Regulators
==========

The ``regulator`` shell interface provides several subcommand to test
the regulators embedded in the PMIC. Below you can find some command examples.

.. code-block:: bash

   # list all the supported voltages by BUCK1
   regulator vlist BUCK1
   1.000 V
   1.100 V
   ...

.. code-block:: bash

   # enable BUCK2
   regulator enable BUCK2
   # disable BUCK2
   regulator disable BUCK2

.. code-block:: bash

   # set BUCK2 voltage to exactly 2V
   regulator vset BUCK2 2V
   # obtain the actual BUCK1 configured voltage
   regulator vget BUCK1
   1.800 V
   # set BUCK1 voltage to a value between 2.35V and 2.45V
   regulator set BUCK1 2.35V 2.45V
   # obtain the actual BUCK1 configured voltage
   regulator get BUCK1
   2.400 V
