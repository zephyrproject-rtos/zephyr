/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/drivers/misc/renesas_rx_dtc/renesas_rx_dtc.h>
#include "platform.h"

#define DT_DRV_COMPAT renesas_rx_dtc

struct dtc_renesas_rx_config {
	volatile struct st_dtc *reg;
	const struct device *clock;
	struct clock_control_rx_subsys_cfg clock_subsys;
};

struct dtc_renesas_rx_data {
	dtc_act_status_t dtc_vt_status[DTC_VECTOR_TABLE_ENTRIES];
};

static transfer_info_t *gp_dtc_vector_table[DTC_VECTOR_TABLE_ENTRIES] DTC_ALIGN_VARIABLE(1024)
	DTC_SECTION_ATTRIBUTE;

static void dtc_renesas_rx_wait_for_transfer(const struct device *dev, uint8_t activation_irq)
{
	const struct dtc_renesas_rx_config *config = dev->config;

	if (!is_valid_activation_irq(activation_irq) || !(config->reg->DTCSTS.BIT.ACT)) {
		return;
	}

	while (activation_irq == config->reg->DTCSTS.BIT.VECN) {
		/* Wait for the transfer to complete. */
		k_sleep(K_MSEC(1));
	}
}

int dtc_renesas_rx_enable_transfer(uint8_t activation_irq)
{
	transfer_info_t *p_info = gp_dtc_vector_table[activation_irq];

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	if (NULL == p_info) {
		return -EACCES;
	}

	irq_disable(activation_irq);
	ICU.DTCER[activation_irq].BIT.DTCE = 1;
	irq_enable(activation_irq);

	return 0;
}

int dtc_renesas_rx_disable_transfer(uint8_t activation_irq)
{
	transfer_info_t *p_info = gp_dtc_vector_table[activation_irq];

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	if (NULL == p_info) {
		return -EACCES;
	}

	/* Clear DTC enable bit in ICU. */
	ICU.DTCER[activation_irq].BIT.DTCE = 0;
	irq_disable(activation_irq);

	return 0;
}

void rx_dtc_block_repeat_initialize(transfer_info_t *p_info)
{
	uint32_t i = 0;

	do {
		/* Update the CRA register to the desired settings */
		if (TRANSFER_MODE_NORMAL != p_info[i].transfer_settings_word_b.mode) {
			uint8_t CRAL = p_info[i].length & DTC_PRV_MASK_CRAL;

			p_info[i].length = (uint16_t)((CRAL << DTC_PRV_OFFSET_CRAH) | CRAL);
		}
	} while (TRANSFER_CHAIN_MODE_DISABLED != p_info[i++].transfer_settings_word_b.chain_mode);
}

void dtc_renesas_rx_get_transfer_status(const struct device *dev,
					struct dtc_transfer_status *status)
{
	const struct dtc_renesas_rx_config *config = dev->config;

	if (0 == (config->reg->DTCSTS.WORD & DTC_PRV_ACT_BIT_MASK)) {
		status->in_progress = false;
	} else {
		status->in_progress = true;
		status->activation_irq = (uint8_t)(config->reg->DTCSTS.WORD & DTC_PRV_VECT_NR_MASK);
	}
}

int dtc_renesas_rx_off(const struct device *dev)
{
	const struct dtc_renesas_rx_config *config = dev->config;
	int ret = 0;

	config->reg->DTCST.BIT.DTCST = 0;
	while (config->reg->DTCSTS.BIT.ACT) {
		/* Wait for the DTC to stop. */
		__asm__ volatile("nop");
	}

	/* Disable the power for DTC module. */
	ret = clock_control_off(config->clock, (clock_control_subsys_t)&config->clock_subsys);

	return ret;
}

int dtc_renesas_rx_on(const struct device *dev)
{
	const struct dtc_renesas_rx_config *config = dev->config;
	int ret = 0;

	/* Enable the power for DTC module. */
	ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	config->reg->DTCST.BIT.DTCST = 1;

	return ret;
}

int dtc_renesas_rx_configuration(const struct device *dev, uint8_t activation_irq,
				 transfer_info_t *p_info)
{
	struct dtc_renesas_rx_data *data = dev->data;
	const struct dtc_renesas_rx_config *config = dev->config;

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	/* Re configuration */
	if (!data->dtc_vt_status[activation_irq]) {
		dtc_renesas_rx_disable_transfer(activation_irq);
		dtc_renesas_rx_wait_for_transfer(dev, activation_irq);
	}

	rx_dtc_block_repeat_initialize(p_info);

	/* Disable read skip prior to modifying settings. */
	config->reg->DTCCR.BIT.RRS = 0;

	/* Update the entry in the DTC Vector table. */
	gp_dtc_vector_table[activation_irq] = p_info;

	/* Enable read skip after all settings are written. */
	config->reg->DTCCR.BIT.RRS = 1;

	data->dtc_vt_status[activation_irq] = DTC_ACT_CONFIGURED;
	return 0;
}

int dtc_renesas_rx_start_transfer(const struct device *dev, uint8_t activation_irq)
{
	struct dtc_renesas_rx_data *data = dev->data;
	int ret;

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	if (data->dtc_vt_status[activation_irq] == DTC_ACT_IDLE) {
		return -EACCES;
	}

	ret = dtc_renesas_rx_enable_transfer(activation_irq);

	if (ret < 0) {
		return ret;
	}

	data->dtc_vt_status[activation_irq] = DTC_ACT_IN_PROGRESS;

	return 0;
}

int dtc_renesas_rx_stop_transfer(const struct device *dev, uint8_t activation_irq)
{
	struct dtc_renesas_rx_data *data = dev->data;
	int ret;

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	if (!data->dtc_vt_status[activation_irq]) {
		return -EACCES;
	}

	ret = dtc_renesas_rx_disable_transfer(activation_irq);

	if (ret < 0) {
		return ret;
	}

	/* Clear pointer in vector table. */
	data->dtc_vt_status[activation_irq] = DTC_ACT_IDLE;
	gp_dtc_vector_table[activation_irq] = NULL;
	return 0;
}

int dtc_renesas_rx_reset_transfer(const struct device *dev, uint8_t activation_irq,
				  void const *p_src, void *p_dest, uint16_t const num_transfers)
{
	const struct dtc_renesas_rx_config *config = dev->config;
	transfer_info_t *const gp_dtc_vector = gp_dtc_vector_table[activation_irq];
	int err = 0;

	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	err = dtc_renesas_rx_disable_transfer(activation_irq);
	if (err < 0) {
		return err;
	}

	dtc_renesas_rx_wait_for_transfer(dev, activation_irq);

	config->reg->DTCCR.BIT.RRS = 0;
	/* Reset transfer based on input parameters. */
	if (NULL != p_src) {
		gp_dtc_vector->p_src = p_src;
	}

	if (NULL != p_dest) {
		gp_dtc_vector->p_dest = p_dest;
	}

	if (TRANSFER_MODE_BLOCK == gp_dtc_vector->transfer_settings_word_b.mode) {
		gp_dtc_vector->num_blocks = num_transfers;
	} else if (TRANSFER_MODE_NORMAL == gp_dtc_vector->transfer_settings_word_b.mode) {
		gp_dtc_vector->length = num_transfers;
	} else {
		/* Do nothing */
	}
	config->reg->DTCCR.BIT.RRS = 1;
	dtc_renesas_rx_enable_transfer(activation_irq);

	return err;
}

int dtc_renesas_rx_info_get(const struct device *dev, uint8_t activation_irq,
			   transfer_properties_t *const p_properties)
{
	if (!is_valid_activation_irq(activation_irq)) {
		return -EINVAL;
	}

	transfer_info_t *p_info = gp_dtc_vector_table[activation_irq];

	p_properties->block_count_max = 0U;
	p_properties->block_count_remaining = 0U;

	if (TRANSFER_MODE_NORMAL != p_info->transfer_settings_word_b.mode) {
		/* Repeat and Block Mode */

		/* transfer_length_max is the same for Block and repeat mode. */
		p_properties->transfer_length_max = DTC_MAX_REPEAT_TRANSFER_LENGTH;
		p_properties->transfer_length_remaining = p_info->length & DTC_PRV_MASK_CRAL;

		if (TRANSFER_MODE_BLOCK == p_info->transfer_settings_word_b.mode) {
			p_properties->block_count_max = DTC_MAX_BLOCK_COUNT;
			p_properties->block_count_remaining = p_info->num_blocks;
		}
	} else {
		p_properties->transfer_length_max = DTC_MAX_NORMAL_TRANSFER_LENGTH;
		p_properties->transfer_length_remaining = p_info->length;
	}

	return 0;
}

static int dtc_renesas_rx_init(const struct device *dev)
{
	const struct dtc_renesas_rx_config *config = dev->config;
	int ret;

	memset(&gp_dtc_vector_table, 0, DTC_VECTOR_TABLE_ENTRIES * sizeof(transfer_info_t *));

	/* Set DTC vector table. */
	config->reg->DTCVBR = gp_dtc_vector_table;
	/* Full-address mode */
	config->reg->DTCADMOD.BIT.SHORT = 0;
	/* Turn on module DTC */
	ret = dtc_renesas_rx_on(dev);

	return ret;
}

#define DTC_DEVICE_INIT(index)                                                                     \
	static const struct dtc_renesas_rx_config p_transfer_cfg = {                               \
		.reg = (struct st_dtc *)DT_INST_REG_ADDR(index),                                   \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                                \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_INST_CLOCKS_CELL(index, mstp),                          \
				.stop_bit = DT_INST_CLOCKS_CELL(index, stop_bit),                  \
			},                                                                         \
	};                                                                                         \
	static struct dtc_renesas_rx_data p_transfer_data;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &dtc_renesas_rx_init, NULL, &p_transfer_data,                 \
			      &p_transfer_cfg, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(DTC_DEVICE_INIT);
