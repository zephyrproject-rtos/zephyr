Building extensions
###################

The LLEXT subsystem allows for the creation of extensions that can be loaded
into a running Zephyr application. When building these extensions, it's very
often useful to have access to the headers and compiler flags used by the main
Zephyr application.

The easiest path to achieve this is to build the extension as part of the
Zephyr application, using the `native Zephyr CMake features
<llext_build_native_>`_. This will result in a single build providing both the
main Zephyr application and the extension(s), which will all automatically be
built with the same parameters.

In some cases, involving the full Zephyr build system may not be feasible or
convenient; maybe the extension is built using a different compiler suite or as
part of a different project altogether. In this case, the extension developer
needs to export the headers and compiler flags used by the main Zephyr
application. This can be done using the `LLEXT Extension Development Kit
<llext_build_edk_>`_.

.. _llext_build_native:

Using the Zephyr CMake features
*******************************

The Zephyr build system provides a set of features that can be used to build
extensions as part of the Zephyr application. This is the simplest way to build
extensions, as it requires minimal additions to an application build system.

Building the extension
----------------------

An extension can be defined in the app's ``CMakeLists.txt`` by invoking the
``add_llext_target`` function, providing the target name, the output and the
source files. Usage is similar to the standard ``add_custom_target`` CMake
function:

.. code-block:: cmake

   add_llext_target(
       <target_name>
       OUTPUT <ext_file.llext>
       SOURCES <src1> [<src2>...]
   )

where:

- ``<target_name>`` is the name of the final CMake target that will result in
  the LLEXT binary being created;
- ``<ext_file.llext>`` is the name of the output file that will contain the
  packaged extension;
- ``<src1> [<src2>...]`` is the list of source files that will be compiled to
  create the extension.

The exact steps of the extension building process depend on the currently
selected :ref:`ELF object format <llext_kconfig_type>`.

The following custom properties of ``<target_name>`` are defined and can be
retrieved using the ``get_target_property()`` CMake function:

``lib_target``

    Target name for the source compilation and/or link step.

``lib_output``

    The binary file resulting from compilation and/or linking steps.

``pkg_input``

     The file to be used as input for the packaging step.

``pkg_output``

    The final extension file name.

Tweaking the build process
--------------------------

The following CMake functions can be used to modify the build system behavior
during the extension build process to a fine degree. Each of the below
functions takes the LLEXT target name as its first argument; it is otherwise
functionally equivalent to the common Zephyr ``target_*`` version.

* ``llext_compile_definitions``
* ``llext_compile_features``
* ``llext_compile_options``
* ``llext_include_directories``
* ``llext_link_options``

Custom build steps
------------------

The ``add_llext_command`` CMake function can be used to add custom build steps
that will be executed during the extension build process. The command will be
run at the specified build step and can refer to the properties of the target
for build-specific details.

The function signature is:

.. code-block:: cmake

   add_llext_command(
       TARGET <target_name>
       [PRE_BUILD | POST_BUILD | POST_PKG]
       COMMAND <command> [args...]
   )

The different build steps are:

``PRE_BUILD``

    Before the extension code is linked, if the architecture uses dynamic
    libraries. This step can access `lib_target` and its own properties.

``POST_BUILD``

    After the extension code is built, but before packaging it in an ``.llext``
    file. This step is expected to create a `pkg_input` file by reading the
    contents of `lib_output`.

``POST_PKG``

    After the extension output file has been created. The command can operate
    on the final llext file `pkg_output`.

Anything else after ``COMMAND`` will be passed to ``add_custom_command()`` as-is
(including multiple commands and other options).

.. _llext_build_edk:

LLEXT Extension Development Kit (EDK)
*************************************

When building extensions as a standalone project, outside of the main Zephyr
build system, it's important to have access to the same set of generated
headers and compiler flags used by the main Zephyr application, since they have
a direct impact on how Zephyr headers are interpreted and the extension is
compiled in general.

This can be achieved by asking Zephyr to generate an Extension Development Kit
(EDK) from the build artifacts of the main Zephyr application, by running the
following command which uses the ``llext-edk`` target:

.. code-block:: shell

    west build -t llext-edk

The generated EDK can be found in the build directory under the ``zephyr``
directory. It's a tarball that contains the headers and compile flags needed
to build extensions. The extension developer can then include the headers
and use the compile flags in their build system to build the extension.

Compile flags
-------------

The EDK includes the convenience files ``cmake.cflags`` (for CMake-based
projects) and ``Makefile.cflags`` (for Make-based ones), which define a set of
variables that contain the compile flags needed by the project. The full list
of flags needed to build an extension is provided by ``LLEXT_CFLAGS``. Also
provided is a more granular set of flags that can be used in support of
different use cases, such as when building mocks for unit tests:

``LLEXT_INCLUDE_CFLAGS``

        Compiler flags to add directories containing non-autogenerated headers
        to the compiler's include search paths.

``LLEXT_GENERATED_INCLUDE_CFLAGS``

        Compiler flags to add directories containing autogenerated headers to
        the compiler's include search paths.

``LLEXT_ALL_INCLUDE_CFLAGS``

        Compiler flags to add all directories containing headers used in the
        build to the compiler's include search paths. This is a combination of
        ``LLEXT_INCLUDE_CFLAGS`` and ``LLEXT_GENERATED_INCLUDE_CFLAGS``.

``LLEXT_GENERATED_IMACROS_CFLAGS``

        Compiler flags for autogenerated headers that must be included in the
        build via ``-imacros``.

``LLEXT_BASE_CFLAGS``

        Other compiler flags that control code generation for the target CPU.
        None of these flags are included in the above lists.

``LLEXT_CFLAGS``

        All flags required to build an extension. This is a combination of
        ``LLEXT_ALL_INCLUDE_CFLAGS``, ``LLEXT_GENERATED_IMACROS_CFLAGS`` and
        ``LLEXT_BASE_CFLAGS``.

.. _llext_kconfig_edk:

LLEXT EDK Kconfig options
-------------------------

The LLEXT EDK can be configured using the following Kconfig options:

:kconfig:option:`CONFIG_LLEXT_EDK_NAME`
    The name of the generated EDK tarball.

:kconfig:option:`CONFIG_LLEXT_EDK_USERSPACE_ONLY`
    If set, the EDK will include headers that do not contain code to route
    syscalls to the kernel. This is useful when building extensions that will
    run exclusively in user mode.

EDK Sample
----------

Refer to :zephyr:code-sample:`llext-edk` for an example of how to use the
LLEXT EDK.
