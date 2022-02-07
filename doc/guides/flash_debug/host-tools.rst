.. _flash-debug-host-tools:

Flash & Debug Host Tools
########################

This guide describes the software tools you can run on your host workstation to
flash and debug Zephyr applications.

Zephyr's west tool has built-in support for all of these in its ``flash``,
``debug``, ``debugserver``, and ``attach`` commands, provided your board
hardware supports them and your Zephyr board directory's :file:`board.cmake`
file declares that support properly. See :ref:`west-build-flash-debug` for
more information on these commands.

.. _atmel_sam_ba_bootloader:


SAM Boot Assistant (SAM-BA)
***************************

Atmel SAM Boot Assistant (Atmel SAM-BA) allows In-System Programming (ISP)
from USB or UART host without any external programming interface.  Zephyr
allows users to develop and program boards with SAM-BA support using
:ref:`west <west-flashing>`.  Zephyr supports devices with/without ROM
bootloader and both extensions from Arduino and Adafruit. Full support was
introduced in Zephyr SDK 0.12.0.

The typical command to flash the board is:

.. code-block:: console

	west flash [ -r bossac ] [ -p /dev/ttyX ]

Flash configuration for devices:

.. tabs::

    .. tab:: With ROM bootloader

        These devices don't need any special configuration.  After building your
        application, just run ``west flash`` to flash the board.

    .. tab:: Without ROM bootloader

        For these devices, the user should:

        1. Define flash partitions required to accommodate the bootloader and
           application image; see :ref:`flash_map_api` for details.
        2. Have board :file:`.defconfig` file with the
           :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option set to ``y`` to
           instruct the build system to use these partitions for code relocation.
           This option can also be set in ``prj.conf`` or any other Kconfig fragment.
        3. Build and flash the SAM-BA bootloader on the device.

    .. tab:: With compatible SAM-BA bootloader

        For these devices, the user should:

        1. Define flash partitions required to accommodate the bootloader and
           application image; see :ref:`flash_map_api` for details.
        2. Have board :file:`.defconfig` file with the
           :kconfig:option:`CONFIG_BOOTLOADER_BOSSA` Kconfig option set to ``y``.  This will
           automatically select the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig
           option which instruct the build system to use these partitions for code
           relocation.  The board :file:`.defconfig` file should have
           :kconfig:option:`CONFIG_BOOTLOADER_BOSSA_ARDUINO` ,
           :kconfig:option:`CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2` or the
           :kconfig:option:`CONFIG_BOOTLOADER_BOSSA_LEGACY` Kconfig option set to ``y``
           to select the right compatible SAM-BA bootloader mode.
           These options can also be set in ``prj.conf`` or any other Kconfig fragment.
        3. Build and flash the SAM-BA bootloader on the device.

.. note::

    The :kconfig:option:`CONFIG_BOOTLOADER_BOSSA_LEGACY` Kconfig option should be used
    as last resource.  Try configure first with Devices without ROM bootloader.


Typical flash layout and configuration
--------------------------------------

For bootloaders that reside on flash, the devicetree partition layout is
mandatory.  For devices that have a ROM bootloader, they are mandatory when
the application uses a storage or other non-application partition.  In this
special case, the boot partition should be omitted and code_partition should
start from offset 0.  It is necessary to define the partitions with sizes that
avoid overlaps, always.

A typical flash layout for devices without a ROM bootloader is:

.. code-block:: devicetree

	/ {
		chosen {
			zephyr,code-partition = &code_partition;
		};
	};

	&flash0 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			boot_partition: partition@0 {
				label = "sam-ba";
				reg = <0x00000000 0x2000>;
				read-only;
			};

			code_partition: partition@2000 {
				label = "code";
				reg = <0x2000 0x3a000>;
				read-only;
			};

			/*
			* The final 16 KiB is reserved for the application.
			* Storage partition will be used by FCB/LittleFS/NVS
			* if enabled.
			*/
			storage_partition: partition@3c000 {
				label = "storage";
				reg = <0x0003c000 0x00004000>;
			};
		};
	};

A typical flash layout for devices with a ROM bootloader and storage
partition is:

.. code-block:: devicetree

	/ {
		chosen {
			zephyr,code-partition = &code_partition;
		};
	};

	&flash0 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			code_partition: partition@0 {
				label = "code";
				reg = <0x0 0xF0000>;
				read-only;
			};

			/*
			* The final 64 KiB is reserved for the application.
			* Storage partition will be used by FCB/LittleFS/NVS
			* if enabled.
			*/
			storage_partition: partition@F0000 {
				label = "storage";
				reg = <0x000F0000 0x00100000>;
			};
		};
	};


Enabling SAM-BA runner
----------------------

In order to instruct Zephyr west tool to use the SAM-BA bootloader the
:file:`board.cmake` file must have
``include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)`` entry.  Note that
Zephyr tool accept more entries to define multiple runners.  By default, the
first one will be selected when using ``west flash`` command.  The remaining
options are available passing the runner option, for instance
``west flash -r bossac``.


More implementation details can be found in the :ref:`boards` documentation.
As a quick reference, see these three board documentation pages:

  - :ref:`sam4e_xpro` (ROM bootloader)
  - :ref:`adafruit_feather_m0_basic_proto` (Adafruit UF2 bootloader)
  - :ref:`arduino_nano_33_iot` (Arduino bootloader)
  - :ref:`arduino_nano_33_ble` (Arduino legacy bootloader)

.. _jlink-debug-host-tools:

J-Link Debug Host Tools
***********************

Segger provides a suite of debug host tools for Linux, macOS, and Windows
operating systems:

- J-Link GDB Server: GDB remote debugging
- J-Link Commander: Command-line control and flash programming
- RTT Viewer: RTT terminal input and output
- SystemView: Real-time event visualization and recording

These debug host tools are compatible with the following debug probes:

- :ref:`lpclink2-jlink-onboard-debug-probe`
- :ref:`opensda-jlink-onboard-debug-probe`
- :ref:`jlink-external-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `J-Link Supported Devices`_.

Download and install the `J-Link Software and Documentation Pack`_ to get the
J-Link GDB Server and Commander, and to install the associated USB device
drivers. RTT Viewer and SystemView can be downloaded separately, but are not
required.

Note that the J-Link GDB server does not yet support Zephyr RTOS-awareness.

.. _openocd-debug-host-tools:

OpenOCD Debug Host Tools
************************

OpenOCD is a community open source project that provides GDB remote debugging
and flash programming support for a wide range of SoCs. A fork that adds Zephyr
RTOS-awareness is included in the Zephyr SDK; otherwise see `Getting OpenOCD`_
for options to download OpenOCD from official repositories.

These debug host tools are compatible with the following debug probes:

- :ref:`opensda-daplink-onboard-debug-probe`
- :ref:`jlink-external-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `OpenOCD Supported Devices`_.

.. note:: On Linux, openocd is available though the `Zephyr SDK
   <https://github.com/zephyrproject-rtos/sdk-ng/releases>`_.
   Windows users should use the following steps to install
   openocd:

   - Download openocd for Windows from here: `OpenOCD Windows`_
   - Copy bin and share dirs to ``C:\Program Files\OpenOCD\``
   - Add ``C:\Program Files\OpenOCD\bin`` to 'PATH' environment variable

.. _pyocd-debug-host-tools:

pyOCD Debug Host Tools
**********************

pyOCD is an open source project from Arm that provides GDB remote debugging and
flash programming support for Arm Cortex-M SoCs. It is distributed on PyPi and
installed when you complete the :ref:`gs_python_deps` step in the Getting
Started Guide. pyOCD includes support for Zephyr RTOS-awareness.

These debug host tools are compatible with the following debug probes:

- :ref:`opensda-daplink-onboard-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `pyOCD Supported Devices`_.

.. _J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack

.. _J-Link Supported Devices:
   https://www.segger.com/downloads/supported-devices.php

.. _Getting OpenOCD:
   http://openocd.org/getting-openocd/

.. _OpenOCD Supported Devices:
   https://github.com/zephyrproject-rtos/openocd/tree/master/tcl/target

.. _pyOCD Supported Devices:
   https://github.com/mbedmicro/pyOCD/tree/master/pyocd/target/builtin

.. _OpenOCD Windows:
    http://gnutoolchains.com/arm-eabi/openocd/
