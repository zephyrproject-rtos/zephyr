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

In the ACRN Shared scenario User VM will be launched from the Service VM.
The User VM should be inside the image specified in the launch script.
The Location of the image is defined during the configuration phase.

Script :file:`launch_user_vm_id1.sh` has the following parameter:

.. code-block:: console

   `add_virtual_device   3 virtio-blk /path_to/zephyr.img`

Create FAT disk image
=====================

Please install :program:`mtools` for accessing FAT disk images without
mounting.

Create and format disk image with the following commands:

.. code-block:: console

   # Create 10M file
   $ dd if=/dev/zero of=zephyr.img bs=1M count=10

   # Format image
   $ mformat -i zephyr.img ::

Populate Zephyr disk image
==========================

Copy built :file:`zephyr.efi` to the disk image with:

.. code-block:: console

   $ mcopy -i zephyr.img build/zephyr/zephyr.efi ::

To avoid manually running :file:`zephyr.efi`, create :file:`startup.nsh`,
and put it inside the Zephyr disk image. UEFI shell looks for this script
file in any of the FAT file systems available and executes it.

Content of the :file:`startup.nsh`:

.. code-block:: bash

   @echo "Auto starting zephyr.efi"
   zephyr.efi

Copy :file:`startup.nsh` to the Zephyr disk image with:

.. code-block:: console

   $ mcopy -i zephyr.img startup.nsh ::

Verify the Zephyr image with the following command:

.. code-block:: console

   $ mdir -i zephyr.img
    Volume in drive : has no label
    Volume Serial Number is 5FF6-E430
   Directory for ::/

   zephyr   efi    107841 2023-11-24  12:09
   startup  nsh        44 2023-11-24  12:11
          2 files             107 885 bytes
                           10 342 400 bytes free

Launch Zephyr User VM
*********************

Launching a User VM is described in the `ACRN Getting Started Guide`_
:menuselection:`7. Launch the User VM --> 3. Launch the User VM`.

Login to Service VM and start Zephyr User VM with:

.. code-block:: console

   $ cd acrn-work
   $ sudo ./launch_user_vm_id1.sh

   UEFI Interactive Shell v2.2
   EDK II
   UEFI v2.70 (ARCN, 0x00010000)
   Mapping table
         FS0: Alias(s):F0:;BLK0:
             PciRoot(0x0)/Pci(0x3,0x0)
   Press ESC in 1 seconds to skip startup.nsh or any other key to continue.
   Auto starting zephyr.efi
   Shell> zephyr.efi
   *** Zephyr EFI Loader ***
   RSDP found at 0x7f9fa014
   Zeroing 99424 bytes of memory at 0x105000
   Copying 32768 data bytes to 0x1000 from image offset
   Copying 20480 data bytes to 0x100000 from image offset 32768
   Copying 48032 data bytes to 0x11d460 from image offset 53248
   Jumping to Entry Point: 0x1137 (48 31 c0 48 31 d2 48)
   *** Booting Zephyr OS build zephyr-v3.5.0-2061-ga44f9fe02961 ***
   Hello World! acrn

References
**********

.. target-notes::

.. _EHL: https://www.intel.com/content/www/us/en/products/docs/processors/embedded/enhanced-for-iot-platform-brief.html
.. _ACRN Project: https://projectacrn.org/
.. _ACRN Documentation: https://projectacrn.github.io/3.2/
.. _ACRN Getting Started Guide: https://projectacrn.github.io/3.2/getting-started/getting-started.html
