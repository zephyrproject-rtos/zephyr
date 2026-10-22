.. _iterable_sections_api:

Iterable Sections
#################

This page contains the reference documentation for the iterable sections APIs,
which can be used for defining iterable areas of equally-sized data structures,
that can be iterated on using :c:macro:`STRUCT_SECTION_FOREACH`.

Overview
********

An iterable section is a group of statically-defined instances of the same
struct that the linker places into a single contiguous output section.
The runtime can then iterate over all instances without having to maintain an
explicit list.

Zephyr currently supports two linker-script pipelines: a template-based one that consumes
``.ld`` scripts registered via ``zephyr_linker_sources()``, and a CMake generated one that consumes
``zephyr_iterable_section()`` calls. To work on all supported toolchains, a new iterable must
currently be declared in both.

When ``CONFIG_CMAKE_LINKER_GENERATOR=y``, the output sections for iterable sections are emitted from
the ``zephyr_iterable_section()`` calls. Any ``ITERABLE_SECTION_RAM/ROM`` definitions in ``.ld``
snippets registered via ``zephyr_linker_sources()`` are ignored.
When ``CONFIG_CMAKE_LINKER_GENERATOR=n``, the reverse holds true: ``zephyr_iterable_section()``
calls are not consumed and the ``.ld`` scripts provide the section definitions.

Because upstream Zephyr must build under both configurations, a new iterable section must be
declared in both places.

To create an iterable section requires three pieces that must agree on the struct name and RAM vs.
ROM placement:

1. **C code**: defines the struct and instantiates entries using
   :c:macro:`STRUCT_SECTION_ITERABLE` (or its ``const`` variant for ROM).

2. **Linker placement**: declared in one of two ways (*both required upstream*):

   - **CMake**: ``zephyr_iterable_section()`` consumed by the CMake-generated pipeline.
   - **Linker script**: ``ITERABLE_SECTION_RAM/ROM`` registered with ``zephyr_linker_sources()``
     consumed by the template-based pipeline.


Step 1: Define the data in C
****************************

Define the struct in a common header and provide a helper macro that
instantiates entries with :c:macro:`STRUCT_SECTION_ITERABLE`:

.. code-block:: c

    struct my_data {
             int a, b;
    };

    #define DEFINE_DATA(name, _a, _b) \
             STRUCT_SECTION_ITERABLE(my_data, name) = { \
                     .a = _a, \
                     .b = _b, \
             }

    ...

    DEFINE_DATA(d1, 1, 2);
    DEFINE_DATA(d2, 3, 4);
    DEFINE_DATA(d3, 5, 6);

For ROM-resident iterables, the instances must be declared ``const`` so the
compiler emits them into a read-only input section. The C declaration and the
CMake placement (see Step 2) must agree on RAM vs. ROM placement.

Step 2: Declare the section in CMake
************************************

The ``NAME`` argument to ``zephyr_iterable_section()`` must match the struct
name passed to :c:macro:`STRUCT_SECTION_ITERABLE`. The ``GROUP`` argument
selects the linker group that the resulting output section is placed inside.

RAM-resident example:

.. code-block:: cmake

   # CMakeLists.txt
   zephyr_iterable_section(NAME my_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})

ROM-resident example (instances declared ``const`` in C):

.. code-block:: cmake

   # CMakeLists.txt
   zephyr_iterable_section(NAME my_data GROUP RODATA_REGION)


See ``zephyr_iterable_section()`` in ``cmake/modules/extensions.cmake`` for the full argument list
and a more detailed explanation of the available ``GROUP`` options.

Step 3: Provide a linker-script
*******************************

The linker script uses :c:macro:`ITERABLE_SECTION_RAM` or
:c:macro:`ITERABLE_SECTION_ROM` to emit the actual section.

RAM-resident:

.. code-block:: c

   /* sections-ram.ld */
   #include <zephyr/linker/iterable_sections.h>

   ITERABLE_SECTION_RAM(my_data, Z_LINK_ITERABLE_SUBALIGN)

ROM-resident:

.. code-block:: c

   /* sections-rom.ld */
   #include <zephyr/linker/iterable_sections.h>

   ITERABLE_SECTION_ROM(my_data, Z_LINK_ITERABLE_SUBALIGN)

Register the linker-script from ``CMakeLists.txt``:

.. code-block:: cmake

   zephyr_linker_sources(<location> <path-to-ld-file>)

See ``zephyr_linker_sources()`` in ``cmake/modules/extensions.cmake`` for the full argument list and
a more detailed explanation of the available ``<location>`` options.

Iterating over the entries
**************************

Once the section is in place, iterate over its entries with
:c:macro:`STRUCT_SECTION_FOREACH`:

.. code-block:: c

   STRUCT_SECTION_FOREACH(my_data, data) {
           printk("%p: a: %d, b: %d\n", data, data->a, data->b);
   }

.. note::
   The linker is going to place the entries sorted by name, so the example
   above would visit ``d1``, ``d2`` and ``d3`` in that order, regardless of how
   they were defined in the code.

API Reference
*************

.. doxygengroup:: iterable_section_apis
