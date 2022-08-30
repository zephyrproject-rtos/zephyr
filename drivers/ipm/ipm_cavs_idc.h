/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_
#define ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_

/* Redeclaration of the earlier IDC register API for platforms being
 * held back on this driver.
 */
# ifndef IPC_DSP_BASE
# define IPC_DSP_BASE(core)    (DT_REG_ADDR(DT_NODELABEL(idc)) + 0x80 * (core))
# endif
#define IPC_IDCTFC(x)         (x * 0x10)
#define IPC_IDCTFC_BUSY       BIT(31)
#define IPC_IDCTFC_MSG_MASK   0x7FFFFFFF
#define IPC_IDCTEFC(x)        (0x4 + x * 0x10)
#define IPC_IDCTEFC_MSG_MASK  0x3FFFFFFF
#define IPC_IDCITC(x)         (0x8 + x * 0x10)
#define IPC_IDCITC_MSG_MASK   0x7FFFFFFF
#define IPC_IDCITC_BUSY       BIT(31)
#define IPC_IDCIETC(x)        (0xc + x * 0x10)
#define IPC_IDCIETC_MSG_MASK  0x3FFFFFFF
#define IPC_IDCIETC_DONE      BIT(30)
#define IPC_IDCCTL            0x50
#define IPC_IDCCTL_IDCTBIE(x) BIT(x)

#define IPM_CAVS_IDC_ID_MASK			\
	(CAVS_IDC_TYPE(CAVS_IDC_TYPE_MASK) |	\
	 CAVS_IDC_HEADER(CAVS_IDC_HEADER_MASK))

/* IDC message type. */
#define CAVS_IDC_TYPE_SHIFT		24U
#define CAVS_IDC_TYPE_MASK		0x7FU
#define CAVS_IDC_TYPE(x)		\
	(((x) & CAVS_IDC_TYPE_MASK) << CAVS_IDC_TYPE_SHIFT)

/* IDC message header. */
#define CAVS_IDC_HEADER_MASK		0xFFFFFFU
#define CAVS_IDC_HEADER(x)		((x) & CAVS_IDC_HEADER_MASK)

/* IDC message extension. */
#define CAVS_IDC_EXTENSION_MASK		0x3FFFFFFFU
#define CAVS_IDC_EXTENSION(x)		((x) & CAVS_IDC_EXTENSION_MASK)

/* Scheduler IPI message (type 0x7F, header 'IPI' in ascii) */
#define IPM_CAVS_IDC_MSG_SCHED_IPI_DATA		0
#define IPM_CAVS_IDC_MSG_SCHED_IPI_ID		\
	(CAVS_IDC_TYPE(0x7FU) | CAVS_IDC_HEADER(0x495049U))

static inline uint32_t idc_read(uint32_t reg, uint32_t core_id)
{
	return *((volatile uint32_t*)(IPC_DSP_BASE(core_id) + reg));
}

static inline void idc_write(uint32_t reg, uint32_t core_id, uint32_t val)
{
	*((volatile uint32_t*)(IPC_DSP_BASE(core_id) + reg)) = val;
}

int cavs_idc_smp_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_ */
