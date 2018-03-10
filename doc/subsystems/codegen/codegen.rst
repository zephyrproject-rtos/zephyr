..
    Copyright (c) 2004-2015 Ned Batchelder
    SPDX-License-Identifier: MIT
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _codegen_intro:

Introduction
############

Python snippets that are inlined in a source file are used as code generators.
The tool to scan the source file for the Python snippets and process them is
Codegen. Codegen and part of this documentation is based on
`Cog <https://nedbatchelder.com/code/cog/index.html>`_ from Ned Batchelder.

The processing of source files is controlled by the CMake extension functions:
zephyr_sources_codegen(..) or zephyr_library_sources_codegen(..). The generated
source files are added to the Zephyr sources. During build the source files are
processed by Codegen and the generated source files are written to the CMake
binary directory.

The inlined Python snippets can contain any Python code, they are regular
Python scripts. All Python snippets in a source file and all Python snippets of
included template files are treated as a python script with a common set of
global Python variables. Global data created in one snippet can be used in
another snippet that is processed later on. This feature could be used, for
example, to customize included template files.

An inlined Python snippet can always access the codegen module. The codegen
module encapsulates and provides all the functions to retrieve information
(options, device tree properties, CMake variables, config properties) and to
output the generated code.

Codegen transforms files in a very simple way: it finds chunks of Python code
embedded in them, executes the Python code, and places its output combined with
the original file into the generated file. The original file can contain
whatever text you like around the Python code. It will usually be source code.

For example, if you run this file through Codegen:

::

    /* This is my C file. */
    ...
    /**
     * @code{.codegen}
     * fnames = ['DoSomething', 'DoAnotherThing', 'DoLastThing']
     * for fn in fnames:
     *     codegen.outl("void %s();" % fn)
     * @endcode{.codegen}
     */
    /** @code{.codeins}@endcode */
    ...

it will come out like this:

::

    /* This is my C file. */
    ...
    /**
     * @code{.codegen}
     * fnames = ['DoSomething', 'DoAnotherThing', 'DoLastThing']
     * for fn in fnames:
     *     codegen.outl("void %s();" % fn)
     * @endcode{.codegen}
     */
    void DoSomething();
    void DoAnotherThing();
    void DoLastThing();
    /** @code{.codeins}@endcode */
    ...

Lines with ``@code{.codegen}`` or ``@code{.codeins}@endcode`` are marker lines.
The lines between ``@code{.codegen}`` and ``@endcode{.codegen}`` are the
generator Python code. The lines between ``@endcode{.codegen}`` and
``@code{.codeins}@endcode`` are the output from the generator.

When Codegen runs, it discards the last generated Python output, executes the
generator Python code, and writes its generated output into the file. All text
lines outside of the special markers are passed through unchanged.

The Codegen marker lines can contain any text in addition to the marker tokens.
This makes it possible to hide the generator Python code from the source file.

In the sample above, the entire chunk of Python code is a C comment, so the
Python code can be left in place while the file is treated as C code.


