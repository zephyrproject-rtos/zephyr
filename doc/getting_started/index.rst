.. _getting_started:

Getting Started Guide
#####################

Follow this guide to set up a :ref:`Zephyr <introducing_zephyr>` development
environment, then build and run a sample application.

.. tip::

   Need help with something? See :ref:`help`.

.. _host_setup:

Install Host Dependencies
*************************

.. _python-pip:

Python and pip
==============

Python 3 and its package manager, pip\ [#pip]_, are used extensively by Zephyr
to install and run scripts that are required to compile and run Zephyr
applications.

Depending on your operating system, you may or may not need to provide the
``--user`` flag to the ``pip3`` command when installing new packages. This is
documented throughout the instructions.
See `Installing Packages`_ in the Python Packaging User Guide for more
information about pip\ [#pip]_, including this `information on -\\-user`_.

- On Linux, make sure ``~/.local/bin`` is on your :envvar:`PATH`
  :ref:`environment variable <env_vars>`, or programs installed with ``--user``
  won't be found\ [#linux_user]_.

- On macOS, `Homebrew disables -\\-user`_.

- On Windows, see the Installing Packages information on ``--user`` if you
  require using this option.

On all operating systems, the ``-U`` flag installs or updates the package if the
package is already installed locally but a more recent version is available. It
is good practice to use this flag if the latest version of a package is
required.

.. _install-required-tools:

Install the required tools
===========================

Follow an operating system specific guide, then come back to this page.

.. toctree::
   :maxdepth: 1

   Linux <installation_linux.rst>
   macOS <installation_mac.rst>
   Windows <installation_win.rst>

.. _get_the_code:

Get the source code
*******************

Zephyr's multi-purpose :ref:`west <west>` tool lets you easily get the Zephyr
project repositories. (While it's easiest to develop with Zephyr using west,
we also have :ref:`documentation for doing without it <no-west>`.)

Install west
============

First, install ``west`` using ``pip3``:

.. code-block:: console

   # Linux
   pip3 install --user -U west

   # macOS (Terminal) and Windows (cmd.exe)
   pip3 install -U west

See :ref:`west-install` for additional details on installing west.

.. _clone-zephyr:

Clone the Zephyr Repositories
=============================

Clone all of Zephyr's repositories in a new :file:`zephyrproject` directory:

.. code-block:: console

   west init zephyrproject
   cd zephyrproject
   west update

You can replace :file:`zephyrproject` with another directory name. West creates
the directory if it doesn't exist. See :ref:`west-multi-repo` for more details.

.. important::

   You need to run ``west update`` any time :file:`zephyr/west.yml` changes.
   This command keeps :ref:`modules` in the :file:`zephyrproject` folder in sync
   with the code in the zephyr repository, so they work correctly together.

   Some examples when ``west update`` is needed are: whenever you
   pull the :file:`zephyr` repository, switch branches in it, or perform a ``git
   bisect`` inside of it.

.. warning::

   Don't clone into a directory with spaces anywhere in the path.
   For example, on Windows, :file:`C:\\Users\\YourName\\zephyrproject` will
   work, but :file:`C:\\Users\\Your Name\\zephyrproject` will cause cryptic
   errors when you try to build an application.

.. _gs_python_deps:

Install Python Dependencies
***************************

Install Python packages required by Zephyr. From the :file:`zephyrproject`
folder that you :ref:`cloned Zephyr into <clone-zephyr>`:

.. code-block:: console

   # Linux
   pip3 install --user -r zephyr/scripts/requirements.txt

   # macOS and Windows
   pip3 install -r zephyr/scripts/requirements.txt

.. _gs_toolchain:

Set Up a Toolchain
******************

Zephyr binaries are compiled and linked by a *toolchain*\
[#tools_native_posix]_. You now need to *install* and *configure* a toolchain.
Toolchains are *installed* in the usual ways you get programs: with installer
programs or system package managers, by downloading and extracting a zip
archive, etc.

You *configure* the toolchain to use by setting :ref:`environment variables
<env_vars>`. You need to set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to a supported
value, along with additional variable(s) specific to the toolchain variant.

The following choices are available. If you're not sure what to use, check your
:ref:`board-level documentation <boards>`. If you're targeting an Arm Cortex-M,
:ref:`toolchain_gnuarmemb` is a safe bet.  On Linux, you can skip this step if
you set up the :ref:`Zephyr SDK <zephyr_sdk>` toolchains or if you want to
:ref:`gs_posix`.


.. toctree::
   :maxdepth: 2

   toolchain_3rd_party_x_compilers.rst
   toolchain_other_x_compilers.rst
   toolchain_host.rst
   toolchain_custom_cmake.rst

.. _getting_started_run_sample:

Build and Run an Application
****************************

Next, build a sample Zephyr application. You can then flash and run it on real
hardware using any supported host system. Depending on your operating system,
you can also run it in emulation with QEMU or as a native POSIX application.
Additional information about building applications can be found in the
:ref:`build_an_application` section.

Build Hello World
=================

Let's build the :ref:`hello_world` sample application.

Zephyr applications are built to run on specific hardware, which is
called a "board"\ [#board_misnomer]_. We'll use the :ref:`reel_board
<reel_board>` here, but you can change ``reel_board`` to another value if you
have a different board. See :ref:`boards` or run ``west boards`` from anywhere
inside the ``zephyrproject`` directory for a list of supported boards.

#. Go to the zephyr repository:

   .. code-block:: console

      cd zephyrproject/zephyr

#. Set up your build environment:

   .. code-block:: console

      # Linux and macOS
      source zephyr-env.sh

      # Windows
      zephyr-env.cmd

#. Build the Hello World sample for the ``reel_board``:

   .. zephyr-app-commands::
      :app: samples/hello_world
      :board: reel_board
      :goals: build

The main build products will be in :file:`build/zephyr`;
:file:`build/zephyr/zephyr.elf` is the Hello World application binary in ELF
format. Other binary formats, disassembly, and map files may be present
depending on your board.

The other sample applications in :zephyr_file:`samples` are documented in
:ref:`samples-and-demos`. If you want to re-use an existing build directory for
another board or application, you need to pass ``-p=auto`` to ``west build``.

Run the Application by Flashing to a Board
==========================================

Most "real hardware" boards supported by Zephyr can be flashed by running
``west flash``. However, this may require board-specific tool installation and
configuration to work properly.

See :ref:`application_run` in the Application Development Primer and your
board's documentation in :ref:`boards` for additional details.

Run the Application in QEMU
===========================

On Linux and macOS, you can run Zephyr applications in emulation on your host
system using QEMU when targeting either the x86 or ARM Cortex-M3 architectures.

To build and run Hello World using the x86 emulation board configuration
(``qemu_x86``), type:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: build run

To exit, type :kbd:`Ctrl-a`, then :kbd:`x`.

Use ``qemu_cortex_m3`` to target an emulated Arm Cortex-M3 instead.

.. _gs_posix:

Run a Sample Application natively (POSIX OS)
============================================

Finally, it is also possible to compile some samples to run as host processes
on a POSIX OS. This is currently only tested on Linux hosts. See
:ref:`native_posix` for more information. On 64 bit host operating systems, you
need to install a 32 bit C library; see :ref:`native_posix_deps` for details.

First, build Hello World for ``native_posix``.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_posix
   :goals: build

Next, run the application.

.. code-block:: console

   west build -t run
   # or just run zephyr.exe directly:
   ./build/zephyr/zephyr.exe

Press :kbd:`Ctrl-C` to exit.

You can run ``./build/zephyr/zephyr.exe --help`` to get a list of available
options.

This executable can be instrumented using standard tools, such as gdb or
valgrind.

.. rubric:: Footnotes

.. [#pip]

   pip is Python's package installer. Its ``install`` command first tries to
   re-use packages and package dependencies already installed on your computer.
   If that is not possible, ``pip install`` downloads them from the Python
   Package Index (PyPI) on the Internet.

   The package versions requested by Zephyr's :file:`requirements.txt` may
   conflict with other requirements on your system, in which case you may
   want to set up a virtualenv for Zephyr development.

.. [#linux_user]

   Installing with ``--user`` avoids conflicts between pip and the system
   package manager, and is the default on Debian-based distributions.

.. [#tools_native_posix]

   Usually, the toolchain is a cross-compiler and related tools which are
   different than the host compilers and other programs available for
   developing software to run natively on your operating system.

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

.. _information on -\\-user: https://packaging.python.org/tutorials/installing-packages/#installing-to-the-user-site
.. _Homebrew disables -\\-user: https://docs.brew.sh/Homebrew-and-Python#note-on-pip-install---user
.. _CMake: https://cmake.org
.. _generators: https://cmake.org/cmake/help/v3.8/manual/cmake-generators.7.html
.. _Installing Packages: https://packaging.python.org/tutorials/installing-packages/
