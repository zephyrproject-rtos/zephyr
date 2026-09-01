.. zephyr:board:: qemu_riscv64

Overview
********

The RISCV64 QEMU board configuration is used to emulate the RISCV64 architecture.

Get the Toolchain and QEMU
**************************

The minimum version of the `Zephyr SDK tools
<https://github.com/zephyrproject-rtos/sdk-ng/releases>`_
with toolchain and QEMU support for the RISCV64 architecture is v0.10.2.
Please see the :ref:`installation instructions <install-required-tools>`
for more details.

ELF Loading Convention
***********************

QEMU's RISC-V ``virt`` machine mirrors the boot behavior of OpenSBI's
``fw_dynamic``, ``fw_jump`` and ``fw_payload`` firmware, as well as the
Berkeley Boot Loader (BBL):

.. code-block:: c

   /*
    * NB: Use low address not ELF entry point to ensure that the fw_dynamic
    * behaviour when loading an ELF matches the fw_payload, fw_jump and BBL
    * behaviour, as well as fw_dynamic with a raw binary, all of which jump to
    * the (expected) load address load address. This allows kernels to have
    * separate SBI and ELF entry points (used by FreeBSD, for example).
    */

In other words, when an ELF is passed to QEMU via ``-kernel``, the vCPU's
program counter is set to the *lowest address the image is loaded at*, not
to the address recorded in the ELF header's ``e_entry`` field. This keeps
boot behavior consistent between raw binaries and ELF images and lets a
kernel expose an SBI entry point that differs from its ELF entry point.

This convention is normally invisible to Zephyr because the linker script
places the image's entry point (``CONFIG_KERNEL_ENTRY``) at the very start
of the ROM region, so the load address and the entry point happen to
coincide. It becomes a problem the moment the two addresses diverge such
as if the ROM region reserves space in front of ``rom_start`` for a
header or padding, because QEMU will then jump to the beginning of that
reserved space instead of to ``CONFIG_KERNEL_ENTRY`` and Zephyr will never
run.

:kconfig:option:`CONFIG_QEMU_DEVICE_LOADER` works around this by
replacing the ``-kernel`` option with one ``-device loader,file=<elf>``
entry per CPU (one for each of the :kconfig:option:`CONFIG_MP_MAX_NUM_CPUS`
cores configured). Unlike ``-kernel``, QEMU's generic loader device honors
the ELF's actual entry point, so every vCPU starts execution at
``CONFIG_KERNEL_ENTRY`` regardless of where it sits relative to the base of
ROM.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``qemu_riscv64`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_riscv64
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from riscv64!
        threadB: Hello World from riscv64!
        threadA: Hello World from riscv64!
        threadB: Hello World from riscv64!
        threadA: Hello World from riscv64!
        threadB: Hello World from riscv64!
        threadA: Hello World from riscv64!
        threadB: Hello World from riscv64!
        threadA: Hello World from riscv64!
        threadB: Hello World from riscv64!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
