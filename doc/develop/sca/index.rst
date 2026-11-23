.. _sca:

Static Code Analysis (SCA)
##########################

Support for static code analysis tools in Zephyr is possible through CMake.

The build setting :makevar:`ZEPHYR_SCA_VARIANT` can be used to specify the SCA
tool to use. :envvar:`ZEPHYR_SCA_VARIANT` is also supported as
:ref:`environment variable <env_vars>`.

Use ``-DZEPHYR_SCA_VARIANT=<tool>``, for example ``-DZEPHYR_SCA_VARIANT=sparse``
to enable the static analysis tool ``sparse``.

.. _sca_infrastructure:

SCA Tool infrastructure
***********************

Support for an SCA tool is implemented in a :file:`sca.cmake` file.
The :file:`sca.cmake` must be placed under :file:`{SCA_ROOT}/cmake/sca/{tool}/sca.cmake`.
Zephyr itself is always added as an :makevar:`SCA_ROOT` but the build system offers the
possibility to add additional folders to the :makevar:`SCA_ROOT` setting.

You can provide support for out of tree SCA tools by creating the following
structure:

.. code-block:: none

   <sca_root>/                 # Custom SCA root
   └── cmake/
       └── sca/
           └── <tool>/         # Name of SCA tool, this is the value given to ZEPHYR_SCA_VARIANT
               └── sca.cmake   # CMake code that configures the tool to be used with Zephyr

To add ``foo`` under ``/path/to/my_tools/cmake/sca`` create the following structure:

.. code-block:: none

   /path/to/my_tools
            └── cmake/
                └── sca/
                    └── foo/
                        └── sca.cmake

To use ``foo`` as SCA tool you must then specify ``-DZEPHYR_SCA_VARIANT=foo``.

Remember to add ``/path/to/my_tools`` to :makevar:`SCA_ROOT`.

:makevar:`SCA_TOOL` can be set as a regular CMake setting using
``-DSCA_ROOT=<sca_root>``, or added by a Zephyr module in its :file:`module.yml`
file, see :ref:`Zephyr Modules - Build settings <modules_build_settings>`

Compiler and linker launchers
=============================

An SCA tool that needs to observe or wrap the compilation and link commands
does so by setting the ``CMAKE_<LANG>_COMPILER_LAUNCHER`` and
``CMAKE_<LANG>_LINKER_LAUNCHER`` variables from its :file:`sca.cmake`. They must
be set as normal variables, not as cache entries.

A launcher set this way replaces any launcher that was already configured,
``ccache`` included. A tool that is a transparent wrapper, meaning it runs the
command it is given unmodified, may instead keep the previous launcher by
appending it:

.. code-block:: cmake

   set(CMAKE_C_COMPILER_LAUNCHER ${my_wrapper} ${CMAKE_C_COMPILER_LAUNCHER})

.. _sca_native_tools:

Native SCA Tool support
***********************

The following is a list of SCA tools natively supported by Zephyr build system.

.. toctree::
   :maxdepth: 1
   :glob:

   *
