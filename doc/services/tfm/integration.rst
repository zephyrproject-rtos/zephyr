Trusted Firmware-M Integration
##############################

The Trusted Firmware-M (TF-M) section contains information about the
integration between TF-M and Zephyr RTOS. Use this information to help
understand how to integrate TF-M with Zephyr for Cortex-M platforms and make
use of its secure run-time services in Zephyr applications.

Board Definitions
*****************

TF-M will be built for the secure processing environment along with Zephyr if
the :kconfig:option:`CONFIG_BUILD_WITH_TFM` flag is set to ``y``.

Generally, this value should never be set at the application level, however,
and all config flags required for TF-M should be set in a board variant with
the ``_ns`` suffix.

This board variant must define an appropriate flash, SRAM and peripheral
configuration that takes into account the initialisation process in the secure
processing environment. :kconfig:option:`CONFIG_TFM_BOARD` must also be set via
`modules/trusted-firmware-m/Kconfig.tfm <https://github.com/zephyrproject-rtos/zephyr/blob/main/modules/trusted-firmware-m/Kconfig.tfm>`__
to the board name that TF-M expects for this target, so that it knows which
target to build for the secure processing environment.

Example: ``mps2_an521_ns``
==========================

The ``mps2_an521`` target is a dual-core Arm Cortex-M33 evaluation board that,
when using the default board variant, would generate a secure Zephyr binary.

The optional ``mps2_an521_ns`` target, however, sets these additional
kconfig flags that indicate that Zephyr should be built as a
non-secure image, linked with TF-M as an external project, and optionally the
secure bootloader:

* :kconfig:option:`CONFIG_TRUSTED_EXECUTION_NONSECURE` ``y``
* :kconfig:option:`CONFIG_ARM_TRUSTZONE_M` ``y``

Comparing the ``mps2_an521.dts`` and ``mps2_an521_ns.dts`` files, we can see
that the ``_ns`` version defines offsets in flash and SRAM memory, which leave
the required space for TF-M and the secure bootloader:

::

    reserved-memory {
		#address-cells = <1>;
		#size-cells = <1>;
		ranges;

		/* The memory regions defined below must match what the TF-M
		 * project has defined for that board - a single image boot is
		 * assumed. Please see the memory layout in:
		 * https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git/tree/platform/ext/target/mps2/an521/partition/flash_layout.h
		 */

		code: memory@100000 {
			reg = <0x00100000 DT_SIZE_K(512)>;
		};

		ram: memory@28100000 {
			reg = <0x28100000 DT_SIZE_M(1)>;
		};
	};

This reserves 1 MB of code memory and 1 MB of RAM for secure boot and TF-M,
such that our non-secure Zephyr application code will start at 0x10000, with
RAM at 0x28100000. 512 KB code memory is available for the NS zephyr image,
along with 1 MB of RAM.

This matches the flash memory layout we see in ``flash_layout.h`` in TF-M:

::

    * 0x0000_0000 BL2 - MCUBoot (0.5 MB)
    * 0x0008_0000 Secure image     primary slot (0.5 MB)
    * 0x0010_0000 Non-secure image primary slot (0.5 MB)
    * 0x0018_0000 Secure image     secondary slot (0.5 MB)
    * 0x0020_0000 Non-secure image secondary slot (0.5 MB)
    * 0x0028_0000 Scratch area (0.5 MB)
    * 0x0030_0000 Protected Storage Area (20 KB)
    * 0x0030_5000 Internal Trusted Storage Area (16 KB)
    * 0x0030_9000 NV counters area (4 KB)
    * 0x0030_A000 Unused (984 KB)

``mps2/an521`` will be passed in to Tf-M as the board target, specified via
:kconfig:option:`CONFIG_TFM_BOARD`.
