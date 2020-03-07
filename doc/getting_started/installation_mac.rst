.. _installing_zephyr_mac:

Install macOS Host Dependencies
###############################

This section describes how to set up a Zephyr development environment on macOS.

These instructions have been tested on the following macOS versions:

* macOS 10.14 Mojave
* macOS 10.15 Catalina

Update Your Operating System
****************************

Before proceeding with the build, ensure your OS is up to date.

.. _mac_requirements:

Install Requirements and Dependencies
*************************************

.. NOTE FOR DOCS AUTHORS: DO NOT PUT DOCUMENTATION BUILD DEPENDENCIES HERE.

   This section is for dependencies to build Zephyr binaries, *NOT* this
   documentation. If you need to add a dependency only required for building
   the docs, add it to doc/README.rst. (This change was made following the
   introduction of LaTeX->PDF support for the docs, as the texlive footprint is
   massive and not needed by users not building PDF documentation.)

Zephyr requires Python 3, while macOS only provides a Python 2
installation. After following these instructions, the version of Python 2
provided by macOS in ``/usr/bin/`` will sit alongside the Python 3 installation
from Homebrew in ``/usr/local/bin``.

First, install :program:`Homebrew` by following instructions on the `Homebrew
site`_. Homebrew is a free and open-source package management system that
simplifies the installation of software on macOS.  While installing Homebrew,
you may be prompted to install additional missing dependencies; please follow
any such instructions as well.

Now install these host dependencies with the ``brew`` command:

.. code-block:: console

   brew install cmake ninja gperf ccache dfu-util qemu dtc python3

.. _Homebrew site: https://brew.sh/

.. _mac-gatekeeper:

Important note about Gatekeeper
*******************************

Starting with macOS 10.15 Catalina, applications launched from the macOS
Terminal application (or any other terminal emulator) are subject to the same
system security policies that are applied to applications launched from the
Dock. This means that if you download executable binaries using a web browser,
macOS will not let you execute those from the Terminal by default. In order to
get around this issue you can take two different approaches:

* Run ``xattr -r -d com.apple.quarantine /path/to/folder`` where
  ``path/to/folder`` is the path to the enclosing folder where the executables
  you want to run are located.

* Open "System Preferences" -> "Security and Privacy" -> "Privacy" and then
  scroll down to "Developer Tools". Then unlock the lock to be able to make
  changes and check the checkbox corresponding to your terminal emulator of
  choice. This will apply to any executable being launched from such terminal
  program.

Note that this section does **not** apply to executables installed with
Homebrew, since those are automatically un-quarantined by ``brew`` itself. This
is however relevant for most :ref:`third_party_x_compilers`.

.. _macOS Gatekeeper: https://en.wikipedia.org/wiki/Gatekeeper_(macOS)

Additional notes for MacPorts users
***********************************

While MacPorts is not officially supported in this guide, it is possible to use
MacPorts instead of Homebrew to get all the required dependencies on macOS.
Note also that you may need to install ``rust`` and ``cargo`` for the Python
dependencies to install correctly.
