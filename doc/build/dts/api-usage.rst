.. _dt-from-c:

Devicetree access from C/C++
############################

This guide describes Zephyr's ``<zephyr/devicetree.h>`` API for reading the
devicetree from C source files. It assumes you're familiar with the concepts in
:ref:`devicetree-intro` and :ref:`dt-bindings`. See :ref:`dt-reference` for
reference material.

A note for Linux developers
***************************

Linux developers familiar with devicetree should be warned that the API
described here differs significantly from how devicetree is used on Linux.

Instead of generating a C header with all the devicetree data which is then
abstracted behind a macro API, the Linux kernel would instead read the
devicetree data structure in its binary form. The binary representation is
parsed at runtime, for example to load and initialize device drivers.

Zephyr does not work this way because the size of the devicetree binary and
associated handling code would be too large to fit comfortably on the
relatively constrained devices Zephyr supports.

.. _dt-node-identifiers:

Node identifiers
****************

To get information about a particular devicetree node, you need a *node
identifier* for it. This is a just a C macro that refers to the node.

These are the main ways to get a node identifier:

By path
   Use :c:func:`DT_PATH()` along with the node's full path in the devicetree,
   starting from the root node. This is mostly useful if you happen to know the
   exact node you're looking for.

By node label
   Use :c:func:`DT_NODELABEL()` to get a node identifier from a :ref:`node
   label <dt-node-labels>`. Node labels are often provided by SoC :file:`.dtsi`
   files to give nodes names that match the SoC datasheet, like ``i2c1``,
   ``spi2``, etc.

By alias
   Use :c:func:`DT_ALIAS()` to get a node identifier for a property of the
   special ``/aliases`` node. This is sometimes done by applications (like
   :zephyr:code-sample:`blinky`, which uses the ``led0`` alias) that need to
   refer to *some* device of a particular type ("the board's user LED") but
   don't care which one is used.

By instance number
   This is done primarily by device drivers, as instance numbers are a way to
   refer to individual nodes based on a matching compatible. Get these with
   :c:func:`DT_INST()`, but be careful doing so. See below.

By chosen node
   Use :c:func:`DT_CHOSEN()` to get a node identifier for ``/chosen`` node
   properties.

By parent/child
   Use :c:func:`DT_PARENT()` and :c:func:`DT_CHILD()` to get a node identifier
   for a parent or child node, starting from a node identifier you already have.

Two node identifiers which refer to the same node are identical and can be used
interchangeably.

.. _dt-node-main-ex:

Here's a DTS fragment for some imaginary hardware we'll return to throughout
this file for examples:

.. literalinclude:: main-example.dts
   :language: devicetree
   :start-after: start-after-here

Here are a few ways to get node identifiers for the ``i2c@40002000`` node:

- ``DT_PATH(soc, i2c_40002000)``
- ``DT_NODELABEL(i2c1)``
- ``DT_ALIAS(sensor_controller)``
- ``DT_INST(x, vnd_soc_i2c)`` for some unknown number ``x``. See the
  :c:func:`DT_INST()` documentation for details.

.. important::

   Non-alphanumeric characters like dash (``-``) and the at sign (``@``) in
   devicetree names are converted to underscores (``_``). The names in a DTS
   are also converted to lowercase.

.. _node-ids-are-not-values:

Node identifiers are not values
*******************************

There is no way to store one in a variable. You cannot write:

.. code-block:: c

   /* These will give you compiler errors: */

   void *i2c_0 = DT_INST(0, vnd_soc_i2c);
   unsigned int i2c_1 = DT_INST(1, vnd_soc_i2c);
   long my_i2c = DT_NODELABEL(i2c1);

If you want something short to save typing, use C macros:

.. code-block:: c

   /* Use something like this instead: */

   #define MY_I2C DT_NODELABEL(i2c1)

   #define INST(i) DT_INST(i, vnd_soc_i2c)
   #define I2C_0 INST(0)
   #define I2C_1 INST(1)

Property access
***************

The right API to use to read property values depends on the node and property.

- :ref:`dt-checking-property-exists`
- :ref:`simple-properties`
- :ref:`reg-properties`
- :ref:`interrupts-properties`
- :ref:`phandle-properties`

.. _dt-checking-property-exists:

Checking properties and values
==============================

You can use :c:func:`DT_NODE_HAS_PROP()` to check if a node has a property. For
the :ref:`example devicetree <dt-node-main-ex>` above:

.. code-block:: c

   DT_NODE_HAS_PROP(DT_NODELABEL(i2c1), clock_frequency)  /* expands to 1 */
   DT_NODE_HAS_PROP(DT_NODELABEL(i2c1), not_a_property)   /* expands to 0 */

.. _simple-properties:

Simple properties
=================

Use ``DT_PROP(node_id, property)`` to read basic integer, boolean, string,
numeric array, and string array properties.

For example, to read the ``clock-frequency`` property's value in the
:ref:`above example <dt-node-main-ex>`:

.. code-block:: c

   DT_PROP(DT_PATH(soc, i2c_40002000), clock_frequency)  /* This is 100000, */
   DT_PROP(DT_NODELABEL(i2c1), clock_frequency)          /* and so is this, */
   DT_PROP(DT_ALIAS(sensor_controller), clock_frequency) /* and this. */

.. important::

   The DTS property ``clock-frequency`` is spelled ``clock_frequency`` in C.
   That is, properties also need special characters converted to underscores.
   Their names are also forced to lowercase.

Properties with ``string`` and ``boolean`` types work the exact same way. The
``DT_PROP()`` macro expands to a string literal in the case of strings, and the
number 0 or 1 in the case of booleans. For example:

.. code-block:: c

   #define I2C1 DT_NODELABEL(i2c1)

   DT_PROP(I2C1, status)  /* expands to the string literal "okay" */

.. note::

   Don't use DT_NODE_HAS_PROP() for boolean properties. Use DT_PROP() instead
   as shown above. It will expand to either 0 or 1 depending on if the property
   is present or absent.

Properties with type ``array``, ``uint8-array``, and ``string-array`` work
similarly, except ``DT_PROP()`` expands to an array initializer in these cases.
Here is an example devicetree fragment:

.. code-block:: devicetree

   foo: foo@1234 {
           a = <1000 2000 3000>; /* array */
           b = [aa bb cc dd];    /* uint8-array */
           c = "bar", "baz";     /* string-array */
   };

Its properties can be accessed like this:

.. code-block:: c

   #define FOO DT_NODELABEL(foo)

   int a[] = DT_PROP(FOO, a);           /* {1000, 2000, 3000} */
   unsigned char b[] = DT_PROP(FOO, b); /* {0xaa, 0xbb, 0xcc, 0xdd} */
   char* c[] = DT_PROP(FOO, c);         /* {"foo", "bar"} */

You can use :c:func:`DT_PROP_LEN()` to get logical array lengths in number of
elements.

.. code-block:: c

   size_t a_len = DT_PROP_LEN(FOO, a); /* 3 */
   size_t b_len = DT_PROP_LEN(FOO, b); /* 4 */
   size_t c_len = DT_PROP_LEN(FOO, c); /* 2 */

``DT_PROP_LEN()`` cannot be used with the special ``reg`` or ``interrupts``
properties. These have alternative macros which are described next.

.. _reg-properties:

reg properties
==============

See :ref:`dt-important-props` for an introduction to ``reg``.

Given a node identifier ``node_id``, ``DT_NUM_REGS(node_id)`` is the
total number of register blocks in the node's ``reg`` property.

You **cannot** read register block addresses and lengths with ``DT_PROP(node,
reg)``. Instead, if a node only has one register block, use
:c:func:`DT_REG_ADDR` or :c:func:`DT_REG_SIZE`:

- ``DT_REG_ADDR(node_id)``: the given node's register block address
- ``DT_REG_SIZE(node_id)``: its size

Use :c:func:`DT_REG_ADDR_BY_IDX` or :c:func:`DT_REG_SIZE_BY_IDX` instead if the
node has multiple register blocks:

- ``DT_REG_ADDR_BY_IDX(node_id, idx)``: address of register block at index
  ``idx``
- ``DT_REG_SIZE_BY_IDX(node_id, idx)``: size of block at index ``idx``

The ``idx`` argument to these must be an integer literal or a macro that
expands to one without requiring any arithmetic. In particular, ``idx`` cannot
be a variable. This won't work:

.. code-block:: c

   /* This will cause a compiler error. */

   for (size_t i = 0; i < DT_NUM_REGS(node_id); i++) {
           size_t addr = DT_REG_ADDR_BY_IDX(node_id, i);
   }

.. _interrupts-properties:

interrupts properties
=====================

See :ref:`dt-important-props` for a brief introduction to ``interrupts``.

Given a node identifier ``node_id``, ``DT_NUM_IRQS(node_id)`` is the total
number of interrupt specifiers in the node's ``interrupts`` property.

The most general purpose API macro for accessing these is
:c:func:`DT_IRQ_BY_IDX`:

.. code-block:: c

   DT_IRQ_BY_IDX(node_id, idx, val)

Here, ``idx`` is the logical index into the ``interrupts`` array, i.e. it is
the index of an individual interrupt specifier in the property. The ``val``
argument is the name of a cell within the interrupt specifier. To use this
macro, check the bindings file for the node you are interested in to find the
``val`` names.

Most Zephyr devicetree bindings have a cell named ``irq``, which is the
interrupt number. You can use :c:func:`DT_IRQN` as a convenient way to get a
processed view of this value.

.. warning::

   Here, "processed" reflects Zephyr's devicetree :ref:`dt-scripts`, which
   change the ``irq`` number in :ref:`zephyr.dts <devicetree-in-out-files>` to
   handle hardware constraints on some SoCs and in accordance with Zephyr's
   multilevel interrupt numbering.

   This is currently not very well documented, and you'll need to read the
   scripts' source code and existing drivers for more details if you are writing
   a device driver.

.. _phandle-properties:

phandle properties
==================

.. note::

   See :ref:`dt-phandles` for a detailed guide to phandles.

Property values can refer to other nodes using the ``&another-node`` phandle
syntax introduced in :ref:`dt-writing-property-values`. Properties which
contain phandles have type ``phandle``, ``phandles``, or ``phandle-array`` in
their bindings. We'll call these "phandle properties" for short.

You can convert a phandle to a node identifier using :c:func:`DT_PHANDLE`,
:c:func:`DT_PHANDLE_BY_IDX`, or :c:func:`DT_PHANDLE_BY_NAME`, depending on the
type of property you are working with.

One common use case for phandle properties is referring to other hardware in
the tree. In this case, you usually want to convert the devicetree-level
phandle to a Zephyr driver-level :ref:`struct device <device_model_api>`.
See :ref:`dt-get-device` for ways to do that.

Another common use case is accessing specifier values in a phandle array. The
general purpose APIs for this are :c:func:`DT_PHA_BY_IDX` and :c:func:`DT_PHA`.
There are also hardware-specific shorthands like :c:func:`DT_GPIO_CTLR_BY_IDX`,
:c:func:`DT_GPIO_CTLR`,
:c:func:`DT_GPIO_PIN_BY_IDX`, :c:func:`DT_GPIO_PIN`,
:c:func:`DT_GPIO_FLAGS_BY_IDX`, and :c:func:`DT_GPIO_FLAGS`.

See :c:func:`DT_PHA_HAS_CELL_AT_IDX` and :c:func:`DT_PROP_HAS_IDX` for ways to
check if a specifier value is present in a phandle property.

.. _other-devicetree-apis:

Other APIs
**********

Here are pointers to some other available APIs.

- :c:func:`DT_CHOSEN`, :c:func:`DT_HAS_CHOSEN`: for properties
  of the special ``/chosen`` node
- :c:func:`DT_HAS_COMPAT_STATUS_OKAY`, :c:func:`DT_NODE_HAS_COMPAT`: global- and
  node-specific tests related to the ``compatible`` property
- :c:func:`DT_BUS`: get a node's bus controller, if there is one
- :c:func:`DT_ENUM_IDX`: for properties whose values are among a fixed list of
  choices
- :ref:`devicetree-flash-api`: APIs for managing fixed flash partitions.
  Also see :ref:`flash_map_api`, which wraps this in a more user-friendly API.

Device driver conveniences
**************************

Special purpose macros are available for writing device drivers, which usually
rely on :ref:`instance identifiers <dt-node-identifiers>`.

To use these, you must define ``DT_DRV_COMPAT`` to the ``compat`` value your
driver implements support for. This ``compat`` value is what you would pass to
:c:func:`DT_INST`.

If you do that, you can access the properties of individual instances of your
compatible with less typing, like this:

.. code-block:: c

   #include <zephyr/devicetree.h>

   #define DT_DRV_COMPAT my_driver_compat

   /* This is same thing as DT_INST(0, my_driver_compat): */
   DT_DRV_INST(0)

   /*
    * This is the same thing as
    * DT_PROP(DT_INST(0, my_driver_compat), clock_frequency)
    */
   DT_INST_PROP(0, clock_frequency)

See :ref:`devicetree-inst-apis` for a generic API reference.

Hardware specific APIs
**********************

Convenience macros built on top of the above APIs are also defined to help
readability for hardware specific code. See :ref:`devicetree-hw-api` for
details.

Generated macros
****************

While the :file:`zephyr/devicetree.h` API is not generated, it does rely on a
generated C header which is put into every application build directory:
:ref:`devicetree_generated.h <dt-outputs>`. This file contains macros with
devicetree data.

These macros have tricky naming conventions which the :ref:`devicetree_api` API
abstracts away. They should be considered an implementation detail, but it's
useful to understand them since they will frequently be seen in compiler error
messages.

This section contains an Augmented Backus-Naur Form grammar for these
generated macros, with examples and more details in comments. See `RFC 7405`_
(which extends `RFC 5234`_) for a syntax specification.

.. literalinclude:: macros.bnf
   :language: abnf

.. _RFC 7405: https://tools.ietf.org/html/rfc7405
.. _RFC 5234: https://tools.ietf.org/html/rfc5234
