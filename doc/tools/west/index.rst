.. _west:

West
####

The Zephyr project includes a swiss-army knife command line tool
named ``west`` (Zephyr is an English name for the Latin
`Zephyrus <https://en.wiktionary.org/wiki/Zephyrus>`_, the ancient Greek god
of the west wind).

West is developed in its own `repository on GitHub`_.

West is used to obtain the source code for the Zephyr project and can also be
used to build, debug, and flash applications.

.. _west-history:

History and motivation
**********************

West was added to the Zephyr project to fulfill two fundamental requirements:

* The ability to work with multiple Git repositories
* The ability to provide a user-friendly command-line interface to the Zephyr
  build system and debug mechanisms

During the development of west, a set of :ref:`west-design-constraints` were
identified to avoid the common pitfalls of tools of this kind.

Multiple Git Repositories
=========================

Zephyr intends to provide all required building blocks needed to deploy complex
IoT applications. This in turn means that the Zephyr project is much more than
an RTOS kernel, and is instead a collection of components that work together.
In this context, there are a few reasons to work with multiple Git
repositories in a standardized manner within the project:

* Clean separation of Zephyr original code and imported projects and libraries
* Avoidance of license incompatibilities between original and imported code
* Reduction in size and scope of the core Zephyr codebase, with additional
  repositories containing optional components instead of being imported
  directly into the tree
* Safety and security certifications
* Enforcement of modularization of the components
* Out-of-tree development based on subsets of the supported boards and SoCs

See :ref:`west-multi-repo` for a detailed explanation of west's handling of
multiple repository management.

Command-line interface
======================

Zephyr uses CMake as its build system management tool, as described in
:ref:`application`.
While CMake is a very powerful system and language, its two-stage nature
as a build system generator makes it non-trivial to present the user with
a consistent set of command-line switches and options that enable advanced
usage of the build tool ultimately responsible for building, flashing, and
debugging the application.
West was introduced to unify and streamline the developer's experience when
working with Zephyr. It is very important to note that west does not
replace the CMake-based build system, but directly uses it and augments it.

See :ref:`west-flash-debug` for a detailed explanation of the flash and debug
command-line interface exposed by west.

.. _west-struct:

Structure
*********

West structure
==============

West is downloaded and installed on a system in two stages:

* Bootstrapper: Installed using ``pip``, implements ``west init``
* Installation: Installed using ``west init``, implements all other commands

Additional information about the two distinct parts of west can be found in
the corresponding sections below.

Repository structure
====================

Beyond west itself, the actual code repositories that west works with
are the following:

* Manifest repository: Cloned by ``west init``, and managed afterward with Git
  by the user. Contains the list of projects in the manifest :file:`west.yml`
  file. In the case of upstream Zephyr, the zephyr repository is the manifest
  repository.
* Projects: Cloned and managed by ``west update``. Listed in the manifest file.

See :ref:`west-mr-model` for more information on the multi-repository layout.

Bootstrapper
============

The bootstrapper module is distributed using `PyPi`_ (i.e. :file:`pip`) and it's
placed in the user's ``PATH`` becoming the only entry point to west.
It implements a single command: ``west init``. This command needs to be run
first to use the rest of functionality included in ``west``. The command
``west init`` will do the following:

* Clone west itself in a :file:`.west/west` folder
* Clone the manifest repository in the folder specified by the manifest file's
  (:file:`west.yml`) ``self.path`` section. Additional information
  on the manifest can be found in the :ref:`west-multi-repo` section)

Once ``west init`` has been run, the bootstrapper will delegate the handling of
any west commands other than ``init`` to the cloned west installation.

This means that there is a single bootstrapper instance installed at any time
(unless you use virtual environments), which can then be used to initialize as
many installations as needed.

.. _west-struct-installation:

Installation
============

A west installation is the result of running ``west init``  in an existing
folder, or ``west init <foldername>`` to create and initialize a folder. As
described on the previous section, an installation always includes a special
:file:`.west/` folder that contains a clone of west itself, as well as a local
clone of the manifest repository (``zephyr`` in the default upstream case).
Additionally, and once ``west update`` has been run, the installation will also
contain a clone of any repositories listed in the manifest. The directory tree
layout of an installation using the default Zephyr manifest therefore looks
like this:

.. code-block:: console

   └── zephyrproject/
       ├── .west/
       │   ├── config
       │   └── west/
       └── zephyr/
           └── west.yml

``west init`` also creates a configuration file :file:`.west/config`
that stores configuration metadata about the west installation.

To learn more about west's handling of multiple repositories refer to
:ref:`west-multi-repo`.

The west bootstrapper delegates handling for all commands except ``init`` to
the installation in :file:`.west/west`.

Usage
*****

West's usage is similar to Git's: general options are followed by a
command, which may also take options and arguments::

  west [common-opts] <command-name> [command-opts] [<command-args>]

Usage guides for specific groups of subcommands are in the following
pages.

.. toctree::
   :maxdepth: 1

   repo-tool.rst
   flash-debug.rst

(This list will expand as additional features are developed.)

Planned Features
****************

Additional features are under development for future versions of
Zephyr:

- Running Zephyr in emulation.

- Bootloader integration: bootloader-aware image building, signing,
  and flashing, as well as building and flashing the bootloader itself.

- Additional multiple repository support: fetching and updating repositories
  that integrate with Zephyr, such as `MCUboot`_, `OpenThread`_ and others.


.. _west-design-constraints:

Design Constraints
******************

West is:

- **Optional**: it is always *possible* to drop back to "raw"
  command-line tools, i.e. use Zephyr without using west (although west itself
  might need to be installed and accessible to the build system). It may not
  always be *convenient* to do so, however. (If all of west's features
  were already conveniently available, there would be no reason to
  develop it.)

- **Compatible with CMake**: building, flashing and debugging, and
  emulator support will always remain compatible with direct use of
  CMake.

- **Cross-platform**: West is written in Python 3, and works on all
  platforms supported by Zephyr.

- **Usable as a Library**: whenever possible, west features are
  implemented as libraries that can be used standalone in other
  programs, along with separate command line interfaces that wrap
  them.  West itself is a Python package named ``west``; its libraries
  are implemented as subpackages.

- **Conservative about features**: no features will be accepted without
  strong and compelling motivation.

- **Clearly specified**: West's behavior in cases where it wraps other
  commands is clearly specified and documented. This enables
  interoperability with third party tools, and means Zephyr developers
  can always find out what is happening "under the hood" when using west.

See `Zephyr issue #6205`_ and for more details and
discussion.

.. _no-west:

Using Zephyr without west
*************************

There are many valid usecases to avoid using a meta-tool such as west which has
been custom-designed for a particular project and its requirements.
This section applies to Zephyr users who fall into one or more of these
categories:

- Already use a multi-repository management system (for example Google repo)

- Do not need additional functionality beyond compiling and linking

In order to obtain the Zephyr source code and build it without the help of
west you will need to manually clone the repositories listed in the
`manifest file`_ one by one, at the path indicated in it.

.. code-block:: console

   mkdir zephyrproject
   cd zephyrproject
   git clone https://github.com/zephyrproject-rtos/zephyr
   # clone additional repositories listed in the manifest file

If you want to manage Zephyr's repositories without west but still need to
use west's additional functionality (flashing, debugging, etc.) it is possible
to do so by manually creating an installation:

.. code-block:: console

   # cd into zephyrproject if not already there
   git clone https://github.com/zephyrproject-rtos/west.git
   echo [manifest] > .west/config
   echo path = zephyr >> .west/config

After that, and in order for ``ninja`` to be able to invoke ``west`` you must
specify the west directory. This can be done as:

- Set the environment variable ``WEST_DIR`` to point to the directory where
  ``west`` was cloned
- Specify ``WEST_DIR`` when running ``cmake``, e.g.
  ``cmake -DWEST_DIR=<path-to-west> ...``

.. _repository on GitHub:
   https://github.com/zephyrproject-rtos/west

.. _PyPi:
   https://pypi.org/project/west/

.. _MCUboot:
   https://mcuboot.com/

.. _OpenThread:
   https://openthread.io/

.. _Zephyr issue #6205:
   https://github.com/zephyrproject-rtos/zephyr/issues/6205

.. _manifest file:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/west.yml
