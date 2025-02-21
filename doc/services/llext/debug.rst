.. _llext_debug:

Debugging extensions
####################

Debugging extensions is a complex task. Since the extension code is by
definition not built with the Zephyr application, the final Zephyr ELF file
does not contain the symbols for extension code. Furthermore, the extension is
dynamically relocated by :c:func:`llext_load` at runtime, so even if the
symbols were available, it would be impossible for the debugger to know the
final locations of the symbols in the extension code.

Setting up the debugger session properly in this case requires a few manual
steps. The following sections will provide some tips on how to do it with the
Zephyr SDK and the debug features provided by ``west``, but the instructions
can be adapted to any GDB-based debugging environment.

Extension debugging process
===========================

1. Make sure the project is set up to display the verbose LLEXT debug output
   (:kconfig:option:`CONFIG_LOG` and :kconfig:option:`CONFIG_LLEXT_LOG_LEVEL_DBG`
   are set).

2. Build the Zephyr application and the extensions.

   For each target ``name`` included in the current build, two files will be
   generated into the ``llext`` subdirectory of the build root:

   ``name_ext_debug.elf``

        An intermediate ELF file with full debugging information.

   ``name.llext``

        The final extension binary, stripped to the essential data required for
        loading into the Zephyr application.

   Other files may be present, depending on the target architecture and the
   build configuration.

3. Start a debugging session of the main Zephyr application. This is described
   in the :ref:`Debugging <west-debugging>` section of the documentation; on
   supported boards it is as easy as running ``west debug``, perhaps with some
   additional arguments.

4. Set a breakpoint just after the :c:func:`llext_load` function in your code
   and let it run. This will load the extension into memory and relocate it.
   The output logs will contain a line with ``gdb add-symbol-file flags:``,
   followed by lines all starting with ``-s``.

5. Type this command in the GDB console to load this extension's symbols:

   .. code-block::

      add-symbol-file <path-to-debug.elf> <load-addresses>

   where ``<path-to-debug.elf>`` is the full path of the ELF file with debug
   information identified in step 2, and ``<load-addresses>`` is a space
   separated list of all the ``-s`` lines collected from the log in the
   previous step.

6. The extension symbols are now available to the debugger. You can set
   breakpoints, inspect variables, and step through the code as usual.

Steps 4-6 can be repeated for every extension that is loaded by the
application, if there are several.

Symbol lookup issues
====================

.. warning::

   It is almost certain that the loaded symbols will be shadowed by others in
   the main application; for example, they may be located inside the memory
   area of the ELF buffer or the LLEXT heap.

   In this case GDB chooses the first known symbol and therefore associates the
   addresses to some ``elf_buffer+0x123`` instead of an expected ``ext_fn``.
   This further confuses its high-level operations like source stepping or
   inspecting locals, since they are meaningless in that context.

Two possible solutions to this problem are discussed in the following
paragraphs.

Discard all Zephyr symbols
--------------------------

The simplest option is to drop all the Zephyr application symbols from GDB by
invoking ``add-symbol-file`` with no arguments, before step 5. This will
however focus the debugging session to the llext only, as all information about
the Zephyr application will be lost. For example, the debugger may not be able to
properly follow stack traces outside the extension code.

It is possible to use the same technique multiple times in the same session to
switch between the main and extension symbol tables as required, but it rapidly
becomes cumbersome.

Edit the ELF file
-----------------

This alternative is more complex but allows for a more seamless debugging
experience. The idea is to edit the main Zephyr ELF file to remove information
about the symbols that overlap with the extension that is to be debugged, so
that when the extension symbols are loaded, GDB will not have any ambiguity.
This can be done by using ``objcopy`` with the ``-N <symbol>`` option.

Identifying the offending symbols is however an iterative trial-and-error
procedure, as there can be many different layers; for example, the ELF buffer
may be itself contained in a symbol for the data segment. Fortunately, this
knowledge can then be used several times as the list is unlikely to change for
a given project.

Example debugging session
=========================

This example demonstrates how to debug the ``detached_fn`` extension in the
``tests/subsys/llext`` project (specifically, the ``writable`` case), on an
emulated ``mps2/an385`` board which is based on an ARM Cortex-M3.

.. note::

   The logs below have been obtained using Zephyr version 4.1 and the Zephyr
   SDK version 0.17.0. However, the exact addresses may still vary between
   runs even when using the same versions. Adjust the commands below to
   match the results of your own session.

The following command will build the project and start the emulator in
debugging mode:

.. code-block::
   :caption: Terminal 1 (build, QEMU emulator, GDB server)

   zephyr$ west build -p -b mps2/an385 tests/subsys/llext/ -T llext.writable -t debugserver_qemu
   -- west build: generating a build system
   [...]
   -- west build: running target debugserver_qemu
   [...]
   [186/187] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-m3

On a separate terminal, set ``ZEPHYR_SDK_INSTALL_DIR`` to the directory for the
Zephyr SDK on your installation, then start the GDB client for the target:

.. code-block::
   :caption: Terminal 2 (GDB client)

   zephyr$ export LLEXT_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.17.0
   zephyr$ ${LLEXT_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb build/zephyr/zephyr.elf
   GNU gdb (Zephyr SDK 0.17.0) 12.1
   [...]
   Reading symbols from build/zephyr/zephyr.elf...
   (gdb)

Connect, set a breakpoint on the ``llext_load`` function and run until it
finishes:

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) target extended-remote :1234
   Remote debugging using :1234
   z_arm_reset () at zephyr/arch/arm/core/cortex_m/reset.S:124
   124         movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
   (gdb) break llext_load
   Breakpoint 1 at 0x236c: file zephyr/subsys/llext/llext.c, line 168.
   (gdb) continue
   Continuing.

   Breakpoint 1, llext_load (ldr=ldr@entry=0x2000bef0 <ztest_thread_stack+3488>,
                             name=name@entry=0x9d98 "test_detached",
                             ext=ext@entry=0x2000abb8 <detached_llext>,
                             ldr_parm=ldr_parm@entry=0x2000bee8 <ztest_thread_stack+3480>)
                 at zephyr/subsys/llext/llext.c:168
   168             *ext = llext_by_name(name);
   (gdb) finish
   Run till exit from #0  llext_load ([...])
       at zephyr/subsys/llext/llext.c:168
   llext_test_detached () at zephyr/tests/subsys/llext/src/test_llext.c:481
   481             zassert_ok(res, "load should succeed");

The first terminal will have printed lots of debugging information related to
the extension loading. Find the section with the addresses:

.. code-block::
   :caption: Terminal 1 (build, QEMU emulator, GDB server)

   [...]
   D: Allocate and copy regions...
   [...]
   D: gdb add-symbol-file flags:
   D: -s .text 0x20000034
   D: -s .data 0x200000b4
   D: -s .bss 0x2000c2e0
   D: -s .rodata 0x200000b8
   D: -s .detach 0x200001d0
   D: Counting exported symbols...
   [...]

Use these addresses to load the symbols into GDB:

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) add-symbol-file build/llext/detached_fn_ext_debug.elf -s .text 0x20000034 -s .data 0x200000b4 -s .bss 0x2000c2e0 -s .rodata 0x200000b8 -s .detach 0x200001d0
   add symbol table from file "build/llext/detached_fn_ext_debug.elf" at
           .text_addr = 0x20000034
           .data_addr = 0x200000b4
           .bss_addr = 0x2000c2e0
           .rodata_addr = 0x200000b8
           .detach_addr = 0x200001d0
   (y or n) y
   Reading symbols from build/llext/detached_fn_ext_debug.elf...
   (gdb) break detached_entry
   Breakpoint 2 at 0x200001d0 (2 locations)
   (gdb) continue
   Continuing.

   Breakpoint 2, 0x200001d0 in test_detached_ext ()
   (gdb) backtrace
   #0  0x200001d0 in test_detached_ext ()
   #1  0x200000ac in test_detached_ext ()
   #2  0x00000706 in llext_test_detached () at zephyr/tests/subsys/llext/src/test_llext.c:496
   #3  0x00001a36 in run_test_functions (suite=0x92bc <z_ztest_test_node_llext>, data=0x0 <cbvprintf_package>, test=0x92d8 <z_ztest_unit_test.llext.test_detached>) at zephyr/subsys/testsuite/ztest/src/ztest.c:328
   #4  test_cb (a=0x92bc <z_ztest_test_node_llext>, b=0x92d8 <z_ztest_unit_test.llext.test_detached>, c=0x0 <cbvprintf_package>) at zephyr/subsys/testsuite/ztest/src/ztest.c:662
   #5  0x00000e96 in z_thread_entry (entry=0x1a05 <test_cb>, p1=0x92bc <z_ztest_test_node_llext>, p2=0x92d8 <z_ztest_unit_test.llext.test_detached>, p3=0x0 <cbvprintf_package>) at zephyr/lib/os/thread_entry.c:48
   #6  0x00000000 in ?? ()

The symbol associated with the breakpoint location and the last stack frames
mistakenly reference the ELF buffer in the Zephyr application instead of the
extension symbols. Note that GDB however knows both:

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) info sym 0x200001d0
   test_detached_ext + 464 in section datas of zephyr/build/zephyr/zephyr.elf
   detached_entry in section .detach of zephyr/build/llext/detached_fn_ext_debug.elf
   (gdb) info sym 0x200000ac
   test_detached_ext + 172 in section datas of zephyr/build/zephyr/zephyr.elf
   test_entry + 8 in section .text of zephyr/build/llext/detached_fn_ext_debug.elf

It is also impossible to inspect the variables in the extension or step through
code properly:

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) print bss_cnt
   No symbol "bss_cnt" in current context.
   (gdb) print data_cnt
   No symbol "data_cnt" in current context.
   (gdb) next
   Single stepping until exit from function test_detached_ext,
   which has no line number information.

   Breakpoint 2, 0x200001ea in test_detached_ext ()
   (gdb)

Discarding symbols
------------------

Discarding the Zephyr symbols and only focusing on the extension restores full
debugging functionality at the cost of losing the global context (note the
backtrace stops outside the extension):

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) symbol-file
   Discard symbol table from `zephyr/build/zephyr/zephyr.elf'? (y or n) y
   Error in re-setting breakpoint 1: No symbol table is loaded.  Use the "file" command.
   No symbol file now.
   (gdb) add-symbol-file build/llext/detached_fn_ext_debug.elf -s .text 0x20000034 -s .data 0x200000b4 -s .bss 0x2000c2e0 -s .rodata 0x200000b8 -s .detach 0x200001d0
   add symbol table from file "build/llext/detached_fn_ext_debug.elf" at
           .text_addr = 0x20000034
           .data_addr = 0x200000b4
           .bss_addr = 0x2000c2e0
           .rodata_addr = 0x200000b8
           .detach_addr = 0x200001d0
   (y or n) y
   Reading symbols from build/llext/detached_fn_ext_debug.elf...
   (gdb) backtrace
   #0  detached_entry () at zephyr/tests/subsys/llext/src/detached_fn_ext.c:18
   #1  0x200000ac in test_entry () at zephyr/tests/subsys/llext/src/detached_fn_ext.c:26
   #2  0x00000706 in ?? ()
   Backtrace stopped: previous frame identical to this frame (corrupt stack?)
   (gdb) next
   19              zassert_true(data_cnt < 0);
   (gdb) print bss_cnt
   $1 = 1
   (gdb) print data_cnt
   $2 = -2
   (gdb)


Editing the ELF file
--------------------

In this alternative approach, the patches to the Zephyr ELF file must be
performed after building the Zephyr binary and starting the emulator on
Terminal 1, but before starting the GDB client on Terminal 2.

The above debugging session already identified ``test_detached_ext``, the char
array that holds the ELF file, as an offending symbol, so that will be removed
in a first pass. Performing the same steps multiple times, ``__data_start`` and
``__data_region_start`` can also be found to overlap the memory area of
interest.

The following commands will remove all of these from the Zephyr ELF file, then
start a debugging session on the modified file:

.. code-block::
   :caption: Terminal 2 (GDB client)

   zephyr$ export LLEXT_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.17.0
   zephyr$ ${LLEXT_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-objcopy -N test_detached_ext -N __data_start -N __data_region_start build/zephyr/zephyr.elf build/zephyr/zephyr-edit.elf
   zephyr$ ${LLEXT_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb build/zephyr/zephyr-edit.elf
   GNU gdb (Zephyr SDK 0.17.0) 12.1
   [...]
   Reading symbols from build/zephyr/zephyr-edit.elf...
   (gdb)

The same steps used in the previous run can be performed again to attach to the
GDB server and load both the extension and its debug symbols. This time, however,
the result is rather different:

 * the ``break`` command includes line number information;

 * the output from ``backtrace`` contains functions from both the extension and
   the Zephyr application;

 * the local variables can be properly inspected.

.. code-block::
   :caption: Terminal 2 (GDB client)

   (gdb) add-symbol-file build/llext/detached_fn_ext_debug.elf [...]
   [...]
   Reading symbols from build/llext/detached_fn_ext_debug.elf...
   (gdb) break detached_entry
   Breakpoint 2 at 0x200001d6: file zephyr/tests/subsys/llext/src/detached_fn_ext.c, line 17.
   (gdb) continue
   Continuing.

   Breakpoint 2, detached_entry () at zephyr/tests/subsys/llext/src/detached_fn_ext.c:17
   17              printk("bss %u @ %p\n", bss_cnt++, &bss_cnt);
   (gdb) backtrace
   #0  detached_entry () at zephyr/tests/subsys/llext/src/detached_fn_ext.c:17
   #1  0x200000ac in test_entry () at zephyr/tests/subsys/llext/src/detached_fn_ext.c:26
   #2  0x00000706 in llext_test_detached () at zephyr/tests/subsys/llext/src/test_llext.c:496
   #3  0x00001a36 in run_test_functions (suite=0x92bc <z_ztest_test_node_llext>, data=0x0 <cbvprintf_package>, test=0x92d8 <z_ztest_unit_test.llext.test_detached>) at zephyr/subsys/testsuite/ztest/src/ztest.c:328
   #4  test_cb (a=0x92bc <z_ztest_test_node_llext>, b=0x92d8 <z_ztest_unit_test.llext.test_detached>, c=0x0 <cbvprintf_package>) at zephyr/subsys/testsuite/ztest/src/ztest.c:662
   #5  0x00000e96 in z_thread_entry (entry=0x1a05 <test_cb>, p1=0x92bc <z_ztest_test_node_llext>, p2=0x92d8 <z_ztest_unit_test.llext.test_detached>, p3=0x0 <cbvprintf_package>) at zephyr/lib/os/thread_entry.c:48
   #6  0x00000000 in ?? ()
   (gdb) print bss_cnt
   $1 = 0
   (gdb) print data_cnt
   $2 = -3
   (gdb)
