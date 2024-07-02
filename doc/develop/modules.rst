.. _modules:

Modules (External projects)
############################

Zephyr relies on the source code of several externally maintained projects in
order to avoid reinventing the wheel and to reuse as much well-established,
mature code as possible when it makes sense. In the context of Zephyr's build
system those are called *modules*. These modules must be integrated with the
Zephyr build system, as described in more detail in other sections on
this page.

To be classified as a candidate for being included in the default list of
modules, an external project is required to have its own life-cycle outside
the Zephyr Project, that is, reside in its own repository, and have its own
contribution and maintenance workflow and release process. Zephyr modules
should not contain code that is written exclusively for Zephyr. Instead,
such code should be contributed to the main zephyr tree.

Modules to be included in the default manifest of the Zephyr project need to
provide functionality or features endorsed and approved by the project Technical
Steering Committee and should comply with the
:ref:`module licensing requirements<modules_licensing>` and
:ref:`contribution guidelines<modules_contributing>`. They should also have a
Zephyr developer that is committed to maintain the module codebase.

Zephyr depends on several categories of modules, including but not limited to:

- Debugger integration
- Silicon vendor Hardware Abstraction Layers (HALs)
- Cryptography libraries
- File Systems
- Inter-Process Communication (IPC) libraries

Additionally, in some cases modules (particularly vendor HALs) can contain
references to optional :ref:`binary blobs <bin-blobs>`.

This page summarizes a list of policies and best practices which aim at
better organizing the workflow in Zephyr modules.

.. _modules-vs-projects:

Modules vs west projects
************************

Zephyr modules, described in this page, are not the same as :ref:`west projects
<west-workspace>`. In fact, modules :ref:`do not require west
<modules_without_west>` at all. However, when using modules :ref:`with west
<modules_using_west>`, then the build system uses west in order to find modules.

In summary:

Modules are repositories that contain a :file:`zephyr/module.yml` file, so that
the Zephyr build system can pull in the source code from the repository.
:ref:`West projects <west-manifests-projects>` are entries in the `projects:`
section in the :file:`west.yml` manifest file.
West projects are often also modules, but not always. There are west projects
that are not included in the final firmware image (eg. tools) and thus do not
need to be modules.
Modules are found by the Zephyr build system either via :ref:`west itself
<modules_using_west>`, or via the :ref:`ZEPHYR_MODULES CMake variable
<modules_without_west>`.

The contents of this page only apply to modules, and not to west projects in
general (unless they are a module themselves).

Module Repositories
*******************

* All modules included in the default manifest shall be hosted in repositories
  under the zephyrproject-rtos GitHub organization.

* The module repository codebase shall include a *module.yml* file in a
  :file:`zephyr/` folder at the root of the repository.

* Module repository names should follow the convention of using lowercase
  letters and dashes instead of underscores. This rule will apply to all
  new module repositories, except for repositories that are directly
  tracking external projects (hosted in Git repositories); such modules
  may be named as their external project counterparts.

  .. note::

     Existing module repositories that do not conform to the above convention
     do not need to be renamed to comply with the above convention.

* Module repositories names should be explicitly set in the :file:`zephyr/module.yml` file.

* Modules should use "zephyr" as the default name for the repository main
  branch. Branches for specific purposes, for example, a module branch for
  an LTS Zephyr version, shall have names starting with the 'zephyr\_' prefix.

* If the module has an external (upstream) project repository, the module
  repository should preserve the upstream repository folder structure.

  .. note::

     It is not required in module repositories to maintain a 'master'
     branch mirroring the master branch of the external repository. It
     is not recommended as this may generate confusion around the module's
     main branch, which should be 'zephyr'.

* Modules should expose all provided header files with an include pathname
  beginning with the module-name.  (E.g., mcuboot should expose its
  ``bootutil/bootutil.h`` as "mcuboot/bootutil/bootutil.h".)

.. _modules_synchronization:

Synchronizing with upstream
===========================

It is preferred to synchronize a module repository with the latest stable
release of the corresponding external project. It is permitted, however, to
update a Zephyr module repository with the latest development branch tip,
if this is required to get important updates in the module codebase. When
synchronizing a module with upstream it is mandatory to document the
rationale for performing the particular update.

Requirements for allowed practices
----------------------------------

Changes to the main branch of a module repository, including synchronization
with upstream code base, may only be applied via pull requests. These pull
requests shall be *verifiable* by Zephyr CI and *mergeable* (e.g. with the
*Rebase and merge*, or *Create a merge commit* option using Github UI). This
ensures that the incoming changes are always **reviewable**, and the
*downstream* module repository history is incremental (that is, existing
commits, tags, etc. are always preserved). This policy also allows to run
Zephyr CI, git lint, identity, and license checks directly on the set of
changes that are to be brought into the module repository.

.. note::

     Force-pushing to a module's main branch is not allowed.

Allowed practices
-----------------

The following practices conform to the above requirements and should be
followed in all modules repositories. It is up to the module code owner
to select the preferred synchronization practice, however, it is required
that the selected practice is consistently followed in the respective
module repository.

**Updating modules with a diff from upstream:**
Upstream changes brought as a single *snapshot* commit (manual diff) in a
pull request against the module's main branch, which may be merged using
the *Rebase & merge* operation. This approach is simple and
should be applicable to all modules with the downside of suppressing the
upstream history in the module repository.

  .. note::

     The above practice is the only allowed practice in modules where
     the external project is not hosted in an upstream Git repository.

The commit message is expected to identify the upstream project URL, the
version to which the module is updated (upstream version, tag, commit SHA,
if applicable, etc.), and the reason for the doing the update.

**Updating modules by merging the upstream branch:**
Upstream changes brought in by performing a Git merge of the intended upstream
branch (e.g. main branch, latest release branch, etc.) submitting the result in
pull request against the module main branch, and merging the pull request using
the *Create a merge commit* operation.
This approach is applicable to modules with an upstream project Git repository.
The main advantages of this approach is that the upstream repository history
(that is, the original commit SHAs) is preserved in the module repository. The
downside of this approach is that two additional merge commits are generated in
the downstream main branch.


Contributing to Zephyr modules
******************************

.. _modules_contributing:


Individual Roles & Responsibilities
===================================

To facilitate management of Zephyr module repositories, the following
individual roles are defined.

**Administrator:** Each Zephyr module shall have an administrator
who is responsible for managing access to the module repository,
for example, for adding individuals as Collaborators in the repository
at the request of the module owner. Module administrators are
members of the Administrators team, that is a group of project
members with admin rights to module GitHub repositories.

**Module owner:** Each module shall have a module code owner. Module
owners will have the overall responsibility of the contents of a
Zephyr module repository. In particular, a module owner will:

* coordinate code reviewing in the module repository
* be the default assignee in pull-requests against the repository's
  main branch
* request additional collaborators to be added to the repository, as
  they see fit
* regularly synchronize the module repository with its upstream
  counterpart following the policies described in
  :ref:`modules_synchronization`
* be aware of security vulnerability issues in the external project
  and update the module repository to include security fixes, as
  soon as the fixes are available in the upstream code base
* list any known security vulnerability issues, present in the
  module codebase, in Zephyr release notes.


  .. note::

     Module owners are not required to be Zephyr
     :ref:`Maintainers <project_roles>`.

**Merger:** The Zephyr Release Engineering team has the right and the
responsibility to merge approved pull requests in the main branch of a
module repository.


Maintaining the module codebase
===============================

Updates in the zephyr main tree, for example, in public Zephyr APIs,
may require patching a module's codebase. The responsibility for keeping
the module codebase up to date is shared between the **contributor** of
such updates in Zephyr and the module **owner**. In particular:

* the contributor of the original changes in Zephyr is required to submit
  the corresponding changes that are required in module repositories, to
  ensure that Zephyr CI on the pull request with the original changes, as
  well as the module integration testing are successful.

* the module owner has the overall responsibility for synchronizing
  and testing the module codebase with the zephyr main tree.
  This includes occasional advanced testing of the module's codebase
  in addition to the testing performed by Zephyr's CI.
  The module owner is required to fix issues in the module's codebase that
  have not been caught by Zephyr pull request CI runs.


.. _modules_changes:

Contributing changes to modules
===============================

Submitting and merging changes directly to a module's codebase, that is,
before they have been merged in the corresponding external project
repository, should be limited to:

* changes required due to updates in the zephyr main tree
* urgent changes that should not wait to be merged in the external project
  first, such as fixes to security vulnerabilities.

Non-trivial changes to a module's codebase, including changes in the module
design or functionality should be discouraged, if the module has an upstream
project repository. In that case, such changes shall be submitted to the
upstream project, directly.

:ref:`Submitting changes to modules <submitting_new_modules>` describes in
detail the process of contributing changes to module repositories.

Contribution guidelines
-----------------------

Contributing to Zephyr modules shall follow the generic project
:ref:`Contribution guidelines <contribute_guidelines>`.

**Pull Requests:** may be merged with minimum of 2 approvals, including
an approval by the PR assignee. In addition to this, pull requests in module
repositories may only be merged if the introduced changes are verified
with Zephyr CI tools, as described in more detail in other sections on
this page.

The merging of pull requests in the main branch of a module
repository must be coupled with the corresponding manifest
file update in the zephyr main tree.

**Issue Reporting:** `GitHub issues`_ are intentionally disabled in module
repositories, in
favor of a centralized policy for issue reporting. Tickets concerning, for
example, bugs or enhancements in modules shall be opened in the main
zephyr repository. Issues should be appropriately labeled using GitHub
labels corresponding to each module, where applicable.

  .. note::

     It is allowed to file bug reports for zephyr modules to track
     the corresponding upstream project bugs in Zephyr. These bug reports
     shall not affect the
     :ref:`Release Quality Criteria<release_quality_criteria>`.


.. _modules_licensing:

Licensing requirements and policies
***********************************

All source files in a module's codebase shall include a license header,
unless the module repository has **main license file** that covers source
files that do not include license headers.

Main license files shall be added in the module's codebase by Zephyr
developers, only if they exist as part of the external project,
and they contain a permissive OSI-compliant license. Main license files
should preferably contain the full license text instead of including an
SPDX license identifier. If multiple main license files are present it
shall be made clear which license applies to each source file in a module's
codebase.

Individual license headers in module source files supersede the main license.

Any new content to be added in a module repository will require to have
license coverage.

  .. note::

     Zephyr recommends conveying module licensing via individual license
     headers and main license files. This not a hard requirement; should
     an external project have its own practice of conveying how licensing
     applies in the module's codebase (for example, by having a single or
     multiple main license files), this practice may be accepted by and
     be referred to in the Zephyr module, as long as licensing requirements,
     for example OSI compliance, are satisfied.

License policies
================

When creating a module repository a developer shall:

* import the main license files, if they exist in the external project, and
* document (for example in the module README or .yml file) the default license
  that covers the module's codebase.

License checks
--------------

License checks (via CI tools) shall be enabled on every pull request that
adds new content in module repositories.


Documentation requirements
**************************

All Zephyr module repositories shall include an .rst file documenting:

* the scope and the purpose of the module
* how the module integrates with Zephyr
* the owner of the module repository
* synchronization information with the external project (commit, SHA, version etc.)
* licensing information as described in :ref:`modules_licensing`.

The file shall be required for the inclusion of the module and the contained
information should be kept up to date.


Testing requirements
********************

All Zephyr modules should provide some level of **integration** testing,
ensuring that the integration with Zephyr works correctly.
Integration tests:

* may be in the form of a minimal set of samples and tests that reside
  in the zephyr main tree
* should verify basic usage of the module (configuration,
  functional APIs, etc.) that is integrated with Zephyr.
* shall be built and executed (for example in QEMU) as part of
  twister runs in pull requests that introduce changes in module
  repositories.

  .. note::

     New modules, that are candidates for being included in the Zephyr
     default manifest, shall provide some level of integration testing.

  .. note::

     Vendor HALs are implicitly tested via Zephyr tests built or executed
     on target platforms, so they do not need to provide integration tests.

The purpose of integration testing is not to provide functional verification
of the module; this should be part of the testing framework of the external
project.

Certain external projects provide test suites that reside in the upstream
testing infrastructure but are written explicitly for Zephyr. These tests
may (but are not required to) be part of the Zephyr test framework.

Deprecating and removing modules
*********************************

Modules may be deprecated for reasons including, but not limited to:

* Lack of maintainership in the module
* Licensing changes in the external project
* Codebase becoming obsolete

The module information shall indicate whether a module is
deprecated and the build system shall issue a warning
when trying to build Zephyr using a deprecated module.

Deprecated modules may be removed from the Zephyr default manifest
after 2 Zephyr releases.

  .. note::

     Repositories of removed modules shall remain accessible via their
     original URL, as they are required by older Zephyr versions.


Integrate modules in Zephyr build system
****************************************

The build system variable :makevar:`ZEPHYR_MODULES` is a `CMake list`_ of
absolute paths to the directories containing Zephyr modules. These modules
contain :file:`CMakeLists.txt` and :file:`Kconfig` files describing how to
build and configure them, respectively. Module :file:`CMakeLists.txt` files are
added to the build using CMake's `add_subdirectory()`_ command, and the
:file:`Kconfig` files are included in the build's Kconfig menu tree.

If you have :ref:`west <west>` installed, you don't need to worry about how
this variable is defined unless you are adding a new module. The build system
knows how to use west to set :makevar:`ZEPHYR_MODULES`. You can add additional
modules to this list by setting the :makevar:`EXTRA_ZEPHYR_MODULES` CMake
variable or by adding a :makevar:`EXTRA_ZEPHYR_MODULES` line to ``.zephyrrc``
(See the section on :ref:`env_vars` for more details). This can be
useful if you want to keep the list of modules found with west and also add
your own.

.. note::
   If the module ``FOO`` is provided by :ref:`west <west>` but also given with
   ``-DEXTRA_ZEPHYR_MODULES=/<path>/foo`` then the module given by the command
   line variable :makevar:`EXTRA_ZEPHYR_MODULES` will take precedence.
   This allows you to use a custom version of ``FOO`` when building and still
   use other Zephyr modules provided by :ref:`west <west>`.
   This can for example be useful for special test purposes.

If you want to permanently add modules to the zephyr workspace and you are
using zephyr as your manifest repository, you can also add a west manifest file
into the :zephyr_file:`submanifests` directory. See
:zephyr_file:`submanifests/README.txt` for more details.

See :ref:`west-basics` for more on west workspaces.

Finally, you can also specify the list of modules yourself in various ways, or
not use modules at all if your application doesn't need them.

.. _module-yml:

Module yaml file description
****************************

A module can be described using a file named :file:`zephyr/module.yml`.
The format of :file:`zephyr/module.yml` is described in the following:

Module name
===========

Each Zephyr module is given a name by which it can be referred to in the build
system.

The name should be specified in the :file:`zephyr/module.yml` file. This will
ensure the module name is not changeable through user-defined directory names
or ``west`` manifest files:

.. code-block:: yaml

   name: <name>

In CMake the location of the Zephyr module can then be referred to using the
CMake variable ``ZEPHYR_<MODULE_NAME>_MODULE_DIR`` and the variable
``ZEPHYR_<MODULE_NAME>_CMAKE_DIR`` holds the location of the directory
containing the module's :file:`CMakeLists.txt` file.

.. note::
   When used for CMake and Kconfig variables, all letters in module names are
   converted to uppercase and all non-alphanumeric characters are converted
   to underscores (_).
   As example, the module ``foo-bar`` must be referred to as
   ``ZEPHYR_FOO_BAR_MODULE_DIR`` in CMake and Kconfig.

Here is an example for the Zephyr module ``foo``:

.. code-block:: yaml

   name: foo

.. note::
   If the ``name`` field is not specified then the Zephyr module name will be
   set to the name of the module folder.
   As example, the Zephyr module located in :file:`<workspace>/modules/bar` will
   use ``bar`` as its module name if nothing is specified in
   :file:`zephyr/module.yml`.

Module integration files (in-module)
====================================

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

.. _sysbuild_module_integration:

Sysbuild integration
====================

:ref:`Sysbuild<sysbuild>` is the Zephyr build system that allows for building
multiple images as part of a single application, the sysbuild build process
can be extended externally with modules as needed, for example to add custom
build steps or add additional targets to a build. Inclusion of
sysbuild-specific build files, :file:`CMakeLists.txt` and :file:`Kconfig`, can
be described as:

.. code-block:: yaml

   build:
     sysbuild-cmake: <cmake-directory>
     sysbuild-kconfig: <directory>/Kconfig

The ``sysbuild-cmake: <cmake-directory>`` part specifies that
:file:`<cmake-directory>` contains the :file:`CMakeLists.txt` to use. The
``sysbuild-kconfig: <directory>/Kconfig`` part specifies the Kconfig file to
use.

Here is an example :file:`module.yml` file referring to
:file:`CMakeLists.txt` and :file:`Kconfig` files in the `sysbuild` directory of
the module:

.. code-block:: yaml

   build:
     sysbuild-cmake: sysbuild
     sysbuild-kconfig: sysbuild/Kconfig

The module description file :file:`zephyr/module.yml` can also be used to
specify that the build files, :file:`CMakeLists.txt` and :file:`Kconfig`, are
located in a :ref:`modules_module_ext_root`.

Build files located in a ``MODULE_EXT_ROOT`` can be described as:

.. code-block:: yaml

   build:
     sysbuild-cmake-ext: True
     sysbuild-kconfig-ext: True

This allows control of the build inclusion to be described externally to the
Zephyr module.

.. _modules-vulnerability-monitoring:

Vulnerability monitoring
========================

The module description file :file:`zephyr/module.yml` can be used to improve vulnerability monitoring.

If your module needs to track vulnerabilities using an external reference
(e.g your module is forked from another repository), you can use the ``security`` section.
It contains the field ``external-references`` that contains a list of references that needs to
be monitored for your module. The supported formats are:

- CPE (Common Platform Enumeration)
- PURL (Package URL)

.. code-block:: yaml

   security:
     external-references:
       - <module-related-cpe>
       - <an-other-module-related-cpe>
       - <module-related-purl>

A real life example for `mbedTLS` module could look like this:

.. code-block:: yaml

   security:
     external-references:
       - cpe:2.3:a:arm:mbed_tls:3.5.2:*:*:*:*:*:*:*
       - pkg:github/Mbed-TLS/mbedtls@V3.5.2

.. note::
   CPE field must follow the CPE 2.3 schema provided by `NVD
   <https://csrc.nist.gov/projects/security-content-automation-protocol/specifications/cpe>`_.
   PURL field must follow the PURL specification provided by `Github
   <https://github.com/package-url/purl-spec/blob/master/PURL-SPECIFICATION.rst>`_.


Build system integration
========================

When a module has a :file:`module.yml` file, it will automatically be included into
the Zephyr build system. The path to the module is then accessible through Kconfig
and CMake variables.

Zephyr modules
--------------

In both Kconfig and CMake, the variable ``ZEPHYR_<MODULE_NAME>_MODULE_DIR``
contains the absolute path to the module.

In CMake, ``ZEPHYR_<MODULE_NAME>_CMAKE_DIR`` contains the
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

During CMake processing of each Zephyr module, the following variables are
also available:

- the current module's name: ``${ZEPHYR_CURRENT_MODULE_NAME}``
- the current module's top level directory: ``${ZEPHYR_CURRENT_MODULE_DIR}``
- the current module's :file:`CMakeLists.txt` directory: ``${ZEPHYR_CURRENT_CMAKE_DIR}``

This removes the need for a Zephyr module to know its own name during CMake
processing. The module can source additional CMake files using these ``CURRENT``
variables. For example:

.. code-block:: cmake

  include(${ZEPHYR_CURRENT_MODULE_DIR}/cmake/code.cmake)

It is possible to append values to a Zephyr `CMake list`_ variable from the module's first
CMakeLists.txt file.
To do so, append the value to the list and then set the list in the PARENT_SCOPE
of the CMakeLists.txt file. For example, to append ``bar`` to the ``FOO_LIST`` variable in the
Zephyr CMakeLists.txt scope:

.. code-block:: cmake

  list(APPEND FOO_LIST bar)
  set(FOO_LIST ${FOO_LIST} PARENT_SCOPE)

An example of a Zephyr list where this is useful is when adding additional
directories to the ``SYSCALL_INCLUDE_DIRS`` list.

Sysbuild modules
----------------

In both Kconfig and CMake, the variable ``SYSBUILD_CURRENT_MODULE_DIR``
contains the absolute path to the sysbuild module. In CMake,
``SYSBUILD_CURRENT_CMAKE_DIR`` contains the absolute path to the directory
containing the :file:`CMakeLists.txt` file that is included into CMake build
system. This variable's value is empty if the module.yml file does not specify
a CMakeLists.txt.

To read these variables for a sysbuild module:

- In CMake: use ``${SYSBUILD_CURRENT_MODULE_DIR}`` for the module's top level
  directory, and ``${SYSBUILD_CURRENT_CMAKE_DIR}`` for the directory containing
  its :file:`CMakeLists.txt`
- In Kconfig: use ``$(SYSBUILD_CURRENT_MODULE_DIR)`` for the module's top level
  directory

In Kconfig, the variable may be used to find additional files to include.
For example, to include the file :file:`some/Kconfig`:

.. code-block:: kconfig

  source "$(SYSBUILD_CURRENT_MODULE_DIR)/some/Kconfig"

The module can source additional CMake files using these variables. For
example:

.. code-block:: cmake

  include(${SYSBUILD_CURRENT_MODULE_DIR}/cmake/code.cmake)

It is possible to append values to a Zephyr `CMake list`_ variable from the
module's first CMakeLists.txt file.
To do so, append the value to the list and then set the list in the
PARENT_SCOPE of the CMakeLists.txt file. For example, to append ``bar`` to the
``FOO_LIST`` variable in the Zephyr CMakeLists.txt scope:

.. code-block:: cmake

  list(APPEND FOO_LIST bar)
  set(FOO_LIST ${FOO_LIST} PARENT_SCOPE)

Sysbuild modules hooks
----------------------

Sysbuild provides an infrastructure which allows a sysbuild module to define
a function which will be invoked by sysbuild at a pre-defined point in the
CMake flow.

Functions invoked by sysbuild:

- ``<module-name>_pre_cmake(IMAGES <images>)``: This function is called for each
  sysbuild module before CMake configure is invoked for all images.
- ``<module-name>_post_cmake(IMAGES <images>)``: This function is called for each
  sysbuild module after CMake configure has completed for all images.
- ``<module-name>_pre_domains(IMAGES <images>)``: This function is called for each
  sysbuild module before domains yaml is created by sysbuild.
- ``<module-name>_post_domains(IMAGES <images>)``: This function is called for each
  sysbuild module after domains yaml has been created by sysbuild.

arguments passed from sysbuild to the function defined by a module:

- ``<images>`` is the list of Zephyr images that will be created by the build system.

If a module ``foo`` want to provide a post CMake configure function, then the
module's sysbuild :file:`CMakeLists.txt` file must define function ``foo_post_cmake()``.

To facilitate naming of functions, the module name is provided by sysbuild CMake
through the ``SYSBUILD_CURRENT_MODULE_NAME`` CMake variable when loading the
module's sysbuild :file:`CMakeLists.txt` file.

Example of how the ``foo`` sysbuild module can define ``foo_post_cmake()``:

.. code-block:: cmake

   function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
     cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})

     message("Invoking ${CMAKE_CURRENT_FUNCTION}. Images: ${POST_CMAKE_IMAGES}")
   endfunction()

Zephyr module dependencies
==========================

A Zephyr module may be dependent on other Zephyr modules to be present in order
to function correctly. Or it might be that a given Zephyr module must be
processed after another Zephyr module, due to dependencies of certain CMake
targets.

Such a dependency can be described using the ``depends`` field.

.. code-block:: yaml

   build:
     depends:
       - <module>

Here is an example for the Zephyr module ``foo`` that is dependent on the Zephyr
module ``bar`` to be present in the build system:

.. code-block:: yaml

   name: foo
   build:
     depends:
       - bar

This example will ensure that ``bar`` is present when ``foo`` is included into
the build system, and it will also ensure that ``bar`` is processed before
``foo``.

.. _modules_module_ext_root:

Module integration files (external)
===================================

Module integration files can be located externally to the Zephyr module itself.
The ``MODULE_EXT_ROOT`` variable holds a list of roots containing integration
files located externally to Zephyr modules.

Module integration files in Zephyr
----------------------------------

The Zephyr repository contain :file:`CMakeLists.txt` and :file:`Kconfig` build
files for certain known Zephyr modules.

Those files are located under

.. code-block:: none

   <ZEPHYR_BASE>
   └── modules
       └── <module_name>
           ├── CMakeLists.txt
           └── Kconfig

Module integration files in a custom location
---------------------------------------------

You can create a similar ``MODULE_EXT_ROOT`` for additional modules, and make
those modules known to Zephyr build system.

Create a ``MODULE_EXT_ROOT`` with the following structure

.. code-block:: none

   <MODULE_EXT_ROOT>
   └── modules
       ├── modules.cmake
       └── <module_name>
           ├── CMakeLists.txt
           └── Kconfig

and then build your application by specifying ``-DMODULE_EXT_ROOT`` parameter to
the CMake build system. The ``MODULE_EXT_ROOT`` accepts a `CMake list`_ of roots as
argument.

A Zephyr module can automatically be added to the ``MODULE_EXT_ROOT``
list using the module description file :file:`zephyr/module.yml`, see
:ref:`modules_build_settings`.

.. note::

   ``ZEPHYR_BASE`` is always added as a ``MODULE_EXT_ROOT`` with the lowest
   priority.
   This allows you to overrule any integration files under
   ``<ZEPHYR_BASE>/modules/<module_name>`` with your own implementation your own
   ``MODULE_EXT_ROOT``.

The :file:`modules.cmake` file must contain the logic that specifies the
integration files for Zephyr modules via specifically named CMake variables.

To include a module's CMake file, set the variable ``ZEPHYR_<MODULE_NAME>_CMAKE_DIR``
to the path containing the CMake file.

To include a module's Kconfig file, set the variable ``ZEPHYR_<MODULE_NAME>_KCONFIG``
to the path to the Kconfig file.

The following is an example on how to add support the ``FOO`` module.

Create the following structure

.. code-block:: none

   <MODULE_EXT_ROOT>
   └── modules
       ├── modules.cmake
       └── foo
           ├── CMakeLists.txt
           └── Kconfig

and inside the :file:`modules.cmake` file, add the following content

.. code-block:: cmake

   set(ZEPHYR_FOO_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/foo)
   set(ZEPHYR_FOO_KCONFIG   ${CMAKE_CURRENT_LIST_DIR}/foo/Kconfig)

Module integration files (zephyr/module.yml)
--------------------------------------------

The module description file :file:`zephyr/module.yml` can be used to specify
that the build files, :file:`CMakeLists.txt` and :file:`Kconfig`, are located
in a :ref:`modules_module_ext_root`.

Build files located in a ``MODULE_EXT_ROOT`` can be described as:

.. code-block:: yaml

   build:
     cmake-ext: True
     kconfig-ext: True

This allows control of the build inclusion to be described externally to the
Zephyr module.

The Zephyr repository itself is always added as a Zephyr module ext root.

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
- ``snippet_root``: Contains additional snippets that are available for use.
  These snippets must be defined in :file:`snippet.yml` files underneath the
  :file:`<snippet_root>/snippets` folder. For example, if you have
  ``snippet_root: foo``, then you should place your module's
  :file:`snippet.yml` files in :file:`<your-module>/foo/snippets` or any
  nested subdirectory.
- ``soc_root``: Contains additional SoCs that are available to the build
  system. Additional SoCs must be located in a :file:`<soc_root>/soc` folder.
- ``arch_root``: Contains additional architectures that are available to the
  build system. Additional architectures must be located in a
  :file:`<arch_root>/arch` folder.
- ``module_ext_root``: Contains :file:`CMakeLists.txt` and :file:`Kconfig` files
  for Zephyr modules, see also :ref:`modules_module_ext_root`.
- ``sca_root``: Contains additional :ref:`SCA <sca>` tool implementations
  available to the build system. Each tool must be located in
  :file:`<sca_root>/sca/<tool>` folder. The folder must contain a
  :file:`sca.cmake`.

Example of a :file:`module.yaml` file containing additional roots, and the
corresponding file system layout.

.. code-block:: yaml

   build:
     settings:
       board_root: .
       dts_root: .
       soc_root: .
       arch_root: .
       module_ext_root: .


requires the following folder structure:

.. code-block:: none

   <zephyr-module-root>
   ├── arch
   ├── boards
   ├── dts
   ├── modules
   └── soc

Twister (Test Runner)
=====================

To execute both tests and samples available in modules, the Zephyr test runner
(twister) should be pointed to the directories containing those samples and
tests. This can be done by specifying the path to both samples and tests in the
:file:`zephyr/module.yml` file.  Additionally, if a module defines out of tree
boards, the module file can point twister to the path where those files
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

.. _modules-bin-blobs:

Binary Blobs
============

Zephyr supports fetching and using :ref:`binary blobs <bin-blobs>`, and their
metadata is contained entirely in :file:`zephyr/module.yml`. This is because
a binary blob must always be associated with a Zephyr module, and thus the
blob metadata belongs in the module's description itself.

Binary blobs are fetched using :ref:`west blobs <west-blobs>`.  If ``west`` is
:ref:`not used <modules_without_west>`, they must be downloaded and
verified manually.

The ``blobs`` section in :file:`zephyr/module.yml` consists of a sequence of
maps, each of which has the following entries:

- ``path``: The path to the binary blob, relative to the :file:`zephyr/blobs/`
  folder in the module repository
- ``sha256``: `SHA-256 <https://en.wikipedia.org/wiki/SHA-2>`_ checksum of the
  binary blob file
- ``type``: The :ref:`type of binary blob <bin-blobs-types>`. Currently limited
  to ``img`` or ``lib``
- ``version``: A version string
- ``license-path``: Path to the license file for this blob, relative to the root
  of the module repository
- ``url``: URL that identifies the location the blob will be fetched from, as
  well as the fetching scheme to use
- ``description``: Human-readable description of the binary blob
- ``doc-url``: A URL pointing to the location of the official documentation for
  this blob

Module Inclusion
================

.. _modules_using_west:

Using West
----------

If west is installed and :makevar:`ZEPHYR_MODULES` is not already set, the
build system finds all the modules in your :term:`west installation` and uses
those. It does this by running :ref:`west list <west-built-in-misc>` to get
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

.. _modules_without_west:

Without West
------------

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
-----------------

If you don't have west installed and don't specify :makevar:`ZEPHYR_MODULES`
yourself, then no additional modules are added to the build. You will still be
able to build any applications that don't require code or Kconfig options
defined in an external repository.

Submitting changes to modules
******************************

When submitting new or making changes to existing modules the main repository
Zephyr needs a reference to the changes to be able to verify the changes. In the
main tree this is done using revisions. For code that is already merged and part
of the tree we use the commit hash, a tag, or a branch name. For pull requests
however, we require specifying the pull request number in the revision field to
allow building the zephyr main tree with the changes submitted to the
module.

To avoid merging changes to master with pull request information, the pull
request should be marked as ``DNM`` (Do Not Merge) or preferably a draft pull
request to make sure it is not merged by mistake and to allow for the module to
be merged first and be assigned a permanent commit hash. Drafts reduce noise by
not automatically notifying anyone until marked as "Ready for review".
Once the module is
merged, the revision will need to be changed either by the submitter or by the
maintainer to the commit hash of the module which reflects the changes.

Note that multiple and dependent changes to different modules can be submitted
using exactly the same process. In this case you will change multiple entries of
all modules that have a pull request against them.

.. _submitting_new_modules:

Process for submitting a new module
===================================

Please follow the process in :ref:`external-src-process` and obtain the TSC
approval to integrate the external source code as a module

If the request is approved, a new repository will
created by the project team and initialized with basic information that would
allow submitting code to the module project following the project contribution
guidelines.

If a module is maintained as a fork of another project on Github, the Zephyr
module related files and changes in relation to upstream need to be maintained
in a special branch named ``zephyr``.

Maintainers from the Zephyr project will create the repository and initialize
it. You will be added as a collaborator in the new repository.  Submit the
module content (code) to the new repository following the guidelines described
:ref:`here <modules_using_west>`, and then add a new entry to the
:zephyr_file:`west.yml` with the following information:

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

Process for submitting changes to existing modules
==================================================

#. Submit the changes using a pull request to an existing repository following
   the :ref:`contribution guidelines <contribute_guidelines>` and
   :ref:`expectations <contributor-expectations>`.
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
