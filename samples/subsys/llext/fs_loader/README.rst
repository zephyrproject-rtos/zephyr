.. zephyr:code-sample:: llext-fs-loader
   :name: Linkable loadable extensions filesystem module
   :relevant-api: llext

   Manage loadable extensions using fielsystem.

Overview
********

This example provides filesystem access to the :ref:`llext` system and provides the
ability to manage loadable code extensions in the filesystem.

Requirements
************

A board with a supported llext architecture and filesystem.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/llext/fs_loader
   :board: nucleo_l496zg
   :goals: build
   :compact:

Running
*******

Once the board has booted, you will be presented with a shell console.
All the llext system related commands are available as sub-commands of llext
which can be seen with llext help

.. code-block:: console

  uart:~$ llext help
  llext - Loadable extension commands
  Subcommands:
    list          :List loaded extensions and their size in memory
    load_hex      :Load an elf file encoded in hex directly from the shell input.
                   Syntax:
                   <ext_name> <ext_hex_string>
	load_fs       :Load an elf file encoded in elf directly from a filesystem.
                   Syntax:
                   <ext_name> <filepath>
    unload        :Unload an extension by name. Syntax:
                   <ext_name>
    list_symbols  :List extension symbols. Syntax:
                   <ext_name>
    call_fn       :Call extension function with prototype void fn(void). Syntax:
                   <ext_name> <function_name>

A hello world llext extension can be found in hello_world.llext folder.

This is built in parallel of zephyr and must be copied in filesystem.



.. code-block:: console

  uart:~$ llext load_fs hello_world /NAND:/hello_world.llext

This extension can then be seen in the list of loaded extensions (`list`), its symbols printed
(`list_symbols`), and the hello_world function which the extension exports can be called and
run (`call_fn`).

.. code-block:: console

  uart:~$ llext call_fn hello_world start
  hello world from new thread
  thread id is ....
