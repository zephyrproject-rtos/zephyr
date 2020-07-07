/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Definitions and helper macros for managing driver memory-mapped
 * input/output (MMIO) regions appropriately in either RAM or ROM.
 *
 * In most cases drivers will just want to include device.h, but
 * including this separately may be needed for arch-level driver code
 * which uses the DEVICE_MMIO_TOPLEVEL variants and including the
 * main device.h would introduce header dependency loops due to that
 * header's reliance on kernel.h.
 */
#ifndef ZEPHYR_INCLUDE_SYS_DEVICE_MMIO_H
#define ZEPHYR_INCLUDE_SYS_DEVICE_MMIO_H

#include <toolchain/common.h>

/**
 * @defgroup device-mmio Device memory-mapped IO management
 * @ingroup device-model
 * @{
 */

/* Storing MMIO addresses in RAM is a system-wide decision based on
 * configuration. This is just used to simplify some other definitions.
 *
 * If we have an MMU enabled, all physical MMIO regions must be mapped into
 * the kernel's virtual address space at runtime, this is a hard requirement.
 *
 * If we have PCIE enabled, this does mean that non-PCIE drivers may waste
 * a bit of RAM, but systems with PCI express are not RAM constrained.
 */
#if defined(CONFIG_MMU) || defined(CONFIG_PCIE)
#define DEVICE_MMIO_IS_IN_RAM
#endif

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>
#include <sys/mm.h>
#include <sys/sys_io.h>

#ifdef DEVICE_MMIO_IS_IN_RAM
/* Store the physical address and size from DTS, we'll memory
 * map into the virtual address space at runtime. This is not applicable
 * to PCIe devices, which must query the bus for BAR information.
 */
struct z_device_mmio_rom {
	/** MMIO physical address */
	uintptr_t phys_addr;

	/** MMIO region size */
	size_t size;
};

#define Z_DEVICE_MMIO_ROM_INITIALIZER(_instance) \
	{ \
		.phys_addr = DT_INST_REG_ADDR(_instance), \
		.size = DT_INST_REG_SIZE(_instance) \
	}

/**
 * Map an MMIO region into the address space.
 *
 * Thin wrapper on top of k_map, or just set the pointer if no MMU
 * (which is weird I should change this)
 *
 * @see k_map()
 *
 * @param linear_addr [out] Output linear address storage location, most
 *		users will want some DEVICE_MMIO_RAM_PTR() value
 * @param phys_addr Physical address base of the MMIO region
 * @param size Size of the MMIO region
 * @param flags Caching mode and access flags, see K_MAP_* macros
 */
static inline void device_map(mm_reg_t *virt_addr, uintptr_t phys_addr,
			      size_t size, uint32_t flags)
{
#ifdef CONFIG_MMU
	/* Pass along flags and add that we want supervisor mode
	 * read-write access.
	 */
	k_map((uint8_t **)virt_addr, phys_addr, size, flags | K_MAP_RW);
#else
	ARG_UNUSED(size);
	ARG_UNUSED(flags);

	*virt_addr = phys_addr;
#endif /* CONFIG_MMU */
}
#else
/* No MMU or PCIe. Just store the address from DTS and treat as a linear
 * address
 */
struct z_device_mmio_rom {
	/** MMIO linear address */
	mm_reg_t addr;
};

#define Z_DEVICE_MMIO_ROM_INITIALIZER(_instance) \
	{ \
		.addr = DT_INST_REG_ADDR(_instance) \
	}
#endif /* DEVICE_MMIO_IS_IN_RAM */
#endif /* !_ASMLANGUAGE */
/** @} */

/**
 * @defgroup device-mmio-single Single MMIO region macros
 * @ingroup device-mmio
 *
 * For drivers which need to manage just one MMIO region, the most common
 * case.
 *
 * @{
 */

/**
 * @def DEVICE_MMIO_RAM
 *
 * Declare storage for MMIO information within a device's dev_data struct.
 *
 * This gets accessed by the DEVICE_MMIO_MAP() and DEVICE_MMIO_GET() macros.
 *
 * Depending on configuration, no memory may be reserved at all.
 * This must be the first member of the driver_data struct.
 *
 * There must be a corresponding DEVICE_MMIO_ROM in config_info if the
 * physical address is known at build time, but may be omitted if not (such
 * as with PCIe)
 *
 * Example for a driver named "foo":
 *
 * struct foo_driver_data {
 *	DEVICE_MMIO_RAM;
 *	int wibble;
 *	...
 * }
 *
 * No build-time initialization of this memory is necessary; it
 * will be set up in the init function by DEVICE_MMIO_MAP().
 *
 * A pointer to this memory may be obtained with DEVICE_MMIO_RAM_PTR().
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_RAM			mm_reg_t _mmio
#else
#define DEVICE_MMIO_RAM
#endif

#ifdef DEVICE_MMIO_IS_IN_RAM
/**
 * @def DEVICE_MMIO_RAM_PTR
 *
 * Return a pointer to the RAM-based storage area for a device's MMIO
 * address.
 *
 * This is useful for the target MMIO address location when using
 * device_map() directly.
 *
 * @param _device device instance object
 * @retval mm_reg_t  pointer to storage location
 */
#define DEVICE_MMIO_RAM_PTR(_device)	(mm_reg_t *)((_device)->dev_data)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_ROM
 *
 * @brief Declare storage for MMIO data within a device's config_info struct
 *
 * This gets accessed by DEVICE_MMIO_MAP() and DEVICE_MMIO_GET() macros.
 *
 * What gets stored here varies considerably by configuration.
 * This must be the first member of the config_info struct. There must be
 * a corresponding DEVICE_MMIO_RAM in driver_data.
 *
 * This storage is not used if the device is PCIe and may be omitted.
 *
 * This should be initialized at build time with information from DTS
 * using DEVICE_MMIO_ROM_INIT().
 *
 * A pointer to this memory may be obtained with DEVICE_MMIO_ROM_PTR().
 *
 * Example for a driver named "foo":
 *
 * struct foo_config_info {
 *	DEVICE_MMIO_ROM;
 *	int baz;
 *	...
 * }
 *
 * @see DEVICE_MMIO_ROM_INIT()
 */
#define DEVICE_MMIO_ROM		struct z_device_mmio_rom _mmio

/**
 * @def DEVICE_MMIO_ROM_PTR
 *
 * Return a pointer to the ROM-based storage area for a device's MMIO
 * information. This macro will not work properly if the ROM storage
 * was omitted from the config_info struct declaration, and should not
 * be used in this case.
 *
 * @param _device device instance object
 * @retval struct device_mmio_rom * pointer to storage location
 */
#define DEVICE_MMIO_ROM_PTR(_device) \
	(struct device_mmio_rom *)((device)->config_info)

/**
 * @def DEVICE_MMIO_ROM_INIT
 *
 * @brief Initialize a DEVICE_MMIO_ROM member
 *
 * Initialize MMIO-related information within a specific instance of
 * a device config_info struct, using information from DTS.
 *
 * Example for an instance of a driver belonging to the "foo" subsystem:
 *
 * struct foo_config_info my_config_info = {
 *	DEVICE_MMIO_ROM_INIT(instance),
 *	.baz = 2;
 *	...
 * }
 *
 * @see DEVICE_MMIO_ROM()
 *
 * @param _instance DTS instance
 */
#define DEVICE_MMIO_ROM_INIT(_instance) \
	._mmio = Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)

/**
 * @def DEVICE_MMIO_MAP
 *
 * @brief Map MMIO memory into the address space
 *
 * This is not intended for PCIe devices; these must be probed at runtime
 * and you will want to make a device_map() call directly, using
 * DEVICE_MMIO_RAM_PTR() as the target virtual address location.
 *
 * The flags argument is currently used for caching mode, which should be
 * one of the DEVICE_CACHE_* macros. Unused bits are reserved for future
 * expansion.
 *
 * @see device_map()
 * @see k_map()
 *
 * @param _device Device object instance
 * @param _flags cache mode flags
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_MAP(_device, _flags) \
	device_map(DEVICE_MMIO_RAM_PTR(_device), \
		   DEVICE_MMIO_ROM_PTR(_device)->phys_addr, \
		   DEVICE_MMIO_ROM_PTR(_device)->size, \
		   (_flags))
#else
#define DEVICE_MMIO_MAP(_device, _flags) do { } while(0)
#endif

/**
 * @def DEVICE_MMIO_GET
 *
 * @brief Obtain the MMIO address for a device
 *
 * For most microcontrollers MMIO addresses can be fixed values known at
 * build time, and we can store this in device->config_info, residing in ROM.
 *
 * However, some devices can only know their MMIO addresses at runtime,
 * because they need to be memory-mapped into the address space, enumerated
 * from PCI, or both.
 *
 * This macro returns the linear address of the driver's MMIO region.
 * This is for drivers which have exactly one MMIO region.
 * A call must have been made to device_map() in the driver init function.
 *
 * @param _device Device object
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_GET(_device)	(*DEVICE_MMIO_RAM_PTR(_device))
#else
#define DEVICE_MMIO_GET(_device)	(DEVICE_MMIO_ROM_PTR(_device)->addr)
#endif
/** @} */

/**
 * @defgroup device-mmio-named Named MMIO region macros
 * @ingroup device-mmio
 *
 * For drivers which need to manage multiple MMIO regions, which will
 * be referenced by name.
 *
 * @{
 */

/**
 * @def DEVICE_MMIO_NAMED_RAM
 *
 * @brief Declare storage for MMIO data within a device's dev_data struct
 *
 * This gets accessed by the DEVICE_MMIO_NAMED_MAP() and
 * DEVICE_MMIO_NAMED_GET() macros.
 *
 * Depending on configuration, no memory may be reserved at all.
 * Multiple named regions may be declared.
 *
 * There must be a corresponding DEVICE_MMIO_ROM in config_info if the
 * physical address is known at build time, but may be omitted if not (such
 * as with PCIe.
 *
 * Example for a driver named "foo":
 *
 * struct foo_driver_data {
 * 	int blarg;
 *	DEVICE_MMIO_NAMED_RAM(courge);
 *	DEVICE_MMIO_NAMED_RAM(grault);
 *	int wibble;
 *	...
 * }
 *
 * No build-time initialization of this memory is necessary; it
 * will be set up in the init function by DEVICE_MMIO_NAMED_MAP().
 *
 * @param _name Member name to use to store within dev_data.
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_RAM(_name)	mm_reg_t _name
#else
#define DEVICE_MMIO_NAMED_RAM(_name)
#endif /* DEVICE_MMIO_IS_IN_RAM */

#ifdef DEVICE_MMIO_IS_IN_RAM
/**
 * @def DEVICE_MMIO_NAMED_RAM_PTR
 *
 * @brief Return a pointer to the RAM storage for a device's named MMIO address
 *
 * This macro requires that the macro DEV_DATA is locally defined and returns
 * a properly typed pointer to the particular dev_data struct for this driver.
 *
 * @param _device device instance object
 * @param _name Member name within dev_data
 * @retval mm_reg_t  pointer to storage location
 */
#define DEVICE_MMIO_NAMED_RAM_PTR(_device, _name) \
		(&(DEV_DATA(_device)->_name))
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_NAMED_ROM
 *
 * @brief Declare storage for MMIO data within a device's config_info struct.
 *
 * This gets accessed by DEVICE_MMIO_NAMED_MAP() and
 * DEVICE_MMIO_NAMED_GET() macros.
 *
 * What gets stored here varies considerably by configuration. Multiple named
 * regions may be declared. There must be corresponding entries in the dev_data
 * struct.
 *
 * This storage is not used if the device is PCIe and may be omitted.
 *
 * If used, this must be initialized at build time with information from DTS
 * using DEVICE_MMIO_NAMED_ROM_INIT()
 *
 * A pointer to this memory may be obtained with DEVICE_MMIO_NAMED_ROM_PTR().
 *
 * Example for a driver named "foo":
 *
 * struct foo_config_info {
 * 	int bar;
 *	DEVICE_MMIO_NAMED_ROM(courge);
 *	DEVICE_MMIO_NAMED_ROM(grault);
 *	int baz;
 *	...
 * }
 *
 * @see DEVICE_MMIO_NAMED_ROM_INIT()
 *
 * @param _name Member name to store within config_info
 */
#define DEVICE_MMIO_NAMED_ROM(_name) struct z_device_mmio_rom _name

/**
 * @def DEVICE_MMIO_ROM_PTR
 *
 * Return a pointer to the ROM-based storage area for a device's MMIO
 * information.
 *
 * This macro requires that the macro DEV_CFG is locally defined and returns
 * a properly typed pointer to the particular config_info struct for this
 * driver.
 *
 * @param _device device instance object
 * @param _name Member name within config_info
 * @retval struct device_mmio_rom * pointer to storage location
 */
#define DEVICE_MMIO_NAMED_ROM_PTR(_device, _name) (&(DEV_CFG(_device)->_name))

/**
 * @def DEVICE_MMIO_NAMED_ROM_INIT
 *
 * @brief Initialize a named DEVICE_MMIO_NAMED_ROM member
 *
 * Initialize MMIO-related information within a specific instance of
 * a device config_info struct, using information from DTS.
 *
 * Example for an instance of a driver belonging to the "foo" subsystem
 * that will have two regions named 'courge' and 'grault':
 *
 * struct foo_config_info my_config_info = {
 *	bar = 7;
 *	DEVICE_MMIO_NAMED_ROM_INIT(courge, instance);
 *	DEVICE_MMIO_NAMED_ROM_INIT(grault, instance);
 *	baz = 2;
 *	...
 * }
 *
 * @see DEVICE_MMIO_NAMED_ROM()
 *
 * @param _name Member name within config_info for the MMIO region
 * @param _instance DTS instance
 */
#define DEVICE_MMIO_NAMED_ROM_INIT(_name, _instance) \
	._name = Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)

/**
 * @def DEVICE_MMIO_NAMED_MAP
 *
 * @brief Set up memory for a named MMIO region
 *
 * This performs the necessary PCI probing and/or MMU virtual memory mapping
 * such that DEVICE_MMIO_GET(_name) returns a suitable linear memory address
 * for the MMIO region.
 *
 * If such operations are not required by the target hardware, this expands
 * to nothing.
 *
 * This should be called from the driver's init function, once for each
 * MMIO region that needs to be mapped.
 *
 * This macro requires that the macros DEV_DATA and DEV_CFG are locally
 * defined and return properly typed pointers to the particular dev_data
 * and config_info structs for this driver.
 *
 * The flags argument is currently used for caching mode, which should be
 * one of the DEVICE_CACHE_* macros. Unused bits are reserved for future
 * expansion.
 *
 * @see DEVICE_MMIO_MAP()
 * @see device_map()
 * @see k_map()
 *
 * @param _device Device object
 * @param _name Member name for MMIO information, as declared with
 * 	        DEVICE_MMIO_NAMED_RAM/DEVICE_MMIO_NAMED_ROM
 * @param flags One of the DEVICE_CACHE_* caching modes
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_MAP(_device, _name, _flags) \
	device_map(DEVICE_MMIO_NAMED_RAM_PTR((_device), _name), \
		   (DEVICE_MMIO_NAMED_ROM_PTR((_device), _name)->phys_addr), \
		   (DEVICE_MMIO_NAMED_ROM_PTR((_device), _name)->size), \
		   (_flags))
#else
#define DEVICE_MMIO_NAMED_MAP(_device, _name, _flags) do { } while(0)
#endif

/**
 * @def DEVICE_MMIO_NAMED_GET
 *
 * @brief Obtain a named MMIO address for a device
 *
 * This macro returns the MMIO base address for a named region from the
 * appropriate place within the device object's linked  data structures.
 *
 * This is for drivers which have multiple MMIO regions.
 *
 * This macro requires that the macros DEV_DATA and DEV_CFG are locally
 * defined and return properly typed pointers to the particular dev_data
 * and config_info structs for this driver.
 *
 * @see DEVICE_MMIO_GET
 *
 * @param _device Device object
 * @param _name Member name for MMIO information, as declared with
 * 	        DEVICE_MMIO_NAMED_RAM/DEVICE_MMIO_NAMED_ROM
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_GET(_device, _name) \
		(*DEVICE_MMIO_NAMED_RAM_PTR((_device), _name))
#else
#define DEVICE_MMIO_NAMED_GET(_device, _name) \
		((DEVICE_MMIO_NAMED_ROM_PTR((_device), _name))->addr)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/** @} */

/**
 * @defgroup device-mmio-toplevel Top-level MMIO region macros
 * @ingroup device-mmio
 *
 * For drivers which do not use Zephyr's driver model and do not
 * associate struct device with a driver instance. Top-level storage
 * is used instead, with either global or static scope.
 *
 * This is often useful for interrupt controller and timer drivers.
 *
 * Currently PCIe devices are not well-supported with this set of macros.
 * Either use Zephyr's driver model for these kinds of devices, or
 * manage memory manually with calls to device_map().
 *
 * @{
 */

/**
 * @def DEVICE_MMIO_TOPLEVEL
 *
 * @brief Declare top-level storage for MMIO information, global scope
 *
 * This is intended for drivers which do not use Zephyr's driver model
 * of config_info/dev_data linked to a struct device.
 *
 * Instead, this is a top-level declaration for the driver's C file.
 * The scope of this declaration is global and may be referenced by
 * other C files, using DEVICE_MMIO_TOPLEVEL_DECLARE.
 *
 * @param _name Base symbol name
 * @param _instance Device-tree instance for this region
 */
 #ifdef DEVICE_MMIO_IS_IN_RAM
 #define DEVICE_MMIO_TOPLEVEL(_name, _instance) \
	mm_reg_t _CONCAT(z_mmio_ram__, _name); \
	const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)
#else
#define DEVICE_MMIO_TOPLEVEL(_name, _instance) \
	const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_TOPLEVEL_DECLARE
 *
 * Provide an extern reference to a top-level MMIO region
 *
 * If a top-level MMIO region defined with DEVICE_MMIO_DEFINE needs to be
 * referenced from other C files, this macro provides the necessary extern
 * definitions.
 *
 * @see DEVICE_MMIO_TOPLEVEL
 *
 * @param _name Name of the top-level MMIO region
 */

#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_DECLARE(_name) \
	extern mm_reg_t _CONCAT(z_mmio_ram__, _name); \
	extern const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name)
#else
#define DEVICE_MMIO_TOPLEVEL_DECLARE(_name) \
	extern const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_TOPLEVEL_STATIC
 *
 * @brief Declare top-level storage for MMIO information, static scope
 *
 * This is intended for drivers which do not use Zephyr's driver model
 * of config_info/dev_data linked to a struct device.
 *
 * Instead, this is a top-level declaration for the driver's C file.
 * The scope of this declaration is static.
 *
 * @param _name Name of the top-level MMIO region
 * @param _instance Device-tree instance for this region
 */
 #ifdef DEVICE_MMIO_IS_IN_RAM
 #define DEVICE_MMIO_TOPLEVEL_STATIC(_name, _instance) \
	static mm_reg_t _CONCAT(z_mmio_ram__, _name); \
	static const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)
#else
#define DEVICE_MMIO_TOPLEVEL_STATIC(_name, _instance) \
	static const struct z_device_mmio_rom _CONCAT(z_mmio_rom__, _name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(_instance)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_TOPLEVEL_MAP
 *
 * @brief Set up memory for a driver'sMMIO region
 *
 * This performs the necessary MMU virtual memory mapping
 * such that DEVICE_MMIO_GET() returns a suitable linear memory address
 * for the MMIO region.
 *
 * If such operations are not required by the target hardware, this expands
 * to nothing.
 *
 * This should be called once from the driver's init function.
 *
 * The flags argument is currently used for caching mode, which should be
 * one of the DEVICE_CACHE_* macros. Unused bits are reserved for future
 * expansion.
 *
 * @see DEVICE_MMIO_MAP()
 * @see device_map()
 * @see k_map()
 *
 * @param _name Name of the top-level MMIO region
 * @param _flags One of the DEVICE_CACHE_* caching modes
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_MAP(_name, _flags) \
	device_map(&_CONCAT(z_mmio_ram__, _name), \
		   _CONCAT(z_mmio_rom__, _name).phys_addr, \
		   _CONCAT(z_mmio_rom__, _name).size, _flags)
#else
#define DEVICE_MMIO_TOPLEVEL_MAP(_name, _flags) do { } while(0)
#endif

/**
 * @def DEVICE_MMIO_TOPLEVEL_GET
 *
 * @brief Obtain the MMIO address for a device declared top-level
 *
 * @see DEVICE_MMIO_GET
 *
 * @param _name Name of the top-level MMIO region
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_GET(_name)	\
		((mm_reg_t)_CONCAT(z_mmio_ram__, _name))
#else
#define DEVICE_MMIO_TOPLEVEL_GET(_name)	\
		((mm_reg_t)_CONCAT(z_mmio_rom__, _name).addr)
#endif
/* @} */

#endif /* ZEPHYR_INCLUDE_SYS_DEVICE_MMIO_H */
