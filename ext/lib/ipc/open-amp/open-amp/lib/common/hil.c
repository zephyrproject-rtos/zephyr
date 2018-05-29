/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       hil.c
 *
 * COMPONENT
 *
 *         OpenAMP  Stack.
 *
 * DESCRIPTION
 *
 *       This file is implementation of generic part of HIL.
 *
 *
 *
 **************************************************************************/

#include <openamp/hil.h>
#include <openamp/remoteproc.h>
#include <metal/io.h>
#include <metal/alloc.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/shmem.h>
#include <metal/utilities.h>
#include <metal/time.h>
#include <metal/cache.h>
#include <stdio.h>

#define DEFAULT_VRING_MEM_SIZE 0x10000
#define HIL_DEV_NAME_PREFIX "hil-dev."

/*--------------------------- Globals ---------------------------------- */
static METAL_DECLARE_LIST (procs);

#if defined (OPENAMP_BENCHMARK_ENABLE)

unsigned long long boot_time_stamp;
unsigned long long shutdown_time_stamp;

#endif

struct hil_mem_device {
	struct metal_device device;
	char name[64];
	metal_phys_addr_t pa;
};

metal_phys_addr_t hil_generic_start_paddr = 0;

static int hil_shm_block_write(struct metal_io_region *io,
		unsigned long offset,
		const void *restrict src,
		memory_order order,
		int len)
{
	void *va = metal_io_virt(io, offset);

	(void)order;
	memcpy(va, src, len);
	metal_cache_flush(va, (unsigned int)len);
	return len;
}

static void hil_shm_block_set(struct metal_io_region *io,
		unsigned long offset,
		unsigned char value,
		memory_order order,
		int len)
{
	void *va = metal_io_virt(io, offset);

	(void)order;
	memset(va, (int)value, len);
	metal_cache_flush(va, (unsigned int)len);
}

static struct metal_io_region hil_shm_generic_io = {
	0,
	&hil_generic_start_paddr,
	(size_t)(-1),
	(sizeof(metal_phys_addr_t) << 3),
	(metal_phys_addr_t)(-1),
	0,
	{NULL, NULL,
	 NULL, hil_shm_block_write, hil_shm_block_set, NULL},
};

struct metal_device *hil_create_generic_mem_dev(
		metal_phys_addr_t pa,
		size_t size, unsigned int flags)
{
	struct hil_mem_device *dev;
	struct metal_device *mdev;
	int ret;

	/* If no generic bus is found in libmetal
	 * there is no need to create the generic device
	 */
	ret = metal_bus_find("generic", NULL);
	if (ret)
		return NULL;
	dev = metal_allocate_memory(sizeof(*dev));
	metal_assert(dev);
	memset(dev, 0, sizeof(*dev));
	sprintf(dev->name, "%s%lx.%lx", HIL_DEV_NAME_PREFIX, pa,
			(unsigned long)size);
	dev->pa = pa;
	mdev = &dev->device;
	mdev->name = dev->name;
	mdev->num_regions = 1;
	metal_io_init(&mdev->regions[0], (void *)pa, &dev->pa, size,
			sizeof(pa) << 3, flags, NULL);

	ret = metal_register_generic_device(mdev);
	metal_assert(!ret);
	ret = metal_device_open("generic", dev->name, &mdev);
	metal_assert(!ret);

	return mdev;
}

void hil_close_generic_mem_dev(struct metal_device *dev)
{
	struct hil_mem_device *mdev;

	if (strncmp(HIL_DEV_NAME_PREFIX, dev->name,
		strlen(HIL_DEV_NAME_PREFIX))) {
		metal_device_close(dev);
	} else {
		metal_list_del(&dev->node);
		mdev = metal_container_of(dev, struct hil_mem_device, device);
		metal_free_memory(mdev);
	}
}

static struct metal_io_region *hil_get_mem_io(
	struct metal_device *dev,
	metal_phys_addr_t pa,
	size_t size)
{
	struct metal_io_region *io;
	unsigned int i;

	for (i = 0; i < dev->num_regions; i++) {
		io = &dev->regions[i];
		if (!pa && io->size >= size)
			return io;
		if (metal_io_phys_to_offset(io, pa) == METAL_BAD_OFFSET)
			continue;
		if (metal_io_phys_to_offset(io, (pa + size)) ==
					METAL_BAD_OFFSET)
			continue;
		return io;
	}

	return NULL;
}

struct hil_proc *hil_create_proc(struct hil_platform_ops *ops,
			unsigned long cpu_id, void *pdata)
{
	struct hil_proc *proc = 0;

	proc = metal_allocate_memory(sizeof(struct hil_proc));
	if (!proc)
		return NULL;
	memset(proc, 0, sizeof(struct hil_proc));

	proc->ops = ops;
	proc->num_chnls = 1;
	proc->cpu_id = cpu_id;
	proc->pdata = pdata;

	/* Setup generic shared memory I/O region */
	proc->sh_buff.io = &hil_shm_generic_io;

	metal_mutex_init(&proc->lock);
	metal_list_add_tail(&procs, &proc->node);

	return proc;
}

/**
 * hil_delete_proc
 *
 * This function deletes the given proc instance and frees the
 * associated resources.
 *
 * @param proc - pointer to hil remote_proc instance
 *
 */
void hil_delete_proc(struct hil_proc *proc)
{
	struct metal_list *node;
	struct metal_device *dev;
	struct metal_io_region *io;
	struct proc_vring *vring;
	int i;

	metal_list_for_each(&procs, node) {
		if (proc ==
			metal_container_of(node, struct hil_proc, node)) {
			metal_list_del(&proc->node);
			metal_mutex_acquire(&proc->lock);
			proc->ops->release(proc);
			/* Close shmem device */
			dev = proc->sh_buff.dev;
			io = proc->sh_buff.io;
			if (dev)
				proc->ops->release_shm(proc, dev, io);
			else if (io && io->ops.close)
				io->ops.close(io);

			/* Close resource table device */
			dev = proc->rsc_dev;
			io = proc->rsc_io;
			if (dev)
				proc->ops->release_shm(proc, dev, io);
			else if (io && io->ops.close)
				io->ops.close(io);

			/* Close vring device */
			for (i = 0; i < HIL_MAX_NUM_VRINGS; i++) {
				vring = &proc->vdev.vring_info[i];
				dev = vring->dev;
				io = vring->io;
				if (dev)
					proc->ops->release_shm(proc, dev, io);
				else if (io && io->ops.close)
					io->ops.close(io);
			}

			metal_mutex_release(&proc->lock);
			metal_mutex_deinit(&proc->lock);
			metal_free_memory(proc);
			return;
		}
	}
}

int hil_init_proc(struct hil_proc *proc)
{
	int ret = 0;
	if (!proc->is_initialized && proc->ops->initialize) {
		ret = proc->ops->initialize(proc);
		if (!ret)
			proc->is_initialized = 1;
		else
			return -1;
	}
	return 0;
}

/**
 * hil_get_chnl_info
 *
 * This function returns channels info for given proc.
 *
 * @param proc - pointer to proc info struct
 * @param num_chnls - pointer to integer variable to hold
 *                    number of available channels
 *
 * @return - pointer to channel info control block
 *
 */
struct proc_chnl *hil_get_chnl_info(struct hil_proc *proc, int *num_chnls)
{
	*num_chnls = proc->num_chnls;
	return (proc->chnls);
}

void hil_notified(struct hil_proc *proc, uint32_t notifyid)
{
	struct proc_vdev *pvdev = &proc->vdev;
	struct fw_rsc_vdev *vdev_rsc = pvdev->vdev_info;
	int i;
	if (vdev_rsc->status & VIRTIO_CONFIG_STATUS_NEEDS_RESET) {
		if (pvdev->rst_cb)
			pvdev->rst_cb(proc, 0);
	} else {
		for(i = 0; i < (int)pvdev->num_vrings; i++) {
			struct fw_rsc_vdev_vring *vring_rsc;
			vring_rsc = &vdev_rsc->vring[i];
			if (notifyid == (uint32_t)(-1) ||
				notifyid == vring_rsc->notifyid)
				virtqueue_notification(
					pvdev->vring_info[i].vq);
		}
	}
}

/**
 * hil_get_vdev_info
 *
 * This function return virtio device for remote core.
 *
 * @param proc - pointer to remote proc
 *
 * @return - pointer to virtio HW device.
 *
 */

struct proc_vdev *hil_get_vdev_info(struct hil_proc *proc)
{
	return (&proc->vdev);

}

/**
 * hil_get_vring_info
 *
 * This function returns vring_info_table. The caller will use
 * this table to get the vring HW info which will be subsequently
 * used to create virtqueues.
 *
 * @param vdev - pointer to virtio HW device
 * @param num_vrings - pointer to hold number of vrings
 *
 * @return - pointer to vring hardware info table
 */
struct proc_vring *hil_get_vring_info(struct proc_vdev *vdev, int *num_vrings)
{
	struct fw_rsc_vdev *vdev_rsc;
	struct fw_rsc_vdev_vring *vring_rsc;
	struct proc_vring *vring;
	int i, ret;

	vdev_rsc = vdev->vdev_info;
	*num_vrings = vdev->num_vrings;
	if (vdev_rsc) {
		vring = &vdev->vring_info[0];
		for (i = 0; i < vdev_rsc->num_of_vrings; i++) {
			struct hil_proc *proc = metal_container_of(
					vdev, struct hil_proc, vdev);
			void *vaddr = METAL_BAD_VA;

			/* Initialize vring with vring resource */
			vring_rsc = &vdev_rsc->vring[i];
			vring[i].num_descs = vring_rsc->num;
			vring[i].align = vring_rsc->align;
			/* Check if vring needs to reinitialize.
			 * Vring needs reinitialization if the vdev
			 * master restarts.
			 */
			if (vring[i].io) {
			    vaddr = metal_io_phys_to_virt(vring[i].io,
							 (metal_phys_addr_t)vring_rsc->da);
			}
			if (vaddr == (void *)METAL_BAD_VA) {
				ret = hil_set_vring(proc, i, NULL, NULL,
						    (metal_phys_addr_t)vring_rsc->da,
						    vring_size(vring_rsc->num,
						    vring_rsc->align));
				if (ret)
					return NULL;
				vaddr = metal_io_phys_to_virt(vring[i].io,
							      (metal_phys_addr_t)vring_rsc->da);
			}
			vring[i].vaddr = vaddr;
		}
	}
	return (vdev->vring_info);
}

/**
 * hil_get_shm_info
 *
 * This function returns shared memory info control block. The caller
 * will use this information to create and manage memory buffers for
 * vring descriptor table.
 *
 * @param proc - pointer to proc instance
 *
 * @return - pointer to shared memory region used for buffers
 *
 */
struct proc_shm *hil_get_shm_info(struct hil_proc *proc)
{
	return (&proc->sh_buff);
}

void hil_free_vqs(struct virtio_device *vdev)
{
	struct hil_proc *proc = vdev->device;
	struct proc_vdev *pvdev = &proc->vdev;
	int num_vrings = (int)pvdev->num_vrings;
	int i;

	metal_mutex_acquire(&proc->lock);
	for(i = 0; i < num_vrings; i++) {
		struct proc_vring *pvring = &pvdev->vring_info[i];
		struct virtqueue *vq = pvring->vq;
		if (vq) {
			virtqueue_free(vq);
			pvring->vq = 0;
		}
	}
	metal_mutex_release(&proc->lock);
}

int hil_enable_vdev_notification(struct hil_proc *proc, int id)
{
	/* We only support single vdev in hil_proc */
	(void)id;
	if (!proc)
		return -1;
	if (proc->ops->enable_interrupt)
		proc->ops->enable_interrupt(&proc->vdev.intr_info);
	return 0;
}

/**
 * hil_enable_vring_notifications()
 *
 * This function is called after successful creation of virtqueues.
 * This function saves queue handle in the vring_info_table which
 * will be used during interrupt handling .This function setups
 * interrupt handlers.
 *
 * @param vring_index - index to vring HW table
 * @param vq          - pointer to virtqueue to save in vring HW table
 *
 * @return            - execution status
 */
int hil_enable_vring_notifications(int vring_index, struct virtqueue *vq)
{
	struct hil_proc *proc_hw = (struct hil_proc *)vq->vq_dev->device;
	struct proc_vring *vring_hw = &proc_hw->vdev.vring_info[vring_index];
	/* Save virtqueue pointer for later reference */
	vring_hw->vq = vq;

	if (proc_hw->ops->enable_interrupt) {
		proc_hw->ops->enable_interrupt(&vring_hw->intr_info);
	}

	return 0;
}

/**
 * hil_vdev_notify()
 *
 * This function generates IPI to let the other side know that there is
 * update in the vritio dev configs
 *
 * @param vdev - pointer to the viritio device
 *
 */
void hil_vdev_notify(struct virtio_device *vdev)
{
	struct hil_proc *proc = vdev->device;
	struct proc_vdev *pvdev = &proc->vdev;

	if (proc->ops->notify) {
		proc->ops->notify(proc, &pvdev->intr_info);
	}
}

/**
 * hil_vring_notify()
 *
 * This function generates IPI to let the other side know that there is
 * job available for it. The required information to achieve this, like interrupt
 * vector, CPU id etc is be obtained from the proc_vring table.
 *
 * @param vq - pointer to virtqueue
 *
 */
void hil_vring_notify(struct virtqueue *vq)
{
	struct hil_proc *proc_hw = (struct hil_proc *)vq->vq_dev->device;
	struct proc_vring *vring_hw =
	    &proc_hw->vdev.vring_info[vq->vq_queue_index];

	if (proc_hw->ops->notify) {
		proc_hw->ops->notify(proc_hw, &vring_hw->intr_info);
	}
}

/**
 * hil_get_status
 *
 * This function is used to check if the given core is up and running.
 * This call will return after it is confirmed that remote core has
 * started.
 *
 * @param proc - pointer to proc instance
 *
 * @return - execution status
 */
int hil_get_status(struct hil_proc *proc)
{
	(void)proc;

	/* For future use only. */
	return 0;
}

/**
 * hil_set_status
 *
 * This function is used to update the status
 * of the given core i.e it is ready for IPC.
 *
 * @param proc - pointer to remote proc
 *
 * @return - execution status
 */
int hil_set_status(struct hil_proc *proc)
{
	(void)proc;

	/* For future use only. */
	return 0;
}

/**
 * hil_boot_cpu
 *
 * This function boots the remote processor.
 *
 * @param proc       - pointer to remote proc
 * @param start_addr - start address of remote cpu
 *
 * @return - execution status
 */
int hil_boot_cpu(struct hil_proc *proc, unsigned int start_addr)
{

	if (proc->ops->boot_cpu) {
		proc->ops->boot_cpu(proc, start_addr);
	}
#if defined (OPENAMP_BENCHMARK_ENABLE)
	boot_time_stamp = metal_get_timestamp();
#endif

	return 0;
}

/**
 * hil_shutdown_cpu
 *
 *  This function shutdowns the remote processor
 *
 * @param proc - pointer to remote proc
 *
 */
void hil_shutdown_cpu(struct hil_proc *proc)
{
	if (proc->ops->shutdown_cpu) {
		proc->ops->shutdown_cpu(proc);
	}
#if defined (OPENAMP_BENCHMARK_ENABLE)
	shutdown_time_stamp = metal_get_timestamp();
#endif
}

/**
 * hil_get_firmware
 *
 * This function returns address and size of given firmware name passed as
 * parameter.
 *
 * @param fw_name    - name of the firmware
 * @param start_addr - pointer t hold start address of firmware
 * @param size       - pointer to hold size of firmware
 *
 * returns -  status of function execution
 *
 */
int hil_get_firmware(char *fw_name, uintptr_t *start_addr,
		     unsigned int *size)
{
	return (config_get_firmware(fw_name, start_addr, size));
}

int hil_poll (struct hil_proc *proc, int nonblock)
{
	return proc->ops->poll(proc, nonblock);
}

int hil_set_shm (struct hil_proc *proc,
		 const char *bus_name, const char *name,
		 metal_phys_addr_t paddr, size_t size)
{
	struct metal_device *dev;
	struct metal_io_region *io;
	int ret;

	if (!proc)
		return -1;
	if (name && bus_name) {
		ret = metal_device_open(bus_name, name, &dev);
		if (ret)
			return ret;
		proc->sh_buff.dev = dev;
		proc->sh_buff.io = NULL;
	} else if (name) {
		ret = metal_shmem_open(name, size, &io);
		if (ret)
			return ret;
		proc->sh_buff.io = io;
	}
	if (!size) {
		if (proc->sh_buff.io) {
			io = proc->sh_buff.io;
			proc->sh_buff.start_paddr = metal_io_phys(io, 0);
			proc->sh_buff.size = io->size;
		} else if (proc->sh_buff.dev) {
			dev = proc->sh_buff.dev;
			io = &dev->regions[0];
			proc->sh_buff.io = io;
			proc->sh_buff.start_paddr = metal_io_phys(io, 0);
			proc->sh_buff.size = io->size;
		}
	} else if (!paddr) {
		if (proc->sh_buff.io) {
			io = proc->sh_buff.io;
			if (io->size != size)
				return -1;
			proc->sh_buff.start_paddr = metal_io_phys(io, 0);
			proc->sh_buff.size = io->size;
		} else if (proc->sh_buff.dev) {
			dev = proc->sh_buff.dev;
			io = &dev->regions[0];
			proc->sh_buff.io = io;
			proc->sh_buff.start_paddr = metal_io_phys(io, 0);
			proc->sh_buff.size = size;
		}
	} else {
		if (proc->sh_buff.io) {
			io = proc->sh_buff.io;
			if (size > io->size)
				return -1;
			if (metal_io_phys_to_offset(io, paddr) ==
					METAL_BAD_OFFSET)
				return -1;
			proc->sh_buff.start_paddr = paddr;
			proc->sh_buff.size = size;
		} else if (proc->sh_buff.dev) {
			dev = proc->sh_buff.dev;
			io = hil_get_mem_io(dev, paddr, size);
			if (!io)
				return -1;
			proc->sh_buff.io = io;
			proc->sh_buff.start_paddr = metal_io_phys(io, 0);
			proc->sh_buff.size = size;
		} else {
			io = proc->ops->alloc_shm(proc, paddr, size, &dev);
			metal_assert(io);
			proc->sh_buff.dev = dev;
			proc->sh_buff.io = io;
			proc->sh_buff.start_paddr = paddr;
			proc->sh_buff.size = size;
		}
	}
	proc->sh_buff.start_addr = metal_io_phys_to_virt(proc->sh_buff.io,
						proc->sh_buff.start_paddr);
	return 0;
}

int hil_set_rsc (struct hil_proc *proc,
		const char *bus_name, const char *name,
		metal_phys_addr_t paddr, size_t size)
{
	struct metal_device *dev;
	struct metal_io_region *io;
	int ret;

	if (!proc)
		return -1;

	if (name && bus_name) {
		ret = metal_device_open(bus_name, name, &dev);
		if (ret)
			return ret;
		proc->rsc_dev = dev;
		io = hil_get_mem_io(dev, 0, size);
		if (!io)
			return -1;
		proc->rsc_io = io;
	} else if (name) {
		ret = metal_shmem_open(name, size, &io);
		if (ret)
			return ret;
		proc->rsc_io = io;
	} else {
		if (proc->rsc_dev || proc->rsc_io)
			return 0;
		io = proc->ops->alloc_shm(proc, paddr, size, &dev);
		if (dev) {
			proc->rsc_dev = dev;
			proc->rsc_io = io;
		}
	}

	return 0;
}

int hil_set_vring (struct hil_proc *proc, int index,
		 const char *bus_name, const char *name,
		metal_phys_addr_t paddr, size_t size)
{
	struct metal_device *dev;
	struct metal_io_region *io;
	struct proc_vring *vring;
	int ret;

	if (!proc)
		return -1;
	if (index >= HIL_MAX_NUM_VRINGS)
		return -1;
	vring = &proc->vdev.vring_info[index];
	if (name && bus_name) {
		ret = metal_device_open(bus_name, name, &dev);
		if (ret)
			return ret;
		vring->dev = dev;
	} else if (name) {
		ret = metal_shmem_open(name, size, &io);
		if (ret)
			return ret;
		vring->io = io;
	} else {
		if (vring->dev) {
			dev = vring->dev;
			io = hil_get_mem_io(dev, paddr, size);
			if (io) {
				vring->io = io;
				return 0;
			}
			proc->ops->release_shm(proc, dev, NULL);
		} else if (vring->io) {
			io = vring->io;
			if (size <= io->size &&
				metal_io_phys_to_offset(io, paddr) !=
					METAL_BAD_OFFSET)
				return 0;
		}
		io = proc->ops->alloc_shm(proc, paddr, size, &dev);
		if (!io)
			return -1;
		vring->io = io;;
	}

	return 0;
}

int hil_set_vdev_ipi (struct hil_proc *proc, int index,
		 unsigned int irq, void *data)
{
	struct proc_intr *vring_intr;

	/* As we support only one vdev for now */
	(void)index;

	if (!proc)
		return -1;
	vring_intr = &proc->vdev.intr_info;
	vring_intr->vect_id = irq;
	vring_intr->data = data;
	return 0;
}

int hil_set_vring_ipi (struct hil_proc *proc, int index,
		 unsigned int irq, void *data)
{
	struct proc_intr *vring_intr;

	if (!proc)
		return -1;
	vring_intr = &proc->vdev.vring_info[index].intr_info;
	vring_intr->vect_id = irq;
	vring_intr->data = data;
	return 0;
}

int hil_set_rpmsg_channel (struct hil_proc *proc, int index,
			   char *name)
{
	if (!proc)
		return -1;
	if (index >= HIL_MAX_NUM_CHANNELS)
		return -1;
	strcpy(proc->chnls[index].name, name);
	return 0;
}

int hil_set_vdev_rst_cb (struct hil_proc *proc, int index,
		hil_proc_vdev_rst_cb_t cb)
{
	(void)index;
	proc->vdev.rst_cb = cb;
	return 0;
}

