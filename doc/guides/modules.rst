
.. _modules:

Modules (External projects)
############################

Zephyr relies on the source code of several externally maintained projects in
order to avoid reinventing the wheel and to reuse as much well-established,
mature code as possible when it makes sense. In the context of Zephyr's build
system those are called *modules*. These modules must be integrated with the
Zephyr build system, which is described in more detail in other sections on
this page.

There are several categories of external projects that Zephyr depends on,
including:

- Debugger integration
- Silicon vendor Hardware Abstraction Layers (HALs)
- Cryptography libraries
- File Systems
- Inter-Process Communication (IPC) libraries

The build system variable :makevar:`ZEPHYR_MODULES` is a `CMake list`_ of
absolute paths to the directories containing Zephyr modules. These modules
contain :file:`CMakeLists.txt` and :file:`Kconfig` files describing how to
build and configure them, respectively. Module :file:`CMakeLists.txt` files are
added to the build using CMake's `add_subdirectory()`_ command, and the
:file:`Kconfig` files are included in the build's Kconfig menu tree.

If you have :ref:`west <west>` installed, you don't need to worry about how
this variable is defined unless you are adding a new module. The build system
knows how to use west to set :makevar:`ZEPHYR_MODULES`. You can add additional
modules to this list by setting the :makevar:`ZEPHYR_EXTRA_MODULES` CMake
variable. This can be useful if you want to keep the list of modules found with
west and also add your own.

Finally, you can also specify the list of modules yourself in various ways, or
not use modules at all if your application doesn't need them.

Topologies supported
********************

The following three source code topologies supported by west:

* **T1**: Star topology with zephyr as the manifest repository:

  - The zephyr repository acts as the central repository and includes a
    complete list of projects used upstream
  - Default (upstream) configuration
  - Analogy with existing mechanisms: Git sub-modules with zephyr as the
    super-project
  - See :ref:`west-installation` for how mainline Zephyr is an example
    of this topology

* **T2**: Star topology with an application repository as the manifest
  repository:

  - A repository containing a Zephyr application acts as the central repository
    and includes a complete list of other projects, including the zephyr
    repository, required to build it
  - Useful for those focused on a single application
  - Analogy with existing mechanisms: Git sub-modules with the application as
    the super-project, zephyr and other projects as sub-modules
  - An installation using this topology could look like this:

    .. code-block:: none

       app-manifest-installation
       ├── application
       │   ├── CMakeLists.txt
       │   ├── prj.conf
       │   ├── src
       │   │   └── main.c
       │   └── west.yml
       ├── modules
       │   └── lib
       │       └── tinycbor
       └── zephyr

* **T3**: Forest topology:

  - A dedicated manifest repository which contains no Zephyr source code,
    and specifies a list of projects all at the same "level"
  - Useful for downstream distributions with no "central" repository
  - Analogy with existing mechanisms: Google repo-based source distribution
  - An installation using this topology could look like this:

    .. code-block:: none

       forest
       ├── app1
       │   ├── CMakeLists.txt
       │   ├── prj.conf
       │   └── src
       │       └── main.c
       ├── app2
       │   ├── CMakeLists.txt
       │   ├── prj.conf
       │   └── src
       │       └── main.c
       ├── manifest-repo
       │   └── west.yml
       ├── modules
       │   └── lib
       │       └── tinycbor
       └── zephyr

Module Initialization Using West
********************************

If west is installed and :makevar:`ZEPHYR_MODULES` is not already set, the
build system finds all the modules in your :term:`west installation` and uses
those. It does this by running :ref:`west list <west-multi-repo-misc>` to get
the paths of all the projects in the installation, then filters the results to
just those projects which have the necessary module metadata files.

Each project in the ``west list`` output is tested like this:

- If the project contains a file named :file:`zephyr/module.yml`, then
  its contents should look like this:

  .. code-block:: yaml

     build:
       cmake: <cmake-directory>
       kconfig: <directory>/Kconfig

  The ``cmake: <cmake-directory>`` part specifies that
  :file:`<cmake-directory>` contains the :file:`CMakeLists.txt` to use. The
  ``kconfig: <directory>/Kconfig`` part specifies the Kconfig file to use.
  Neither is required: ``cmake`` defaults to ``zephyr``, and ``kconfig``
  defaults to ``zephyr/Kconfig``.

  Here is an example :file:`module.yml` file referring to
  :file:`CMakeLists.txt` and :file:`Kconfig` files in the root directory of the
  module:

  .. code-block:: yaml

     build:
       cmake: .
       kconfig: Kconfig

- Otherwise (i.e. if the project has no :file:`zephyr/module.yml`), then the
  build system looks for :file:`zephyr/CMakeLists.txt` and
  :file:`zephyr/Kconfig` files in the project. If both are present, the project
  is considered a module, and those files will be added to the build.

- If neither of those checks succeed, the project is not considered a module,
  and is not added to :makevar:`ZEPHYR_MODULES`.

Module Initialization Without West
**********************************

If you don't have west installed or don't want the build system to use it to
find Zephyr modules, you can set :makevar:`ZEPHYR_MODULES` yourself using one
of the following options. Each of the directories in the list must contain
either a :file:`zephyr/module.yml` file or the files
:file:`zephyr/CMakeLists.txt` and :file:`Kconfig`, as described in the previous
section.

#. At the CMake command line, like this:

   .. code-block:: console

      cmake -DZEPHYR_MODULES=<path-to-module1>[;<path-to-module2>[...]] ...

#. At the top of your application's top level :file:`CMakeLists.txt`, like this:

   .. code-block:: cmake

      set(ZEPHYR_MODULES <path-to-module1> <path-to-module2> [...])
      include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

   If you choose this option, make sure to set the variable **before** including
   the boilerplate file, as shown above.

#. In a separate CMake script which is pre-loaded to populate the CMake cache,
   like this:

   .. code-block:: cmake

      # Put this in a file with a name like "zephyr-modules.cmake"
      set(ZEPHYR_MODULES <path-to-module1> <path-to-module2>
        CACHE STRING "pre-cached modules")

   You can tell the build system to use this file by adding ``-C
   zephyr-modules.cmake`` to your CMake command line.

Not Using Modules
*****************

If you don't have west installed and don't specify :makevar:`ZEPHYR_MODULES`
yourself, then no additional modules are added to the build. You will still be
able to build any applications that don't require code or Kconfig options
defined in an external repository.

.. _CMake list: https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#lists
