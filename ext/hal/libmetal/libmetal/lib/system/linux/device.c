/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/device.c
 * @brief	Linux libmetal device operations.
 */

#include <metal/device.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <metal/irq.h>

#define MAX_DRIVERS	64

struct linux_bus;
struct linux_device;

struct linux_driver {
	const char		*drv_name;
	const char		*mod_name;
	const char		*cls_name;
	struct sysfs_driver	*sdrv;
	int			(*dev_open)(struct linux_bus *lbus,
					    struct linux_device *ldev);
	void			(*dev_close)(struct linux_bus *lbus,
					     struct linux_device *ldev);
	void			(*dev_irq_ack)(struct linux_bus *lbus,
					     struct linux_device *ldev,
					     int irq);
	int 			(*dev_dma_map)(struct linux_bus *lbus,
						struct linux_device *ldev,
						uint32_t dir,
						struct metal_sg *sg_in,
						int nents_in,
						struct metal_sg *sg_out);
	void 			(*dev_dma_unmap)(struct linux_bus *lbus,
						struct linux_device *ldev,
						uint32_t dir,
						struct metal_sg *sg,
						int nents);
};

struct linux_bus {
	struct metal_bus	bus;
	const char		*bus_name;
	struct linux_driver	drivers[MAX_DRIVERS];
	struct sysfs_bus	*sbus;
};

struct linux_device {
	struct metal_device		device;
	char				dev_name[PATH_MAX];
	char				dev_path[PATH_MAX];
	char				cls_path[PATH_MAX];
	metal_phys_addr_t		region_phys[METAL_MAX_DEVICE_REGIONS];
	struct linux_driver		*ldrv;
	struct sysfs_device		*sdev;
	struct sysfs_attribute		*override;
	int				fd;
};

static struct linux_bus *to_linux_bus(struct metal_bus *bus)
{
	return metal_container_of(bus, struct linux_bus, bus);
}

static struct linux_device *to_linux_device(struct metal_device *device)
{
	return metal_container_of(device, struct linux_device, device);
}

static int metal_uio_read_map_attr(struct linux_device *ldev, unsigned index,
				   const char *name, unsigned long *value)
{
	const char *cls = ldev->cls_path;
	struct sysfs_attribute *attr;
	char path[SYSFS_PATH_MAX];
	int result;

	result = snprintf(path, sizeof(path), "%s/maps/map%u/%s", cls, index, name);
	if (result >= (int)sizeof(path))
		return -EOVERFLOW;
	attr = sysfs_open_attribute(path);
	if (!attr || sysfs_read_attribute(attr) != 0)
		return -errno;

	*value = strtoul(attr->value, NULL, 0);
	return 0;
}

static int metal_uio_dev_bind(struct linux_device *ldev,
			      struct linux_driver *ldrv)
{
	struct sysfs_attribute *attr;
	int result;

	if (strcmp(ldev->sdev->driver_name, ldrv->drv_name) == 0)
		return 0;

	if (strcmp(ldev->sdev->driver_name, SYSFS_UNKNOWN) != 0) {
		metal_log(METAL_LOG_INFO, "device %s in use by driver %s\n",
			  ldev->dev_name, ldev->sdev->driver_name);
		return -EBUSY;
	}

	attr = sysfs_get_device_attr(ldev->sdev, "driver_override");
	if (!attr) {
		metal_log(METAL_LOG_ERROR, "device %s has no override\n",
			  ldev->dev_name);
		return -errno;
	}

	result = sysfs_write_attribute(attr, ldrv->drv_name,
				       strlen(ldrv->drv_name));
	if (result) {
		metal_log(METAL_LOG_ERROR, "failed to set override on %s\n",
			  ldev->dev_name);
		return -errno;
	}
	ldev->override = attr;

	attr = sysfs_get_driver_attr(ldrv->sdrv, "bind");
	if (!attr) {
		metal_log(METAL_LOG_ERROR, "driver %s has no bind\n", ldrv->drv_name);
		return -ENOTSUP;
	}

	result = sysfs_write_attribute(attr, ldev->dev_name,
				       strlen(ldev->dev_name));
	if (result) {
		metal_log(METAL_LOG_ERROR, "failed to bind %s to %s\n",
			  ldev->dev_name, ldrv->drv_name);
		return -errno;
	}

	metal_log(METAL_LOG_DEBUG, "bound device %s to driver %s\n",
		  ldev->dev_name, ldrv->drv_name);

	return 0;
}

static int metal_uio_dev_open(struct linux_bus *lbus, struct linux_device *ldev)
{
	char *instance, path[SYSFS_PATH_MAX];
	struct linux_driver *ldrv = ldev->ldrv;
	unsigned long *phys, offset=0, size=0;
	struct metal_io_region *io;
	struct dlist *dlist;
	int result, i;
	void *virt;
	int irq_info;


	ldev->fd = -1;

	ldev->sdev = sysfs_open_device(lbus->bus_name, ldev->dev_name);
	if (!ldev->sdev) {
		metal_log(METAL_LOG_ERROR, "device %s:%s not found\n",
			  lbus->bus_name, ldev->dev_name);
		return -ENODEV;
	}
	metal_log(METAL_LOG_DEBUG, "opened sysfs device %s:%s\n",
		  lbus->bus_name, ldev->dev_name);

	result = metal_uio_dev_bind(ldev, ldrv);
	if (result)
		return result;

	result = snprintf(path, sizeof(path), "%s/uio", ldev->sdev->path);
	if (result >= (int)sizeof(path))
		return -EOVERFLOW;
	dlist = sysfs_open_directory_list(path);
	if (!dlist) {
		metal_log(METAL_LOG_ERROR, "failed to scan class path %s\n",
			  path);
		return -errno;
	}

	dlist_for_each_data(dlist, instance, char) {
		result = snprintf(ldev->cls_path, sizeof(ldev->cls_path),
				  "%s/%s", path, instance);
		if (result >= (int)sizeof(ldev->cls_path))
			return -EOVERFLOW;
		result = snprintf(ldev->dev_path, sizeof(ldev->dev_path),
				  "/dev/%s", instance);
		if (result >= (int)sizeof(ldev->dev_path))
			return -EOVERFLOW;
		break;
	}
	sysfs_close_list(dlist);

	if (sysfs_path_is_dir(ldev->cls_path) != 0) {
		metal_log(METAL_LOG_ERROR, "invalid device class path %s\n",
			  ldev->cls_path);
		return -ENODEV;
	}

	i = 0;
	do {
		if (!access(ldev->dev_path, F_OK))
			break;
		usleep(10);
		i++;
	} while (i < 1000);
	if (i >= 1000) {
		metal_log(METAL_LOG_ERROR, "failed to open file %s, timeout.\n",
			  ldev->dev_path);
		return -ENODEV;
	}
	result = metal_open(ldev->dev_path, 0);
	if (result < 0) {
		metal_log(METAL_LOG_ERROR, "failed to open device %s\n",
			  ldev->dev_path, strerror(-result));
		return result;
	}
	ldev->fd = result;

	metal_log(METAL_LOG_DEBUG, "opened %s:%s as %s\n",
		  lbus->bus_name, ldev->dev_name, ldev->dev_path);

	for (i = 0, result = 0; !result && i < METAL_MAX_DEVICE_REGIONS; i++) {
		phys = &ldev->region_phys[ldev->device.num_regions];
		result = (result ? result :
			 metal_uio_read_map_attr(ldev, i, "offset", &offset));
		result = (result ? result :
			 metal_uio_read_map_attr(ldev, i, "addr", phys));
		result = (result ? result :
			 metal_uio_read_map_attr(ldev, i, "size", &size));
		result = (result ? result :
			 metal_map(ldev->fd, offset, size, 0, 0, &virt));
		if (!result) {
			io = &ldev->device.regions[ldev->device.num_regions];
			metal_io_init(io, virt, phys, size, -1, 0, NULL);
			ldev->device.num_regions++;
		}
	}

	irq_info = 1;
	if (write(ldev->fd, &irq_info, sizeof(irq_info)) <= 0) {
		metal_log(METAL_LOG_INFO,
			  "%s: No IRQ for device %s.\n",
			  __func__, ldev->dev_name);
		ldev->device.irq_num =  0;
		ldev->device.irq_info = (void *)-1;
	} else {
		ldev->device.irq_num =  1;
		ldev->device.irq_info = (void *)(intptr_t)ldev->fd;
	}

	return 0;
}

static void metal_uio_dev_close(struct linux_bus *lbus,
				struct linux_device *ldev)
{
	(void)lbus;

	if ((intptr_t)ldev->device.irq_info >= 0)
		/* Normally this call would not be needed, and is added as precaution.
		   Also for uio there is only 1 interrupt associated to the fd/device,
		   we therefore do not need to specify a particular device */
		metal_irq_unregister(ldev->fd, NULL, NULL, NULL);

	if (ldev->override) {
		sysfs_write_attribute(ldev->override, "", 1);
		ldev->override = NULL;
	}
	if (ldev->sdev) {
		sysfs_close_device(ldev->sdev);
		ldev->sdev = NULL;
	}
	if (ldev->fd >= 0) {
		close(ldev->fd);
	}
}

static void metal_uio_dev_irq_ack(struct linux_bus *lbus,
				 struct linux_device *ldev,
				 int irq)
{
	(void)lbus;
	(void)irq;
	int irq_info = 1;
	unsigned int val;
	int ret;

	ret = read(ldev->fd, (void *)&val, sizeof(val));
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s, read uio irq fd %d failed: %d.\n",
						__func__, ldev->fd, ret);
		return;
	}
	ret = write(ldev->fd, &irq_info, sizeof(irq_info));
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s, write uio irq fd %d failed: %d.\n",
						__func__, ldev->fd, errno);
	}
}

static int metal_uio_dev_dma_map(struct linux_bus *lbus,
				 struct linux_device *ldev,
				 uint32_t dir,
				 struct metal_sg *sg_in,
				 int nents_in,
				 struct metal_sg *sg_out)
{
	int i, j;
	void *vaddr_sg_lo, *vaddr_sg_hi, *vaddr_lo, *vaddr_hi;
	struct metal_io_region *io;

	(void)lbus;
	(void)dir;

	/* Check if the the input virt address is MMIO address */
	for (i = 0; i < nents_in; i++) {
		vaddr_sg_lo = sg_in[i].virt;
		vaddr_sg_hi = vaddr_sg_lo + sg_in[i].len;
		for (j = 0, io = ldev->device.regions;
		     j < (int)ldev->device.num_regions; j++, io++) {
			vaddr_lo = io->virt;
			vaddr_hi = vaddr_lo + io->size;
			if (vaddr_sg_lo >= vaddr_lo &&
			    vaddr_sg_hi <= vaddr_hi) {
				break;
			}
		}
		if (j == (int)ldev->device.num_regions) {
			metal_log(METAL_LOG_WARNING,
			  "%s,%s: input address isn't MMIO addr: 0x%x,%d.\n",
			__func__, ldev->dev_name, vaddr_sg_lo, sg_in[i].len);
			return -EINVAL;
		}
	}
	if (sg_out != sg_in)
		memcpy(sg_out, sg_in, nents_in*(sizeof(struct metal_sg)));
	return nents_in;
}

static void metal_uio_dev_dma_unmap(struct linux_bus *lbus,
				 struct linux_device *ldev,
				 uint32_t dir,
				 struct metal_sg *sg,
				 int nents)
{
	(void) lbus;
	(void) ldev;
	(void) dir;
	(void) sg;
	(void) nents;
	return;
}

static struct linux_bus linux_bus[] = {
	{
		.bus_name	= "platform",
		.drivers = {
			{
				.drv_name  = "uio_pdrv_genirq",
				.mod_name  = "uio_pdrv_genirq",
				.cls_name  = "uio",
				.dev_open  = metal_uio_dev_open,
				.dev_close = metal_uio_dev_close,
				.dev_irq_ack  = metal_uio_dev_irq_ack,
				.dev_dma_map = metal_uio_dev_dma_map,
				.dev_dma_unmap = metal_uio_dev_dma_unmap,
			},
			{
				.drv_name  = "uio_dmem_genirq",
				.mod_name  = "uio_dmem_genirq",
				.cls_name  = "uio",
				.dev_open  = metal_uio_dev_open,
				.dev_close = metal_uio_dev_close,
				.dev_irq_ack  = metal_uio_dev_irq_ack,
				.dev_dma_map = metal_uio_dev_dma_map,
				.dev_dma_unmap = metal_uio_dev_dma_unmap,
			},
			{ 0 /* sentinel */ }
		}
	},
	{
		.bus_name	= "pci",
		.drivers = {
			{
				.drv_name  = "vfio-pci",
				.mod_name  = "vfio-pci",
			},
			{
				.drv_name  = "uio_pci_generic",
				.mod_name  = "uio_pci_generic",
				.cls_name  = "uio",
				.dev_open  = metal_uio_dev_open,
				.dev_close = metal_uio_dev_close,
				.dev_irq_ack  = metal_uio_dev_irq_ack,
				.dev_dma_map = metal_uio_dev_dma_map,
				.dev_dma_unmap = metal_uio_dev_dma_unmap,
			},
			{ 0 /* sentinel */ }
		}
	},
	{
		/* sentinel */
		.bus_name = NULL,
	},
};

#define for_each_linux_bus(lbus)					\
	for ((lbus) = linux_bus; (lbus)->bus_name; (lbus)++)
#define for_each_linux_driver(lbus, ldrv)			\
	for ((ldrv) = lbus->drivers; (ldrv)->drv_name; (ldrv)++)


static int metal_linux_dev_open(struct metal_bus *bus,
				const char *dev_name,
				struct metal_device **device)
{
	struct linux_bus *lbus = to_linux_bus(bus);
	struct linux_device *ldev = NULL;
	struct linux_driver *ldrv;
	int error;

	ldev = malloc(sizeof(*ldev));
	if (!ldev)
		return -ENOMEM;

	for_each_linux_driver(lbus, ldrv) {

		/* Check if we have a viable driver. */
		if (!ldrv->sdrv || !ldrv->dev_open)
			continue;

		/* Allocate a linux device if we haven't already. */
		if (!ldev)
			ldev = malloc(sizeof(*ldev));
		if (!ldev)
			return -ENOMEM;

		/* Reset device data. */
		memset(ldev, 0, sizeof(*ldev));
		strncpy(ldev->dev_name, dev_name, sizeof(ldev->dev_name) - 1);
		ldev->fd = -1;
		ldev->ldrv = ldrv;
		ldev->device.bus = bus;

		/* Try and open the device. */
		error = ldrv->dev_open(lbus, ldev);
		if (error) {
			ldrv->dev_close(lbus, ldev);
			continue;
		}

		*device = &ldev->device;
		(*device)->name = ldev->dev_name;

		metal_list_add_tail(&bus->devices, &(*device)->node);
		return 0;
	}

	if (ldev)
		free(ldev);

	return -ENODEV;
}

static void metal_linux_dev_close(struct metal_bus *bus,
				  struct metal_device *device)
{
	struct linux_device *ldev = to_linux_device(device);
	struct linux_bus *lbus = to_linux_bus(bus);

	ldev->ldrv->dev_close(lbus, ldev);
	metal_list_del(&device->node);
	free(ldev);
}

static void metal_linux_bus_close(struct metal_bus *bus)
{
	struct linux_bus *lbus = to_linux_bus(bus);
	struct linux_driver *ldrv;

	for_each_linux_driver(lbus, ldrv) {
		if (ldrv->sdrv)
			sysfs_close_driver(ldrv->sdrv);
		ldrv->sdrv = NULL;
	}

	sysfs_close_bus(lbus->sbus);
	lbus->sbus = NULL;
}

static void metal_linux_dev_irq_ack(struct metal_bus *bus,
			     struct metal_device *device,
			     int irq)
{
	struct linux_device *ldev = to_linux_device(device);
	struct linux_bus *lbus = to_linux_bus(bus);

	return ldev->ldrv->dev_irq_ack(lbus, ldev, irq);
}

static int metal_linux_dev_dma_map(struct metal_bus *bus,
			     struct metal_device *device,
			     uint32_t dir,
			     struct metal_sg *sg_in,
			     int nents_in,
			     struct metal_sg *sg_out)
{
	struct linux_device *ldev = to_linux_device(device);
	struct linux_bus *lbus = to_linux_bus(bus);

	return ldev->ldrv->dev_dma_map(lbus, ldev, dir, sg_in,
				       nents_in, sg_out);
}

static void metal_linux_dev_dma_unmap(struct metal_bus *bus,
			        struct metal_device *device,
			        uint32_t dir,
			        struct metal_sg *sg,
			        int nents)
{
	struct linux_device *ldev = to_linux_device(device);
	struct linux_bus *lbus = to_linux_bus(bus);

	ldev->ldrv->dev_dma_unmap(lbus, ldev, dir, sg,
				       nents);
}

static const struct metal_bus_ops metal_linux_bus_ops = {
	.bus_close	= metal_linux_bus_close,
	.dev_open	= metal_linux_dev_open,
	.dev_close	= metal_linux_dev_close,
	.dev_irq_ack	= metal_linux_dev_irq_ack,
	.dev_dma_map	= metal_linux_dev_dma_map,
	.dev_dma_unmap	= metal_linux_dev_dma_unmap,
};

static int metal_linux_register_bus(struct linux_bus *lbus)
{
	lbus->bus.name = lbus->bus_name;
	lbus->bus.ops  = metal_linux_bus_ops;
	return metal_bus_register(&lbus->bus);
}

static int metal_linux_probe_driver(struct linux_bus *lbus,
				    struct linux_driver *ldrv)
{
	char command[256];
	int ret;

	ldrv->sdrv = sysfs_open_driver(lbus->bus_name, ldrv->drv_name);

	/* Try probing the module and then open the driver. */
	if (!ldrv->sdrv) {
		ret = snprintf(command, sizeof(command),
			       "modprobe %s > /dev/null 2>&1", ldrv->mod_name);
		if (ret >= (int)sizeof(command))
			return -EOVERFLOW;
		ret = system(command);
		if (ret < 0) {
			metal_log(METAL_LOG_WARNING,
				  "%s: executing system command '%s' failed.\n",
				  __func__, command);
		}
		ldrv->sdrv = sysfs_open_driver(lbus->bus_name, ldrv->drv_name);
	}

	/* Try sudo probing the module and then open the driver. */
	if (!ldrv->sdrv) {
		ret = snprintf(command, sizeof(command),
			       "sudo modprobe %s > /dev/null 2>&1", ldrv->mod_name);
		if (ret >= (int)sizeof(command))
			return -EOVERFLOW;
		ret = system(command);
		if (ret < 0) {
			metal_log(METAL_LOG_WARNING,
				  "%s: executing system command '%s' failed.\n",
				  __func__, command);
		}
		ldrv->sdrv = sysfs_open_driver(lbus->bus_name, ldrv->drv_name);
	}

	/* If all else fails... */
	return ldrv->sdrv ? 0 : -ENODEV;
}

static int metal_linux_probe_bus(struct linux_bus *lbus)
{
	struct linux_driver *ldrv;
	int error = -ENODEV;

	lbus->sbus = sysfs_open_bus(lbus->bus_name);
	if (!lbus->sbus)
		return -ENODEV;

	for_each_linux_driver(lbus, ldrv) {
		error = metal_linux_probe_driver(lbus, ldrv);
		if (!error)
			break;
	}

	if (error) {
		sysfs_close_bus(lbus->sbus);
		lbus->sbus = NULL;
		return error;
	}

	error = metal_linux_register_bus(lbus);
	if (error) {
		sysfs_close_driver(ldrv->sdrv);
		ldrv->sdrv = NULL;
		sysfs_close_bus(lbus->sbus);
		lbus->sbus = NULL;
	}

	return error;
}

int metal_linux_bus_init(void)
{
	struct linux_bus *lbus;
	int valid = 0;

	for_each_linux_bus(lbus)
		valid += metal_linux_probe_bus(lbus) ? 0 : 1;

	return valid ? 0 : -ENODEV;
}

void metal_linux_bus_finish(void)
{
	struct linux_bus *lbus;
	struct metal_bus *bus;

	for_each_linux_bus(lbus) {
		if (metal_bus_find(lbus->bus_name, &bus) == 0)
			metal_bus_unregister(bus);
	}
}

int metal_generic_dev_sys_open(struct metal_device *dev)
{
	(void)dev;
	return 0;
}

int metal_linux_get_device_property(struct metal_device *device,
				    const char *property_name,
				    void *output, int len)
{
	int fd = 0;
	int status = 0;
	const int flags = O_RDONLY;
	const int mode = S_IRUSR | S_IRGRP | S_IROTH;
	struct linux_device *ldev = to_linux_device(device);
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s/of_node/%s",
			 ldev->sdev->path, property_name);
	fd = open(path, flags, mode);
	if (fd < 0)
		return -errno;
	status = read(fd, output, len);

	return status < 0 ? -errno : 0;
}

