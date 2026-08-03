/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_unicomm

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define RSTCTL_KEY_UNLOCK       0xB1000000U
#define RSTCTL_STICKY_BIT_CLEAR 0x00000002U
#define RSTCTL_ASSERT_RESET     0x00000001U

#define PWREN_ENABLE  0x00000001U
#define PWREN_DISABLE 0x00000000U
#define PWREN_KEY     0x26000000U

typedef struct {
	uint32_t RESERVED0[512];
	volatile uint32_t pwren;
	volatile uint32_t rstctl;
	volatile uint32_t clkcfg;
	uint32_t RESERVED1[2];
	volatile uint32_t stat;
	uint32_t RESERVED2[570];
	volatile uint32_t ipmode;
} UNICOMM_Regs_t;

enum IPMode {
	IPMODE_UART,
	IPMODE_SPI,
	IPMODE_I2CC,
	IPMODE_I2CT,
};

struct ti_unicomm_config {
	void *inst_base;
	bool fixed_mode;
};

struct ti_unicomm_data {
	enum IPMode ip_mode;
};

static int ti_unicomm_init(const struct device *dev)
{
	const struct ti_unicomm_config *cfg = dev->config;
	const struct ti_unicomm_data *data = dev->data;

	volatile UNICOMM_Regs_t *unicomm = (UNICOMM_Regs_t *)cfg->inst_base;

	/* Reset, set IP mode and enable power */
	unicomm->rstctl = RSTCTL_KEY_UNLOCK | RSTCTL_STICKY_BIT_CLEAR | RSTCTL_ASSERT_RESET;
	if (!cfg->fixed_mode) {
		unicomm->ipmode = data->ip_mode;
	}
	unicomm->pwren = PWREN_KEY | PWREN_ENABLE;

	return 0;
}

/*
 * Derive the IPMODE enum value at compile time from the active child node's
 * compatible string.
 */
#define TI_UNICOMM_CHILD_IPMODE_UART(node_id)                                                      \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, ti_mspm0_uart), (IPMODE_UART), ())

#define TI_UNICOMM_CHILD_IPMODE(node_id) TI_UNICOMM_CHILD_IPMODE_UART(node_id)

#define TI_UNICOMM_INIT(idx)                                                                       \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(idx) == 1,                                      \
		     "UNICOMM node should have one active child!");                                \
                                                                                                   \
	static const struct ti_unicomm_config ti_unicomm_cfg_##idx = {                             \
		.inst_base = (void *)DT_INST_REG_ADDR(idx),                                        \
		.fixed_mode = COND_CODE_1(							   \
				DT_INST_PROP(idx, unicomm_fixed_mode),			   \
				(true),								   \
				(false)) };  \
                                                                                                   \
	static struct ti_unicomm_data ti_unicomm_data_##idx = {                                    \
		.ip_mode = DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, TI_UNICOMM_CHILD_IPMODE)};       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &ti_unicomm_init, NULL, &ti_unicomm_data_##idx,                 \
			      &ti_unicomm_cfg_##idx, PRE_KERNEL_1, 0, NULL);

DT_INST_FOREACH_STATUS_OKAY(TI_UNICOMM_INIT)
