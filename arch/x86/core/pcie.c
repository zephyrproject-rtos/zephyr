/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/drivers/pcie/pcie.h>

#ifdef CONFIG_ACPI
#include <zephyr/arch/x86/acpi.h>
#endif

#ifdef CONFIG_PCIE_MSI
#include <kernel_arch_func.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/interrupt_controller/sysapic.h>
#include <zephyr/arch/x86/cpuid.h>
#endif

/* PCI Express Extended Configuration Mechanism (MMIO) */
#ifdef CONFIG_PCIE_MMIO_CFG

#define MAX_PCI_BUS_SEGMENTS 4

static struct {
	uint32_t start_bus;
	uint32_t n_buses;
	uint8_t *mmio;
} bus_segs[MAX_PCI_BUS_SEGMENTS];

static bool do_pcie_mmio_cfg;

static void pcie_mm_init(void)
{
#ifdef CONFIG_ACPI
	struct acpi_mcfg *m = z_acpi_find_table(ACPI_MCFG_SIGNATURE);

	if (m != NULL) {
		int n = (m->sdt.length - sizeof(*m)) / sizeof(m->pci_segs[0]);

		for (int i = 0; i < n && i < MAX_PCI_BUS_SEGMENTS; i++) {
			size_t size;
			uintptr_t phys_addr;

			bus_segs[i].start_bus = m->pci_segs[i].start_bus;
			bus_segs[i].n_buses = 1 + m->pci_segs[i].end_bus
				- m->pci_segs[i].start_bus;

			phys_addr = m->pci_segs[i].base_addr;
			/* 32 devices & 8 functions per bus, 4k per device */
			size = bus_segs[i].n_buses * (32 * 8 * 4096);

			device_map((mm_reg_t *)&bus_segs[i].mmio, phys_addr,
				   size, K_MEM_CACHE_NONE);
		}

		do_pcie_mmio_cfg = true;
	}
#endif
}

static inline void pcie_mm_conf(pcie_bdf_t bdf, unsigned int reg,
				bool write, uint32_t *data)
{
	for (int i = 0; i < ARRAY_SIZE(bus_segs); i++) {
		int off = PCIE_BDF_TO_BUS(bdf) - bus_segs[i].start_bus;

		if (off >= 0 && off < bus_segs[i].n_buses) {
			bdf = PCIE_BDF(off,
				       PCIE_BDF_TO_DEV(bdf),
				       PCIE_BDF_TO_FUNC(bdf));

			volatile uint32_t *regs
				= (void *)&bus_segs[i].mmio[bdf << 4];

			if (write) {
				regs[reg] = *data;
			} else {
				*data = regs[reg];
			}
		}
	}
}

#endif /* CONFIG_PCIE_MMIO_CFG */

/* Traditional Configuration Mechanism */

#define PCIE_X86_CAP	0xCF8U	/* Configuration Address Port */
#define PCIE_X86_CAP_BDF_MASK	0x00FFFF00U  /* b/d/f bits */
#define PCIE_X86_CAP_EN		0x80000000U  /* enable bit */
#define PCIE_X86_CAP_WORD_MASK	0x3FU  /*  6-bit word index .. */
#define PCIE_X86_CAP_WORD_SHIFT	2U  /* .. is in CAP[7:2] */

#define PCIE_X86_CDP	0xCFCU	/* Configuration Data Port */

/*
 * Helper function for exported configuration functions. Configuration access
 * is not atomic, so spinlock to keep drivers from clobbering each other.
 */
static inline void pcie_io_conf(pcie_bdf_t bdf, unsigned int reg,
				bool write, uint32_t *data)
{
	static struct k_spinlock lock;
	k_spinlock_key_t k;

	bdf &= PCIE_X86_CAP_BDF_MASK;
	bdf |= PCIE_X86_CAP_EN;
	bdf |= (reg & PCIE_X86_CAP_WORD_MASK) << PCIE_X86_CAP_WORD_SHIFT;

	k = k_spin_lock(&lock);
	sys_out32(bdf, PCIE_X86_CAP);

	if (write) {
		sys_out32(*data, PCIE_X86_CDP);
	} else {
		*data = sys_in32(PCIE_X86_CDP);
	}

	sys_out32(0U, PCIE_X86_CAP);
	k_spin_unlock(&lock, k);
}

static inline void pcie_conf(pcie_bdf_t bdf, unsigned int reg,
			     bool write, uint32_t *data)

{
#ifdef CONFIG_PCIE_MMIO_CFG
	if (bus_segs[0].mmio == NULL) {
		pcie_mm_init();
	}

	if (do_pcie_mmio_cfg) {
		pcie_mm_conf(bdf, reg, write, data);
	} else
#endif
	{
		pcie_io_conf(bdf, reg, write, data);
	}
}

/* these functions are explained in include/drivers/pcie/pcie.h */

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	uint32_t data = 0U;

	pcie_conf(bdf, reg, false, &data);
	return data;
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	pcie_conf(bdf, reg, true, &data);
}

#ifdef CONFIG_PCIE_MSI

#ifdef CONFIG_INTEL_VTD_ICTL

#include <zephyr/drivers/interrupt_controller/intel_vtd.h>
#include <zephyr/arch/x86/acpi.h>

static const struct device *vtd = DEVICE_DT_GET_ONE(intel_vt_d);

#endif /* CONFIG_INTEL_VTD_ICTL */

/* these functions are explained in include/drivers/pcie/msi.h */

#define MSI_MAP_DESTINATION_ID_SHIFT 12
#define MSI_RH BIT(3)

uint32_t pcie_msi_map(unsigned int irq,
		      msi_vector_t *vector,
		      uint8_t n_vector)
{
	uint32_t dest_id;

	ARG_UNUSED(irq);

#if defined(CONFIG_INTEL_VTD_ICTL)
	if (vector != NULL && n_vector > 0) {
		return vtd_remap_msi(vtd, vector, n_vector);
	}
#endif

	dest_id = z_x86_cpuid_get_current_physical_apic_id() <<
		MSI_MAP_DESTINATION_ID_SHIFT;

	/* Directing to current physical CPU (may not be BSP)
	 * Destination ID - RH 1 - DM 0
	 */
	return 0xFEE00000U | dest_id | MSI_RH;
}

uint16_t pcie_msi_mdr(unsigned int irq,
		      msi_vector_t *vector)
{
	if (vector != NULL) {
		if (IS_ENABLED(CONFIG_INTEL_VTD_ICTL)) {
			return 0;
		}

#if defined(CONFIG_PCIE_MSI_X)
		if (vector->msix) {
			return 0x4000U | vector->arch.vector;
		}
#endif
	}

	return 0x4000U | Z_IRQ_TO_INTERRUPT_VECTOR(irq);
}

#if defined(CONFIG_INTEL_VTD_ICTL) || defined(CONFIG_PCIE_MSI_X)

uint8_t arch_pcie_msi_vectors_allocate(unsigned int priority,
				       msi_vector_t *vectors,
				       uint8_t n_vector)
{
	int prev_vector = -1;
	int i, irq, vector;

	if (vectors == NULL || n_vector == 0) {
		return 0;
	}


#ifdef CONFIG_INTEL_VTD_ICTL
	{
		int irte;

		if (!device_is_ready(vtd)) {
			return 0;
		}

		irte = vtd_allocate_entries(vtd, n_vector);
		if (irte < 0) {
			return 0;
		}

		for (i = 0; i < n_vector; i++, irte++) {
			vectors[i].arch.irte = irte;
			vectors[i].arch.remap = true;
		}
	}
#endif /* CONFIG_INTEL_VTD_ICTL */

	for (i = 0; i < n_vector; i++) {
		if (n_vector == 1) {
			/* This path is taken by PCIE device with fixed
			 * or single MSI: IRQ has been already allocated
			 * and/or set on the PCIe bus. Thus we only require
			 * to get it.
			 */
			irq = pcie_get_irq(vectors->bdf);
		} else {
			irq = arch_irq_allocate();
		}

		if ((irq == PCIE_CONF_INTR_IRQ_NONE) || (irq == -1)) {
			return -1;
		}

		vector = z_x86_allocate_vector(priority, prev_vector);
		if (vector < 0) {
			return 0;
		}

		vectors[i].arch.irq = irq;
		vectors[i].arch.vector = vector;

#ifdef CONFIG_INTEL_VTD_ICTL
		vtd_set_irte_vector(vtd, vectors[i].arch.irte,
				    vectors[i].arch.vector);
		vtd_set_irte_irq(vtd, vectors[i].arch.irte,
				 vectors[i].arch.irq);
		vtd_set_irte_msi(vtd, vectors[i].arch.irte, true);
#endif
		prev_vector = vectors[i].arch.vector;
	}

	return n_vector;
}

bool arch_pcie_msi_vector_connect(msi_vector_t *vector,
				  void (*routine)(const void *parameter),
				  const void *parameter,
				  uint32_t flags)
{
#ifdef CONFIG_INTEL_VTD_ICTL
	if (vector->arch.remap) {
		union acpi_dmar_id id;

		if (!device_is_ready(vtd)) {
			return false;
		}

		id.bits.bus = PCIE_BDF_TO_BUS(vector->bdf);
		id.bits.device = PCIE_BDF_TO_DEV(vector->bdf);
		id.bits.function = PCIE_BDF_TO_FUNC(vector->bdf);

		vtd_remap(vtd, vector->arch.irte, vector->arch.vector,
			  flags, id.raw);
	}
#endif /* CONFIG_INTEL_VTD_ICTL */

	z_x86_irq_connect_on_vector(vector->arch.irq, vector->arch.vector,
				    routine, parameter);

	return true;
}

#endif /* CONFIG_INTEL_VTD_ICTL || CONFIG_PCIE_MSI_X */
#endif /* CONFIG_PCIE_MSI */
