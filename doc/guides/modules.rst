.. _modules:

Modules (External projects)
############################

Zephyr relies on the source code of several externally maintained projects in
order to avoid reinventing the wheel and to reuse as much well-established,
mature code as possible when it makes sense. In the context of Zephyr's build
system those are called *modules*. These modules must be integrated with the
Zephyr build system, as described in more detail in other sections on
this page.

Zephyr depends on several categories of modules, including:

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
variable or by adding a :makevar:`ZEPHYR_EXTRA_MODULES` line to ``.zephyrrc``
(See the section on :ref:`env_vars` for more details). This can be
useful if you want to keep the list of modules found with west and also add
your own.

See the section about :ref:`west-multi-repo` for more details.

Finally, you can also specify the list of modules yourself in various ways, or
not use modules at all if your application doesn't need them.


Module yaml file description
****************************

A module can be described using a file named :file:`zephyr/module.yml`.
The format of :file:`zephyr/module.yml` is described in the following:


Build files
===========

Inclusion of build files, :file:`CMakeLists.txt` and :file:`Kconfig`, can be
described as:

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


Build system integration
========================

When a module has a :file:`module.yml` file, it will automatically be included into
the Zephyr build system. The path to the module is then accessible through Kconfig
and CMake variables.

In both Kconfig and CMake, the variable ``ZEPHYR_<module-name>_MODULE_DIR``
contains the absolute path to the module.

In CMake, ``ZEPHYR_<module-name>_CMAKE_DIR`` contains the
absolute path to the directory containing the :file:`CMakeLists.txt` file that
is included into CMake build system. This variable's value is empty if the
module.yml file does not specify a CMakeLists.txt.

To read these variables for a Zephyr module named ``foo``:

- In CMake: use ``${ZEPHYR_FOO_MODULE_DIR}`` for the module's top level directory, and ``${ZEPHYR_FOO_CMAKE_DIR}`` for the directory containing its :file:`CMakeLists.txt`
- In Kconfig: use ``$(ZEPHYR_FOO_MODULE_DIR)`` for the module's top level directory

Notice how a lowercase module name ``foo`` is capitalized to ``FOO``
in both CMake and Kconfig.

These variables can also be used to test whether a given module exists.
For example, to verify that ``foo`` is the name of a Zephyr module:

.. code-block:: cmake

  if(ZEPHYR_FOO_MODULE_DIR)
    # Do something if FOO exists.
  endif()

In Kconfig, the variable may be used to find additional files to include.
For example, to include the file :file:`some/Kconfig` in module ``foo``:

.. code-block:: kconfig

  source "$(ZEPHYR_FOO_MODULE_DIR)/some/Kconfig"

During CMake processing of each Zephyr module, the following two variables are
also available:

- the current module's top level directory: ``${ZEPHYR_CURRENT_MODULE_DIR}``
- the current module's :file:`CMakeLists.txt` directory: ``${ZEPHYR_CURRENT_CMAKE_DIR}``

This removes the need for a Zephyr module to know its own name during CMake
processing. The module can source additional CMake files using these ``CURRENT``
variables. For example:

.. code-block:: cmake

  include(${ZEPHYR_CURRENT_MODULE_DIR}/cmake/code.cmake)

.. _modules_build_settings:

Build settings
==============

It is possible to specify additional build settings that must be used when
including the module into the build system.

All ``root`` settings are relative to the root of the module.

Build settings supported in the :file:`module.yml` file are:

- ``board_root``: Contains additional boards that are available to the build
  system. Additional boards must be located in a :file:`<board_root>/boards`
  folder.
- ``dts_root``: Contains additional dts files related to the architecture/soc
  families. Additional dts files must be located in a :file:`<dts_root>/dts`
  folder.
- ``soc_root``: Contains additional SoCs that are available to the build
  system. Additional SoCs must be located in a :file:`<soc_root>/soc` folder.
- ``arch_root``: Contains additional architectures that are available to the
  build system. Additional architectures must be located in a
  :file:`<arch_root>/arch` folder.

Example of a :file:`module.yaml` file containing additional roots, and the
corresponding file system layout.

.. code-block:: yaml

   build:
     settings:
       board_root: .
       dts_root: .
       soc_root: .
       arch_root: .


requires the following folder structure:

.. code-block:: none

   <module-root>
   ├── arch
   ├── boards
   ├── dts
   └── soc



Sanitycheck
===========

To execute both tests and samples available in modules, the Zephyr test runner
(sanitycheck) should be pointed to the directories containing those samples and
tests. This can be done by specifying the path to both samples and tests in the
:file:`zephyr/module.yml` file.  Additionally, if a module defines out of tree
boards, the module file can point sanitycheck to the path where those files
are maintained in the module. For example:


.. code-block:: yaml

    build:
      cmake: .
    samples:
      - samples
    tests:
      - tests
    boards:
      - boards


Module Inclusion
****************

.. _modules_using_west:

Using West
==========

If west is installed and :makevar:`ZEPHYR_MODULES` is not already set, the
build system finds all the modules in your :term:`west installation` and uses
those. It does this by running :ref:`west list <west-multi-repo-misc>` to get
the paths of all the projects in the installation, then filters the results to
just those projects which have the necessary module metadata files.

Each project in the ``west list`` output is tested like this:

- If the project contains a file named :file:`zephyr/module.yml`, then the
  content of that file will be used to determine which files should be added
  to the build, as described in the previous section.

- Otherwise (i.e. if the project has no :file:`zephyr/module.yml`), the
  build system looks for :file:`zephyr/CMakeLists.txt` and
  :file:`zephyr/Kconfig` files in the project. If both are present, the project
  is considered a module, and those files will be added to the build.

- If neither of those checks succeed, the project is not considered a module,
  and is not added to :makevar:`ZEPHYR_MODULES`.

Without West
============

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
      find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

   If you choose this option, make sure to set the variable **before**  calling
   ``find_package(Zephyr ...)``, as shown above.

#. In a separate CMake script which is pre-loaded to populate the CMake cache,
   like this:

   .. code-block:: cmake

      # Put this in a file with a name like "zephyr-modules.cmake"
      set(ZEPHYR_MODULES <path-to-module1> <path-to-module2>
        CACHE STRING "pre-cached modules")

   You can tell the build system to use this file by adding ``-C
   zephyr-modules.cmake`` to your CMake command line.

Not using modules
=================

If you don't have west installed and don't specify :makevar:`ZEPHYR_MODULES`
yourself, then no additional modules are added to the build. You will still be
able to build any applications that don't require code or Kconfig options
defined in an external repository.

.. _submitting_new_modules:


Submitting changes to modules
******************************

When submitting new or making changes to existing modules the main repository
Zephyr needs a reference to the changes to be able to verify the changes. In the
main tree this is done using revisions. For code that is already merged and part
of the tree we use the commit hash, a tag, or a branch name. For pull requests
however, we require specifying the pull request number in the revision field to
allow building the Zephyr main tree with the changes submitted to the
module.

To avoid merging changes to master with pull request information, the pull
request should be marked as ``DNM`` (Do Not Merge) or preferably a draft pull
request to make sure it is not merged by mistake and to allow for the module to
be merged first and be assigned a permanent commit hash. Once the module is
merged, the revision will need to be changed either by the submitter or by the
maintainer to the commit hash of the module which reflects the changes.

Note that multiple and dependent changes to different modules can be submitted
using exactly the same process. In this case you will change multiple entries of
all modules that have a pull request against them.


Submitting a new module
========================

Requirements
-------------

Modules to be included in the default manifest of the Zephyr project need to
provide functionality or features endorsed and approved by the project technical
steering committee and should follow the project licensing and
:ref:`contribute_guidelines`.

A request for a new module should be initialized using an RFC issue in the
Zephyr project issue tracking system with details about the module and how it
integrates into the project. If the request is approved, a new repository will
created by the project team and initialized with basic information that would
allow submitting code to the module project following the project contribution
guidelines.

All modules should be hosted in repositories under the Zephyr organization. The
manifest should only point to repositories maintained under the Zephyr project.
If a module is maintained as a fork of another project on Github, the Zephyr module
related files and changes in relation to upstream need to be maintained in a
special branch named ``zephyr``.

Process
-------

Follow the following steps to request a new module:

#. Use `GitHub issues`_ to open an issue with details about the module to be
   created
#. Propose a name for the repository to be created under the Zephyr project
   organization on Github.
#. Maintainers from the Zephyr project will create the repository and initialize
   it. You will be added as a collaborator in the new repository.
#. Submit the module content (code) to the new repository following the
   guidelines described :ref:`here <modules_using_west>`.
#. Add a new entry to the :zephyr_file:`west.yml` with the following
   information:

   .. code-block:: console

        - name: <name of repository>
          path: <path to where the repository should be cloned>
          revision: <ref pointer to module pull request>


For example, to add *my_module* to the manifest:

.. code-block:: console

    - name: my_module
      path: modules/lib/my_module
      revision: pull/23/head


Where 23 in the example above indicated the pull request number submitted to the
*my_module* repository. Once the module changes are reviewed and merged, the
revision needs to be changed to the commit hash from the module repository.

.. _changes_to_existing_module:

Changes to existing modules
===========================

#. Submit the changes using a pull request to an existing repository following
   the :ref:`contribution guidelines <contribute_guidelines>`.
#. Submit a pull request changing the entry referencing the module into the
   :zephyr_file:`west.yml` of the main Zephyr tree with the following
   information:

   .. code-block:: console

        - name: <name of repository>
          path: <path to where the repository should be cloned>
          revision: <ref pointer to module pull request>


For example, to add *my_module* to the manifest:

.. code-block:: console

    - name: my_module
      path: modules/lib/my_module
      revision: pull/23/head

Where 23 in the example above indicated the pull request number submitted to the
*my_module* repository. Once the module changes are reviewed and merged, the
revision needs to be changed to the commit hash from the module repository.



.. _CMake list: https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#lists
.. _add_subdirectory(): https://cmake.org/cmake/help/latest/command/add_subdirectory.html

.. _GitHub issues: https://github.com/zephyrproject-rtos/zephyr/issues
