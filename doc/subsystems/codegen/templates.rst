..
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0

.. _codegen_templates:

Code Generation Templates
#########################

Code generation templates provide sopisticated code generation functions.

.. contents::
   :depth: 2
   :local:
   :backlinks: top

Templates have to be included to gain access to the template's functions
and variables.

 ::

    /* This file uses templates. */
    ...
    /**
     * @code{.codegen}
     * template_in_var = 1
     * codegen.out_include('templates/template_tmpl.c')
     * if template_out_var not None:
     *     codegen.outl("int x = %s;" % template_out_var)
     * @endcode{.codegen}
     */
    /** @code{.codeins}@endcode */
    ...




