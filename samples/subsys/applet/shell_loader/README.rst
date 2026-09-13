.. zephyr:code-sample:: applet-shell-loader
   :name: Applet loader shell module

   Load and run LLEXT-backed applets using shell commands.

Overview
********

This example provides shell access to the applet subsystem, which
allows for the grouping of threads into subapplications ("applets")
and simplified memory domain management for those applets. Users can
choose between a native applet, which is compiled into the Zephyr image,
or a LLEXT-backed applet, which is compiled as an ELF and loaded at runtime,
but due to the nature of this sample, only the latter is demonstrated.

This example is the applet equivalent of the
:zephyr:code-sample:`llext-shell-loader` sample: an ELF is pasted into the
shell as a hex string, but instead of loading it and calling an exported
function directly, the extension is wrapped in an *applet*. The applet
subsystem then creates a thread for it, adds the extension's regions to a
dedicated memory domain, and runs the ``applet_main()`` entry point on that
thread.

Requirements
************

A board with a supported LLEXT architecture and a shell capable console. The
example below uses an ARMv7 target for illustration; the workflow is the same
on any LLEXT-supported architecture, only the toolchain prefix and the
generated hex payload change.

Building
********

The following command will build the main shell application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/applet/shell_loader
   :board: mps2/an385
   :goals: build
   :compact:

.. note::

   You may need to disable memory protection for the sample to work (e.g.
   ``CONFIG_ARM_MPU=n`` on ARM, ``CONFIG_XTENSA_MMU=n`` /
   ``CONFIG_XTENSA_MPU=n`` on Xtensa, ``CONFIG_RISCV_PMP=n`` on RISC-V), etc.

This sample also includes the source for three applet extensions, which can be
used to exercise the applet features:

* :zephyr_file:`samples/subsys/applet/shell_loader/applet/hello_world_applet.c`
  prints a single line and returns.
* :zephyr_file:`samples/subsys/applet/shell_loader/applet/ping_applet.c` is a
  two-thread applet: one thread produces samples, the other one sums them and
  hands the running total over to a peer applet.
* :zephyr_file:`samples/subsys/applet/shell_loader/applet/pong_applet.c` is a
  service applet that never returns: it folds its own seed into every value
  ``ping`` hands over, and has to be killed to stop.

They are built alongside the application, and can also be rebuilt on their own:

.. code-block:: console

   $ ninja -C build hello_world_ext ping_ext pong_ext

The resulting :file:`build/hello_world.llext`, :file:`build/ping.llext` and
:file:`build/pong.llext` are the objects loaded by the shell.

``hello_world_applet.c`` is self-contained, so on a host machine with the
Zephyr SDK and the matching toolchain in ``PATH`` it can also be produced
directly. Pick the toolchain prefix that matches your target, for example
``arm-zephyr-eabi-`` for ARM, ``xtensa-<soc>_zephyr-elf-`` for Xtensa or
``riscv64-zephyr-elf-`` for RISC-V:

.. code-block:: console

   $ <toolchain-prefix>gcc -mlong-calls -c -o build/hello_world.llext samples/subsys/applet/shell_loader/applet/hello_world_applet.c

``ping_applet.c`` and ``pong_applet.c`` include
:zephyr_file:`include/zephyr/kernel.h` and the sample's own
:zephyr_file:`samples/subsys/applet/shell_loader/applet_link.h`, so they must
be built through the Zephyr build system (or the
:ref:`LLEXT EDK <llext_build_edk>`) to pick up the right include paths.

.. note::

   LLEXT targets do not inherit the optimization level of the main image. The
   sample therefore passes an explicit ``-Os`` to them: without it,
   :c:func:`k_msleep` keeps a call to the 64-bit division helper of libgcc,
   which is not part of the kernel's exported symbol table and makes the load
   fail with ``Undefined symbol with no entry in symbol table``.

.. note::

   The applet subsystem looks up the entry point by symbol name
   (``applet_main`` by default, see ``APPLET_ENTRY_SYM``), so that symbol must
   be present in the underlying LLEXT extension's export table. LLEXT by default
   only exports symbols explicitly marked with the :c:macro:`EXPORT_SYMBOL` macro, which
   requires using the full Zephyr build system, or at least the
   :ref:`LLEXT EDK <llext_build_edk>`.

   To avoid this complexity and allow users to compile the applets without
   the build system or EDK, this sample configures Zephyr to use all global
   symbols defined in the extension ELF file via the Kconfig option
   :kconfig:option:`CONFIG_LLEXT_IMPORT_ALL_GLOBALS`. This is not recommended
   for large extensions as the memory usage increases significantly.

The compiled extension can be inspected with the usual binutils utilities and
then converted to a hex string usable by the ``applet load_hex`` shell command:

.. code-block:: console

  $ <toolchain-prefix>objdump -r -d -x build/hello_world.llext
  $ xxd -p -c 99999 build/hello_world.llext

Running
*******

Once the board has booted, you will be presented with a shell prompt.
All the applet related commands are available as sub-commands of ``applet``,
and can be seen with ``applet help``:

.. code-block:: console

  uart:~$ applet help
  applet - Applet commands
  Subcommands:
    list        :List loaded applets, their state and thread count
    load_hex    :Load an elf file encoded in hex directly from the shell input.
                 The seed is handed to every thread of the applet as its
                 argument.
                 Syntax:
                 <applet_name> <ext_hex_string> [seed]
    add_thread  :Give a loaded applet an extra thread running an exported symbol
                 of its extension. Threads of the same applet share its memory
                 domain.
                 Syntax:
                 <applet_name> <symbol>
    start       :Start a loaded applet, running its applet_main() entry point.
                 Syntax:
                 <applet_name>
    join        :Wait for every thread of an applet to finish.
                 Syntax:
                 <applet_name> [timeout_ms]
    kill        :Abort every running thread of an applet.
                 Syntax:
                 <applet_name>
    unload      :Unload an applet and release its ELF buffer.
                 Syntax:
                 <applet_name>

The hex string generated above can be used to load the extension as an applet.
The optional third argument is a seed, handed to every thread of the applet as
its ``void *arg``:

.. code-block:: console

  uart:~$ applet load_hex hello <hex> 42
  Successfully loaded applet hello (852 bytes, seed 42)

Loading an applet does not run any of its code yet: it only loads the ELF and
attaches a thread to the ``applet_main`` entry point. The applet can then be
listed, started, waited on, and unloaded:

.. code-block:: console

  uart:~$ applet list
  | Name             | State      | Threads |
  |            hello |     loaded |       1 |
  uart:~$ applet start hello
  hello world from applet (arg=0x2a)
  Started applet hello
  uart:~$ applet join hello
  Applet hello finished
  uart:~$ applet unload hello
  Unloaded applet hello

A pair of cooperating applets
*****************************

``ping`` and ``pong`` show the rest of the applet features: several threads
inside one applet, two applets running at the same time, joining an applet that
is still busy, and killing one that never returns.

They exchange values through a small structure the application declares in its
own memory and exports to the extensions, see
:zephyr_file:`samples/subsys/applet/shell_loader/applet_link.h`. Under
:kconfig:option:`CONFIG_USERSPACE` that structure lives in an application
memory partition which the shell adds to every applet's memory domain with
:c:func:`applet_add_partition`, so the applets can reach it but not each
other's private data.

``pong`` is the service side, so start it first. It runs until killed:

.. code-block:: console

  uart:~$ applet load_hex pong <hex> 100
  Successfully loaded applet pong (1084 bytes, seed 100)
  uart:~$ applet start pong
  [pong] folding 100 into every exchange until killed
  Started applet pong

``ping`` needs a second thread, which has to be added while the applet is still
in the ``loaded`` state. ``applet_main`` generates pseudo-random samples from
the seed, and ``ping_sum`` accumulates them and hands the running total to
``pong``:

.. code-block:: console

  uart:~$ applet load_hex ping <hex> 7
  Successfully loaded applet ping (1528 bytes, seed 7)
  uart:~$ applet add_thread ping ping_sum
  Added thread ping_sum to applet ping
  uart:~$ applet list
  | Name             | State      | Threads |
  |             pong |    running |       1 |
  |             ping |     loaded |       2 |
  uart:~$ applet start ping
  Started applet ping
  [ping/sum] exchange 1: total 161
  [ping/sum] exchange 2: total 494
  [ping/sum] exchange 3: total 750

Joining an applet that is still running returns ``-EAGAIN`` once the timeout
expires, and leaves the applet untouched. Joining again with a long enough
timeout waits for both of its threads to return:

.. code-block:: console

  uart:~$ applet join ping 300
  Applet ping did not finish, return code -11
  uart:~$ applet join ping 5000
  [ping/gen] produced 8 samples
  [ping/sum] final total 1823
  Applet ping finished

``pong`` never returns on its own, so joining it would block forever. It is
stopped with ``applet kill``, which aborts every thread of that applet without
touching the rest of the system:

.. code-block:: console

  uart:~$ applet kill pong
  Killed applet pong
  uart:~$ applet list
  | Name             | State      | Threads |
  |             pong |       dead |       1 |
  |             ping |       dead |       2 |
  uart:~$ applet unload ping
  Unloaded applet ping
  uart:~$ applet unload pong
  Unloaded applet pong

The totals above are reproducible: they only depend on the two seeds. Loading
the same pair with different seeds produces a different sequence.

The number of extra threads an applet can be given is limited by
``APPLET_SHELL_MAX_THREADS`` in
:zephyr_file:`samples/subsys/applet/shell_loader/src/main.c` (two by default),
which sizes the statically allocated stack array.

Loading multiple applets
************************

Each ``load_hex`` invocation allocates its own ELF buffer and stack slots, and
tracks them under the applet name. All of them are released when ``unload`` is
called for that name, so several applets can stay loaded and run independently:

.. code-block:: console

  uart:~$ applet load_hex hello <hex>
  uart:~$ applet load_hex worker <hex>
  uart:~$ applet list
  | Name             | State      | Threads |
  |            hello |     loaded |       1 |
  |           worker |     loaded |       1 |
  uart:~$ applet start hello
  uart:~$ applet start worker
  uart:~$ applet unload worker

The number of simultaneously loaded applets is limited by the
``APPLET_SHELL_MAX_LOADED`` slot count in
:zephyr_file:`samples/subsys/applet/shell_loader/src/main.c` (four by default).
Adjust it there if more concurrent applets are needed.

Running applets in user mode
****************************

When :kconfig:option:`CONFIG_USERSPACE` is enabled, the applet subsystem places
each applet in its own memory domain and runs its threads unprivileged, so a
misbehaving applet cannot corrupt the kernel or other applets. This requires a
target with hardware memory protection, which is disabled in this sample's
default configuration to keep the hex loading workflow simple.

The whole walkthrough above also works in user mode. Build it with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/applet/shell_loader
   :board: mps2/an385
   :goals: build
   :gen-args: -DCONFIG_USERSPACE=y
   :compact:

Nothing changes in the extensions themselves: they keep calling
:c:func:`printk` and :c:func:`k_sleep`, which the kernel turns into system
calls when the caller is unprivileged.

.. note::

   On MPU based targets the number of partitions a memory domain can hold is
   bounded by the number of hardware regions left after the static ones, which
   on the Cortex-M platforms used here leaves very little headroom. An applet
   needs one partition per region of its extension, plus the C library
   partition added by the applet subsystem, plus the shared one this sample
   adds. Adding more partitions to ``applet_parts[]`` makes
   :c:func:`applet_add_partition` fail with ``-ENOSPC`` for the larger
   extensions.

   For the same reason the sample keeps
   :kconfig:option:`CONFIG_LLEXT_LOG_LEVEL_INF`: raising it to debug level
   makes ``llext_bootstrap()`` log from the applet's user mode thread, which
   then also needs ``k_log_partition`` in its domain.
