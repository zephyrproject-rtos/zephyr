..
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _codegen:

Inline Code Generation
######################

For some repetitive or parameterized coding tasks, it's convenient to
use a code generating tool to build C code fragments, instead of writing
(or editing) that source code by hand. Such a tool can also access CMake build
parameters and device tree information to generate source code automatically
tailored and tuned to a specific project configuration.

The Zephyr project supports a code generating tool that processes embedded
Python "snippets" inlined in your source files. It can be used, for example,
to generate source code that creates and fills data structures, adapts
programming logic, creates configuration-specific code fragments, and more.

.. toctree::
   :maxdepth: 1

   codegen
   functions
   build
   principle
