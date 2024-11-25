.. _nanopb_reference:

Nanopb
######

`Nanopb <https://jpa.kapsi.fi/nanopb/>`_ is a C implementation of Google's
`Protocol Buffers <https://protobuf.dev/>`_.

Requirements
************

Nanopb uses the protocol buffer compiler to generate source and header files,
make sure the ``protoc`` executable is installed and available.

.. tabs::

   .. group-tab:: Ubuntu

      Use ``apt`` to install dependency:

         .. code-block:: shell

            sudo apt install protobuf-compiler

   .. group-tab:: macOS

      Use ``brew`` to install dependency:

         .. code-block:: shell

            brew install protobuf

   .. group-tab:: Windows

      Use ``choco`` to install dependency:

         .. code-block:: shell

            choco install protoc


Additionally, Nanopb is an optional module and needs to be added explicitly to the workspace:

.. code-block:: shell

   west config manifest.project-filter -- +nanopb
   west update
   west packages pip --install

Configuration
*************

Make sure to include ``nanopb`` within your ``CMakeLists.txt`` file as follows:

.. code-block:: cmake

   list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
   include(nanopb)

Adding ``proto`` files can be done with the ``zephyr_nanopb_sources()`` CMake function which
ensures the generated header and source files are created before building the specified target.

Nanopb has `generator options <https://jpa.kapsi.fi/nanopb/docs/reference.html#generator-options>`_
that can be used to configure messages or fields. This allows to set fixed sizes or skip fields
entirely.

The internal CMake generator has an extension to configure ``*.options.in`` files automatically
with CMake variables.

See :zephyr_file:`samples/modules/nanopb/src/simple.options.in` and
:zephyr_file:`samples/modules/nanopb/CMakeLists.txt` for usage example.
