Linux Optional Installation Steps
#################################

The following installation procedures are optional. At the beginning of
each procedure you will find under what conditions the procedure has to
be followed.

Running on Additional Hardware
******************************

Installing a Custom QEMU for ARM Platforms
==========================================

If you require to test ARM builds, a localized patch to the QEMU source
is needed. The patch corrects the issues with the locking interfaces
QEMU uses. If you are working only with the x86 builds of the Zephyr kernel,
install QEMU from your systems default package manager.

Follow these steps to enable a customized build of QEMU:

#. Clone the QEMU repository, type:

.. code-block:: bash
   $ git clone git://git.qemu-project.org/qemu.git

#. Checkout the v2.1 stable branch, type:

.. code-block:: bash

   $ cd qemu

   $ git checkout stable-2.1

#. Apply our internal patch, type:

.. code-block:: bash

   $ git am $ZEPHYR_BASE/scripts/0001-armv7m-support-basepri-primask-interrupt-locking.patch

#. Update the submodules as needed, type:

.. code-block:: bash

   $ git submodule update --init pixman

   $ git submodule update --init dtc

#. Build QEMU v2.1, type:

.. code-block:: bash

   $ ./configure && make

* You can also build QEMU to a private directory, type:

.. code-block:: bash

   $ ./configure --prefix=$MY_PREFERED_INSTALL_LOCATION && make

* Install QEMU, type:

.. code-block:: bash

   $ sudo make install

Running a Project on Galileo Gen2
=================================

Running a Project
-----------------

Follow the directions in :ref:`RequiredSteps`

#. Set the BSP to Quark by changing the :command:`make` command to:

.. code-block:: bash

   make BSP=quark ARCH=x86**

#. Use one of these cables for serial output:

    `<http://www.ftdichip.com/Products/Cables/USBTTLSerial.htm>`__

#. Format a microSD as FAT

#. Create the following directories

:file:`efi`

:file:`efi/boot`

:file:`kernel`

#. Copy the :file:`{microkernel\|nanokernel}.elf` file to the
   :file:`$SDCARD/kernel` folder

#. Copy your built version of GRUB to :file:`$SDCARD/efi/boot`

#. Create :file:`$SDCARD/efi/boot/grub.cfg` containing the following:

.. code-block:: bash

   set default=0 **

   set timeout=10 **

   menuentry "This is my boot message" {**

      multiboot /kernel/{microkernel\|nanokernel}.elf**

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

#. If you’ve added a custom GRUB, please run it from here.


Building a Custom GRUB
----------------------

If you are having problems running an application using the default GRUB
of the hardware, follow these steps to test on Galileo2
boards using a custom GRUB.

#. Install the requirements to build GRUB on your host machine.

In Ubuntu, type:

.. code-block:: bash

    $ sudo apt-get install gnu-efi-i386 bison libopts25
    libselinux1-dev autogen m4 autoconf help2man libopts25-dev flex
    libfont-freetype-perl automake autotools-dev libfreetype6-dev
    texinfo

In Fedora, type:

.. code-block:: bash

   $ sudo yum install gnu-efi-i386 bison libopts25
   libselinux1-dev autogen m4 autoconf help2man libopts25-dev flex
   libfont-freetype-perl automake autotools-dev libfreetype6-dev texinfo

#. Clone the GRUB repository, type:

.. code-block:: bash

   $ cd ~

   $ git clone http://git.savannah.gnu.org/r/grub.git/**

#. Build the GRUB code, type:

.. code-block:: bash

    $ cd grub

    $ ./autogen.sh CFLAGS=”-march=i586 -m32” ./configure --with-platform=efi --target=i386 --program-prefix=""

    $ make

    $ cd grub-core

    $ ../grub-mkimage -O i386-efi -d . -o grub.efi -p "" part_gpt part_msdos
     fat ext2 normal chain boot configfile linux multiboot help serial terminal
     elf efi_gop efi_uga terminfo

#. Look for the binary at :file:`~/grub/grub-core/grub.efi`.

Troubleshoot
------------

If your custom built GRUB version returns the error:

.. error::

   error reported: Invalid parameter

Follow these steps:

#. Use the built-in version of GRUB with the following file:

.. code-block:: bash

   $ grub.efi

#. Locate your SDcard in the system

.. code-block:: bash

    $ ls

#. You see two entries similar to:

:file:`(hd0)`, :file:`(hd0,msdos1)`

#. Verify the contents, type:

.. code-block:: bash

   $ ls (hd0,msdos1)/efi/

The command shows the contents of your SDcard.

#. If the command did not work, find the correct mount, type:

.. code-block:: bash

   $ configfile (hd0,msdos1)/efi/boot/grub.cfg

The command uses the Galileo’s built-in GRUB to parse your config file
and list the options you’ve set.
