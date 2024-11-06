.. _dt-howtos:

Devicetree HOWTOs
#################

This page has step-by-step advice for getting things done with devicetree.

.. tip:: See :ref:`dt-trouble` for troubleshooting advice.

.. _get-devicetree-outputs:

Get your devicetree and generated header
****************************************

A board's devicetree (:ref:`BOARD.dts <devicetree-in-out-files>`) pulls in
common node definitions via ``#include`` preprocessor directives. This at least
includes the SoC's ``.dtsi``. One way to figure out the devicetree's contents
is by opening these files, e.g. by looking in
``dts/<ARCH>/<vendor>/<soc>.dtsi``, but this can be time consuming.

If you just want to see the "final" devicetree for your board, build an
application and open the :file:`zephyr.dts` file in the build directory.

.. tip::

   You can build :zephyr:code-sample:`hello_world` to see the "base" devicetree for your board
   without any additional changes from :ref:`overlay files <dt-input-files>`.

For example, using the :ref:`qemu_cortex_m3` board to build :zephyr:code-sample:`hello_world`:

.. code-block:: sh

   # --cmake-only here just forces CMake to run, skipping the
   # build process to save time.
   west build -b qemu_cortex_m3 samples/hello_world --cmake-only

You can change ``qemu_cortex_m3`` to match your board.

CMake prints the input and output file locations like this:

.. code-block:: none

   -- Found BOARD.dts: .../zephyr/boards/arm/qemu_cortex_m3/qemu_cortex_m3.dts
   -- Generated zephyr.dts: .../zephyr/build/zephyr/zephyr.dts
   -- Generated devicetree_generated.h: .../zephyr/build/zephyr/include/generated/zephyr/devicetree_generated.h

The :file:`zephyr.dts` file is the final devicetree in DTS format.

The :file:`devicetree_generated.h` file is the corresponding generated header.

See :ref:`devicetree-in-out-files` for details about these files.

.. _dt-get-device:

Get a struct device from a devicetree node
******************************************

When writing Zephyr applications, you'll often want to get a driver-level
:ref:`struct device <device_model_api>` corresponding to a devicetree node.

For example, with this devicetree fragment, you might want the struct device
for ``serial@40002000``:

.. code-block:: devicetree

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

Once you have a node identifier there are two ways to proceed. One way to get a
device is to use :c:macro:`DEVICE_DT_GET`:

.. code-block:: c

   const struct device *const uart_dev = DEVICE_DT_GET(MY_SERIAL);

   if (!device_is_ready(uart_dev)) {
           /* Not ready, do not use */
           return -ENODEV;
   }

There are variants of :c:macro:`DEVICE_DT_GET` such as
:c:macro:`DEVICE_DT_GET_OR_NULL`, :c:macro:`DEVICE_DT_GET_ONE` or
:c:macro:`DEVICE_DT_GET_ANY`. This idiom fetches the device pointer at
build-time, which means there is no runtime penalty. This method is useful if
you want to store the device pointer as configuration data. But because the
device may not be initialized, or may have failed to initialize, you must verify
that the device is ready to be used before passing it to any API functions.
(This check is done for you by :c:func:`device_get_binding`.)

In some situations the device cannot be known at build-time, e.g., if it depends
on user input like in a shell application. In this case you can get the
``struct device`` by combining :c:func:`device_get_binding` with the device
name:

.. code-block:: c

   const char *dev_name = /* TODO: insert device name from user */;
   const struct device *uart_dev = device_get_binding(dev_name);

You can then use ``uart_dev`` with :ref:`uart_api` API functions like
:c:func:`uart_configure`. Similar code will work for other device types; just
make sure you use the correct API for the device.

If you're having trouble, see :ref:`dt-trouble`. The first thing to check is
that the node has ``status = "okay"``, like this:

.. code-block:: c

   #define MY_SERIAL DT_NODELABEL(my_serial)

   #if DT_NODE_HAS_STATUS(MY_SERIAL, okay)
   const struct device *const uart_dev = DEVICE_DT_GET(MY_SERIAL);
   #else
   #error "Node is disabled"
   #endif

If you see the ``#error`` output, make sure to enable the node in your
devicetree. In some situations your code will compile but it will fail to link
with a message similar to:

.. code-block:: none

   ...undefined reference to `__device_dts_ord_N'
   collect2: error: ld returned 1 exit status

This likely means there's a Kconfig issue preventing the device driver from
being built, resulting in a reference that does not exist. If your code compiles
successfully, the last thing to check is if the device is ready, like this:

.. code-block:: c

   if (!device_is_ready(uart_dev)) {
        printk("Device not ready\n");
   }

If you find that the device is not ready, it likely means that the device's
initialization function failed. Enabling logging or debugging driver code may
help in such situations. Note that you can also use :c:func:`device_get_binding`
to obtain a reference at runtime. If it returns ``NULL`` it can either mean that
device's driver failed to initialize or that it does not exist.

.. _dts-find-binding:

Find a devicetree binding
*************************

:ref:`dt-bindings` are YAML files which declare what you can do with the nodes
they describe, so it's critical to be able to find them for the nodes you are
using.

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
variable :makevar:`DTC_OVERLAY_FILE` contains a space- or semicolon-separated
list of overlay files to use. If :makevar:`DTC_OVERLAY_FILE` specifies multiple
files, they are included in that order by the C preprocessor.  A file in a
Zephyr module can be referred to by escaping the Zephyr module dir variable
like ``\${ZEPHYR_<module>_MODULE_DIR}/<path-to>/dts.overlay``
when setting the DTC_OVERLAY_FILE variable.

You can set :makevar:`DTC_OVERLAY_FILE` to contain exactly the files you want
to use. Here is an :ref:`example <west-building-dtc-overlay-file>` using
``west build``.

If you don't set :makevar:`DTC_OVERLAY_FILE`, the build system will follow
these steps, looking for files in your application configuration directory to
use as devicetree overlays:

#. If the file :file:`socs/<SOC>_<BOARD_QUALIFIERS>.overlay` exists, it will be used.
#. If the file :file:`boards/<BOARD>.overlay` exists, it will be used in addition to the above.
#. If the current board has :ref:`multiple revisions <porting_board_revisions>`
   and :file:`boards/<BOARD>_<revision>.overlay` exists, it will be used in addition to the above.
#. If one or more files have been found in the previous steps, the build system
   stops looking and just uses those files.
#. Otherwise, if :file:`<BOARD>.overlay` exists, it will be used, and the build
   system will stop looking for more files.
#. Otherwise, if :file:`app.overlay` exists, it will be used.

Extra devicetree overlays may be provided using ``EXTRA_DTC_OVERLAY_FILE`` which
will still allow the build system to automatically use devicetree overlays
described in the above steps.

The build system appends overlays specified in ``EXTRA_DTC_OVERLAY_FILE``
to the overlays in ``DTC_OVERLAY_FILE`` when processing devicetree overlays.
This means that changes made via ``EXTRA_DTC_OVERLAY_FILE`` have higher
precedence than those made via ``DTC_OVERLAY_FILE``.

All configuration files will be taken from the application's configuration
directory except for files with an absolute path that are given with the
``DTC_OVERLAY_FILE`` or ``EXTRA_DTC_OVERLAY_FILE`` argument.

See :ref:`Application Configuration Directory <application-configuration-directory>`
on how the application configuration directory is defined.

Using :ref:`shields` will also add devicetree overlay files.

The :makevar:`DTC_OVERLAY_FILE` value is stored in the CMake cache and used
in successive builds.

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

.. code-block:: devicetree

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

.. Disable syntax highlighting as this construct does not seem supported by pygments
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

.. code-block:: devicetree

   / {
   	aliases {
   		my-serial = &serial0;
   	};
   };

Chosen nodes work the same way. For example:

.. code-block:: devicetree

   / {
   	chosen {
   		zephyr,console = &serial0;
   	};
   };

To delete a property (in addition to deleting properties in general, this is
how to set a boolean property to false if it's true in BOARD.dts):

.. code-block:: devicetree

   &serial0 {
   	/delete-property/ some-unwanted-property;
   };

You can add subnodes using overlays. For example, to configure a SPI or I2C
child device on an existing bus node, do something like this:

.. code-block:: devicetree

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

.. _dt-create-devices:

Write device drivers using devicetree APIs
******************************************

"Devicetree-aware" :ref:`device drivers <device_model_api>` should create a
``struct device`` for each ``status = "okay"`` devicetree node with a
particular :ref:`compatible <dt-important-props>` (or related set of
compatibles) supported by the driver.

Writing a devicetree-aware driver begins by defining a :ref:`devicetree binding
<dt-bindings>` for the devices supported by the driver. Use existing bindings
from similar drivers as a starting point. A skeletal binding to get started
needs nothing more than this:

.. code-block:: yaml

   description: <Human-readable description of your binding>
   compatible: "foo-company,bar-device"
   include: base.yaml

See :ref:`dts-find-binding` for more advice on locating existing bindings.

After writing your binding, your driver C file can then use the devicetree API
to find ``status = "okay"`` nodes with the desired compatible, and instantiate
a ``struct device`` for each one. There are two options for instantiating each
``struct device``: using instance numbers, and using node labels.

In either case:

- Each ``struct device``\ 's name should be set to its devicetree node's
  ``label`` property. This allows the driver's users to :ref:`dt-get-device` in
  the usual way.

- Each device's initial configuration should use values from devicetree
  properties whenever practical. This allows users to configure the driver
  using :ref:`devicetree overlays <use-dt-overlays>`.

Examples for how to do this follow. They assume you've already implemented the
device-specific configuration and data structures and API functions, like this:

.. code-block:: c

   /* my_driver.c */
   #include <zephyr/drivers/some_api.h>

   /* Define data (RAM) and configuration (ROM) structures: */
   struct my_dev_data {
   	/* per-device values to store in RAM */
   };
   struct my_dev_cfg {
   	uint32_t freq; /* Just an example: initial clock frequency in Hz */
   	/* other configuration to store in ROM */
   };

   /* Implement driver API functions (drivers/some_api.h callbacks): */
   static int my_driver_api_func1(const struct device *dev, uint32_t *foo) { /* ... */ }
   static int my_driver_api_func2(const struct device *dev, uint64_t bar) { /* ... */ }
   static struct some_api my_api_funcs = {
   	.func1 = my_driver_api_func1,
   	.func2 = my_driver_api_func2,
   };

.. _dt-create-devices-inst:

Option 1: create devices using instance numbers
===============================================

Use this option, which uses :ref:`devicetree-inst-apis`, if possible. However,
they only work when devicetree nodes for your driver's ``compatible`` are all
equivalent, and you do not need to be able to distinguish between them.

To use instance-based APIs, begin by defining ``DT_DRV_COMPAT`` to the
lowercase-and-underscores version of the compatible that the device driver
supports. For example, if your driver's compatible is ``"vnd,my-device"`` in
devicetree, you would define ``DT_DRV_COMPAT`` to ``vnd_my_device`` in your
driver C file:

.. code-block:: c

   /*
    * Put this near the top of the file. After the includes is a good place.
    * (Note that you can therefore run "git grep DT_DRV_COMPAT drivers" in
    * the zephyr Git repository to look for example drivers using this style).
    */
   #define DT_DRV_COMPAT vnd_my_device

.. important::

   As shown, the DT_DRV_COMPAT macro should have neither quotes nor special
   characters. Remove quotes and convert special characters to underscores
   when creating ``DT_DRV_COMPAT`` from the compatible property.

Finally, define an instantiation macro, which creates each ``struct device``
using instance numbers. Do this after defining ``my_api_funcs``.

.. code-block:: c

   /*
    * This instantiation macro is named "CREATE_MY_DEVICE".
    * Its "inst" argument is an arbitrary instance number.
    *
    * Put this near the end of the file, e.g. after defining "my_api_funcs".
    */
   #define CREATE_MY_DEVICE(inst)					\
   	static struct my_dev_data my_data_##inst = {			\
   		/* initialize RAM values as needed, e.g.: */		\
   		.freq = DT_INST_PROP(inst, clock_frequency),		\
   	};								\
   	static const struct my_dev_cfg my_cfg_##inst = {		\
   		/* initialize ROM values as needed. */			\
   	};								\
   	DEVICE_DT_INST_DEFINE(inst,					\
   			      my_dev_init_function,			\
			      NULL,             			\
   			      &my_data_##inst,				\
   			      &my_cfg_##inst,				\
   			      MY_DEV_INIT_LEVEL, MY_DEV_INIT_PRIORITY,	\
   			      &my_api_funcs);

Notice the use of APIs like :c:macro:`DT_INST_PROP` and
:c:macro:`DEVICE_DT_INST_DEFINE` to access devicetree node data. These
APIs retrieve data from the devicetree for instance number ``inst`` of
the node with compatible determined by ``DT_DRV_COMPAT``.

Finally, pass the instantiation macro to :c:macro:`DT_INST_FOREACH_STATUS_OKAY`:

.. code-block:: c

   /* Call the device creation macro for each instance: */
   DT_INST_FOREACH_STATUS_OKAY(CREATE_MY_DEVICE)

``DT_INST_FOREACH_STATUS_OKAY`` expands to code which calls
``CREATE_MY_DEVICE`` once for each enabled node with the compatible determined
by ``DT_DRV_COMPAT``. It does not append a semicolon to the end of the
expansion of ``CREATE_MY_DEVICE``, so the macro's expansion must end in a
semicolon or function definition to support multiple devices.

Option 2: create devices using node labels
==========================================

Some device drivers cannot use instance numbers. One example is an SoC
peripheral driver which relies on vendor HAL APIs specialized for individual IP
blocks to implement Zephyr driver callbacks. Cases like this should use
:c:macro:`DT_NODELABEL` to refer to individual nodes in the devicetree
representing the supported peripherals on the SoC. The devicetree.h
:ref:`devicetree-generic-apis` can then be used to access node data.

For this to work, your :ref:`SoC's dtsi file <dt-input-files>` must define node
labels like ``mydevice0``, ``mydevice1``, etc. appropriately for the IP blocks
your driver supports. The resulting devicetree usually looks something like
this:

.. code-block:: devicetree

   / {
           soc {
                   mydevice0: dev@0 {
                           compatible = "vnd,my-device";
                   };
                   mydevice1: dev@1 {
                           compatible = "vnd,my-device";
                   };
           };
   };

The driver can use the ``mydevice0`` and ``mydevice1`` node labels in the
devicetree to operate on specific device nodes:

.. code-block:: c

   /*
    * This is a convenience macro for creating a node identifier for
    * the relevant devices. An example use is MYDEV(0) to refer to
    * the node with label "mydevice0".
    */
   #define MYDEV(idx) DT_NODELABEL(mydevice ## idx)

   /*
    * Define your instantiation macro; "idx" is a number like 0 for mydevice0
    * or 1 for mydevice1. It uses MYDEV() to create the node label from the
    * index.
    */
   #define CREATE_MY_DEVICE(idx)					\
	static struct my_dev_data my_data_##idx = {			\
		/* initialize RAM values as needed, e.g.: */		\
		.freq = DT_PROP(MYDEV(idx), clock_frequency),		\
	};								\
	static const struct my_dev_cfg my_cfg_##idx = { /* ... */ };	\
   	DEVICE_DT_DEFINE(MYDEV(idx),					\
   			my_dev_init_function,				\
			NULL,           				\
			&my_data_##idx,					\
			&my_cfg_##idx,					\
			MY_DEV_INIT_LEVEL, MY_DEV_INIT_PRIORITY,	\
			&my_api_funcs)

Notice the use of APIs like :c:macro:`DT_PROP` and
:c:macro:`DEVICE_DT_DEFINE` to access devicetree node data.

Finally, manually detect each enabled devicetree node and use
``CREATE_MY_DEVICE`` to instantiate each ``struct device``:

.. code-block:: c

   #if DT_NODE_HAS_STATUS(DT_NODELABEL(mydevice0), okay)
   CREATE_MY_DEVICE(0)
   #endif

   #if DT_NODE_HAS_STATUS(DT_NODELABEL(mydevice1), okay)
   CREATE_MY_DEVICE(1)
   #endif

Since this style does not use ``DT_INST_FOREACH_STATUS_OKAY()``, the driver
author is responsible for calling ``CREATE_MY_DEVICE()`` for every possible
node, e.g. using knowledge about the peripherals available on supported SoCs.

.. _dt-drivers-that-depend:

Device drivers that depend on other devices
*******************************************

At times, one ``struct device`` depends on another ``struct device`` and
requires a pointer to it. For example, a sensor device might need a pointer to
its SPI bus controller device. Some advice:

- Write your devicetree binding in a way that permits use of
  :ref:`devicetree-hw-api` from devicetree.h if possible.
- In particular, for bus devices, your driver's binding should include a
  file like :zephyr_file:`dts/bindings/spi/spi-device.yaml` which provides
  common definitions for devices addressable via a specific bus. This enables
  use of APIs like :c:macro:`DT_BUS` to obtain a node identifier for the bus
  node. You can then :ref:`dt-get-device` for the bus in the usual way.

Search existing bindings and device drivers for examples.

.. _dt-apps-that-depend:

Applications that depend on board-specific devices
**************************************************

One way to allow application code to run unmodified on multiple boards is by
supporting a devicetree alias to specify the hardware specific portions, as is
done in the :zephyr:code-sample:`blinky` sample. The application can then be configured in
:ref:`BOARD.dts <devicetree-in-out-files>` files or via :ref:`devicetree
overlays <use-dt-overlays>`.
