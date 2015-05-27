Linux Optional Installation Steps
#################################

The following installation procedures are optional. At the beginning of
each procedure you will find under what conditions the procedure has to
be followed.

Packages Required for Building Crosstool-NG
****************************************************************

Your host system must have the following packages for crosstool-NG:

.. code-block:: bash

   $ sudo apt-get install gperf gawk bison flex texinfo libtool \
     automake ncurses- devexpat libexpat1-dev libexpat1 python-dev

Install libtool-bin for Debian systems, type:

.. code-block:: bash

   $ sudo apt-get install libtool-bin

Requirements for building ARC Toolchain
****************************************
Install the needed packages for building ARC in Ubuntu, type:

.. code-block:: bash

   $ sudo apt-get install texinfo byacc flex libncurses5-dev \
     zlib1g-dev libexpat1-dev libx11-dev texlive build-essential

Install the needed packages for building ARC in Fedora, type:

.. code-block:: bash

   $ sudo yum groupinstall "Development Tools"

   $ sudo yum install texinfo-tex byacc flex ncurses-devel \
     zlib-devel expat-devel libX11-devel git

Optional Packages for building Crosstool-NG
********************************************

The following packages are optional since the first crosstool-NG
build downloads them if they are not installed.

Install the optional packages on your host system manually, type:

.. code-block:: bash

   $ sudo apt-get install gmp mpfr isl cloog mpc binutils

Installing the Crosstool-NG Toolchain (Optional)
************************************************

If you have installed the Tiny Mountain SDK provided by Yocto, you can
skip these steps.

 #. Use the curl command to install the crosstool next generation
    toolchain. Type:

.. code-block:: bash

   $ curl -O http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-1.20.0.tar.bz2


#. Extract the toolchain by typing the following commands in the
   console:

.. code-block:: bash

   $ tar xjf crosstool-ng-1.20.0.tar.bz2


#. Move to the crosstool folder:

.. code-block:: bash

   $ cd crosstool-ng-1.20.0


#. Specify the location where to install the crosstool-ng using
   configure. Note that the prefix path must be absolute.

.. code-block:: bash

   $ ./configure

   $ make


#. Install the toolchain by typing the following commands in the
   console:

.. code-block:: bash

   $ sudo make install

   $ sudo cp ct-ng.comp /etc/bash_completion.d/


#. Create the installation directory for the tool by typing the
   following commands in the console:

.. code-block:: bash

   $ sudo mkdir /opt/crosstool-ng

   $ sudo chown $USER:/opt/crosstool-ng

.. note::

   The preconfigured path can be changed via ct-ng menuconfig.
   Changing the path may result in the rest of the instructions not
   working.

Create the Needed Build Tools
=============================

#. Create the directories for the builds x86 and ARM. Type:

.. code-block:: bash

   $ mkdir ${HOME}/x86-build

   $ mkdir ${HOME}/arm-build

   $ mkdir ${HOME}/cross-src

#. Return to the parent directory. Type:

.. code-block:: bash

   $ cd $TIMO_BASE

#. Copy the toolchain configurations to the build directories by
   typing the following commands in the console:

.. code-block:: bash

   $ cp scripts/cross_compiler/x86.config ${HOME}/x86-build/.config

   $ cp scripts/cross_compiler/arm.config ${HOME}/arm-build/.config

#. Build and install the toolchains by typing the following commands
   in the console:

.. code-block:: bash

   $ cd ${HOME}/x86-build

   $ ct-ng build

   $ cd ${HOME}/arm-build

   $ ct-ng build

#. Add xtools to your shell, type:

.. code-block:: bash

   $ export VXMICRO_GCC_VARIANT=xtools

Alternatively you can add it to your :file:`~/.bashrc` file.

Adding in the ARC Toolchain
***************************

If you have installed the Tiny Mountain SDK provided by Yocto, you can
skip these steps.

Building the Toolchain Locally
==============================

Currently the documentation for building the toolchain locally is in the
process of being written, but the short version is found below (taken
liberally from the toolchain/README.mk)

#. Grab the compiler by running git clone:

.. code-block:: bash

   $ git clone https://github.com/foss-for-synopsys-dwc-arc-processors/toolchain

#. When this completes:

.. code-block:: bash

   $ cd toolchain ; ./arc-clone-all.sh

#. This will copy all the ARC toolchains into your directory.

.. code-block:: bash

   $ git checkout arc-releases ./build-all.sh --no-pdf --install-dir
   /opt/arc --jobs <number of cores>

#. Add the new binary to your path:

.. code-block:: bash

   $ export PATH=/opt/arc/bin:$PATH

Using a Pre-Built Binary
========================

Synopsys does provide a pre-built binary for use. Currently it supports
Ubuntu installs. When using this option, the Tiny Mountain project
cannot assist in debugging what might go wrong.

#. Download the pre-built binary, type:

.. code-block:: bash

   $ curl -o https://github.com/foss-for-synopsys-dwc-arc-processors/toolchain/releases/download/arc-2014.12/arc_gnu_2014.12_prebuilt_elf32_le_linux_install.tar.gz

#. Install the binary in /opt/arc, type:

.. code-block:: bash

   $ tar xf arc-2014.12/arc_gnu_2014.12_prebuilt_elf32_le_linux_install. tar.gz -C /opt/arc/x-tools

   $ cd /opt/arc/x-tools

.. warning::

   The commands above are not verified.

#. Ensure that the pre-built toolchain is found automatically by
   defs.gcc.x86-linux2.variant_xtools and gcc/arch/arc/defs.exec, type:

.. code-block:: bash

   $ ln -s arc_gnu_2014.12_prebuilt_elf32_le_linux_install/ arc-elf32

Running on Additional Hardware
******************************

Installing a Custom QEMU for ARM Platforms
==========================================

If you require to test ARM builds, a localized patch to the QEMU source
is needed. The patch corrects the issues with the locking interfaces
QEMU uses. If you are working only with the x86 builds of Tiny
Mountain, install QEMU from your systems default package manager.

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

   $ git am $TIMO_BASE/scripts/0001-armv7m-support-basepri-primask-interrupt-locking.patch

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

   make BSP=quark**

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

If you are having problems runing Tiny Mountain using the default GRUB
of the hardware, follow these steps to test Tiny Mountain on Galileo2
boards using a custom GRUB.

#. Install the requirements to build Tiny Mountain for GRUB on host
   machine.

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
