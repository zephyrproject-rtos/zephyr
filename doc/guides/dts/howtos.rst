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
that the node has ``status = "okay"``, like this:

.. code-block:: c

   #define MY_SERIAL DT_NODELABEL(my_serial)

   #if DT_NODE_HAS_STATUS(MY_SERIAL, okay)
   struct device *uart_dev = device_get_binding(DT_LABEL(MY_SERIAL));
   #else
   #error "Node is disabled"
   #endif

If you see the ``#error`` output, make sure to enable the node in your
devicetree. If you don't see the ``#error`` but ``uart_dev`` is NULL, then
there's likely either a Kconfig issue preventing the device driver from
creating the device, or the device's initialization function failed.

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
variable :makevar:`DTC_OVERLAY_FILE` contains a space- or semicolon-separated
list of overlays. If :makevar:`DTC_OVERLAY_FILE` specifies multiple files, they
are included in that order by the C preprocessor.

Here are some ways to set it:

1. on the cmake build command line
   (``-DDTC_OVERLAY_FILE="file1.overlay;file2.overlay"``)
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

To delete a property (in addition to deleting properties in general, this is
how to set a boolean property to false if it's true in BOARD.dts):

.. code-block:: none

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

.. _dt-create-devices:

Write device drivers using devicetree APIs
******************************************

"Devicetree-aware" :ref:`device drivers <device_model_api>` should create a
``struct device`` for each ``status = "okay"`` devicetree node with a
particular :ref:`compatible <dt-important-props>` (or related set of
compatibles) supported by the driver.

.. note::

  Historically, Zephyr has used Kconfig options like :option:`CONFIG_SPI_0` and
  :option:`CONFIG_I2C_1` to enable driver support for individual devices of
  some type. For example, if ``CONFIG_I2C_1=y``, the SoC's I2C peripheral
  driver would create a ``struct device`` for "I2C bus controller number 1".

  This style predates support for devicetree in Zephyr and its use is now
  discouraged. Existing device drivers may be made "devicetree-aware"
  in future releases.

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
   #include <drivers/some_api.h>

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
   	DEVICE_AND_API_INIT(my_dev_##inst,				\
   			    DT_INST_LABEL(inst),			\
   			    my_dev_init_function,			\
   			    &my_data_##inst,				\
   			    &my_cfg_##inst,				\
   			    MY_DEV_INIT_LEVEL, MY_DEV_INIT_PRIORITY,	\
   			    &my_api_funcs);

Notice the use of APIs like :c:func:`DT_INST_LABEL` and :c:func:`DT_INST_PROP`
to access devicetree node data. These APIs retrieve data from the devicetree
for instance number ``inst`` of the node with compatible determined by
``DT_DRV_COMPAT``.

Finally, pass the instantiation macro to :c:func:`DT_INST_FOREACH_STATUS_OKAY`:

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
:c:func:`DT_NODELABEL` to refer to individual nodes in the devicetree
representing the supported peripherals on the SoC. The devicetree.h
:ref:`devicetree-generic-apis` can then be used to access node data.

For this to work, your :ref:`SoC's dtsi file <dt-input-files>` must define node
labels like ``mydevice0``, ``mydevice1``, etc. appropriately for the IP blocks
your driver supports. The resulting devicetree usually looks something like
this:

.. code-block:: DTS

   / {
           soc {
                   mydevice0: dev@... {
                           compatible = "vnd,my-device";
                   };
                   mydevice1: dev@... {
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
	DEVICE_AND_API_INIT(my_dev_##idx,				\
			    DT_LABEL(MYDEV(idx)),			\
			    my_dev_init_function,			\
			    &my_data_##idx,				\
			    &my_cfg_##idx,				\
			    MY_DEV_INIT_LEVEL, MY_DEV_INIT_PRIORITY,	\
			    &my_api_funcs)

Notice the use of APIs like :c:func:`DT_LABEL` and :c:func:`DT_PROP` to access
devicetree node data.

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
  use of APIs like :c:func:`DT_BUS` to obtain a node identifier for the bus
  node. You can then :ref:`dt-get-device` for the bus in the usual way.

Search existing bindings and device drivers for examples.

.. _dt-apps-that-depend:

Applications that depend on board-specific devices
**************************************************

One way to allow application code to run unmodified on multiple boards is by
supporting a devicetree alias to specify the hardware specific portions, as is
done in the :ref:`blinky-sample`. The application can then be configured in
:ref:`BOARD.dts <devicetree-in-out-files>` files or via :ref:`devicetree
overlays <use-dt-overlays>`.

.. _dt-migrate-legacy:

Migrate from the legacy macros
******************************

This section shows how to migrate from the :ref:`dt-legacy-macros` to the
:ref:`devicetree.h API <dt-from-c>`. (Please feel free to :ref:`ask for help
<help>` if a use case you need is missing here and existing documentation is
not enough to figure out what to do.)

This DTS is used for examples:

.. literalinclude:: ../../../tests/lib/devicetree/legacy_api/app.overlay
   :language: DTS
   :start-after: start-after-here
   :end-before: end-before-here

The following shows equivalent ways to access this devicetree, using legacy
macros and the new devicetree.h API.

.. warning::

   The INST numbers below were carefully chosen to work. Instance numbering
   properties have changed in the devicetree.h API compared to the legacy
   macros, and are not guaranteed to be the same in all cases. See
   :c:func:`DT_INST` for details.

.. code-block:: c

   /*
    * label
    *
    * These use the label property in /migration/gpio@1000.
    * They all expand to "MGR_GPIO".
    */

   /* Legacy: */
   DT_VND_GPIO_1000_LABEL
   DT_INST_0_VND_GPIO_LABEL
   DT_ALIAS_MGR_GPIO_LABEL

   /* Use these instead: */
   DT_LABEL(DT_PATH(migration, gpio_1000))
   DT_LABEL(DT_INST(0, vnd_gpio))
   DT_LABEL(DT_ALIAS(mgr_gpio))
   DT_LABEL(DT_NODELABEL(migration_gpio))

   /*
    * reg base addresses and sizes
    *
    * These use the reg property in /migration/gpio@1000.
    * The base addresses all expand to 0x1000, and sizes to 0x2000.
    */

   /* Legacy addresses: */
   DT_VND_GPIO_1000_BASE_ADDRESS
   DT_INST_0_VND_GPIO_BASE_ADDRESS
   DT_ALIAS_MGR_GPIO_BASE_ADDRESS

   /* Use these instead: */
   DT_REG_ADDR(DT_PATH(migration, gpio_1000))
   DT_REG_ADDR(DT_INST(0, vnd_gpio))
   DT_REG_ADDR(DT_ALIAS(mgr_gpio))
   DT_REG_ADDR(DT_NODELABEL(migration_gpio))

   /* Legacy sizes: */
   DT_VND_GPIO_1000_SIZE
   DT_INST_0_VND_GPIO_SIZE
   DT_ALIAS_MGR_GPIO_SIZE

   /* Use these instead: */
   DT_REG_SIZE(DT_PATH(migration, gpio_1000))
   DT_REG_SIZE(DT_INST(0, vnd_gpio))
   DT_REG_SIZE(DT_ALIAS(mgr_gpio))
   DT_REG_SIZE(DT_NODELABEL(migration_gpio))

   /*
    * interrupts IRQ numbers and priorities
    *
    * These use the interrupts property in /migration/gpio@1000.
    * The interrupt number is 0, and the priority is 1.
    */

   /* Legacy interrupt numbers: */
   DT_VND_GPIO_1000_IRQ_0
   DT_INST_0_VND_GPIO_IRQ_0
   DT_ALIAS_MGR_GPIO_IRQ_0

   /* Use these instead: */
   DT_IRQN(DT_PATH(migration, gpio_1000))
   DT_IRQN(DT_INST(0, vnd_gpio))
   DT_IRQN(DT_ALIAS(mgr_gpio))
   DT_IRQN(DT_NODELABEL(migration_gpio))

   /* Legacy priorities: */
   DT_VND_GPIO_1000_IRQ_0_PRIORITY,
   DT_INST_0_VND_GPIO_IRQ_0_PRIORITY,
   DT_ALIAS_MGR_GPIO_IRQ_0_PRIORITY,

   /* Use these instead: */
   DT_IRQ(DT_PATH(migration, gpio_1000), priority)
   DT_IRQ(DT_INST(0, vnd_gpio), priority)
   DT_IRQ(DT_ALIAS(mgr_gpio), priority)
   DT_IRQ(DT_NODELABEL(migration_gpio), priority)

   /*
    * Other property access
    *
    * These use the baud-rate property in /migration/serial@3000.
    * They all expand to 115200.
    */

   /* Legacy: */
   DT_VND_SERIAL_3000_BAUD_RATE
   DT_ALIAS_MGR_SERIAL_BAUD_RATE
   DT_INST_0_VND_SERIAL_BAUD_RATE

   /* Use these instead: */
   DT_PROP(DT_PATH(migration, serial_3000), baud_rate)
   DT_PROP(DT_ALIAS(mgr_serial), baud_rate)
   DT_PROP(DT_NODELABEL(migration_serial), baud_rate)
   DT_PROP(DT_INST(0, vnd_serial), baud_rate)

   /*
    * I2C bus controller label access for an I2C peripheral device.
    *
    * These are different ways to get the bus controller label property
    * from the peripheral device /migration/i2c@1000/i2c-dev-10.
    *
    * They all expand to "MGR_I2C".
    */

   /* Legacy: */
   DT_VND_I2C_10000_VND_I2C_DEVICE_10_BUS_NAME
   DT_ALIAS_MGR_I2C_DEV_BUS_NAME
   DT_INST_0_VND_I2C_DEVICE_BUS_NAME

   /* Use these instead (the extra #defines are just for readability): */
   #define I2C_DEV_PATH		DT_PATH(migration, i2c_10000, i2c_dev_10)
   #define I2C_DEV_ALIAS		DT_ALIAS(mgr_i2c_dev)
   #define I2C_DEV_NODELABEL	DT_NODELABEL(mgr_i2c_device)
   #define I2C_DEV_INST		DT_INST(0, vnd_i2c_device)

   DT_LABEL(DT_BUS(I2C_DEV_PATH))
   DT_LABEL(DT_BUS(I2C_DEV_ALIAS)))
   DT_LABEL(DT_BUS(I2C_DEV_NODELABEL)))
   DT_LABEL(DT_BUS(I2C_DEV_INST)))

   /*
    * SPI device chip-select controller.
    *
    * These use /migration/spi@2000/spi-dev@0. They all expand to
    * "MGR_GPIO", which is the label property of /migration/gpio@1000,
    * which is the SPI device's chip select pin GPIO controller. This is
    * taken from the parent node's cs-gpios property.
    */

   /* Legacy */
   DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_CONTROLLER
   DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_CONTROLLER
   DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_CONTROLLER

   /* Use these instead (extra #defines just for readability): */
   #define SPI_DEV_PATH		DT_PATH(migration, spi_20000, migration_spi_dev_0)
   #define SPI_DEV_ALIAS		DT_ALIAS(mgr_spi_dev)
   #define SPI_DEV_NODELABEL	DT_NODELABEL(mgr_spi_device)
   #define SPI_DEV_INST		DT_INST(0, vnd_spi_device)

   DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_PATH)
   DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_ALIAS)
   DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_NODELABEL)
   DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_INST)

   /*
    * SPI device chip-select pin.
    *
    * These use /migration/spi@2000/spi-dev@0.
    * They all expand to 17, which is also from cs-gpios.
    */

   /* Legacy: */
   DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_PIN
   DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_PIN
   DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_PIN
   DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_PIN

   /* Use these instead (extra #defines from above): */
   DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_PATH)
   DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_ALIAS)
   DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_NODEPIN)
   DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_INST)

   /*
    * SPI device chip-select pin's flags for the gpio.h API.
    *
    * These use /migration/spi@2000/spi-dev@0. They all expand to
    * GPIO_ACTIVE_LOW (technically, its numeric value after
    * preprocessing), which is also from cs-gpios.
    */

   /* Legacy: */
   DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_FLAGS
   DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_FLAGS
   DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_FLAGS
   DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_FLAGS

   /* Use these instead (extra #defines from above): */
   DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_PATH)
   DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_ALIAS)
   DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_NODEFLAGS)
   DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_INST)

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

If you're getting a compile error reading a node property, check your node
identifier and property. For example, if you get a build error on a line that
looks like this:

.. code-block:: c

   int baud_rate = DT_PROP(DT_NODELABEL(my_serial), current_speed);

Try checking the node by adding this to the file and recompiling:

.. code-block:: c

   #if !DT_NODE_EXISTS(DT_NODELABEL(my_serial))
   #error "whoops"
   #endif

If you see the "whoops" error message when you rebuild, the node identifier
isn't referring to a valid node. :ref:`get-devicetree-outputs` and debug from
there.

Some hints for what to check next if you don't see the "whoops" error message:

- did you :ref:`dt-use-the-right-names`?
- does the :ref:`property exist <dt-checking-property-exists>`?
- does the node have a :ref:`matching binding <dt-bindings>`?

.. _missing-dt-binding:

Check for missing bindings
==========================

If the build fails to :ref:`dts-find-binding` for a node, then either the
node's ``compatible`` property is not defined, or its value has no matching
binding. If the property is set, check for typos in its name. In a devicetree
source file, ``compatible`` should look like ``"vnd,some-device"`` --
:ref:`dt-use-the-right-names`.

If your binding file is not under :file:`zephyr/dts`, you may need to set
:ref:`DTS_ROOT <dts_root>`.

Errors with DT_INST_() APIs
===========================

If you're using an API like :c:func:`DT_INST_PROP`, you must define
``DT_DRV_COMPAT`` to the lowercase-and-underscores version of the compatible
you are interested in. See :ref:`dt-create-devices-inst`.
