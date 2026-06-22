.. zephyr:code-sample:: regulator_shell
   :name: Regulator Shell Sample

   Interact with regulators using the shell module.

Overview
********

This sample lets users interact with their regulator devices using the shell.

Building and Running
********************

Build and flash as follows, replacing ``arduino_nano_matter`` with your board:

.. zephyr-app-commands::
   :zephyr-app: samples/regulator/regulator_shell
   :board: arduino_nano_matter
   :goals: build flash
   :compact:

Shell Module Command Help
=========================

You can access the regulator shell module by typing ``regulator`` in the shell.
For all available commands, type ``help`` in the shell.
