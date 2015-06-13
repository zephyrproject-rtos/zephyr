Setup A Local Development Environment
**************************************

The source is housed on Intel Corporation’s 01.org service. The process for
getting access is not detailed in this section. See the
Accessing_Gerrit_ for more details.  A quick summary follows:

#. Ensure that SSH has been set up properly.

#. Clone the repository. Make sure to replace 01ORGUSERNAME with your
   actual 01.org user name and type:

    .. code-block:: bash

       $ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab`

#. Change to the main project directory, type:

    .. code-block:: bash

       $ cd forto-collab

#. Source the project environment file to setup project variables, type:

    .. code-block:: bash

       $ source zephyr-env.bash

Building a Sample Application
==============================

To build an example application follow these steps:

#. Go to the root directory of the |project|.

#. Set the paths properly in the :file:`$ZEPHYR_BASE` directory,
   type:

    .. code-block:: bash

       $ source zephyr-env.bash

#. Build the example project, type:

    .. code-block:: bash

       $ cd $ZEPHYR_BASE/samples/microkernel/apps/hello_world

       $ make pristine && make

.. note::

   You can override the default BSP with the one you want by adding
   :makevar:`BSP=`. The complete options available for the BSP flag
   can be found at :file:`$ZEPHYR_BASE/arch` under the respective
   architecture, for example :file:`$ZEPHYR_BASE/arch/x86/generic_pc`.
   You need to override the ARCH flag with the architecture that
   corresponds to your BSP by adding :makevar:`ARCH=` and the options
   you need to the make command, for example:

   :command:`make BSP=generic_pc ARCH=x86`

   The complete options available for the ARCH flag can be found at
   :file:`$ZEPHYR_BASE`, for example  :file:`$ZEPHYR_BASE/arch/x86`.

The sample projects for the microkernel are found
at :file:`$ZEPHYR_BASE/samples/microkernel/apps`. After building the application successfully, the results can be found in the :file:`outdir`directory in the application root directory.
:file:`$SAMPLE_PROJECT/outdir/microkernel.elf`.

For sample projects in the :file:`$ZEPHYR_BASE/samples/nanokernel/apps`
directory, the results can be found at
:file:`$SAMPLE_PROJECT/outdir/nanokernel.elf`.

Testing Applications
**************************************

Running Applications in QEMU
==============================

Using QEMU from a different path
---------------------------------

If the QEMU binary path is different to the default path, set the
variable :envvar:`QEMU_BIN_PATH` with the new path, type:

.. code-block:: bash

   $ export QEMU_BIN_PATH=/usr/local/bin

Another option is to add it to the make command, for example:

.. code-block:: bash

   $ make QEMU_BIN_PATH=/usr/local/bin qemu

Running a Microkernel Application
----------------------------------

Run a microkernel application using the default BSP (generic_pc), type:

.. code-block:: bash

   $ make pristine && make qemu

Run an application using the quark BSP, type:

.. code-block:: bash

   $ make pristine && make BSP=quark ARCH=x86 qemu

Run an application using the ARM BSP, type:

.. code-block:: bash

   $ make pristine && make BSP=ti_lm3s6965 ARCH=arm qemu

Running a Nanokernel Application
--------------------------------

Run a nanokernel application using the default BSP (generic_pc) use the
following commands:

.. code-block:: bash

   $ make pristine && make qemu

Run an application using the quark BSP use the following commands:

.. code-block:: bash

   $ make pristine && make BSP=quark ARCH=x86 qemu

Run an application using the ARM BSP use the following commands:

.. code-block:: bash

   $ make pristine && make BSP=ti_lm3s6965 ARCH=arm qemu


Running an Application on Galileo Gen2
=======================================

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

#. Assuming you have built a micro kernel, copy the kernel file :file:`microkernel.elf` to the :file:`$SDCARD/kernel` folder

#. Copy your built version of GRUB to :file:`$SDCARD/efi/boot`

#. Create :file:`$SDCARD/efi/boot/grub.cfg` containing the following:

.. code-block:: bash

   set default=0 **

   set timeout=10 **

   menuentry "This is my boot message" {**

      multiboot /kernel/microkernel.elf**

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
