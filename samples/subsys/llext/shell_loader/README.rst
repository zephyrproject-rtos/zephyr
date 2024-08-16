.. zephyr:code-sample:: llext-shell-loader
   :name: Linkable loadable extensions shell module
   :relevant-api: llext_apis

   Manage loadable extensions using shell commands.

Overview
********

This example provides shell access to the :ref:`llext` system and provides the
ability to manage loadable code extensions in the shell.

Requirements
************

A board with a supported llext architecture and shell capable console.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/llext/shell_loader
   :board: robokit1
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
  $ arm-zephyr-eabi-objdump -r -d -x hello_world.elf

	hello_world.elf:     file format elf32-littlearm
	hello_world.elf
	architecture: armv4t, flags 0x00000011:
	HAS_RELOC, HAS_SYMS
	start address 0x00000000
	private flags = 0x5000000: [Version5 EABI]

	Sections:
	Idx Name          Size      VMA       LMA       File off  Algn
	  0 .text         00000038  00000000  00000000  00000034  2**2
	                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
	  1 .data         00000000  00000000  00000000  0000006c  2**0
	                  CONTENTS, ALLOC, LOAD, DATA
	  2 .bss          00000000  00000000  00000000  0000006c  2**0
	                  ALLOC
	  3 .rodata       00000025  00000000  00000000  0000006c  2**2
	                  CONTENTS, ALLOC, LOAD, READONLY, DATA
	  4 .comment      00000021  00000000  00000000  00000091  2**0
	                  CONTENTS, READONLY
	  5 .ARM.attributes 0000002a  00000000  00000000  000000b2  2**0
	                  CONTENTS, READONLY
	SYMBOL TABLE:
	00000000 l    df *ABS*	00000000 hello_world.c
	00000000 l    d  .text	00000000 .text
	00000000 l    d  .data	00000000 .data
	00000000 l    d  .bss	00000000 .bss
	00000000 l    d  .rodata	00000000 .rodata
	00000000 l     O .rodata	00000004 number
	00000000 l    d  .comment	00000000 .comment
	00000000 l    d  .ARM.attributes	00000000 .ARM.attributes
	00000000 g     F .text	00000034 hello_world
	00000000         *UND*	00000000 printk



	Disassembly of section .text:

	00000000 <hello_world>:
	   0:	b580      	push	{r7, lr}
	   2:	af00      	add	r7, sp, #0
	   4:	4b08      	ldr	r3, [pc, #32]	; (28 <hello_world+0x28>)
	   6:	0018      	movs	r0, r3
	   8:	4b08      	ldr	r3, [pc, #32]	; (2c <hello_world+0x2c>)
	   a:	f000 f813 	bl	34 <hello_world+0x34>
	   e:	222a      	movs	r2, #42	; 0x2a
	  10:	4b07      	ldr	r3, [pc, #28]	; (30 <hello_world+0x30>)
	  12:	0011      	movs	r1, r2
	  14:	0018      	movs	r0, r3
	  16:	4b05      	ldr	r3, [pc, #20]	; (2c <hello_world+0x2c>)
	  18:	f000 f80c 	bl	34 <hello_world+0x34>
	  1c:	46c0      	nop			; (mov r8, r8)
	  1e:	46bd      	mov	sp, r7
	  20:	bc80      	pop	{r7}
	  22:	bc01      	pop	{r0}
	  24:	4700      	bx	r0
	  26:	46c0      	nop			; (mov r8, r8)
	  28:	00000004 	.word	0x00000004
				28: R_ARM_ABS32	.rodata
	  2c:	00000000 	.word	0x00000000
				2c: R_ARM_ABS32	printk
	  30:	00000014 	.word	0x00000014
				30: R_ARM_ABS32	.rodata
	  34:	4718      	bx	r3
	  36:	46c0      	nop			; (mov r8, r8)

  $ xxd -p hello_world.elf | tr -d '\n'

The resulting hex string can be used to load the extension.

.. code-block:: console

  uart:~$ llext load_hex hello_world 7f454c4601010100000000000000000001002800010000000000000000000000680200000000000534000000000028000b000a0080b500af084b1800084b00f013f82a22074b11001800054b00f00cf8c046bd4680bc01bc0047c0460400000000000000140000001847c0462a00000068656c6c6f20776f726c640a0000000041206e756d62657220697320256c750a00004743433a20285a65706879722053444b20302e31362e31292031322e322e30004129000000616561626900011f000000053454000602080109011204140115011703180119011a011e06000000000000000000000000000000000100000000000000000000000400f1ff000000000000000000000000030001000000000000000000000000000300030000000000000000000000000003000400000000000000000000000000030005000f00000000000000000000000000050012000000000000000400000001000500190000000000000000000000000001000f0000002800000000000000000001001900000034000000000000000000010000000000000000000000000003000600000000000000000000000000030007001c000000010000003400000012000100280000000000000000000000100000000068656c6c6f5f776f726c642e63002464006e756d6265720024740068656c6c6f5f776f726c64007072696e746b000028000000020500002c000000020e00003000000002050000002e73796d746162002e737472746162002e7368737472746162002e72656c2e74657874002e64617461002e627373002e726f64617461002e636f6d6d656e74002e41524d2e6174747269627574657300000000000000000000000000000000000000000000000000000000000000000000000000000000000000001f0000000100000006000000000000003400000038000000000000000000000004000000000000001b000000090000004000000000000000fc0100001800000008000000010000000400000008000000250000000100000003000000000000006c00000000000000000000000000000001000000000000002b0000000800000003000000000000006c0000000000000000000000000000000100000000000000300000000100000002000000000000006c00000025000000000000000000000004000000000000003800000001000000300000000000000091000000210000000000000000000000010000000100000041000000030000700000000000000000b20000002a0000000000000000000000010000000000000001000000020000000000000000000000dc000000f0000000090000000d000000040000001000000009000000030000000000000000000000cc0100002f0000000000000000000000010000000000000011000000030000000000000000000000140200005100000000000000000000000100000000000000

This extension can then be seen in the list of loaded extensions (`list`), its symbols printed
(`list_symbols`), and the hello_world function which the extension exports can be called and
run (`call_fn`).

.. code-block:: console

  uart:~$ llext call_fn hello_world hello_world
  hello world
  A number is 42

In this sample there are 3 absolute (R_ARM_ABS32) relocations, 2 of which are meant to hold addresses into the .rodata sections where the strings are located. A third is an address of where the printk function (symbol) can be found. At load time llext replaces the values in the .text section with real memory addresses so that printk works as expected with the strings included in the hello world sample.
