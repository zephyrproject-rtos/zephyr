ACRN User VM board
##################

Zephyr is capable of running as a guest under the x86 ACRN Hypervisor.
For more information please refer to: `ACRN Project`_.

There are several ACRN scenario types: Shared, Partitioned, and Hybrid.
Those configurations are described in the `ACRN Documentation`_.

Here we will use the Shared scenario type since this is the only type that
has up-to-date documentation available. In the Shared scenario type, ACRN
Hypervisor launches the Service VM, which is in our example based on
Ubuntu 22.04. The Service VM then launches the Post-launched Zephyr User VM.

In this tutorial, we will show you how to build a minimal running
Zephyr application and ACRN Hypervisor using the `ACRN Getting Started Guide`_
with the help of an advanced ACRN Configurator.

Build Zephyr Application
************************

First, build the Zephyr application you want to run in ACRN as you
normally would, selecting an appropriate board:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: acrn
   :goals: build

In this tutorial, we will use the Intel Elkhart Lake Reference Board
(`EHL`_ CRB).

Configure and build ACRN
************************

The easiest way of setting up ACRN is to follow the
`ACRN Getting Started Guide`_. The Guide shows how to set up ACRN in a Shared
scenario with Ubuntu 22.04 as a Service VM and same Ubuntu 22.04 as a
Post-launched User VM. The idea is to simply replace the Post-launched User VM
with Zephyr.

Follow all steps in the `ACRN Getting Started Guide`_ and make the following
changes in the following steps:

In Step :menuselection:`3. Generate a Scenario Configuration File and Launch Script`:

* When in :menuselection:`9. Configure the post-launched VM as follows`:

  - In step :menuselection:`e`: Do not add a :guilabel:`Virtio console device`.
    The virtio console driver is not implemented in Zephyr yet.

  - In step :menuselection:`f`: Change the path to the Zephyr EFI boot media you
    will build later.

  - Select :guilabel:`Emulate COM1 as stdio I/O`. This will emulate the first serial
    port for Zephyr VM, which is the default console. For the Service VM when
    Zephyr VM is launched, `stdio` would become an interface with this serial port.

In Step :menuselection:`7. Launch the User VM --> Step 1` instead of downloading
Ubuntu 22.04 ISO image use the Zephyr EFI Boot media image.

Assemble EFI Boot Media
***********************

ACRN will boot on the hardware via the GNU GRUB bootloader, which is
itself launched from the EFI firmware.  These need to be configured
correctly.

Locate GRUB
===========

First, you will need a GRUB EFI binary that corresponds to your
hardware.  In many cases, a simple upstream build from source or a
copy from a friendly Linux distribution will work.  In some cases it
will not, however, and GRUB will need to be specially patched for
specific hardware.  Contact your hardware support team (pause for
laughter) for clear instructions for how to build a working GRUB.  In
practice you may just need to ask around and copy a binary from the
last test that worked for someone.

Create EFI Boot Filesystem
==========================

Now attach your boot media (e.g. a USB stick on /dev/sdb, your
hardware may differ!) to a Linux system and create an EFI boot
partition (type code 0xEF) large enough to store your boot artifacts.
This command feeds the relevant commands to fdisk directly, but you
can type them yourself if you like:

    .. code-block:: console

        # for i in n p 1 "" "" t ef w; do echo $i; done | fdisk /dev/sdb
        ...
        <lots of fdisk output>

Now create a FAT filesystem in the new partition and mount it:

    .. code-block:: console

        # mkfs.vfat -n ACRN_ZEPHYR /dev/sdb1
        # mkdir -p /mnt/acrn
        # mount /dev/sdb1 /mnt/acrn

Copy Images and Configure GRUB
==============================

ACRN does not have access to a runtime filesystem of its own.  It
receives its guest VMs (i.e. zephyr.bin) as GRUB "multiboot" modules.
This means that we must rely on GRUB's filesystem driver.  The three
files (GRUB, ACRN and Zephyr) all need to be copied into the
"/efi/boot" directory of the boot media.  Note that GRUB must be named
"bootx64.efi" for the firmware to recognize it as the bootloader:

    .. code-block:: console

        # mkdir -p /mnt/acrn/efi/boot
        # cp $PATH_TO_GRUB_BINARY /mnt/acrn/efi/boot/bootx64.efi
        # cp $ZEPHYR_BASE/build/zephyr/zephyr.bin /mnt/acrn/efi/boot/
        # cp $PATH_TO_ACRN/build/hypervisor/acrn.bin /mnt/acrn/efi/boot/

At boot, GRUB will load a "efi/boot/grub.cfg" file for its runtime
configuration instructions (a feature, ironically, that both ACRN and
Zephyr lack!).  This needs to load acrn.bin as the boot target and
pass it the zephyr.bin file as its first module (because Zephyr was
configured as ``<vm id="0">`` above).  This minimal configuration will
work fine for all but the weirdest hardware (i.e. "hd0" is virtually
always the boot filesystem from which grub loaded), no need to fiddle
with GRUB plugins or menus or timeouts:

    .. code-block:: console

        # cat > /mnt/acrn/efi/boot/grub.cfg<<EOF
        set root='hd0,msdos1'
        multiboot2 /efi/boot/acrn.bin
        module2 /efi/boot/zephyr.bin Zephyr_RawImage
        boot
        EOF

Now the filesystem should be complete

    .. code-block:: console

        # umount /dev/sdb1
        # sync

Launch Zephyr User VM
*********************

Launching a User VM is described in the `ACRN Getting Started Guide`_
:menuselection:`7. Launch the User VM --> 3. Launch the User VM`.

Login to Service VM and start Zephyr User VM with:

.. code-block:: console

   $ cd acrn-work
   $ sudo ./launch_user_vm_id1.sh

   WARNING: no console will be available to OS
   error: no suitable video mode found.
   *** Booting Zephyr OS build zephyr-v3.5.0 ***
   Hello World! acrn

References
**********

.. target-notes::

.. _EHL: https://www.intel.com/content/www/us/en/products/docs/processors/embedded/enhanced-for-iot-platform-brief.html
.. _ACRN Project: https://projectacrn.org/
.. _ACRN Documentation: https://projectacrn.github.io/3.2/
.. _ACRN Getting Started Guide: https://projectacrn.github.io/3.2/getting-started/getting-started.html
