.. _boot_mode_api:

Boot mode
#########

The boot mode interface can be used to dynamically change the state of an
application or run a different application with a minimal set of functions when
a device is rebooted (an example is to have a buttonless way of entering
MCUboot's serial recovery feature from the main application).

Where the boot mode is stored is selected with a backend, see `Backends`_.

The boot mode interface can be enabled with :kconfig:option:`CONFIG_BOOT_MODE`
and then accessed by using the boot mode functions. If using MCUboot with serial
recovery, it can be built with ``CONFIG_MCUBOOT_SERIAL`` and
``CONFIG_BOOT_SERIAL_BOOT_MODE`` enabled which will allow rebooting directly
into the serial recovery mode by using:

.. code-block:: C

	#include <zephyr/bootmode.h>
	#include <zephyr/sys/reboot.h>

	bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
	sys_reboot(0);

Backends
********

The backend is enabled by default from the compatible of the ``zephyr,boot-mode`` chosen node.
Exactly one backend can be enabled.

Retention area
==============

This backend is enabled by :kconfig:option:`CONFIG_BOOT_MODE_RETENTION`, which is on by default
when the ``zephyr,boot-mode`` chosen node is a ``zephyr,retention`` node and
:kconfig:option:`CONFIG_RETENTION` is enabled. The deprecated
:kconfig:option:`CONFIG_RETENTION_BOOT_MODE` enables both :kconfig:option:`CONFIG_BOOT_MODE` and
this backend. It stores any value from 0 to 255.

To keep the boot mode in a retention area, a data retention entry must exist in
the device tree, which is dedicated for use as the boot mode selection (the user
area data size only needs to be a single byte), and this area be assigned to the
chosen node of ``zephyr,boot-mode``. See the following example:

.. code-block:: devicetree

	/ {
		sram@2003FFFF {
			compatible = "zephyr,memory-region", "mmio-sram";
			reg = <0x2003FFFF 0x1>;
			zephyr,memory-region = "RetainedMem";
			status = "okay";

			retainedmem {
				compatible = "zephyr,retained-ram";
				status = "okay";
				#address-cells = <1>;
				#size-cells = <1>;

				retention0: retention@0 {
					compatible = "zephyr,retention";
					status = "okay";
					reg = <0x0 0x1>;
				};
			};
		};

		chosen {
			zephyr,boot-mode = &retention0;
		};
	};

	/* Reduce SRAM0 usage by 1 byte to account for non-init area */
	&sram0 {
		reg = <0x20000000 0x3FFFF>;
	};

API Reference
*************

.. doxygengroup:: boot_mode_interface
