.. _board_porting_guide:

Board Porting Guide
###################

To add Zephyr support for a new :term:`board`, you at least need a *board
directory* with various files in it. Files in the board directory inherit
support for at least one SoC and all of its features. Therefore, Zephyr must
support your :term:`SoC` as well.

Boards, SoCs, etc.
******************

Zephyr's hardware support hierarchy has these layers, from most to least
specific:

- Board: a particular CPU instance and its peripherals in a concrete hardware
  specification
- SoC: the exact system on a chip the board's CPU is part of
- SoC series: a smaller group of tightly related SoCs
- SoC family: a wider group of SoCs with similar characteristics
- CPU core: a particular CPU in an architecture
- Architecture: an instruction set architecture

You can visualize the hierarchy like this:

.. figure:: board/hierarchy.png
   :width: 500px
   :align: center
   :alt: Configuration Hierarchy

   Configuration Hierarchy

Here are some examples. Notice how the SoC series and family levels are
not always used.

.. list-table::
   :header-rows: 1

   * - Board
     - SoC
     - SoC series
     - SoC family
     - CPU core
     - Architecture
   * - :ref:`nrf52dk_nrf52832 <nrf52dk_nrf52832>`
     - nRF52832
     - nRF52
     - Nordic nRF5
     - Arm Cortex-M4
     - Arm
   * - :ref:`frdm_k64f <frdm_k64f>`
     - MK64F12
     - Kinetis K6x
     - NXP Kinetis
     - Arm Cortex-M4
     - Arm
   * - :ref:`stm32h747i_disco <stm32h747i_disco_board>`
     - STM32H747XI
     - STM32H7
     - STMicro STM32
     - Arm Cortex-M7
     - Arm
   * - :ref:`rv32m1_vega_ri5cy <rv32m1_vega>`
     - RV32M1
     - (Not used)
     - (Not used)
     - RI5CY
     - RISC-V

Make sure your SoC is supported
*******************************

Start by making sure your SoC is supported by Zephyr. If it is, it's time to
:ref:`create-your-board-directory`. If you don't know, try:

- checking :ref:`boards` for names that look relevant, and reading individual
  board documentation to find out for sure.
- asking your SoC vendor

If you need to add SoC, CPU core, or even architecture support, this is the
wrong page, but here is some general advice.

Architecture
============

See :ref:`architecture_porting_guide`.

CPU Core
========

CPU core support files go in ``core`` subdirectories under :zephyr_file:`arch`,
e.g. :zephyr_file:`arch/x86/core`.

See :ref:`gs_toolchain` for information about toolchains (compiler, linker,
etc.) supported by Zephyr. If you need to support a new toolchain,
:ref:`build_overview` is a good place to start learning about the build system.
Please reach out to the community if you are looking for advice or want to
collaborate on toolchain support.

SoC
===

Zephyr SoC support files are in architecture-specific subdirectories of
:zephyr_file:`soc`. They are generally grouped by SoC family.

When adding a new SoC family or series for a vendor that already has SoC
support within Zephyr, please try to extract common functionality into shared
files to avoid duplication. If there is no support for your vendor yet, you can
add it in a new directory ``zephyr/soc/<YOUR-ARCH>/<YOUR-SOC>``; please use
self-explanatory directory names.

.. _create-your-board-directory:

Create your board directory
***************************

Once you've found an existing board that uses your SoC, you can usually start
by copy/pasting its board directory and changing its contents for your
hardware.

You need to give your board a unique name. Run ``west boards`` for a list of
names that are already taken, and pick something new. Let's say your board is
called ``plank`` (please don't actually use that name).

Start by creating the board directory ``zephyr/boards/<ARCH>/plank``, where
``<ARCH>`` is your SoC's architecture subdirectory. (You don't have to put your
board directory in the zephyr repository, but it's the easiest way to get
started. See :ref:`custom_board_definition` for documentation on moving your
board directory to a separate repository once it's working.)

Your board directory should look like this:

.. code-block:: none

   boards/<ARCH>/plank
   ├── board.cmake
   ├── CMakeLists.txt
   ├── doc
   │   ├── plank.png
   │   └── index.rst
   ├── Kconfig.board
   ├── Kconfig.defconfig
   ├── plank_defconfig
   ├── plank.dts
   └── plank.yaml

Replace ``plank`` with your board's name, of course.

The mandatory files are:

#. :file:`plank.dts`: a hardware description in :ref:`devicetree
   <dt-guide>` format. This declares your SoC, connectors, and any
   other hardware components such as LEDs, buttons, sensors, or communication
   peripherals (USB, BLE controller, etc).

#. :file:`Kconfig.board`, :file:`Kconfig.defconfig`, :file:`plank_defconfig`:
   software configuration in :ref:`kconfig` formats. This provides default
   settings for software features and peripheral drivers.

The optional files are:

- :file:`board.cmake`: used for :ref:`flash-and-debug-support`
- :file:`CMakeLists.txt`: if you need to add additional source files to
  your build.
- :file:`doc/index.rst`, :file:`doc/plank.png`: documentation for and a picture
  of your board. You only need this if you're :ref:`contributing-your-board` to
  Zephyr.
- :file:`plank.yaml`: a YAML file with miscellaneous metadata used by the
  :ref:`twister_script`.

.. _default_board_configuration:

Write your devicetree
*********************

The devicetree file :file:`boards/<ARCH>/plank/plank.dts` describes your board
hardware in the Devicetree Source (DTS) format (as usual, change ``plank`` to
your board's name). If you're new to devicetree, see :ref:`devicetree-intro`.

In general, :file:`plank.dts` should look like this:

.. code-block:: devicetree

   /dts-v1/;
   #include <your_soc_vendor/your_soc.dtsi>

   / {
   	model = "A human readable name";
   	compatible = "yourcompany,plank";

   	chosen {
   		zephyr,console = &your_uart_console;
   		zephyr,sram = &your_memory_node;
   		/* other chosen settings  for your hardware */
   	};

   	/*
   	 * Your board-specific hardware: buttons, LEDs, sensors, etc.
   	 */

   	leds {
   		compatible = "gpio-leds";
   		led0: led_0 {
   			gpios = < /* GPIO your LED is hooked up to */ >;
   			label = "LED 0";
   		};
   		/* ... other LEDs ... */
   	};

   	buttons {
   		compatible = "gpio-keys";
   		/* ... your button definitions ... */
   	};

   	/* These aliases are provided for compatibility with samples */
   	aliases {
   		led0 = &led0; /* now you support the blinky sample! */
   		/* other aliases go here */
   	};
   };

   &some_peripheral_you_want_to_enable { /* like a GPIO or SPI controller */
   	status = "okay";
   };

   &another_peripheral_you_want {
   	status = "okay";
   };

If you're in a hurry, simple hardware can usually be supported by copy/paste
followed by trial and error. If you want to understand details, you will need
to read the rest of the devicetree documentation and the devicetree
specification.

.. _dt_k6x_example:

Example: FRDM-K64F and Hexiwear K64
===================================

.. Give the filenames instead of the full paths below, as it's easier to read.
   The cramped 'foo.dts<path>' style avoids extra spaces before commas.

This section contains concrete examples related to writing your board's
devicetree.

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

.. code-block:: devicetree

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

.. code-block:: devicetree

   &can0 {
	status = "okay";
	bus-speed = <125000>;
   };

The ``&can0 { ... };`` syntax adds/overrides properties on the node with label
``can0``, i.e. the ``can@4002400`` node defined in the :file:`.dtsi` file.

Other examples of board-specific customization is pointing properties in
``aliases`` and ``chosen`` to the right nodes (see :ref:`dt-alias-chosen`), and
making GPIO/pinmux assignments.

Write Kconfig files
*******************

Zephyr uses the Kconfig language to configure software features. Your board
needs to provide some Kconfig settings before you can compile a Zephyr
application for it.

Setting Kconfig configuration values is documented in detail in
:ref:`setting_configuration_values`.

There are three mandatory Kconfig files in the board directory for a board
named ``plank``:

.. code-block:: none

   boards/<ARCH>/plank
   ├── Kconfig.board
   ├── Kconfig.defconfig
   └── plank_defconfig

:file:`Kconfig.board`
  Included by :zephyr_file:`boards/Kconfig` to include your board
  in the list of options.

  This should at least contain a definition for a ``BOARD_PLANK`` option,
  which looks something like this:

  .. code-block:: none

     config BOARD_PLANK
     	bool "Plank board"
     	depends on SOC_SERIES_YOUR_SOC_SERIES_HERE
     	select SOC_PART_NUMBER_ABCDEFGH

:file:`Kconfig.defconfig`
  Board-specific default values for Kconfig options.

  The entire file should be inside an ``if BOARD_PLANK`` / ``endif`` pair of
  lines, like this:

  .. code-block:: none

     if BOARD_PLANK

     # Always set CONFIG_BOARD here. This isn't meant to be customized,
     # but is set as a "default" due to Kconfig language restrictions.
     config BOARD
     	default "plank"

     # Other options you want enabled by default go next. Examples:

     config FOO
     	default y

     if NETWORKING
     config SOC_ETHERNET_DRIVER
     	default y
     endif # NETWORKING

     endif # BOARD_PLANK

:file:`plank_defconfig`
  A Kconfig fragment that is merged as-is into the final build directory
  :file:`.config` whenever an application is compiled for your board.

  You should at least select your board's SOC and do any mandatory settings for
  your system clock, console, etc. The results are architecture-specific, but
  typically look something like this:

  .. code-block:: none

     CONFIG_SOC_${VENDOR_XYZ3000}=y                      /* select your SoC */
     CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=120000000   /* set up your clock, etc */
     CONFIG_SERIAL=y

:file:`plank_x_y_z.conf`
  A Kconfig fragment that is merged as-is into the final build directory
  :file:`.config` whenever an application is compiled for your board revision
  ``x.y.z``.

Build, test, and fix
********************

Now it's time to build and test the application(s) you want to run on your
board until you're satisfied.

For example:

.. code-block:: console

   west build -b plank samples/hello_world
   west flash

For ``west flash`` to work, see :ref:`flash-and-debug-support` below. You can
also just flash :file:`build/zephyr/zephyr.elf`, :file:`zephyr.hex`, or
:file:`zephyr.bin` with any other tools you prefer.

.. _porting-general-recommendations:

General recommendations
***********************

For consistency and to make it easier for users to build generic applications
that are not board specific for your board, please follow these guidelines
while porting.

- Unless explicitly recommended otherwise by this section, leave peripherals
  and their drivers disabled by default.

- Configure and enable a system clock, along with a tick source.

- Provide pin and driver configuration that matches the board's valuable
  components such as sensors, buttons or LEDs, and communication interfaces
  such as USB, Ethernet connector, or Bluetooth/Wi-Fi chip.

- If your board uses a well-known connector standard (like Arduino, Mikrobus,
  Grove, or 96Boards connectors), add connector nodes to your DTS and configure
  pin muxes accordingly.

- Configure components that enable the use of these pins, such as
  configuring an SPI instance to use the usual Arduino SPI pins.

- If available, configure and enable a serial output for the console
  using the ``zephyr,console`` chosen node in the devicetree.

- If your board supports networking, configure a default interface.

- Enable all GPIO ports connected to peripherals or expansion connectors.

- If available, enable pinmux and interrupt controller drivers.

- It is recommended to enable the MPU by default, if there is support for it
  in hardware. For boards with limited memory resources it is acceptable to
  disable it. When the MPU is enabled, it is recommended to also enable
  hardware stack protection (CONFIG_HW_STACK_PROTECTION=y) and, thus, allow the
  kernel to detect stack overflows when the system is running in privileged
  mode.

.. _flash-and-debug-support:

Flash and debug support
***********************

Zephyr supports :ref:`west-build-flash-debug` via west extension commands.

To add ``west flash`` and ``west debug`` support for your board, you need to
create a :file:`board.cmake` file in your board directory. This file's job is
to configure a "runner" for your board. (There's nothing special you need to
do to get ``west build`` support for your board.)

"Runners" are Zephyr-specific Python classes that wrap :ref:`flash and debug
host tools <flash-debug-host-tools>` and integrate with west and the zephyr build
system to support ``west flash`` and related commands. Each runner supports
flashing, debugging, or both. You need to configure the arguments to these
Python scripts in your :file:`board.cmake` to support those commands like this
example :file:`board.cmake`:

.. code-block:: cmake

   board_runner_args(jlink "--device=nrf52" "--speed=4000")
   board_runner_args(pyocd "--target=nrf52" "--frequency=4000000")

   include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
   include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
   include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

This example configures the ``nrfjprog``, ``jlink``, and ``pyocd`` runners.

.. warning::

   Runners usually have names which match the tools they wrap, so the ``jlink``
   runner wraps Segger's J-Link tools, and so on. But the runner command line
   options like ``--speed`` etc. are specific to the Python scripts.

For more details:

- Run ``west flash --context`` to see a list of available runners which support
  flashing, and ``west flash --context -r <RUNNER>`` to view the specific options
  available for an individual runner.
- Run ``west debug --context`` and ``west debug --context <RUNNER>`` to get
  the same output for runners which support debugging.
- Run ``west flash --help`` and ``west debug --help`` for top-level options
  for flashing and debugging.
- See :ref:`west-runner` for Python APIs.
- Look for :file:`board.cmake` files for other boards similar to your own for
  more examples.

To see what a ``west flash`` or ``west debug`` command is doing exactly, run it
in verbose mode:

.. code-block:: sh

   west --verbose flash
   west --verbose debug

Verbose mode prints any host tool commands the runner uses.

The order of the ``include()`` calls in your :file:`board.cmake` matters. The
first ``include`` sets the default runner if it's not already set. For example,
including ``nrfjprog.board.cmake`` first means that ``nrfjprog`` is the default
flash runner for this board. Since ``nrfjprog`` does not support debugging,
``jlink`` is the default debug runner.

.. _porting_board_revisions:

Multiple board revisions
************************

See :ref:`application_board_version` for basics on this feature from the user
perspective.

To create a new board revision for the ``plank`` board, create these additional
files in the board folder:

.. code-block:: none

   boards/<ARCH>/plank
   ├── plank_<revision>.conf     # optional
   ├── plank_<revision>.overlay  # optional
   └── revision.cmake

When the user builds for board ``plank@<revision>``:

- The optional Kconfig settings specified in the file
  :file:`plank_<revision>.conf` will be merged into the board's default Kconfig
  configuration.

- The optional devicetree overlay :file:`plank_<revision>.overlay` will be added
  to the common :file:`plank.dts` devicetree file

- The :file:`revision.cmake` file controls how the Zephyr build system matches
  the ``<board>@<revision>`` string specified by the user when building an
  application for the board.

Currently, ``<revision>`` can be either a numeric ``MAJOR.MINOR.PATCH`` style
revision like ``1.5.0``, an integer number like ``1``, or single letter like
``A``, ``B``, etc. Zephyr provides a CMake board extension function,
``board_check_revision()``, to make it easy to match either style from
:file:`revision.cmake`.

Valid board revisions may be specified as arguments to the
``board_check_revision()`` function, like:

.. code-block:: cmake

   board_check_revision(FORMAT MAJOR.MINOR.PATCH
                        VALID_REVISIONS 0.1.0 0.3.0 ...
   )

.. note::
   ``VALID_REVISIONS`` can be omitted if all valid revisions have specific
   Kconfig fragments, such as ``<board>_0_1_0.conf``, ``<board>_0_3_0.conf``.
   This allows you to just place Kconfig revision fragments in the board
   folder and not have to keep the corresponding ``VALID_REVISIONS`` in sync.

The following sections describe how to support these styles of revision
numbers.

MAJOR.MINOR.PATCH revisions
===========================

Let's say you want to add support for revisions ``0.5.0``, ``1.0.0``, and
``1.5.0`` of the ``plank`` board with both Kconfig fragments and devicetree
overlays. Create :file:`revision.cmake` with
``board_check_revision(FORMAT MAJOR.MINOR.PATCH)``, and create the following
additional files in the board directory:

.. code-block:: none

   boards/<ARCH>/plank
   ├── plank_0_5_0.conf
   ├── plank_0_5_0.overlay
   ├── plank_1_0_0.conf
   ├── plank_1_0_0.overlay
   ├── plank_1_5_0.conf
   ├── plank_1_5_0.overlay
   └── revision.cmake

Notice how the board files have changed periods (".") in the revision number to
underscores ("_").

Fuzzy revision matching
-----------------------

To support "fuzzy" ``MAJOR.MINOR.PATCH`` revision matching for the ``plank``
board, use the following code in :file:`revision.cmake`:

.. code-block:: cmake

   board_check_revision(FORMAT MAJOR.MINOR.PATCH)

If the user selects a revision between those available, the closest revision
number that is not larger than the user's choice is used. For example, if the
user builds for ``plank@0.7.0``, the build system will target revision
``0.5.0``.

The build system will print this at CMake configuration time:

.. code-block:: console

   -- Board: plank, Revision: 0.7.0 (Active: 0.5.0)

This allows you to only create revision configuration files for board revision
numbers that introduce incompatible changes.

Any revision less than the minimum defined will be treated as an error.

You may use ``0.0.0`` as a minimum revision to build for by creating the file
:file:`plank_0_0_0.conf` in the board directory. This will be used for any
revision lower than ``0.5.0``, for example if the user builds for
``plank@0.1.0``.

Exact revision matching
-----------------------

Alternatively, the ``EXACT`` keyword can be given to ``board_check_revision()``
in :file:`revision.cmake` to allow exact matches only, like this:

.. code-block:: cmake

   board_check_revision(FORMAT MAJOR.MINOR.PATCH EXACT)

With this :file:`revision.cmake`, building for ``plank@0.7.0`` in the above
example will result in the following error message:

.. code-block:: console

   Board revision `0.7.0` not found.  Please specify a valid board revision.

Letter revision matching
========================

Let's say instead that you need to support revisions ``A``, ``B``, and ``C`` of
the ``plank`` board. Create the following additional files in the board
directory:

.. code-block:: none

   boards/<ARCH>/plank
   ├── plank_A.conf
   ├── plank_A.overlay
   ├── plank_B.conf
   ├── plank_B.overlay
   ├── plank_C.conf
   ├── plank_C.overlay
   └── revision.cmake

And add the following to :file:`revision.cmake`:

.. code-block:: cmake

   board_check_revision(FORMAT LETTER)

Number revision matching
========================

Let's say instead that you need to support revisions ``1``, ``2``, and ``3`` of
the ``plank`` board. Create the following additional files in the board
directory:

.. code-block:: none

   boards/<ARCH>/plank
   ├── plank_1.conf
   ├── plank_1.overlay
   ├── plank_2.conf
   ├── plank_2.overlay
   ├── plank_3.conf
   ├── plank_3.overlay
   └── revision.cmake

And add the following to :file:`revision.cmake`:

.. code-block:: cmake

   board_check_revision(FORMAT NUMBER)

board_check_revision() details
==============================

.. code-block:: cmake

   board_check_revision(FORMAT <LETTER | NUMBER | MAJOR.MINOR.PATCH>
                        [EXACT]
                        [DEFAULT_REVISION <revision>]
                        [HIGHEST_REVISION <revision>]
                        [VALID_REVISIONS <revision> [<revision> ...]]
   )

This function supports the following arguments:

* ``FORMAT LETTER``: matches single letter revisions from ``A`` to ``Z`` only
* ``FORMAT NUMBER``: matches integer revisions
* ``FORMAT MAJOR.MINOR.PATCH``: matches exactly three digits. The command line
  allows for loose typing, that is ``-DBOARD=<board>@1`` and
  ``-DBOARD=<board>@1.0`` will be handled as ``-DBOARD=<board>@1.0.0``.
  Kconfig fragment and devicetree overlay files must use full numbering to avoid
  ambiguity, so only :file:`<board>_1_0_0.conf` and
  :file:`<board>_1_0_0.overlay` are allowed.

* ``EXACT``: if given, the revision is required to be an exact match.
  Otherwise, the closest matching revision not greater than the user's choice
  will be selected.

* ``DEFAULT_REVISION <revision>``: if given, ``<revision>`` is the default
  revision to use when user has not selected a revision number. If not given,
  the build system prints an error when the user does not specify a board
  revision.

* ``HIGHEST_REVISION``: if given, specifies the highest valid revision for a
  board. This can be used to ensure that a newer board cannot be used with an
  older Zephyr. For example, if the current board directory supports revisions
  0.x.0-0.99.99 and 1.0.0-1.99.99, and it is expected that the implementation
  will not work with board revision 2.0.0, then giving ``HIGHEST_REVISION
  1.99.99`` causes an error if the user builds using ``<board>@2.0.0``.

* ``VALID_REVISIONS``: if given, specifies a list of revisions that are valid
  for this board. If this argument is not given, then each Kconfig fragment of
  the form ``<board>_<revision>.conf`` in the board folder will be used as a
  valid revision for the board.

.. _porting_custom_board_revisions:

Custom revision.cmake files
***************************

Some boards may not use board revisions supported by
``board_check_revision()``. To support revisions of any type, the file
:file:`revision.cmake` can implement custom revision matching without calling
``board_check_revision()``.

To signal to the build system that it should use a different revision than the
one specified by the user, :file:`revision.cmake` can set the variable
``ACTIVE_BOARD_REVISION`` to the revision to use instead. The corresponding
Kconfig files and devicetree overlays must be named
:file:`<board>_<ACTIVE_BOARD_REVISION>.conf` and
:file:`<board>_<ACTIVE_BOARD_REVISION>.overlay`.

For example, if the user builds for ``plank@zero``, :file:`revision.cmake` can
set ``ACTIVE_BOARD_REVISION`` to ``one`` to use the files
:file:`plank_one.conf` and :file:`plank_one.overlay`.

.. _contributing-your-board:

Contributing your board
***********************

If you want to contribute your board to Zephyr, first -- thanks!

There are some extra things you'll need to do:

#. Make sure you've followed all the :ref:`porting-general-recommendations`.
   They are requirements for boards included with Zephyr.

#. Add documentation for your board using the template file
   :zephyr_file:`doc/templates/board.tmpl`. See :ref:`zephyr_doc` for
   information on how to build your documentation before submitting
   your pull request.

#. Prepare a pull request adding your board which follows the
   :ref:`contribute_guidelines`.
