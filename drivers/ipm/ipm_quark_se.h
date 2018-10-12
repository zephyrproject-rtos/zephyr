/* ipm_quark_se.h - Quark SE mailbox driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_IPM_IPM_QUARK_SE_H_
#define ZEPHYR_DRIVERS_IPM_IPM_QUARK_SE_H_

#include <kernel.h>
#include <board.h> /* for SCSS_REGISTER_BASE */
#include <ipm.h>
#include <device.h>
#include <init.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUARK_SE_IPM_OUTBOUND		0
#define QUARK_SE_IPM_INBOUND		1

#if defined(CONFIG_SOC_QUARK_SE_C1000)
/* First byte of the QUARK_SE_IPM_MASK register is for the Lakemont */
#define QUARK_SE_IPM_MASK_START_BIT		0
#define QUARK_SE_IPM_INTERRUPT			21
#define QUARK_SE_IPM_ARC_LMT_DIR		QUARK_SE_IPM_INBOUND
#define QUARK_SE_IPM_LMT_ARC_DIR		QUARK_SE_IPM_OUTBOUND

#elif defined(CONFIG_SOC_QUARK_SE_C1000_SS)
/* Second byte is for ARC */
#define QUARK_SE_IPM_MASK_START_BIT		8
#define QUARK_SE_IPM_INTERRUPT			57
#define QUARK_SE_IPM_ARC_LMT_DIR		QUARK_SE_IPM_OUTBOUND
#define QUARK_SE_IPM_LMT_ARC_DIR		QUARK_SE_IPM_INBOUND

#else
#error "Unsupported platform for ipm_quark_se driver"
#endif

#define QUARK_SE_IPM_CHANNELS	8
#define QUARK_SE_IPM_DATA_REGS	4
#define QUARK_SE_IPM_MAX_ID_VAL	0x7FFFFFFF

/* QUARK_SE EAS section 28.5.1.123 */
#define QUARK_SE_IPM_CTRL_CTRL_MASK	BIT_MASK(31)
#define QUARK_SE_IPM_CTRL_IRQ_BIT	BIT(31)

#define QUARK_SE_IPM_STS_STS_BIT	BIT(0)
#define QUARK_SE_IPM_STS_IRQ_BIT	BIT(1)

struct __packed quark_se_ipm {
	u32_t ctrl;
	u32_t data[QUARK_SE_IPM_DATA_REGS]; /* contiguous 32-bit registers */
	u32_t sts;
};

/* Base address for mailboxes
 *
 * Layout:
 *
 * quark_se_ipm[8]
 * QUARK_SE_IPM_CHALL_STS
 */
#define QUARK_SE_IPM_BASE		(SCSS_REGISTER_BASE + 0xa00)

/* 28.5.1.73 Host processor Interrupt routing mask 21
 *
 * Bits		Description
 * 31:24	Mailbox SS Halt interrupt maskIUL
 * 23:16	Mailbox Host Halt interrupt mask
 * 15:8		Mailbox SS interrupt mask
 * 7:0		Mailbox Host interrupt mask
 */
#define QUARK_SE_IPM_MASK		(SCSS_REGISTER_BASE + 0x4a0)

/* All status bits of the mailboxes
 *
 * Bits		Description
 * 31:16	Reserved
 * 15:0		CHn_STS bits (sts/irq) for all channels
 */
#define QUARK_SE_IPM_CHALL_STS	(SCSS_REGISTER_BASE + 0x0AC0)

#define QUARK_SE_IPM(channel)	((volatile struct quark_se_ipm *)(QUARK_SE_IPM_BASE + \
					((channel) * sizeof(struct quark_se_ipm))))

struct quark_se_ipm_controller_config_info {
	int (*controller_init)(void);
};

struct quark_se_ipm_config_info {
	int channel;
	int direction;
	volatile struct quark_se_ipm *ipm;
};


struct quark_se_ipm_driver_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

const struct ipm_driver_api ipm_quark_se_api_funcs;

void quark_se_ipm_isr(void *param);

int quark_se_ipm_initialize(struct device *d);
int quark_se_ipm_controller_initialize(struct device *d);

#define QUARK_SE_IPM_DEFINE(name, ch, dir) \
	struct quark_se_ipm_config_info quark_se_ipm_config_##name = { \
		.ipm = QUARK_SE_IPM(ch), \
		.channel = ch, \
		.direction = dir \
	}; \
	struct quark_se_ipm_driver_data quark_se_ipm_runtime_##name; \
	DEVICE_AND_API_INIT(name, _STRINGIFY(name), quark_se_ipm_initialize, \
			    &quark_se_ipm_runtime_##name, \
			    &quark_se_ipm_config_##name, \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
			    &ipm_quark_se_api_funcs)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_IPM_IPM_QUARK_SE_H_ */
