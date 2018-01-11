
.. _native_posix:

Native POSIX execution (native_posix)
#######################################

Overview
********

Using this board, a Zephyr application can be compiled together with
the Zephyr kernel, creating a normal console executable that runs as
a native application on the host OS, without emulation.  Instead,
you use native host tools for compiling, debugging, and analyzing your
Zephyr application, eliminating the need for architecture-specific
target hardware in the early phases of development.

.. figure:: native_layers.svg
    :align: center
    :alt: Zephyr layering in native build
    :figclass: align-center

    Zephyr layering when built against an embedded target (left), and
    targeting the native_posix board (right)


Host system dependencies
========================

This port is designed to run in POSIX compatible operating systems.
It has only been tested on Linux, but should also be compatible with macOS.

.. note::

   You must have the 32-bit C library installed in your system
   (in Ubuntu 16.04 install the gcc-multilib package)


Architecture
************

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
the issue of code execution speed is ignored.
The board uses a different time than real time: simulated time.
This simulated time is, in principle, not linked to the host time.

The Zephyr application sees the code executing as if the CPU were running at
an infinitely high clock, and fully decoupled from the underlying host CPU
speed.
No simulated time passes while the application or kernel code execute.

The CPU boot is emulated by creating the Zephyr initialization thread and
letting it run. This in turn may spawn more Zephyr threads.
Eventually the SW will run to completion, that is, it will set the CPU
back to sleep.

At this point, control is transferred back to the HW models and the simulation
time can be advanced.

When the HW models raise an interrupt, the CPU wakes back up: the interrupt
is handled, the SW runs until completion again, and control is
transferred back to the HW models, all in zero simulated time.

If the SW unmasks a pending interrupt while running, or triggers a SW
interrupt, the interrupt controller may raise the interrupt immediately
depending on interrupt priorities, masking, and locking state.

Normally the resulting executable runs fully decoupled from the real host time.
That is, simulated time will advance as fast as it can. This is desirable when
running in a debuger or testing in batch, but not if one wants to interact
with external interfaces which are based on the real host time.

Peripherals
***********

The following peripherals are currently provided with this board:

**Console/printk driver**:

A driver is provided that redirects any printk write to the native
host application's stdout.

**Simple timer**:

A simple timer provides the kernel with a 10ms tick.
This peripheral driver also provides the needed functionality for this
architecture-specific k_busy_wait().

This timer, may also be configured with NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME
to slow down the execution to real host time.
This will provide the illusion that the simulated time is running at the same
speed as the real host time.
In reality, the timer will monitor how much real host time
has actually passed since boot, and when needed, the timer will pause
the execution before raising each timer interrupt.
Normally the Zephyr application and HW models run in very little time
on the host CPU, so this is a good enough approach.


**Interrupt controller**

A simple yet generic interrupt controller is provided. It can nest interrupts
and provides interrupt priorities. Interrupts can be individually masked or
unmasked. SW interrupts are also supported.


Important limitations
=====================

The assumption that simulated time can only pass while the CPU is sleeping
means that there can not be busy wait loops in the application code that
wait for something to happen without letting the CPU sleep.
If busy wait loops do exist, they will behave as infinite loops and
will stall the execution.

As simulated time does not pass while the CPU is running, it also means no HW
interrupts will interrupt the threads' execution unless the SW enables or
unmasks them.
This is intentional, as it provides a deterministic environment to develop and
debug.
But note that this may hide issues in the SW that may only be triggered in the
real platform.

This native port of Zephyr is not meant to, and could not possibly
help debug races between HW and SW, or similar timing related issues.


How to use it
*************

Compiling
=========

Specify the native_posix board target to build a native POSIX application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
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

Note that the Zephyr kernel does not actually exit once the application is
finished. It simply goes into the idle loop forever.
Therefore you must stop the application manually (Ctrl+C in Linux).

Application tests using the ``ztest`` framework will exit after all
tests have completed.

If you want your application to gracefully finish when it reaches some point,
you may add a conditionally compiled (CONFIG_BOARD_NATIVE_POSIX) call to
``main_clean_up(exit_code)`` at that point.


Debugging
=========

Since the Zephyr executable is a native application, it can be debuged and
instrumented as any other native program. The program is compiled with debug
information, so it can be run directly in, for example, ``gdb`` or instrumented
with ``valgrind``.

Because the execution of your Zephyr application is fully deterministic
(there are no asynchronous or random components), you can execute the
code multiple times and get the exact same result. Instrumenting the
code does not affect its execution.