.. _blinfo_api:

Bootloader Information
######################

The bootloader information (abbreviated to blinfo) subsystem is an extension of
the :ref:`retention_api` which allows for reading shared data from a bootloader
and allowing applications to query it. It has an optional feature of organising
the information retrieved from the bootloader and storing it in the
:ref:`settings_api` with the ``blinfo/`` prefix.

Devicetree setup
****************

To use the bootloader information subsystem, a retention area needs to be
created which has a retained data section as its parent, generally non-init RAM
is used for this purpose. See the following example (examples in this guide are
based on the :ref:`nrf52840dk_nrf52840` board and memory layout):

.. code-block:: devicetree

	/ {
		sram@2003F000 {
			compatible = "zephyr,memory-region", "mmio-sram";
			reg = <0x2003F000 DT_SIZE_K(1)>;
			zephyr,memory-region = "RetainedMem";
			status = "okay";

			retainedmem {
				compatible = "zephyr,retained-ram";
				status = "okay";
				#address-cells = <1>;
				#size-cells = <1>;

				boot_info0: boot_info@0 {
					compatible = "zephyr,retention";
					status = "okay";
					reg = <0x0 0x100>;
				};
			};
		};

		chosen {
			zephyr,bootloader-info = &boot_info0;
		};
	};


	/* Reduce SRAM0 usage by 1KB to account for non-init area */
	&sram0 {
		reg = <0x20000000 DT_SIZE_K(255)>;
	};

Note that this configuration needs to be applied on both the bootloader
(MCUboot) and application to be usable. It can be combined with other retention
system APIs such as the :ref:`boot_mode_api`

MCUboot setup
*************

Once the above devicetree configuration is applied, MCUboot needs to be
configured to store the shared data in this area, the following Kconfigs need
to be set for this:

* :kconfig:option:`CONFIG_RETAINED_MEM` - Enables retained memory driver
* :kconfig:option:`CONFIG_RETENTION` - Enables retention system
* :kconfig:option:`CONFIG_BOOT_SHARE_DATA` - Enables shared data
* :kconfig:option:`CONFIG_BOOT_SHARE_DATA_BOOTINFO` - Enables boot information
  shared data type
* :kconfig:option:`CONFIG_BOOT_SHARE_BACKEND_RETENTION` - Stores shared data
  using retention/blinfo subsystem

Application setup
*****************

The application must enable the following base Kconfig options for the
bootloader information subsystem to function:

* :kconfig:option:`CONFIG_RETAINED_MEM`
* :kconfig:option:`CONFIG_RETENTION`
* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO`
* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO_TYPE_MCUBOOT`

The following include is needed to use the bootloader information subsystem:

.. code-block:: C

	#include <zephyr/retention/blinfo.h>

By default, only the lookup function is provided: :c:func:`blinfo_lookup`, the
application can call this to query the information from the bootloader. This
function is enabled by default with
:kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_FUNCTION`, however,
applications can optionally choose to use the settings storage feature instead.
In this mode, the bootloader information can be queries by using settings keys,
the following Kconfig options need to be enabled for this mode:

* :kconfig:option:`CONFIG_SETTINGS`
* :kconfig:option:`CONFIG_SETTINGS_RUNTIME`
* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_SETTINGS`

This allows the information to be queried via the
:c:func:`settings_runtime_get` function with the following keys:

* ``blinfo/mode`` The mode that MCUboot is configured for
  (``enum mcuboot_mode`` value)
* ``blinfo/signature_type`` The signature type MCUboot is configured for
  (``enum mcuboot_signature_type`` value)
* ``blinfo/recovery`` The recovery type enabled in MCUboot
  (``enum mcuboot_recovery_mode`` value)
* ``blinfo/running_slot`` The running slot, useful for direct-XIP mode to know
  which slot to use for an update
* ``blinfo/bootloader_version`` Version of the bootloader
  (``struct image_version`` object)
* ``blinfo/max_application_size`` Maximum size of an application (in bytes)
  that can be loaded

In addition to the previous include, the following includes are required for
this mode:

.. code-block:: C

	#include <bootutil/boot_status.h>
	#include <bootutil/image.h>
	#include <zephyr/mcuboot_version.h>
	#include <zephyr/settings/settings.h>

API Reference
*************

Bootloader information API
==========================

.. doxygengroup:: bootloader_info_interface
