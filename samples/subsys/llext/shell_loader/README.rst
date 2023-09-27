.. zephyr:code-sample:: llext-shell-loader
   :name: Linkable loadable extensions shell module
   :relevant-api: llext

   Manage loadable extensions using shell commands.

Overview
********

This example provides shell access to the :ref:`llext` system and provides the
ability to manage loadable code extensions in the shell.

Requirements
************

A board with shell capable console.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/llext/shell_loader
   :goals: build
   :compact:

.. note:: You may need to disable memory protection for the sample to work (e.g. CONFIG_ARM_MPU=n).

Running
*******

Once the board has booted, you will be presented with a shell prompt.
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
    unload        :Unload an extension by name. Syntax:
                   <ext_name>
    list_symbols  :List extension symbols. Syntax:
                   <ext_name>
    call_fn       :Call extension function with prototype void fn(void). Syntax:
                   <ext_name> <function_name>

A hello world C file can be found in tests/subsys/llext/hello_world/hello_world.c

This can be built into a relocatable elf usable on arm v7 platforms. It can be
inspected with some binutils to see symbols, sections, and relocations.
Then using additional tools converted to a hex string usable by the llext
load_hex shell command.

On a host machine with the zephyr sdk setup and the arm toolchain in PATH

.. code-block:: console

  $ arm-zephyr-eabi-gcc -mlong-calls -mthumb -c -o hello_world.elf tests/subsys/llext/hello_world/hello_world.c
  $ arm-zephyr-eabi-objdump -x -t hello_world.elf

  hello_world.elf:     file format elf32-littlearm
  hello_world.elf
  architecture: armv4t, flags 0x00000011:
  HAS_RELOC, HAS_SYMS
  start address 0x00000000
  private flags = 0x5000000: [Version5 EABI]

  Sections:
  Idx Name          Size      VMA       LMA       File off  Algn
    0 .text         00000024  00000000  00000000  00000034  2**2
                    CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
    1 .data         00000000  00000000  00000000  00000058  2**0
                    CONTENTS, ALLOC, LOAD, DATA
    2 .bss          00000000  00000000  00000000  00000058  2**0
                    ALLOC
    3 .rodata       0000000d  00000000  00000000  00000058  2**2
                    CONTENTS, ALLOC, LOAD, READONLY, DATA
    4 .comment      00000021  00000000  00000000  00000065  2**0
                    CONTENTS, READONLY
    5 .ARM.attributes 0000002a  00000000  00000000  00000086  2**0
                    CONTENTS, READONLY
  arm-zephyr-eabi-objdump: hello_world.elf: not a dynamic object
  SYMBOL TABLE:
  00000000 l    df *ABS*	00000000 hello_world.c
  00000000 l    d  .text	00000000 .text
  00000000 l    d  .data	00000000 .data
  00000000 l    d  .bss	00000000 .bss
  00000000 l    d  .rodata	00000000 .rodata
  00000000 l    d  .comment	00000000 .comment
  00000000 l    d  .ARM.attributes	00000000 .ARM.attributes
  00000000 g     F .text	00000020 hello_world
  00000000         *UND*	00000000 printk


  RELOCATION RECORDS FOR [.text]:
  OFFSET   TYPE              VALUE
  00000018 R_ARM_ABS32       .rodata
  0000001c R_ARM_ABS32       printk

  $ xxd -p hello_world.elf | tr -d '\n'


The resulting hex string can be used to load the extension.

.. code-block:: console

  uart:~$ llext load_hex hello_world 7f454c4601010100000000000000000001002800010000000000000000000000180200000000000534000000000028000b000a0080b500af044b1800044b00f009f8c046bd4680bc01bc004700000000000000001847c04668656c6c6f20776f726c640a00004743433a20285a65706879722053444b20302e31362e31292031322e322e30004129000000616561626900011f000000053454000602080109011204140115011703180119011a011e06000000000000000000000000000000000100000000000000000000000400f1ff00000000000000000000000003000100000000000000000000000000030003000000000000000000000000000300040000000000000000000000000003000500090000000000000000000000000005000c000000000000000000000000000100090000001800000000000000000001000c00000020000000000000000000010000000000000000000000000003000600000000000000000000000000030007000f0000000100000020000000120001001b0000000000000000000000100000000068656c6c6f2e630024640024740068656c6c6f5f776f726c64007072696e746b00000018000000020500001c000000020d0000002e73796d746162002e737472746162002e7368737472746162002e72656c2e74657874002e64617461002e627373002e726f64617461002e636f6d6d656e74002e41524d2e6174747269627574657300000000000000000000000000000000000000000000000000000000000000000000000000000000000000001f0000000100000006000000000000003400000024000000000000000000000004000000000000001b000000090000004000000000000000b40100001000000008000000010000000400000008000000250000000100000003000000000000005800000000000000000000000000000001000000000000002b00000008000000030000000000000058000000000000000000000000000000010000000000000030000000010000000200000000000000580000000d000000000000000000000004000000000000003800000001000000300000000000000065000000210000000000000000000000010000000100000041000000030000700000000000000000860000002a0000000000000000000000010000000000000001000000020000000000000000000000b0000000e0000000090000000c00000004000000100000000900000003000000000000000000000090010000220000000000000000000000010000000000000011000000030000000000000000000000c40100005100000000000000000000000100000000000000

This extension can then be seen in the list of loaded extensions (`list`), its symbols printed (`list_symbols`), and the hello_world
function which the extension exports can be called and run (`call_fn`).

.. code-block:: console

  uart:~$ llext call_fn hello_world hello_world
  hello world
