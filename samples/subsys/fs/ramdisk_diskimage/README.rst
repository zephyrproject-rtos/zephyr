.. zephyr:code-sample:: ramdisk_diskimage
   :name: RAM disk from Disk Image

   Mounts an embedded disk image as a RAM disk.

Overview
********

This sample demonstrates how an existing disk image, embedded into the
application as binary data, can be used as the backing store of a RAM disk.

Requirements
************

To create the FAT-formatted test disk image in a portable manner, this sample
uses Python **pyfatfs** module, so this must be installed before building this
sample.

.. code-block:: console

   pip install pyfatfs

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/ramdisk_diskimage
   :board: qemu_x86
   :gen-args: -DDTC_OVERLAY_FILE=ramdisk.overlay
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

The disk image used by the sample is created on the fly at build time.
However, in practical scenarios, such disk images would typically be created
by some external tool, so would already exist before Zephyr build gets
initiated.

The size of the test disk image is configurable (default 1MiB). It can be
changed by setting :kconfig:option:`RAMDISK_DISKIMAGE_SAMPLE_DISK_SIZE` to
the desired value (in KiB), in the project configuration file.

The test RAM disk is formatted to FAT filesystem. The minimum supported size, limited
by Zephyr FAT filesystem support, is 64KiB. Larger disk sizes are limited by the
available RAM on the board being used and (on MMU based platforms) also by size of
the kernel address space (determined by :kconfig:option:`CONFIG_KERNEL_VM_BASE`
and :kconfig:option:`CONFIG_KERNEL_VM_SIZE`). So on some boards, you may have
to update :kconfig:option:`CONFIG_KERNEL_VM_SIZE`, to a suitably higher value,
even if the board has plenty of available RAM. For example, **qemu_cortex_a9**
has plenty of RAM but to create disks larger than about 8MiB, you'd need to increase
the kernel address space size accordingly.

Sample Output
=============

.. code-block:: console

    Test disk image mounted as a RAM disk at /RAM:
    Disk stats: bsize 512, frsize 512, blocks 2017, bfree 2016

    Reading existing file 'test.txt':
    This is an existing test file.

    Disk space in use: 512 bytes

    Creating a new file 'test2.txt'.
    Reading the new file 'test2.txt':
    This is a newly created test file.

    Disk space in use: 1024 bytes

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
