.. _beyond-gsg:

Beyond the Getting Started Guide
################################

The :ref:`getting_started` gives a straight-forward path to set up
your Linux, macOS, or Windows environment for Zephyr development. In
this document, we delve deeper into Zephyr development setup
issues and alternatives.

.. _python-pip:

Python and pip
**************

Python 3 and its package manager, pip\ [#pip]_, are used extensively by Zephyr
to install and run scripts required to compile and run Zephyr
applications, set up and maintain the Zephyr development environment,
and build project documentation.

Depending on your operating system, you may need to provide the
``--user`` flag to the ``pip3`` command when installing new packages. This is
documented throughout the instructions.
See `Installing Packages`_ in the Python Packaging User Guide for more
information about pip\ [#pip]_, including `information on -\\-user`_.

- On Linux, make sure ``~/.local/bin`` is at the front of your :envvar:`PATH`
  :ref:`environment variable <env_vars>`, or programs installed with ``--user``
  won't be found. Installing with ``--user`` avoids conflicts between pip
  and the system package manager, and is the default on Debian-based
  distributions.

- On macOS, `Homebrew disables -\\-user`_.

- On Windows, see the `Installing Packages`_ information on ``--user`` if you
  require using this option.

On all operating systems, pip's ``-U`` flag installs or updates the package if the
package is already installed locally but a more recent version is available. It
is good practice to use this flag if the latest version of a package is
required.  (Check the :zephyr_file:`scripts/requirements.txt` file to
see if a specific Python package version is expected.)

Advanced Platform Setup
***********************

Here are some alternative instructions for more advanced platform setup
configurations for supported development platforms:

.. toctree::
   :maxdepth: 1

   Linux setup alternatives <getting_started/installation_linux.rst>
   macOS setup alternatives <getting_started/installation_mac.rst>
   Windows setup alternatives <getting_started/installation_win.rst>

.. _gs_toolchain:

Install a Toolchain
*******************

Zephyr binaries are compiled and linked by a *toolchain* comprised of
a cross-compiler and related tools which are different from the compiler
and tools used for developing software that runs natively on your host
operating system.

You can install the :ref:`Zephyr SDK <toolchain_zephyr_sdk>` to get toolchains for all
supported architectures, or install an :ref:`alternate toolchain <toolchains>`
recommended by the SoC vendor or a specific board (check your specific
:ref:`board-level documentation <boards>`).

You can configure the Zephyr build system to use a specific toolchain by
setting :ref:`environment variables <env_vars>` such as
:envvar:`ZEPHYR_TOOLCHAIN_VARIANT <{TOOLCHAIN}_TOOLCHAIN_PATH>` to a supported
value, along with additional variable(s) specific to the toolchain variant.

.. _gs_toolchain_update:

Updating the Zephyr SDK toolchain
*********************************

When updating Zephyr SDK, check whether the :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`
or :envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variables are already set.

* If the variables are not set, the latest compatible version of Zephyr SDK will be selected
  by default. Proceed to next step without making any changes.

* If :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` is set, the corresponding toolchain will be selected
  at build time. Zephyr SDK is identified by the value ``zephyr``.
  If the :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` environment variable is not ``zephyr``, then either
  unset it or change its value to ``zephyr`` to make sure Zephyr SDK is selected.

* If the :envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variable is set, it will override
  the default lookup location for Zephyr SDK. If you install Zephyr SDK to one
  of the :ref:`recommended locations <toolchain_zephyr_sdk_bundle_variables>`,
  you can unset this variable. Otherwise, set it to your chosen install location.

For more information about these environment variables in Zephyr, see :ref:`env_vars_important`.

Cloning the Zephyr Repositories
*******************************

The Zephyr project source is maintained in the `GitHub zephyr repo
<https://github.com/zephyrproject-rtos/zephyr>`_. External modules used
by Zephyr are found in the parent `GitHub Zephyr project
<https://github.com/zephyrproject-rtos/>`_.  Because of these
dependencies, it's convenient to use the Zephyr-created :ref:`west
<west>` tool to fetch and manage the Zephyr and external module source
code.  See :ref:`west-basics` for more details.

Once your development tools are installed, use :ref:`west` to create,
initialize, and download sources from the zephyr and external module
repos.  We'll use the name ``zephyrproject``, but you can choose any
name that does not contain a space anywhere in the path.

.. code-block:: console

   west init zephyrproject
   cd zephyrproject
   west update

The ``west update`` command fetches and keeps :ref:`modules` in the
:file:`zephyrproject` folder in sync with the code in the local zephyr
repo.

.. warning::

   You must run ``west update`` any time the :file:`zephyr/west.yml`
   changes, caused, for example, when you pull the :file:`zephyr`
   repository, switch branches in it, or perform a ``git bisect`` inside of
   it.

Keeping Zephyr updated
======================

To update the Zephyr project source code, you need to get the latest
changes via ``git``. Afterwards, run ``west update`` as mentioned in
the previous paragraph.

.. code-block:: console

   # replace zephyrproject with the path you gave west init
   cd zephyrproject/zephyr
   git pull
   west update

Export Zephyr CMake package
***************************

The :ref:`cmake_pkg` can be exported to CMake's user package registry if it has
not already been done as part of :ref:`getting_started`.

.. _gs-board-aliases:

Board Aliases
*************

Developers who work with multiple boards may find explicit board names
cumbersome and want to use aliases for common targets.  This is
supported by a CMake file with content like this:

.. code-block:: cmake

   # Variable foo_BOARD_ALIAS=bar replaces BOARD=foo with BOARD=bar and
   # sets BOARD_ALIAS=foo in the CMake cache.
   set(pca10028_BOARD_ALIAS nrf51dk_nrf51422)
   set(pca10056_BOARD_ALIAS nrf52840dk_nrf52840)
   set(k64f_BOARD_ALIAS frdm_k64f)
   set(sltb004a_BOARD_ALIAS efr32mg_sltb004a)

and specifying its location in :envvar:`ZEPHYR_BOARD_ALIASES`.  This
enables use of aliases ``pca10028`` in contexts like
``cmake -DBOARD=pca10028`` and ``west -b pca10028``.

Build and Run an Application
****************************

You can build, flash, and run Zephyr applications on real
hardware using a supported host system. Depending on your operating system,
you can also run it in emulation with QEMU, or as a native application with
:ref:`native_sim <native_sim>`.
Additional information about building applications can be found in the
:ref:`build_an_application` section.

Build Blinky
============

Let's build the :zephyr:code-sample:`blinky` sample application.

Zephyr applications are built to run on specific hardware, called a
"board"\ [#board_misnomer]_. We'll use the Phytec :ref:`reel_board
<reel_board>` here, but you can change the ``reel_board`` build target
to another value if you have a different board. See :ref:`boards` or run
``west boards`` from anywhere inside the ``zephyrproject`` directory for
a list of supported boards.

#. Go to the zephyr repository:

   .. code-block:: console

      cd zephyrproject/zephyr

#. Build the blinky sample for the ``reel_board``:

   .. zephyr-app-commands::
      :app: samples/basic/blinky
      :board: reel_board
      :goals: build

The main build products will be in :file:`build/zephyr`;
:file:`build/zephyr/zephyr.elf` is the blinky application binary in ELF
format. Other binary formats, disassembly, and map files may be present
depending on your board.

The other sample applications in the :zephyr_file:`samples` folder are
documented in :ref:`samples-and-demos`.

.. note:: If you want to reuse an
   existing build directory for another board or application, you need to
   add the parameter ``-p=auto`` to ``west build`` to clean out settings
   and artifacts from the previous build.

Run the Application by Flashing to a Board
==========================================

Most hardware boards supported by Zephyr can be flashed by running
``west flash``. This may require board-specific tool installation and
configuration to work properly.

See :ref:`application_run` and your specific board's documentation in
:ref:`boards` for additional details.

.. _setting-udev-rules:

Setting udev rules
===================

Flashing a board requires permission to directly access the board
hardware, usually managed by installation of the flashing tools.  On
Linux systems, if the ``west flash`` command fails, you likely need to
define udev rules to grant the needed access permission.

Udev is a device manager for the Linux kernel and the udev daemon
handles all user space events raised when a hardware device is added (or
removed) from the system.  We can add a rules file to grant access
permission by non-root users to certain USB-connected devices.

The OpenOCD (On-Chip Debugger) project conveniently provides a rules
file that defined board-specific rules for most Zephyr-supported
arm-based boards, so we recommend installing this rules
file by downloading it from their sourceforge repo, or if you've
installed the Zephyr SDK there is a copy of this rules file in the SDK
folder:

* Either download the OpenOCD rules file and copy it to the right
  location::

     wget -O 60-openocd.rules https://sf.net/p/openocd/code/ci/master/tree/contrib/60-openocd.rules?format=raw
     sudo cp 60-openocd.rules /etc/udev/rules.d

* or copy the rules file from the Zephyr SDK folder::

     sudo cp ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d

Then, in either case, ask the udev daemon to reload these rules::

   sudo udevadm control --reload

Unplug and plug in the USB connection to your board, and you should have
permission to access the board hardware for flashing. Check your
board-specific documentation (:ref:`boards`) for further information if
needed.

Run the Application in QEMU
===========================

On Linux and macOS, you can run Zephyr applications via emulation on your host
system using `QEMU <https://www.qemu.org/>`_ when targeting either
the x86 or ARM Cortex-M3 architectures. (QEMU is included with the Zephyr
SDK installation.)

On Windows, you need to install QEMU manually from
`Download QEMU <https://www.qemu.org/download/#windows>`_. After installation,
add path to QEMU installation folder to PATH environment variable.
To enable QEMU in Test Runner (Twister) on Windows,
:ref:`set the environment variable <env_vars>`
``QEMU_BIN_PATH`` to the path of QEMU installation folder.

For example, you can build and run the :ref:`hello_world` sample using
the x86 emulation board configuration (``qemu_x86``), with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: build run

To exit QEMU, type :kbd:`Ctrl-a`, then :kbd:`x`.

Use ``qemu_cortex_m3`` to target an emulated Arm Cortex-M3 sample.

.. _gs_native:

Run a Sample Application natively (Linux)
=========================================

You can compile some samples to run as host programs
on Linux. See :ref:`native_sim` for more information. On 64-bit host operating systems, you
need to install a 32-bit C library, or build targeting :ref:`native_sim_64 <native_sim32_64>`.

First, build Hello World for ``native_sim``.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_sim
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
   reuse packages and package dependencies already installed on your computer.
   If that is not possible, ``pip install`` downloads them from the Python
   Package Index (PyPI) on the Internet.

   The package versions requested by Zephyr's :file:`requirements.txt` may
   conflict with other requirements on your system, in which case you may
   want to set up a virtualenv for Zephyr development.

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

.. _information on -\\-user:
 https://packaging.python.org/tutorials/installing-packages/#installing-to-the-user-site
.. _Homebrew disables -\\-user:
 https://docs.brew.sh/Homebrew-and-Python#note-on-pip-install---user
.. _Installing Packages:
 https://packaging.python.org/tutorials/installing-packages/
