.. _module-shell-loader-sample:

Loadable modules with the shell
###############################

Overview
********

This example provides shell access to the module system and provides the
ability to manage loadable code modules in the shell.

Requirements
************

A board with shell capable console. Optionally one that provides flash and
and file system for loading modules from a filesystem.

Building
********

.. code-block:: console

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/module/shell_loader
   :goals: build
   :compact:

Running
*******

Once the board has booted, you will be presented with a shell prompt.
All the module system related commands are available as sub-commands of module.
All file system related commands are available as sub-commands of fs.

Listing modules and their respective memory usage

.. code-block:: console

  module list

Loading a module from the shell as a hex blob

.. code-block:: console

  module load_hex name_of_module <hex_of_elf_file>

Unloading a module

.. code-block:: console

 module unload name_of_module
