/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
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

#include <string.h>
#include <errno.h>
#include <openamp/hil.h>
#include <metal/alloc.h>
#include <metal/irq.h>
#include <metal/atomic.h>

/* ------------------------- Macros --------------------------*/
#define SCUGIC_PERIPH_BASE      	   0xF8F00000
#define SCUGIC_DIST_BASE        	   (SCUGIC_PERIPH_BASE + 0x00001000)
#define ESAL_DP_SLCR_BASE                  0xF8000000
#define GIC_DIST_SOFTINT                   0xF00
#define GIC_SFI_TRIG_CPU_MASK              0x00FF0000
#define GIC_SFI_TRIG_SATT_MASK             0x00008000
#define GIC_SFI_TRIG_INTID_MASK            0x0000000F
#define GIC_CPU_ID_BASE                    (1 << 4)
#define A9_CPU_SLCR_RESET_CTRL             0x244
#define A9_CPU_SLCR_CLK_STOP               (1 << 4)
#define A9_CPU_SLCR_RST                    (1 << 0)

#define unlock_slcr()                       HIL_MEM_WRITE32(ESAL_DP_SLCR_BASE + 0x08, 0xDF0DDF0D)
#define lock_slcr()                         HIL_MEM_WRITE32(ESAL_DP_SLCR_BASE + 0x04, 0x767B767B)

/* L2Cpl310 L2 cache controller base address. */
#define         HIL_PL130_BASE              0xF8F02000

/********************/
/* Register offsets */
/********************/

#define         HIL_PL130_INVALLINE         0x770
#define         HIL_PL130_CLEANINVLINE      0x7F0


#define         HIL_PA_SBZ_MASK             ~(HIL_CACHE_LINE_SIZE - 1UL)
#define         HIL_CACHE_LINE_SIZE         32
#define         HIL_CACHE_INV_ALL_WAYS      0xFF
#define         HIL_CACHE_UNLOCK_ALL_WAYS   0xFFFF0000
#define         HIL_CACHE_CLEAR_INT         0x1FF

/* Memory attributes */
#define NORM_NONCACHE 0x11DE2 	/* Normal Non-cacheable */
#define STRONG_ORDERED 0xC02	/* Strongly ordered */
#define DEVICE_MEMORY 0xC06	/* Device memory */
#define RESERVED 0x0		/* reserved memory */

#define HIL_DEV_NAME_PREFIX "hil-dev."

#define _rproc_wait() asm volatile("wfi")

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
struct hil_platform_ops zynq_a9_proc_ops = {
	.enable_interrupt = _enable_interrupt,
	.notify           = _notify,
	.boot_cpu         = _boot_cpu,
	.shutdown_cpu     = _shutdown_cpu,
	.poll             = _poll,
	.alloc_shm = _alloc_shm,
	.release_shm = _release_shm,
	.initialize = _initialize,
	.release = _release,
};

struct hil_mem_device {
	struct metal_device device;
	char name[64];
	metal_phys_addr_t pa;
};

static metal_phys_addr_t git_dist_base_addr = SCUGIC_DIST_BASE;
static struct metal_io_region gic_dist_io = {
	(void *)SCUGIC_DIST_BASE,
	&git_dist_base_addr,
	0x1000,
	(sizeof(metal_phys_addr_t) << 3),
	(metal_phys_addr_t)(-1),
	0,
	{NULL},
};

//volatile unsigned int ipi_counter = 0;
//volatile unsigned int enableirq_counter = 0;

int _ipi_handler(int vect_id, void *data)
{
	struct proc_intr *intr_info = data;

	(void) vect_id;

	atomic_flag_clear((atomic_uint *)&(intr_info->data));

	//ipi_counter++;
	return 0;
}

static int _enable_interrupt(struct proc_intr *intr)
{
	//enableirq_counter++;
	/* Register ISR */
	metal_irq_register(intr->vect_id, _ipi_handler,
			intr->dev, intr);

	/* Enable the interrupts */
	metal_irq_enable(intr->vect_id);

	/* FIXME: This is a workaround for Zynq. As Linux is possible
	 * to have already generate the soft IRQ
	 */
	atomic_flag_clear((atomic_uint *)&(intr->data));
	return 0;
}

static void _notify(struct hil_proc *proc, struct proc_intr *intr_info)
{
	unsigned long mask = 0;

	mask = ((1 << (GIC_CPU_ID_BASE + proc->cpu_id)) | (intr_info->vect_id))
	    & (GIC_SFI_TRIG_CPU_MASK | GIC_SFI_TRIG_INTID_MASK);

	/* Trigger IPI */
	metal_io_write32(&gic_dist_io, GIC_DIST_SOFTINT, mask);
}

static int _poll(struct hil_proc *proc, int nonblock)
{
	struct proc_vring *vring;
	unsigned int flags;
	struct proc_intr *intr_info;
	int i = 0;
	int kicked = 0;

	while(1) {
		vring = &proc->vdev.vring_info[i];
		intr_info = &(vring->intr_info);
		flags = metal_irq_save_disable();
		if (!(atomic_flag_test_and_set(
			(atomic_uint *)&(intr_info->data)))) {
			metal_irq_restore_enable(flags);
			virtqueue_notification(vring->vq);
			kicked = 1;
			if (i)
				return 0;
			i++;
		} else if (!i) {
			metal_irq_restore_enable(flags);
			i++;
		} else {
			if (kicked) {
				metal_irq_restore_enable(flags);
				return 0;
			} else if (nonblock) {
				metal_irq_restore_enable(flags);
				return -EAGAIN;
			} else {
				_rproc_wait();
				metal_irq_restore_enable(flags);
				i--;
				continue;
			}
		}
	}
}

extern char zynq_trampoline;
extern char zynq_trampoline_jump;
extern char zynq_trampoline_end;

static int _boot_cpu(struct hil_proc *proc, unsigned int load_addr)
{
	/* FIXME: Will need to add the boot_cpu implementation back */
#if 0
	unsigned int reg;
	unsigned int tramp_size;
	unsigned int tramp_addr = 0;

	if (load_addr) {
		tramp_size = zynq_trampoline_end - zynq_trampoline;
		if ((load_addr < tramp_size) || (load_addr & 0x3)) {
			return -1;
		}

		tramp_size = &zynq_trampoline_jump - &zynq_trampoline;

		/*
		 * Trampoline code is copied to address 0 from where remote core is expected to
		 * fetch first instruction after reset.If master is using the address 0 then
		 * this mem copy will screwed the system. It is user responsibility to not
		 * copy trampoline code in such cases.
		 *
		 */
		memcpy((char *)tramp_addr, &zynq_trampoline, tramp_size);
		/* Write image address at the word reserved at the trampoline end */
		HIL_MEM_WRITE32((char *)(tramp_addr + tramp_size), load_addr);
	}

	unlock_slcr();

	reg = HIL_MEM_READ32(ESAL_DP_SLCR_BASE + A9_CPU_SLCR_RESET_CTRL);
	reg &= ~(A9_CPU_SLCR_CLK_STOP << cpu_id);
	HIL_MEM_WRITE32(ESAL_DP_SLCR_BASE + A9_CPU_SLCR_RESET_CTRL, reg);
	/* De-assert reset signal and start clock to start the core */
	reg &= ~(A9_CPU_SLCR_RST << cpu_id);
	HIL_MEM_WRITE32(ESAL_DP_SLCR_BASE + A9_CPU_SLCR_RESET_CTRL, reg);

	lock_slcr();

#else
	(void)proc;
	(void)load_addr;
#endif
	return 0;
}

static void _shutdown_cpu(struct hil_proc *proc)
{
	/* FIXME: Will need to add the shutdown CPU implementation back */
#if 0
	unsigned int reg;

	unlock_slcr();

	reg = HIL_MEM_READ32(ESAL_DP_SLCR_BASE + A9_CPU_SLCR_RESET_CTRL);
	/* Assert reset signal and stop clock to halt the core */
	reg |= (A9_CPU_SLCR_CLK_STOP | A9_CPU_SLCR_RST) << cpu_id;
	HIL_MEM_WRITE32(ESAL_DP_SLCR_BASE + A9_CPU_SLCR_RESET_CTRL, reg);

	lock_slcr();
#else
	(void)proc;
#endif
}

static struct metal_io_region* _alloc_shm(struct hil_proc *proc,
			metal_phys_addr_t pa,
			size_t size,
			struct metal_device **dev)
{
	(void)proc;

	*dev = hil_create_generic_mem_dev(pa, size,
				NORM_NONCACHE | STRONG_ORDERED);
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
	int i;
	struct proc_intr *intr_info;

	for (i = 0; i < 2; i++) {
		intr_info = &(proc->vdev.vring_info[i].intr_info);
		atomic_store((atomic_uint *)&(intr_info->data), 1);
	}

	return 0;
}

static void _release(struct hil_proc *proc)
{
	(void)proc;
	return;
}

