.. _dt-howtos:

Devicetree HOWTOs
#################

This page has step-by-step advice for getting things done with devicetree.

.. _get-devicetree-outputs:

Get your devicetree and generated header
****************************************

A board's devicetree (:ref:`BOARD.dts <devicetree-in-out-files>`) pulls in
common node definitions via ``#include`` preprocessor directives. This at least
includes the SoC's ``.dtsi``. One way to figure out the devicetree's contents
is by opening these files, e.g. by looking in
``dts/<ARCH>/<vendor>/<soc>.dtsi``, but this can be time consuming.

Furthermore, you might want to see the actual generated header file. You might
also be working with a board definition outside of the zephyr repository,
making it unclear where ``BOARD.dts`` is in the first place.

Luckily, there is an easy way to do both: build your application.

For example, using west and the :ref:`qemu_cortex_m3` board to build
:ref:`hello_world`, forcing CMake to re-run:

.. code-block:: sh

   west build -b qemu_cortex_m3 -s samples/hello_world --cmake

The build system prints the output file locations:

.. code-block:: none

   -- Found BOARD.dts: .../zephyr/boards/arm/qemu_cortex_m3/qemu_cortex_m3.dts
   -- Generated zephyr.dts: .../zephyr/build/zephyr/zephyr.dts
   -- Generated devicetree_unfixed.h: .../zephyr/build/zephyr/include/generated/devicetree_unfixed.h

Change ``qemu_cortex_m3`` to the board you are using, of course.

.. _dt-get-device:

Get a struct device from a devicetree node
******************************************

When writing Zephyr applications, you'll often want to get a driver-level
:ref:`struct device <device_model_api>` corresponding to a devicetree node.

For example, with this devicetree fragment, you might want the struct device
for ``serial@40002000``:

.. code-block:: DTS

   / {
           soc {
                   serial0: serial@40002000 {
                           status = "okay";
                           current-speed = <115200>;
                           /* ... */
                   };
           };

           aliases {
                   my-serial = &serial0;
           };

           chosen {
                   zephyr,console = &serial0;
           };
   };

Start by making a :ref:`node identifier <dt-node-identifiers>` for the device
you are interested in. There are different ways to do this; pick whichever one
works best for your requirements. Here are some examples:

.. code-block:: c

   /* Option 1: by node label */
   #define MY_SERIAL DT_NODELABEL(serial0)

   /* Option 2: by alias */
   #define MY_SERIAL DT_ALIAS(my_serial)

   /* Option 3: by chosen node */
   #define MY_SERIAL DT_CHOSEN(zephyr_console)

   /* Option 4: by path */
   #define MY_SERIAL DT_PATH(soc, serial_40002000)

Once you have a node identifier, get the ``struct device`` by combining
:c:func:`DT_LABEL` with :c:func:`device_get_binding`:

.. code-block:: c

   struct device *uart_dev = device_get_binding(DT_LABEL(MY_SERIAL));

You can then use ``uart_dev`` with :ref:`uart_api` API functions like
:c:func:`uart_configure`. Similar code will work for other device types; just
make sure you use the correct API for the device.

There's no need to override the ``label`` property to something else: just make
a node identifier and pass it to ``DT_LABEL`` to get the right string to pass
to ``device_get_binding()``.

If you're having trouble, see :ref:`dt-trouble`. The first thing to check is
that the node is enabled (``status = "okay"``) and has a matching binding, like
this:

.. code-block:: c

   #define MY_SERIAL DT_NODELABEL(my_serial)

   #if DT_HAS_NODE(MY_SERIAL)
   struct device *uart_dev = device_get_binding(DT_LABEL(MY_SERIAL));
   #else
   #error "Node is disabled or has no matching binding"
   #endif

If you see the ``#error`` output, something is wrong with either your
devicetree or bindings.

.. _dts-find-binding:

Find a devicetree binding
*************************

Devicetree binding YAML files document what you can do with the nodes they
describe, so it's critical to be able to find them for the nodes you are using.

If you don't have them already, :ref:`get-devicetree-outputs`. To find a node's
binding, open the generated header file, which starts with a list of nodes in a
block comment:

.. code-block:: c

   /*
    * [...]
    * Nodes in dependency order (ordinal and path):
    *   0   /
    *   1   /aliases
    *   2   /chosen
    *   3   /flash@0
    *   4   /memory@20000000
    *          (etc.)
    * [...]
    */

Make note of the path to the node you want to find, like ``/flash@0``. Search
for the node's output in the file, which starts with something like this if the
node has a matching binding:

.. code-block:: c

   /*
    * Devicetree node:
    *   /flash@0
    *
    * Binding (compatible = soc-nv-flash):
    *   $ZEPHYR_BASE/dts/bindings/mtd/soc-nv-flash.yaml
    * [...]
    */

See :ref:`missing-dt-binding` for troubleshooting.

.. _set-devicetree-overlays:

Set devicetree overlays
***********************

Devicetree overlays are explained in :ref:`devicetree-intro`. The CMake
variable :makevar:`DTC_OVERLAY_FILE` contains a space- or colon-separated list
of overlays. If :makevar:`DTC_OVERLAY_FILE` specifies multiple files, they are
included in that order by the C preprocessor.

Here are some ways to set it:

1. on the cmake build command line
   (``-DDTC_OVERLAY_FILE=file1.overlay;file2.overlay``)
#. with the CMake ``set()`` command in the application ``CMakeLists.txt``,
   before including zephyr's :file:`boilerplate.cmake` file
#. using a ``DTC_OVERLAY_FILE`` environment variable (deprecated)
#. create a ``boards/<BOARD>.overlay`` file in the application
   folder, for the current board
#. create a ``<BOARD>.overlay`` file in the application folder

Here is an example :ref:`using west build <west-building-dtc-overlay-file>`.
However you set the value, it is saved in the CMake cache between builds.

The :ref:`build system <build_overview>` prints all the devicetree overlays it
finds in the configuration phase, like this:

.. code-block:: none

   -- Found devicetree overlay: .../some/file.overlay

.. _use-dt-overlays:

Use devicetree overlays
***********************

See :ref:`set-devicetree-overlays` for how to add an overlay to the build.

Overlays can override node property values in multiple ways.
For example, if your BOARD.dts contains this node:

.. code-block:: DTS

   / {
           soc {
                   serial0: serial@40002000 {
                           status = "okay";
                           current-speed = <115200>;
                           /* ... */
                   };
           };
   };

These are equivalent ways to override the ``current-speed`` value in an
overlay:

.. code-block:: none

   /* Option 1 */
   &serial0 {
   	current-speed = <9600>;
   };

   /* Option 2 */
   &{/soc/serial@40002000} {
   	current-speed = <9600>;
   };

We'll use the ``&serial0`` style for the rest of these examples.

You can add aliases to your devicetree using overlays: an alias is just a
property of the ``/aliases`` node. For example:

.. code-block:: none

   / {
   	aliases {
   		my-serial = &serial0;
   	};
   };

Chosen nodes work the same way. For example:

.. code-block:: none

   / {
   	chosen {
   		zephyr,console = &serial0;
   	};
   };

To delete a property (this is how you override a true boolean property to a
false value):

.. code-block:: none

   /* Option 1 */
   &serial0 {
   	/delete-property/ some-unwanted-property;
   };

You can add subnodes using overlays. For example, to configure a SPI or I2C
child device on an existing bus node, do something like this:

.. code-block:: none

   /* SPI device example */
   &spi1 {
	my_spi_device: temp-sensor@0 {
		compatible = "...";
		label = "TEMP_SENSOR_0";
		/* reg is the chip select number, if needed;
		 * If present, it must match the node's unit address. */
		reg = <0>;

		/* Configure other SPI device properties as needed.
		 * Find your device's DT binding for details. */
		spi-max-frequency = <4000000>;
	};
   };

   /* I2C device example */
   &i2c2 {
	my_i2c_device: touchscreen@76 {
		compatible = "...";
		label = "TOUCHSCREEN";
		/* reg is the I2C device address.
		 * It must match the node's unit address. */
		reg = <76>;

		/* Configure other I2C device properties as needed.
		 * Find your device's DT binding for details. */
	};
   };

Other bus devices can be configured similarly:

- create the device as a subnode of the parent bus
- set its properties according to its binding

Assuming you have a suitable device driver associated with the
``my_spi_device`` and ``my_i2c_device`` compatibles, you should now be able to
enable the driver via Kconfig and :ref:`get the struct device <dt-get-device>`
for your newly added bus node, then use it with that driver API.

.. _dt-driver-howto:

Create struct devices in a driver
*********************************

If you're writing a device driver, it should be devicetree aware so that
applications can configure it and access devices as described above. In short,
you must create a ``struct device`` for every enabled instance of the
compatible that the device driver supports, and set each device's name to the
``DT_LABEL()`` of its devicetree node.

The :file:`devicetree.h` API has helpers for writing device drivers based on
:ref:`DT_INST node identifiers <dt-node-identifiers>` for each of the possible
instance numbers on your SoC.

Assuming you're using instances, start by defining ``DT_DRV_COMPAT`` at the top
of the file to the lowercase-and-underscores version of the :ref:`compatible
<dt-important-props>` that the device driver is handling. For example, if your
driver is handling nodes with compatible ``"vnd,my-device"``, you should put
this at the top of your driver:

.. code-block:: c

   #define DT_DRV_COMPAT vnd_my_device

.. important::

   The DT_DRV_COMPAT macro should have neither quotes nor special characters.
   Remove quotes and convert special characters to underscores.

The typical pattern after that is to define the API functions, then define a
macro which creates the device by instance number, and then call it for each
enabled instance. Currently, this looks like this:

.. code-block:: c

   #include <drivers/some_api.h>

   #include <devicetree.h>
   #define DT_DRV_COMPAT vnd_my_device

   /*
    * Define RAM and ROM structures:
    */

   struct my_dev_data {
	/* per-device values to store in RAM */
   };

   struct my_dev_cfg {
	u32_t freq; /* Just an example: clock frequency in Hz */
	/* other device configuration to store in ROM */
   };

   /*
    * Implement some_api.h callbacks:
    */

   struct some_api my_api_funcs = { /* ... */ };

   /*
    * Now use DT_INST APIs to create a struct device for each enabled node:
    */

   #define CREATE_MY_DEVICE(inst)                                       \
	static struct my_dev_data my_dev_data_##inst = {                \
		/* initialize RAM values as needed */                   \
	};                                                              \
	static const struct my_dev_cfg my_dev_cfg_##inst = {            \
		/* initialize ROM values, usually from devicetree */    \
		.freq = DT_INST_PROP(inst, clock_frequency),            \
		/* ... */                                               \
	};                                                              \
	DEVICE_AND_API_INIT(my_dev_##inst,                              \
			    DT_INST_LABEL(inst),                        \
			    my_dev_init_function,                       \
			    &my_dev_data_##inst,                        \
			    &my_dev_cfg_##inst,                         \
			    MY_DEV_INIT_LEVEL, MY_DEV_INIT_PRIORITY,    \
			    &my_api_funcs)

   #if DT_HAS_NODE(DT_DRV_INST(0))
   CREATE_MY_DEVICE(0);
   #endif

   #if DT_HAS_NODE(DT_DRV_INST(1))
   CREATE_MY_DEVICE(1);
   #endif

   /* And so on, for all "possible" instance numbers you need to support. */

Notice the use of :c:func:`DT_INST_PROP` and :c:func:`DT_DRV_INST`. These are
helpers which rely on ``DT_DRV_COMPAT`` to choose devicetree nodes of a chosen
compatible at a given index.

As shown above, the driver uses additional information from
:file:`devicetree.h` to create :ref:`struct device <device_struct>` instances
than just the node label. Devicetree property values used to configure the
device at boot time are stored in ROM in the value pointed to by a
``device->config->config_info`` field. This allows users to configure your
driver using overlays.

The Zephyr convention is to name each ``struct device`` using its devicetree
node's ``label`` property using ``DT_INST_LABEL()``. This allows applications
to :ref:`dt-get-device`.

.. _dt-trouble:

Troubleshoot devicetree issues
******************************

Here are some tips for fixing misbehaving devicetree code.

Try again with a pristine build directory
=========================================

See :ref:`west-building-pristine` for examples, or just delete the build
directory completely and retry.

This is general advice which is especially applicable to debugging devicetree
issues, because the outputs are created at CMake configuration time, and are
not always regenerated when one of their inputs changes.

Make sure <devicetree.h> is included
====================================

Unlike Kconfig symbols, the :file:`devicetree.h` header must be included
explicitly.

Many Zephyr header files rely on information from devicetree, so including some
other API may transitively include :file:`devicetree.h`, but that's not
guaranteed.

.. _dt-use-the-right-names:

Make sure you're using the right names
======================================

Remember that:

- In C/C++, devicetree names must be lowercased and special characters must be
  converted to underscores. Zephyr's generated devicetree header has DTS names
  converted in this way into the C tokens used by the preprocessor-based
  ``<devicetree.h>`` API.
- In overlays, use devicetree node and property names the same way they
  would appear in any DTS file. Zephyr overlays are just DTS fragments.

For example, if you're trying to **get** the ``clock-frequency`` property of a
node with path ``/soc/i2c@12340000`` in a C/C++ file:

.. code-block:: c

   /*
    * foo.c: lowercase-and-underscores names
    */

   /* Don't do this: */
   #define MY_CLOCK_FREQ DT_PROP(DT_PATH(soc, i2c@1234000), clock-frequency)
   /*                                           ^               ^
    *                                        @ should be _     - should be _  */

   /* Do this instead: */
   #define MY_CLOCK_FREQ DT_PROP(DT_PATH(soc, i2c_1234000), clock_frequency)
   /*                                           ^               ^           */

And if you're trying to **set** that property in a devicetree overlay:

.. code-block:: DTS

   /*
    * foo.overlay: DTS names with special characters, etc.
    */

   /* Don't do this; you'll get devicetree errors. */
   &{/soc/i2c_12340000/} {
   	clock_frequency = <115200>;
   };

   /* Do this instead. Overlays are just DTS fragments. */
   &{/soc/i2c@12340000/} {
   	clock-frequency = <115200>;
   };

Validate properties
===================

If you're getting a compile error reading a node property, remember
:ref:`not-all-dt-nodes`, then check your node identifier and property.
For example, if you get a build error on a line that looks like this:

.. code-block:: c

   int baud_rate = DT_PROP(DT_NODELABEL(my_serial), current_speed);

Try checking the node by adding this to the file and recompiling:

.. code-block:: c

   #if DT_HAS_NODE(DT_NODELABEL(my_serial)) == 0
   #error "whoops"
   #endif

If you see the "whoops" error message when you rebuild, the node identifier
isn't referring to a valid node. :ref:`get-devicetree-outputs` and debug from
there.

Some hints:

- :ref:`dt-use-the-right-names`
- Is the node's ``status`` property set to ``"okay"``? If not, it's disabled.
  The generated header will tell you if the node is disabled.
- Does the node have a matching binding? The generated header also tells you
  this information for each node; see :ref:`dts-find-binding`.
- Does the property exist? See :ref:`dt-checking-property-exists`.

If you're sure the property is defined but ``DT_NODE_HAS_PROP()`` disagrees,
check for a missing binding.

.. _missing-dt-binding:

Check for missing bindings
==========================

If the build fails to :ref:`dts-find-binding` for a node, then either the
node's ``compatible`` property is missing, or its value has no matching
binding. If the property is set, check for typos in its name. In a devicetree
source file, ``compatible`` should look like ``"vnd,some-device"`` --
:ref:`dt-use-the-right-names`.

If your binding file is not under :file:`zephyr/dts`, you may need to set
:ref:`DTS_ROOT <dts_root>`.

Errors with DT_INST_() APIs
===========================

If you're using an API like :c:func:`DT_INST_PROP`, you must define
``DT_DRV_COMPAT`` to the lowercase-and-underscores version of the compatible
you are interested in. See :ref:`dt-driver-howto`.
