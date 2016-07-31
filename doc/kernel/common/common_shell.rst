.. _shell:

Zephyr OS Shell
###############

Overview
********

Zephyr OS Shell enables multiple Zephyr OS modules to use and expose their
shell interface simultaneously.

Each module can support shell functionality dynamically by its Kconfig file,
which enables or disables the shell usage for the module.

Using shell commands
********************

Use one of the following formats:

Specific module's commands
==========================
   `MODULE_NAME COMMAND`
	One of the available modules is “KERNEL”, for the Kernel module.
	More information can be found in :c:macro:`SHELL_REGISTER`.

Help commands
=============
   `help`
	Prints the available modules.
   `help MODULE_NAME`
   	Prints the names of the available commands for the module.
   `help MODULE_NAME COMMAND`
   	Prints help for the module's command (the help should show function
	goal and required parameters).

Select module commands
======================
   `set_module MODULE_NAME`
	Use this command when using the shell only for one module.
	After entering this command, you will not need to enter module
	name in further	commands.
	If the selected module has set a default shell prompt during its
	initialization, the prompt will	be changed to that one.
	Otherwise, the prompt will be changed to the selected module’s name to
	reflect the current module in use.
   `set_module`
	Clears selected module. Restores prompt as well.

Shell configuration
*******************
There are two levels of configuration: Infrastructure level and Module level.

Infrastructure level
====================
The default value for ENABLE_SHELL flag should be considered per product.
This flag enables shell services.
If it is enabled, kernel shell commands are also available for use.
See the :option:`CONFIG_ENABLE_SHELL` Kconfig options for more information.

Module level
============
Each module using shell service should add a unique flag in its Kconfig file.

Example:
CONFIG_SAMPLE_MODULE_USE_SHELL=y

In the module’s code, the shell usage depends on this config parameter.
This module-specific flag should also depend on ENABLE_SHELL flag.

Therefore, there is one global flag, in addition to a unique flag per each
module.
The default value for ENABLE_SHELL flag should be considered per product.

Configuration steps to add shell functionality to a module
==========================================================
 #. Check that ENABLE_SHELL is set to yes.
 #. Add the module unique flag to its Kconfig file.


Writing a shell module
**********************
In order to support shell in your module, the application must do the following:

 #. Module configuration flag:
	Declare a new flag in your module Kconfig file.
	It should depend on `ENABLE_SHELL` flag.

 #. Module registration to shell:
	Add your shell identifier and register its callback functions in the
	shell database using :c:macro:`SHELL_REGISTER`.

 #. Optional:
	:c:func:`shell_register_default_module`

	:c:func:`shell_register_prompt_handler`

	Usage:
		In case of a sample applications as well as test environment,
		user can choose to set a default module in code level.
		In this case, the function shell_register_default_module should
		be called after calling SHELL_REGISTER in application level.
		If the function shell_register_prompt_handler was called
		as well, the prompt will be changed to that one.
		Otherwise, the prompt will be changed to the selected module’s
		name, in order to reflect the current module in use.

	Note:
		Even if a default module was set in code level, it can be
		overwritten by “set_module” shell command.

	When to use shell_register_default_module:

	* Use this command in case of using the shell only for one module.
	  After entering this command, no need to enter module name in further
	  commands.

	* Use this function for shell backward compatibility.

	More details on those optional functions can be found
	in :ref:`shell_api_functions`.


.. _shell_api_functions:

Shell Api Functions
*******************
.. doxygengroup:: _shell_api_functions
   :project: Zephyr
   :content-only:
