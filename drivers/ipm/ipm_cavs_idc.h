/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_
#define ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_

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

#endif /* ZEPHYR_DRIVERS_IPM_IPM_CAVS_IDC_H_ */
