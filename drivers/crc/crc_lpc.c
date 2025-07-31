/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 *
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_crc

#include <zephyr/drivers/crc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc.h>

LOG_MODULE_REGISTER(crc_lpc, CONFIG_CRC_LOG_LEVEL);

/* CRC Register Definitions */
typedef struct {
	__IO uint32_t MODE;      /* CRC mode register */
	__IO uint32_t SEED;      /* CRC seed register */
	union {
		__I  uint32_t SUM;   /* CRC checksum register (read) */
		__O  uint32_t WR_DATA; /* CRC data register (write) */
	};
} CRC_Type;

/* CRC Mode Register Bits */
#define CRC_MODE_CRC_POLY_MASK    0x3U
#define CRC_MODE_CRC_POLY_SHIFT   0U
#define CRC_MODE_BIT_RVS_WR_MASK  (1U << 2U)
#define CRC_MODE_CMPL_WR_MASK     (1U << 3U)
#define CRC_MODE_BIT_RVS_SUM_MASK (1U << 4U)
#define CRC_MODE_CMPL_SUM_MASK    (1U << 5U)

/* Polynomial values for MODE register */
#define CRC_POLY_CCITT  0U  /* x^16+x^12+x^5+1 */
#define CRC_POLY_CRC16  1U  /* x^16+x^15+x^2+1 */
#define CRC_POLY_CRC32  2U  /* x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1 */

struct crc_lpc_config {
	CRC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct crc_lpc_data {
	struct crc_config current_config;
	bool configured;
};

static int crc_lpc_configure(const struct device *dev,
			     const struct crc_config *config)
{
	const struct crc_lpc_config *dev_config = dev->config;
	struct crc_lpc_data *data = dev->data;
	CRC_Type *base = dev_config->base;
	uint32_t mode = 0;
	uint32_t hw_poly;

	/* Map polynomial type to hardware values */
	switch (config->type) {
	case CRC_POLY_TYPE_CRC16_CCITT:
		hw_poly = CRC_POLY_CCITT;
		break;
	case CRC_POLY_TYPE_CRC16:
		hw_poly = CRC_POLY_CRC16;
		break;
	case CRC_POLY_TYPE_CRC32:
		hw_poly = CRC_POLY_CRC32;
		break;
	default:
		LOG_ERR("Unsupported CRC polynomial type %d", config->type);
		return -ENOTSUP;
	}

	/* Configure polynomial */
	mode |= (hw_poly << CRC_MODE_CRC_POLY_SHIFT) & CRC_MODE_CRC_POLY_MASK;

	/* Configure bit reversal and complement options */
	if (config->reflect_input) {
		mode |= CRC_MODE_BIT_RVS_WR_MASK;
	}
	if (config->complement_input) {
		mode |= CRC_MODE_CMPL_WR_MASK;
	}
	if (config->reflect_output) {
		mode |= CRC_MODE_BIT_RVS_SUM_MASK;
	}
	if (config->complement_output) {
		mode |= CRC_MODE_CMPL_SUM_MASK;
	}

	/* Write configuration */
	base->MODE = mode;
	base->SEED = config->seed;

	/* Save configuration */
	data->current_config = *config;
	data->configured = true;

	LOG_DBG("CRC configured: type=%d, seed=0x%08x, mode=0x%08x",
		config->type, config->seed, mode);

	return 0;
}

static uint32_t crc_lpc_compute(const struct device *dev,
				const uint8_t *data,
				size_t len)
{
	const struct crc_lpc_config *dev_config = dev->config;
	struct crc_lpc_data *dev_data = dev->data;
	CRC_Type *base = dev_config->base;
	uint32_t result;

	if (!dev_data->configured) {
		LOG_ERR("CRC not configured");
		return 0;
	}

	/* Reset to seed value */
	base->WR_DATA = dev_data->current_config.seed;

	/* Process data */
	while (len--) {
		base->WR_DATA = *data++;
	}

	/* Get result */
	result = base->SUM;

	LOG_DBG("CRC computed: result=0x%08x", result);

	return result;
}

static int crc_lpc_append(const struct device *dev,
			  const uint8_t *data,
			  size_t len)
{
	const struct crc_lpc_config *dev_config = dev->config;
	struct crc_lpc_data *dev_data = dev->data;
	CRC_Type *base = dev_config->base;

	if (!dev_data->configured) {
		LOG_ERR("CRC not configured");
		return -EINVAL;
	}

	/* Process data */
	while (len--) {
		base->WR_DATA = *data++;
	}

	return 0;
}

static uint32_t crc_lpc_get_result(const struct device *dev)
{
	const struct crc_lpc_config *dev_config = dev->config;
	struct crc_lpc_data *dev_data = dev->data;
	CRC_Type *base = dev_config->base;

	if (!dev_data->configured) {
		LOG_ERR("CRC not configured");
		return 0;
	}

	return base->SUM;
}

static void crc_lpc_reset(const struct device *dev)
{
	const struct crc_lpc_config *dev_config = dev->config;
	struct crc_lpc_data *dev_data = dev->data;
	CRC_Type *base = dev_config->base;

	if (!dev_data->configured) {
		LOG_WRN("CRC not configured");
		return;
	}

	/* Reset to seed value */
	base->WR_DATA = dev_data->current_config.seed;
}

static int crc_lpc_init(const struct device *dev)
{
	const struct crc_lpc_config *config = dev->config;
	int ret;

	/* Enable clock */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable CRC clock");
		return ret;
	}

	LOG_DBG("CRC initialized");

	return 0;
}

static const struct crc_driver_api crc_lpc_api = {
	.configure = crc_lpc_configure,
	.compute = crc_lpc_compute,
	.append = crc_lpc_append,
	.get_result = crc_lpc_get_result,
	.reset = crc_lpc_reset,
};

#define CRC_LPC_INIT(n)							\
	static const struct crc_lpc_config crc_lpc_config_##n = {	\
		.base = (CRC_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
				DT_INST_CLOCKS_CELL(n, name),		\
	};								\
									\
	static struct crc_lpc_data crc_lpc_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      crc_lpc_init,				\
			      NULL,					\
			      &crc_lpc_data_##n,			\
			      &crc_lpc_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_CRC_INIT_PRIORITY,			\
			      &crc_lpc_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_LPC_INIT)