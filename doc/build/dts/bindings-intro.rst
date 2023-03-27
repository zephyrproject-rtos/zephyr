.. _dt-binding-compat:

Introduction to Devicetree Bindings
###################################

.. note::

   For a detailed syntax reference, see :ref:`dt-bindings-file-syntax`.

Devicetree nodes are matched to bindings using their :ref:`compatible
properties <dt-important-props>`.

During the :ref:`build_configuration_phase`, the build system tries to match
each node in the devicetree to a binding file. When this succeeds, the build
system uses the information in the binding file both when validating the node's
contents and when generating macros for the node.

.. _dt-bindings-simple-example:

A simple example
****************

Here is an example devicetree node:

.. code-block:: devicetree

   /* Node in a DTS file */
   bar-device {
   	compatible = "foo-company,bar-device";
        num-foos = <3>;
   };

Here is a minimal binding file which matches the node:

.. code-block:: yaml

   # A YAML binding matching the node

   compatible: "foo-company,bar-device"

   properties:
     num-foos:
       type: int
       required: true

The build system matches the ``bar-device`` node to its YAML binding because
the node's ``compatible`` property matches the binding's ``compatible:`` line.

What the build system does with bindings
****************************************

The build system uses bindings both to validate devicetree nodes and to convert
the devicetree's contents into the generated :ref:`devicetree_generated.h
<dt-outputs>` header file.

For example, the build system would use the above binding to check that the
required ``num-foos`` property is present in the ``bar-device`` node, and that
its value, ``<3>``, has the correct type.

The build system will then generate a macro for the ``bar-device`` node's
``num-foos`` property, which will expand to the integer literal ``3``. This
macro lets you get the value of the property in C code using the API which is
discussed later in this guide in :ref:`dt-from-c`.

For another example, the following node would cause a build error, because it
has no ``num-foos`` property, and this property is marked required in the
binding:

.. code-block:: devicetree

   bad-node {
   	compatible = "foo-company,bar-device";
   };

Other ways nodes are matched to bindings
****************************************

If a node has more than one string in its ``compatible`` property, the build
system looks for compatible bindings in the listed order and uses the first
match.

Take this node as an example:

.. code-block:: devicetree

   baz-device {
   	compatible = "foo-company,baz-device", "generic-baz-device";
   };

The ``baz-device`` node would get matched to a binding with a ``compatible:
"generic-baz-device"`` line if the build system can't find a binding with a
``compatible: "foo-company,baz-device"`` line.

Nodes without compatible properties can be matched to bindings associated with
their parent nodes. These are called "child bindings". If a node describes
hardware on a bus, like I2C or SPI, then the bus type is also taken into
account when matching nodes to bindings. (See :ref:`dt-bindings-on-bus` for
details).

See :ref:`dt-zephyr-user` for information about a special node that doesn't
require any binding.

.. _dt-where-bindings-are-located:

Where bindings are located
**************************

Binding file names usually match their ``compatible:`` lines. For example, the
above example binding would be named :file:`foo-company,bar-device.yaml` by
convention.

The build system looks for bindings in :file:`dts/bindings`
subdirectories of the following places:

- the zephyr repository
- your :ref:`application source directory <application>`
- your :ref:`board directory <board_porting_guide>`
- any :ref:`shield directories <shields>`
- any directories manually included in the :ref:`DTS_ROOT <dts_root>`
  CMake variable
- any :ref:`module <modules>` that defines a ``dts_root`` in its
  :ref:`modules_build_settings`

The build system will consider any YAML file in any of these, including in any
subdirectories, when matching nodes to bindings. A file is considered YAML if
its name ends with ``.yaml`` or ``.yml``.

.. warning::

   The binding files must be located somewhere inside the :file:`dts/bindings`
   subdirectory of the above places.

   For example, if :file:`my-app` is your application directory, then you must
   place application-specific bindings inside :file:`my-app/dts/bindings`. So
   :file:`my-app/dts/bindings/serial/my-company,my-serial-port.yaml` would be
   found, but :file:`my-app/my-company,my-serial-port.yaml` would be ignored.
