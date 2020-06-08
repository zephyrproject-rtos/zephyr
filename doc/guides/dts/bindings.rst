.. _dt-bindings:

Devicetree bindings
###################

A devicetree on its own is only half the story for describing hardware. The
devicetree format itself is relatively unstructured, and doesn't tell the
:ref:`build system <build_overview>` which pieces of information in a
particular devicetree are useful to :ref:`device drivers <device_model_api>` or
:ref:`applications <application>`.

*Devicetree bindings* provide the other half of this information. Zephyr
devicetree bindings are YAML files in a custom format (Zephyr does not use the
dt-schema tools used by the Linux kernel). The build system uses bindings
when generating code for :ref:`dt-from-c`.

.. _dt-binding-compat:

Mapping nodes to bindings
*************************

During the :ref:`build_configuration_phase`, the build system tries to map each
node in the devicetree to a binding file. The build system only generates
macros for devicetree nodes which have matching bindings. Nodes are mapped to
bindings by their :ref:`compatible properties <dt-syntax>`. Take the following
node as an example:

.. code-block:: DTS

   bar-device {
   	compatible = "foo-company,bar-device";
   	/* ... */
   };

The build system will try to map the ``bar-device`` node to a YAML binding with
this ``compatible:`` line:

.. code-block:: yaml

   compatible: "foo-company,bar-device"

Built-in bindings are stored in :zephyr_file:`dts/bindings/`. Binding file
names usually match their ``compatible:`` lines, so the above binding would be
named :file:`foo-company,bar-device.yaml`.

If a node has more than one string in its ``compatible`` property, the build
system looks for compatible bindings in the listed order and uses the first
match. Take this node as an example:

.. code-block:: DTS

   baz-device {
   	compatible = "foo-company,baz-device", "generic-baz-device";
   };

The ``baz-device`` node would get mapped to the binding for compatible
``"generic-baz-device"`` if the build system can't find a binding for
``"foo-company,baz-device"``.

Nodes without compatible properties can be mapped to bindings associated with
their parent nodes. For an example, see the ``pwmleds`` node in the bindings
file format described below.

If a node describes hardware on a bus, like I2C or SPI, then the bus type is
also taken into account when mapping nodes to bindings. See the comments near
``on-bus:`` in the bindings syntax for details.

Bindings file syntax
********************

Below is a template that shows the Zephyr bindings file syntax. It is stored in
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
deprecation warnings. Support for the legacy bindings syntax was originally
scheduled to be dropped in the Zephyr 2.3 release, but will now be maintained
until Zephyr 2.4.
