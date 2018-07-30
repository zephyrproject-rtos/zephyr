
.. _native_posix:

Native POSIX execution (native_posix)
#######################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:

Overview
********

Using this board, a Zephyr application can be compiled together with
the Zephyr kernel, creating a normal console executable that runs as
a native application on the host OS, without emulation. Instead,
you use native host tools for compiling, debugging, and analyzing your
Zephyr application, eliminating the need for architecture-specific
target hardware in the early phases of development.

Host system dependencies
========================

This port is designed to run in POSIX compatible operating systems.
It has only been tested on Linux, but should also be compatible with macOS.

.. note::

   You must have the 32-bit C library installed in your system
   (in Ubuntu 16.04 install the gcc-multilib package)

.. note::

   This port will **not** work in Windows Subsystem for Linux (WSL) because WSL
   does not support native 32-bit binaries.

Important limitations
*********************

The underlying assumptions behind this port set some limitations on what
can and cannot be done.
These limitations are due to the code executing natively in
the host CPU without any instrumentation or means to interrupt it unless the
simulated CPU is sleeping.

You can imagine the code executes in a simulated CPU
which runs at an infinitely fast clock: No time passes while the CPU is
running.
Therefore interrupts, including timer interrupts, will not arrive
while code executes, except immediately after the SW enables or unmasks
them if they were pending.

This behavior is intentional, as it provides a deterministic environment to
develop and debug.
For more information please see the
`Rationale for this port`_ and `Architecture`_ sections

Therefore these limitations apply:

- There can **not** be busy wait loops in the application code that wait for
  something to happen without letting the CPU sleep.
  If busy wait loops do exist, they will behave as infinite loops and
  will stall the execution. For example, the following busy wait loop code,
  which could be interrupted on actual hardware, will stall the execution of
  all threads, kernel, and HW models:

  .. code-block:: c

     while (1){}

  Similarly the following code where we expect ``condition`` to be
  updated by an interrupt handler or another thread, will also stall
  the application when compiled for this port.

  .. code-block:: c

     volatile condition = true;
     while (condition){}


- Code that depends on its own execution speed will normally not
  work as expected. For example, code such as shown below, will likely not
  work as expected:

  .. code-block:: c

     peripheral_x->run = true;

     /* Wait for a number of CPU cycles */
     for (int i = 0; i < 100; i++) NOP;

     /* We expect the peripheral done and ready to do something else */


- This port is not meant to, and could not possibly help debug races between
  HW and SW, or similar timing related issues.

- You may not use hard coded memory addresses because there is no I/O or
  MMU emulation.


Working around these limitations
================================

If a busy wait loop exists, it will become evident as the application will be
stalled in it. To find the loop, you can run the binary in a debugger and
pause it after the execution is stuck; it will be paused in
some part of that loop.

The best solution is to remove that busy wait loop, and instead use
an appropriate kernel primitive to synchronize your threads.
Note that busy wait loops are in general a bad coding practice as they
keep the CPU executing and consuming power.

If removing the busy loop is really not an option, you may add a conditionally
compiled call to :c:func:`k_cpu_idle` if you are waiting for an
interrupt, or a call to :c:func:`k_busy_wait` with some small delay in
microseconds.
In the previous example, modifying the code as follows would work:

.. code-block:: c

   volatile condition = true;
   while (condition) {
   	#if defined(CONFIG_ARCH_POSIX)
   		k_cpu_idle();
   	#endif
   }


How to use it
*************

Compiling
=========

Specify the native_posix board target to build a native POSIX application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_posix
   :goals: build
   :compact:

Running
=======

The result of the compilation is an executable (zephyr.exe) placed in the
zephyr/ subdirectory of the build folder.
Run the zephyr.exe executable as you would any other Linux console application.

.. code-block:: console

   $ zephyr/zephyr.exe
   # Press Ctrl+C to exit

This executable accepts several command line options depending on the
compilation configuration.
You can run it with the ``--help`` command line switch to get a list of
available options::

   $ zephyr/zephyr.exe --help

     [-h] [--h] [--help] [-?]  :Display this help
     [-rt]                     :Slow down the execution to the host real time
     [-no-rt]                  :Do NOT slow down the execution to real time, but
                                advance Zephyr's time as fast as possible and
                                decoupled from the host time
     [-rt-drift=<drift>]       :Drift of the simulated clock relative to the
                                real host time.
                                Normally this would be set to a value of a few
                                ppm (e.g. 50e-6)
                                This option has no effect in non-real time mode
     [-rt-ratio=<ratio>]       :Relative speed of the simulated time vs real
                                time, for example, set to 2 to have simulated
                                time pass at double the speed of real time.
                                Note that both rt-drift & rt-ratio adjust the
                                same clock speed, and therefore it does not make
                                sense to use them simultaneously.
                                This option has no effect in non-real time mode
     [-rtc-offset=<offset>]    :At boot, offset the RTC by this number of
                                seconds.
     [-rtc-reset]              :Start the simulated real time clock at 0.
                                Otherwise, it is started at the value of the
                                host's RTC.
     [-stop_at=<time>]         :In simulated seconds, when to stop automatically
     [-seed=<r_seed>]          :Seed for the entropy device
     [-testargs <arg>...]      :Any argument that follows will be ignored
                                by the top level, and made
                                available for possible tests

Note that the Zephyr kernel does not actually exit once the application is
finished. It simply goes into the idle loop forever.
Therefore you must stop the application manually (Ctrl+C in Linux).

Application tests using the ``ztest`` framework will exit after all
tests have completed.

If you want your application to gracefully finish when it reaches some point,
you may add a conditionally compiled (:option:`CONFIG_ARCH_POSIX`) call to
``posix_exit(int status)`` at that point.

Debugging
=========

Since the Zephyr executable is a native application, it can be debugged and
instrumented as any other native program. The program is compiled with debug
information, so it can be run directly in, for example, ``gdb`` or instrumented
with ``valgrind``.

Because the execution of your Zephyr application is normally deterministic
(there are no asynchronous or random components), you can execute the
code multiple times and get the exact same result. Instrumenting the
code does not affect its execution.

To ease debugging you may want to compile your code without optimizations
(e.g., -O0) by setting :option:`CONFIG_NO_OPTIMIZATIONS`.

Address Sanitizer (ASan)
========================

You can also build Zephyr with `Address Sanitizer`_. To do this, set
:option:`CONFIG_ASAN`, for example, in the application project file, or in the
cmake command line invocation.

Note that you will need the ASan library installed in your system.
In Debian/Ubuntu this is ``libasan1``.

.. _Address Sanitizer:
   https://github.com/google/sanitizers/wiki/AddressSanitizer

Rationale for this port
***********************

The main intents of this port are:

- Allow functional debugging, instrumentation and analysis of the code with
  native tooling.
- Allow functional regression testing, and simulations in which we have the
  full functionality of the code.
- Run tests fast: several minutes of simulated time per wall time second.
- Possibility to connect to external tools which may be able to run much
  faster or much slower than real time.
- Deterministic, repeatable runs:
  There must not be any randomness or indeterminism (unless host peripherals
  are used).
  The result must **not** be affected by:

  - Debugging or instrumenting the code.
  - Pausing in a breakpoint and continuing later.
  - The host computer performance or its load.

The aim of this port is not to debug HW/SW races, missed HW programming
deadlines, or issues in which an interrupt comes when it was not expected.
Normally those would be debugged with a cycle accurate Instruction Set Simulator
(ISS) or with a development board.

Comparison with other options
*****************************

This port does not try to replace cycle accurate instruction set simulators
(ISS), development boards, or QEMU, but to complement them. This port's main aim
is to meet the targets described in the previous `Rationale for this port`_
section.

.. figure:: Port_vs_QEMU_vs.svg
    :align: center
    :alt: Comparison of different debugging targets
    :figclass: align-center

    Comparison of different debugging options. Note that realism has many
    dimensions: Having the real memory map or emulating the exact time an
    instruction executes is just some of it; Emulating peripherals accurately
    is another side.

This native port compiles your code directly to x86, with no instrumentation or
monitoring code. Your code executes directly in the host CPU. That is, your code
executes just as fast as it possibly can.

Simulated time is normally decoupled from real host time.
The problem of how to emulate the instruction execution speed is solved
by assuming that code executes in zero simulated time.

There is no I/O or MMU emulation. If you try to access memory through hardcoded
addresses your binary will simply segfault.
The drivers and HW models for this architecture will hide this from the
application developers when it relates to those peripherals.
In general this port is not meant to help developing low level drivers for
target HW. But for developing application code.

Your code can be debugged, instrumented, or analyzed with all normal native
development tools just like any other Linux application.

Execution is fully reproducible, you can pause it without side-effects.

How does this port compare to QEMU:
===================================

With QEMU you compile your image targeting the board which is closer to
your desired board. For example an ARM based one. QEMU emulates the real memory
layout of the board, loads the compiled binary and through instructions
translation executes that ARM targeted binary on the host CPU.
Depending on configuration, QEMU also provides models of some peripherals
and, in some cases, can expose host HW as emulated target peripherals.

QEMU cannot provide any emulation of execution speed. It simply
executes code as fast as it can, and lets the host CPU speed determine the
emulated CPU speed. This produces highly indeterministic behavior,
as the execution speed depends on the host system performance and its load.

As instructions are translated to the host architecture, and the target CPU and
MMU are emulated, there is a performance penalty.

You can connect gdb to QEMU, but have few other instrumentation abilities.

Execution is not reproducible. Some bugs may be triggered only in some runs
depending on the computer and its load.

How does this port compare to an ISS:
======================================

With a cycle accurate instruction set simulator you compile targeting either
your real CPU/platform or a close enough relative. The memory layout is modeled
and some or all peripherals too.

The simulator loads your binary, slowly interprets each instruction, and
accounts for the time each instruction takes.
Time is simulated and is fully decoupled from real time.
Simulations are on the order of 10 to 100 times slower than real time.

Some instruction set simulators work with gdb, and may
provide some extra tools for analyzing your code.

Execution is fully reproducible. You can normally pause your execution without
side-effects.


Architecture
************

.. figure:: native_layers.svg
    :align: center
    :alt: Zephyr layering in native build
    :figclass: align-center

    Zephyr layering when built against an embedded target (left), and
    targeting the native_posix board (right)

This board is based on the POSIX architecture port of Zephyr.
In this architecture each Zephyr thread is mapped to one POSIX pthread,
but only one of these pthreads executes at a time.
This architecture provides the same interface to the Kernel as other
architectures and is therefore transparent for the application.

This board does not try to emulate any particular embedded CPU or SOC.
The code is compiled natively for the host x86 system, as a 32-bit
binary assuming pointer and integer types are 32-bits wide.

To ensure determinism when the Zephyr code is running,
and to ease application debugging,
the board uses a different time than real time: simulated time.
This simulated time is, in principle, not linked to the host time.

The Zephyr application sees the code executing as if the CPU were running at
an infinitely fast clock, and fully decoupled from the underlying host CPU
speed.
No simulated time passes while the application or kernel code execute.

The CPU boot is emulated by creating the Zephyr initialization thread and
letting it run. This in turn may spawn more Zephyr threads.
Eventually the SW will run to completion, that is, it will set the CPU
back to sleep.

At this point, control is transferred back to the HW models and the simulation
time can be advanced.

When the HW models raise an interrupt, the CPU wakes back up, the interrupt
is handled, the SW runs until completion again, and control is
transferred back to the HW models, all in zero simulated time.

If the SW unmasks a pending interrupt while running, or triggers a SW
interrupt, the interrupt controller may raise the interrupt immediately
depending on interrupt priorities, masking, and locking state.

About time in native_posix
==========================

Normally simulated time runs fully decoupled from the real host time
and as fast as the host compute power would allow.
This is desirable when running in a debugger or testing in batch, but not if
interacting with external interfaces based on the real host time.

The Zephyr kernel is only aware of the simulated time as provided by the
HW models. Therefore any normal Zephyr thread will also know only about
simulated time.

The only link between the simulated time and the real/host time, if any,
is created by the clock and timer model.

This model can be configured to slow down the execution of native_posix to
real time.
You can do this with the ``--rt`` and ``--no-rt`` options from the command line.
The default behavior is set with
:option:`CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME`.
Note that all this model does is wait before raising the
next system tick interrupt until the corresponding real/host time.
If, for some reason, native_posix runs slower than real time, all this
model can do is "catch up" as soon as possible by not delaying the
following ticks.
So if the host load is too high, or you are running in a debugger, you will
see simulated time lagging behind the real host time.
This solution ensures that normal runs are still deterministic while
providing an illusion of real timeness to the observer.

When locked to real time, simulated time can also be set to run faster or
slower than real time.
This can be controlled with the ``--rt-ratio=<ratio>`` and ``-rt-drift=<drift>``
command line options. Note that both of these options control the same
underlying mechanism, and that ``drift`` is by definition equal to
``ratio - 1``.
It is also possible to adjust this clock speed on the fly with
:c:func:`native_rtc_adjust_clock()`.

In this way if, for example, ``--rt-ratio=2`` is given, the simulated time
will advance at twice the real time speed.
Similarly if ``--rt-drift=-100e-6`` is given, the simulated time will progress
100ppm slower than real time.
Note that the these 2 options have no meaning when running in non real-time
mode.

How simulated time and real time relate to each other
-----------------------------------------------------

Simulated time (``st``) can be calculated from real time (``rt``) as

``st = (rt - last_rt) * ratio + last_st``

And vice-versa:

``rt = (st - last_st) / ratio + last_rt``

Where ``last_rt`` and ``last_st`` are respectively the real time and the
simulated time when the last clock ratio adjustment took place.

All times are kept in microseconds.

Peripherals
***********

The following peripherals are currently provided with this board:

**Console driver**:
  A console driver is provided which by default is configured to:

  - Redirect any :c:func:`printk` write to the native host application's
    ``stdout``.

  - Feed any input from the native application ``stdin`` to a possible
    running :ref:`Shell`. For more information refer to the section
    `Shell support`_.

**Clock, timer and system tick model**
  This model provides the system tick timer. By default
  :option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` configures it to tick every 10ms.

  This peripheral driver also provides the needed functionality for this
  architecture-specific :c:func:`k_busy_wait`.

  Please refer to the section `About time in native_posix`_ for more
  information.

**Real time clock**
  The real time clock model provides a model of a constantly powered clock.
  By default this is initialized to the host time at boot.

  This RTC can also be set to start from time 0 with the ``--rtc-reset`` command
  line option.

  It is possible to offset the RTC clock value at boot with the
  ``--rtc-offset=<offset>`` option,
  or to adjust it dynamically with the function :c:func:`native_rtc_offset`.

  After start, this RTC advances with the simulated time, and is therefore
  affected by the simulated time speed ratio.
  See `About time in native_posix`_ for more information.

  The time can be queried with the functions :c:func:`native_rtc_gettime_us`
  and :c:func:`native_rtc_gettime`. Both accept as parameter the clock source:

  - ``RTC_CLOCK_BOOT``: It counts the simulated time passed since boot.
    It is not subject to offset adjustments
  - ``RTC_CLOCK_REALTIME``: RTC persistent time. It is affected by
    offset adjustments.
  - ``RTC_CLOCK_PSEUDOHOSTREALTIME``: A version of the real host time,
    as if the host was also affected by the clock speed ratio and offset
    adjustments performed to the simulated clock and this RTC. Normally
    this value will be a couple of hundredths of microseconds ahead of the
    simulated time, depending on the host execution speed.
    This clock source should be used with care, as depending on the actual
    execution speed of native_posix and the host load,
    it may return a value considerably ahead of the simulated time.

**Entropy device**:
  An entropy device based on the host :c:func:`random` API.
  This device will generate the same sequence of random numbers if initialized
  with the same random seed.
  You can change this random seed value by using the command line option:
  ``--seed=<random_seed>`` where the value specified is a 32-bit integer
  such as 97229 (decimal),  0x17BCD (hex), or 0275715 (octal).

**Interrupt controller**:
  A simple yet generic interrupt controller is provided. It can nest interrupts
  and provides interrupt priorities. Interrupts can be individually masked or
  unmasked. SW interrupts are also supported.

**Ethernet driver**:
  A simple TAP based ethernet driver is provided. The driver will create
  a **zeth** network interface to the host system. One can communicate with
  Zephyr via this network interface. Multiple TAP based network interfaces can
  be created if needed. The IP address configuration can be specified for each
  network interface instance.
  See :option:`CONFIG_ETH_NATIVE_POSIX_SETUP_SCRIPT` option for more details.
  The :ref:`eth-native-posix-sample` sample app provides
  some use examples and more information about this driver configuration.

  Note that this device can only be used with Linux hosts, and that the user
  needs elevated permissions.

**Bluetooth controller**:
  It's possible to use the host's Bluetooth adapter as a Bluetooth
  controller for Zephyr. To do this the HCI device needs to be passed as
  a command line option to ``zephyr.exe``. For example, to use ``hci0``,
  use ``sudo zephyr.exe --bt-dev=hci0``. Using the device requires root
  privileges (or the CAP_NET_ADMIN POSIX capability, to be exact) so
  ``zephyr.exe`` needs to be run through ``sudo``. The chosen HCI device
  must be powered down and support Bluetooth Low Energy (i.e. support the
  Bluetooth specification version 4.0 or greater).

Shell support
*************

When the :ref:`Shell` subsystem is compiled with your application, the native
standard input (``stdin``) will be redirected to the shell.
You may use the shell interactively through the console,
by piping another process output to it, or by feeding it a file.

When using it interactively you may want to select the option
:option:`CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME`.

When feeding ``stdin`` from a pipe or file, the console driver will ensure
reproducibility between runs of the process:

- The execution of the process will be stalled while waiting for new ``stdin``
  data to be ready.

- Commands will be fed to the shell as fast as the shell can process them.
  To allow controlling the flow of commands to the shell, you may use the
  driver directive ``!wait <ms>``.

- When the file ends, or the pipe is closed the driver will stop attempting to
  read it.

Driver directives
=================

The console driver understands a set of special commands: driver directives.
These directives are captured by the console driver itself and are not
forwarded to the shell.
These directives are:

- ``!wait <ms>``: When received, the driver will pause feeding commands to the
  shell for ``<ms>`` milliseconds.

- ``!quit``: When received the driver will cause the application to gracefully
  exit by calling :c:func:`posix_exit`.


Use example
===========

For example, you can build the shell sample app:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: native_posix
   :goals: build
   :compact:

And feed it the following set of commands through a pipe:

.. code-block:: console

   echo -e \
   'select kernel\nuptime\n!wait 500\nuptime\n!wait 1000\nuptime\n!quit' \
   | zephyr/zephyr.exe
