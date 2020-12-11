.. _kscan-sample:

KSCAN Interface
####################################

Overview
********

This sample demonstrates how to use the :ref:`KSCAN API <kscan_api>`.
Callbacks are registered that will write to the console indicating KSCAN events.
These events indicate key presses and releases.

Building and Running
********************

The sample can be built and executed on boards supporting a Keyboard Matrix.
It requires a correct fixture setup. Please connect a Keyboard Matrix to
exercise the functionality (you need to obtain the right keymap from the vendor
because they vary across different manufactures).
For the correct execution of that sample in sanitycheck, add into boards's
map-file next fixture settings::

      - fixture: fixture_connect_keyboard

Sample output
=============

.. code-block:: console

   KSCAN test with a Keyboard matrix
   Note: You are expected to see several callbacks
   as you press and release keys!
