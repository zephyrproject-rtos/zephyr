.. _RequiredSteps:

Required Installation Steps
###########################

Install all required packages for a Tiny Mountain development
environment.

Use the following procedures to build a Tiny Mountain example starting
with a clean system.

#. `Installing the Yocto Software Development Kit`_

#. `Installing the Tiny Mountain Software`_

#. `Building a Tiny Mountain Example`_

#. `Running Your Projects on QEMU`_

Installing the Yocto Software Development Kit
*********************************************

The Tiny Mountain :abbr:`Software Development Kit (SDK)` provided by
Yocto contains all necessary tools and cross compilers needed to build
Tiny Mountain on all supported architectures. In addition it includes
host tools such as Qemu built with the needed patch to support Tiny
Mountain and a host compiler for building host tools if necessary. If
you use this SDK, there is no need to build any cross compilers or
emulation environments. The SDK supports the following architectures:

* :abbr:`IA-32 (Intel Architecture 32 bits)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

Follow these steps to install the SDK on your host system.

#. Download the Yocto self-extractable binary from:

   http://yct-rtos02.ostc.intel.com/tm-toolchains-i686-setup.run

#. Run the binary, type:

    .. code-block:: bash

       $ chmod +x tm-toolchains-i686-setup.run

       $ sudo ./tm-toolchains-i686-setup.run


#. Follow the installation instructions on the screen. The
   toolchain's default installation location is :file:`/opt/poky-tm`.

    .. code-block:: bash

       Verifying archive integrity... All good.

       Uncompressing SDK for TM 100%

       Enter target directory for SDK (default: /opt/poky-tm/1.8):

#. Enter a new location or hit :kbd:`Return` to accept default.

    .. code-block:: bash

       Installing SDK to /opt/poky-tm/1.8

       Creating directory /opt/poky-tm/1.8

       Success

       [*] Installing x86 tools...

       [*] Installing arm tools...

       [*] Installing arc tools...

       [*] Installing additional host tools...

       Success installing SDK. SDK is ready to be used.

#. To use the Yocto SDK, export the following environment variables,
   type:

    .. code-block:: bash

       $ export ZEPHYR_GCC_VARIANT=yocto

       $ export YOCTO_SDK_INSTALL_DIR=/opt/poky-tm/1.8

#. When you build Tiny Mountain now, the Yocto SDK will be used.


Installing the General Development Requirements
***********************************************

Install the required software for a Tiny Mountain environment. See:
:ref:`Requirements` to learn what packages are needed.

If you are using Ubuntu, use:

.. code-block:: bash

   $ sudo apt-get

If you are using Fedora, use:

.. code-block:: bash

   $ sudo yum

.. note:: For troubleshooting information, refer to the appropriate component's documentation.

Installing the Tiny Mountain Software
*************************************

The current source is housed on Intel’s 01.org service. The process for
getting access is not detailed in this document, but can be found in
the document. Section 3 details the steps for checking out the code,
but can be summarized with the following steps:

#. Ensure that SSH has been set up porperly. See :ref:`GerritSSH` for
   details.

#. Clone the repository, type:

    .. code-block:: bash

       $ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab`

#. Change to the Tiny Mountain directory, type:

    .. code-block:: bash

       $ cd forto-collab

#. Source the build environment to set the Tiny Mountain environment
   variables, type:

    .. code-block:: bash

       $ source timo-env.bash

Building a Tiny Mountain Example
================================

To build a Tiny Mountain example follow these steps:

#. Go to the root directory of your foss-rtos checkout

#. Set the paths properly in the :file:`$TIMO_BASE` directory,
   type:

    .. code-block:: bash

       $ source timo-env.bash

#. Build Tiny Mountain with the example project, type:

    .. code-block:: bash

       $ cd $TIMO_BASE/samples/microkernel/apps/hello_world

       $ make pristine && make



.. note::

   You can override the default BSP with the one you want by adding
   :makevar:`BSP=`. The complete options available for the BSP flag
   can be found at :file:`$TIMO_BASE/arch` under the respective
   architecture, for example :file:`$TIMO_BASE/arch/x86/generic_pc`.
   You need to override the ARCH flag with the architecture that
   corresponds to your BSP by adding :makevar:`ARCH=` and the options
   you need to the make command, for example:

   :command:`make BSP=generic_pc ARCH=x86`

   The complete options available for the ARCH flag can be found at
   :file:`$TIMO_BASE`, for example  :file:`$TIMO_BASE/arch/x86`.

The sample projects for the microkernel are found
at :file:`$TIMO_BASE/samples/microkernel/apps` and the results are at
:file:`$SAMPLE_PROJECT/outdir/microkernel.{ bin | elf }`.

For sample projects in the :file:`$TIMO_BASE/samples/nanokernel/apps`
directory, the results can be found at
:file:`$SAMPLE_PROJECT/outdir/nanokernel.{ bin | elf }`.

Running Your Projects on QEMU
*****************************

Using QEMU from a different path
================================

If the QEMU binary path is different to the default path, set the
variable :envvar:`QEMU_BIN_PATH` with the new path, type:

.. code-block:: bash

   $ export QEMU_BIN_PATH=/usr/local/bin

Another option is to add it to the make command, for example:

.. code-block:: bash

   $ make QEMU_BIN_PATH=/usr/local/bin qemu

Running a Microkernel Project
-----------------------------

Run a microkernel project using the default BSP (generic_pc), type:

.. code-block:: bash

   $ make pristine && make qemu

Run a project using the quark BSP, type:

.. code-block:: bash

   $ make pristine && make BSP=quark ARCH=x86 qemu

Run a project using the ARM BSP, type:

.. code-block:: bash

   $ make pristine && make BSP=ti_lm3s6965 ARCH=arm qemu

Running a Nanokernel Project
----------------------------

Run a nanokernel project using the default BSP (generic_pc) use the
following commands:

.. code-block:: bash

   $ make pristine && make qemu

Run a project using the quark BSP use the following commands:

.. code-block:: bash

   $ make pristine && make BSP=quark ARCH=x86 qemu

Run a project using the ARM BSP use the following commands:

.. code-block:: bash

   $ make pristine && make BSP=ti_lm3s6965 ARCH=arm qemu
