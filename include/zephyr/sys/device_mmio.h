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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

/**
 * @defgroup device-mmio Device memory-mapped IO management
 * @ingroup device_model
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
#if defined(CONFIG_MMU) || defined(CONFIG_PCIE) || defined(CONFIG_EXTERNAL_ADDRESS_TRANSLATION)
#define DEVICE_MMIO_IS_IN_RAM
#endif

#if defined(CONFIG_EXTERNAL_ADDRESS_TRANSLATION)
#include <zephyr/drivers/mm/system_mm.h>
#endif

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/sys/sys_io.h>

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

#define Z_DEVICE_MMIO_ROM_INITIALIZER(node_id) \
	{ \
		.phys_addr = DT_REG_ADDR(node_id), \
		.size = DT_REG_SIZE(node_id) \
	}

#define Z_DEVICE_MMIO_NAMED_ROM_INITIALIZER(name, node_id) \
	{ \
		.phys_addr = DT_REG_ADDR_BY_NAME(node_id, name), \
		.size = DT_REG_SIZE_BY_NAME(node_id, name) \
	}

/**
 * Set linear address for device MMIO access
 *
 * This function sets the `virt_addr` parameter to the correct linear
 * address for the MMIO region.
 *
 * If the MMU is enabled, mappings may be created in the page tables.
 *
 * Normally, only a caching mode needs to be set for the 'flags' parameter.
 * The mapped linear address will have read-write access to supervisor mode.
 *
 * @see k_map()
 *
 * @param virt_addr [out] Output linear address storage location, most
 *		users will want some DEVICE_MMIO_RAM_PTR() value
 * @param phys_addr Physical address base of the MMIO region
 * @param size Size of the MMIO region
 * @param flags Caching mode and access flags, see K_MEM_CACHE_* and
 *              K_MEM_PERM_* macros
 */
__boot_func
static inline void device_map(mm_reg_t *virt_addr, uintptr_t phys_addr,
			      size_t size, uint32_t flags)
{
#ifdef CONFIG_MMU
	/* Pass along flags and add that we want supervisor mode
	 * read-write access.
	 */
	z_phys_map((uint8_t **)virt_addr, phys_addr, size,
		   flags | K_MEM_PERM_RW);
#else
	ARG_UNUSED(size);
	ARG_UNUSED(flags);
#ifdef CONFIG_EXTERNAL_ADDRESS_TRANSLATION
	sys_mm_drv_page_phys_get((void *) phys_addr, virt_addr);
#else
	*virt_addr = phys_addr;
#endif /* CONFIG_EXTERNAL_ADDRESS_TRANSLATION */
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

#define Z_DEVICE_MMIO_ROM_INITIALIZER(node_id) \
	{ \
		.addr = (mm_reg_t)DT_REG_ADDR_U64(node_id) \
	}

#define Z_DEVICE_MMIO_NAMED_ROM_INITIALIZER(name, node_id) \
	{ \
		.addr = (mm_reg_t)DT_REG_ADDR_BY_NAME_U64(node_id, name) \
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
 * This must be the first member of the data struct.
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
 * Return a pointer to the RAM-based storage area for a device's MMIO
 * address.
 *
 * This is useful for the target MMIO address location when using
 * device_map() directly.
 *
 * @param device device node_id object
 * @retval mm_reg_t  pointer to storage location
 */
#define DEVICE_MMIO_RAM_PTR(device)	(mm_reg_t *)((device)->data)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @brief Declare storage for MMIO data within a device's config struct
 *
 * This gets accessed by DEVICE_MMIO_MAP() and DEVICE_MMIO_GET() macros.
 *
 * What gets stored here varies considerably by configuration.
 * This must be the first member of the config struct. There must be
 * a corresponding DEVICE_MMIO_RAM in data.
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
 * struct foo_config {
 *	DEVICE_MMIO_ROM;
 *	int baz;
 *	...
 * }
 *
 * @see DEVICE_MMIO_ROM_INIT()
 */
#define DEVICE_MMIO_ROM		struct z_device_mmio_rom _mmio

/**
 * Return a pointer to the ROM-based storage area for a device's MMIO
 * information. This macro will not work properly if the ROM storage
 * was omitted from the config struct declaration, and should not
 * be used in this case.
 *
 * @param dev device instance object
 * @retval struct device_mmio_rom * pointer to storage location
 */
#define DEVICE_MMIO_ROM_PTR(dev) \
	((struct z_device_mmio_rom *)((dev)->config))

/**
 * @brief Initialize a DEVICE_MMIO_ROM member
 *
 * Initialize MMIO-related information within a specific instance of
 * a device config struct, using information from DTS.
 *
 * Example for a driver belonging to the "foo" subsystem:
 *
 * struct foo_config my_config = {
 *	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(...)),
 *	.baz = 2;
 *	...
 * }
 *
 * @see DEVICE_MMIO_ROM()
 *
 * @param node_id DTS node_id
 */
#define DEVICE_MMIO_ROM_INIT(node_id) \
	._mmio = Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)

/**
 * @def DEVICE_MMIO_MAP(device, flags)
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
 * @param dev Device object instance
 * @param flags cache mode flags
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_MAP(dev, flags) \
	device_map(DEVICE_MMIO_RAM_PTR(dev), \
		   DEVICE_MMIO_ROM_PTR(dev)->phys_addr, \
		   DEVICE_MMIO_ROM_PTR(dev)->size, \
		   (flags))
#else
#define DEVICE_MMIO_MAP(dev, flags) do { } while (false)
#endif

/**
 * @def DEVICE_MMIO_GET(dev)
 *
 * @brief Obtain the MMIO address for a device
 *
 * For most microcontrollers MMIO addresses can be fixed values known at
 * build time, and we can store this in device->config, residing in ROM.
 *
 * However, some devices can only know their MMIO addresses at runtime,
 * because they need to be memory-mapped into the address space, enumerated
 * from PCI, or both.
 *
 * This macro returns the linear address of the driver's MMIO region.
 * This is for drivers which have exactly one MMIO region.
 * A call must have been made to device_map() in the driver init function.
 *
 * @param dev Device object
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_GET(dev)	(*DEVICE_MMIO_RAM_PTR(dev))
#else
#define DEVICE_MMIO_GET(dev)	(DEVICE_MMIO_ROM_PTR(dev)->addr)
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
 * @def DEVICE_MMIO_NAMED_RAM(name)
 *
 * @brief Declare storage for MMIO data within a device's dev_data struct
 *
 * This gets accessed by the DEVICE_MMIO_NAMED_MAP() and
 * DEVICE_MMIO_NAMED_GET() macros.
 *
 * Depending on configuration, no memory may be reserved at all.
 * Multiple named regions may be declared.
 *
 * There must be a corresponding DEVICE_MMIO_ROM in config if the
 * physical address is known at build time, but may be omitted if not (such
 * as with PCIe.
 *
 * Example for a driver named "foo":
 *
 * struct foo_driver_data {
 *      int blarg;
 *      DEVICE_MMIO_NAMED_RAM(corge);
 *      DEVICE_MMIO_NAMED_RAM(grault);
 *      int wibble;
 *      ...
 * }
 *
 * No build-time initialization of this memory is necessary; it
 * will be set up in the init function by DEVICE_MMIO_NAMED_MAP().
 *
 * @param name Member name to use to store within dev_data.
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_RAM(name)	mm_reg_t name
#else
#define DEVICE_MMIO_NAMED_RAM(name)
#endif /* DEVICE_MMIO_IS_IN_RAM */

#ifdef DEVICE_MMIO_IS_IN_RAM
/**
 * @brief Return a pointer to the RAM storage for a device's named MMIO address
 *
 * This macro requires that the macro DEV_DATA is locally defined and returns
 * a properly typed pointer to the particular dev_data struct for this driver.
 *
 * @param dev device instance object
 * @param name Member name within dev_data
 * @retval mm_reg_t  pointer to storage location
 */
#define DEVICE_MMIO_NAMED_RAM_PTR(dev, name) \
		(&(DEV_DATA(dev)->name))
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @brief Declare storage for MMIO data within a device's config struct.
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
 * struct foo_config {
 *      int bar;
 *      DEVICE_MMIO_NAMED_ROM(corge);
 *      DEVICE_MMIO_NAMED_ROM(grault);
 *      int baz;
 *      ...
 * }
 *
 * @see DEVICE_MMIO_NAMED_ROM_INIT()
 *
 * @param name Member name to store within config
 */
#define DEVICE_MMIO_NAMED_ROM(name) struct z_device_mmio_rom name

/**
 * Return a pointer to the ROM-based storage area for a device's MMIO
 * information.
 *
 * This macro requires that the macro DEV_CFG is locally defined and returns
 * a properly typed pointer to the particular config struct for this
 * driver.
 *
 * @param dev device instance object
 * @param name Member name within config
 * @retval struct device_mmio_rom * pointer to storage location
 */
#define DEVICE_MMIO_NAMED_ROM_PTR(dev, name) (&(DEV_CFG(dev)->name))

/**
 * @brief Initialize a named DEVICE_MMIO_NAMED_ROM member
 *
 * Initialize MMIO-related information within a specific instance of
 * a device config struct, using information from DTS.
 *
 * Example for an instance of a driver belonging to the "foo" subsystem
 * that will have two regions named 'corge' and 'grault':
 *
 * struct foo_config my_config = {
 *	bar = 7;
 *	DEVICE_MMIO_NAMED_ROM_INIT(corge, DT_DRV_INST(...));
 *	DEVICE_MMIO_NAMED_ROM_INIT(grault, DT_DRV_INST(...));
 *	baz = 2;
 *	...
 * }
 *
 * @see DEVICE_MMIO_NAMED_ROM()
 *
 * @param name Member name within config for the MMIO region
 * @param node_id DTS node identifier
 */
#define DEVICE_MMIO_NAMED_ROM_INIT(name, node_id) \
	.name = Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)

/**
 * @brief Initialize a named DEVICE_MMIO_NAMED_ROM member using a named DT
 *        reg property.
 *
 * Same as @ref DEVICE_MMIO_NAMED_ROM_INIT but the size and address are taken
 * from a named DT reg property.
 *
 * Example for an instance of a driver belonging to the "foo" subsystem
 * that will have two DT-defined regions named 'chip' and 'dale':
 *
 * @code{.dts}
 *
 *    foo@E5000000 {
 *         reg = <0xE5000000 0x1000>, <0xE6000000 0x1000>;
 *         reg-names = "chip", "dale";
 *         ...
 *    };
 *
 * @endcode
 *
 * @code{.c}
 *
 * struct foo_config my_config = {
 *	bar = 7;
 *	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(chip, DT_DRV_INST(...));
 *	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(dale, DT_DRV_INST(...));
 *	baz = 2;
 *	...
 * }
 *
 * @endcode
 *
 * @see DEVICE_MMIO_NAMED_ROM_INIT()
 *
 * @param name Member name within config for the MMIO region and name of the
 *             reg property in the DT
 * @param node_id DTS node identifier
 */
#define DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(name, node_id) \
	.name = Z_DEVICE_MMIO_NAMED_ROM_INITIALIZER(name, node_id)

/**
 * @brief Set up memory for a named MMIO region
 *
 * This performs the necessary PCI probing and/or MMU virtual memory mapping
 * such that DEVICE_MMIO_GET(name) returns a suitable linear memory address
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
 * and config structs for this driver.
 *
 * The flags argument is currently used for caching mode, which should be
 * one of the DEVICE_CACHE_* macros. Unused bits are reserved for future
 * expansion.
 *
 * @param dev Device object
 * @param name Member name for MMIO information, as declared with
 *             DEVICE_MMIO_NAMED_RAM/DEVICE_MMIO_NAMED_ROM
 * @param flags One of the DEVICE_CACHE_* caching modes
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_MAP(dev, name, flags) \
	device_map(DEVICE_MMIO_NAMED_RAM_PTR((dev), name), \
		   (DEVICE_MMIO_NAMED_ROM_PTR((dev), name)->phys_addr), \
		   (DEVICE_MMIO_NAMED_ROM_PTR((dev), name)->size), \
		   (flags))
#else
#define DEVICE_MMIO_NAMED_MAP(dev, name, flags) do { } while (false)
#endif

/**
 * @def DEVICE_MMIO_NAMED_GET(dev, name)
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
 * and config structs for this driver.
 *
 * @see DEVICE_MMIO_GET
 *
 * @param dev Device object
 * @param name Member name for MMIO information, as declared with
 *             DEVICE_MMIO_NAMED_RAM/DEVICE_MMIO_NAMED_ROM
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_NAMED_GET(dev, name) \
		(*DEVICE_MMIO_NAMED_RAM_PTR((dev), name))
#else
#define DEVICE_MMIO_NAMED_GET(dev, name) \
		((DEVICE_MMIO_NAMED_ROM_PTR((dev), name))->addr)
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

 #define Z_TOPLEVEL_ROM_NAME(name) _CONCAT(z_mmio_rom__, name)
 #define Z_TOPLEVEL_RAM_NAME(name) _CONCAT(z_mmio_ram__, name)

/**
 * @def DEVICE_MMIO_TOPLEVEL(name, node_id)
 *
 * @brief Declare top-level storage for MMIO information, global scope
 *
 * This is intended for drivers which do not use Zephyr's driver model
 * of config/dev_data linked to a struct device.
 *
 * Instead, this is a top-level declaration for the driver's C file.
 * The scope of this declaration is global and may be referenced by
 * other C files, using DEVICE_MMIO_TOPLEVEL_DECLARE.
 *
 * @param name Base symbol name
 * @param node_id Device-tree node identifier for this region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL(name, node_id) \
	__pinned_bss \
	mm_reg_t Z_TOPLEVEL_RAM_NAME(name); \
	__pinned_rodata \
	const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)
#else
#define DEVICE_MMIO_TOPLEVEL(name, node_id) \
	__pinned_rodata \
	const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def DEVICE_MMIO_TOPLEVEL_DECLARE(name)
 *
 * Provide an extern reference to a top-level MMIO region
 *
 * If a top-level MMIO region defined with DEVICE_MMIO_DEFINE needs to be
 * referenced from other C files, this macro provides the necessary extern
 * definitions.
 *
 * @see DEVICE_MMIO_TOPLEVEL
 *
 * @param name Name of the top-level MMIO region
 */

#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_DECLARE(name) \
	extern mm_reg_t Z_TOPLEVEL_RAM_NAME(name); \
	extern const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name)
#else
#define DEVICE_MMIO_TOPLEVEL_DECLARE(name) \
	extern const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * @def  DEVICE_MMIO_TOPLEVEL_STATIC(name, node_id)
 *
 * @brief Declare top-level storage for MMIO information, static scope
 *
 * This is intended for drivers which do not use Zephyr's driver model
 * of config/dev_data linked to a struct device.
 *
 * Instead, this is a top-level declaration for the driver's C file.
 * The scope of this declaration is static.
 *
 * @param name Name of the top-level MMIO region
 * @param node_id Device-tree node identifier for this region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_STATIC(name, node_id) \
	__pinned_bss \
	static mm_reg_t Z_TOPLEVEL_RAM_NAME(name); \
	__pinned_rodata \
	static const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)
#else
#define DEVICE_MMIO_TOPLEVEL_STATIC(name, node_id) \
	__pinned_rodata \
	static const struct z_device_mmio_rom Z_TOPLEVEL_ROM_NAME(name) = \
		Z_DEVICE_MMIO_ROM_INITIALIZER(node_id)
#endif /* DEVICE_MMIO_IS_IN_RAM */

#ifdef DEVICE_MMIO_IS_IN_RAM
/**
 * @brief Return a pointer to the RAM storage for a device's toplevel MMIO
 * address.
 *
 * @param name Name of toplevel MMIO region
 * @retval mm_reg_t  pointer to storage location
 */
#define DEVICE_MMIO_TOPLEVEL_RAM_PTR(name) &Z_TOPLEVEL_RAM_NAME(name)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/**
 * Return a pointer to the ROM-based storage area for a toplevel MMIO region.
 *
 * @param name MMIO region name
 * @retval struct device_mmio_rom * pointer to storage location
 */
#define DEVICE_MMIO_TOPLEVEL_ROM_PTR(name) &Z_TOPLEVEL_ROM_NAME(name)

/**
 * @def DEVICE_MMIO_TOPLEVEL_MAP(name, flags)
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
 * @param name Name of the top-level MMIO region
 * @param flags One of the DEVICE_CACHE_* caching modes
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_MAP(name, flags) \
	device_map(&Z_TOPLEVEL_RAM_NAME(name), \
		   Z_TOPLEVEL_ROM_NAME(name).phys_addr, \
		   Z_TOPLEVEL_ROM_NAME(name).size, flags)
#else
#define DEVICE_MMIO_TOPLEVEL_MAP(name, flags) do { } while (false)
#endif

/**
 * @def DEVICE_MMIO_TOPLEVEL_GET(name)
 *
 * @brief Obtain the MMIO address for a device declared top-level
 *
 * @see DEVICE_MMIO_GET
 *
 * @param name Name of the top-level MMIO region
 * @return mm_reg_t  linear address of the MMIO region
 */
#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_TOPLEVEL_GET(name)	\
		((mm_reg_t)Z_TOPLEVEL_RAM_NAME(name))
#else
#define DEVICE_MMIO_TOPLEVEL_GET(name)	\
		((mm_reg_t)Z_TOPLEVEL_ROM_NAME(name).addr)
#endif
/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_DEVICE_MMIO_H */
