/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_V2_H_
#define ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_V2_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/pinctrl.h>

#define ESPI_XEC_V2_DEBUG	1

struct espi_isr {
	uint8_t girq_id;
	uint8_t girq_pos;
	void (*the_isr)(const struct device *dev);
};

struct espi_vw_isr {
	uint8_t signal;
	uint8_t girq_id;
	uint8_t girq_pos;
	void (*the_isr)(int girq, int bpos, void *dev);
};

struct espi_xec_irq_info {
	uint8_t gid;	/* GIRQ id [8, 26] */
	uint8_t gpos;	/* bit position in GIRQ [0, 31] */
	uint8_t anid;	/* Aggregated GIRQ NVIC number */
	uint8_t dnid;	/* Direct GIRQ NVIC number */
};

struct espi_xec_config {
	uint32_t base_addr;
	uint32_t vw_base_addr;
	uint8_t pcr_idx;
	uint8_t pcr_bitpos;
	uint8_t irq_info_size;
	uint8_t rsvd[1];
	const struct espi_xec_irq_info *irq_info_list;
	const struct pinctrl_dev_config *pcfg;
};

#define ESPI_XEC_CONFIG(dev)						\
	((struct espi_xec_config * const)(dev)->config)

struct espi_xec_data {
	sys_slist_t callbacks;
	struct k_sem tx_lock;
	struct k_sem rx_lock;
	struct k_sem flash_lock;
	uint8_t plt_rst_asserted;
	uint8_t espi_rst_asserted;
	uint8_t sx_state;
#ifdef ESPI_XEC_V2_DEBUG
	uint32_t espi_rst_count;
#endif
};

#define ESPI_XEC_DATA(dev)						\
	((struct espi_xec_data * const)(dev)->data)

struct xec_signal {
	uint8_t xec_reg_idx;
	uint8_t bit;
	uint8_t dir;
};

enum mchp_msvw_regs {
	MCHP_MSVW00,
	MCHP_MSVW01,
	MCHP_MSVW02,
	MCHP_MSVW03,
	MCHP_MSVW04,
	MCHP_MSVW05,
	MCHP_MSVW06,
	MCHP_MSVW07,
	MCHP_MSVW08,
};

enum mchp_smvw_regs {
	MCHP_SMVW00,
	MCHP_SMVW01,
	MCHP_SMVW02,
	MCHP_SMVW03,
	MCHP_SMVW04,
	MCHP_SMVW05,
	MCHP_SMVW06,
	MCHP_SMVW07,
	MCHP_SMVW08,
};

enum xec_espi_girq_idx {
	pc_girq_idx = 0,
	bm1_girq_idx,
	bm2_girq_idx,
	ltr_girq_idx,
	oob_up_girq_idx,
	oob_dn_girq_idx,
	fc_girq_idx,
	rst_girq_idx,
	vw_ch_en_girq_idx,
	max_girq_idx,
};


int xec_host_dev_init(const struct device *dev);
int xec_host_dev_connect_irqs(const struct device *dev);

int espi_xec_read_lpc_request(const struct device *dev,
			      enum lpc_peripheral_opcode op,
			      uint32_t  *data);

int espi_xec_write_lpc_request(const struct device *dev,
			       enum lpc_peripheral_opcode op,
			       uint32_t *data);

#endif /* ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_V2_H_ */
