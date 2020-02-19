.. _dt-macros:

Macros generated from Devicetree
################################

This page describes the C preprocessor macros which Zephyr's build system
generates from a :ref:`devicetree`. It assumes you're familiar with the
concepts in :ref:`devicetree-intro`.

Example
*******

Take this devicetree node as an example:

.. code-block:: none

   sim@40047000 {
   	compatible = "nxp,kinetis-sim";
   	reg = <0x40047000 0x1060>;
   	label = "SIM";
   	...
   };

Below is sample header content generated for this node, in
:file:`include/devicetree_unfixed.h` in the build directory:

.. code-block:: c

   /*
    * Devicetree node:
    *   /soc/sim@40047000
    *
    * Binding (compatible = nxp,kinetis-sim):
    *   $ZEPHYR_BASE/dts/bindings/arm/nxp,kinetis-sim.yaml
    *
    * Dependency Ordinal: 24
    *
    * Requires:
    *   7   /soc
    *
    * Supports:
    *   25  /soc/i2c@40066000
    *   26  /soc/i2c@40067000
    *   ...
    *
    * Description:
    *   Kinetis System Integration Module (SIM) IP node
    */
   #define DT_NXP_KINETIS_SIM_40047000_BASE_ADDRESS    0x40047000
   #define DT_INST_0_NXP_KINETIS_SIM_BASE_ADDRESS      DT_NXP_KINETIS_SIM_40047000_BASE_ADDRESS
   #define DT_NXP_KINETIS_SIM_40047000_SIZE            4192
   #define DT_INST_0_NXP_KINETIS_SIM_SIZE              DT_NXP_KINETIS_SIM_40047000_SIZE
   /* Human readable string describing the device (used by Zephyr for API name) */
   #define DT_NXP_KINETIS_SIM_40047000_LABEL           "SIM"
   #define DT_INST_0_NXP_KINETIS_SIM_LABEL             DT_NXP_KINETIS_SIM_40047000_LABEL
   #define DT_INST_0_NXP_KINETIS_SIM                   1

All macros generated from devicetree start with ``DT_`` and use all-uppercase.
Most macro names contain a node identifier, and macros that correspond to
devicetree properties also contain an identifier for the property.

Node identifiers
****************

Macros generated from particular devicetree nodes start with ``DT_<node>``,
where ``<node>`` identifies the node. Several identifiers are generated for
nodes:

``DT_(<bus>_)<compatible>_<unit-address>``
    The compatible string for the node followed by its unit address. If the
    node has several compatible strings, the first one that matched a binding
    is used.

    If the node appears on a bus (has ``on-bus:`` in its binding), then the
    compatible string and unit address of the bus node is put before the
    compatible string for the node itself.  If the node does not appear on
    a bus (no ``on-bus:`` in the binding) then there will be no bus portion
    of the node identifier.

    For nodes that don't have a unit address (no ``...@<address>`` in the node
    name), the unit address of the parent node plus the node name is used instead.
    If the parent node has no unit address either, the name of the node is used for
    ``<unit-address>``, as a fallback.

    .. code-block:: none

       i2c0: i2c@40066000 {
               compatible = "nxp,kinetis-i2c";
               status = "okay";
               #address-cells = <1>;
               #size-cells = <0>;
               reg = <0x40066000 0x1000>;

               fxos8700@1d {
                       compatible = "nxp,fxos8700";
                       reg = <0x1d>;
               };
       };

    For the ``i2c@40066000`` node above, the compatible string is
    ``"nxp,kinetis-i2c"``. This gets converted to ``NXP_KINETIS_I2C`` by
    uppercasing and replacing non-alphanumeric characters with underscores.
    Adding the unit address gives the node identifier
    ``DT_NXP_KINETIS_I2C_40066000``.

    For the ``fxos8700@1d`` node above, since the binding specifies ``on-bus: i2c``
    the bus portion of the node identifier will be: ``DT_NXP_KINETIS_I2C_40066000``.
    The device node portion of the identifier will be ``NXP_FXOS8700_1D``.  The full
    node identifier will be: ``DT_NXP_KINETIS_I2C_40066000_NXP_FXOS8700_1D``.

    .. code-block:: none

       ethernet@400c0004 {
               compatible = "nxp,kinetis-ethernet";
               reg = <0x400c0004 0x620>;
               status = "okay";
               ptp {
                       compatible = "nxp,kinetis-ptp";
                       status = "okay";
                       interrupts = <0x52 0x0>;
               };
       };

    For the ``ptp`` node above, since the node has no unit address, the unit
    address portion will combine the parent's unit address and the node's
    name.  The unit address portion of the identifier will be: ``400C0004_PTP``.
    The full node identifier that combines the node compatible and unit address
    will be: ``DT_NXP_KINETIS_PTP_400C0004_PTP``.

    .. code-block:: none

       soc {
              temp1 {
                      compatible = "nxp,kinetis-temperature";
                      status = "okay";
              };
       };

    For the ``temp1`` node above, since the node has no unit address the unit
    address portion and the parent has no unit address we will utilize the
    nodes name ``TEMP1`` as the unit address portion.
    The full node identifier that combines the node compatible and unit address
    will be: ``DT_NXP_KINETIS_TEMPERATURE_TEMP1``.

``DT_INST_<instance-no.>_<compatible>``
    The compatible string for the node together with an instance number.

    The instance number is a unique index among all enabled
    (``status = "okay"``) nodes that have a particular compatible string,
    counting from zero. For example, if there are two enabled nodes that have
    ``compatible = "foo,uart"``, then these node identifiers get generated:

    .. code-block:: none

       DT_INST_0_FOO_UART
       DT_INST_1_FOO_UART

    .. note::

       The instance numbers in no way reflect any numbering scheme that
       might exist in SoC documentation, node labels, or node unit addresses.
       The instance number is a simple index among enabled nodes with the
       same compatible.  There is no guarantee that the same device
       node is given the same instance number between builds.  The only
       guarantee is that instance numbers will start at 0, be contiguous,
       and be assigned for each enabled node with a matching compatible.

``DT_ALIAS_<alias>``
    Generated from the names of any properties in the ``/aliases`` node.
    See :ref:`dt-alias-chosen` for an overview.

    For example, assume ``/aliases`` looks like this:

    .. code-block:: none

       aliases {
               uart-1 = &uart1;
       };

    The alias name (``uart-1``) is converted to ``UART_1`` by uppercasing and
    replacing non-alphanumeric characters with underscores, generating the node
    identifier ``DT_ALIAS_UART_1`` for the ``&uart1`` node.

    .. note::

       Currently, an older deprecated ``DT_<compatible>_<alias>`` form is also
       generated for aliases. For the example above, assuming the compatible
       string for the ``&uart1`` node is ``"foo,uart"``, this gives
       ``DT_FOO_UART_UART_1``.

       Work is underway to replace this form with ``DT_ALIAS_*``.

Property identifiers
********************

Macros for particular properties on nodes have the form
``DT_<node>_<property>``, where ``<node>`` is a node identifier (see above), and
``<property>`` identifies the property.

For following ``<property>`` have special case handling:

- ``reg`` (:ref:`documented below <reg_macros>`)
- ``interrupts`` (:ref:`documented below <irq_macros>`)
- ``clocks`` (:ref:`documented below <clk_macros>`)
- ``cs-gpios`` for SPI GPIO chip select (:ref:`documented below <spi_cs_macros>`)

All other ``<property>`` are just the property name uppercased with non-alphanumeric
characters replaced with underscores. ``current-speed = ...`` turns into
``CURRENT_SPEED``, for example.

.. _dt-property-macros:

Macros generated from properties
********************************

This section explains what values get generated for different property types
(as declared in :ref:`dt-bindings`), with examples.

The table below gives the values generated for simple types. They should be
mostly intuitive. Note that an index is added at the end of identifiers
generated from properties with ``array`` or ``string-array`` type, and that
``array`` properties generate an additional compound initializer (``{ ... }``).

+------------------+------------------------+----------------------------------------+
| Type             | Example                | Generated macros                       |
+==================+========================+========================================+
| ``int``          | ``foo = <1>``          | ``#define DT_<node>_FOO 1``            |
+------------------+------------------------+----------------------------------------+
| ``array``        | ``foo = <1 2>``        | | ``#define DT_<node>_FOO_0 1``        |
|                  |                        | | ``#define DT_<node>_FOO_1 2``        |
|                  |                        | | ``#define DT_<node>_FOO {1, 2}``     |
+------------------+------------------------+----------------------------------------+
| ``string``       | ``foo = "bar"``        | ``#define DT_<node>_FOO "bar"``        |
+------------------+------------------------+----------------------------------------+
| ``string-array`` | ``foo = "bar", "baz"`` | | ``#define DT_<node>_FOO_0 "bar"``    |
|                  |                        | | ``#define DT_<node>_FOO_1 "baz"``    |
+------------------+------------------------+----------------------------------------+
| ``uint8-array``  | ``foo = [01 02]``      | ``#define DT_<node>_FOO {0x01, 0x02}`` |
+------------------+------------------------+----------------------------------------+

For ``type: boolean``, the generated macro is set to 1 if the property exists
on the node, and to 0 otherwise:

.. code-block:: none

   #define DT_<node>_FOO 0/1

For non-boolean types the property macros are not generated if the ``category``
is ``optional`` and the property is not present in the devicetree source.

The generation for properties with type ``phandle-array`` is the most complex.
To understand it, it is a good idea to first go through the documentation for
``phandle-array`` in :ref:`dt-bindings`.

Take the following devicetree nodes and binding contents as an example:

.. code-block:: none
   :caption: Devicetree nodes for PWM controllers

   pwm_ctrl_0: pwm-controller-0 {
        compatible = "vendor,pwm-controller";
        label = "pwm-0";
        #pwm-cells = <2>;
        ...
   };

   pwm_ctrl_1: pwm-controller-1 {
        compatible = "vendor,pwm-controller";
        label = "pwm-1";
        #pwm-cells = <2>;
        ...
   };

.. code-block:: yaml
   :caption: ``pwm-cells`` declaration in binding for ``vendor-pwm-controller``

   pwm-cells:
       - channel
       - period

Assume the property assignment looks like this:

.. code-block:: none

   pwm-user@0 {
           compatible = "vendor,foo";
           status = "okay";
           reg = <0 1024>;
           pwms = <&pwm_ctrl_0 1 10
                   &pwm_ctrl_1 2 20>;
           pwm-names = "first", "second";
   };

These macros then get generated:

.. code-block:: none

   #define DT_VENDOR_FOO_0_PWMS_CONTROLLER_0    "PWM_0"
   #define DT_VENDOR_FOO_0_PWMS_CHANNEL_0       1
   #define DT_VENDOR_FOO_0_PWMS_PERIOD_0        10

   #define DT_VENDOR_FOO_0_PWMS_CONTROLLER_1    "PWM_1"
   #define DT_VENDOR_FOO_0_PWMS_CHANNEL_1       2
   #define DT_VENDOR_FOO_0_PWMS_PERIOD_1        20

   /* Initializers */

   #define DT_VENDOR_FOO_0_PWMS_0               {"pwm_0", 1, 10}
   #define DT_VENDOR_FOO_0_PWMS_1               {"pwm_1", 2, 20}
   #define DT_VENDOR_FOO_0_PWMS                 {DT_VENDOR_FOO_0_PWMS_0, DT_VENDOR_FOO_0_PWMS_1}

   #define DT_VENDOR_FOO_0_PWMS_COUNT           2

Macros with a ``*_0`` suffix deal with the first entry in ``pwms``
(``<&pwm_ctrl_0 1 10>``). Macros with a ``*_1`` suffix deal with the second
entry (``<&pwm_ctrl_1 2 20>``). The index suffix is only added if there's more
than one entry in the property.

The ``DT_VENDOR_FOO_0_PWMS_CONTROLLER(_<index>)`` macros are set to the string from
the ``label`` property of the referenced controller. The
``DT_VENDOR_FOO_0_PWMS_CHANNEL(_<index>)`` and ``DT_VENDOR_FOO_0_PWMS_PERIOD(_<index>)``
macros are set to the values of the corresponding cells in the ``pwms``
assignment, with macro names generated from the strings in ``pwm-cells:`` in
the binding for the controller.

The macros in the ``/* Initializers */`` section provide the same information
as ``DT_VENDOR_FOO_0_PWMS_CHANNEL/PERIOD``, except as a compound initializer that can
be used to initialize a C ``struct``.

If a ``pwm-names`` property exists on the same node as ``pwms`` (or similarly
for other ``phandle-array`` properties), it gives a list of strings that names
each entry in ``pwms``. The names are used to generate extra macro names with
the name instead of an index. For example, ``pwm-names = "first", "second"``
together with the example property generates these additional macros:

.. code-block:: none

   #define DT_VENDOR_FOO_0_FIRST_PWMS_CONTROLLER    "PWM_0"
   #define DT_VENDOR_FOO_0_FIRST_PWMS_CHANNEL       1
   #define DT_VENDOR_FOO_0_FIRST_PWMS_PERIOD        10

   #define DT_VENDOR_FOO_0_SECOND_PWMS_CONTROLLER   "PWM_1"
   #define DT_VENDOR_FOO_0_SECOND_PWMS_CHANNEL      2
   #define DT_VENDOR_FOO_0_SECOND_PWMS_PERIOD       20

   ...

Macros generated from ``enum:`` keys
************************************

Properties declared with an ``enum:`` key in their :ref:`dt-bindings`
generate a macro that gives the the zero-based index of the property's value in
the ``enum:`` list.

Take this binding declaration as an example:

.. code-block:: yaml

   properties:
       foo:
           type: string
           enum:
               - one
               - two
               - three

The assignment ``foo = "three"`` then generates this macro:

.. code-block:: none

    #define DT_<node>_FOO_ENUM 2

.. _reg_macros:

Macros generated from ``reg``
*****************************

``reg`` properties generate the macros ``DT_<node>_BASE_ADDRESS(_<index>)`` and
``DT_<node>_SIZE(_<index>)``. ``<index>`` is a numeric index starting from 0,
which is only added if there's more than one register defined in ``reg``.

For example, the ``reg = <0x4004700 0x1060>`` assignment in the example
devicetree above gives these macros:

.. code-block:: c

   #define DT_<node>_BASE_ADDRESS    0x40047000
   #define DT_<node>_SIZE            4192

.. note::

   The length of the address and size portions of ``reg`` is determined from
   the ``#address-cells`` and ``#size-cells`` properties. See the devicetree
   specification for more information.

   In this case, both ``#address-cells`` and ``#size-cells`` are 1, and there's
   just a single register in ``reg``. Four numbers would give two registers.

If a ``reg-names`` property exists on the same node as ``reg``, it gives a list
of strings that names each register in ``reg``. The names are used to generate
extra macros. For example, ``reg-names = "foo"`` together with the example node
generates these macros:

.. code-block:: c

   #define DT_NXP_KINETIS_SIM_40047000_FOO_BASE_ADDRESS    0x40047000
   #define DT_NXP_KINETIS_SIM_40047000_FOO_SIZE            4192

.. _irq_macros:

Interrupt-related macros
************************

Take these devicetree nodes as an example:

.. code-block:: none

   timer@123 {
        compatible = "vendor,timer";
        interrupts = <1 5 2 6>;
        interrupt-parent = <&intc>;
   };

   intc: interrupt-controller {
        compatible = "vendor,intc";
   };

Assume that the binding for the interrupt controller (which would have
``compatible: "vendor,intc"``) has this:

.. code-block:: yaml

   interrupt-cells:
       - irq
       - priority

Then these macros get generated:

.. code-block:: c

   #define DT_VENDOR_TIMER_123_IRQ_0                   1
   #define DT_VENDOR_TIMER_123_IRQ_0_PRIORITY          5
   #define DT_VENDOR_TIMER_123_IRQ_1                   2
   #define DT_VENDOR_TIMER_123_IRQ_1_PRIORITY          6

These macros have the the format ``DT_<node>_IRQ_<index>(_<name>)``, where
``<node>`` is the node identifier, ``<index>`` is an index that identifies the
particular interrupt, and ``<name>`` is the identifier for the cell value (a
number within ``interrupts = <...>``), taken from the binding.

Bindings for interrupt controllers are expected to declare a cell named ``irq``
in ``interrupt-cells``, giving the interrupt number. The ``_<name>`` suffix is
skipped for macros generated from ``irq`` cells, which is why there's e.g. a
``DT_VENDOR_TIMER_123_IRQ_0`` macro and no ``DT_VENDOR_TIMER_123_IRQ_0_IRQ``
macro.

If the interrupt controller in turn generates other interrupts, a multi-level
interrupt encoding is used for the interrupt number. See
:ref:`multi_level_interrupts` for more information.

Additional macros that use names instead of indices for interrupts can be
generated by including an ``interrupt-names`` property on the
interrupt-generating node. For example, ``interrupt-names = "timer-a",
"timer-b"`` gives these extra macros:

.. code-block:: c

   #define DT_VENDOR_TIMER_123_IRQ_TIMER_A             1
   #define DT_VENDOR_TIMER_123_IRQ_TIMER_A_PRIORITY    5
   #define DT_VENDOR_TIMER_123_IRQ_TIMER_B             2
   #define DT_VENDOR_TIMER_123_IRQ_TIMER_B_PRIORITY    6

.. _clk_macros:

Values for properties generated from ``clocks``
***********************************************

``clocks`` work the same as other ``phandle-array`` properties, except the
generated macros have ``CLOCK`` in them instead of ``CLOCKS``, giving for
example ``DT_<node>_CLOCK_CONTROLLER_0`` instead of
``DT_<node>_CLOCKS_CONTROLLER_0``.

.. note::

   This inconsistency might be fixed in the future.

In addition, if the clock controller node has a ``fixed-clock`` property, it is
expected to also have a ``clock-frequency`` property giving the frequency, and
an additional macro is generated:

.. code-block:: c

   #define DT_<node>_CLOCKS_CLOCK_FREQUENCY <frequency>

.. _spi_cs_macros:

Macros related to SPI GPIO chip select
**************************************

.. boards/arm/sensortile_box/sensortile_box.dts has a real-world example

Take these devicetree nodes as an example. where the binding for
``vendor,spi-controller`` is assumed to have ``bus: spi``, and the bindings for
the SPI slaves are assumed to have ``on-bus: spi``:

.. code-block:: none

   gpioa: gpio@400ff000 {
        compatible = "vendor,gpio-ctlr";
        reg = <0x400ff000 0x40>;
        label = "GPIOA";
        gpio-controller;
        #gpio-cells = <0x1>;
   };

   spi {
	compatible = "vendor,spi-controller";
	cs-gpios = <&gpioa 1>, <&gpioa 2>;
	spi-slave@0 {
		compatible "vendor,foo-spi-device";
		reg = <0>;
	};
	spi-slave@1 {
		compatible "vendor,bar-spi-device";
		reg = <1>;
	}
   };

Here, the unit address of the SPI slaves (0 and 1) is taken as a chip select
number, which is used as an index into ``cs-gpios`` (a ``phandle-array``).
``spi-slave@0`` is matched to ``<&gpioa 1>``, and ``spi-slave@1`` to
``<&gpiob 2>``.

The output for ``spi-slave@0`` and ``spi-slave@1`` is the same as if the
devicetree had looked like this:

.. code-block:: none

   gpioa: gpio@400ff000 {
        compatible = "vendor,gpio-ctlr";
        reg = <0x400ff000 0x40>;
        label = "GPIOA";
        gpio-controller;
        #gpio-cells = <1>;
   };

   spi {
	compatible = "vendor,spi-controller";
	spi-slave@0 {
		compatible "vendor,foo-spi-device";
		reg = <0>;
		cs-gpios = <&gpioa 1>;
	};
	spi-slave@1 {
		compatible "vendor,bar-spi-device";
		reg = <1>;
		cs-gpios = <&gpioa 2>;
	}
   };

See the ``phandle-array`` section in :ref:`dt-property-macros` for more
information.

For example, since the node labeled ``gpioa`` has property
``label = "GPIOA"`` and 1 and 2 are pin numbers, macros like the following
will be generated for ``spi-slave@0``:

.. code-block:: none

   #define DT_<node>_CS_GPIOS_CONTROLLER    "GPIOA"
   #define DT_<node>_CS_GPIOS_PIN           1

.. _dt-existence-macros:

Compatible-string and node existence macros
*******************************************

An existence flag is written for each compatible strings that appears on some
enabled node:

.. code-block:: none

   #define DT_COMPAT_<compatible> 1

An existence flag is also written for all enabled nodes that matched some
binding:

.. code-block:: none

   #define DT_INST_<instance-no.>_<compatible>    1

For the example ``sim@40047000`` node above, assuming the node is the first
node with ``compatible = "nxp,kinetis-sim"``, the macros look like this:

.. code-block:: c

   #define DT_COMPAT_NXP_KINETIS_SIM    1
   #define DT_INST_0_NXP_KINETIS_SIM    1

Bus-related macros
******************

These macros get generated for nodes that appear on buses (have ``on-bus:`` in
their binding):

.. code-block:: none

   #define DT_<node>_BUS_NAME                "<bus-label>"
   #define DT_<compatible>_BUS_<bus-name>    1

``<bus-label>`` is taken from the ``label`` property on the bus node, which
must exist. ``<bus-name>`` is the identifier for the bus as given in
``on-bus:`` in the binding.

Macros generated from flash partitions
**************************************

.. note::

   This section only covers flash partitions. See :ref:`dt-alias-chosen` for
   some other flash-related macros that get generated from devicetree, via
   ``/chosen``.

If a node has a name that looks like ``partition@<unit-address>``, it is
assumed to represent a flash partition.

Assume the devicetree has this:

.. code-block:: none

   flash@0 {
        ...
        label = "foo-flash";

        partitions {
                ...
                #address-cells = <1>;
                #size-cells = <1>;

                boot_partition: partition@0 {
                        label = "mcuboot";
                        reg = <0x00000000 0x00010000>;
                        read-only;
                };
                slot0_partition: partition@10000 {
                        label = "image-0";
                        reg = <0x00010000 0x00020000
                               0x00040000 0x00010000>;
                };
                ...
        }

These macros then get generated:

.. code-block:: c

   #define DT_FLASH_AREA_MCUBOOT_ID           0
   #define DT_FLASH_AREA_MCUBOOT_READ_ONLY    1
   #define DT_FLASH_AREA_MCUBOOT_OFFSET_0     0x0
   #define DT_FLASH_AREA_MCUBOOT_SIZE_0       0x10000
   #define DT_FLASH_AREA_MCUBOOT_OFFSET       DT_FLASH_AREA_MCUBOOT_OFFSET_0
   #define DT_FLASH_AREA_MCUBOOT_SIZE         DT_FLASH_AREA_MCUBOOT_SIZE_0
   #define DT_FLASH_AREA_MCUBOOT_DEV          "foo-flash"

   #define DT_FLASH_AREA_IMAGE_0_ID           0
   #define DT_FLASH_AREA_IMAGE_0_READ_ONLY    1
   #define DT_FLASH_AREA_IMAGE_0_OFFSET_0     0x10000
   #define DT_FLASH_AREA_IMAGE_0_SIZE_0       0x20000
   #define DT_FLASH_AREA_IMAGE_0_OFFSET_1     0x40000
   #define DT_FLASH_AREA_IMAGE_0_SIZE_1       0x10000
   #define DT_FLASH_AREA_IMAGE_0_OFFSET       DT_FLASH_AREA_IMAGE_0_OFFSET_0
   #define DT_FLASH_AREA_IMAGE_0_SIZE         DT_FLASH_AREA_IMAGE_0_SIZE_0
   #define DT_FLASH_AREA_IMAGE_0_DEV          "foo-flash"

   /* Same macros, just with index instead of label */
   #define DT_FLASH_AREA_0_ID           0
   #define DT_FLASH_AREA_0_READ_ONLY    1
   ...

The ``*_ID`` macro gives the zero-based index for the partition.

The ``*_OFFSET_<index>`` and ``*_SIZE_<index>`` macros give the offset and size
for each partition, derived from ``reg``. The ``*_OFFSET`` and ``*_SIZE``
macros, with no index, are aliases that point to the first sector (with index
0).
