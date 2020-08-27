.. _peci-sample:

PECI Interface
####################################

Overview
********

This sample demonstrates how to use the :ref:`PECI API <peci_api>`.
Callbacks are registered that will write to the console indicating PECI events.
These events indicate PECI host interaction.

Building and Running
********************

The sample can be built and executed on boards supporting PECI.
Please connect ensure the microcontroller used is connected to a PECI Host
in order to execute.

Sample output
=============

.. code-block:: console

   PECI test
   Note: You are expected to see several interactions including ID and
   temperature retrieval.
