..
    Copyright (c) 2004-2015 Ned Batchelder
    SPDX-License-Identifier: MIT
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _codegen_functions:

Code Generation Functions
#########################

A module called ``codegen`` provides the core functions for inline
code generation. It encapsulates all the functions to retrieve information
(options, device tree properties, CMake variables, config properties) and
to output the generated code.

.. contents::
   :depth: 2
   :local:
   :backlinks: top

The ``codegen`` module is automatically imported by all code snippets. No
explicit import is necessary.

Output
------

.. function:: codegen.out(sOut=’’ [, dedent=False][, trimblanklines=False])

    Writes text to the output.

    :param sOut: The string to write to the output.
    :param dedent: If dedent is True, then common initial white space is
                   removed from the lines in sOut before adding them to the
                   output.
    :param trimblanklines: If trimblanklines is True,
                           then an initial and trailing blank line are removed
                           from sOut before adding them to the output.

    ``dedent`` and ``trimblanklines`` make it easier to use
    multi-line strings, and they are only are useful for multi-line strings:

    ::

        codegen.out("""
            These are lines I
            want to write into my source file.
        """, dedent=True, trimblanklines=True)

.. function:: codegen.outl

    Same as codegen.out, but adds a trailing newline.

.. attribute:: codegen.inFile

    An attribute, the path of the input file.

.. attribute:: codegen.outFile

    An attribute, the path of the output file.

.. attribute:: codegen.firstLineNum

    An attribute, the line number of the first line of Python code in the
    generator. This can be used to distinguish between two generators in the
    same input file, if needed.

.. attribute:: codegen.previous

    An attribute, the text output of the previous run of this generator. This
    can be used for whatever purpose you like, including outputting again with
    codegen.out()

The codegen module also provides a set of convenience functions:


Code generation module import
-----------------------------

.. function:: codegen.module_import(module_name)

    Import a module from the codegen/modules package.

    After import the module's functions and variables can be accessed by
    module_name.func() and module_name.var.

    :param module_name: Module to import. Specified without any path.

    See :ref:`codegen_modules` for the available modules.

Template file inclusion
-----------------------

.. function:: codegen.out_include(include_file)

    Write the text from include_file to the output. The :file:`include_file`
    is processed by Codegen. Inline code generation in ``include_file``
    can access the globals defined in the ``including source file`` before
    inclusion. The ``including source file`` can access the globals defined in
    the ``include_file`` (after inclusion).

    :param include_file: path of include file, either absolute path or relative
                         to current directory or relative to templates directory
                         (e.g. 'templates/drivers/simple_tmpl.c')

    See :ref:`codegen_templates` for the templates in the Codegen templates
    folders.

.. function:: codegen.guard_include()

   Prevent the current file to be included by ``codegen.out_include()``
   when called the next time.

Configuration property access
-----------------------------

.. function:: codegen.config_property(property_name [, default="<unset>"])

    Get the value of a configuration property from :file:`autoconf.h`. If
    ``property_name`` is not given in :file:`autoconf.h` the default value is
    returned.

CMake variable access
---------------------

.. function:: codegen.cmake_variable(variable_name [, default="<unset>"])

    Get the value of a CMake variable. If variable_name is not provided to
    Codegen by CMake the default value is returned. The following variables
    are provided to Codegen:

    - "PROJECT_NAME"
    - "PROJECT_SOURCE_DIR"
    - "PROJECT_BINARY_DIR"
    - "CMAKE_SOURCE_DIR"
    - "CMAKE_BINARY_DIR"
    - "CMAKE_CURRENT_SOURCE_DIR"
    - "CMAKE_CURRENT_BINARY_DIR"
    - "CMAKE_CURRENT_LIST_DIR"
    - "CMAKE_FILES_DIRECTORY"
    - "CMAKE_PROJECT_NAME"
    - "CMAKE_SYSTEM"
    - "CMAKE_SYSTEM_NAME"
    - "CMAKE_SYSTEM_VERSION"
    - "CMAKE_SYSTEM_PROCESSOR"
    - "CMAKE_C_COMPILER"
    - "CMAKE_CXX_COMPILER"
    - "CMAKE_COMPILER_IS_GNUCC"
    - "CMAKE_COMPILER_IS_GNUCXX"
    - "GENERATED_DTS_BOARD_H"
    - "GENERATED_DTS_BOARD_CONF"

.. function:: codegen.cmake_cache_variable(variable_name [, default="<unset>"])

    Get the value of a CMake variable from CMakeCache.txt. If variable_name
    is not given in CMakeCache.txt the default value is returned.

Extended device tree database access
------------------------------------

.. function:: codegen.edts()

    Get the extended device tree database.

    :return: extended device tree database

Guarding chunks of source code
------------------------------

.. function:: codegen.outl_guard_config(property_name)

    Write a guard (#if [guard]) C preprocessor directive to output.

    If there is a configuration property of the given name the property value
    is used as guard value, otherwise it is set to 0.

    :param property_name: Name of the configuration property.

.. function:: codegen.outl_unguard_config(property_name)

    Write an unguard (#endif) C preprocessor directive to output.

    This is the closing command for codegen.outl_guard_config().

    :param property_name: Name of the configuration property.

Error handling
--------------

.. function:: codegen.error(msg='Error raised by codegen.' [, frame_index=0] [, snippet_lineno=0])

    Raise a codegen.Error exception.

    Instead of raising standard python errors, codegen generators can use
    this function. Extra information is added that maps the python snippet
    line seen by the Python interpreter to the line of the file that inlines
    the python snippet.

    :param msg: Exception message.
    :param frame_index: Call frame index. The call frame offset of the function
                        calling codegen.error(). Zero if directly called in a
                        snippet. Add one for every level of function call.
    :param snippet_lineno: Line number within snippet.

Logging
-------

.. function:: codegen.log(message [, message_type=None] [, end="\n"] [, logonly=True])

.. function:: codegen.msg(msg)

    Prints msg to stdout with a “Message: ” prefix.

.. function:: codegen.prout(s [, end="\n"])

.. function:: codegen.prerr(s [, end="\n"])
