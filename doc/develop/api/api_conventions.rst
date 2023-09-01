.. _api_conventions.rst:

API Conventions
###############

Zephyr provides a few programming interfaces that users can use for creating their applications.
Zephyr's API space includes the following areas:

- C programming interface (function prototypes, structures, and macros), provided in C header files
- Configuration system (Kconfig)
- Hardware description system (Devicetree)

Other areas, such as the build system (CMake), may also be seen as Zephyr's API. However, any area
that is not explicitly listed herein is not considered Zephyr's API as of today.

API classification
******************
The files that define the Zephyr APIs may contain symbols intended for different usage. The intended
API usage is designated by the API class. Zephyr APIs are classified as:

Private
    These APIs are intended for use within the boundary of a :term:`software component`. Private APIs
    defined in the main Zephyr tree are not subject to :ref:`api_lifecycle`. Therefore, they can be changed or
    removed at any time. Changes to the private APIs may not be documented at all, and are not included in the 
    migration guide.

Internal
    In general, these APIs are intended for use only between certain software components that are
    located in the main Zephyr tree. The context where the API is called or implemented is well defined. For
    example, functions prefixed with `arch_` are intended for use by the Zephyr kernel to invoke
    architecture-specific code. An API is classified as internal on a case by case basis.
    Internal APIs should not be used by out-of-tree code. Internal APIs are not subject
    to :ref:`api_lifecycle`. Therefore, can be changed or removed at any time. However, changes to the
    internal APIs must be documented in the migration guide.

Public
    These APIs intended for use from any :term:`software component`. Public APIs may be used in-tree
    and out-of-tree. Public APIs are subject to the :ref:`api_lifecycle`. Therefore, changes to an
    API are introduced and documented according to the rules defined for the API life cycle. This includes 
    documenting any breaking changes in the migration guide.

Note, that only APIs defined in the main Zephyr tree are subject to :ref:`api_lifecycle`. External projects
used as :ref:`modules` may define their own rules for API lifecycle.

Zephyr is a constantly evolving project and API classification may change over time. A Private or
Internal API may be promoted to Internal or Public API, respectively. Zephyr users are encouraged to
follow :ref:`rfcs` process to recommend changes in API classification.

The following sections provide guidelines on how to identify the class of an API depending on its
type.

Header files contents
=====================
Private
    Functions and data types declared in header files located in
    :zephyr_file:`include/zephyr/private/`. In addition, private symbols are prefixed with ``z_``.
    Due to historical reasons some APIs prefixed with ``z_`` are public.

Internal
    Functions and data types declared :zephyr_file:`include/zephyr/internal`. In addition, Internal
    APIs must use ``@internal`` doxygen command.

Public
    Functions and data types declared in header files located in :zephyr_file:`include/zephyr/`.

In addition, the following prefixes are reserved by Zephyr kernel for use in Zephyr Public APIs:

.. list-table:: Prefixes and Descriptions
   :header-rows: 1
   :widths: 10 40 40
   :stub-columns: 1

   * - Prefix
     - Description
     - Example
   * - ``atomic_``
     - Denotes an atomic operation.
     - :c:func:`atomic_inc`
   * - ``device_``
     - Denotes an API relating to devices and their initialization.
     - :c:func:`device_get_binding`
   * - ``irq_``
     - Denotes an IRQ management operation.
     - :c:func:`irq_disable`
   * - ``k_``
     - Kernel-specific function.
     - :c:func:`k_malloc`
   * - ``sys_``
     - Catch-all for APIs that do not fit into the other namespaces.
     - :c:func:`sys_write32`

Kconfig symbols
===============
All Kconfig symbols are Public. The :ref:`api_lifecycle` of a Kconfig symbol is defined by the
:ref:`api_lifecycle` of a :term:`software component` to which the binding belongs. For example,
Kconfig symbols defined for regulators follow the lifecycle of the :ref:`regulator_api`.

Devicetree bindings
===================
Device tree bindings and their content are Public. The :ref:`api_lifecycle` of a binding is defined
by the :ref:`api_lifecycle` of a :term:`software component` to which the binding belongs. For
example, bindings defined for regulators follow the lifecycle of the :ref:`regulator_api`.
