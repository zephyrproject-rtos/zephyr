.. _kbuild_project:

Developing an Application and the Build System
**********************************************

The build system is an application centric system. The application drives the
configuration and build process of the kernel binary.

Application development can be organized in three independent directories:
the kernel base directory, the project directory, and the project source code
directory.

The Zephyr Kernel's base directory hosts the kernel source code, the
configuration options, and the kernel build definitions.

The application directory is the directory that puts together the kernel with
the developer's application. It hosts the definitions of the developers'
applications. For example, application-specific configuration options and the
application's
source code.

The Application Project
=======================

A common application project is composed of the following files:

* **Makefile**: Defines the application's build process and integrates the
  developer's application with the kernel's build system.

* **Configuration file**: Allows the developer to override the board's
  default configuration.

* **The :file:`src/` directory**: By default, this directory hosts the
  application's source code. This location can be overridden and defined in a
  different directory.

   * **Makefile**: Adds the developer's source code into the build system's
     recursion model.

The application's source code can be organized in subdirectories.
Each directory must follow the Kbuild Makefile conventions; see
:ref:`kbuild_makefiles`.

Application Definition
======================

The developer defines the relationship of application to build system through
the Makefiles. The application is integrated into the build system by
including the Makefile.inc file provided.

.. code-block:: make

   include $(ZEPHYR_BASE)/Makefile.inc

The following predefined variables configure the development project:

* :makevar:`ARCH`: Specifies the build architecture for the kernel. The
  architectures currently supported are x86, arm and arc. The build system
  sets C flags, selects the default configuration and uses the appropriate
  toolchain tools. The default value is x86.

* :makevar:`ZEPHYR_BASE`: Sets the path to the kernel's base directory.
  This variable is usually set by the :file:`zephyr_env.sh` script.
  It can be used to get the kernel's base directory, as used in the
  Makefile.inc inclusion above, or it can be overridden to select an
  specific instance of the kernel.

* :makevar:`PROJECT_BASE`: Provides the developer's application project
  directory path. It is set by the :file:`Makefile.inc` file.

* :makevar:`SOURCE_DIR`: Overrides the default value for the application's
  source code directory. The developer source code directory is set to
  :file:`$(PROJECT_BASE/)src/` by default. This directory name should end
  with slash **'/'**.

* :makevar:`BOARD`: Selects the board that the application's
  build will use for the default configuration.

* :makevar:`CONF_FILE`: Indicates the name of a configuration fragment file.
  This file includes the kconfig configuration values that override the
  default configuration values.

* :makevar:`O`: Optional. Indicates the output directory that Kconfig uses.
  The output directory stores all the files generated during the build
  process. The default output directory is the :file:`$(PROJECT_BASE)/outdir`
  directory.

Application Debugging
=====================

This section is a quick hands-on reference to start debugging your
application with QEMU. Most content in this section is already covered on
`QEMU`_ and `GNU_Debugger`_ reference manuals.

.. _QEMU: http://wiki.qemu.org/Main_Page

.. _GNU_Debugger: http://www.gnu.org/software/gdb

In this quick reference you find shortcuts, specific environmental variables
and parameters that can help you to quickly set up your debugging
environment.

The simplest way to debug an application running in QEMU is using the GNU
Debugger and setting a local GDB server in your development system
through QEMU.

You will need an ELF binary image for debugging purposes.
The build system generates the image in the output directory.
By default, the kernel binary name is :file:`zephyr.elf`.  The name can be
changed using Kconfig.

.. note::

   We will use the standard 1234 TCP port to open a
   :abbr:`GDB (GNU Debugger)` server instance. This port number can be
   changed for a port that best suits the development system.

QEMU is the supported emulation system of the kernel. QEMU must be invoked
with the -s and -S options.

* ``-S`` Do not start CPU at startup; rather, you must type 'c' in the
  monitor.
* ``-s`` Shorthand for :literal:`-gdb tcp::1234`: open a GDB server on
  TCP port 1234.

The build system can build the elf binary and call the QEMU process with
the :makevar:`qemu` target. The QEMU debug options can be set using the
environment variable :envvar:`QEMU_EXTRA_FLAGS`. To set the ``-s`` and
``-S`` options:

.. code-block:: bash

    export QEMU_EXTRA_FLAGS="-s -S"

The build and emulation processes are called with the Makefile ``qemu``
target:

.. code-block:: bash

   make qemu

The build system will start a QEMU instance with the CPU halted at startup
and with a GDB server instance listening at the TCP port 1234.

The :file:`.gdbinit` will help initialize your GDB instance on every run.
In this example, the initialization file points to the GDB server instance.
It configures a connection to a remote target at the local host on the TCP
port 1234. The initialization sets the kernel's root directory as a
reference. The :file:`.gdbinit` file contains the following lines:

.. code-block:: bash

   target remote localhost:1234
   dir ZEPHYR_BASE

.. note::

   Substitute ZEPHYR_BASE for the current kernel's root directory.

Execute the application to debug from the same directory that you chose for
the :file:`gdbinit` file. The command can include the ``--tui`` option
to enable the use of a terminal user interface. The following commands
connects to the GDB server using :file:`gdb`. The command loads the symbol
table from the elf binary file. In this example, the elf binary file name
corresponds to :file:`zephyr.elf` file:

.. code-block:: bash

   gdb --tui zephyr.elf

.. note::

   The GDB version on the development system might not support the --tui
   option.

Finally, this command connects to the GDB server using the Data
Displayer Debugger (:file:`ddd`). The command loads the symbol table from the
elf binary file, in this instance, the :file:`zephyr.elf` file.

.. note::

   The :abbr:`DDD (Data Displayer Debugger)` may not be installed in your
   development system by default. Follow your system instructions to install
   it.

.. code-block:: bash

   ddd --gdb --debugger "gdb zephyr.elf"

.. note::

   Both commands execute the :abbr:`gdb (GNU Debugger)`.
   The command name might change depending on the toolchain you are using
   and your cross-development tools.
