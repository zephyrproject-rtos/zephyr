.. _iterable_sections_api:

Iterable Sections
#################

This page contains the reference documentation for the iterable sections APIs,
which can be used for defining iterable areas of equally-sized data structures,
that can be iterated on using :c:macro:`STRUCT_SECTION_FOREACH`.

Usage
*****

Iterable section elements are typically used by defining the data structure and
associated initializer in a common header file, so that they can be
instantiated anywhere in the code base.

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

Then the linker has to be told to place the structures into a contiguous output section. This is
done from CMake using the ``zephyr_iterable_section()`` function, which is the toolchain-agnostic
input to Zephyr's linker script generator and works across every linker backend Zephyr supports.

The ``NAME`` argument must match the struct type name passed to
:c:macro:`STRUCT_SECTION_ITERABLE`. For RAM-resident iterables, place the section in the data
region:

.. code-block:: cmake

   # CMakeLists.txt
   zephyr_iterable_section(NAME my_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})

For ROM-resident iterables, place the output section in the read-only region. Note that the
instances themselves must also be declared ``const`` (e.g. ``const STRUCT_SECTION_ITERABLE(...)``)
so the compiler emits them into a read-only input section, the C declaration and the CMake placement
must agree.

.. code-block:: cmake

   # CMakeLists.txt
   zephyr_iterable_section(NAME my_data GROUP RODATA_REGION)

.. note::
   A number of in-tree subsystems also ship hand-written ``.ld`` snippets based on
   :c:macro:`ITERABLE_SECTION_RAM` / :c:macro:`ITERABLE_SECTION_ROM` and register them with
   ``zephyr_linker_sources()``. Those snippets exist for the legacy GNU-LD template-based linker
   pipeline and are not required for new code: ``zephyr_iterable_section()`` alone is sufficient
   and is the recommended approach.

The data can then be accessed using :c:macro:`STRUCT_SECTION_FOREACH`.

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
