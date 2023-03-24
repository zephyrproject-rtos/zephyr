.. _dt-bindings-file-syntax:

Devicetree bindings syntax
##########################

This page documents the syntax of Zephyr's bindings format. Zephyr bindings
files are YAML files. A :ref:`simple example <dt-bindings-simple-example>` was
given in the introduction page.

.. contents:: Contents
   :local:
   :depth: 3

Top level keys
**************

The top level of a bindings file maps keys to values. The top-level keys look
like this:

.. code-block:: yaml

   # A high level description of the device the binding applies to:
   description: |
      This is the Vendomatic company's foo-device.

      Descriptions which span multiple lines (like this) are OK,
      and are encouraged for complex bindings.

      See https://yaml-multiline.info/ for formatting help.

   # You can include definitions from other bindings using this syntax:
   include: other.yaml

   # Used to match nodes to this binding:
   compatible: "manufacturer,foo-device"

   # Used to indicate that nodes of this type generate interrupts:
   interrupt-source: <true | false>

   properties:
     # Requirements for and descriptions of the properties that this
     # binding's nodes need to satisfy go here.

   child-binding:
     # You can constrain the children of the nodes matching this binding
     # using this key.

   # If the node describes bus hardware, like an SPI bus controller
   # on an SoC, use 'bus:' to say which one, like this:
   bus: spi

   # If the node instead appears as a device on a bus, like an external
   # SPI memory chip, use 'on-bus:' to say what type of bus, like this.
   # Like 'compatible', this key also influences the way nodes match
   # bindings.
   on-bus: spi

   foo-cells:
     # "Specifier" cell names for the 'foo' domain go here; example 'foo'
     # values are 'gpio', 'pwm', and 'dma'. See below for more information.

These keys are explained in the following sections.

.. _dt-bindings-description:

Description
***********

A free-form description of node hardware goes here. You can put links to
datasheets or example nodes or properties as well.

.. _dt-bindings-compatible:

Compatible
**********

This key is used to match nodes to this binding as described in
:ref:`dt-binding-compat`. It should look like this in a binding file:

.. code-block:: YAML

   # Note the comma-separated vendor prefix and device name
   compatible: "manufacturer,device"

This devicetree node would match the above binding:

.. code-block:: devicetree

   device {
   	compatible = "manufacturer,device";
   };

Assuming no binding has ``compatible: "manufacturer,device-v2"``, it would also
match this node:

.. code-block:: devicetree

    device-2 {
        compatible = "manufacturer,device-v2", "manufacturer,device";
    };

Each node's ``compatible`` property is tried in order. The first matching
binding is used. The :ref:`on-bus: <dt-bindings-on-bus>` key can be used to
refine the search.

If more than one binding for a compatible is found, an error is raised.

The ``manufacturer`` prefix identifies the device vendor. See
:zephyr_file:`dts/bindings/vendor-prefixes.txt` for a list of accepted vendor
prefixes. The ``device`` part is usually from the datasheet.

Some bindings apply to a generic class of devices which do not have a specific
vendor. In these cases, there is no vendor prefix. One example is the
:dtcompatible:`gpio-leds` compatible which is commonly used to describe board
LEDs connected to GPIOs.

interrupt-source
****************

This key is a boolean. It is used to indicate that the hardware represented by
this compatible generates interrupts. If the binding contains
``interrupt-source: true``, then any node with this compatible must have either
an ``interrupts`` or an ``interrupts-extended`` property set.

Refer to the Devicetree Specification v0.3 section 2.4, Interrupts and
Interrupt Mapping, for more details about these properties.

.. _dt-bindings-properties:

Properties
**********

The ``properties:`` key describes properties that nodes which match the binding
contain. For example, a binding for a UART peripheral might look something like
this:

.. code-block:: YAML

   compatible: "manufacturer,serial"

   properties:
     reg:
       type: array
       description: UART peripheral MMIO register space
       required: true
     current-speed:
       type: int
       description: current baud rate
       required: true

In this example, a node with compatible ``"manufacturer,serial"`` must contain
a node named ``current-speed``. The property's value must be a single integer.
Similarly, the node must contain a ``reg`` property.

The build system uses bindings to generate C macros for devicetree properties
that appear in DTS files. You can read more about how to get property values in
source code from these macros in :ref:`dt-from-c`. Generally speaking, the
build system only generates macros for properties listed in the ``properties:``
key for the matching binding. Properties not mentioned in the binding are
generally ignored by the build system.

The one exception is that the build system will always generate macros for
standard properties, like :ref:`reg <dt-important-props>`, whose meaning is
defined by the devicetree specification. This happens regardless of whether the
node has a matching binding or not.

Property entry syntax
=====================

Property entries in ``properties:`` are written in this syntax:

.. code-block:: none

   <property name>:
     required: <true | false>
     type: <string | int | boolean | array | uint8-array | string-array |
            phandle | phandles | phandle-array | path | compound>
     deprecated: <true | false>
     default: <default>
     description: <description of the property>
     enum:
       - <item1>
       - <item2>
       ...
       - <itemN>
     const: <string | int | array | uint8-array | string-array>
     specifier-space: <space-name>

.. _dt-bindings-example-properties:

Example property definitions
============================

Here are some more examples.

.. code-block:: YAML

   properties:
       # Describes a property like 'current-speed = <115200>;'. We pretend that
       # it's obligatory for the example node and set 'required: true'.
       current-speed:
           type: int
           required: true
           description: Initial baud rate for bar-device

       # Describes an optional property like 'keys = "foo", "bar";'
       keys:
           type: string-array
           description: Keys for bar-device

       # Describes an optional property like 'maximum-speed = "full-speed";'
       # the enum specifies known values that the string property may take
       maximum-speed:
           type: string
           description: Configures USB controllers to work up to a specific speed.
           enum:
              - "low-speed"
              - "full-speed"
              - "high-speed"
              - "super-speed"

       # Describes an optional property like 'resolution = <16>;'
       # the enum specifies known values that the int property may take
       resolution:
         type: int
         enum:
          - 8
          - 16
          - 24
          - 32

       # Describes a required property '#address-cells = <1>';  the const
       # specifies that the value for the property is expected to be the value 1
       "#address-cells":
           type: int
           required: true
           const: 1

       int-with-default:
           type: int
           default: 123
           description: Value for int register, default is power-up configuration.

       array-with-default:
           type: array
           default: [1, 2, 3] # Same as 'array-with-default = <1 2 3>'

       string-with-default:
           type: string
           default: "foo"

       string-array-with-default:
           type: string-array
           default: ["foo", "bar"] # Same as 'string-array-with-default = "foo", "bar"'

       uint8-array-with-default:
           type: uint8-array
           default: [0x12, 0x34] # Same as 'uint8-array-with-default = [12 34]'

required
========

Adding ``required: true`` to a property definition will fail the build if a
node matches the binding, but does not contain the property.

The default setting is ``required: false``; that is, properties are optional by
default. Using ``required: false`` is therefore redundant and strongly
discouraged.

type
====

The type of a property constrains its values. The following types are
available. See :ref:`dt-writing-property-values` for more details about writing
values of each type in a DTS file. See :ref:`dt-phandles` for more information
about the ``phandle*`` type properties.

.. list-table::
   :header-rows: 1
   :widths: 1 3 2

   * - Type
     - Description
     - Example in DTS

   * - ``string``
     - exactly one string
     - ``status = "disabled";``

   * - ``int``
     - exactly one 32-bit value (cell)
     - ``current-speed = <115200>;``

   * - ``boolean``
     - flags that don't take a value when true, and are absent if false
     - ``hw-flow-control;``

   * - ``array``
     - zero or more 32-bit values (cells)
     - ``offsets = <0x100 0x200 0x300>;``

   * - ``uint8-array``
     - zero or more bytes, in hex ('bytestring' in the Devicetree specification)
     - ``local-mac-address = [de ad be ef 12 34];``

   * - ``string-array``
     - zero or more strings
     - ``dma-names = "tx", "rx";``

   * - ``phandle``
     - exactly one phandle
     - ``interrupt-parent = <&gic>;``

   * - ``phandles``
     - zero or more phandles
     - ``pinctrl-0 = <&usart2_tx_pd5 &usart2_rx_pd6>;``

   * - ``phandle-array``
     - a list of phandles and 32-bit cells (usually specifiers)
     - ``dmas = <&dma0 2>, <&dma0 3>;``

   * - ``path``
     - a path to a node as a phandle path reference or path string
     - ``zephyr,bt-c2h-uart = &uart0;`` or
       ``foo = "/path/to/some/node";``

   * - ``compound``
     - a catch-all for more complex types (no macros will be generated)
     - ``foo = <&label>, [01 02];``

deprecated
==========

A property with ``deprecated: true`` indicates to both the user and the tooling
that the property is meant to be phased out.

The tooling will report a warning if the devicetree includes the property that
is flagged as deprecated. (This warning is upgraded to an error in the
:ref:`twister_script` for upstream pull requests.)

The default setting is ``deprecated: false``. Using ``deprecated: false`` is
therefore redundant and strongly discouraged.

.. _dt-bindings-default:

default
=======

The optional ``default:`` setting gives a value that will be used if the
property is missing from the devicetree node.

For example, with this binding fragment:

.. code-block:: YAML

   properties:
     foo:
       type: int
       default: 3

If property ``foo`` is missing in a matching node, then the output will be as
if ``foo = <3>;`` had appeared in the DTS (except YAML data types are used for
the default value).

Note that combining ``default:`` with ``required: true`` will raise an error.

For rules related to ``default`` in upstream Zephyr bindings, see
:ref:`dt-bindings-default-rules`.

See :ref:`dt-bindings-example-properties` for examples. Putting ``default:`` on
any property type besides those used in :ref:`dt-bindings-example-properties`
will raise an error.

enum
====

The ``enum:`` line is followed by a list of values the property may contain. If
a property value in DTS is not in the ``enum:`` list in the binding, an error
is raised. See :ref:`dt-bindings-example-properties` for examples.

const
=====

This specifies a constant value the property must take. It is mainly useful for
constraining the values of common properties for a particular piece of
hardware.

.. _dt-bindings-specifier-space:

specifier-space
===============

.. warning::

   It is an abuse of this feature to use it to name properties in
   unconventional ways.

   For example, this feature is not meant for cases like naming a property
   ``my-pin``, then assigning it to the "gpio" specifier space using this
   feature. Properties which refer to GPIOs should use conventional names, i.e.
   end in ``-gpios`` or ``-gpio``.

This property, if present, manually sets the specifier space associated with a
property with type ``phandle-array``.

Normally, the specifier space is encoded implicitly in the property name. A
property named ``foos`` with type ``phandle-array`` implicitly has specifier
space ``foo``. As a special case, ``*-gpios`` properties have specifier space
"gpio", so that ``foo-gpios`` will have specifier space "gpio" rather than
"foo-gpio".

You can use ``specifier-space`` to manually provide a space if
using this convention would result in an awkward or unconventional name.

For example:

.. code-block:: YAML

   compatible: ...
   properties:
     bar:
       type: phandle-array
       specifier-space: my-custom-space

Above, the ``bar`` property's specifier space is set to "my-custom-space".

You could then use the property in a devicetree like this:

.. code-block:: DTS

   controller1: custom-controller@1000 {
           #my-custom-space-cells = <2>;
   };

   controller2: custom-controller@2000 {
           #my-custom-space-cells = <1>;
   };

   my-node {
           bar = <&controller1 10 20>, <&controller2 30>;
   };

Generally speaking, you should reserve this feature for cases where the
implicit specifier space naming convention doesn't work. One appropriate
example is an ``mboxes`` property with specifier space "mbox", not "mboxe". You
can write this property as follows:

.. code-block:: YAML

   properties:
     mboxes:
       type: phandle-array
       specifier-space: mbox

.. _dt-bindings-child:

Child-binding
*************

``child-binding`` can be used when a node has children that all share the same
properties. Each child gets the contents of ``child-binding`` as its binding,
though an explicit ``compatible = ...`` on the child node takes precedence, if
a binding is found for it.

Consider a binding for a PWM LED node like this one, where the child nodes are
required to have a ``pwms`` property:

.. code-block:: devicetree

   pwmleds {
           compatible = "pwm-leds";

           red_pwm_led {
                   pwms = <&pwm3 4 15625000>;
           };
           green_pwm_led {
                   pwms = <&pwm3 0 15625000>;
           };
           /* ... */
   };

The binding would look like this:

.. code-block:: YAML

   compatible: "pwm-leds"

   child-binding:
     description: LED that uses PWM

     properties:
       pwms:
         type: phandle-array
         required: true

``child-binding`` also works recursively. For example, this binding:

.. code-block:: YAML

   compatible: foo

   child-binding:
     child-binding:
       properties:
         my-property:
           type: int
           required: true

will apply to the ``grandchild`` node in this DTS:

.. code-block:: devicetree

   parent {
           compatible = "foo";
           child {
                   grandchild {
                           my-property = <123>;
                   };
           };
   };

.. _dt-bindings-bus:

Bus
***

If the node is a bus controller, use ``bus:`` in the binding to say what type
of bus. For example, a binding for a SPI peripheral on an SoC would look like
this:

.. code-block:: YAML

   compatible: "manufacturer,spi-peripheral"
   bus: spi
   # ...

The presence of this key in the binding informs the build system that the
children of any node matching this binding appear on this type of bus.

This in turn influences the way ``on-bus:`` is used to match bindings for the
child nodes.

For a single bus supporting multiple protocols, e.g. I3C and I2C, the ``bus:``
in the binding can have a list as value:

.. code-block:: YAML

   compatible: "manufacturer,i3c-controller"
   bus: [i3c, i2c]
   # ...

.. _dt-bindings-on-bus:

On-bus
******

If the node appears as a device on a bus, use ``on-bus:`` in the binding to say
what type of bus.

For example, a binding for an external SPI memory chip should include this line:

.. code-block:: YAML

   on-bus: spi

And a binding for an I2C based temperature sensor should include this line:

.. code-block:: YAML

   on-bus: i2c

When looking for a binding for a node, the build system checks if the binding
for the parent node contains ``bus: <bus type>``. If it does, then only
bindings with a matching ``on-bus: <bus type>`` and bindings without an
explicit ``on-bus`` are considered. Bindings with an explicit ``on-bus: <bus
type>`` are searched for first, before bindings without an explicit ``on-bus``.
The search repeats for each item in the node's ``compatible`` property, in
order.

This feature allows the same device to have different bindings depending on
what bus it appears on. For example, consider a sensor device with compatible
``manufacturer,sensor`` which can be used via either I2C or SPI.

The sensor node may therefore appear in the devicetree as a child node of
either an SPI or an I2C controller, like this:

.. code-block:: devicetree

   spi-bus@0 {
      /* ... some compatible with 'bus: spi', etc. ... */

      sensor@0 {
          compatible = "manufacturer,sensor";
          reg = <0>;
          /* ... */
      };
   };

   i2c-bus@0 {
      /* ... some compatible with 'bus: i2c', etc. ... */

      sensor@79 {
          compatible = "manufacturer,sensor";
          reg = <79>;
          /* ... */
      };
   };

You can write two separate binding files which match these individual sensor
nodes, even though they have the same compatible:

.. code-block:: YAML

   # manufacturer,sensor-spi.yaml, which matches sensor@0 on the SPI bus:
   compatible: "manufacturer,sensor"
   on-bus: spi

   # manufacturer,sensor-i2c.yaml, which matches sensor@79 on the I2C bus:
   compatible: "manufacturer,sensor"
   properties:
     uses-clock-stretching:
       type: boolean
   on-bus: i2c

Only ``sensor@79`` can have a ``use-clock-stretching`` property. The
bus-sensitive logic ignores :file:`manufacturer,sensor-i2c.yaml` when searching
for a binding for ``sensor@0``.

.. _dt-bindings-cells:

Specifier cell names (\*-cells)
*******************************

This section documents how to name the cells in a specifier within a binding.
These concepts are discussed in detail later in this guide in
:ref:`dt-phandle-arrays`.

Consider a binding for a node whose phandle may appear in a ``phandle-array``
property, like the PWM controllers ``pwm1`` and ``pwm2`` in this example:

.. code-block:: DTS

   pwm1: pwm@deadbeef {
       compatible = "foo,pwm";
       #pwm-cells = <2>;
   };

   pwm2: pwm@deadbeef {
       compatible = "foo,pwm";
       #pwm-cells = <1>;
   };

   my-node {
       pwms = <&pwm1 1 2000>, <&pwm2 3000>;
   };

The bindings for compatible ``"foo,pwm"`` and ``"bar,pwm"`` must give a name to
the cells that appear in a PWM specifier using ``pwm-cells:``, like this:

.. code-block:: YAML

   # foo,pwm.yaml
   compatible: "foo,pwm"
   ...
   pwm-cells:
     - channel
     - period

   # bar,pwm.yaml
   compatible: "bar,pwm"
   ...
   pwm-cells:
     - period

A ``*-names`` (e.g. ``pwm-names``) property can appear on the node as well,
giving a name to each entry.

This allows the cells in the specifiers to be accessed by name, e.g. using APIs
like :c:macro:`DT_PWMS_CHANNEL_BY_NAME`.

If the specifier is empty (e.g. ``#clock-cells = <0>``), then ``*-cells`` can
either be omitted (recommended) or set to an empty array. Note that an empty
array is specified as e.g. ``clock-cells: []`` in YAML.

.. _dt-bindings-include:

Include
*******

Bindings can include other files, which can be used to share common property
definitions between bindings. Use the ``include:`` key for this. Its value is
either a string or a list.

In the simplest case, you can include another file by giving its name as a
string, like this:

.. code-block:: YAML

   include: foo.yaml

If any file named :file:`foo.yaml` is found (see
:ref:`dt-where-bindings-are-located` for the search process), it will be
included into this binding.

Included files are merged into bindings with a simple recursive dictionary
merge. The build system will check that the resulting merged binding is
well-formed. It is allowed to include at any level, including ``child-binding``,
like this:

.. code-block:: YAML

   # foo.yaml will be merged with content at this level
   include: foo.yaml

   child-binding:
     # bar.yaml will be merged with content at this level
     include: bar.yaml

It is an error if a key appears with a different value in a binding and in a
file it includes, with one exception: a binding can have ``required: true`` for
a :ref:`property definition <dt-bindings-properties>` for which the included
file has ``required: false``. The ``required: true`` takes precedence, allowing
bindings to strengthen requirements from included files.

Note that weakening requirements by having ``required: false`` where the
included file has ``required: true`` is an error. This is meant to keep the
organization clean.

The file :zephyr_file:`base.yaml <dts/bindings/base/base.yaml>` contains
definitions for many common properties. When writing a new binding, it is a
good idea to check if :file:`base.yaml` already defines some of the needed
properties, and include it if it does.

Note that you can make a property defined in base.yaml obligatory like this,
taking :ref:`reg <dt-important-props>` as an example:

.. code-block:: YAML

   reg:
     required: true

This relies on the dictionary merge to fill in the other keys for ``reg``, like
``type``.

To include multiple files, you can use a list of strings:

.. code-block:: YAML

   include:
     - foo.yaml
     - bar.yaml

This includes the files :file:`foo.yaml` and :file:`bar.yaml`. (You can
write this list in a single line of YAML as ``include: [foo.yaml, bar.yaml]``.)

When including multiple files, any overlapping ``required`` keys on properties
in the included files are ORed together. This makes sure that a ``required:
true`` is always respected.

In some cases, you may want to include some property definitions from a file,
but not all of them. In this case, ``include:`` should be a list, and you can
filter out just the definitions you want by putting a mapping in the list, like
this:

.. code-block:: YAML

   include:
     - name: foo.yaml
       property-allowlist:
         - i-want-this-one
         - and-this-one
     - name: bar.yaml
       property-blocklist:
         - do-not-include-this-one
         - or-this-one

Each map element must have a ``name`` key which is the filename to include, and
may have ``property-allowlist`` and ``property-blocklist`` keys that filter
which properties are included.

You cannot have a single map element with both ``property-allowlist`` and
``property-blocklist`` keys. A map element with neither ``property-allowlist``
nor ``property-blocklist`` is valid; no additional filtering is done.

You can freely intermix strings and mappings in a single ``include:`` list:

.. code-block:: YAML

   include:
     - foo.yaml
     - name: bar.yaml
       property-blocklist:
         - do-not-include-this-one
         - or-this-one

Finally, you can filter from a child binding like this:

.. code-block:: YAML

   include:
     - name: bar.yaml
       child-binding:
         property-allowlist:
           - child-prop-to-allow

Nexus nodes and maps
********************

All ``phandle-array`` type properties support mapping through ``*-map``
properties, e.g. ``gpio-map``, as defined by the Devicetree specification.

This is used, for example, to define connector nodes for common breakout
headers, such as the ``arduino_header`` nodes that are conventionally defined
in the devicetrees for boards with Arduino compatible expansion headers.
