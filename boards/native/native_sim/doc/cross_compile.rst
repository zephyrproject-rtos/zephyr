.. _posix_arch_cross_compile:

Cross-compiling the POSIX architecture
**************************************

Even though the main use case for native_sim and other POSIX architecture based targets is to allow
you to debug and instrument your code in your workstation during development, in some cases it can
be useful to build it in one machine and execute it in another. When these machines have different
processor architectures, we need to cross compile it.

.. warning::

   native_sim is a development and debug aid, meant to be reproducible and allow easy
   instrumentation and access to the execution status. For security reasons, you should never
   deliver final products in which the production code is being run in native_sim.

Preparing your host
===================

.. note::

   This instructions guide you on how to set an Ubuntu 24.04 x86_64 development machine for
   cross-compiling for 32bit (``arm`` or ``armhf``) and 64bit (``aarch64``) ARM targets.
   And with that setup how to cross-compile for such a target.
   But there is different ways to setup your cross-compiling environment, and you may have a
   different host or target architecture. Therefore you can treat this guide as inspiration.

The first thing you need, is to have a cross-compiling toolchain in your development computer.
On an Ubuntu 24.04 x86_64 you can install the ``gcc-aarch64-linux-gnu`` package for 64bit ARM
builds. This will install these toolchain binaries as ``/usr/bin/aarch64-linux-gnu-*`` and its
libraries and headers in ``/usr/aarch64-linux-gnu/``.
Similarly, for the 32bit ``armhf`` the ``gcc-arm-linux-gnueabihf`` package, will install the
binaries as ``/usr/bin/arm-linux-gnueabihf-*`` and its libraries and headers in
``/usr/arm-linux-gnueabihf/``.

.. note::

   In Ubuntu 24.04 the ``gcc-multilib`` package conflicts with the ``gcc-aarch64-linux-gnu``
   package, and, to support 32bit builds in x86_64, instructions guide you to install it.
   This is not strictly a problem, as ``gcc-multilib`` is mostly a meta-package. What is required
   for 32bit builds in ``x86_64`` machines are the *dependencies* of ``gcc-multilib``. It is fine to
   remove ``gcc-multilib`` and ``g++-multilib`` after installing them, while leaving their
   dependencies, or installing these dependencies directly. Just be sure to not remove these
   dependencies after.

Building
========

To build you need to pass to cmake the following options:

  * ``ZEPHYR_TOOLCHAIN_VARIANT=cross-compile``
  * ``NATIVE_TARGET_HOST`` set for the target machine. In this example, that is ``aarch64``
    for 64bit ARM builds, or ``arm`` for 32bit ARM builds.
  * ``CROSS_COMPILE`` with the cross-compiling toolchain prefix. That is
    ``/usr/bin/aarch64-linux-gnu-``, or ``/usr/bin/arm-linux-gnueabihf-``.

Depending on your compiler you may also need to set ``SYSROOT_DIR`` to your compiler sysroot path.

In this example, building the ``hello_world`` sample for ``aarch64`` would be:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_sim//64
   :goals: build
   :gen-args: -DNATIVE_TARGET_HOST=aarch64 -DZEPHYR_TOOLCHAIN_VARIANT=cross-compile -DCROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
   :compact:

Or for 32bit ``armhf``:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_sim
   :goals: build
   :gen-args: -DNATIVE_TARGET_HOST=arm -DZEPHYR_TOOLCHAIN_VARIANT=cross-compile -DCROSS_COMPILE=/usr/bin/arm-linux-gnueabihf-
   :compact:

Now you have an executable, ``zephyr/zephyr.exe``, you can copy and run in your target machine.

A quick test: Executing it in your host
=======================================

It is possible to execute this cross compiled executable in your development host. For this you need
`qemu's user space emulator <https://www.qemu.org/docs/master/user/main.html>`_, which in
Ubuntu 24.04 you can install with the ``qemu-user`` package. With this, you can run the executable
as:

.. code-block:: console

   $ qemu-aarch64 -L /usr/aarch64-linux-gnu/ build/zephyr/zephyr.exe
   # Press Ctrl+C to exit

For the 64bit ARM version, or

.. code-block:: console

   $ qemu-armhf -L /usr/arm-linux-gnueabihf/ build/zephyr/zephyr.exe
   # Press Ctrl+C to exit

for the 32bit ARM version.
