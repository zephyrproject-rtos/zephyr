.. _acrn:

ACRN UOS (User Operating System)
#################################

Overview
********

This board configuration defines an ACRN User OS execution environment for
running Zephyr RTOS applications.

ACRN is a flexible, lightweight reference hypervisor, built with real-time
and safety-criticality in mind, optimized to streamline embedded development
through an open source platform. Check out the `Introduction to Project ACRN
<https://projectacrn.github.io/latest/introduction/>`_ for more information.

This baseline configuration can be used as a starting point for creating
demonstration ACRN UOS configurations. It currently supports the following
devices:

* I/O APIC
* local APIC timer
* NS16550 UARTs

.. note::
   This ACRN board configuration is for illustrative purposes only.
   Because of its reliance on virtualized hardware provided by ACRN,
   it is not suitable for production real-time applications. Real-time
   response under ACRN requires direct access to the underlying
   hardware, so production applications should be derived from the
   board configurations that describe that underlying hardware.

   For example, if you wish to run an application under ACRN on an Up
   Squared, start with the Up Squared board configuration, not this one.

Serial Ports
------------

The serial ports are assumed present at traditional ``COM1:`` and ``COM2:``
I/O-space addresses (based at ``0x3f8`` and ``0x2f8``, respectively). Only
polled operation is supported in this baseline configuration, as IRQ
assignments under ACRN are configurable (and frequently non-standard).
Interrupt-driven and MMIO operation are also possible.

Building and Running
********************

This details the process for building the :ref:`hello_world` sample and
running it as an ACRN User OS.

On the Zephyr Build System
--------------------------

#. The build process for the ACRN UOS target is similar to other boards. We
   will build the :ref:`hello_world` sample for ACRN with:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: acrn
      :goals: build

   This will build the application ELF binary in
   ``samples/hello_world/build/zephyr/zephyr.elf``.

#. Build GRUB2 boot loader image

   We can build the GRUB2 bootloader for Zephyr using
   ``boards/x86/common/scripts/build_grub.sh``:

   .. code-block:: none

      $ ./boards/x86/common/scripts/build_grub.sh x86_64

   The EFI executable will be found at
   ``boards/x86/common/scripts/grub/bin/grub_x86_64.efi``.

#. Preparing the boot device

   .. code-block:: none

      $ dd if=/dev/zero of=zephyr.img bs=1M count=35
      $ mkfs.vfat -F 32 zephyr.img
      $ LOOP_DEV=`sudo losetup -f -P --show zephyr.img`
      $ sudo mount $LOOP_DEV /mnt
      $ sudo mkdir -p /mnt/efi/boot
      $ sudo cp boards/x86/common/scripts/grub/bin/grub_x86_64.efi /mnt/efi/boot/bootx64.efi
      $ sudo mkdir -p /mnt/kernel
      $ sudo cp samples/hello_world/build/zephyr/zephyr.elf /mnt/kernel

   Create ``/mnt/efi/boot/grub.cfg`` containing the following:

   .. code-block:: console

     set default=0
     set timeout=10

     menuentry "Zephyr Kernel" {
         multiboot /kernel/zephyr.elf
     }

   And then unmount the image file:

   .. code-block:: console

      $ sudo umount /mnt

   You now have a virtual disk image with a bootable Zephyr in ``zephyr.img``.
   If the Zephyr build system is not the ACRN SOS, then you will need to
   transfer this image to the ACRN SOS (via, e.g., a USB stick or network).

On the ACRN SOS
---------------

#. If you are not already using the ACRN SOS, follow `Getting started guide
   for Intel NUC
   <https://projectacrn.github.io/latest/getting-started/apl-nuc.html>`_ to
   install and boot "The ACRN Service OS".

#. Boot Zephyr as User OS

   On the ACRN SOS, prepare a directory and populate it with Zephyr files.

   .. code-block:: none

      $ mkdir zephyr
      $ cd zephyr
      $ cp /usr/share/acrn/samples/nuc/launch_zephyr.sh .
      $ cp /usr/share/acrn/bios/OVMF.fd .

   You will also need to copy the ``zephyr.img`` created in the first
   section into this directory. Then run ``launch_zephyr.sh`` script
   to launch the Zephyr as a UOS.

   .. code-block:: none

      $ sudo ./launch_zephyr.sh

   Then Zephyr will boot up automatically. You will see the banner:

   .. code-block:: console

      Hello World! acrn

   Which indicates that Zephyr is running successfully under ACRN!
