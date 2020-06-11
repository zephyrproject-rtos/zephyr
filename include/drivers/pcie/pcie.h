/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_

#include <dt-bindings/pcie/pcie.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef pcie_bdf_t
 * @brief A unique PCI(e) endpoint (bus, device, function).
 *
 * A PCI(e) endpoint is uniquely identified topologically using a
 * (bus, device, function) tuple. The internal structure is documented
 * in include/dt-bindings/pcie/pcie.h: see PCIE_BDF() and friends, since
 * these tuples are referenced from devicetree.
 */
typedef uint32_t pcie_bdf_t;

/**
 * @typedef pcie_id_t
 * @brief A unique PCI(e) identifier (vendor ID, device ID).
 *
 * The PCIE_CONF_ID register for each endpoint is a (vendor ID, device ID)
 * pair, which is meant to tell the system what the PCI(e) endpoint is. Again,
 * look to PCIE_ID_* macros in include/dt-bindings/pcie/pcie.h for more.
 */
typedef uint32_t pcie_id_t;

/*
 * These functions are arch-, board-, or SoC-specific.
 */

/**
 * @brief Read a 32-bit word from an endpoint's configuration space.
 *
 * This function is exported by the arch/SoC/board code.
 *
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @return the word read (0xFFFFFFFFU if nonexistent endpoint or word)
 */
extern uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg);

/**
 * @brief Write a 32-bit word to an endpoint's configuration space.
 *
 * This function is exported by the arch/SoC/board code.
 *
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @param data the value to write
 */
extern void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data);

/**
 * @brief Probe for the presence of a PCI(e) endpoint.
 *
 * @param bdf the endpoint to probe
 * @param id the endpoint ID to expect, or PCI_ID_ANY for "any device"
 * @return true if the device is present, false otherwise
 */
extern bool pcie_probe(pcie_bdf_t bdf, pcie_id_t id);

/**
 * @brief Get the nth MMIO address assigned to an endpoint.
 * @param bdf the PCI(e) endpoint
 * @param index (0-based) index
 * @return the address, or PCI_CONF_BAR_NONE if nonexistent.
 *
 * A PCI(e) endpoint has 0 or more memory-mapped regions. This function
 * allows the caller to enumerate them by calling with index=0..n. If
 * PCI_CONF_BAR_NONE is returned, there are no further regions. The indices
 * are order-preserving with respect to the endpoint BARs: e.g., index 0
 * will return the lowest-numbered memory BAR on the endpoint.
 */
extern uintptr_t pcie_get_mbar(pcie_bdf_t bdf, unsigned int index);

/**
 * @brief Set or reset bits in the endpoint command/status register.
 *
 * @param bdf the PCI(e) endpoint
 * @param bits the powerset of bits of interest
 * @param on use true to set bits, false to reset them
 */
extern void pcie_set_cmd(pcie_bdf_t bdf, uint32_t bits, bool on);

/**
 * @brief Return the IRQ assigned by the firmware/board to an endpoint.
 *
 * @param bdf the PCI(e) endpoint
 * @return the IRQ number, or PCIE_CONF_INTR_IRQ_NONE if unknown.
 */
extern unsigned int pcie_wired_irq(pcie_bdf_t bdf);

/**
 * @brief Enable the PCI(e) endpoint to generate the specified IRQ.
 *
 * @param bdf the PCI(e) endpoint
 * @param irq the IRQ to generate
 *
 * If MSI is enabled and the endpoint supports it, the endpoint will
 * be configured to generate the specified IRQ via MSI. Otherwise, it
 * is assumed that the IRQ IRQ has been routed by the boot firmware
 * to the specified IRQ, and the IRQ is enabled (at the I/O APIC, or
 * wherever appropriate).
 */
extern void pcie_irq_enable(pcie_bdf_t bdf, unsigned int irq);

/*
 * Configuration word 0 aligns directly with pcie_id_t.
 */

#define PCIE_CONF_ID		0U

/*
 * Configuration word 1 contains command and status bits.
 */

#define PCIE_CONF_CMDSTAT	1U	/* command/status register */

#define PCIE_CONF_CMDSTAT_IO		0x00000001U  /* I/O access enable */
#define PCIE_CONF_CMDSTAT_MEM		0x00000002U  /* mem access enable */
#define PCIE_CONF_CMDSTAT_MASTER	0x00000004U  /* bus master enable */
#define PCIE_CONF_CMDSTAT_CAPS		0x00100000U  /* capabilities list */

/*
 * Configuration word 2 has additional function identification that
 * we only care about for debug output (PCIe shell commands).
 */

#define PCIE_CONF_CLASSREV	2U	/* class/revision register */

#define PCIE_CONF_CLASSREV_CLASS(w)	(((w) >> 24) & 0xFFU)
#define PCIE_CONF_CLASSREV_SUBCLASS(w)  (((w) >> 16) & 0xFFU)
#define PCIE_CONF_CLASSREV_PROGIF(w)	(((w) >> 8) & 0xFFU)
#define PCIE_CONF_CLASSREV_REV(w)	((w) & 0xFFU)

/*
 * The only part of configuration word 3 that is of interest to us is
 * the header type, as we use it to distinguish functional endpoints
 * from bridges (which are, for our purposes, transparent).
 */

#define PCIE_CONF_TYPE		3U

#define PCIE_CONF_TYPE_BRIDGE(w)	(((w) & 0x007F0000U) != 0U)

/*
 * Words 4-9 are BARs are I/O or memory decoders. Memory decoders may
 * be 64-bit decoders, in which case the next configuration word holds
 * the high-order bits (and is, thus, not a BAR itself).
 */

#define PCIE_CONF_BAR0		4U
#define PCIE_CONF_BAR1		5U
#define PCIE_CONF_BAR2		6U
#define PCIE_CONF_BAR3		7U
#define PCIE_CONF_BAR4		8U
#define PCIE_CONF_BAR5		9U

#define PCIE_CONF_BAR_IO(w)		(((w) & 0x00000001U) == 0x00000001U)
#define PCIE_CONF_BAR_MEM(w)		(((w) & 0x00000001U) != 0x00000001U)
#define PCIE_CONF_BAR_64(w)		(((w) & 0x00000006U) == 0x00000004U)
#define PCIE_CONF_BAR_ADDR(w)		((w) & ~0xfUL)
#define PCIE_CONF_BAR_NONE		0U

/*
 * Word 15 contains information related to interrupts.
 *
 * We're only interested in the low byte, which is [supposed to be] set by
 * the firmware to indicate which wire IRQ the device interrupt is routed to.
 */

#define PCIE_CONF_INTR		15U

#define PCIE_CONF_INTR_IRQ(w)	((w) & 0xFFU)
#define PCIE_CONF_INTR_IRQ_NONE	0xFFU  /* no interrupt routed */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_ */
