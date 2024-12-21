.. _eeprom_shell:

EEPROM Shell
############

.. contents::
    :local:
    :depth: 1

Overview
********

The EEPROM shell provides an ``eeprom`` command with a set of subcommands for the :ref:`shell
<shell_api>` module. It allows testing and exploring the :ref:`EEPROM <eeprom_api>` driver API
through an interactive interface without having to write a dedicated application. The EEPROM shell
can also be enabled in existing applications to aid in interactive debugging of EEPROM issues.

In order to enable the EEPROM shell, the following :ref:`Kconfig <kconfig>` options must be enabled:

* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_EEPROM`
* :kconfig:option:`CONFIG_EEPROM_SHELL`

For example, building the :zephyr:code-sample:`hello_world` sample for the :ref:`native_sim` with the EEPROM shell:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: native_sim
   :gen-args: -DCONFIG_SHELL=y -DCONFIG_EEPROM=y -DCONFIG_EEPROM_SHELL=y
   :goals: build

See the :ref:`shell <shell_api>` documentation for general instructions on how to connect and
interact with the shell. The EEPROM shell comes with built-in help (unless
:kconfig:option:`CONFIG_SHELL_HELP` is disabled). The built-in help messages can be printed by
passing ``-h`` or ``--help`` to the ``eeprom`` command or any of its subcommands. All subcommands
also support tab-completion of their arguments.

.. tip::
   All of the EEPROM shell subcommands take the name of an EEPROM peripheral as their first argument,
   which also supports tab-completion. A list of all devices available can be obtained using the
   ``device list`` shell command when :kconfig:option:`CONFIG_DEVICE_SHELL` is enabled. The examples
   below all use the device name ``eeprom@0``.

EEPROM Size
***********

The size of an EEPROM can be inspected using the ``eeprom size`` subcommand as shown below:

.. code-block:: console

   uart:~$ eeprom size eeprom@0
   32768 bytes

Writing Data
************

Data can be written to an EEPROM using the ``eeprom write`` subcommand. This subcommand takes at
least three arguments; the EEPROM device name, the offset to start writing to, and at least one data
byte. In the following example, the hexadecimal sequence of bytes ``0x0d 0x0e 0x0a 0x0d 0x0b 0x0e
0x0e 0x0f`` is written to offset ``0x0``:

.. code-block:: console

   uart:~$ eeprom write eeprom@0 0x0 0x0d 0x0e 0x0a 0x0d 0x0b 0x0e 0x0e 0x0f
   Writing 8 bytes to EEPROM...
   Verifying...
   Verify OK

It is also possible to fill a portion of the EEPROM with the same pattern using the ``eeprom fill``
subcommand. In the following example, the pattern ``0xaa`` is written to 16 bytes starting at offset
``0x8``:

.. code-block:: console

   uart:~$ eeprom fill eeprom@0 0x8 16 0xaa
   Writing 16 bytes of 0xaa to EEPROM...
   Verifying...
   Verify OK

Reading Data
************

Data can be read from an EEPROM using the ``eeprom read`` subcommand. This subcommand takes three
arguments; the EEPROM device name, the offset to start reading from, and the number of bytes to
read:

.. code-block:: console

   uart:~$ eeprom read eeprom@0 0x0 8
   Reading 8 bytes from EEPROM, offset 0...
   00000000: 0d 0e 0a 0d 0b 0e 0e 0f                          |........         |
