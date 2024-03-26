.. _json_parser:

JSON Parser
###########

Overview
********

A simple sample to consume json-c parsing/tokener API and convert to
json_object.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/json-c
   :host-os: unix
   :board: nucleo_f429zi
   :goals: run
   :compact:

To build for another board, change "nucleo_f429zi" above to that board's name.

Sample Output
=============

.. code-block:: console

    {
      "question": "Mum, clouds hide alien spaceships don't they ?",
      "answer": "Of course not! (sigh)"
    }
