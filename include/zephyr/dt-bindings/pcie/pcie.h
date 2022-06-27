/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PCIE_PCIE_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PCIE_PCIE_H_

/*
 * Set the device's IRQ (in devicetree, or whatever) to PCIE_IRQ_DETECT
 * if the device doesn't support MSI and we don't/can't know the wired IRQ
 * allocated by the firmware ahead of time. Use of this functionality will
 * generally also require CONFIG_DYNAMIC_INTERRUPTS.
 */

#define PCIE_IRQ_DETECT		0xFFFFFFFU

/*
 * We represent a PCI device ID as [31:16] device ID, [15:0] vendor ID. Not
 * coincidentally, this is same representation used in PCI configuration space.
 */

#define PCIE_ID_VEND_SHIFT	0U
#define PCIE_ID_VEND_MASK	0xFFFFU
#define PCIE_ID_DEV_SHIFT	16U
#define PCIE_ID_DEV_MASK		0xFFFFU

#define PCIE_ID(vend, dev) \
	((((vend) & PCIE_ID_VEND_MASK) << PCIE_ID_VEND_SHIFT) | \
	 (((dev) & PCIE_ID_DEV_MASK) << PCIE_ID_DEV_SHIFT))

#define PCIE_ID_TO_VEND(id) (((id) >> PCIE_ID_VEND_SHIFT) & PCIE_ID_VEND_MASK)
#define PCIE_ID_TO_DEV(id)  (((id) >> PCIE_ID_DEV_SHIFT) & PCIE_ID_DEV_MASK)

#define PCIE_ID_NONE PCIE_ID(0xFFFF, 0xFFFF)

#define PCIE_BDF_NONE 0xFFFFFFFFU

/*
 * Since our internal representation of bus/device/function is arbitrary,
 * we choose the same format employed in the x86 Configuration Address Port:
 *
 * [23:16] bus number, [15:11] device number, [10:8] function number
 *
 * All other bits must be zero.
 *
 * The x86 (the only arch, at present, that supports PCI) takes advantage
 * of this shared format to avoid unnecessary layers of abstraction.
 */

#define PCIE_BDF_BUS_SHIFT	16U
#define PCIE_BDF_BUS_MASK	0xFFU
#define PCIE_BDF_DEV_SHIFT	11U
#define PCIE_BDF_DEV_MASK	0x1FU
#define PCIE_BDF_FUNC_SHIFT	8U
#define PCIE_BDF_FUNC_MASK	0x7U

#define PCIE_BDF(bus, dev, func) \
	((((bus) & PCIE_BDF_BUS_MASK) << PCIE_BDF_BUS_SHIFT) | \
	 (((dev) & PCIE_BDF_DEV_MASK) << PCIE_BDF_DEV_SHIFT) | \
	 (((func) & PCIE_BDF_FUNC_MASK) << PCIE_BDF_FUNC_SHIFT))

#define PCIE_BDF_TO_BUS(bdf) (((bdf) >> PCIE_BDF_BUS_SHIFT) & PCIE_BDF_BUS_MASK)
#define PCIE_BDF_TO_DEV(bdf) (((bdf) >> PCIE_BDF_DEV_SHIFT) & PCIE_BDF_DEV_MASK)

#define PCIE_BDF_TO_FUNC(bdf) \
	(((bdf) >> PCIE_BDF_FUNC_SHIFT) & PCIE_BDF_FUNC_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PCIE_PCIE_H_ */
