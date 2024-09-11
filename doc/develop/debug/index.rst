.. _develop_debug:

Debugging
#########

.. _application_debugging:

Application Debugging
*********************

This section is a quick hands-on reference to start debugging your
application with QEMU. Most content in this section is already covered in
`QEMU`_ and `GNU_Debugger`_ reference manuals.

.. _QEMU: http://wiki.qemu.org/Main_Page

.. _GNU_Debugger: http://www.gnu.org/software/gdb

In this quick reference, you'll find shortcuts, specific environmental
variables, and parameters that can help you to quickly set up your debugging
environment.

The simplest way to debug an application running in QEMU is using the GNU
Debugger and setting a local GDB server in your development system through QEMU.

You will need an :abbr:`ELF (Executable and Linkable Format)` binary image for
debugging purposes.  The build system generates the image in the build
directory.  By default, the kernel binary name is :file:`zephyr.elf`. The name
can be changed using :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.

GDB server
==========

We will use the standard 1234 TCP port to open a :abbr:`GDB (GNU Debugger)`
server instance. This port number can be changed for a port that best suits the
development environment. There are multiple ways to do this. Each way starts a
QEMU instance with the processor halted at startup and with a GDB server
instance listening for a connection.

Running QEMU directly
~~~~~~~~~~~~~~~~~~~~~

You can run QEMU to listen for a "gdb connection" before it starts executing any
code to debug it.

.. code-block:: bash

   qemu -s -S <image>

will setup Qemu to listen on port 1234 and wait for a GDB connection to it.

The options used above have the following meaning:

* ``-S`` Do not start CPU at startup; rather, you must type 'c' in the
  monitor.
* ``-s`` Shorthand for :literal:`-gdb tcp::1234`: open a GDB server on
  TCP port 1234.


Running QEMU via :command:`ninja`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run the following inside the build directory of an application:

.. code-block:: console

   ninja debugserver

QEMU will write the console output to the path specified in
:makevar:`${QEMU_PIPE}` via CMake, typically :file:`qemu-fifo` within the build
directory. You may monitor this file during the run with :command:`tail -f
qemu-fifo`.

Running QEMU via :command:`west`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run the following from your project root:

.. code-block:: console

   west build -t debugserver_qemu

QEMU will write the console output to the terminal from which you invoked
:command:`west`.

Configuring the :command:`gdbserver` listening device
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Kconfig option :kconfig:option:`CONFIG_QEMU_GDBSERVER_LISTEN_DEV` controls
the listening device, which can be a TCP port number or a path to a character
device. GDB releases 9.0 and newer also support Unix domain sockets.

If the option is unset, then the QEMU invocation will lack a ``-s`` or a
``-gdb`` parameter. You can then use the :envvar:`QEMU_EXTRA_FLAGS` shell
environment variable to pass in your own listen device configuration.

GDB client
==========

Connect to the server by running :command:`gdb` and giving these commands:

.. code-block:: bash

   $ path/to/gdb path/to/zephyr.elf
   (gdb) target remote localhost:1234
   (gdb) dir ZEPHYR_BASE

.. note::

   Substitute the correct :ref:`ZEPHYR_BASE <important-build-vars>` for your
   system.

You can use a local GDB configuration :file:`.gdbinit` to initialize your GDB
instance on every run. Your home directory is a typical location for
:file:`.gdbinit`, but you can configure GDB to load from other locations,
including the directory from which you invoked :command:`gdb`. This example
file performs the same configuration as above:

.. code-block:: none

   target remote localhost:1234
   dir ZEPHYR_BASE

Alternate interfaces
~~~~~~~~~~~~~~~~~~~~

GDB provides a curses-based interface that runs in the terminal. Pass the ``--tui``
option when invoking :command:`gdb` or give the ``tui enable`` command within
:command:`gdb`.

.. note::

   The GDB version on your development system might not support the ``--tui``
   option. Please make sure you use the GDB binary from the SDK which
   corresponds to the toolchain that has been used to build the binary.

Finally, the command below connects to the GDB server using the :abbr:`DDD
(Data Display Debugger)`, a graphical frontend for GDB. The following command
loads the symbol table from the ELF binary file, in this instance,
:file:`zephyr.elf`.

.. code-block:: bash

   ddd --gdb --debugger "gdb zephyr.elf"

Both commands execute :command:`gdb`. The command name might
change depending on the toolchain you are using and your cross-development
tools.

:command:`ddd` may not be installed in your
development system by default. Follow your system instructions to install
it. For example, use :command:`sudo apt-get install ddd` on an Ubuntu system.

Debugging
=========

As configured above, when you connect the GDB client, the application will be
stopped at system startup. You may set breakpoints, step through code, etc. as
when running the application directly within :command:`gdb`.

.. note::

   :command:`gdb` will not print the system console output as the application runs,
   unlike when you run a native application in GDB directly. If you just
   :command:`continue` after connecting the client, the application will run,
   but nothing will appear to happen. Check the console output as described
   above.

Debug with Eclipse
******************

Overview
========

CMake supports generating a project description file that can be imported into
the Eclipse Integrated Development Environment (IDE) and used for graphical
debugging.

The `GNU MCU Eclipse plug-ins`_ provide a mechanism to debug ARM projects in
Eclipse with pyOCD, Segger J-Link, and OpenOCD debugging tools.

The following tutorial demonstrates how to debug a Zephyr application in
Eclipse with pyOCD in Windows. It assumes you have already installed the GCC
ARM Embedded toolchain and pyOCD.

Set Up the Eclipse Development Environment
==========================================

#. Download and install `Eclipse IDE for C/C++ Developers`_.

#. In Eclipse, install the `GNU MCU Eclipse plug-ins`_ by opening the menu
   ``Window->Eclipse Marketplace...``, searching for ``GNU MCU Eclipse``, and
   clicking ``Install`` on the matching result.

#. Configure the path to the pyOCD GDB server by opening the menu
   ``Window->Preferences``, navigating to ``MCU``, and setting the ``Global
   pyOCD Path``.

Generate and Import an Eclipse Project
======================================

#. Set up a GNU Arm Embedded toolchain as described in
   :ref:`toolchain_gnuarmemb`.

#. Navigate to a folder outside of the Zephyr tree to build your application.

   .. code-block:: console

      # On Windows
      cd %userprofile%

   .. note::
      If the build directory is a subdirectory of the source directory, as is
      usually done in Zephyr, CMake will warn:

      "The build directory is a subdirectory of the source directory.

      This is not supported well by Eclipse.  It is strongly recommended to use
      a build directory which is a sibling of the source directory."

#. Configure your application with CMake and build it with ninja. Note the
   different CMake generator specified by the ``-G"Eclipse CDT4 - Ninja"``
   argument. This will generate an Eclipse project description file,
   :file:`.project`, in addition to the usual ninja build files.

   .. zephyr-app-commands::
      :tool: all
      :zephyr-app: samples/synchronization
      :host-os: win
      :board: frdm_k64f
      :gen-args: -G"Eclipse CDT4 - Ninja"
      :goals: build
      :compact:

#. In Eclipse, import your generated project by opening the menu
   ``File->Import...`` and selecting the option ``Existing Projects into
   Workspace``. Browse to your application build directory in the choice,
   ``Select root directory:``. Check the box for your project in the list of
   projects found and click the ``Finish`` button.

Create a Debugger Configuration
===============================

#. Open the menu ``Run->Debug Configurations...``.

#. Select ``GDB PyOCD Debugging``, click the ``New`` button, and configure the
   following options:

   - In the Main tab:

     - Project: ``my_zephyr_app@build``
     - C/C++ Application: :file:`zephyr/zephyr.elf`

   - In the Debugger tab:

     - pyOCD Setup

       - Executable path: :file:`${pyocd_path}\\${pyocd_executable}`
       - Uncheck "Allocate console for semihosting"

     - Board Setup

       - Bus speed: 8000000 Hz
       - Uncheck "Enable semihosting"

     - GDB Client Setup

       - Executable path example (use your ``GNUARMEMB_TOOLCHAIN_PATH``):
         :file:`C:\\gcc-arm-none-eabi-6_2017-q2-update\\bin\\arm-none-eabi-gdb.exe`

   - In the SVD Path tab:

     - File path: :file:`<workspace
       top>\\modules\\hal\\nxp\\mcux\\devices\\MK64F12\\MK64F12.xml`

     .. note::
        This is optional. It provides the SoC's memory-mapped register
        addresses and bitfields to the debugger.

#. Click the ``Debug`` button to start debugging.

RTOS Awareness
==============

Support for Zephyr RTOS awareness is implemented in `pyOCD v0.11.0`_ and later.
It is compatible with GDB PyOCD Debugging in Eclipse, but you must enable
CONFIG_DEBUG_THREAD_INFO=y in your application.

Debugging I2C communication
***************************

There is a possibility to log all or some of the I2C transactions done by the application.
This feature is enabled by the Kconfig option :kconfig:option:`CONFIG_I2C_DUMP_MESSAGES`, but it
uses the ``LOG_DBG`` function to print the contents so the
:kconfig:option:`CONFIG_I2C_LOG_LEVEL_DBG` option must also be enabled.

The sample output of the dump looks like this::

   D: I2C msg: io_i2c_ctrl7_port0, addr=50
   D:    W      len=01: 00
   D:    R Sr P len=08:
   D: contents:
   D: 43 42 41 00 00 00 00 00 |CBA.....

The first line indicates the I2C controller and the target address of the transaction.
In above example, the I2C controller is named ``io_i2c_ctrl7_port0`` and the target device address
is ``0x50``

.. note::

   the address, length and contents values are in hexadecimal, but lack the ``0x`` prefix

Next lines contain messages, both sent and received. The contents of write messages is
always shown, while the content of read messages is controlled by a parameter to the
function ``i2c_dump_msgs_rw``. This function is available for use by user, but is also
called internally by ``i2c_transfer`` API function with read content dump enabled.
Before the length parameter, the header of the message is printed using abbreviations:

  - W - write message
  - R - read message
  - Sr - restart bit
  - P - stop bit

The above example shows one write message with byte ``0x00`` representing the address of register to
read from the I2C target. After that the log shows the length of received message and following
that, the bytes read from the target ``43 42 41 00 00 00 00 00``.
The content dump consist of both the hex and ASCII representation.

Filtering the I2C communication dump
====================================

By default, all I2C communication is logged between all I2C controllers and I2C targets.
It may litter the log with unrelated devices and make it difficult to effectively debug the
communication with a device of interest.

Enable the Kconfig option :kconfig:option:`CONFIG_I2C_DUMP_MESSAGES_ALLOWLIST` to create an
allowlist of I2C targets to log.
The allowlist of devices is configured using the devicetree, for example::

  / {
      i2c {
          display0: some-display@a {
              ...
          };
          sensor3: some-sensor@b {
              ...
          };
      };

      i2c-dump-allowlist {
          compatible = "zephyr,i2c-dump-allowlist";
          devices = < &display0 >, < &sensor3 >;
      };
  };

The filters nodes are identified by the compatible string with ``zephyr,i2c-dump-allowlist`` value.
The devices are selected using the ``devices`` property with phandles to the devices on the I2C bus.

In the above example, the communication with device ``display0`` and ``sensor3`` will be displayed
in the log.



.. _Eclipse IDE for C/C++ Developers: https://www.eclipse.org/downloads/packages/eclipse-ide-cc-developers/oxygen2
.. _GNU MCU Eclipse plug-ins: https://gnu-mcu-eclipse.github.io/plugins/install/
.. _pyOCD v0.11.0: https://github.com/mbedmicro/pyOCD/releases/tag/v0.11.0
