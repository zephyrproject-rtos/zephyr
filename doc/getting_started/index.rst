.. _getting_started:

Getting Started Guide
#####################

Follow this guide to set up a :ref:`Zephyr <introducing_zephyr>` development
environment on your system, and then build and run a sample application.

.. tip::

   Need help with something? See :ref:`help`.

.. _host_setup:

Set Up a Development System
***************************

Follow one of the following guides for your host operating system.

.. toctree::
   :maxdepth: 1

   Linux <installation_linux.rst>
   macOS <installation_mac.rst>
   Windows <installation_win.rst>

.. _get_the_code:

Get the source code
*******************

Zephyr's multi-purpose :ref:`west` tool lets you easily get the Zephyr project
source code, instead of manually cloning the Zephyr repos along with west
itself.

.. warning::

   It's possible to use Zephyr without installing west, but you have
   to **really** know :ref:`what you are doing <no-west>`.

Bootstrap west
==============

First, install the ``west`` binary and bootstrapper:

.. code-block:: console

   # Linux
   pip3 install --user west

   # macOS and Windows
   pip3 install west

.. note::
   See :ref:`west-install` for additional details on installing west.

.. _clone-zephyr:

Clone the Zephyr Repositories
=============================

.. warning::
   If you have run ``source zephyr-env.sh`` (on Linux or macOS) or
   ``zephyr-env.cmd`` (on Windows) on a clone of zephyr that predates the
   introduction of west, then the copy of west included in the clone will
   override the bootstrapper installed with ``pip``. In that case close the
   shell and open a new one in order to remove it from the ``PATH``. You
   can check which ``west`` is being executed by running::

      west --version

   You should see ``West bootstrapper version: v0.5.0`` (or higher).

Next, clone the Zephyr source code repositories from GitHub using the
``west`` tool you just installed:

.. code-block:: console

   west init zephyrproject
   cd zephyrproject
   west update

.. note::
   You can replace :file:`zephyrproject` with the folder name of your choice.
   West will create the named folder if it doesn't already exist.
   If no folder name is specified, west initializes the current
   working directory.

.. note::
   If you had previously cloned the zephyr repository manually using Git,
   create an empty enclosing folder (for example :file:`zephyrproject/`),
   and move the cloned repository into it. From the enclosing folder run::

      west init -l zephyr/
      west update

   The ``-l <path to zephyr>`` parameter instructs ``west`` to use an existing
   local copy instead of cloning a remote repository. This will create a full
   Zephyr installation (see below).

Running ``west init`` will clone west itself into ``./.west/west`` and
initialize a local installation. Running ``west update`` will pull all the
projects referenced by the manifest file (:file:`zephyr/west.yml`) into the
folders specified in it. See :ref:`west-multi-repo` for additional details, a
list of the folders and files that west will create as part of the process,
and more on how ``west`` helps manage multiple repositories.

.. warning::

   Don't clone Zephyr to a directory with spaces anywhere in the path.
   For example, on Windows, :file:`C:\\Users\\YourName\\zephyrproject` will
   work, but :file:`C:\\Users\\Your Name\\zephyrproject` will cause cryptic
   errors when you try to build an application.

.. _gs_python_deps:

Install Python Dependencies
***************************

Next, install additional Python packages required by Zephyr in a shell or
``cmd.exe`` prompt:

.. code-block:: console

   # Linux
   pip3 install --user -r zephyr/scripts/requirements.txt

   # macOS and Windows
   pip3 install -r zephyr/scripts/requirements.txt

Some notes on pip's ``--user`` option:

- Installing with ``--user`` is the default behavior on Debian-based
  distributions and is generally recommended on Linux to avoid conflicts with
  Python packages installed using the system package manager.

- On Linux, verify the Python user install directory ``~/.local/bin`` is at the front of
  your PATH environment variable, otherwise installed packages won't be
  found.

- On macOS, Homebrew disables the ``--user`` flag\ [#homebrew_user]_.

- On Windows using ``cmd.exe``, although it's possible to use the ``--user``
  flag, it makes it harder for the command prompt to find executables installed
  by pip.


Set Up a Toolchain
******************

.. note::

   On Linux, you can skip this step if you installed the :ref:`Zephyr SDK
   <zephyr_sdk>`, which includes toolchains for all supported Zephyr
   architectures.

   In some specific configurations like non-MCU x86 targets on Linux,
   you may be able to re-use the native development tools provided by
   your operating system instead of an SDK by setting
   ``ZEPHYR_TOOLCHAIN_VARIANT=host`` for gcc or
   ``ZEPHYR_TOOLCHAIN_VARIANT=llvm`` for clang.

   If you want, you can use the SDK host tools (such as OpenOCD) with a
   different toolchain by keeping the :envvar:`ZEPHYR_SDK_INSTALL_DIR`
   environment variable set to the Zephyr SDK installation directory, while
   setting :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` appropriately for a non-SDK
   toolchain.

Zephyr binaries are compiled using software called a *toolchain*. You need to
*install* and *configure* a toolchain to develop Zephyr applications\
[#tools_native_posix]_.

Toolchains can be *installed* in different ways, including using installer
programs, system package managers, or simply downloading a zip file or other
archive and extracting the files somewhere on your computer.  You *configure*
the toolchain by setting the environment variable
:envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to a recognized value, along with some
additional variable(s) specific to that toolchain (usually, this is just one
more variable which contains the path where you installed the toolchain on your
file system).

.. note::

   In previous releases of Zephyr, the ``ZEPHYR_TOOLCHAIN_VARIANT`` variable
   was called ``ZEPHYR_GCC_VARIANT``.

The following toolchain installation options are available. The right choice
for you depends on where you want to run Zephyr and any other requirements you
may have. Check your :ref:`board-level documentation <boards>` if you are
unsure about what choice to use.

.. toctree::
   :maxdepth: 2

   toolchain_3rd_party_x_compilers.rst
   toolchain_other_x_compilers.rst
   toolchain_custom_cmake.rst


To use the same toolchain in new sessions in the future you can make
sure the variables are set persistently.

On macOS and Linux, you can set the variables by putting the ``export`` lines
setting environment variables in a file :file:`~/.zephyrrc`. On Windows, you
can put the ``set`` lines in :file:`%userprofile%\\zephyrrc.cmd`. These files
are used to modify your environment when you run ``zephyr-env.sh`` (Linux,
macOS) and ``zephyr-env.cmd`` (Windows), which you will learn about in the next
step.

.. _getting_started_run_sample:

Build and Run an Application
****************************

Next, build a sample Zephyr application. You can then flash and run it on real
hardware using any supported host system. Depending on your operating system,
you can also run it in emulation with QEMU or as a native POSIX application.

.. _getting_started_cmake:

A Brief Note on the Zephyr Build System
=======================================

The Zephyr build system uses `CMake`_. CMake creates build systems in different
formats, called `generators`_. Zephyr supports the following generators:

 * ``Unix Makefiles``: Supported on UNIX-like platforms (Linux, macOS).
 * ``Ninja``: Supported on all platforms.

This documentation and Zephyr's continuous integration system mainly use
``Ninja``, but you should be able to use any supported generator to build
Zephyr applications, both when using ``cmake`` directly or ``west``.

Build the Application
=====================

Follow these steps to build the :ref:`hello_world` sample application provided
with Zephyr.

As mentioned earlier, Zephyr's build system is based on
:ref:`CMake <application>`. You can build an application either by using
``cmake`` directly or by using :ref:`west <west>`, Zephyr's meta-tool that is
also used to :ref:`manage the repositories <west-multi-repo>`. You can find
additional information about west's build capabilities in
:ref:`west-build-flash-debug`.

Zephyr applications have to be configured and built to run on some hardware
configuration, which is called a "board"\ [#board_misnomer]_. These steps show
how to build the Hello World application for the :ref:`reel_board` board.  You
can build for a different board by changing ``reel_board`` to another
supported value. See :ref:`boards` for more information, or use the ``usage``
build target from an initialized build directory to get a list.

.. note::

   If you want to re-use your existing build directory to build for another
   board, you must delete that directory's contents first by using the
   ``pristine`` build target.

#. Navigate to the main project directory:

   .. code-block:: console

      cd zephyrproject/zephyr

#. Set up your build environment:

   .. code-block:: console

      # On Linux/macOS
      source zephyr-env.sh
      # On Windows
      zephyr-env.cmd

#. Build the Hello World sample for the ``reel_board``:

   .. Note: we don't use :zephyr-app: here because we just told the user to cd
      to ZEPHYR_BASE, so it's not necessary for clarity and would clutter the
      instructions a bit.

   .. zephyr-app-commands::
      :tool: all
      :app: samples/hello_world
      :board: reel_board
      :goals: build

   On Linux/macOS you can also use ``cmake`` to build with ``make`` instead
   of ``ninja``:

   .. zephyr-app-commands::
      :tool: cmake
      :app: samples/hello_world
      :generator: make
      :host-os: unix
      :board: reel_board
      :goals: build

The main build products are in :file:`samples/hello_world/build/zephyr`.
The final application binary in ELF format is named :file:`zephyr.elf` by
default. Other binary formats and byproducts such as disassembly and map files
will be present depending on the target and build system configuration.

Other sample projects demonstrating Zephyr's features are located in
:zephyr_file:`samples` and are documented in :ref:`samples-and-demos`.

Run the Application by Flashing to a Board
==========================================

Most "real hardware" boards supported by Zephyr can be flashed by running
``west flash`` or ``ninja flash`` from the build directory. However, this may
require board-specific tool installation and configuration to work properly.

See :ref:`application_run` in the Application Development Primer and the
documentation provided with your board at :ref:`boards` for additional details
if you get an error.

Run the Application in QEMU
===========================

On Linux and macOS, you can run Zephyr applications in emulation on your host
system using QEMU when targeting either the X86 or ARM Cortex-M3 architectures.

To build and run Hello World using the x86 emulation board configuration
(``qemu_x86``), type:

.. zephyr-app-commands::
   :tool: all
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: build run

To exit, type :kbd:`Ctrl-a`, then :kbd:`x`.

Use the ``qemu_cortex_m3`` board configuration to run on an emulated Arm
Cortex-M3.

Run a Sample Application natively (POSIX OS)
============================================

Finally, it is also possible to compile some samples to run as native processes
on a POSIX OS. This is currently only tested on Linux hosts.

On 64 bit host operating systems, you will also need a 32 bit C library
installed. See the :ref:`native_posix` section on host dependencies for more
information.

To compile and run Hello World in this way, type:

.. zephyr-app-commands::
   :tool: all
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_posix
   :goals: build

and then:

.. code-block:: console

   # With west
   west build -t run

   # With ninja
   ninja run

   # or just:
   zephyr/zephyr.exe
   # Press Ctrl+C to exit

You can run ``zephyr/zephyr.exe --help`` to get a list of available
options.  See the :ref:`native_posix` document for more information.

This executable can be instrumented using standard tools, such as gdb or
valgrind.

.. rubric:: Footnotes

.. [#homebrew_user]

   For details, see
   https://docs.brew.sh/Homebrew-and-Python#note-on-pip-install---user.

.. [#tools_native_posix]

   Usually, the toolchain is a cross-compiler and related tools which are
   different than the host compilers and other programs available for
   developing software to run natively on your operating system.

   One exception is when building Zephyr as a host binary to run on a POSIX
   operating system. In this case, you still need to set up a toolchain, but it
   will provide host compilers instead of cross compilers. For details on this
   option, see :ref:`native_posix`.

.. [#board_misnomer]

   This has become something of a misnomer over time. While the target can be,
   and often is, a microprocessor running on its own dedicated hardware
   board, Zephyr also supports using QEMU to run targets built for other
   architectures in emulation, targets which produce native host system
   binaries that implement Zephyr's driver interfaces with POSIX APIs, and even
   running different Zephyr-based binaries on CPU cores of differing
   architectures on the same physical chip. Each of these hardware
   configurations is called a "board," even though that doesn't always make
   perfect sense in context.

.. _Manifest repository: https://github.com/zephyrproject-rtos/manifest
.. _CMake: https://cmake.org
.. _generators: https://cmake.org/cmake/help/v3.8/manual/cmake-generators.7.html
