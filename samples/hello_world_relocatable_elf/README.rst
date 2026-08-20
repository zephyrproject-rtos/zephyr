.. zephyr:code-sample:: hello_world_relocatable_elf
   :name: Hello World (Relocatable ELF)

   Print "Hello World" from a self-relocating PIE image and verify its
   runtime address matches (or differs from) its link-time address.

Overview
********

A variant of the :zephyr:code-sample:`hello_world` sample built as a
self-relocating position-independent executable. After linking, the build
raises the load address (the LMA, or ``PhysAddr`` in ``readelf -l`` output) of
every segment by ``SAMPLE_LOAD_OFFSET`` (``0x1000`` by default). QEMU's ELF
loader places each segment at its LMA, so the image runs away from the address
it was linked for and has to relocate itself.

At runtime the sample prints the address ``main()`` was linked at, derived
from the load offset recorded by the early self-relocation code, and the
address ``main()`` is actually running at. It asserts that the recorded load
offset equals ``SAMPLE_LOAD_OFFSET`` and that a statically initialized pointer
to ``main()`` was relocated to the address ``main()`` runs at, which needs
:kconfig:option:`CONFIG_ASSERT`, enabled in ``prj.conf``. A failed assertion
stops the sample before it prints the "Relocation OK" line that twister looks
for.

Requirements
************

Position-independent executables need an LLVM toolchain: the GNU linker in the
Zephyr SDK does not support ``-pie`` for bare-metal RISC-V targets. Use the
Zephyr SDK LLVM (``ZEPHYR_TOOLCHAIN_VARIANT=zephyr/llvm``) or another LLVM
toolchain with a position-independent C library. The libc must be built with
``-fPIC``, which is why ``prj.conf`` selects the picolibc module instead of the
toolchain's prebuilt libc.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world_relocatable_elf
   :host-os: unix
   :board: qemu_riscv32
   :gen-args: -DZEPHYR_TOOLCHAIN_VARIANT=zephyr/llvm
   :goals: run
   :compact:

To build for another board, change "qemu_riscv32" above to that board's name.
Pass ``-DSAMPLE_LOAD_OFFSET=<offset>`` to use a different offset, or ``0`` to
run the image at its link address.

Sample Output
=============

.. code-block:: console

    Hello World! qemu_riscv32/qemu_virt_riscv32
    main() linked at:  0x8000349e
    main() running at: 0x8000449e
    Relocation OK: load offset 0x1000

The two addresses differ by ``SAMPLE_LOAD_OFFSET``. The exact addresses depend
on the toolchain and libc.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
