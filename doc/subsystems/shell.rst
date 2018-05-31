.. _shell:

Shell
######

Overview
********

The Shell enables multiple subsystem to use and expose their shell interface
simultaneously.

Each subsystem can support shell functionality dynamically by its Kconfig file,
which enables or disables the shell usage for the subsystem.

Using shell commands
********************

Use one of the following formats:

Help commands
=============

``help``
 Prints the list of available modules.

``help MODULE_NAME``
 Prints the names of the available commands for the module.

``help MODULE_NAME COMMAND``
 Prints help for the module's command (the help describes the available
 module commands and required parameters).


Specific module's commands
==========================

A shell interface exposing subsystem features is a shell module, multiple
modules can be available at the same time.

``MODULE_NAME COMMAND``
 One of the available modules is "KERNEL", for `Kernel module commands`_.
 Information about registering a new module and its commands can be
 found in :c:macro:`SHELL_REGISTER` documentation.


Select module commands
======================

``select MODULE_NAME``
 Use this command when using the shell only for one module. After entering this
 command, you will not need to enter the module name in further commands. If
 the selected module has set a default shell prompt during its initialization,
 the prompt will be changed to that one. Otherwise, the prompt will be
 changed to the selected module's name to reflect the current module in use.

``select``
 Clears selected module. Restores prompt as well.


Kernel module commands
======================

When enabled through option :option:`CONFIG_KERNEL_SHELL`, the Kernel
shell module commands display useful kernel-specific information.

You can issue these Kernel module shell commands by specifying
the module and command::

   shell> kernel version

or by first selecting the kernel module, and then issuing the specific
command::

   shell> select kernel
   kernel> version

Here are the Kernel module shell commands:

``version``
 Displays the kernel version number

``uptime``
 Displays the system uptime in milliseconds

``cycles``
 Displays the current time (in cycles), as measured by the system's hardware clock

``threads``
 Displays information about the running threads (if
 :option:`CONFIG_OBJECT_TRACING` and :option:`CONFIG_THREAD_MONITOR` are
 enabled).

``stacks``
 Displays size and use information about the main, idle, interrupt and system
 workqueue call stacks (if :option:`CONFIG_INIT_STACKS` is enabled)

Other commands
==============

``noprompt``
 This command will disable the shell prompt. The shell will still be fully
 functional, but the prompt will not be printed each time the shell expects a
 new command.

Shell configuration
*******************
There are two levels of configuration: Infrastructure level and Module level.

Infrastructure level
====================

The option :option:`CONFIG_CONSOLE_SHELL` enables the shell subsystem and enable the
default features of the shell subsystem.

Module/Subsystem level
======================
Each subsystem using the shell service should add a unique flag in its Kconfig file.

Example:

CONFIG_NET_SHELL=y

In the subsystem's code, the shell usage depends on this config parameter.
This subsystem specific flag should also depend on :option:`CONFIG_CONSOLE_SHELL` flag.

Configuration steps to add shell functionality to a module
==========================================================

 #. Check that :option:`CONFIG_CONSOLE_SHELL` is set to yes.
 #. Add the subsystem unique flag to its Kconfig file.

Writing a shell module
**********************

In order to support shell in your subsystem, the application must do the following:

#. Module configuration flag: Declare a new flag in your subsystem Kconfig file.
   It should depend on :option:`CONFIG_CONSOLE_SHELL` flag.

#. Module registration to shell: Add your shell identifier and register its
   callback functions in the shell database using :c:macro:`SHELL_REGISTER`.

Optionally, you can use one of the following API functions to override default
behavior and settings:

* :c:func:`shell_register_default_module`

* :c:func:`shell_register_prompt_handler`

In case of a sample applications as well as the test environment, you can choose to
set a default module in code level. In this case, the function
:c:func:`shell_register_default_module` should be called after calling SHELL_REGISTER in
application level.  If the function
:c:func:`shell_register_prompt_handler` was called as
well, the prompt will be changed to that one.  Otherwise, the prompt will be
changed to the selected module's name, in order to reflect the current module in
use.


.. note::

   Even if a default module was set in code level, it can be overwritten by
   "select" shell command.

You can use  :c:func:`shell_register_default_module` in the following cases:

* Use this command in case of using the shell only for one module.
  After entering this command, no need to enter module name in further
  commands.

* Use this function for shell backward compatibility.

More details on those optional functions can be found in
:ref:`shell_api_functions`.


.. _shell_api_functions:

Shell API Functions
*******************
.. doxygengroup:: _shell_api_functions
   :project: Zephyr
