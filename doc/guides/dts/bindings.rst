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
dt-schema tools used by the Linux kernel). With one exception in
:ref:`dt-inferred-bindings` the build system uses bindings when generating
code for :ref:`dt-from-c`.

.. note::

   See the :ref:`devicetree_binding_index` for information on existing
   bindings.

.. _dt-binding-compat:

Mapping nodes to bindings
*************************

During the :ref:`build_configuration_phase`, the build system tries to map each
node in the devicetree to a binding file. When this succeeds, the build system
uses the information in the binding file when generating macros for the node.

Nodes are mapped to binding files by their :ref:`compatible properties
<dt-syntax>`. Take the following node as an example:

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

.. _dt-inferred-bindings:

Inferred bindings
*****************

Zephyr's devicetree scripts can "infer" a binding for the special
``/zephyr,user`` node based on the values observed in its properties.

This node matches a binding which is dynamically created by the build system
based on the values of its properties in the final devicetree. It does not have
a ``compatible`` property.

This node is meant for sample code and applications. The devicetree API
provides it as a convenient container when only a few simple properties are
needed, such as storing a hardware-dependent value, phandle(s), or GPIO pin.

For example, with this DTS fragment:

.. code-block:: DTS

   #include <dt-bindings/gpio/gpio.h>

   / {
	zephyr,user {
		boolean;
		bytes = [81 82 83];
		number = <23>;
		numbers = <1>, <2>, <3>;
		string = "text";
		strings = "a", "b", "c";

		handle = <&gpio0>;
		handles = <&gpio0>, <&gpio1>;
		signal-gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;
	};
   };

You can get the simple values like this:

.. code-block:: C

   #define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

   DT_PROP(ZEPHYR_USER_NODE, boolean) // 1
   DT_PROP(ZEPHYR_USER_NODE, bytes)   // {0x81, 0x82, 0x83}
   DT_PROP(ZEPHYR_USER_NODE, number)  // 23
   DT_PROP(ZEPHYR_USER_NODE, numbers) // {1, 2, 3}
   DT_PROP(ZEPHYR_USER_NODE, string)  // "text"
   DT_PROP(ZEPHYR_USER_NODE, strings) // {"a", "b", "c"}

You can convert the phandles in the ``handle`` and ``handles`` properties to
device pointers like this:

.. code-block:: C

   /*
    * Same thing as:
    *
    * ... my_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    */
   const struct device *my_device =
   	DEVICE_DT_GET(DT_PROP(ZEPHYR_USER_NODE, handle));

   #define PHANDLE_TO_DEVICE(node_id, prop, idx) \
        DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

   /*
    * Same thing as:
    *
    * ... *my_devices[] = {
    *         DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    *         DEVICE_DT_GET(DT_NODELABEL(gpio1)),
    * };
    */
   const struct device *my_devices[] = {
   	DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, handles, PHANDLE_TO_DEVICE)
   };

And you can convert the pin defined in ``signal-gpios`` to a ``struct
gpio_dt_spec``, then use it like this:

.. code-block:: C

   #include <drivers/gpio.h>

   const struct gpio_dt_spec signal =
           GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, signal_gpios);

   /* Configure the pin */
   gpio_pin_configure_dt(&signal, GPIO_OUTPUT_INACTIVE);

   /* Set the pin to its active level */
   gpio_pin_set(signal.port, signal.pin, 1);

(See :c:struct:`gpio_dt_spec`, :c:macro:`GPIO_DT_SPEC_GET`, and
:c:func:`gpio_pin_configure_dt` for details on these APIs.)
