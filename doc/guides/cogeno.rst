..
    Copyright (c) 2018..2020 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _cogeno:

Inline code generation
######################

For some repetitive or parameterized coding tasks, it's convenient to
use a code generating tool to build code fragments, instead of writing
(or editing) that source code by hand.

Cogeno, the inline code generation tool, processes Python or Jinja2 "snippets"
inlined in your source files. It can also access CMake build
parameters and device tree information to generate source code automatically
tailored and tuned to a specific project configuration.

Cogeno can be used, for example, to generate source code that creates
and fills data structures, adapts programming logic, creates
configuration-specific code fragments, and more.

About cogeno
************

Cogeno uses script snippets that are inlined in a source file as code generators.

The inlined script snippets can contain any `Python 3 <https://www.python.org>`_
or `Jinja2 <http://jinja.pocoo.org/>`_ code, they are regular scripts.

All Python snippets in a source file and all Python snippets of
included template files are treated as a python script with a common set of
global Python variables. Global data created in one snippet can be used in
another snippet that is processed later on. This feature could be used, for
example, to customize included template files.

Jinja2 snippets provide a - compared to Python - simplified script language.

An inlined script snippet can always access the cogeno module. The cogeno
module encapsulates and provides all the functions to retrieve information
(options, device tree properties, CMake variables, config properties) and to
output the generated code.

Cogeno transforms files in a very simple way: it finds snippets of script code
embedded in them, executes the script code, and places its output combined with
the original file into the generated file. The original file can contain
whatever text you like around the script code. It will usually be source code.

For example, if you run this file through cogeno:

::

    /* This is my C file. */
    ...
    /**
     * @code{.cogeno.py}
     * fnames = ['DoSomething', 'DoAnotherThing', 'DoLastThing']
     * for fn in fnames:
     *     cogeno.outl(f'void {fn}();')
     * @endcode{.cogeno.py}
     */
    /** @code{.cogeno.ins}@endcode */
    ...

it will come out like this:

::

    /* This is my C file. */
    ...
    /**
     * @code{.cogeno.py}
     * fnames = ['DoSomething', 'DoAnotherThing', 'DoLastThing']
     * for fn in fnames:
     *     cogeno.outl(f'void {fn}();')
     * @endcode{.cogeno.py}
     */
    void DoSomething();
    void DoAnotherThing();
    void DoLastThing();
    /** @code{.cogeno.ins}@endcode */
    ...

Lines with ``@code{.cogeno.py}`` or ``@code{.cogeno.ins}@endcode`` are marker lines.
The lines between ``@code{.cogeno.py}`` and ``@endcode{.cogeno.py}`` are the
generator Python code. The lines between ``@endcode{.cogeno.py}`` and
``@code{.cogeno.ins}@endcode`` are the output from the generator.

When cogeno runs, it discards the last generated Python output, executes the
generator Python code, and writes its generated output into the file. All text
lines outside of the special markers are passed through unchanged.

The cogeno marker lines can contain any text in addition to the marker tokens.
This makes it possible to hide the generator Python code from the source file.

In the sample above, the entire chunk of Python code is a C comment, so the
Python code can be left in place while the file is treated as C code.

Cogeno is developed in its own `repository on GitLab <https://gitlab.com/b0661/cogeno>`_.

Cogeno's documentation is available:

- online at `Read the Docs <https://cogeno.readthedocs.io/en/latest/index.html>`_.
- in the `repository on GitLab <https://gitlab.com/b0661/cogeno>`_.

About cogeno modules
********************

Cogeno includes several modules to support specific code generation tasks.

* Ccode module

  The ccode module supports code generation for the C language. E.g. one of the
  function allows to generate C defines from the content of the Extended
  Device Tree Specification (EDTS) database.

* CMake module

  The cmake module provides access to CMake variables and the CMake cache.

* Extended device tree specification module

  The edts module provides access to the device tree specification data of
  a project that is stored in the Extended Device Tree Specification (EDTS)
  database.

  The EDTS database may be loaded from a json file, stored to a json file or
  extracted from the DTS files and the bindings yaml files of the project. The
  EDTS database is automatically available to cogeno scripts. It can also be
  used as a standalone tool.

* Zephyr module

  The Zephyr module provides functions to generate device driver instantiations.

 ::

    /* This file uses modules. */
    ...
    /**
     * @code{.cogeno.py}
     * cogeno.import_module('zephyr')
     * zephyr.device_declare_single(device_config, driver_name, device_init,
     *                              device_pm_control, device_level,
     *                              device_prio, device_api, device_info)
     * @endcode{.cogeno.py}
     */
    /** @code{.cogeno.ins}@endcode */
    ...

About cogeno templates
**********************

Code generation templates provide sophisticated code generation functions.

Templates are simply text files. They may be hierarchical organized.
There is always one top level template. All the other templates have
to be included to gain access to the template's functions and variables.

A template file usually contains normal text and templating commands
intermixed. A bound sequence of templating commands is called a script
snippet. As a special case a template file may be a script snippet
as a whole.

 ::

    /* This file uses templates. */
    ...
    /**
     * @code{.cogeno.py}
     * template_in_var = 1
     * cogeno.out_include('templates/template_tmpl.c')
     * if template_out_var not None:
     *     cogeno.outl("int x = %s;" % template_out_var)
     * @endcode{.cogeno.py}
     */
    /** @code{.cogeno.ins}@endcode */
    ...


Inlince code generation in the build process
********************************************

Inline code generation has to be invoked as part of the build process.

In Zephyr the processing of source files is controlled by the CMake extension functions:
``zephyr_sources_cogeno(..)`` or ``zephyr_library_sources_cogeno(..)``. The generated
source files are added to the Zephyr sources. During build the source files are
processed by cogeno and the generated source files are written to the CMake
binary directory. Zephyr uses `CMake <https://cmake.org/>`_ as the tool to manage building
the project. A file that contains inline code generation has to be added to the project
by one of the following commands in a :file:`CMakeList.txt` file:

.. function:: zephyr_sources_cogeno(file [COGENO_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_sources_cogeno_ifdef(ifguard file [COGENO_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_library_sources_cogeno(file [COGENO_DEFINES defines..] [DEPENDS target.. file..])

.. function:: zephyr_library_sources_cogeno_ifdef(ifguard file [COGENO_DEFINES defines..] [DEPENDS target.. file..])

The arguments given by the ``COGENO_DEFINES`` keyword have to be of the form
``define_name=define_value``. The arguments become globals in the python
snippets and can be accessed by ``define_name``.

Dependencies given by the ``DEPENDS`` key word are added to the dependencies
of the generated file.
