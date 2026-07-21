.. zephyr:code-sample:: pytest_shell
   :name: Pytest shell application testing

   Execute pytest tests against the Zephyr shell.

Overview
********

The sample project illustrates usage of pytest framework integrated with
Twister test runner.

A simple application provides basic Zephyr shell interface. Twister builds it
and then calls pytest in subprocess which runs tests from
``pytest/test_shell.py`` file. The first test verifies valid response for
``help`` command, second one verifies if application is able to return
information about used kernel version. Both tests use ``shell`` fixture, which
is defined in ``pytest-twister-harness`` plugin. More information about plugin
can be found :ref:`here <integration_with_pytest>`.

Requirements
************

Board (hardware, ``native_sim`` or ``QEMU``) with UART console.

Building and Running
********************

Build and run sample by Twister:

.. code-block:: console

   $ ./scripts/twister -vv --platform native_sim -T samples/subsys/testsuite/pytest/shell


Sample Output
=============

.. code-block:: console

    ...
    samples/subsys/testsuite/pytest/shell/pytest/test_shell.py::test_shell_print_help
    INFO: send "help" command
    DEBUG: #: uart:~$ help
    DEBUG: #: Please press the <Tab> button to see all available commands.
    DEBUG: #: You can also use the <Tab> button to prompt or auto-complete all commands or its subcommands.
    DEBUG: #: You can try to call commands with <-h> or <--help> parameter for more information.
    DEBUG: #: Shell supports following meta-keys:
    DEBUG: #: Ctrl + (a key from: abcdefklnpuw)
    DEBUG: #: Alt  + (a key from: bf)
    DEBUG: #: Please refer to shell documentation for more details.
    DEBUG: #: Available commands:
    DEBUG: #: clear    :Clear screen.
    DEBUG: #: device   :Device commands
    DEBUG: #: devmem   :Read/write physical memory
    DEBUG: #:             Usage:
    DEBUG: #:             Read memory at address with optional width:
    DEBUG: #:             devmem address [width]
    DEBUG: #:             Write memory at address with mandatory width and value:
    DEBUG: #:             devmem address <width> <value>
    DEBUG: #: help     :Prints the help message.
    DEBUG: #: history  :Command history.
    DEBUG: #: kernel   :Kernel commands
    DEBUG: #: rem      :Ignore lines beginning with 'rem '
    DEBUG: #: resize   :Console gets terminal screen size or assumes default in case the
    DEBUG: #:             readout fails. It must be executed after each terminal width change
    DEBUG: #:             to ensure correct text display.
    DEBUG: #: retval   :Print return value of most recent command
    DEBUG: #: shell    :Useful, not Unix-like shell commands.
    DEBUG: #: uart:~$
    INFO: response is valid
    PASSED
    samples/subsys/testsuite/pytest/shell/pytest/test_shell.py::test_shell_print_version
    INFO: send "kernel version" command
    DEBUG: #: uart:~$ kernel version
    DEBUG: #: Zephyr version 3.5.99
    DEBUG: #: uart:~$
    INFO: response is valid
    PASSED
    ...
