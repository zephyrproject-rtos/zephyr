.. _dt-inferred-bindings:
.. _dt-zephyr-user:

The ``/zephyr,user`` node
#########################

Zephyr's devicetree scripts can "infer" a binding for the special
``/zephyr,user`` node based on the values observed in its properties.

This node matches a binding which is dynamically created by the build system
based on the values of its properties in the final devicetree. It does not have
a ``compatible`` property.

This node is meant for sample code and applications. The devicetree API
provides it as a convenient container when only a few simple properties are
needed, such as storing a hardware-dependent value, phandle(s), or GPIO pin.

For example, with this DTS fragment:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>

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

   #include <zephyr/drivers/gpio.h>

   #define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

   const struct gpio_dt_spec signal =
           GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, signal_gpios);

   /* Configure the pin */
   gpio_pin_configure_dt(&signal, GPIO_OUTPUT_INACTIVE);

   /* Set the pin to its active level */
   gpio_pin_set_dt(&signal, 1);

(See :c:struct:`gpio_dt_spec`, :c:macro:`GPIO_DT_SPEC_GET`, and
:c:func:`gpio_pin_configure_dt` for details on these APIs.)
