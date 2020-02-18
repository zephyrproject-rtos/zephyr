Devicetree HOWTOs
#################

This page has instructions for getting things done with :ref:`devicetree` in
Zephyr.

Adding support for a board
**************************

Devicetree is currently supported on all embedded targets except posix
(boards/posix).

Adding devicetree support for a given board requires adding a number of files.
These files will contain the DTS information that describes a platform, the
bindings in YAML format, and any fixup files required to support the platform.

It is best practice to separate common peripheral information that could be
used across multiple cores, SoC families, or boards in :file:`.dtsi` files,
reserving the :file:`.dts` suffix for the primary DTS file for a given board.

.. _dt_k6x_example:

Example: FRDM-K64F and Hexiwear K64
===================================

.. Give the filenames instead of the full paths below, as it's easier to read.
   The cramped 'foo.dts<path>' style avoids extra spaces before commas.

The FRDM-K64F and Hexiwear K64 board devicetrees are defined in
:zephyr_file:`frdm_k64fs.dts <boards/arm/frdm_k64f/frdm_k64f.dts>` and
:zephyr_file:`hexiwear_k64.dts <boards/arm/hexiwear_k64/hexiwear_k64.dts>`
respectively. Both boards have NXP SoCs from the same Kinetis SoC family, the
K6X.

Common devicetree definitions for K6X are stored in :zephyr_file:`nxp_k6x.dtsi
<dts/arm/nxp/nxp_k6x.dtsi>`, which is included by both board :file:`.dts`
files. :zephyr_file:`nxp_k6x.dtsi<dts/arm/nxp/nxp_k6x.dtsi>` in turn includes
:zephyr_file:`armv7-m.dtsi<dts/arm/armv7-m.dtsi>`, which has common definitions
for Arm v7-M cores.

Since :zephyr_file:`nxp_k6x.dtsi<dts/arm/nxp/nxp_k6x.dtsi>` is meant to be
generic across K6X-based boards, it leaves many devices disabled by default
using ``status`` properties.  For example, there is a CAN controller defined as
follows (with unimportant parts skipped):

.. code-block:: none

   can0: can@40024000 {
   	...
   	status = "disabled";
   	...
   };

It is up to the board :file:`.dts` or application overlay files to enable these
devices as desired, by setting ``status = "okay"``. The board :file:`.dts`
files are also responsible for any board-specific configuration of the device,
such as adding nodes for on-board sensors, LEDs, buttons, etc.

For example, FRDM-K64 (but not Hexiwear K64) :file:`.dts` enables the CAN
controller and sets the bus speed:

.. code-block:: none

   &can0 {
   	status = "okay";
   	bus-speed = <125000>;
   };

The ``&can0 { ... };`` syntax adds/overrides properties on the node with label
``can0``, i.e. the ``can@4002400`` node defined in the :file:`.dtsi` file.

Other examples of board-specific customization is pointing properties in
``aliases`` and ``chosen`` to the right nodes (see :ref:`dt-alias-chosen`), and
making GPIO/pinmux assignments.

Devicetree Source File Template
===============================

A board's :file:`.dts` file contains at least a version line, optional
includes, and a root node definition with ``model`` and ``compatible``
properties. These property values denote the particular board.

.. code-block:: none

   /dts-v1/;

   #include <vendor/soc.dtsi>

   / {
           model = "Human readable board name";
           compatible = "vendor,soc-on-your-board's-mcu";
           /* rest of file */
   };

You can use other board :file:`.dts` files as a starting point.

The following is a more precise list of required files:

* Base architecture support

  * Add architecture-specific DTS directory, if not already present.
    Example: dts/arm for Arm.
  * Add target specific devicetree files for base SoC.  These should be
    .dtsi files to be included in the board-specific devicetree files.
  * Add target specific YAML binding files in the dts/bindings/ directory.
    Create the yaml directory if not present.

* SoC family support

  * Add one or more SoC family .dtsi files that describe the hardware
    for a set of devices.  The file should contain all the relevant
    nodes and base configuration that would be applicable to all boards
    utilizing that SoC family.
  * Add SoC family YAML binding files that describe the nodes present in the .dtsi file.

* Board specific support

  * Add a board level .dts file that includes the SoC family .dtsi files
    and enables the nodes required for that specific board.
  * Board .dts file should specify the SRAM and FLASH devices, if present.

    * Flash device node might specify flash partitions. For more details see
      :ref:`flash_partitions`

  * Add board-specific YAML binding files, if required.  This would occur if the
    board has additional hardware that is not covered by the SoC family
    .dtsi/.yaml files.

* Fixup files

  * Fixup files contain mappings from existing Kconfig options to the actual
    underlying DTS derived configuration #defines.  Fixup files are temporary
    artifacts until additional DTS changes are made to make them unnecessary.

* Overlay Files (optional)

  * Overlay files contain tweaks or changes to the SoC and Board support files
    described above. They can be used to modify devicetree configurations
    without having to change the SoC and Board files. See
    :ref:`application_dt` for more information on overlay files and the Zephyr
    build system.

.. _dt-alias-chosen:

``aliases`` and ``chosen`` nodes
================================

Using an alias with a common name for a particular node makes it easier for you
to write board-independent source code. Devicetree ``aliases`` nodes  are used
for this purpose, by mapping certain generic, commonly used names to specific
hardware resources:

.. code-block:: yaml

   aliases {
      led0 = &led0;
      sw0 = &button0;
      sw1 = &button1;
      uart-0 = &uart0;
      uart-1 = &uart1;
   };

Certain software subsystems require a specific hardware resource to bind to in
order to function properly. Some of those subsystems are used with many
different boards, which makes using the devicetree ``chosen`` nodes very
convenient. By doing, so the software subsystem can rely on having the specific
hardware peripheral assigned to it. In the following example we bind the shell
to ``uart1`` in this board:

.. code-block:: yaml

   chosen {
      zephyr,shell-uart = &uart1;
   };

The table below lists Zephyr-specific ``chosen`` properties. The macro
identifiers that start with ``CONFIG_*`` are generated from Kconfig symbols
that reference devicetree data via the :ref:`Kconfig preprocessor
<kconfig-functions>`.

.. note::

   Since the particular devicetree isn't known while generating Kconfig
   documentation, the Kconfig symbol reference pages linked below do not
   include information derived from devicetree. Instead, you might see e.g. an
   empty default:

   .. code-block:: none

      default "" if HAS_DTS

   To see how the preprocessor is used for a symbol, look it up directly in the
   :file:`Kconfig` file where it is defined instead. The reference page for the
   symbol gives the definition location.

.. list-table::
   :header-rows: 1

   * - ``chosen`` node name
     - Generated macros

   * - ``zephyr,flash``
     - ``DT_FLASH_BASE_ADDRESS``/``DT_FLASH_SIZE``/``DT_FLASH_ERASE_BLOCK_SIZE``/``DT_FLASH_WRITE_BLOCK_SIZE``
   * - ``zephyr,code-partition``
     - ``DT_CODE_PARTITION_OFFSET``/``DT_CODE_PARTITION_SIZE``
   * - ``zephyr,sram``
     - :option:`CONFIG_SRAM_BASE_ADDRESS`/:option:`CONFIG_SRAM_SIZE`
   * - ``zephyr,ccm``
     - ``DT_CCM_BASE_ADDRESS``/``DT_CCM_SIZE``
   * - ``zephyr,dtcm``
     - ``DT_DTCM_BASE_ADDRESS``/``DT_DTCM_SIZE``
   * - ``zephyr,ipc_shm``
     - ``DT_IPC_SHM_BASE_ADDRESS``/``DT_IPC_SHM_SIZE``
   * - ``zephyr,console``
     - :option:`CONFIG_UART_CONSOLE_ON_DEV_NAME`
   * - ``zephyr,shell-uart``
     - :option:`CONFIG_UART_SHELL_ON_DEV_NAME`
   * - ``zephyr,bt-uart``
     - :option:`CONFIG_BT_UART_ON_DEV_NAME`
   * - ``zephyr,uart-pipe``
     - :option:`CONFIG_UART_PIPE_ON_DEV_NAME`
   * - ``zephyr,bt-mon-uart``
     - :option:`CONFIG_BT_MONITOR_ON_DEV_NAME`
   * - ``zephyr,bt-c2h-uart``
     - :option:`CONFIG_BT_CTLR_TO_HOST_UART_DEV_NAME`
   * - ``zephyr,uart-mcumgr``
     - :option:`CONFIG_UART_MCUMGR_ON_DEV_NAME`

Adding support for a device driver
**********************************

Zephyr device drivers typically use information from :file:`devicetree.h` to
statically allocate and initialize :ref:`struct device <device_struct>`
instances. :ref:`dt-macros` are usually included via :file:`devicetree.h`, then
stored in ROM in the value pointed to by a ``device->config->config_info``
field. For example, a ``struct device`` corresponding to an I2C peripheral
would store the peripheral address in its ``reg`` property there.

Application source code with a pointer to the ``struct device`` can then pass
it to driver APIs in :zephyr_file:`include/drivers/`. These API functions
usually take a ``struct device*`` as their first argument. This allows the
driver API to use information from devicetree to interact with the device
hardware.

.. _flash_partitions:

Managing flash partitions
*************************

Devicetree can be used to describe a partition layout for any flash
device in the system.

Two important uses for this mechanism are:

#. To force the Zephyr image to be linked into a specific area on
   Flash.

   This is useful, for example, if the Zephyr image must be linked at
   some offset from the flash device's start, to be loaded by a
   bootloader at runtime.

#. To generate compile-time definitions for the partition layout,
   which can be shared by Zephyr subsystems and applications to
   operate on specific areas in flash.

   This is useful, for example, to create areas for storing file
   systems or other persistent state.  These defines only describe the
   boundaries of each partition. They don't, for example, initialize a
   partition's flash contents with a file system.

Partitions are generally managed using device tree overlays. Refer to
:ref:`application_dt` for details on using overlay files.

Defining Partitions
===================

The partition layout for a flash device is described inside the
``partitions`` child node of the flash device's node in the device
tree.

You can define partitions for any flash device on the system.

Most Zephyr-supported SoCs with flash support in device tree
will define a label ``flash0``.   This label refers to the primary
on-die flash programmed to run Zephyr. To generate partitions
for this device, add the following snippet to a device tree overlay
file:

.. We can't highlight dts at time of writing:
.. https://github.com/zephyrproject-rtos/zephyr/issues/6029
.. code-block:: none

	&flash0 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			/* Define your partitions here; see below */
		};
	};

To define partitions for another flash device, modify the above to
either use its label or provide a complete path to the flash device
node in the device tree.

The content of the ``partitions`` node looks like this:

.. code-block:: none

	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;

		partition1_label: partition@START_OFFSET_1 {
			label = "partition1_name";
			reg = <0xSTART_OFFSET_1 0xSIZE_1>;
		};

		/* ... */

		partitionN_label: partition@START_OFFSET_N {
			label = "partitionN_name";
			reg = <0xSTART_OFFSET_N 0xSIZE_N>;
		};
	};

Where:

- ``partitionX_label`` are device tree labels that can be used
  elsewhere in the device tree to refer to the partition

- ``partitionX_name`` controls how defines generated by the Zephyr
  build system for this partition will be named

- ``START_OFFSET_x`` is the start offset in hexadecimal notation of
  the partition from the beginning of the flash device

- ``SIZE_x`` is the hexadecimal size, in bytes, of the flash partition

The partitions do not have to cover the entire flash device. The
device tree compiler currently does not check if partitions overlap;
you must ensure they do not when defining them.

Example Primary Flash Partition Layout
======================================

Here is a complete (but hypothetical) example device tree overlay
snippet illustrating these ideas. Notice how the partitions do not
overlap, but also do not cover the entire device.

.. code-block:: none

	&flash0 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			code_dts_label: partition@8000 {
				label = "zephyr-code";
				reg = <0x00008000 0x34000>;
			};

			data_dts_label: partition@70000 {
				label = "application-data";
				reg = <0x00070000 0xD000>;
			};
		};
	};

Linking Zephyr Within a Partition
=================================

To force the linker to output a Zephyr image within a given flash
partition, add this to a device tree overlay:

.. code-block:: none

	/ {
		chosen {
			zephyr,code-partition = &slot0_partition;
		};
	};

Then, enable the :option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option.

Flash Partition Macros
======================

The Zephyr build system generates definitions for each flash device
partition. These definitions are available to any files which
include ``<zephyr.h>``.

Consider this flash partition:

.. code-block:: none

	dts_label: partition@START_OFFSET {
		label = "def-name";
		reg = <0xSTART_OFFSET 0xSIZE>;
	};

The build system will generate the following corresponding defines:

.. code-block:: c

   #define FLASH_AREA_DEF_NAME_LABEL        "def-name"
   #define FLASH_AREA_DEF_NAME_OFFSET_0     0xSTART_OFFSET
   #define FLASH_AREA_DEF_NAME_SIZE_0       0xSIZE
   #define FLASH_AREA_DEF_NAME_OFFSET       FLASH_AREA_MCUBOOT_OFFSET_0
   #define FLASH_AREA_DEF_NAME_SIZE         FLASH_AREA_MCUBOOT_SIZE_0

As you can see, the ``label`` property is capitalized when forming the
macro names. Other simple conversions to ensure it is a valid C
identifier, such as converting "-" to "_", are also performed. The
offsets and sizes are available as well.

.. _mcuboot_partitions:

MCUboot Partitions
==================

`MCUboot`_ is a secure bootloader for 32-bit microcontrollers.

Some Zephyr boards provide definitions for the flash partitions which
are required to build MCUboot itself, as well as any applications
which must be chain-loaded by MCUboot.

The device tree labels for these partitions are:

**boot_partition**
  This is the partition where the bootloader is expected to be
  placed. MCUboot's build system will attempt to link the MCUboot
  image into this partition.

**slot0_partition**
  MCUboot loads the executable application image from this
  partition. Any application bootable by MCUboot must be linked to run
  from this partition.

**slot1_partition**
  This is the partition which stores firmware upgrade images. Zephyr
  applications which receive firmware updates must ensure the upgrade
  images are placed in this partition (the Zephyr DFU subsystem can be
  used for this purpose). MCUboot checks for upgrade images in this
  partition, and can move them to ``slot0_partition`` for execution.
  The ``slot0_partition`` and ``slot1_partition`` must be the same
  size.

**scratch_partition**
  This partition is used as temporary storage while swapping the
  contents of ``slot0_partition`` and ``slot1_partition``.

.. important::

   Upgrade images are only temporarily stored in ``slot1_partition``.
   They must be linked to execute of out of ``slot0_partition``.

See the  `MCUboot documentation`_ for more details on these partitions.

.. _MCUboot: https://mcuboot.com/

.. _MCUboot documentation:
   https://github.com/runtimeco/mcuboot/blob/master/docs/design.md#image-slots

File System Partitions
======================

**storage_partition**
  This is the area where e.g. LittleFS or NVS or FCB expects its partition.
