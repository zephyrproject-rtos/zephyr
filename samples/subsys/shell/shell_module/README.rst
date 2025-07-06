.. zephyr:code-sample:: shell-module
   :name: Custom Shell module
   :relevant-api: shell_api

   Register shell commands using the Shell API

Overview
********

This is a simple application demonstrating how to write and register commands
using the :ref:`Shell API <shell_api>`:

Register Static commands
   ``version`` is a static command that prints the kernel version.

Conditionally Register commands
   ``login`` and ``logout`` are conditionally registered commands depending
   on :kconfig:option:`CONFIG_SHELL_START_OBSCURED`.

Register Dynamic commands
   See ``dynamic`` command and :zephyr_file:`samples/subsys/shell/shell_module/src/dynamic_cmd.c`
   for details on how dynamic commands are implemented.

Register Dictionary commands
   ``dictionary`` implements subsect of dictionary commands.

Set a Bypass callback
   ``bypass`` implements the bypass callback.

Set a Login command
   ``login`` and ``logout`` implement the login and logout mechanism, respectively.

Obscure user-input with asterisks
   ``login`` and ``logout`` implement the feature of enabling and disabling
   this functionality, respectively.

Requirements
************

* A target configured with the shell interface, exposed through any of
  its :ref:`backends <backends>`.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/shell/shell_module`
in the Zephyr tree.

The sample can be built for several platforms.

Emulation Targets
=================

The sample may run on emulation targets. The following commands build the
application for the qemu_x86.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

After running the application, the console displays the shell interface, and
shows the shell prompt, at which point the user may start the interaction.

On-Hardware
===========

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: nrf52840dk/nrf52840
   :goals: flash
   :compact:

Sample Output
*************

.. code-block:: console

   uart:~$
     bypass              clear               date
     demo                device              devmem
     dynamic             help                history
     kernel              log                 log_test
     rem                 resize              retval
     section_cmd         shell               shell_uart_release
     stats               version
   uart:~$ demo
   demo - Demo commands
   Subcommands:
     dictionary  : Dictionary commands
     hexdump     : Hexdump params command.
     params      : Print params command.
     ping        : Ping command.
     board       : Show board name command.
   uart:~$ dynamic
   dynamic - Demonstrate dynamic command usage.
   Subcommands:
     add      : Add a new dynamic command.
               Example usage: [ dynamic add test ] will add a dynamic command
               'test'.
               In this example, command name length is limited to 32 chars. You can
               add up to 20 commands. Commands are automatically sorted to ensure
               correct shell completion.
     execute  : Execute a command.
     remove   : Remove a command.
     show     : Show all added dynamic commands.
   uart:~$

Details on Shell Subsystem
==========================

For more details on the Shell subsystem, check the general :ref:`Shell documentation <shell_api>`.
