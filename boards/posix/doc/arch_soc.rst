.. _Posix arch:

The POSIX architecture
######################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:

Overview
********

The POSIX architecture, in combination with the inf_clock SOC layer,
provides the foundation, architecture and SOC layers for a set of virtual test
boards.

Using these, a Zephyr application can be compiled together with
the Zephyr kernel, creating a normal executable that runs as
a native application on the host OS, without emulation. Instead,
you use native host tools for compiling, debugging, and analyzing your
Zephyr application, eliminating the need for architecture-specific
target hardware in the early phases of development.

.. note::

   The POSIX architecture is not related and should not be confused with the
   :ref:`POSIX OS abstraction<posix_support>`.
   The later provides an adapatation shim that enables running applications
   which require POSIX APIs on Zephyr.


Types of POSIX arch based boards
================================

Today there are two types of POSIX boards: The :ref:`native_posix<native_posix>`
board and the :ref:`bsim boards<bsim boards>`.
While they share the main objectives and principles, the first is intended as
a HW agnostic test platform which in some cases utilizes the host OS
peripherals, while the second intend to simulate a particular HW platform,
with focus on their radio (e.g. BT LE) and utilize the `BabbleSim`_ physical layer
simulation and framework, while being fully decoupled of the host.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _posix_arch_deps:

Host system dependencies
========================

This port is designed to run in POSIX compatible operating systems,
but it has only been tested on Linux.

.. note::

   You must have the 32-bit C library installed in your system
   (in Ubuntu 16.04 install the gcc-multilib package)

.. note::

   The 32 bit version of this port does not directly work in Windows Subsystem
   for Linux (WSL) because WSL does not support native 32-bit binaries.
   You may want to consider WSL2, or, if using native_posix,
   you can also just use the native_posix_64
   target: Check :ref:`32 and 64bit versions<native_posix32_64>`.
   Otherwise `with some tinkering
   <https://github.com/microsoft/WSL/issues/2468#issuecomment-374904520>`_ it
   should be possible to make it work.


.. _posix_arch_limitations:

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
`Rationale for this port`_ and :ref:`Architecture<posix_arch_architecture>`
sections

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


.. _posix_arch_rationale:

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


.. _posix_arch_compare:

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

This native port compiles your code directly for the host architectture
(typically x86), with no instrumentation or
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

.. _posix_arch_architecture:

Architecture and design
***********************

.. figure:: layering.svg
    :align: center
    :alt: Zephyr layering in native build
    :figclass: align-center

    Zephyr layering when built against an embedded target (left), and
    targeting a POSIX arch based board (right)

Arch layer
==========

In this architecture each Zephyr thread is mapped to one POSIX pthread.
The POSIX architecture emulates a single threaded CPU/MCU by only allowing
one SW thread to execute at a time, as commanded by the Zephyr kernel.
Whenever the Zephyr kernel desires to context switch two threads,
the POSIX arch blocks and unblocks the corresponding pthreads.

This architecture provides the same interface to the Kernel as other
architectures and is therefore transparent for the application.

When using this architecture, the code is compiled natively for the host system,
and typically as a 32-bit binary assuming pointer and integer types are 32-bits
wide.

Note that all threads use a normal Linux pthread stack, and do not use
the Zephyr thread stack allocation for their call stacks or automatic
variables. The Zephyr stacks (which are allocated in "static memory") are
only used by the POSIX architecture for thread bookkeeping.

SOC and board layers
====================

.. note::

   This description applies to all current POSIX arch based boards on tree,
   but it is not a requirement for another board to follow what is described here.

When the executable process is started (that is the the board
:c:func:`main`, which is the linux executable C :c:func:`main`),
first, early initialization steps are taken care of
(command line argument parsing, initialization of the HW models, etc).

After, the "CPU simulation" is started, by creating a new pthread
and provisionally blocking the original thread. The original thread will only
be used for HW models after this;
while this newly created thread will be the first "SW" thread and start
executing the boot of the embedded code (including the POSIX arch code).

During this MCU boot process, the Zephyr kernel will be initialized and
eventually this will call into the embedded application `main()`,
just like in the embedded target.
As the embedded SW execution progresses, more Zephyr threads may be spawned,
and for each the POSIX architecture will create a dedicated pthread.

Eventually the simulated CPU will be put to sleep by the embedded SW
(normally when the boot is completed). This whole simulated CPU boot,
until the first time it goes to sleep happens in 0 simulated time.

At this point the last executing SW pthread will be blocked,
and the first thread (reserved for the HW models now) will be allowed
to execute again. This thread will, from now on, be the one handling both the
HW models and the device simulated time.

The HW models are designed around timed events,
and this thread will check what is the next
scheduled HW event, advance simulated time until that point, and call the
corresponding HW model event function.

Eventually one of these HW models will raise an interrupt to the simulated CPU.
When the IRQ controller wants to wake the simulated CPU, the HW thread is
blocked, and the simulated CPU is awaken by letting the last SW thread continue
executing.

This process of getting the CPU to sleep, letting the HW models run,
and raising an interrupt which wake the CPU again is repeated until the end
of the simulation, where the CPU execution always takes 0 simulated time.

When a SW thread is awakened by an interrupt, it will be made to enter the
interrupt handler by the soc_inf code.

If the SW unmasks a pending interrupt while running, or triggers a SW
interrupt, the interrupt controller may raise the interrupt immediately
depending on interrupt priorities, masking, and locking state.

Interrupts are executed in the context (and using the stack) of the SW
thread in which they are received. Meaning, there is no dedicated thread or
stack for interrupt handling.

To ensure determinism when the Zephyr code is running,
and to ease application debugging,
the board uses a different time than real time: simulated time.
How and if simulated time relates to the host time, is up to the simulated
board.

The Zephyr application sees the code executing as if the CPU were running at
an infinitely fast clock, and fully decoupled from the underlying host CPU
speed.
No simulated time passes while the application or kernel code execute.

.. _posix_busy_wait:

Busy waits
==========

Busy waits work thanks to provided board functionality.
This does not need to be the same for all boards, but both native_posix and the
nrf52_bsim board work similarly thru the combination of a board specific
`arch_busy_wait()` and a special fake HW timer (provided by the board).

When a SW thread wants to busy wait, this fake timer will be programmed in
the future time corresponding to the end of the busy wait and the CPU will
be put immediately to sleep in the busy_wait caller context.
When this fake HW timer expires the CPU will be waken with a special
non-maskable phony interrupt which does not have a corresponding interrupt
handler but will resume the busy_wait SW execution.
Note that other interrupts may arrive while the busy wait is in progress,
which may delay the `k_busy_wait()` return just like in real life.

Interrupts may be locked out or masked during this time, but the special
fake-timer non-maskable interrupt will wake the CPU nonetheless.


NATIVE_TASKS
============

The soc_inf layer provides a special type of hook called the NATIVE_TASKS.

These allow registering (at build/link time) functions which will be called
at different stages during the process execution: Before command line parsing
(so dynamic command line arguments can be registered using this hook),
before initialization of the HW models, before the simulated CPU is started,
after the simulated CPU goes to sleep for the first time,
and when the application exists.
