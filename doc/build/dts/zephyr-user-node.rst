.. _dt-inferred-bindings:
.. _dt-zephyr-user:

The ``/zephyr,user`` node
#########################

Zephyr's devicetree scripts handle the ``/zephyr,user`` node as a special case:
you can put essentially arbitrary properties inside it and retrieve their
values without having to write a binding. It is meant as a convenient container
when only a few simple properties are needed.

.. note::

   This node is meant for sample code and user applications. It should not be
   used in the upstream Zephyr source code for device drivers, subsystems, etc.

Simple values
*************

You can store numeric or array values in ``/zephyr,user`` if you want them to
be configurable at build time via devicetree.

For example, with this devicetree overlay:

.. code-block:: devicetree

   / {
	zephyr,user {
		boolean;
		bytes = [81 82 83];
		number = <23>;
		numbers = <1>, <2>, <3>;
		string = "text";
		strings = "a", "b", "c";
	};
   };

You can get the above property values in C/C++ code like this:

.. code-block:: C

   #define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

   DT_PROP(ZEPHYR_USER_NODE, boolean) // 1
   DT_PROP(ZEPHYR_USER_NODE, bytes)   // {0x81, 0x82, 0x83}
   DT_PROP(ZEPHYR_USER_NODE, number)  // 23
   DT_PROP(ZEPHYR_USER_NODE, numbers) // {1, 2, 3}
   DT_PROP(ZEPHYR_USER_NODE, string)  // "text"
   DT_PROP(ZEPHYR_USER_NODE, strings) // {"a", "b", "c"}

Devices
*******

You can store :ref:`phandles <dt-phandles>` in ``/zephyr,user`` if you want to
be able to reconfigure which devices your application uses in simple cases
using devicetree overlays.

For example, with this devicetree overlay:

.. code-block:: devicetree

   / {
	zephyr,user {
		handle = <&gpio0>;
		handles = <&gpio0>, <&gpio1>;
        };
   };

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

GPIOs
*****

The ``/zephyr,user`` node is a convenient place to store application-specific
GPIOs that you want to be able to reconfigure with a devicetree overlay.

For example, with this devicetree overlay:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>

   / {
	zephyr,user {
		signal-gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;
        };
   };

You can convert the pin defined in ``signal-gpios`` to a ``struct
gpio_dt_spec`` in your source code, then use it like this:

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
