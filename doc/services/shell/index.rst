.. _shell_api:

Shell
######

.. contents::
    :local:
    :depth: 2

Overview
********

This module allows you to create and handle a shell with a user-defined command
set. You can use it in examples where more than simple button or LED user
interaction is required. This module is a Unix-like shell with these features:

* Support for multiple instances.
* Advanced cooperation with the :ref:`logging_api`.
* Support for static and dynamic commands.
* Support for dictionary commands.
* Smart command completion with the :kbd:`Tab` key.
* Built-in commands: :command:`clear`, :command:`shell`, :command:`colors`,
  :command:`echo`, :command:`history` and :command:`resize`.
* Viewing recently executed commands using keys: :kbd:`↑` :kbd:`↓` or meta keys.
* Text edition using keys: :kbd:`←`, :kbd:`→`, :kbd:`Backspace`,
  :kbd:`Delete`, :kbd:`End`, :kbd:`Home`, :kbd:`Insert`.
* Support for ANSI escape codes: ``VT100`` and ``ESC[n~`` for cursor control
  and color printing.
* Support for editing multiline commands.
* Built-in handler to display help for the commands.
* Support for wildcards: ``*`` and ``?``.
* Support for meta keys.
* Support for getopt and getopt_long.
* Kconfig configuration to optimize memory usage.

.. note::
	Some of these features have a significant impact on RAM and flash usage,
	but many can be disabled when not needed.  To default to options which
	favor reduced RAM and flash requirements instead of features, you should
	enable :kconfig:option:`CONFIG_SHELL_MINIMAL` and selectively enable just the
	features you want.

The module can be connected to any transport for command input and output.
At this point, the following transport layers are implemented:

* MQTT
* Segger RTT
* SMP
* Telnet
* UART
* USB
* DUMMY - not a physical transport layer.

Connecting to Segger RTT via TCP (on macOS, for example)
========================================================

On macOS JLinkRTTClient won't let you enter input. Instead, please use following
procedure:

* Open up a first Terminal window and enter:

  .. code-block:: none

     JLinkRTTLogger -Device NRF52840_XXAA -RTTChannel 1 -if SWD -Speed 4000 ~/rtt.log

  (change device if required)

* Open up a second Terminal window and enter:

  .. code-block:: none

     nc localhost 19021

* Now you should have a network connection to RTT that will let you enter input
  to the shell.


Telnet Backend
==============

Enabling :kconfig:option:`CONFIG_SHELL_BACKEND_TELNET` will allow users to use telnet
as a shell backend. Connecting to it can be done using PuTTY or any ``telnet`` client.
For example:

.. code-block:: none

  telnet <ip address> <port>

By default the telnet client won't handle telnet commands and configuration. Although
command support can be enabled with :kconfig:option:`CONFIG_SHELL_TELNET_SUPPORT_COMMAND`.
This will give the telnet client access to a very limited set of supported commands but
still can be turned on if needed. One of the command options it supports is the ``ECHO``
option. This will allow the client to be in character mode (character at a time),
similar to a UART backend in that regard. This will make the client send a character
as soon as it is typed having the effect of increasing the network traffic
considerably. For that cost, it will enable the line editing,
`tab completion <tab-feature_>`_, and `history <history-feature_>`_
features of the shell.


Commands
********

Shell commands are organized in a tree structure and grouped into the following
types:

* Root command (level 0): Gathered and alphabetically sorted in a dedicated
  memory section.
* Static subcommand (level > 0): Number and syntax must be known during compile
  time. Created in the software module.
* Dynamic subcommand (level > 0): Number and syntax does not need to be known
  during compile time. Created in the software module.


Creating commands
=================

Use the following macros for adding shell commands:

* :c:macro:`SHELL_CMD_REGISTER` - Create root command. All root commands must
  have different name.
* :c:macro:`SHELL_COND_CMD_REGISTER` - Conditionally (if compile time flag is
  set) create root command. All root commands must have different name.
* :c:macro:`SHELL_CMD_ARG_REGISTER` - Create root command with arguments.
  All root commands must have different name.
* :c:macro:`SHELL_COND_CMD_ARG_REGISTER` - Conditionally (if compile time flag
  is set) create root command with arguments. All root commands must have
  different name.
* :c:macro:`SHELL_CMD` - Initialize a command.
* :c:macro:`SHELL_COND_CMD` - Initialize a command if compile time flag is set.
* :c:macro:`SHELL_EXPR_CMD` - Initialize a command if compile time expression is
  non-zero.
* :c:macro:`SHELL_CMD_ARG` - Initialize a command with arguments.
* :c:macro:`SHELL_COND_CMD_ARG` - Initialize a command with arguments if compile
  time flag is set.
* :c:macro:`SHELL_EXPR_CMD_ARG` - Initialize a command with arguments if compile
  time expression is non-zero.
* :c:macro:`SHELL_STATIC_SUBCMD_SET_CREATE` - Create a static subcommands
  array.
* :c:macro:`SHELL_SUBCMD_DICT_SET_CREATE` - Create a dictionary subcommands
  array.
* :c:macro:`SHELL_DYNAMIC_CMD_CREATE` - Create a dynamic subcommands array.

Commands can be created in any file in the system that includes
:zephyr_file:`include/zephyr/shell/shell.h`. All created commands are available for all
shell instances.

Static commands
---------------

Example code demonstrating how to create a root command with static
subcommands.

.. image:: images/static_cmd.PNG
      :align: center
      :alt: Command tree with static commands.

.. code-block:: c

	/* Creating subcommands (level 1 command) array for command "demo". */
	SHELL_STATIC_SUBCMD_SET_CREATE(sub_demo,
		SHELL_CMD(params, NULL, "Print params command.",
						       cmd_demo_params),
		SHELL_CMD(ping,   NULL, "Ping command.", cmd_demo_ping),
		SHELL_SUBCMD_SET_END
	);
	/* Creating root (level 0) command "demo" */
	SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

Example implementation can be found under following location:
:zephyr_file:`samples/subsys/shell/shell_module/src/main.c`.

Dictionary commands
===================
This is a special kind of static commands. Dictionary commands can be used
every time you want to use a pair: (string <-> corresponding data) in
a command handler. The string is usually a verbal description of a given data.
The idea is to use the string as a command syntax that can be prompted by the
shell and corresponding data can be used to process the command.

Let's use an example. Suppose you created a command to set an ADC gain.
It is a perfect place where a dictionary can be used. The dictionary would
be a set of pairs: (string: gain_value, int: value) where int value could
be used with the ADC driver API.

Abstract code for this task would look like this:

.. code-block:: c

	static int gain_cmd_handler(const struct shell *sh,
				    size_t argc, char **argv, void *data)
	{
		int gain;

		/* data is a value corresponding to called command syntax */
		gain = (int)data;
		adc_set_gain(gain);

		shell_print(sh, "ADC gain set to: %s\n"
				   "Value send to ADC driver: %d",
				   argv[0],
				   gain);

		return 0;
	}

	SHELL_SUBCMD_DICT_SET_CREATE(sub_gain, gain_cmd_handler,
		(gain_1, 1, "gain 1"), (gain_2, 2, "gain 2"),
		(gain_1_2, 3, "gain 1/2"), (gain_1_4, 4, "gain 1/4")
	);
	SHELL_CMD_REGISTER(gain, &sub_gain, "Set ADC gain", NULL);


This is how it would look like in the shell:

.. image:: images/dict_cmd.png
      :align: center
      :alt: Dictionary commands example.

Dynamic commands
----------------

Example code demonstrating how to create a root command with static and dynamic
subcommands. At the beginning dynamic command list is empty. New commands
can be added by typing:

.. code-block:: none

	dynamic add <new_dynamic_command>

Newly added commands can be prompted or autocompleted with the :kbd:`Tab` key.

.. image:: images/dynamic_cmd.PNG
      :align: center
      :alt: Command tree with static and dynamic commands.

.. code-block:: c

	/* Buffer for 10 dynamic commands */
	static char dynamic_cmd_buffer[10][50];

	/* commands counter */
	static uint8_t dynamic_cmd_cnt;

	/* Function returning command dynamically created
	 * in  dynamic_cmd_buffer.
	 */
	static void dynamic_cmd_get(size_t idx,
				    struct shell_static_entry *entry)
	{
		if (idx < dynamic_cmd_cnt) {
			entry->syntax = dynamic_cmd_buffer[idx];
			entry->handler  = NULL;
			entry->subcmd = NULL;
			entry->help = "Show dynamic command name.";
		} else {
			/* if there are no more dynamic commands available
			 * syntax must be set to NULL.
			 */
			entry->syntax = NULL;
		}
	}

	SHELL_DYNAMIC_CMD_CREATE(m_sub_dynamic_set, dynamic_cmd_get);
	SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_dynamic,
		SHELL_CMD(add, NULL,"Add new command to dynamic_cmd_buffer and"
			  " sort them alphabetically.",
			  cmd_dynamic_add),
		SHELL_CMD(execute, &m_sub_dynamic_set,
			  "Execute a command.", cmd_dynamic_execute),
		SHELL_CMD(remove, &m_sub_dynamic_set,
			  "Remove a command from dynamic_cmd_buffer.",
			  cmd_dynamic_remove),
		SHELL_CMD(show, NULL,
			  "Show all commands in dynamic_cmd_buffer.",
			  cmd_dynamic_show),
		SHELL_SUBCMD_SET_END
	);
	SHELL_CMD_REGISTER(dynamic, &m_sub_dynamic,
		   "Demonstrate dynamic command usage.", cmd_dynamic);

Example implementation can be found under following location:
:zephyr_file:`samples/subsys/shell/shell_module/src/dynamic_cmd.c`.

Commands execution
==================

Each command or subcommand may have a handler. The shell executes the handler
that is found deepest in the command tree and further subcommands (without a
handler) are passed as arguments. Characters within parentheses are treated
as one argument. If shell won't find a handler it will display an error message.

Commands can be also executed from a user application using any active backend
and a function :c:func:`shell_execute_cmd`, as shown in this example:

.. code-block:: c

	int main(void)
	{
		/* Below code will execute "clear" command on a DUMMY backend */
		shell_execute_cmd(NULL, "clear");

		/* Below code will execute "shell colors off" command on
		 * an UART backend
		 */
		shell_execute_cmd(shell_backend_uart_get_ptr(),
				  "shell colors off");
	}

Enable the DUMMY backend by setting the Kconfig
:kconfig:option:`CONFIG_SHELL_BACKEND_DUMMY` option.

Commands execution example
--------------------------

Let's assume a command structure as in the following figure, where:

* :c:macro:`root_cmd` - root command without a handler
* :c:macro:`cmd_xxx_h` - command has a handler
* :c:macro:`cmd_xxx` - command does not have a handler

.. image:: images/execution.png
      :align: center
      :alt: Command tree with static commands.

Example 1
^^^^^^^^^
Sequence: :c:macro:`root_cmd` :c:macro:`cmd_1_h` :c:macro:`cmd_12_h`
:c:macro:`cmd_121_h` :c:macro:`parameter` will execute command
:c:macro:`cmd_121_h` and :c:macro:`parameter` will be passed as an argument.

Example 2
^^^^^^^^^
Sequence: :c:macro:`root_cmd` :c:macro:`cmd_2` :c:macro:`cmd_22_h`
:c:macro:`parameter1` :c:macro:`parameter2` will execute command
:c:macro:`cmd_22_h` and :c:macro:`parameter1` :c:macro:`parameter2`
will be passed as an arguments.

Example 3
^^^^^^^^^
Sequence: :c:macro:`root_cmd` :c:macro:`cmd_1_h` :c:macro:`parameter1`
:c:macro:`cmd_121_h` :c:macro:`parameter2` will execute command
:c:macro:`cmd_1_h` and :c:macro:`parameter1`, :c:macro:`cmd_121_h` and
:c:macro:`parameter2` will be passed as an arguments.

Example 4
^^^^^^^^^
Sequence: :c:macro:`root_cmd` :c:macro:`parameter` :c:macro:`cmd_121_h`
:c:macro:`parameter2` will not execute any command.


Command handler
----------------

Simple command handler implementation:

.. code-block:: c

	static int cmd_handler(const struct shell *sh, size_t argc,
				char **argv)
	{
		ARG_UNUSED(argc);
		ARG_UNUSED(argv);

		shell_fprintf(shell, SHELL_INFO, "Print info message\n");

		shell_print(sh, "Print simple text.");

		shell_warn(sh, "Print warning text.");

		shell_error(sh, "Print error text.");

		return 0;
	}

Function :c:func:`shell_fprintf` or the shell print macros:
:c:macro:`shell_print`, :c:macro:`shell_info`, :c:macro:`shell_warn` and
:c:macro:`shell_error` can be used from the command handler or from threads,
but not from an interrupt context. Instead, interrupt handlers should use
:ref:`logging_api` for printing.

Command help
------------

Every user-defined command or subcommand can have its own help description.
The help for commands and subcommands can be created with respective macros:
:c:macro:`SHELL_CMD_REGISTER`, :c:macro:`SHELL_CMD_ARG_REGISTER`,
:c:macro:`SHELL_CMD`, and :c:macro:`SHELL_CMD_ARG`.

Shell prints this help message when you call a command
or subcommand with ``-h`` or ``--help`` parameter.

Parent commands
---------------

In the subcommand handler, you can access both the parameters passed to
commands or the parent commands, depending on how you index ``argv``.

* When indexing ``argv`` with positive numbers, you can access the parameters.
* When indexing ``argv`` with negative numbers, you can access the parent
  commands.
* The subcommand to which the handler belongs has the ``argv`` index of 0.

.. code-block:: c

	static int cmd_handler(const struct shell *sh, size_t argc,
			       char **argv)
	{
		ARG_UNUSED(argc);

		/* If it is a subcommand handler parent command syntax
		 * can be found using argv[-1].
		 */
		shell_print(sh, "This command has a parent command: %s",
			      argv[-1]);

		/* Print this command syntax */
		shell_print(sh, "This command syntax is: %s", argv[0]);

		/* Print first argument */
		shell_print(sh, "%s", argv[1]);

		return 0;
	}

Built-in commands
=================

These commands are activated by :kconfig:option:`CONFIG_SHELL_CMDS` set to ``y``.

* :command:`clear` - Clears the screen.
* :command:`history` - Shows the recently entered commands.
* :command:`resize` - Must be executed when terminal width is different than 80
  characters or after each change of terminal width. It ensures proper
  multiline text display and :kbd:`←`, :kbd:`→`, :kbd:`End`, :kbd:`Home` keys
  handling. Currently this command works only with UART flow control switched
  on. It can be also called with a subcommand:

	* :command:`default` - Shell will send terminal width = 80 to the
	  terminal and assume successful delivery.

  These command needs extra activation:
  :kconfig:option:`CONFIG_SHELL_CMDS_RESIZE` set to ``y``.
* :command:`select` - It can be used to set new root command. Exit to main
  command tree is with alt+r. This command needs extra activation:
  :kconfig:option:`CONFIG_SHELL_CMDS_SELECT` set to ``y``.
* :command:`shell` - Root command with useful shell-related subcommands like:

	* :command:`echo` - Toggles shell echo.
        * :command:`colors` - Toggles colored syntax. This might be helpful in
          case of Bluetooth shell to limit the amount of transferred bytes.
	* :command:`stats` - Shows shell statistics.

.. _tab-feature:

Tab Feature
***********

The Tab button can be used to suggest commands or subcommands. This feature
is enabled by :kconfig:option:`CONFIG_SHELL_TAB` set to ``y``.
It can also be used for partial or complete auto-completion of commands.
This feature is activated by
:kconfig:option:`CONFIG_SHELL_TAB_AUTOCOMPLETION` set to ``y``.
When user starts writing a command and presses the :kbd:`Tab` button then
the shell will do one of 3 possible things:

* Autocomplete the command.
* Prompts available commands and if possible partly completes the command.
* Will not do anything if there are no available or matching commands.

.. image:: images/tab_prompt.png
      :align: center
      :alt: Tab Feature usage example

.. _history-feature:

History Feature
***************

This feature enables commands history in the shell. It is activated by:
:kconfig:option:`CONFIG_SHELL_HISTORY` set to ``y``. History can be accessed
using keys: :kbd:`↑` :kbd:`↓` or :kbd:`Ctrl+n` and :kbd:`Ctrl+p`
if meta keys are active.
Number of commands that can be stored depends on size
of :kconfig:option:`CONFIG_SHELL_HISTORY_BUFFER` parameter.

Wildcards Feature
*****************

The shell module can handle wildcards. Wildcards are interpreted correctly
when expanded command and its subcommands do not have a handler. For example,
if you want to set logging level to ``err`` for the ``app`` and ``app_test``
modules you can execute the following command:

.. code-block:: none

	log enable err a*

.. image:: images/wildcard.png
      :align: center
      :alt: Wildcard usage example

This feature is activated by :kconfig:option:`CONFIG_SHELL_WILDCARD` set to ``y``.

Meta Keys Feature
*****************

The shell module supports the following meta keys:

.. list-table:: Implemented meta keys
   :widths: 10 40
   :header-rows: 1

   * - Meta keys
     - Action
   * - :kbd:`Ctrl+a`
     - Moves the cursor to the beginning of the line.
   * - :kbd:`Ctrl+b`
     - Moves the cursor backward one character.
   * - :kbd:`Ctrl+c`
     - Preserves the last command on the screen and starts a new command in
       a new line.
   * - :kbd:`Ctrl+d`
     - Deletes the character under the cursor.
   * - :kbd:`Ctrl+e`
     - Moves the cursor to the end of the line.
   * - :kbd:`Ctrl+f`
     - Moves the cursor forward one character.
   * - :kbd:`Ctrl+k`
     - Deletes from the cursor to the end of the line.
   * - :kbd:`Ctrl+l`
     - Clears the screen and leaves the currently typed command at the top of
       the screen.
   * - :kbd:`Ctrl+n`
     - Moves in history to next entry.
   * - :kbd:`Ctrl+p`
     - Moves in history to previous entry.
   * - :kbd:`Ctrl+u`
     - Clears the currently typed command.
   * - :kbd:`Ctrl+w`
     - Removes the word or part of the word to the left of the cursor. Words
       separated by period instead of space are treated as one word.
   * - :kbd:`Alt+b`
     - Moves the cursor backward one word.
   * - :kbd:`Alt+f`
     - Moves the cursor forward one word.

This feature is activated by :kconfig:option:`CONFIG_SHELL_METAKEYS` set to ``y``.

Getopt Feature
*****************

Some shell users apart from subcommands might need to use options as well.
the arguments string, looking for supported options. Typically, this task
is accomplished by the ``getopt`` family functions.

For this purpose shell supports the getopt and getopt_long libraries available
in the FreeBSD project. This feature is activated by:
:kconfig:option:`CONFIG_GETOPT` set to ``y`` and :kconfig:option:`CONFIG_GETOPT_LONG`
set to ``y``.

This feature can be used in thread safe as well as non thread safe manner.
The former is full compatible with regular getopt usage while the latter
a bit differs.

An example non-thread safe usage:

.. code-block:: c

  char *cvalue = NULL;
  while ((char c = getopt(argc, argv, "abhc:")) != -1) {
        switch (c) {
        case 'c':
                cvalue = optarg;
                break;
        default:
                break;
        }
  }

An example thread safe usage:

.. code-block:: c

  char *cvalue = NULL;
  struct getopt_state *state;
  while ((char c = getopt(argc, argv, "abhc:")) != -1) {
        state = getopt_state_get();
        switch (c) {
        case 'c':
                cvalue = state->optarg;
                break;
        default:
                break;
        }
  }

Thread safe getopt functionality is activated by
:kconfig:option:`CONFIG_SHELL_GETOPT` set to ``y``.

Obscured Input Feature
**********************

With the obscured input feature, the shell can be used for implementing a login
prompt or other user interaction whereby the characters the user types should
not be revealed on screen, such as when entering a password.

Once the obscured input has been accepted, it is normally desired to return the
shell to normal operation.  Such runtime control is possible with the
``shell_obscure_set`` function.

An example of login and logout commands using this feature is located in
:zephyr_file:`samples/subsys/shell/shell_module/src/main.c` and the config file
:zephyr_file:`samples/subsys/shell/shell_module/prj_login.conf`.

This feature is activated upon startup by :kconfig:option:`CONFIG_SHELL_START_OBSCURED`
set to ``y``. With this set either way, the option can still be controlled later
at runtime. :kconfig:option:`CONFIG_SHELL_CMDS_SELECT` is useful to prevent entry
of any other command besides a login command, by means of the
``shell_set_root_cmd`` function. Likewise, :kconfig:option:`CONFIG_SHELL_PROMPT_UART`
allows you to set the prompt upon startup, but it can be changed later with the
``shell_prompt_change`` function.

Shell Logger Backend Feature
****************************

Shell instance can act as the :ref:`logging_api` backend. Shell ensures that log
messages are correctly multiplexed with shell output. Log messages from logger
thread are enqueued and processed in the shell thread. Logger thread will block
for configurable amount of time if queue is full, blocking logger thread context
for that time. Oldest log message is removed from the queue after timeout and
new message is enqueued. Use the ``shell stats show`` command to retrieve
number of log messages dropped by the shell instance. Log queue size and timeout
are :c:macro:`SHELL_DEFINE` arguments.

This feature is activated by: :kconfig:option:`CONFIG_SHELL_LOG_BACKEND` set to ``y``.

.. warning::
	Enqueuing timeout must be set carefully when multiple backends are used
	in the system. The shell instance could	have a slow transport or could
	block, for example, by a UART with hardware flow control. If timeout is
	set too high, the logger thread could be blocked and impact other logger
	backends.

.. warning::
	As the shell is a complex logger backend, it can not output logs if
	the application crashes before the shell thread is running. In this
	situation, you can enable one of the simple logging backends instead,
	such as UART (:kconfig:option:`CONFIG_LOG_BACKEND_UART`) or
	RTT (:kconfig:option:`CONFIG_LOG_BACKEND_RTT`), which are available earlier
	during system initialization.

RTT Backend Channel Selection
*****************************

Instead of using the shell as a logger backend, RTT shell backend and RTT log
backend can also be used simultaneously, but over different channels. By
separating them, the log can be captured or monitored without shell output or
the shell may be scripted without log interference. Enabling both the Shell RTT
backend and the Log RTT backend does not work by default, because both default
to channel ``0``. There are two options:

1. The Shell buffer can use an alternate channel, for example using
:kconfig:option:`CONFIG_SHELL_BACKEND_RTT_BUFFER` set to ``1``.
This allows monitoring the log using `JLinkRTTViewer
<https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/#j-link-rtt-viewer>`_
while a script interfaces over channel 1.

2. The Log buffer can use an alternate channel, for example using
:kconfig:option:`CONFIG_LOG_BACKEND_RTT_BUFFER` set to ``1``.
This allows interactive use of the shell through JLinkRTTViewer, while the log
is written to file.

.. warning::
	Regardless of the channel selection, the RTT log backend must be explicitly
	enabled using :kconfig:option:`CONFIG_LOG_BACKEND_RTT` set to ``y``, because it
	defaults to ``n`` when the Shell RTT backend is also enabled using
	:kconfig:option:`CONFIG_SHELL_BACKEND_RTT` being set to ``y``.

Usage
*****

To create a new shell instance user needs to activate requested
backend using ``menuconfig``.

The following code shows a simple use case of this library:

.. code-block:: c

	int main(void)
	{

	}

	static int cmd_demo_ping(const struct shell *sh, size_t argc,
				 char **argv)
	{
		ARG_UNUSED(argc);
		ARG_UNUSED(argv);

		shell_print(sh, "pong");
		return 0;
	}

	static int cmd_demo_params(const struct shell *sh, size_t argc,
				   char **argv)
	{
		int cnt;

		shell_print(sh, "argc = %d", argc);
		for (cnt = 0; cnt < argc; cnt++) {
			shell_print(sh, "  argv[%d] = %s", cnt, argv[cnt]);
		}
		return 0;
	}

	/* Creating subcommands (level 1 command) array for command "demo". */
	SHELL_STATIC_SUBCMD_SET_CREATE(sub_demo,
		SHELL_CMD(params, NULL, "Print params command.",
						       cmd_demo_params),
		SHELL_CMD(ping,   NULL, "Ping command.", cmd_demo_ping),
		SHELL_SUBCMD_SET_END
	);
	/* Creating root (level 0) command "demo" without a handler */
	SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

	/* Creating root (level 0) command "version" */
	SHELL_CMD_REGISTER(version, NULL, "Show kernel version", cmd_version);


Users may use the :kbd:`Tab` key to complete a command/subcommand or to see the
available subcommands for the currently entered command level.
For example, when the cursor is positioned at the beginning of the command
line and the :kbd:`Tab` key is pressed, the user will see all root (level 0)
commands:

.. code-block:: none

	  clear  demo  shell  history  log  resize  version


.. note::
	To view the subcommands that are available for a specific command, you
	must first type a :kbd:`space` after this command and then hit
	:kbd:`Tab`.

These commands are registered by various modules, for example:

* :command:`clear`, :command:`shell`, :command:`history`, and :command:`resize`
  are built-in commands which have been registered by
  :zephyr_file:`subsys/shell/shell.c`
* :command:`demo` and :command:`version` have been registered in example code
  above by main.c
* :command:`log` has been registered by :zephyr_file:`subsys/logging/log_cmds.c`

Then, if a user types a :command:`demo` command and presses the :kbd:`Tab` key,
the shell will only print the subcommands registered for this command:

.. code-block:: none

	  params  ping

API Reference
*************

.. doxygengroup:: shell_api
