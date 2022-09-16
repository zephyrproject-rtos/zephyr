/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>

#include <zephyr/drivers/pcie/pcie.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PCIE_CONTROLLER
struct msi_vector_generic {
	unsigned int irq;
	uint32_t address;
	uint16_t eventid;
	unsigned int priority;
};

typedef struct msi_vector_generic arch_msi_vector_t;

#define PCI_DEVID(bus, dev, fn)	((((bus) & 0xff) << 8) | (((dev) & 0x1f) << 3) | ((fn) & 0x07))
#define PCI_BDF_TO_DEVID(bdf)	PCI_DEVID(PCIE_BDF_TO_BUS(bdf), \
					  PCIE_BDF_TO_DEV(bdf),	\
					  PCIE_BDF_TO_FUNC(bdf))

#endif

struct msix_vector {
	uint32_t msg_addr;
	uint32_t msg_up_addr;
	uint32_t msg_data;
	uint32_t vector_ctrl;
};

struct msi_vector {
	pcie_bdf_t bdf;
	arch_msi_vector_t arch;
#ifdef CONFIG_PCIE_MSI_X
	struct msix_vector *msix_vector;
	bool msix;
#endif /* CONFIG_PCIE_MSI_X */
};

typedef struct msi_vector msi_vector_t;

#ifdef CONFIG_PCIE_MSI_MULTI_VECTOR

/**
 * @brief Allocate vector(s) for the endpoint MSI message(s)
 *
 * @param bdf the target PCI endpoint
 * @param priority the MSI vectors base interrupt priority
 * @param vectors an array for storing allocated MSI vectors
 * @param n_vector the size of the MSI vectors array
 *
 * @return the number of allocated MSI vectors.
 */
extern uint8_t pcie_msi_vectors_allocate(pcie_bdf_t bdf,
					 unsigned int priority,
					 msi_vector_t *vectors,
					 uint8_t n_vector);

/**
 * @brief Connect the MSI vector to the handler
 *
 * @param bdf the target PCI endpoint
 * @param vector the MSI vector to connect
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flags Arch-specific IRQ configuration flag
 *
 * @return True on success, false otherwise
 */
extern bool pcie_msi_vector_connect(pcie_bdf_t bdf,
				    msi_vector_t *vector,
				    void (*routine)(const void *parameter),
				    const void *parameter,
				    uint32_t flags);

#endif /* CONFIG_PCIE_MSI_MULTI_VECTOR */

/**
 * @brief Compute the target address for an MSI posted write.
 *
 * This function is exported by the arch, board or SoC code.
 *
 * @param irq The IRQ we wish to trigger via MSI.
 * @param vector The vector for which you want the address (or NULL)
 * @param n_vector the size of the vector array
 * @return A (32-bit) value for the MSI MAP register.
 */
extern uint32_t pcie_msi_map(unsigned int irq,
			     msi_vector_t *vector,
			     uint8_t n_vector);

/**
 * @brief Compute the data for an MSI posted write.
 *
 * This function is exported by the arch, board or SoC code.
 *
 * @param irq The IRQ we wish to trigger via MSI.
 * @param vector The vector for which you want the data (or NULL)
 * @return A (16-bit) value for MSI MDR register.
 */
extern uint16_t pcie_msi_mdr(unsigned int irq,
			     msi_vector_t *vector);

/**
 * @brief Configure the given PCI endpoint to generate MSIs.
 *
 * @param bdf the target PCI endpoint
 * @param vectors an array of allocated vector(s)
 * @param n_vector the size of the vector array
 * @param irq The IRQ we wish to trigger via MSI.
 * @return true if the endpoint supports MSI, false otherwise.
 */
extern bool pcie_msi_enable(pcie_bdf_t bdf,
			    msi_vector_t *vectors,
			    uint8_t n_vector,
			    unsigned int irq);

/**
 * @brief Check if the given PCI endpoint supports MSI/MSI-X
 *
 * @param bdf the target PCI endpoint
 * @return true if the endpoint support MSI/MSI-X
 */
extern bool pcie_is_msi(pcie_bdf_t bdf);

/*
 * The first word of the MSI capability is shared with the
 * capability ID and list link.  The high 16 bits are the MCR.
 */

#define PCIE_MSI_MCR		0U

#define PCIE_MSI_MCR_EN		0x00010000U  /* enable MSI */
#define PCIE_MSI_MCR_MMC	0x000E0000U  /* Multi Messages Capable mask */
#define PCIE_MSI_MCR_MMC_SHIFT	17
#define PCIE_MSI_MCR_MME	0x00700000U  /* mask of # of enabled IRQs */
#define PCIE_MSI_MCR_MME_SHIFT	20
#define PCIE_MSI_MCR_64		0x00800000U  /* 64-bit MSI */

/*
 * The MAP follows the MCR. If PCIE_MSI_MCR_64, then the MAP
 * is two words long. The MDR follows immediately after the MAP.
 */

#define PCIE_MSI_MAP0		1U
#define PCIE_MSI_MAP1_64	2U
#define PCIE_MSI_MDR_32		2U
#define PCIE_MSI_MDR_64		3U

/*
 * As for MSI, he first word of the MSI-X capability is shared
 * with the capability ID and list link.  The high 16 bits are the MCR.
 */

#define PCIE_MSIX_MCR			0U

#define PCIE_MSIX_MCR_EN		0x80000000U /* Enable MSI-X */
#define PCIE_MSIX_MCR_FMASK		0x40000000U /* Function Mask */
#define PCIE_MSIX_MCR_TSIZE		0x07FF0000U /* Table size mask */
#define PCIE_MSIX_MCR_TSIZE_SHIFT	16
#define PCIE_MSIR_TABLE_ENTRY_SIZE	16

#define PCIE_MSIX_TR			1U
#define PCIE_MSIX_TR_BIR		0x00000007U /* Table BIR mask */
#define PCIE_MSIX_TR_OFFSET		0xFFFFFFF8U /* Offset mask */

#define PCIE_MSIX_PBA			2U
#define PCIE_MSIX_PBA_BIR		0x00000007U /* PBA BIR mask */
#define PCIE_MSIX_PBA_OFFSET		0xFFFFFFF8U /* Offset mask */

#define PCIE_VTBL_MA			0U /* Msg Address offset */
#define PCIE_VTBL_MUA			4U /* Msg Upper Address offset */
#define PCIE_VTBL_MD			8U /* Msg Data offset */
#define PCIE_VTBL_VCTRL			12U /* Vector control offset */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_ */
