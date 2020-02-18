.. _dt-bindings:

Devicetree bindings
###################

A :ref:`devicetree` on its own is only half the story for describing the
available hardware devices. The tree itself doesn't tell the build system
which pieces of information are useful to device drivers, or what
:ref:`C macros <dt-macros>` to generate from the devicetree itself.

*Devicetree bindings* provide the other half of this information. Zephyr
devicetree bindings are files in YAML format. The build system only generates
macros for devicetree nodes which have bindings.

Nodes are mapped to bindings via their ``compatible`` string(s). Take
the following node as an example:

.. code-block:: none

   bar-device {
   	compatible = "foo-company,bar-device";
   	...
   };

This node would get mapped to a binding with this in it:

.. code-block:: yaml

   compatible: "foo-company,bar-device"

You might also run across this legacy syntax, which works the same way:

.. code-block:: yaml

   ...

   properties:
       compatible:
           constraint: "foo-company,bar-device"

       ...

Bindings are stored in :zephyr_file:`dts/bindings/`. The filename usually
matches the ``compatible`` string.

If a node has more than one ``compatible`` string, then the first binding found
is used, going from the first string to the last. For example, a node with
``compatible = "foo-company,bar-device", "generic-bar-device"`` would get
mapped to the binding for ``generic-bar-device`` if there is no binding for
``foo-company,bar-device``.

If a node appears on a bus (e.g. I2C or SPI), then the bus type is also taken
into account when mapping nodes to bindings. See the description of ``parent``
and ``child`` in the template below.

Bindings template
*****************

Below is a template that shows the format of binding files, stored in
:zephyr_file:`dts/binding-template.yaml`.

.. literalinclude:: ../../../dts/binding-template.yaml
   :language: yaml

.. _legacy_binding_syntax:

Legacy bindings syntax
**********************

The bindings syntax described above was introduced in the Zephyr 2.1 release.
This section describes how to update bindings written in the legacy syntax,
starting with this example written in the legacy syntax.

.. code-block:: yaml

   title: ...
   description: ...

   inherits:
       !include foo.yaml

   parent:
       bus: spi

   parent-bus: spi

   properties:
       compatible:
           constraint: "company,device"
           type: string-array

       frequency:
           type: int
           category: optional

   sub-node:
       properties:
           child-prop:
               type: int
               category: required

   # Assume this is a binding for an interrupt controller
   "#cells":
       - irq
       - priority
       - flags

This should now be written like this:

.. code-block:: yaml

   description: ...

   compatible: "company,device"

   include: foo.yaml

   bus: spi

   properties:
       frequency:
           type: int
           required: false

   child-binding:
       description: ...

       properties:
           child-prop:
               type: int
               required: true

   interrupt-cells:
       - irq
       - priority
       - cells

The legacy syntax is still supported for backwards compatibility, but generates
deprecation warnings. Support will be dropped in the Zephyr 2.3 release.
