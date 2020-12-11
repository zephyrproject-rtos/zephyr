.. _ps2-sample:

PS/2 Interface
####################################

Overview
********

This sample demonstrates how to use the :ref:`PS/2 API <ps2_api>`.
Callbacks are registered that will write to the console indicating PS2 events.
These events indicate mouse configuration responses and mouse interaction.

Building and Running
********************

The sample can be built and executed on boards supporting PS/2.
It requires a correct fixture setup. Please connect a PS/2 mouse in order to
exercise the functionality.
For the correct execution of that sample in sanitycheck, add into boards's
map-file next fixture settings::

      - fixture: fixture_connect_mouse

Sample output
=============

.. code-block:: console

   PS/2 test with mouse
   Note: You are expected to see several interrupts
   as you configure/move the mouse!
