/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2020 acontis technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/device_mmio.h>
#include <drivers/pcie/pcie.h>

#ifdef CONFIG_ACPI
#include <arch/x86/acpi.h>
#endif

#ifdef CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
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


#define PCIE_MM_CONF_write(phys_addr, data) (*phys_addr = data)
#define PCIE_MM_CONF_read(phys_addr, data) (*data = *phys_addr)

#define PCIE_MM_CONF(rw, size, type, ptr_type)                                 \
static inline void pcie_mm_conf_##rw##_##size                                  \
	(pcie_bdf_t bdf, unsigned int addr, type data)                         \
{                                                                              \
	for (int i = 0; i < ARRAY_SIZE(bus_segs); i++) {                       \
		int off = PCIE_BDF_TO_BUS(bdf) - bus_segs[i].start_bus;        \
\
		if (off >= 0 && off < bus_segs[i].n_buses) {                   \
			bdf = PCIE_BDF(off,                                    \
			    PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));      \
\
			volatile ptr_type phys_addr =                          \
				(void *)(bus_segs[i].mmio + (bdf << 4) + addr);\
\
			PCIE_MM_CONF_##rw(phys_addr, data);                    \
		}                                                              \
	}                                                                      \
}

PCIE_MM_CONF(read, 8, uint8_t *, uint8_t *)
PCIE_MM_CONF(read, 16, uint16_t *, uint16_t *)
PCIE_MM_CONF(read, 32, uint32_t *, uint32_t *)

PCIE_MM_CONF(write, 8, uint8_t, uint8_t *)
PCIE_MM_CONF(write, 16, uint16_t, uint16_t *)
PCIE_MM_CONF(write, 32, uint32_t, uint32_t *)

#endif /* CONFIG_PCIE_MMIO_CFG */

/* Traditional Configuration Mechanism */

#define PCIE_X86_CAP	0xCF8U	/* Configuration Address Port */
#define PCIE_X86_CAP_BDF_MASK	0x00FFFF00U  /* b/d/f bits */
#define PCIE_X86_CAP_EN		0x80000000U  /* enable bit */
#define PCIE_X86_CAP_WORD_MASK	0xFCU  /*  6-bit word index .. */

#define PCIE_X86_CDP	0xCFCU	/* Configuration Data Port */

/*
 * Helper function for exported configuration functions. Configuration access
 * ain't atomic, so spinlock to keep drivers from clobbering each other.
 */
static struct k_spinlock pcie_io_conf_spinlock;

#define PCIE_IO_CONF_write(size, data) sys_out##size(data, PCIE_X86_CDP)
#define PCIE_IO_CONF_read(size, data) (*data = sys_in##size(PCIE_X86_CDP))

#define PCIE_IO_CONF(rw, size, type)                   \
static inline void pcie_io_conf_##rw##_##size          \
	(pcie_bdf_t bdf, unsigned int addr, type data) \
{                                                      \
	k_spinlock_key_t k;                            \
\
	bdf &= PCIE_X86_CAP_BDF_MASK;                  \
	bdf |= PCIE_X86_CAP_EN;                        \
	bdf |= (addr & PCIE_X86_CAP_WORD_MASK);        \
\
	k = k_spin_lock(&pcie_io_conf_spinlock);       \
	sys_out32(bdf, PCIE_X86_CAP);                  \
\
	PCIE_IO_CONF_##rw(size, data);                 \
\
	sys_out32(0U, PCIE_X86_CAP);                   \
	k_spin_unlock(&pcie_io_conf_spinlock, k);      \
}

PCIE_IO_CONF(read, 8, uint8_t *)
PCIE_IO_CONF(read, 16, uint16_t *)
PCIE_IO_CONF(read, 32, uint32_t *)

PCIE_IO_CONF(write, 8, uint8_t)
PCIE_IO_CONF(write, 16, uint16_t)
PCIE_IO_CONF(write, 32, uint32_t)

/* these functions are explained in include/drivers/pcie/pcie.h */

#ifdef CONFIG_PCIE_MMIO_CFG
#define IF_CONFIG_PCIE_MMIO_CFG(...) __VA_ARGS__
#else
#define IF_CONFIG_PCIE_MMIO_CFG(...)
#endif

#define PCIE_CONF_VALIDATE_write(size, data) 0
#define PCIE_CONF_VALIDATE_read(size, data) \
	((*data == (uint##size##_t)(~0x0)) ? -ENXIO : 0)

#define PCIE_CONF(rw, size, type)                                         \
int pcie_conf_##rw##_##size(pcie_bdf_t bdf, unsigned int addr, type data) \
{                                                                         \
	/* validate alignment */                                          \
	if (addr & ((size / 8) - 1)) {                                    \
		return -EINVAL;                                           \
	}                                                                 \
IF_CONFIG_PCIE_MMIO_CFG(                                                  \
	if (bus_segs[0].mmio == NULL) {                                   \
		pcie_mm_init();                                           \
	}                                                                 \
	if (do_pcie_mmio_cfg) {                                           \
		pcie_mm_conf_##rw##_##size(bdf, addr, data);              \
	} else                                                            \
)                                                                         \
	{                                                                 \
		pcie_io_conf_##rw##_##size(bdf, addr, data);              \
	}                                                                 \
	return PCIE_CONF_VALIDATE_##rw(size, data);                       \
}

PCIE_CONF(read, 8, uint8_t *)
PCIE_CONF(read, 16, uint16_t *)
PCIE_CONF(read, 32, uint32_t *)

PCIE_CONF(write, 8, uint8_t)
PCIE_CONF(write, 16, uint16_t)
PCIE_CONF(write, 32, uint32_t)


uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	uint32_t data = 0;

	pcie_conf_read_32(bdf, reg * sizeof(uint32_t), &data);
	return data;
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	pcie_conf_write_32(bdf, reg * sizeof(uint32_t), data);
}

#ifdef CONFIG_PCIE_MSI

/* these functions are explained in include/drivers/pcie/msi.h */

uint32_t pcie_msi_map(unsigned int irq)
{
	ARG_UNUSED(irq);
	return 0xFEE00000U;  /* standard delivery to BSP local APIC */
}

uint16_t pcie_msi_mdr(unsigned int irq)
{
	unsigned char vector = Z_IRQ_TO_INTERRUPT_VECTOR(irq);

	return 0x4000U | vector;  /* edge triggered */
}

#endif
