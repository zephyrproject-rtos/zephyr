.. _devicetree-scope-purpose:

Scope and purpose
*****************

A *devicetree* is primarily a hierarchical data structure that describes
hardware. The `Devicetree specification`_ defines its source and binary
representations.

.. _Devicetree specification: https://www.devicetree.org/

Zephyr uses devicetree to describe:

- the hardware available on its :ref:`boards`
- that hardware's initial configuration

As such, devicetree is both a hardware description language and a configuration
language for Zephyr. See :ref:`dt_vs_kconfig` for some comparisons between
devicetree and Zephyr's other main configuration language, Kconfig.

There are two types of devicetree input files: *devicetree sources* and
*devicetree bindings*. The sources contain the devicetree itself. The bindings
describe its contents, including data types. The :ref:`build system
<build_overview>` uses devicetree sources and bindings to produce a generated C
header. The generated header's contents are abstracted by the ``devicetree.h``
API, which you can use to get information from your devicetree.

Here is a simplified view of the process:

.. figure:: zephyr_dt_build_flow.png
   :figclass: align-center

   Devicetree build flow

All Zephyr and application source code files can include and use
``devicetree.h``. This includes :ref:`device drivers <device_model_api>`,
:ref:`applications <application>`, :ref:`tests <testing>`, the kernel, etc.

The API itself is based on C macros. The macro names all start with ``DT_``. In
general, if you see a macro that starts with ``DT_`` in a Zephyr source file,
it's probably a ``devicetree.h`` macro. The generated C header contains macros
that start with ``DT_`` as well; you might see those in compiler error
messages. You always can tell a generated- from a non-generated macro:
generated macros have some lowercased letters, while the ``devicetree.h`` macro
names have all capital letters.
