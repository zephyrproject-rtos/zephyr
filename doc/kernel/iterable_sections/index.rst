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

Then the linker has to be setup to place the structure in a
contiguous segment using one of the linker macros such as
:c:macro:`ITERABLE_SECTION_RAM` or :c:macro:`ITERABLE_SECTION_ROM`. Custom
linker snippets are normally declared using one of the
``zephyr_linker_sources()`` CMake functions, using the appropriate section
identifier, ``DATA_SECTIONS`` for RAM structures and ``SECTIONS`` for ROM ones.

.. code-block:: cmake

   # CMakeLists.txt
   zephyr_linker_sources(DATA_SECTIONS iterables.ld)

.. code-block:: c

   # iterables.ld
   ITERABLE_SECTION_RAM(my_data, 4)

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
