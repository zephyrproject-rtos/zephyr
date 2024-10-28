.. _native_posix:

Native POSIX execution (native_posix)
#######################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:

Overview
********

.. warning::
   ``native_posix`` is deprecated in favour of :ref:`native_sim<native_sim>`, and will be removed
   in the 4.2 release.

.. note::
   For native_posix users, if needed, :ref:`native_sim<native_sim>` includes a compatibility mode
   :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT`,
   which will set its configuration to mimic a native_posix-like configuration.

``native_posix`` is the predecessor of :ref:`native_sim<native_sim>`.
Just like with :ref:`native_sim<native_sim>` you can build your Zephyr application
with the Zephyr kernel, creating a normal Linux executable with your host tooling,
and can debug and instrument it like any other Linux program.

But unlike with :ref:`native_sim<native_sim>` you are limited to only using the host C library.
:ref:`native_sim<native_sim>` supports all ``native_posix`` use cases.

This board does not intend to simulate any particular HW, but it provides
a few peripherals such as an Ethernet driver, display, UART, etc., to enable
developing and testing application code which would require them.
This board supports the same :ref:`peripherals<native_sim_peripherals>`
:ref:`and backends as native_sim<native_sim_backends>`.

.. _native_posix_deps:

Host system dependencies
************************

Please check the
:ref:`Posix Arch Dependencies<posix_arch_deps>`

.. _native_important_limitations:

Important limitations
*********************

This board inherits
:ref:`the limitations of its architecture<posix_arch_limitations>`

Moreover, being limited to build only with the host C library, it is not possible to build
applications with the :ref:`Zephyr POSIX OS abstraction<posix_support>`, as there would be symbol
collisions between the host OS and this abstraction layer.

.. _native_posix_how_to_use:

How to use it
*************

To build, simply specify the ``native_posix`` board as target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_posix
   :goals: build
   :compact:

Now you have a Linux executable, ``./build/zephyr/zephyr.exe``, you can use just like any
other Linux program.

You can run, debug, build it with sanitizers or with coverage just like with
:ref:`native_sim<native_sim>`.
Please check :ref:`native_sim's how to<native_sim_how_to_use>` for more info.

.. _native_posix32_64:

32 and 64bit versions
*********************

Just like :ref:`native_sim<native_sim>`, ``native_posix`` comes with two targets:
A 32 bit and 64 bit version.
The 32 bit version, ``native_posix``, is the default target, which will compile
your code for the ILP32 ABI (i386 in a x86 or x86_64 system) where pointers
and longs are 32 bits.
This mimics the ABI of most embedded systems Zephyr targets,
and is therefore normally best to test and debug your code, as some bugs are
dependent on the size of pointers and longs.
This target requires either a 64 bit system with multilib support installed or
one with a 32bit userspace.

The 64 bit version, ``native_posix/native/64``, compiles your code targeting the
LP64 ABI (x86-64 in x86 systems), where pointers and longs are 64 bits.
You can use this target if you cannot compile or run 32 bit binaries.
