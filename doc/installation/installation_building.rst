Setup A Local Development Environment
**************************************

The |project| source code is maintained using GIT and is served using Gerrit.

Gerrit access requires some basic user setup.  The following process has
been defined as a simple walk-through to enable quick access to the
Gerrit services.


.. _Getting Access:

Getting Access
================

#. `Create`_ or `update`_ a 01.org_ account.

#. Submit your your 01.org_ account and corporate email address to
   |PM| `<mailto:hirally.santiago.rodriguez@intel.com>`_.

#. Once access is granted, `access Gerrit`_.

#. Log in using your 01.org account credentials.

.. _Create: https://01.org/user/register

.. _update: https://01.org/user/login

.. _access Gerrit: https://oic-review.01.org/gerrit/

.. _01.org: https://01.org/

Configuring SSH to Use Gerrit
=============================

Gerrit uses SSH to interact with your GIT client. A SSH private key
needs to be generated on the development machine with a matching public
key on the Gerrit server.

If you already have a SSH key-pair you would like to use, please skip
down to step.

Please follow the steps below to get started.

1. Create a key-pair in your Linux machine, type:

.. code-block:: bash

   $ ssh-keygen -t rsa -C "John Doe john.doe@example.com"

.. note:: This will ask you for a password to protect the private key as it
   generates a unique key. Please keep this password private, and DO
   NOT enter a blank password.


The generated key-pair is found at:
:file:`~/.ssh/id_rsa and ~/.ssh/id_rsa.pub`.

2. Add the the private key in the :file:`id_rsa` file in your key ring,
type:

.. code-block:: bash

   $ ssh-add ~/.ssh/id_rsa

3. Add your the public key :file:`id_rsa.pub` to the Gerrit account:

   a. Go to `access Gerrit`_.

   b. Click on your account name in the upper right corner.

   c. From the pop-up menu, select :guilabel:`Settings`.

   d. On the left hand menu select, click on
   :guilabel:`SSH Public Keys`.

   e. Click Add key and paste the contents of your public key
   :file:`~/.id/id_rsa.pub`.

.. note:: To obtain the contents of your public key on a Linux machine type:

   :command:`$ cat ~/.ssh/id_rsa.pub`

   The output is the contents of :file:`~/.id/id_rsa.pub`. Paste it into the Add SSH key window in Gerrit.

.. warning:: Potential Security Risk
   Do not copy your private key :file:`~/.ssh/id_rsa` Use only the public
   :file:`~/.id/id_rsa.pub`.



Checking the Source Code Out
=============================

#. Ensure that SSH has been set up porperly. See
   `Configuring SSH to Use Gerrit`_ for details.

#. Clone the repository, type:

   :command:`$ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab`

#. You have checked out a local copy of the source code. Develop
   freely, issuing as many commits and rebases as needed.


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
directory, the results can be found at :file:`$SAMPLE_PROJECT/outdir/nanokernel.elf`.

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
