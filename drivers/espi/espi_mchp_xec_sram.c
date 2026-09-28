/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi/mchp_xec_espi.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include "espi_mchp_xec_v2.h"

/* SRAM BARs map EC SRAM regions into Host memory space. The driver owns an
 * EC buffer per enabled BAR, aligned on its size as required by hardware.
 */
#define XEC_SRAM_BARS_NODE DT_CHILD(DT_NODELABEL(espi0), sram_bars)

#if DT_NODE_HAS_STATUS_OKAY(XEC_SRAM_BARS_NODE)

#define XEC_SRAM_BAR_ACCESS(node_id)                                                               \
	(DT_ENUM_IDX(node_id, access) == 0   ? MCHP_EC_SRAM_BAR_H0_ACCESS_RO                       \
	 : DT_ENUM_IDX(node_id, access) == 1 ? MCHP_EC_SRAM_BAR_H0_ACCESS_WO                       \
					     : MCHP_EC_SRAM_BAR_H0_ACCESS_RW)

#define XEC_SRAM_BAR_CHECK(node_id)                                                                \
	BUILD_ASSERT(DT_REG_ADDR(node_id) < MCHP_ESPI_SRAM_BAR_ID_MAX,                             \
		     "XEC eSPI SRAM BAR index must be 0 or 1");                                    \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_PROP(node_id, size)) &&                                    \
			     (DT_PROP(node_id, size) <= KB(32)),                                   \
		     "XEC eSPI SRAM BAR size must be a power of 2 <= 32KB");                       \
	BUILD_ASSERT((DT_PROP(node_id, host_address) % DT_PROP(node_id, size)) == 0,               \
		     "XEC eSPI SRAM BAR host-address must be aligned on size");

DT_FOREACH_CHILD_STATUS_OKAY(XEC_SRAM_BARS_NODE, XEC_SRAM_BAR_CHECK)

#define XEC_SRAM_BAR_BUF(node_id)                                                                  \
	static uint8_t xec_sram_bar_buf_##node_id[DT_PROP(node_id, size)]                         \
		__aligned(DT_PROP(node_id, size));

DT_FOREACH_CHILD_STATUS_OKAY(XEC_SRAM_BARS_NODE, XEC_SRAM_BAR_BUF)

struct xec_sram_bar_cfg {
	uint8_t *buf;
	uint32_t host_addr;
	uint32_t size;
	uint8_t id;
	uint8_t access;
};

#define XEC_SRAM_BAR_CFG(node_id)                                                                  \
	{                                                                                          \
		.buf = xec_sram_bar_buf_##node_id,                                                 \
		.host_addr = DT_PROP(node_id, host_address),                                       \
		.size = DT_PROP(node_id, size),                                                    \
		.id = DT_REG_ADDR(node_id),                                                        \
		.access = XEC_SRAM_BAR_ACCESS(node_id),                                            \
	},

static const struct xec_sram_bar_cfg xec_sram_bars[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(XEC_SRAM_BARS_NODE, XEC_SRAM_BAR_CFG)};

void xec_sram_bars_init(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_mc_sram_bar_acc_regs *const ec_regs =
		(struct xec_espi_mc_sram_bar_acc_regs *)(devcfg->mc_base_addr +
							 MCHP_ESPI_MC_BAR_SRAM_ACC_OFS);
	struct xec_espi_mc_cfg_regs *const host_regs =
		(struct xec_espi_mc_cfg_regs *)(devcfg->mc_base_addr + MCHP_ESPI_MC_CFG_OFS);

	for (size_t i = 0; i < ARRAY_SIZE(xec_sram_bars); i++) {
		const struct xec_sram_bar_cfg *sb = &xec_sram_bars[i];
		/* EC-only register layout matches the Host view register */
		volatile struct espi_sram_host_bar *ec = &ec_regs->HSRAMBAR[sb->id];
		volatile struct espi_sram_host_bar *host = &host_regs->HSRAMBAR[sb->id];
		uint32_t ec_addr = (uint32_t)sb->buf;

		ec->ACCSZ = 0; /* disable while reprogramming */

		host->HBASE_LSH = (uint16_t)(sb->host_addr & 0xffffu);
		host->HBASE_MSH = (uint16_t)(sb->host_addr >> 16);

		ec->HBASE_LSH = (uint16_t)(ec_addr & 0xffffu);
		ec->HBASE_MSH = (uint16_t)(ec_addr >> 16);
		ec->ACCSZ = (uint16_t)((u32_count_trailing_zeros(sb->size)
					<< MCHP_EC_SRAM_BAR_H0_SIZE_POS) |
				       sb->access | MCHP_EC_SRAM_BAR_H0_VALID);
	}
}

int mchp_xec_espi_sram_bar_get(const struct device *dev, uint8_t id, uint8_t **buf, size_t *size)
{
	if ((dev == NULL) || (buf == NULL) || (size == NULL)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(xec_sram_bars); i++) {
		if (xec_sram_bars[i].id == id) {
			*buf = xec_sram_bars[i].buf;
			*size = xec_sram_bars[i].size;
			return 0;
		}
	}

	return -ENODEV;
}

#else

void xec_sram_bars_init(const struct device *dev)
{
	ARG_UNUSED(dev);
}

int mchp_xec_espi_sram_bar_get(const struct device *dev, uint8_t id, uint8_t **buf, size_t *size)
{
	if ((dev == NULL) || (buf == NULL) || (size == NULL)) {
		return -EINVAL;
	}

	ARG_UNUSED(id);

	return -ENODEV;
}

#endif /* DT_NODE_HAS_STATUS_OKAY(XEC_SRAM_BARS_NODE) */
