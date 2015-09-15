.. _running_apps:

Running Applications
####################

Running Applications in QEMU
****************************

Using QEMU from a different path
================================

If the QEMU binary path is different to the default path, set the
variable :envvar:`QEMU_BIN_PATH` with the new path, type:

.. code-block:: bash

   $ export QEMU_BIN_PATH=/usr/local/bin

Another option is to add it to the make command, for example:

.. code-block:: bash

   $ make QEMU_BIN_PATH=/usr/local/bin qemu

Running a Microkernel Application
=================================

Run a microkernel application using the default platform configuration, type:

.. code-block:: bash

   $ make pristine && make qemu

Run an application using the x86 basic_atom platform configuration, type:

.. code-block:: bash

   $ make pristine && make PLATFORM_CONFIG=basic_atom ARCH=x86 qemu

Run an application using the ARM basic_cortex_m3 platform configuration, type:

.. code-block:: bash

   $ make pristine && make PLATFORM_CONFIG=basic_cortex_m3 ARCH=arm qemu

Running a Nanokernel Application
================================

Run a nanokernel application using the default platform configuration use the
following commands:

.. code-block:: bash

   $ make pristine && make qemu

Run an application using the x86 basic_atom platform configuration, use the
following commands:

.. code-block:: bash

   $ make pristine && make PLATFORM_CONFIG=basic_atom ARCH=x86 qemu

Run an application using the ARM basic_cortex_m3 platform configuration use the
following commands:

.. code-block:: bash

   $ make pristine && make PLATFORM_CONFIG=basic_cortex_m3 ARCH=arm qemu


Running an Application on Galileo Gen2
**************************************

#. Set the platform configuration to Galileo by changing the :command:`make` command to:

.. code-block:: bash

   make PLATFORM_CONFIG=galileo

#. Use one of these cables for serial output:

    `<http://www.ftdichip.com/Products/Cables/USBTTLSerial.htm>`__

#. Format a microSD as FAT

#. Create the following directories

   :file:`efi`

   :file:`efi/boot`

   :file:`kernel`

#. Assuming you have built a micro kernel, copy the kernel file :file:`microkernel.elf` to the :file:`$SDCARD/kernel` folder

#. Copy your built version of GRUB to :file:`$SDCARD/efi/boot/bootia32.efi`

#. Create :file:`$SDCARD/efi/boot/grub.cfg` containing the following:

.. code-block:: bash

   set default=0
   set timeout=10

   menuentry "Zephyr microkernel" {
      multiboot /kernel/microkernel.elf
    }

#. Insert the SDcard in the Galileo board.

#. Connect the board to the host system using the serial cable.

#. Configure your host system to watch for serial data.

    * On Linux, minicom is a popular method for reading serial
      data.

    * On Windows, PuTTY has an option to set up configuration for
      serial data.

#. Power on the Galileo board.

#. When asked press :kbd:`F7`.

#. By default Galileo has a pre-installed GRUB and Linux distro.
   Press :kbd:`c` to cancel the current boot.

#. Quit the currently running GRUB.

#. On the menu select the :guilabel:`UEFI Internal Shell` option.

#. If you've added a custom GRUB, please run it from here.

Building a Custom GRUB
**********************

If you are having problems running an application using the default GRUB
of the hardware, follow these steps to test on Galileo2 boards using a custom
GRUB.

1. Install the requirements to build GRUB on your host machine.

In Ubuntu, type:

.. code-block:: bash

    $ sudo apt-get install gnu-efi:i386 bison libopts25 \
    libselinux1-dev autogen m4 autoconf help2man libopts25-dev flex \
    libfont-freetype-perl automake autotools-dev libfreetype6-dev \
    texinfo \

In Fedora, type:

.. code-block:: bash

   $ sudo yum install gnu-efi-i386 bison libopts25 \
   libselinux1-dev autogen m4 autoconf help2man libopts25-dev flex \
   libfont-freetype-perl automake autotools-dev libfreetype6-dev texinfo

2. Clone and build the GRUB repository using the script in Zephyr tree, type:

.. code-block:: bash

   $ cd $ZEPHYR_BASE
   $ ./scripts/build_grub.sh

4. Look for the binary at :file:`$ZEPHYR_BASE/scripts/grub/bin/grub.efi`.

Troubleshoot
============

If your custom built GRUB version returns the error:

.. error::

   error reported: Invalid parameter

Follow these steps:

1. Use the built-in version of GRUB with the following file:

.. code-block:: bash

   $ grub.efi

2. Locate your SDcard in the system

.. code-block:: bash

    $ ls

3. You see two entries similar to:

:file:`(hd0)`, :file:`(hd0,msdos1)`

4. Verify the contents, type:

.. code-block:: bash

   $ ls (hd0,msdos1)/efi/

The command shows the contents of your SDcard.

5. If the command did not work, find the correct mount, type:

.. code-block:: bash

   $ configfile (hd0,msdos1)/efi/boot/grub.cfg

The command uses the Galileo’s built-in GRUB to parse your config file
and list the options you’ve set.
