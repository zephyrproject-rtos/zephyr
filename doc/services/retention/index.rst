.. _retention_api:

Retention System
################

The retention system provides an API which allows applications to read and
write data from and to memory areas or devices that retain the data while the
device is powered. This allows for sharing information between different
applications or within a single application without losing state information
when a device reboots. The stored data should not persist in the event of a
power failure (or during some low-power modes on some devices) nor should it be
stored to a non-volatile storage like :ref:`flash_api`, :ref:`eeprom_api`, or
battery-backed RAM.

The retention system builds on top of the retained data driver, and adds
additional software-level features to it for ensuring the validity of data.
Optionally, a magic header can be used to check if the front of
the retained data memory section contains this specific value, and an optional
checksum (1, 2, or 4-bytes in size) of the stored data can be appended to the
end of the data. Additionally, the retention system API allows partitioning of
the retained data sections into multiple distinct areas. For example, a 64-byte
retained data area could be split up into 4 bytes for a boot mode, 16 bytes for
a timestamp, 44 bytes for a last log message. All of these sections can be
accessed or updated independently. The prefix and checksum can be set
per-instance using devicetree.

Devicetree setup
****************

To use the retention system, a retained data driver must be setup for the board
you are using, there is a zephyr driver which can be used which will use some
RAM as non-init for this purpose. The retention system is then initialised as a
child node of this device 1 or more times - note that the memory region will
need to be decremented to account for this reserved portion of RAM. See the
following example (examples in this guide are based on the
:ref:`nrf52840dk_nrf52840` board and memory layout):

.. code-block:: devicetree

	/ {
		sram@2003FC00 {
			compatible = "zephyr,memory-region", "mmio-sram";
			reg = <0x2003FC00 DT_SIZE_K(1)>;
			zephyr,memory-region = "RetainedMem";
			status = "okay";

			retainedmem {
				compatible = "zephyr,retained-ram";
				status = "okay";
				#address-cells = <1>;
				#size-cells = <1>;

				/* This creates a 256-byte partition */
				retention0: retention@0 {
					compatible = "zephyr,retention";
					status = "okay";

					/* The total size of this area is 256
					 * bytes which includes the prefix and
					 * checksum, this means that the usable
					 * data storage area is 256 - 3 = 253
					 * bytes
					 */
					reg = <0x0 0x100>;

					/* This is the prefix which must appear
					 * at the front of the data
					 */
					prefix = [08 04];

					/* This uses a 1-byte checksum */
					checksum = <1>;
				};

				/* This creates a 768-byte partition */
				retention1: retention@100 {
					compatible = "zephyr,retention";
					status = "okay";

					/* Start position must be after the end
					 * of the previous partition. The total
					 * size of this area is 768 bytes which
					 * includes the prefix and checksum,
					 * this means that the usable data
					 * storage area is 768 - 6 = 762 bytes
					 */
					reg = <0x100 0x300>;

					/* This is the prefix which must appear
					 * at the front of the data
					 */
					prefix = [00 11 55 88 fa bc];

					/* If omitted, there will be no
					 * checksum
					 */
				};
			};
		};
	};

	/* Reduce SRAM0 usage by 1KB to account for non-init area */
	&sram0 {
		reg = <0x20000000 DT_SIZE_K(255)>;
	};

The retention areas can then be accessed using the data retention API (once
enabled with :kconfig:option:`CONFIG_RETENTION`, which requires that
:kconfig:option:`CONFIG_RETAINED_MEM` be enabled) by getting the device by
using:

.. code-block:: C

	#include <zephyr/device.h>
	#include <zephyr/retention/retention.h>

	const struct device *retention1 = DEVICE_DT_GET(DT_NODELABEL(retention1));
	const struct device *retention2 = DEVICE_DT_GET(DT_NODELABEL(retention2));

When the write function is called, the magic header and checksum (if enabled)
will be set on the area, and it will be marked as valid from that point
onwards.

Mutex protection
****************

Mutex protection of retention areas is enabled by default when applications are
compiled with multithreading support. This means that different threads can
safely call the retention functions without clashing with other concurrent
thread function usage, but means that retention functions cannot be used from
ISRs. It is possible to disable mutex protection globally on all retention
areas by enabling :kconfig:option:`CONFIG_RETENTION_MUTEX_FORCE_DISABLE` -
users are then responsible for ensuring that the function calls do not conflict
with each other. Note that to use this, retention driver mutex support must
also be disabled by enabling
:kconfig:option:`CONFIG_RETAINED_MEM_MUTEX_FORCE_DISABLE`.

.. _boot_mode_api:

Boot mode
*********

An addition to the retention subsystem is a boot mode interface, this can be
used to dynamically change the state of an application or run a different
application with a minimal set of functions when a device is rebooted (an
example is to have a buttonless way of entering mcuboot's serial recovery
feature from the main application).

To use the boot mode feature, a data retention entry must exist in the device
tree, which is dedicated for use as the boot mode selection (the user area data
size only needs to be a single byte), and this area be assigned to the chosen
node of ``zephyr,boot-mode``. See the following example:

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

The boot mode interface can be enabled with
:kconfig:option:`CONFIG_RETENTION_BOOT_MODE` and then accessed by using the
boot mode functions. If using mcuboot with serial recovery, it can be built
with ``CONFIG_MCUBOOT_SERIAL`` and ``CONFIG_BOOT_SERIAL_BOOT_MODE`` enabled
which will allow rebooting directly into the serial recovery mode by using:

.. code-block:: C

	#include <zephyr/retention/bootmode.h>
	#include <zephyr/sys/reboot.h>

	bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
	sys_reboot(0);

Retention system modules
************************

Modules can expand the functionality of the retention system by using it as a
transport (e.g. between a bootloader and application).

.. toctree::
    :maxdepth: 1

    blinfo.rst

API Reference
*************

Retention system API
====================

.. doxygengroup:: retention_api

Boot mode interface
===================

.. doxygengroup:: boot_mode_interface
