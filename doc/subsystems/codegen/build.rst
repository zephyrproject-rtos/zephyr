..
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _codegen_build:

Code Generation in the Build Process
####################################

Code generation has to be invoked as part of the build process. Zephyr uses
`CMake <https://cmake.org/>`_ as the tool to manage building the project.

A file that contains inline code generation has to be added to the project
by one of the following commands in a :file:`CMakeList.txt` file.

.. function:: zephyr_sources_codegen(codegen_file.c [CODEGEN_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_sources_codegen_ifdef(ifguard codegen_file.c [CODEGEN_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_library_sources_codegen(codegen_file.c [CODEGEN_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_library_sources_codegen_ifdef(ifguard codegen_file.c [CODEGEN_DEFINES defines..] [DEPENDS target.. file..])

The arguments given by the ``CODEGEN_DEFINES`` keyword have to be of the form
``define_name=define_value``. The arguments become globals in the python
snippets and can be accessed by ``define_name``.

Dependencies given by the ``DEPENDS`` key word are added to the dependencies
of the generated file.
