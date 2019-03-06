.. _device-tree:

Device Tree
###########

.. contents::
    :local:
    :depth: 2

Introduction
************

Device tree is a way of describing hardware and configuration information
for boards.  Device tree was adopted for use in the Linux kernel for the
PowerPC architecture.  However, it is now in use for ARM and other
architectures.

The device tree is a data structure for dynamically describing hardware
using a Device Tree Source (DTS) data structure language, instead of
hard coding every detail of a board's hardware into the operating system.

In Linux, DTS is compiled into a compact Device Tree Blob (DTB) using a Device
Tree Compiler (DTC), then the hardware-describing DTB is passed to the operating
system at boot time. This allows the same compiled Linux kernel to support
different hardware configurations within an architecture family (e.g., ARM,
x86, PowerPC) and moves a significant part of the hardware description out of
the kernel binary itself.

For larger systems, the flexibility this offers offsets the extra runtime memory
overhead incurred.  But the primary targets for Zephyr
applications are small micro-controller systems with limited memory
resources.  So instead of requiring the additional runtime memory
to store the DTB blob and the code to parse it, the DTS information
is used at compile time.

Device tree uses a specific format to describe the device nodes in a system.
This format is described in the `Device Tree Specification`_.

.. _Device Tree Specification: https://github.com/devicetree-org/devicetree-specification/releases

More device tree information can be found at the `device tree repository`_.

.. _device tree repository: https://git.kernel.org/pub/scm/utils/dtc/dtc.git


System build requirements
*************************

The Zephyr device tree feature requires a device tree compiler (DTC) and Python
YAML packages.  Refer to the installation guide for your specific host OS:

* :ref:`installing_zephyr_win`
* :ref:`installation_linux`
* :ref:`installing_zephyr_mac`


Zephyr and Device Tree
**********************

Device Tree provides a unified description of a hardware system used in an
application. It is used in Zephyr to describe hardware and provide a boot-time
configuration of this hardware.

In Zephyr, the device tree is also used to describe Zephyr-specific
configuration information.  This Zephyr-specific information augments the device
tree descriptions and sits on top of it, rather than diverging from it.  The
main reason for this is to leverage existing device tree files that a SoC vendor
may already have defined for a given platform.

The device tree files are compiled using the device tree compiler.  The compiler
runs the .dts file through the C preprocessor to resolve any macro or #defines
utilized in the file.  The output of the compile is another dts formatted file.

After compilation, a python script extracts information from the compiled device
tree file using a set of rules specified in YAML binding files.  The extracted
information is placed in a header file that is used by the rest of the code as
the project is compiled.

Temporary fixup files are required for device tree support on most devices.
These fixup files by default reside in the board and soc directories and are
named ``dts_fixup.h``.  These fixup files map the generated include information to
the current driver/source usage.

.. _dt_vs_kconfig:

Device Tree vs Kconfig
**********************

As mentioned above there are several tools used to configure the build with
Zephyr.
The two main ones, Device Tree and Kconfig, might seem to overlap in purpose,
but in fact they do not. This section serves as a reference to help you decide
whether to place configuration items in Device Tree or Kconfig.

The scope of each configuration tool can be summarized as follows:

* Device Tree is used to describe the **hardware** and its **boot-time
  configuration**.
* Kconfig is used to describe which **software features** will be built into
  the final image, and their **configuration**.

Hence Device Tree deals mostly with hardware and Kconfig with software.
A couple of noteworthy exceptions are:

* Device Tree's ``chosen`` keyword, which allows the user to select a
  particular instance of a hardware device to be used for a concrete purpose
  by the software. An example of this is selecting a particular UART for use as
  the system's console.
* Device Tree's ``status`` keyword, which allows the user to enable or disable
  a particular instance of a hardware device in the Device Tree file itself.
  This takes precedence over Kconfig.

To further clarify this separation, let's use a particular, well-known
example to illustrate this: serial ports a.k.a. UARTs. Let's consider a
board containing a SoC with 2 UART instances:

* The fact that the target hardware **contains** 2 UARTs is described with Device
  Tree. This includes the UART type, its driver compatibility and certain
  immutable (i.e. not software-configurable) settings such as the base address
  of the hardware peripheral in memory or its interrupt line.
* Additionally, **hardware boot-time configuration** is also described with Device
  Tree. This includes things such as the IRQ priority and boot-time UART
  baudrate. These may also be modifiable at runtime later, but their boot-time
  default configuration is described in Device Tree.
* The fact that the user intends to include **software support** for UART in the
  build is described in Kconfig. Through the use of Kconfig, users can opt not
  to include support for this particular hardware item if they don't require it.

Another example is a device with a 2.4GHz, multi-protocol radio supporting
both the Bluetooth Low Energy and 802.15.4 wireless technologies. In this case:

* Device Tree describes the presence of a radio peripheral compatible with a
  certain radio driver.
* Additional hardware boot-time configuration settings may also be present
  in the Device Tree files. In this particular case it could be a
  default TX power in dBm if the hardware does have a simple, cross-wireless
  technology register for that.
* Kconfig will describe which **protocol stack** is to be used with that radio.
  The user may decide to select BLE or 802.15.4, which will both depend on
  the presence of a compatible radio in the Device Tree files.

Device tree file formats
************************

Hardware and software is described inside of device tree files in clear text format.
These files have the file suffix of .dtsi or .dts.  The .dtsi files are meant to
be included by other files.  Typically for a given board you have some number of
.dtsi include files that pull in common device descriptions that are used across
a given SoC family.

Example: FRDM K64F Board and Hexiwear K64
=========================================

Both of these boards are based on the same NXP Kinetis SoC family, the K6X.  The
following shows the include hierarchy for both boards.

boards/arm/frdm_k64f/frdm_k64f.dts includes the following files::

  dts/arm/nxp/nxp_k6x.dtsi
  dts/arm/armv7-m.dtsi

boards/arm/hexiwear_k64/hexiwear_k64.dts includes the same files::

  dts/arm/nxp/nxp_k6x.dtsi
  dts/arm/armv7-m.dtsi

The board-specific .dts files enable nodes, define the Zephyr-specific items,
and also adds board-specific changes like gpio/pinmux assignments.  These types
of things will vary based on the board layout and application use.

Currently supported boards
**************************

Device tree is currently supported on all embedded targets except posix
(boards/posix) and qemu_x86_64 (boards/x86_64).

Adding support for a board
**************************

Adding device tree support for a given board requires adding a number of files.
These files will contain the DTS information that describes a platform, the
YAML descriptions that define the contents of a given Device Tree peripheral
node, and also any fixup files required to support the platform.

When writing Device Tree Source files, it is good to separate out common
peripheral information that could be used across multiple SoC families or
boards.  DTS files are identified by their file suffix.  A .dtsi suffix denotes
a DTS file that is used as an include in another DTS file.  A .dts suffix
denotes the primary DTS file for a given board.

The primary DTS file will contain at a minimum a version line, optional
includes, and the root node definition.  The root node will contain a model and
compatible that denotes the unique board described by the .dts file.

Device Tree Source File Template
================================

.. code-block:: none

  /dts-v1/;

  / {
          model = "Model name for your board";
          compatible = "compatible for your board";
          /* rest of file */
  };


One suggestion for starting from scratch on a platform/board is to examine other
boards and their device tree source files.

The following is a more precise list of required files:

* Base architecture support

  * Add architecture-specific DTS directory, if not already present.
    Example: dts/arm for ARM.
  * Add target specific device tree files for base SoC.  These should be
    .dtsi files to be included in the board-specific device tree files.
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
    described above. They can be used to modify Device Tree configurations
    without having to change the SoC and Board files. See
    :ref:`application_dt` for more information on overlay files and the Zephyr
    build system.

.. _dt-alias-chosen:

``aliases`` and ``chosen`` nodes
================================

Using an alias with a common name for a particular node makes it easier for you
to write board-independent source code. Device Tree ``aliases`` nodes  are used
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
different boards, which makes using the Device Tree ``chosen`` nodes very
convenient. By doing, so the software subsystem can rely on having the specific
hardware peripheral assigned to it. In the following example we bind the shell
to ``uart1`` in this board:

.. code-block:: yaml

   chosen {
      zephyr,shell-uart = &uart1;
   };

The full set of Zephyr-specific ``chosen`` nodes follows:

.. list-table::
   :header-rows: 1

   * - ``chosen`` node name
     - Generated symbol

   * - ``zephyr,flash``
     - ``CONFIG_FLASH``
   * - ``zephyr,sram``
     - ``CONFIG_SRAM_SIZE``/``CONFIG_SRAM_BASE_ADDRESS``
       (via ``DT_SRAM_SIZE``/``DT_SRAM_BASE_ADDRESS``)
   * - ``zephyr,ccm``
     - ``DT_CCM``
   * - ``zephyr,console``
     - ``DT_UART_CONSOLE_ON_DEV_NAME``
   * - ``zephyr,shell-uart``
     - ``DT_UART_SHELL_ON_DEV_NAME``
   * - ``zephyr,bt-uart``
     - ``DT_BT_UART_ON_DEV_NAME``
   * - ``zephyr,uart-pipe``
     - ``DT_UART_PIPE_ON_DEV_NAME``
   * - ``zephyr,bt-mon-uart``
     - ``DT_BT_MONITOR_ON_DEV_NAME``
   * - ``zephyr,uart-mcumgr``
     - ``DT_UART_MCUMGR_ON_DEV_NAME``

As chosen properties tend to be related to software configuration, it can be
useful for the build system to know if a chosen property was defined. We
generate a define for each chosen property, for example:

``zephyr,flash`` will generate: ``#define DT_CHOSEN_ZEPHYR_FLASH 1``

As a consequence ``zephyr,flash`` related code could safely be implemented
under ``#ifdef DT_CHOSEN_ZEPHYR_FLASH`` instruction.

Adding support for device tree in drivers
*****************************************

As drivers and other source code is converted over to make use of device tree
generated information, these drivers may require changes to match the generated
#define information.


Source Tree Hierarchy
*********************

The device tree files are located in a couple of different directories.  The
directory split is done based on architecture, and there is also a common
directory where architecture agnostic device tree and YAML binding files are
located.

Assuming the current working directory is the ZEPHYR_BASE, the directory
hierarchy looks like the following::

  dts/common/
  dts/<ARCH>/
  dts/bindings/
  boards/<ARCH>/<BOARD>/

The common directory contains a ``skeleton.dtsi`` which provides device tree root
node definition.  The bindings subdirectory contains YAML binding files used
to instruct how the python DTS parsing script should extract nodes information
in a format that will be usable by the system.

Example: Subset of DTS/YAML files for NXP FRDM K64F (Subject to Change)::

  dts/arm/armv7-m.dtsi
  dts/arm/k6x/nxp_k6x.dtsi
  boards/arm/frdm_k64f/frdm_k64f.dts
  dts/bindings/interrupt-controller/arm,v7m-nvic.yaml
  dts/bindings/gpio/nxp,kinetis-gpio.yaml
  dts/bindings/pinctrl/nxp,kinetis-pinmux.yaml
  dts/bindings/serial/nxp,kinetis-uart.yaml

YAML bindings for device nodes
******************************

Device tree describes hardware and configuration, but it doesn't tell the
system which pieces of information are useful, or how to generate configuration
data from the device tree nodes.  For this, we rely on YAML binding files to
describe the contents or definition of a device tree node and instruct how the
extracted information should be formatted.

A YAML description (called "YAML binding") must be provided for every device node
that is a source of information for the system.  A YAML binding file
is associated to each node `compatible` property.  Information within the YAML
file will instruct the python DTS parsing script (located in ``scripts/dts``) how
each property of the node is expected to be generated, either the type of the
value or the format of its name.  Node properties are generated as C-style
``#define``'s in include files
made available to all Zephyr components.

A YAML template file is provided to show the required format.  This file is
located at::

  dts/bindings/device_node.yaml.template

YAML files must end in a .yaml suffix.  YAML files are scanned during the
information extraction phase and are matched to device tree nodes via the
compatible property.


Include files generation
************************

At build time, after a board's ``.dts`` file has been processed by the DTC
(Device Tree Compiler), a corresponding ``.dts_compiled`` file is generated
under the ``zephyr`` directory.
This ``.dts_compiled`` file is processed by the python DTS parsing script
and generates an include file named
``include/generated/generated_dts_board_unfixed.h``
that holds all the information extracted from the DTS file with
the format specified by the YAML bindings.  For example:

.. code-block:: c

   /* gpio_keys */
   #define DT_GPIO_KEYS_0		1

   /* button_0 */
   #define DT_GPIO_KEYS_BUTTON_0_GPIO_CONTROLLER	"GPIO_2"
   #define DT_GPIO_KEYS_BUTTON_0_GPIO_FLAGS	0
   #define DT_GPIO_KEYS_BUTTON_0_GPIO_PIN		6
   #define DT_GPIO_KEYS_BUTTON_0_LABEL		"User SW2"
   #define DT_GPIO_KEYS_SW1_GPIO_CONTROLLER	DT_GPIO_KEYS_BUTTON_0_GPIO_CONTROLLER
   #define DT_GPIO_KEYS_SW1_GPIO_FLAGS		DT_GPIO_KEYS_BUTTON_0_GPIO_FLAGS
   #define DT_GPIO_KEYS_SW1_GPIO_PIN		DT_GPIO_KEYS_BUTTON_0_GPIO_PIN
   #define DT_GPIO_KEYS_SW1_LABEL			DT_GPIO_KEYS_BUTTON_0_LABEL
   #define SW1_GPIO_CONTROLLER			DT_GPIO_KEYS_BUTTON_0_GPIO_CONTROLLER
   #define SW1_GPIO_FLAGS				DT_GPIO_KEYS_BUTTON_0_GPIO_FLAGS
   #define SW1_GPIO_PIN				DT_GPIO_KEYS_BUTTON_0_GPIO_PIN
   #define SW1_LABEL				DT_GPIO_KEYS_BUTTON_0_LABEL

Additionally, a file named ``generated_dts_board_fixups.h`` is
generated in the same directory concatenating all board-related fixup files.

The include file ``include/generated_dts_board.h`` includes both these generated
files, giving Zephyr C source files access to the board's device tree information.

.. include:: flash_partitions.inc
