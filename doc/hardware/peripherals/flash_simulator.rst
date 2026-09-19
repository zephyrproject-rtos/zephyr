.. _flash_simulator:

Flash Simulator
###############

Overview
********

The Flash Simulator provides a virtual, RAM-backed flash memory device. It is primarily
used for testing and development on native targets (such as ``native_sim`` or ``qemu_x86``)
where a physical flash device is not available.

The simulator implements the standard Zephyr flash API, allowing applications to read,
write, and erase data exactly as they would with a real hardware flash controller.

Configuration
*************

Devicetree
==========

To use the flash simulator, you must define a node in your devicetree that is compatible
with ``zephyr,sim-flash``.

The physical properties of the simulated flash (such as its size and block dimensions)
are configured using standard devicetree properties:

.. code-block:: devicetree

	flash0: flash_simulator@0 {
		compatible = "zephyr,sim-flash";
		reg = <0x00000000 DT_SIZE_K(1024)>;
		erase-block-size = <1024>;
		write-block-size = <4>;
		erase-value = <0xff>;
	};

* ``reg``: Defines the base address and the total size of the simulated flash memory.
* ``erase-block-size``: The size of a single erasable block in bytes.
* ``write-block-size``: The minimum write alignment in bytes.
* ``erase-value``: The value of an erased flash cell. Typically ``0xff`` or ``0x00``.
* ``memory-region``: (Optional) A phandle to a memory region. If used, the memory is not cleared on boot.

Kconfig Options
===============

The simulator supports several Kconfig options to mimic the behavior of physical hardware:

* :kconfig:option:`CONFIG_FLASH_SIMULATOR`: Enables the flash simulator driver.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE`: Requires explicit erase operations before writing. This simulates real flash devices that must be erased before bits can be flipped.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_SIMULATE_TIMING`: Enables hardware access timing simulation.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_MIN_READ_TIME_US`: The simulated delay for a read operation.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_MIN_WRITE_TIME_US`: The simulated delay for a write operation.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_MIN_ERASE_TIME_US`: The simulated delay for an erase operation.
* :kconfig:option:`CONFIG_FLASH_SIMULATOR_STATS`: Enables gathering of operation statistics via the Zephyr statistics subsystem.
