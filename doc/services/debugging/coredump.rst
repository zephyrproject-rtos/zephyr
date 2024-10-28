.. _coredump:

Core Dump
#########

The core dump module enables dumping the CPU registers and memory content
for offline debugging. This module is called when a fatal error is
encountered and prints or stores data according to which backends
are enabled.

Configuration
*************

Configure this module using the following options.

* ``DEBUG_COREDUMP``: enable the module.

Here are the options to enable output backends for core dump:

* ``DEBUG_COREDUMP_BACKEND_LOGGING``: use log module for core dump output.
* ``DEBUG_COREDUMP_BACKEND_FLASH_PARTITION``: use flash partition for core
  dump output.
* ``DEBUG_COREDUMP_BACKEND_NULL``: fallback core dump backend if other
  backends cannot be enabled. All output is sent to null.

Here are the choices regarding memory dump:

* ``DEBUG_COREDUMP_MEMORY_DUMP_MIN``: only dumps the stack of the exception
  thread, its thread struct, and some other bare minimal data to support
  walking the stack in the debugger. Use this only if absolute minimum of data
  dump is desired.

* ``DEBUG_COREDUMP_MEMORY_DUMP_THREADS``: Dumps the thread struct and stack of all
  threads and all data required to debug threads.

* ``DEBUG_COREDUMP_MEMORY_DUMP_LINKER_RAM``: Dumps the memory region between
  _image_ram_start[] and _image_ram_end[]. This includes at least data, noinit,
  and BSS sections. This is the default.

Additional memory can be included in a dump (even with the "DEBUG_COREDUMP_MEMORY_DUMP_MIN"
config selected) through one or more :ref:`coredump devices <coredump_device_api>`

Usage
*****

When the core dump module is enabled, during a fatal error, CPU registers
and memory content are printed or stored according to which backends
are enabled. This core dump data can fed into a custom-made GDB server as
a remote target for GDB (and other GDB compatible debuggers). CPU registers,
memory content and stack can be examined in the debugger.

This usually involves the following steps:

1. Get the core dump log from the device depending on enabled backends.
   For example, if the log module backend is used, get the log output
   from the log module backend.

2. Convert the core dump log into a binary format that can be parsed by
   the GDB server. For example,
   :zephyr_file:`scripts/coredump/coredump_serial_log_parser.py` can be used
   to convert the serial console log into a binary file.

3. Start the custom GDB server using the script
   :zephyr_file:`scripts/coredump/coredump_gdbserver.py` with the core dump
   binary log file, and the Zephyr ELF file as parameters. The GDB server
   can also be started from within GDB, see below.

4. Start the debugger corresponding to the target architecture.

.. note::
   Developers for Intel ADSP CAVS 15-25 platforms using
   ``ZEPHYR_TOOLCHAIN_VARIANT=zephyr`` should use the debugger in the
   ``xtensa-intel_apl_adsp`` toolchain of the SDK.

5. When ``DEBUG_COREDUMP_BACKEND_FLASH_PARTITION`` is enabled the core dump
   data is stored in the flash partition. The flash partition must be defined
   in the device tree:

	.. code-block:: devicetree

		&flash0 {
			partitions {
				coredump_partition: partition@255000 {
					label = "coredump-partition";
					reg = <0x255000 DT_SIZE_K(4)>;
				};
		};

Example
-------

This example uses the log module backend tied to serial console.
This was done on :zephyr:board:`qemu_x86` where a null pointer was dereferenced.

This is the core dump log from the serial console, and is stored
in :file:`coredump.log`:

::

   Booting from ROM..*** Booting Zephyr OS build zephyr-v2.3.0-1840-g7bba91944a63  ***
   Hello World! qemu_x86
   E: Page fault at address 0x0 (error code 0x2)
   E: Linear address not present in page tables
   E:   PDE: 0x0000000000115827 Writable, User, Execute Enabled
   E:   PTE: Non-present
   E: EAX: 0x00000000, EBX: 0x00000000, ECX: 0x00119d74, EDX: 0x000003f8
   E: ESI: 0x00000000, EDI: 0x00101aa7, EBP: 0x00119d10, ESP: 0x00119d00
   E: EFLAGS: 0x00000206 CS: 0x0008 CR3: 0x00119000
   E: call trace:
   E: EIP: 0x00100459
   E:      0x00100477 (0x0)
   E:      0x00100492 (0x0)
   E:      0x001004c8 (0x0)
   E:      0x00105465 (0x105465)
   E:      0x00101abe (0x0)
   E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
   E: Current thread: 0x00119080 (unknown)
   E: #CD:BEGIN#
   E: #CD:5a4501000100050000000000
   E: #CD:4101003800
   E: #CD:0e0000000200000000000000749d1100f803000000000000009d1100109d1100
   E: #CD:00000000a71a100059041000060200000800000000901100
   E: #CD:4d010080901100e0901100
   E: #CD:0100000000000000000000000180000000000000000000000000000000000000
   E: #CD:00000000000000000000000000000000e364100000000000000000004c9c1100
   E: #CD:000000000000000000000000b49911000004000000000000fc03000000000000
   E: #CD:4d0100b4991100b49d1100
   E: #CD:f8030000020000000200000002000000f8030000fd03000a02000000dc9e1100
   E: #CD:149a1160fd03000002000000dc9e1100249a110087201000049f11000a000000
   E: #CD:349a11000a4f1000049f11000a9e1100449a11000a8b10000200000002000000
   E: #CD:449a1100388b1000049f11000a000000549a1100ad201000049f11000a000000
   E: #CD:749a11000a201000049f11000a000000649a11000a201000049f11000a000000
   E: #CD:749a1100e8201000049f11000a000000949a1100890b10000a0000000a000000
   E: #CD:a49a1100890b10000a0000000a000000f8030000189b11000200000002000000
   E: #CD:f49a1100289b11000a000000189b1100049b11009b0710000a000000289b1100
   E: #CD:f49a110087201000049f110045000000f49a1100509011000a00000020901100
   E: #CD:f49a110060901100049f1100ffffffff0000000000000000049f1100ffffffff
   E: #CD:0000000000000000630b1000189b1100349b1100af0b1000630b1000289b1100
   E: #CD:55891000789b11000000000020901100549b1100480000004a891000609b1100
   E: #CD:649b1100d00b10004a891000709b110000000000609b11000a00000000000000
   E: #CD:849b1100709b11004a89100000000000949b1100794a10000000000058901100
   E: #CD:20901100c34a10000a00001734020000d001000000000000d49b110038000000
   E: #CD:c49b110078481000b49911000004000000000000000000000c9c11000c9c1100
   E: #CD:149c110000000000d49b110038000000f49b1100da481000b499110000040000
   E: #CD:0e0000000200000000000000744d0100b4991100b49d1100009d1100109d1100
   E: #CD:149c110099471000b4991100000400000800000000901100ad861000409c1100
   E: #CD:349c1100e94710008090110000000000349c1100b64710008086100045000000
   E: #CD:849c11002d53100000000000d09c11008090110020861000f5ffffff8c9c1100
   E: #CD:000000000000000000000000a71a1000a49c1100020200008090110000000000
   E: #CD:a49c1100020200000800000000000000a49c11001937100000000000d09c1100
   E: #CD:0c9d0000bc9c0000b49d1100b4991100c49c1100ae37100000000000d09c1100
   E: #CD:0800000000000000c888100000000000109d11005d031000d09c1100009d1100
   E: #CD:109d11000000000000000000a71a1000f803000000000000749d110002000000
   E: #CD:5904100008000000060200000e0000000202000002020000000000002c9d1100
   E: #CD:7704100000000000d00b1000c9881000549d110000000000489d110092041000
   E: #CD:00000000689d1100549d11000000000000000000689d1100c804100000000000
   E: #CD:c0881000000000007c9d110000000000749d11007c9d11006554100065541000
   E: #CD:00000000000000009c9d1100be1a100000000000000000000000000038041000
   E: #CD:08000000020200000000000000000000f4531000000000000000000000000000
   E: #CD:END#
   E: Halting system


1. Run the core dump serial log converter:

   .. code-block:: console

      ./scripts/coredump/coredump_serial_log_parser.py coredump.log coredump.bin

2. Start the custom GDB server:

   .. code-block:: console

      ./scripts/coredump/coredump_gdbserver.py build/zephyr/zephyr.elf coredump.bin

3. Start GDB:

   .. code-block:: console

      <path to SDK>/x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gdb build/zephyr/zephyr.elf

4. Inside GDB, connect to the GDB server via port 1234:

   .. code-block:: console

      (gdb) target remote localhost:1234

5. Examine the CPU registers:

   .. code-block:: console

      (gdb) info registers

   Output from GDB:

   ::

      eax            0x0                 0
      ecx            0x119d74            1154420
      edx            0x3f8               1016
      ebx            0x0                 0
      esp            0x119d00            0x119d00 <z_main_stack+844>
      ebp            0x119d10            0x119d10 <z_main_stack+860>
      esi            0x0                 0
      edi            0x101aa7            1055399
      eip            0x100459            0x100459 <func_3+16>
      eflags         0x206               [ PF IF ]
      cs             0x8                 8
      ss             <unavailable>
      ds             <unavailable>
      es             <unavailable>
      fs             <unavailable>
      gs             <unavailable>

6. Examine the backtrace:

   .. code-block:: console

      (gdb) bt


   Output from GDB:

   ::

      #0  0x00100459 in func_3 (addr=0x0) at zephyr/rtos/zephyr/samples/hello_world/src/main.c:14
      #1  0x00100477 in func_2 (addr=0x0) at zephyr/rtos/zephyr/samples/hello_world/src/main.c:21
      #2  0x00100492 in func_1 (addr=0x0) at zephyr/rtos/zephyr/samples/hello_world/src/main.c:28
      #3  0x001004c8 in main () at zephyr/rtos/zephyr/samples/hello_world/src/main.c:42

Starting the GDB server from within GDB
---------------------------------------

You can use ``target remote |`` to start the custom GDB server from inside
GDB, instead of in a separate shell.

1. Start GDB:

   .. code-block:: console

      <path to SDK>/x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gdb build/zephyr/zephyr.elf

2. Inside GDB, start the GDB server using the ``--pipe`` option:

   .. code-block:: console

      (gdb) target remote | ./scripts/coredump/coredump_gdbserver.py --pipe build/zephyr/zephyr.elf coredump.bin


File Format
***********

The core dump binary file consists of one file header, one
architecture-specific block, zero or one threads metadata block(s),
and multiple memory blocks. All numbers in
the headers below are little endian.

File Header
-----------

The file header consists of the following fields:

.. list-table:: Core dump binary file header
   :widths: 2 1 7
   :header-rows: 1

   * - Field
     - Data Type
     - Description
   * - ID
     - ``char[2]``
     - ``Z``, ``E`` as identifier of file.
   * - Header version
     - ``uint16_t``
     - Identify the version of the header. This needs to be incremented
       whenever the header struct is modified. This allows parser to
       reject older header versions so it will not incorrectly parse
       the header.
   * - Target code
     - ``uint16_t``
     - Indicate which target (e.g. architecture or SoC) so the parser
       can instantiate the correct register block parser.
   * - Pointer size
     - 'uint8_t'
     - Size of ``uintptr_t`` in power of 2. (e.g. 5 for 32-bit,
       6 for 64-bit). This is needed to accommodate 32-bit and 64-bit
       target in parsing the memory block addresses.
   * - Flags
     - ``uint8_t``
     -
   * - Fatal error reason
     - ``unsigned int``
     - Reason for the fatal error, as the same in
       ``enum k_fatal_error_reason`` defined in
       :zephyr_file:`include/zephyr/fatal.h`

Architecture-specific Block
---------------------------

The architecture-specific block contains the byte stream of data specific
to the target architecture (e.g. CPU registers)

.. list-table:: Architecture-specific Block
   :widths: 2 1 7
   :header-rows: 1

   * - Field
     - Data Type
     - Description
   * - ID
     - ``char``
     - ``A`` to indicate this is a architecture-specific block.
   * - Header version
     - ``uint16_t``
     - Identify the version of this block. To be interpreted by the target
       architecture specific block parser.
   * - Number of bytes
     - ``uint16_t``
     - Number of bytes following the header which contains the byte stream
       for target data. The format of the byte stream is specific to
       the target and is only being parsed by the target parser.
   * - Register byte stream
     - ``uint8_t[]``
     - Contains target architecture specific data.

Threads Metadata Block
---------------------------

The threads metadata block contains the byte stream of data necessary
for debugging threads.

.. list-table:: Threads Metadata Block
   :widths: 2 1 7
   :header-rows: 1

   * - Field
     - Data Type
     - Description
   * - ID
     - ``char``
     - ``T`` to indicate this is a threads metadata block.
   * - Header version
     - ``uint16_t``
     - Identify the version of the header. This needs to be incremented
       whenever the header struct is modified. This allows parser to
       reject older header versions so it will not incorrectly parse
       the header.
   * - Number of bytes
     - ``uint16_t``
     - Number of bytes following the header which contains the byte stream
       for target data.
   * - Byte stream
     - ``uint8_t[]``
     - Contains data necessary for debugging threads.

Memory Block
------------

The memory block contains the start and end addresses and the data within
the memory region.

.. list-table:: Memory Block
   :widths: 2 1 7
   :header-rows: 1

   * - Field
     - Data Type
     - Description
   * - ID
     - ``char``
     - ``M`` to indicate this is a memory block.
   * - Header version
     - ``uint16_t``
     - Identify the version of the header. This needs to be incremented
       whenever the header struct is modified. This allows parser to
       reject older header versions so it will not incorrectly parse
       the header.
   * - Start address
     - ``uintptr_t``
     - The start address of the memory region.
   * - End address
     - ``uintptr_t``
     - The end address of the memory region.
   * - Memory byte stream
     - ``uint8_t[]``
     - Contains the memory content between the start and end addresses.

Adding New Target
*****************

The architecture-specific block is target specific and requires new
dumping routine and parser for new targets. To add a new target,
the following needs to be done:

#. Add a new target code to the ``enum coredump_tgt_code`` in
   :zephyr_file:`include/zephyr/debug/coredump.h`.
#. Implement :c:func:`arch_coredump_tgt_code_get` simply to return
   the newly introduced target code.
#. Implement :c:func:`arch_coredump_info_dump` to construct
   a target architecture block and call :c:func:`coredump_buffer_output`
   to output the block to core dump backend.
#. Add a parser to the core dump GDB stub scripts under
   ``scripts/coredump/gdbstubs/``

   #. Extends the ``gdbstubs.gdbstub.GdbStub`` class.
   #. During ``__init__``, store the GDB signal corresponding to
      the exception reason in ``self.gdb_signal``.
   #. Parse the architecture-specific block from
      ``self.logfile.get_arch_data()``. This needs to match the format
      as implemented in step 3 (inside :c:func:`arch_coredump_info_dump`).
   #. Implement the abstract method ``handle_register_group_read_packet``
      where it returns the register group as GDB expected. Refer to
      GDB's code and documentation on what it is expecting for
      the new target.
   #. Optionally implement ``handle_register_single_read_packet``
      for registers not covered in the ``g`` packet.

#. Extend ``get_gdbstub()`` in
   :zephyr_file:`scripts/coredump/gdbstubs/__init__.py` to return
   the newly implemented GDB stub.

API documentation
*****************

.. doxygengroup:: coredump_apis

.. doxygengroup:: arch-coredump
