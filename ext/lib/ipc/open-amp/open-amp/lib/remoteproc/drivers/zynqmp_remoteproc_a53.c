/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       platform.c
 *
 * DESCRIPTION
 *
 *       This file is the Implementation of IPC hardware layer interface
 *       for Xilinx Zynq ZC702EVK platform.
 *
 **************************************************************************/

#include <errno.h>
#include <string.h>
#include <metal/io.h>
#include <metal/device.h>
#include <metal/utilities.h>
#include <metal/atomic.h>
#include <metal/irq.h>
#include <metal/alloc.h>
#include <openamp/hil.h>
#include <openamp/virtqueue.h>

/* IPI REGs OFFSET */
#define IPI_TRIG_OFFSET          0x00000000    /* IPI trigger register offset */
#define IPI_OBS_OFFSET           0x00000004    /* IPI observation register offset */
#define IPI_ISR_OFFSET           0x00000010    /* IPI interrupt status register offset */
#define IPI_IMR_OFFSET           0x00000014    /* IPI interrupt mask register offset */
#define IPI_IER_OFFSET           0x00000018    /* IPI interrupt enable register offset */
#define IPI_IDR_OFFSET           0x0000001C    /* IPI interrupt disable register offset */

/* memory attributes */
#define DEVICE_SHARED		0x00000001U /*device, shareable*/
#define DEVICE_NONSHARED	0x00000010U /*device, non shareable*/
#define NORM_NSHARED_NCACHE	0x00000008U /* Non cacheable  non shareable */
#define NORM_SHARED_NCACHE	0x0000000CU /* Non cacheable shareable */
#define	PRIV_RW_USER_RW		(0x00000003U<<8U) /*Full Access*/

#define _rproc_wait() asm volatile("wfi")

/* -- FIX ME: ipi info is to be defined -- */
struct ipi_info {
	const char *name;
	const char *bus_name;
	struct metal_device *dev;
	struct metal_io_region *io;
	metal_phys_addr_t paddr;
	uint32_t ipi_chn_mask;
	int registered;
	atomic_int sync;
};

/*--------------------------- Declare Functions ------------------------ */
static int _enable_interrupt(struct proc_intr *intr);
static void _notify(struct hil_proc *proc, struct proc_intr *intr_info);
static int _boot_cpu(struct hil_proc *proc, unsigned int load_addr);
static void _shutdown_cpu(struct hil_proc *proc);
static int _poll(struct hil_proc *proc, int nonblock);
static int _initialize(struct hil_proc *proc);
static void _release(struct hil_proc *proc);

static int _ipi_handler(int vect_id, void *data);
static struct metal_io_region* _alloc_shm(struct hil_proc *proc,
			metal_phys_addr_t pa,
			size_t size,
			struct metal_device **dev);
static void _release_shm(struct hil_proc *proc,
			struct metal_device *dev,
			struct metal_io_region *io);

/*--------------------------- Globals ---------------------------------- */
struct hil_platform_ops zynqmp_r5_a53_proc_ops = {
	.enable_interrupt     = _enable_interrupt,
	.notify               = _notify,
	.boot_cpu             = _boot_cpu,
	.shutdown_cpu         = _shutdown_cpu,
	.poll                 = _poll,
	.alloc_shm = _alloc_shm,
	.release_shm = _release_shm,
	.initialize    = _initialize,
	.release    = _release,
};

int _ipi_handler(int vect_id, void *data)
{
	struct proc_intr *intr = data;
	struct ipi_info *ipi = intr->data;
	struct metal_io_region *io = ipi->io;
	unsigned int ipi_intr_status =
	    (unsigned int)metal_io_read32(io, IPI_ISR_OFFSET);

	(void) vect_id;

	if (ipi_intr_status & ipi->ipi_chn_mask) {
		atomic_flag_clear(&ipi->sync);
		metal_io_write32(io, IPI_ISR_OFFSET,
				ipi->ipi_chn_mask);
		return 0;
	}
	return -1;
}

static int _enable_interrupt(struct proc_intr *intr)
{
	struct ipi_info *ipi =
	    (struct ipi_info *)(intr->data);
	struct metal_io_region *io = ipi->io;

	if (ipi->registered) {
		return 0;
	}

	/* Register ISR */
	metal_irq_register(intr->vect_id, _ipi_handler,
				intr->dev, intr);
	/* Enable IPI interrupt */
	metal_irq_enable(intr->vect_id);
	metal_io_write32(io, IPI_IER_OFFSET, ipi->ipi_chn_mask);
	ipi->registered = 1;

	return 0;
}

static void _notify(struct hil_proc *proc, struct proc_intr *intr_info)
{

	(void)proc;
	struct ipi_info *ipi = (struct ipi_info *)(intr_info->data);
	if (ipi == NULL)
		return;

	/* Trigger IPI */
	metal_io_write32(ipi->io, IPI_TRIG_OFFSET, ipi->ipi_chn_mask);
}

static int _boot_cpu(struct hil_proc *proc, unsigned int load_addr)
{
	(void)proc;
	(void)load_addr;
	return -1;
}

static void _shutdown_cpu(struct hil_proc *proc)
{
	(void)proc;
	return;
}

static int _poll(struct hil_proc *proc, int nonblock)
{
	struct proc_vdev *vdev;
	struct ipi_info *ipi;
	unsigned int flags;

	vdev = &proc->vdev;
	ipi = (struct ipi_info *)(vdev->intr_info.data);
	while(1) {
		flags = metal_irq_save_disable();
		if (!(atomic_flag_test_and_set(&ipi->sync))) {
			metal_irq_restore_enable(flags);
			hil_notified(proc, (uint32_t)(-1));
			return 0;
		}
		if (nonblock) {
			metal_irq_restore_enable(flags);
			return -EAGAIN;
		}
		_rproc_wait();
		metal_irq_restore_enable(flags);
	}
}

static struct metal_io_region* _alloc_shm(struct hil_proc *proc,
			metal_phys_addr_t pa,
			size_t size,
			struct metal_device **dev)
{
	(void)proc;

	*dev = hil_create_generic_mem_dev(pa, size,
				NORM_SHARED_NCACHE | PRIV_RW_USER_RW);
	if ((*dev))
		return &((*dev)->regions[0]);

	return NULL;

}

static void _release_shm(struct hil_proc *proc,
			struct metal_device *dev,
			struct metal_io_region *io)
{
	(void)proc;
	(void)io;
	hil_close_generic_mem_dev(dev);
}

static int _initialize(struct hil_proc *proc)
{
	int ret;
	struct proc_intr *intr_info;
	struct ipi_info *ipi;

	if (!proc)
		return -1;

	intr_info = &(proc->vdev.intr_info);
	ipi = intr_info->data;

	if (ipi && ipi->name && ipi->bus_name) {
		ret = metal_device_open(ipi->bus_name, ipi->name,
					     &ipi->dev);
		if (ret)
			return -ENODEV;
		ipi->io = metal_device_io_region(ipi->dev, 0);
	} else if (ipi->paddr) {
		ipi->io = metal_allocate_memory(
			sizeof(struct metal_io_region));
		if (!ipi->io)
			goto error;
		metal_io_init(ipi->io, (void *)ipi->paddr,
			&ipi->paddr, 0x1000,
			sizeof(metal_phys_addr_t) << 3,
			0,
			NULL);
	}

	if (ipi->io) {
		metal_io_write32(ipi->io, IPI_IDR_OFFSET,
			ipi->ipi_chn_mask);
		atomic_store(&ipi->sync, 1);
	}
	ipi->registered = 0;
	return 0;

error:
	_release(proc);
	return -1;
}

static void _release(struct hil_proc *proc)
{
	struct proc_intr *intr_info;
	struct ipi_info *ipi;

	if (!proc)
		return;
	intr_info = &(proc->vdev.intr_info);
	ipi = (struct ipi_info *)(intr_info->data);
	if (ipi) {
		if (ipi->io) {
			metal_io_write32(ipi->io, IPI_IDR_OFFSET,
				ipi->ipi_chn_mask);
			if (ipi->dev) {
				metal_device_close(ipi->dev);
				ipi->dev = NULL;
			} else {
				metal_free_memory(ipi->io);
			}
			ipi->io = NULL;
		}
	}

}

