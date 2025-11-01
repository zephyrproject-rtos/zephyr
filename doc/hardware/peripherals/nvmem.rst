.. _nvmem_api:

Non-Volatile Memory (NVMEM)
###########################

The NVMEM device driver model provides a uniform way for NVMEM consumers
to access non-volatile memory devices such as EEPROMs, OTP/eFuse, FRAM,
and battery-backed RAM (NVMEM providers).

The API is provider-centric: drivers expose basic read/write operations
and report their total size. Providers are responsible for enforcing
device semantics (e.g., read-only policy, hardware-defined one-time
programming for OTP/eFuse). Optionally, providers can expose configuration
descriptors for consumer and tool diagnostics.

.. contents::
    :local:
    :depth: 2


Goals and vision
****************

The NVMEM model unifies access to diverse non-volatile memories behind a tiny,
consistent API so application and driver consumers can:

- Treat NVMEM like a simple byte-addressable space (read/write/get_size).
- Discover device semantics at runtime (type and read-only via nvmem_info).
- Describe reusable named “cells” in devicetree and bind consumers to them.

Providers retain full control of hardware semantics and safety (e.g., enforcing
OTP bit programming rules, honoring write-protect pins, respecting page/word
granularity). The long-term vision is a single, cross-silicon way to read
calibration data, MAC addresses, keys, serial numbers, and platform state.


Key concepts
************

- NVMEM provider:
  A device driver that implements the NVMEM API for a specific storage (e.g.,
  I2C EEPROM, on-chip eFuse/OTP, FRAM, battery-backed SRAM).

- NVMEM consumer:
  Any code that reads/writes NVMEM data. This includes applications,
  subsystems (e.g., networking fetching a MAC address), and manufacturing tools.

- NVMEM cell:
  A named region within a provider’s address space described in devicetree using
  a fixed layout. Cells allow consumers to bind to stable names instead of hardcoding
  offsets and sizes.


NVMEM Provider API
******************

Zephyr's NVMEM provider API is implemented by device drivers for specific
non-volatile memories (e.g., AT24/AT25 EEPROMs, NXP OCOTP eFuse). Consumers
interact with these providers via a small set of functions:

- :c:func:`nvmem_read` – Read bytes from the provider address space.
- :c:func:`nvmem_write` – Write bytes to the provider (if permitted).
- :c:func:`nvmem_get_size` – Get the total size (in bytes).
- :c:func:`nvmem_get_info` – Optionally obtain a provider descriptor.
  (:c:struct:`nvmem_info`) including storage type and current read-only state.

Provider responsibilities and guidance
======================================

- Enforce device semantics in their :c:func:`nvmem_read`/:c:func:`nvmem_write`
  implementations (e.g., OTP one-way bit programming per hardware; return ``-EROFS``
  when writes are disabled)
- Handle alignment/stride/granularity according to hardware rules
- Optionally expose :c:struct:`nvmem_info` via :c:func:`nvmem_get_info`
  for diagnostics (e.g., storage :c:enum:`nvmem_type`, runtime read-only)

Providers must implement the API and enforce hardware semantics in their
read/write paths. Practical guidance when writing a provider driver:

- Validate parameters: reject out-of-range access with ``-EINVAL`` and ``-ERANGE``.
- Enforce mutability: return ``-EROFS`` when the device is effectively read-only
  at runtime (e.g., OTP default, Kconfig gate).
- Preserve hardware rules: respect alignment, bus/page/word granularity,
  maximum transfer sizes, and one-way programming for OTP (0→1 only).
- Report size accurately via :c:func:`nvmem_get_size`.
- Optionally expose :c:struct:`nvmem_info` via :c:func:`nvmem_get_info` to
  describe the storage type (EEPROM/OTP/FRAM) and current read-only state.

Common return codes:

- ``0`` on success
- Negative errno on error: ``-EINVAL``/ ``-ERANGE`` (bad args), ``-EIO`` (bus
  or hardware failure), ``-ENOTSUP`` (operation not supported), ``-EROFS``
  (writes disabled)


Typical NVMEM Consumer usage:
*****************************
1. Obtain the provider device, for example via devicetree:

.. code-block:: c

	const struct device *nvmem = DEVICE_DT_GET(DT_NODELABEL(mac_eeprom));

	if (!device_is_ready(nvmem)) {
		return -ENODEV;
	}

2. Read or write using byte offsets in the provider address space:

.. code-block:: c

	uint8_t mac[6];
	int err = nvmem_read(nvmem, /* offset */ 0xFA, mac, sizeof(mac));

	if (err) {
		/* handle error */
	}

	/* Optional write, if provider allows */
	err = nvmem_write(nvmem, 0x100, some_data, some_len);
	if (err == -EROFS) {
		/* provider is read-only (e.g., OTP default) */
	}

1. Optionally query provider configuration (when available) for display or
   validation purposes:

.. code-block:: c

	const struct nvmem_info *info = nvmem_get_info(nvmem);
	size_t size = nvmem_get_size(nvmem);

	if (info) {
		printk("nvmem: type=%d, read_only=%d, size=%zu\n",
		(int)info->type, info->read_only, size);
	}


Devicetree
**********

Providers can declare a fixed layout of named “cells” using the
``fixed-layout`` binding. This allows consumers to reference fields (like a
MAC address) by name without hardcoding offsets.

Bindings overview
=================

- ``nvmem-provider.yaml``: Common provider properties (e.g., ``size``,
  ``read-only``) that silicon- or part-specific bindings include.
- ``fixed-layout.yaml``: Schema for defining cell children under a provider.
- ``nvmem-consumer.yaml``: Properties for consumers referencing cells via
  ``nvmem-cells`` and ``nvmem-cell-names``.

Example: provider without cells
===============================

Some designs access a provider using raw offsets only. This is fine when the
consumer owns the address map or offsets are trivial.

.. code-block:: devicetree

	fram0: fram@0 {
		compatible = "infineon,fm25xxx";
		reg = <0>;		/* SPI chip select or bus addr, device-specific */
		size = <8192>;		/* total bytes if required by the binding */
		status = "okay";
	};

.. code-block:: c

	const struct device *nvmem = DEVICE_DT_GET(DT_NODELABEL(fram0));

	if (!device_is_ready(nvmem)) {
		return -ENODEV;
	}

	uint8_t buf[16];
	int err = nvmem_read(nvmem, 0x100, buf, sizeof(buf));

	if (err) {
		/* handle error */
	}

	/* Optional write */
	err = nvmem_write(nvmem, 0x120, buf, 8);
	if (err == -EROFS) {
		/* provider is read-only (e.g., OTP default or WP asserted) */
	}

Fixed layout for cells
======================

Example devicetree fragment declaring a provider with a named MAC-address cell:

.. code-block:: devicetree

	mac_eeprom: mac_eeprom@2 {
		compatible = "atmel,at24";		/* provider example */
		reg = <0x2>;				/* I2C address (provider-specific) */
		status = "okay";

		nvmem-layout {
			compatible = "fixed-layout";
			#address-cells = <1>;
			#size-cells = <1>;

			mac_address: mac_address@fa {
				reg = <0xFA 0x06>;	/* offset, size in bytes */
				#nvmem-cell-cells = <0>;
			};
		};
	};

Consumers can then obtain the cell at build time using helper macros from
``<zephyr/drivers/nvmem.h>``:

.. code-block:: c

	#include <zephyr/drivers/nvmem.h>

	const struct nvmem_cell mac = NVMEM_CELL_INIT(DT_NODELABEL(mac_address));

	if (nvmem_cell_is_ready(&mac)) {
		uint8_t buf[6];
		int err = nvmem_read(mac.dev, mac.offset, buf, mac.size);
		/* handle err */
	}

Other useful helpers include:

- :c:macro:`NVMEM_CELL_GET_BY_NAME(node, name)` and ``_OR`` variants
- :c:macro:`NVMEM_CELL_GET_BY_IDX(node, idx)` and ``_OR`` variants
- :c:macro:`NVMEM_CELL_INST_GET_BY_NAME(inst, name)` and ``_IDX`` variants

Consumer diagnostics with nvmem_info
====================================

Consumers may optionally query runtime provider info for validation or display:

.. code-block:: c

	const struct nvmem_info *info = nvmem_get_info(mac.dev);
	size_t size = nvmem_get_size(mac.dev);

	if (info) {
		printk("nvmem: type=%d, read_only=%d, size=%zu\n",
			(int)info->type, info->read_only, size);
	}

NXP OCOTP (eFuse) example
=========================

The NXP OCOTP provider accesses eFuse via a ROM API table and supports a fixed
layout for named cells. Example:

.. code-block:: devicetree

	ocotp: ocotp@0 {
		compatible = "nxp,ocotp";
		rom-api-tree-addr = <0x00200120>;	/* SoC-specific */
		status = "okay";

		nvmem-layout {
			compatible = "fixed-layout";
			#address-cells = <1>;
			#size-cells = <1>;

			tmpsns_calib: tmpsns-calib@134 {
				reg = <0x134 0x1>;
				#nvmem-cell-cells = <0>;
				read-only;
			};
		};
	};

Note: Providers define how offsets are interpreted; OCOTP treats ``reg``
addresses as byte offsets into the eFuse space and extracts the requested
bytes from the containing 32-bit words.


Write-protect strategy (OTP/eFuse and read-only providers)
**********************************************************

Protecting against unintended writes is a layered approach. Depending on the
provider and platform, some or all of these layers apply:

- Devicetree level

   - Provider-wide ``read-only;`` property in the NVMEM node marks the entire
     device immutable. Provider drivers should reflect this in
     :c:func:`nvmem_get_info` and reject writes with ``-EROFS``.
   - Per-cell ``read-only;`` under ``fixed-layout`` signals to consumers/tools
     that a particular field must not be modified.

.. code-block:: devicetree

	ocotp: ocotp@0 {
		compatible = "nxp,ocotp";
		status = "okay";
		read-only;			/* treat the whole provider as immutable */

		nvmem-layout {
			compatible = "fixed-layout";
			#address-cells = <1>;
			#size-cells = <1>;

			mac0: mac0@134 {
				reg = <0x134 0x6>;
				read-only;	/* cell is immutable */
				#nvmem-cell-cells = <0>;
			};
		};
	};

- Kconfig level

   - Feature gates to allow programming only when explicitly enabled. Example:
     :kconfig:option:`CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE` defaults to off and
     must be turned on to permit OCOTP writes.

- CMake/build level

   - The build emits an explicit warning if OCOTP programming is enabled.
     Examples(see ``drivers/nvmem/CMakeLists.txt``):

.. code-block:: cmake

	if(CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE)
	  message(WARNING "OCOTP fuse programming is destructive and irreversible;
		  enable only for provisioning in a controlled environment.
		  DO NOT enable for production builds.")
	endif()

- Provider level

   - The write implementation is authoritative: it must reject writes when the
     device is read-only for any reason (DT ``read-only``, lifecycle lock, WP
     pin, or Kconfig gate) and must enforce OTP programming semantics (e.g.,
     allow only the hardware-supported one-way bit transition and reject
     attempts to reverse a programmed bit). Return ``-EROFS`` as appropriate.

.. code-block:: c

	static int ocotp_write(const struct device *dev, off_t off,
			const void *buf, size_t len)
	{
		const struct nvmem_info *info = nvmem_get_info(dev);

		if (!IS_ENABLED(CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE) || (info && info->read_only)) {
			return -EROFS;
		}
		/* Enforce hardware one-time programming rules (only allowed bit transitions),
		 * plus alignment and word-lane constraints here.
		 */
		return program_otp_words(dev, off, buf, len);
	}

- Consumer level

   - Treat ``-EROFS`` from :c:func:`nvmem_write` as a hard stop; optionally
     consult :c:func:`nvmem_get_info` to show a friendly diagnostic or to choose
     a read-only flow.

.. code-block:: c

	int err = nvmem_write(nvmem, off, data, len);

	if (err == -EROFS) {
		const struct nvmem_info *i = nvmem_get_info(nvmem);
		printk("Writes disabled (type=%d, ro=%d)\n", i ? (int)i->type : -1, i ? i->read_only : -1);
	}


Configuration Options
*********************

Related configuration options:

- :kconfig:option:`CONFIG_NVMEM_MODEL` – Enable the NVMEM driver model
- :kconfig:option:`CONFIG_NVMEM_MODEL_API` – Export the NVMEM API
- :kconfig:option:`CONFIG_NVMEM_INIT_PRIORITY` – Provider init priority
- :kconfig:option:`CONFIG_NVMEM_LOG_LEVEL` – Logging verbosity for NVMEM drivers
- Provider selections (enable as needed):
   - :kconfig:option:`CONFIG_NVMEM_LPC11U6X` – NXP LPC11U6X on-chip EEPROM
   - :kconfig:option:`CONFIG_NVMEM_FM25XXX` – Infineon FM25XXX FRAM
   - :kconfig:option:`CONFIG_NVMEM_BFLB_EFUSE` – Bouffalo Lab eFuse (read-only)
   - :kconfig:option:`CONFIG_NVMEM_NXP_OCOTP` – Enable NXP OCOTP eFuse provider
      - :kconfig:option:`CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE` – Allow OCOTP programming (default: off)


Notes and guidance
******************

- Consumers use the provider API directly (:c:func:`nvmem_read` / :c:func:`nvmem_write`) and,
  when using fixed layouts, the :c:struct:`nvmem_cell` helpers provided
  in ``<zephyr/drivers/nvmem.h>``.
- Providers are the source of truth for policy: read-only behavior, lock/Write
  Protect enforcement, shadow handling, and SoC-specific sequences are fully
  handled inside each provider's driver.
- For OTP/eFuse, writes are typically disabled by default. Attempting a write
  returns ``-EROFS`` unless the platform explicitly enables programming.
- Offsets are provider-defined. Some providers accept raw byte offsets while
  others require word alignment or page boundaries—consult the provider’s
  binding and driver.
- Concurrency: NVMEM providers are devices; follow normal Zephyr device
  concurrency patterns. If the hardware requires mutual exclusion, implement it
  in the driver.
- Don’t assume memory-mapped access. Always go through the API; many providers
  are bus-attached and may have side-effectful access sequences.


API Reference
*************

.. doxygengroup:: nvmem_model_interface
   :project: Zephyr
   :content-only:
