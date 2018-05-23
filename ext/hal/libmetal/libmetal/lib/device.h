/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	device.h
 * @brief	Bus abstraction for libmetal.
 */

#ifndef __METAL_BUS__H__
#define __METAL_BUS__H__

#include <stdint.h>
#include <metal/io.h>
#include <metal/list.h>
#include <metal/dma.h>
#include <metal/sys.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup device Bus Abstraction
 *  @{ */

#ifndef METAL_MAX_DEVICE_REGIONS
#define METAL_MAX_DEVICE_REGIONS	32
#endif

struct metal_bus;
struct metal_device;

/** Bus operations. */
struct metal_bus_ops {
	void		(*bus_close)(struct metal_bus *bus);
	int		(*dev_open)(struct metal_bus *bus,
				    const char *dev_name,
				    struct metal_device **device);
	void		(*dev_close)(struct metal_bus *bus,
				     struct metal_device *device);
	void		(*dev_irq_ack)(struct metal_bus *bus,
				     struct metal_device *device,
				     int irq);
	int		(*dev_dma_map)(struct metal_bus *bus,
				       struct metal_device *device,
				       uint32_t dir,
				       struct metal_sg *sg_in,
				       int nents_in,
				       struct metal_sg *sg_out);
	void		(*dev_dma_unmap)(struct metal_bus *bus,
				       struct metal_device *device,
				       uint32_t dir,
				       struct metal_sg *sg,
				       int nents);
};

/** Libmetal bus structure. */
struct metal_bus {
	const char		*name;
	struct metal_bus_ops	ops;
	struct metal_list	devices;
	struct metal_list	node;
};

/** Libmetal generic bus. */
extern struct metal_bus metal_generic_bus;

/** Libmetal device structure. */
struct metal_device {
	const char             *name;       /**< Device name */
	struct metal_bus       *bus;        /**< Bus that contains device */
	unsigned               num_regions; /**< Number of I/O regions in
					      device */
	struct metal_io_region regions[METAL_MAX_DEVICE_REGIONS]; /**< Array of
                                                        I/O regions in device*/
	struct metal_list      node;       /**< Node on bus' list of devices */
	int                    irq_num;    /**< Number of IRQs per device */
	void                   *irq_info;  /**< IRQ ID */
};

/**
 * @brief	Register a libmetal bus.
 * @param[in]	bus	Pre-initialized bus structure.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_bus_register(struct metal_bus *bus);

/**
 * @brief	Unregister a libmetal bus.
 * @param[in]	bus	Pre-registered bus structure.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_bus_unregister(struct metal_bus *bus);

/**
 * @brief	Find a libmetal bus by name.
 * @param[in]	name	Bus name.
 * @param[out]	bus	Returned bus handle.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_bus_find(const char *name, struct metal_bus **bus);

/**
 * @brief	Statically register a generic libmetal device.
 *
 * In non-Linux systems, devices are always required to be statically
 * registered at application initialization.
 * In Linux system, devices can be dynamically opened via sysfs or libfdt based
 * enumeration at runtime.
 * This interface is used for static registration of devices. Subsequent calls
 * to metal_device_open() look up in this list of pre-registered devices on the
 * "generic" bus.
 * "generic" bus is used on non-Linux system to group the memory mapped devices.
 *
 * @param[in]	device	Generic device.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_register_generic_device(struct metal_device *device);

/**
 * @brief	Open a libmetal device by name.
 * @param[in]	bus_name	Bus name.
 * @param[in]	dev_name	Device name.
 * @param[out]	device		Returned device handle.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_device_open(const char *bus_name, const char *dev_name,
			     struct metal_device **device);

/**
 * @brief	Close a libmetal device.
 * @param[in]	device		Device handle.
 */
extern void metal_device_close(struct metal_device *device);

/**
 * @brief	Get an I/O region accessor for a device region.
 *
 * @param[in]	device		Device handle.
 * @param[in]	index		Region index.
 * @return I/O accessor handle, or NULL on failure.
 */
static inline struct metal_io_region *
metal_device_io_region(struct metal_device *device, unsigned index)
{
	return (index < device->num_regions
		? &device->regions[index]
		: NULL);
}

/** @} */

#ifdef METAL_INTERNAL
extern int metal_generic_dev_sys_open(struct metal_device *dev);
extern int metal_generic_dev_open(struct metal_bus *bus, const char *dev_name,
				  struct metal_device **device);
extern int metal_generic_dev_dma_map(struct metal_bus *bus,
				     struct metal_device *device,
				     uint32_t dir,
				     struct metal_sg *sg_in,
				     int nents_in,
				     struct metal_sg *sg_out);
extern void metal_generic_dev_dma_unmap(struct metal_bus *bus,
					struct metal_device *device,
					uint32_t dir,
					struct metal_sg *sg,
					int nents);
#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_BUS__H__ */
