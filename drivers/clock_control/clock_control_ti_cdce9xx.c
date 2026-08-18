/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Driver for TI Multi PLL CDCE9xx clock synthesizer
 *
 * This driver always connects the Y1 to the input clock, Y2/Y3 to PLL1, Y4/Y5 to PLL2, ...
 * The option of using the output mux to select from different sources increases the complexity of
 * the driver, as the sources have to be monitored in case the rate changes.
 * The driver assigns ownership to the first output that requests a PLL. The vco frequency
 * calculation of the PLL prefers the owner, trying to provide the best possible result for it. The
 * second output can only use its pdiv to get its requested rate. Ownership is transferred if for
 * the current owner a rate == 0 is requested (switching the output off and taking it from the PLL
 * handling).
 */

#include "clock_control_ti_cdce9xx_regs.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ti_cdce9xx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(cdce9xx, CONFIG_CDCE9XX_LOG_LEVEL);

#define FIELD_REPLACE(reg, mask, value) (((reg) & ~(mask)) | FIELD_PREP(mask, value))

#define CDCE9XX_VCO_MIN_HZ 80U * 1000U * 1000U
#define CDCE9XX_VCO_MAX_HZ 230U * 1000U * 1000U

#define CDCE9XX_N_MIN 1U
#define CDCE9XX_N_MAX 4095U

#define CDCE9XX_M_MIN 1U
#define CDCE9XX_M_MAX 511U

#define CDCE9XX_PDIV1_MAX 1023U

#define CDCE9XX_PDIV_MIN 1U
#define CDCE9XX_PDIV_MAX 127U

#define CDCE9XX_SSC_CENTER_RANGE_START 8U

#define PLL_INDEX(x)    (((int)x / 2) - 1)
#define OUTPUT_INDEX(x) ((int)x - (int)CLOCK_CONTROL_TI_CDCE9XX_Y2)

/* Calculate "x * n / d". */
/* clang-format off */
#define mult_frac(x, n, d)                                                                         \
	({                                                                                         \
		__typeof__(x) x_ = (x);                                                            \
		__typeof__(n) n_ = (n);                                                            \
		__typeof__(d) d_ = (d);                                                            \
                                                                                                   \
		__typeof__(x_) q = x_ / d_;                                                        \
		__typeof__(x_) r = x_ % d_;                                                        \
		q * n_ + r * n_ / d_;                                                             \
	})
/* clang-format on */

struct cdce9xx_pll_dts_config {
	uint16_t m;
	uint16_t n;
	uint8_t reg_base;
	uint8_t ssc;
	uint8_t first_div;
	uint8_t second_div;
};

struct cdce9xx_dts_config {
	struct i2c_dt_spec bus;
	uint32_t input_freq;
	uint8_t input_clock_type;
	uint8_t xtal_load_pf;
	bool keep_y1_enabled;
	uint16_t pdiv1;
	uint32_t num_plls;
	struct cdce9xx_pll_dts_config plls_dts[];
};

struct cdce9xx_output {
	uint8_t pdiv;
	bool pll_owner;
	clock_control_subsys_t sys;
	uint32_t req_rate;
	uint32_t rate;
};

struct cdce9xx_pll_config {
	uint16_t m;
	uint16_t n;
	uint32_t vco_rate;
	uint8_t reg_base;
};

struct cdce9xx_data {
	struct k_mutex mutex;
	uint32_t y1_output_freq;
	uint16_t y1_pdiv1;
	struct cdce9xx_pll_config *pll;
	struct cdce9xx_output *output;
};

static int cdce9xx_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	uint8_t rx_buf[2];

	int rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be16(rx_buf);

	return rc;
}

static int cdce9xx_write(const struct device *dev, uint8_t addr, uint8_t reg_data)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	uint8_t tx_buf[2];

	tx_buf[0] = CDCE9XX_CMD_BYTE | addr;
	tx_buf[1] = reg_data;

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int cdce9xx_update(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	uint8_t content = 0;
	int rc = cdce9xx_read(dev, addr, &content);

	if (rc < 0) {
		LOG_ERR("Failed to read 0x%02x!", addr);
		return rc;
	}

	if (FIELD_GET(mask, content) != val) {
		content = FIELD_REPLACE(content, mask, val);
		rc = cdce9xx_write(dev, addr, content);
		if (rc < 0) {
			LOG_ERR("Failed to update reg 0x%02x with 0x%02x!", addr, content);
			return rc;
		}
	}

	return 0;
}

static void dump_current_state(const struct device *dev)
{
#if CONFIG_CDCE9XX_LOG_LEVEL >= LOG_LEVEL_DBG
	const char *y3_mux[3] = {"Pdiv1", "Pdiv2", "Pdiv3"};
	const char *y5_mux[3] = {"Pdiv2", "Pdiv4", "Pdiv5"};
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	uint8_t reg_content = 0;
	uint16_t pdiv1 = 0;

	cdce9xx_read(dev, CDCE9XX_REG_Y1_CTRL, &reg_content);
	pdiv1 = (reg_content & CDCE9XX_Y1_CTRL_PDIV1_H) << 8;
	cdce9xx_read(dev, CDCE9XX_REG_PDIV1_L, &reg_content);
	pdiv1 |= (uint16_t)reg_content;

	LOG_DBG("Input frequency: %d", cfg->input_freq);
	LOG_DBG("Pdiv1: %d", pdiv1);
	cdce9xx_read(dev, 0x14, &reg_content);
	LOG_DBG("PLL1 multiplexer: %s, %d Hz", reg_content & CDCE9XX_PLL_MUX ? "bypass" : "enabled",
		data->pll[0].vco_rate);
	LOG_DBG("Y2 multiplexer: %s", reg_content & CDCE9XX_FIRST_OUT_MUX ? "Pdiv2" : "Pdiv1");
	LOG_DBG("Y3 multiplexer: %s", y3_mux[FIELD_GET(CDCE9XX_SECOND_OUT_MUX, reg_content)]);
	cdce9xx_read(dev, 0x16, &reg_content);
	LOG_DBG("Pdiv2: %d", (uint8_t)FIELD_GET(CDCE9XX_PLL_FIRST_PDIV, reg_content));
	cdce9xx_read(dev, 0x17, &reg_content);
	LOG_DBG("Pdiv3: %d", (uint8_t)FIELD_GET(CDCE9XX_PLL_SECOND_PDIV, reg_content));

	cdce9xx_read(dev, 0x24, &reg_content);
	LOG_DBG("PLL2 multiplexer: %s, %d Hz", reg_content & CDCE9XX_PLL_MUX ? "bypass" : "enabled",
		data->pll[1].vco_rate);
	LOG_DBG("Y4 multiplexer: %s", reg_content & CDCE9XX_FIRST_OUT_MUX ? "Pdiv4" : "Pdiv2");
	LOG_DBG("Y5 multiplexer: %s", y5_mux[FIELD_GET(CDCE9XX_SECOND_OUT_MUX, reg_content)]);
	cdce9xx_read(dev, 0x26, &reg_content);
	LOG_DBG("Pdiv4: %d", (uint8_t)FIELD_GET(CDCE9XX_PLL_FIRST_PDIV, reg_content));
	cdce9xx_read(dev, 0x27, &reg_content);
	LOG_DBG("Pdiv5: %d", (uint8_t)FIELD_GET(CDCE9XX_PLL_SECOND_PDIV, reg_content));
#endif
}

static inline bool is_sys_valid(uint32_t num_plls, clock_control_subsys_t sys)
{
	return ((sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) ||
		(sys >= CLOCK_CONTROL_TI_CDCE9XX_Y2 &&
		 (uint32_t)sys < ((num_plls * 2) + (uint32_t)CLOCK_CONTROL_TI_CDCE9XX_Y2)));
}

static int set_y1_output_div(const struct device *dev, uint16_t pdiv1)
{
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	uint8_t y1_ctrl = 0;
	int rc = -EINVAL;

	if (pdiv1 <= CDCE9XX_PDIV1_MAX) {
		/* Enable Y1 and apply pdiv from dts. */
		y1_ctrl =
			FIELD_PREP(CDCE9XX_Y1_CTRL_ST0, CDCE9XX_OUTPUT_STATE_ENABLED) |
			FIELD_PREP(CDCE9XX_Y1_CTRL_ST1, CDCE9XX_OUTPUT_STATE_ENABLED) |
			FIELD_PREP(CDCE9XX_Y1_CTRL_PDIV1_H, (pdiv1 >> 8) & CDCE9XX_Y1_CTRL_PDIV1_H);
		rc = cdce9xx_write(dev, CDCE9XX_REG_Y1_CTRL, y1_ctrl);
		if (rc < 0) {
			LOG_ERR("Failed to update PLL CDCE9XX_REG_Y1_CTRL!");
			return rc;
		}
		rc = cdce9xx_write(dev, CDCE9XX_REG_PDIV1_L, pdiv1 & CDCE9XX_PDIV1_L);
		if (rc < 0) {
			LOG_ERR("Failed to update PLL CDCE9XX_REG_PDIV1_L!");
			return rc;
		}

		data->y1_pdiv1 = pdiv1;
	}

	return rc;
}

static int set_output_pdiv(const struct device *dev, struct cdce9xx_output *output, uint8_t pdiv)
{
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = 0;

	uint32_t sys = (uint32_t)output->sys;
	uint8_t reg_ofs = data->pll[PLL_INDEX(sys)].reg_base;

	if ((sys % 2) == 0) {
		/* Set Pdiv (Pdiv2, Pdiv4, ...) */
		rc = cdce9xx_update(dev, reg_ofs + CDCE9XX_REG_PLL_FIRST_PDIV_OFFSET,
				    CDCE9XX_PLL_FIRST_PDIV, pdiv);
		if (rc != 0) {
			LOG_ERR("Write CDCE9XX_REG_PLL_FIRST_PDIV_OFFSET failed: %d", rc);
		}
	} else {
		/* Set Pdiv (Pdiv3, Pdiv5, ...) */
		rc = cdce9xx_write(dev, reg_ofs + CDCE9XX_REG_PLL_SECOND_PDIV_OFFSET, pdiv);
		if (rc != 0) {
			LOG_ERR("Write CDCE9XX_REG_PLL_SECOND_PDIV_OFFSET failed: %d", rc);
		}
	}
	output->pdiv = pdiv;

	return rc;
}

static int set_output_mux(const struct device *dev, struct cdce9xx_output *output)
{
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = 0;

	uint32_t sys = (uint32_t)output->sys;
	uint8_t reg_ofs = data->pll[PLL_INDEX(sys)].reg_base;

	if ((sys % 2) == 0) {
		/* Set Mux (M2, M4,...)*/
		rc = cdce9xx_update(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET,
				    CDCE9XX_FIRST_OUT_MUX, 1);
		if (rc != 0) {
			LOG_ERR("Write CDCE9XX_REG_PLL_MUX_OFFSET failed: %d", rc);
		}
	} else {
		/* Set Mux (M3, M5,...)*/
		rc = cdce9xx_update(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET,
				    CDCE9XX_SECOND_OUT_MUX, 2);
		if (rc != 0) {
			LOG_ERR("Write CDCE9XX_REG_PLL_MUX_OFFSET failed: %d", rc);
		}
	}

	return rc;
}

static int verify_chip_id(const struct device *dev)
{
	uint8_t chip_id = 0;

	int rc = cdce9xx_read(dev, CDCE9XX_REG_ID, &chip_id);

	if (rc < 0) {
		LOG_ERR("Failed to read PLL CDCE9XX_REG_ID!");
		return rc;
	}

	uint8_t vendor_id = FIELD_GET(CDCE9XX_ID_VID, chip_id);

	if (vendor_id != CDCE9XX_VENDOR_ID) {
		LOG_ERR("Invalid vendor id: 0x%x (expected 0x%x)", vendor_id,
			(uint8_t)CDCE9XX_VENDOR_ID);
		return -EINVAL;
	}

	LOG_INF("Initializing CDCE9xx with name %s", dev->name);

	return 0;
}

static int update_input_clock_type(const struct device *dev)
{
	const struct cdce9xx_dts_config *cfg = dev->config;

	if (cfg->input_clock_type <= CDCE9XX_INCLK_LVCMOS) {
		int rc = cdce9xx_update(dev, CDCE9XX_REG_CTRL, CDCE9XX_CTRL_INCLK,
					cfg->input_clock_type);

		if (rc < 0) {
			LOG_ERR("Failed to update PLL CDCE9XX_REG_CTRL!");
			return rc;
		}
	} else {
		LOG_ERR("Invalid input-clock-type in dts: %d", cfg->input_clock_type);
		return -EINVAL;
	}

	return 0;
}

static int set_ssc(const struct device *dev)
{
	/* See ssc register description in datasheet. */
	const uint8_t ssc_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7};
	const struct cdce9xx_dts_config *cfg = dev->config;

	for (int i = 0; i < (int)cfg->num_plls; i++) {
		uint8_t ssc = FIELD_PREP(CDCE9XX_PLL_SSC_0, ssc_map[cfg->plls_dts[i].ssc]);

		ssc |= FIELD_PREP(CDCE9XX_PLL_SSC_1, ssc_map[cfg->plls_dts[i].ssc]);
		int rc = cdce9xx_write(
			dev, cfg->plls_dts[i].reg_base + CDCE9XX_REG_PLL_SSC_2_OFFSET, ssc);

		if (rc < 0) {
			LOG_ERR("Failed to set ssc in PLL %d!", i + 1);
			return rc;
		}
		uint8_t scc_center = 0;

		if (cfg->plls_dts[i].ssc >= CDCE9XX_SSC_CENTER_RANGE_START) {
			scc_center = 1;
		}

		rc = cdce9xx_update(dev,
				    cfg->plls_dts[i].reg_base + CDCE9XX_REG_PLL_FIRST_PDIV_OFFSET,
				    CDCE9XX_PLL_SSC_DC, scc_center);
		if (rc < 0) {
			LOG_ERR("Failed to set ssc-center in PLL %d!", i + 1);
			return rc;
		}
	}

	return 0;
}

static int update_xtal_load_pf(const struct device *dev)
{
	const struct cdce9xx_dts_config *cfg = dev->config;

	if (cfg->xtal_load_pf <= CDCE9XX_XCSEL_MAX) {
		uint8_t xcsel = 0;

		int rc = cdce9xx_read(dev, CDCE9XX_REG_XCSEL, &xcsel);

		if (rc < 0) {
			LOG_ERR("Failed to read PLL CDCE9XX_REG_XCSEL!");
			return rc;
		}

		if (FIELD_GET(CDCE9XX_XCSEL, xcsel) != cfg->xtal_load_pf) {
			xcsel = FIELD_REPLACE(xcsel, CDCE9XX_XCSEL, cfg->xtal_load_pf);
			rc = cdce9xx_write(dev, CDCE9XX_REG_XCSEL, xcsel);
			if (rc < 0) {
				LOG_ERR("Failed to update PLL CDCE9XX_REG_XCSEL!");
				return rc;
			}
		}
	} else {
		LOG_ERR("Invalid xtal-load-pf in dts: %d", cfg->xtal_load_pf);
		return -EINVAL;
	}

	return 0;
}

static int reset_device(const struct device *dev)
{
	int rc = 0;
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = dev->data;

	if (cfg->keep_y1_enabled == false && cfg->pdiv1 == 0) {
		uint8_t y1_ctrl = 0;

		rc = cdce9xx_read(dev, CDCE9XX_REG_Y1_CTRL, &y1_ctrl);
		if (rc < 0) {
			LOG_ERR("Failed to read CDCE9XX_REG_Y1_CTRL!");
			return rc;
		}

		/* Device power down. */
		y1_ctrl = FIELD_REPLACE(y1_ctrl, CDCE9XX_Y1_CTRL_ST0, CDCE9XX_OUTPUT_STATE_PD);
		y1_ctrl = FIELD_REPLACE(y1_ctrl, CDCE9XX_Y1_CTRL_ST1, CDCE9XX_OUTPUT_STATE_PD);
		rc = cdce9xx_write(dev, CDCE9XX_REG_Y1_CTRL, y1_ctrl);
		if (rc < 0) {
			LOG_ERR("Failed to update PLL CDCE9XX_REG_Y1_CTRL!");
			return rc;
		}
		rc = cdce9xx_write(dev, CDCE9XX_REG_PDIV1_L, 0);
		if (rc < 0) {
			LOG_ERR("Failed to write PLL CDCE9XX_REG_PDIV1_L!");
			return rc;
		}
	} else {
		rc = set_y1_output_div(dev, cfg->pdiv1);
		if (rc < 0) {
			LOG_ERR("Failed to set Y1!");
			return rc;
		}
		data->y1_output_freq = cfg->input_freq / cfg->pdiv1;
	}

	uint8_t shutdown = CDCE9XX_PLL_MUX;

	/* Disable plls. */
	for (int i = 0; i < (int)cfg->num_plls; i++) {
		rc = cdce9xx_write(dev, cfg->plls_dts[i].reg_base + CDCE9XX_REG_PLL_MUX_OFFSET,
				   shutdown);
		if (rc < 0) {
			LOG_ERR("Failed to shutdown PLL %d!", i);
			return rc;
		}
	}

	/* Disable outputs (Y2, Y3, ...). */
	for (int i = 0; i < (int)cfg->num_plls; i++) {
		set_output_pdiv(dev, &data->output[i * 2], 0);
		set_output_pdiv(dev, &data->output[i * 2 + 1], 0);
	}
	return 0;
}

static uint16_t pdiv1_calc_divider(uint32_t root_rate, uint32_t rate)
{
	uint32_t divider;

	if (rate == 0 || root_rate > CDCE9XX_VCO_MAX_HZ) {
		return 0;
	}
	if (rate >= root_rate) {
		return 1;
	}

	divider = DIV_ROUND_CLOSEST(root_rate, rate);
	if (divider > CDCE9XX_PDIV1_MAX) {
		divider = CDCE9XX_PDIV1_MAX;
	}

	return (uint16_t)divider;
}

static uint8_t pdiv_calc_divider(uint32_t vco_rate, uint32_t rate)
{
	uint32_t divider;

	if (rate == 0 || vco_rate > CDCE9XX_VCO_MAX_HZ) {
		return 0;
	}
	if (rate >= vco_rate) {
		return 1;
	}

	divider = DIV_ROUND_CLOSEST(vco_rate, rate);
	if (divider > CDCE9XX_PDIV_MAX) {
		divider = CDCE9XX_PDIV_MAX;
	}

	return (uint8_t)divider;
}

static uint32_t pll_calc_rounded_rate(uint32_t root_rate, uint32_t rate, uint16_t *m_out,
				      uint16_t *n_out)
{
	uint32_t best_rate = 0;
	uint32_t best_error = UINT32_MAX;
	uint16_t best_m = UINT16_MAX;
	uint16_t best_n = UINT16_MAX;

	for (uint16_t m = CDCE9XX_M_MIN; m <= CDCE9XX_M_MAX; m++) {
		/* Ideal (real-valued) n for this m: n = rate * m / root_rate.
		 * Since actual_rate(n) is strictly monotonic in n for a fixed m,
		 * only floor(n_ideal) and ceil(n_ideal) can possibly be optimal.
		 */
		uint64_t n_scaled = (uint64_t)rate * m;

		uint32_t n_floor = (uint32_t)(n_scaled / root_rate);

		for (uint32_t n = n_floor; n <= n_floor + 1; n++) {
			if (n < CDCE9XX_N_MIN || n > CDCE9XX_N_MAX) {
				continue;
			}

			uint64_t actual_rate = ((uint64_t)root_rate * n) / m;

			if (actual_rate < CDCE9XX_VCO_MIN_HZ || actual_rate > CDCE9XX_VCO_MAX_HZ) {
				continue;
			}

			uint32_t error = (actual_rate > rate) ? (uint32_t)(actual_rate - rate)
							      : (uint32_t)(rate - actual_rate);

			if (error < best_error) {
				best_error = error;
				best_rate = (uint32_t)actual_rate;
				best_m = m;
				best_n = (uint16_t)n;
				if (error == 0) {
					goto done;
				}
			}
		}
	}

done:
	if (m_out != NULL) {
		*m_out = best_m;
	}
	if (n_out != NULL) {
		*n_out = best_n;
	}
	return best_rate;
}

static uint32_t clk_calc_best_vco_rate(uint32_t root_rate, uint32_t rate,
				       struct cdce9xx_pll_config *pll_config)
{
	uint32_t best_rate_error = rate;
	uint16_t pdiv_min;
	uint16_t pdiv_max;
	uint16_t pdiv_best;
	uint16_t pdiv_now;

	pdiv_min = (uint16_t)max(CDCE9XX_PDIV_MIN, DIV_ROUND_UP(CDCE9XX_VCO_MIN_HZ, rate));
	pdiv_max = (uint16_t)min(CDCE9XX_PDIV_MAX, CDCE9XX_VCO_MAX_HZ / rate);

	if (pdiv_min > pdiv_max) {
		return 0; /* No can do? */
	}

	pdiv_best = pdiv_min;
	for (pdiv_now = pdiv_min; pdiv_now <= pdiv_max; ++pdiv_now) {
		uint16_t m;
		uint16_t n;
		uint32_t target_rate = rate * pdiv_now;
		uint32_t pll_rate = pll_calc_rounded_rate(root_rate, target_rate, &m, &n);
		uint32_t actual_rate;
		uint32_t rate_error;

		if (pll_rate <= 0) {
			continue;
		}
		actual_rate = pll_rate / pdiv_now;
		rate_error = actual_rate > rate ? actual_rate - rate : rate - actual_rate;

		if (rate_error < best_rate_error) {
			pdiv_best = pdiv_now;
			best_rate_error = rate_error;
			pll_config->m = m;
			pll_config->n = n;
		}

		/* It is not getting better. */
		if (rate_error == 0) {
			break;
		}
	}

	return rate * pdiv_best;
}

static uint8_t pll_calculate_parameter(uint32_t root_rate, uint32_t rate,
				       struct cdce9xx_pll_config *pll_config)
{
	uint32_t vco_rate = pll_config->vco_rate;
	uint16_t divider = pdiv_calc_divider(vco_rate, rate);

	if (vco_rate / divider != rate || pll_config->m == 0 || pll_config->n == 0) {
		vco_rate = clk_calc_best_vco_rate(root_rate, rate, pll_config);
		divider = pdiv_calc_divider(vco_rate, rate);
	}

	if (divider > 0 && vco_rate > 0) {
		LOG_DBG("vco_rate: %u, m: %u, n: %u", vco_rate, pll_config->m, pll_config->n);
		pll_config->vco_rate = vco_rate;
	} else {
		LOG_INF("Cannot provide vco_rate for requested rate: %u", rate);
		divider = 0;
		pll_config->vco_rate = 0;
		pll_config->m = 0;
		pll_config->n = 0;
	}

	return (uint8_t)divider;
}

/* calculate p = max(0, 4 - int(log2 (n/m))) */
static uint8_t pll_calc_p(uint16_t n, uint16_t m)
{
	uint8_t p;
	uint16_t r = n / m;

	if (r >= 16) {
		return 0;
	}
	p = 4;
	while (r > 1) {
		r >>= 1;
		--p;
	}
	return p;
}

/* Returns VCO range bits for VCO1_0_RANGE */
static uint8_t pll_calc_range_bits(uint32_t vco_rate, uint16_t n, uint16_t m)
{
	unsigned long rate = vco_rate;

	rate = mult_frac(vco_rate, (uint32_t)n, (uint32_t)m);
	if (rate >= 175000000) {
		return 0x3;
	}
	if (rate >= 150000000) {
		return 0x02;
	}
	if (rate >= 125000000) {
		return 0x01;
	}
	return 0x00;
}

static int disable_pll(const struct device *dev, const struct cdce9xx_pll_config *pll_config)
{
	uint8_t reg_ofs = pll_config->reg_base;

	/* Set PLL mux to bypass mode, leave the rest as is */
	return cdce9xx_update(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET, CDCE9XX_PLL_MUX, 1);
}

static int configure_pll(const struct device *dev, const struct cdce9xx_pll_config *pll_config)
{
	uint16_t n = pll_config->n;
	uint16_t m = pll_config->m;
	uint16_t r;
	uint8_t q;
	uint8_t p;
	uint16_t nn;
	uint8_t pll[4]; /* Bits are spread out over 4 byte registers */
	uint8_t reg_ofs = pll_config->reg_base;
	int rc = 0;

	if ((!m || !n) || (m == n)) {
		rc = disable_pll(dev, pll_config);
	} else {
		uint8_t pll_state = 0;

		rc = cdce9xx_read(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET, &pll_state);
		if (rc != 0) {
			return rc;
		}
		pll_state |= CDCE9XX_PLL_MUX;
		pll_state = FIELD_REPLACE(pll_state, CDCE9XX_PLL_ST1, 0);
		pll_state = FIELD_REPLACE(pll_state, CDCE9XX_PLL_ST0, 0);
		/* Disable PLL. */
		rc = cdce9xx_write(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET, pll_state);
		if (rc != 0) {
			return rc;
		}
		/* According to data sheet: */
		/* p = max(0, 4 - int(log2 (n/m))) */
		p = pll_calc_p(n, m);
		/* nn = n * 2^p */
		nn = n * BIT(p);
		/* q = int(nn/m) */
		q = nn / m;
		if ((q < 16) || (q > 63)) {
			LOG_DBG("%s invalid q=%d", __func__, q);
			return -EINVAL;
		}
		r = nn - (m * q);
		if (r > 511) {
			LOG_DBG("%s invalid r=%d", __func__, r);
			return -EINVAL;
		}
		LOG_DBG("%s n=%d m=%d p=%d q=%d r=%d", __func__, n, m, p, q, r);
		/* Encode into register bits. */
		pll[0] = n >> 4;
		pll[1] = ((n & 0x0F) << 4) | ((r >> 5) & 0x0F);
		pll[2] = ((r & 0x1F) << 3) | ((q >> 3) & 0x07);
		pll[3] = ((q & 0x07) << 5) | (p << 2) |
			 pll_calc_range_bits(pll_config->vco_rate, n, m);
		/* Write to registers. */
		for (uint32_t i = 0; i < ARRAY_SIZE(pll); ++i) {
			rc = cdce9xx_write(dev, reg_ofs + CDCE9XX_REG_PLL_0_N_11_4_OFFSET + i,
					   pll[i]);
			if (rc != 0) {
				return rc;
			}
		}
		pll_state &= ~CDCE9XX_PLL_MUX;
		pll_state = FIELD_REPLACE(pll_state, CDCE9XX_PLL_ST1, 3);
		pll_state = FIELD_REPLACE(pll_state, CDCE9XX_PLL_ST0, 3);

		/* Enable PLL. */
		rc = cdce9xx_write(dev, reg_ofs + CDCE9XX_REG_PLL_MUX_OFFSET, pll_state);
	}

	return rc;
}

static struct cdce9xx_output *get_other_output(struct cdce9xx_output *output,
					       clock_control_subsys_t sys)
{
	if ((int)sys % 2 == 0) {
		return ++output;
	} else {
		return --output;
	}
}

static uint32_t get_common_vco_rate(uint32_t root_freq, uint32_t rate, uint32_t existing_rate,
				    struct cdce9xx_pll_config *pll_config)
{
	uint64_t lcm_vco_rate = sys_lcm(rate, existing_rate);

	while (lcm_vco_rate <= CDCE9XX_VCO_MAX_HZ) {
		if (lcm_vco_rate >= CDCE9XX_VCO_MIN_HZ) {
			break;
		}

		/* Double the smaller rate to be able to scale it down with the pdiv of the output.
		 */
		if (rate < existing_rate) {
			rate *= 2;
		} else {
			existing_rate *= 2;
		}
		lcm_vco_rate = sys_lcm(rate, existing_rate);
	}

	if (lcm_vco_rate < CDCE9XX_VCO_MIN_HZ || lcm_vco_rate > CDCE9XX_VCO_MAX_HZ) {
		lcm_vco_rate = 0;
	} else {
		lcm_vco_rate = pll_calc_rounded_rate(root_freq, (uint32_t)lcm_vco_rate,
						     &pll_config->m, &pll_config->n);
		if (lcm_vco_rate == 0) {
			pll_config->m = 0;
			pll_config->n = 0;
		} else {
			LOG_INF("vco_rate %u Hz (least common multiple), configurable with m = %u "
				"and n = %u, and "
				"within vco range: (rate %u Hz, other_output->rate %u)",
				(uint32_t)lcm_vco_rate, pll_config->m, pll_config->n, rate,
				existing_rate);

			pll_config->vco_rate = (uint32_t)lcm_vco_rate;
		}
	}

	return (uint32_t)lcm_vco_rate;
}

static int configure_pll_from_dts(const struct device *dev)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = dev->data;
	bool enable_pll = false;
	int rc = 0;

	for (int i = 0; i < cfg->num_plls; i++) {
		const struct cdce9xx_pll_dts_config *pll_dts = &cfg->plls_dts[i];
		struct cdce9xx_pll_config *pll = &data->pll[i];
		struct cdce9xx_output *output = NULL;
		bool owner_set = false;

		if (pll_dts->n != 0 && pll_dts->m != 0) {
			pll->m = pll_dts->m;
			pll->n = pll_dts->n;
			pll->vco_rate = (uint32_t)((cfg->input_freq * pll_dts->n) / pll_dts->m);
			pll->reg_base = pll_dts->reg_base;
			LOG_DBG("pll%d: n %d, m %d, first_div %d, second_div %d", i + 1, pll_dts->n,
				pll_dts->m, pll_dts->first_div, pll_dts->second_div);
			rc = configure_pll(dev, pll);
			if (rc != 0) {
				return rc;
			}

			output = &data->output[2 * i];
			if (pll_dts->first_div != 0) {
				rc = set_output_pdiv(dev, output, pll_dts->first_div);
				if (rc != 0) {
					return rc;
				}
				enable_pll = true;
				output->rate = pll->vco_rate / pll_dts->first_div;
				output->req_rate = output->rate;
				output->pll_owner = true;
				owner_set = true;
			}
			rc = set_output_mux(dev, output);
			if (rc != 0) {
				return rc;
			}

			output = &data->output[2 * i + 1];
			if (pll_dts->second_div != 0) {
				rc = set_output_pdiv(dev, output, pll_dts->second_div);
				if (rc != 0) {
					return rc;
				}
				enable_pll = true;
				output->rate = pll->vco_rate / pll_dts->second_div;
				output->req_rate = output->rate;
				if (owner_set == false) {
					output->pll_owner = true;
				}
			}
			rc = set_output_mux(dev, output);
			if (rc != 0) {
				return rc;
			}
		}
	}

	if (enable_pll == true) {
		uint8_t y1_ctrl = 0;

		rc = cdce9xx_read(dev, CDCE9XX_REG_Y1_CTRL, &y1_ctrl);
		if (rc < 0) {
			return rc;
		}

		if (FIELD_GET(CDCE9XX_Y1_CTRL_ST0, y1_ctrl) == CDCE9XX_OUTPUT_STATE_PD ||
		    FIELD_GET(CDCE9XX_Y1_CTRL_ST1, y1_ctrl) == CDCE9XX_OUTPUT_STATE_PD) {
			/* Take device from power down. */
			y1_ctrl = FIELD_REPLACE(y1_ctrl, CDCE9XX_Y1_CTRL_ST0,
						CDCE9XX_OUTPUT_STATE_HIZ);
			y1_ctrl = FIELD_REPLACE(y1_ctrl, CDCE9XX_Y1_CTRL_ST1,
						CDCE9XX_OUTPUT_STATE_HIZ);
			rc = cdce9xx_write(dev, CDCE9XX_REG_Y1_CTRL, y1_ctrl);
			if (rc < 0) {
				return rc;
			}
		}
	}
	return rc;
}

static int cdce9xx_init(const struct device *dev)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = dev->data;
	int rc = 0;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->mutex);

	for (int i = 0; i < cfg->num_plls; i++) {
		data->pll[i].reg_base = cfg->plls_dts[i].reg_base;
		data->output[i * 2 + 0].sys =
			(clock_control_subsys_t)((int)CLOCK_CONTROL_TI_CDCE9XX_Y2 + (i * 2 + 0));
		data->output[i * 2 + 1].sys =
			(clock_control_subsys_t)((int)CLOCK_CONTROL_TI_CDCE9XX_Y2 + (i * 2 + 1));
	}

	rc = verify_chip_id(dev);
	if (rc != 0) {
		return rc;
	}

	rc = reset_device(dev);
	if (rc != 0) {
		return rc;
	}

	rc = update_input_clock_type(dev);
	if (rc != 0) {
		return rc;
	}

	rc = update_xtal_load_pf(dev);
	if (rc != 0) {
		return rc;
	}

	rc = set_ssc(dev);
	if (rc != 0) {
		return rc;
	}

	rc = configure_pll_from_dts(dev);

	return rc;
}

static int cdce9xx_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = -EINVAL;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) {
		if (cfg->keep_y1_enabled == false && data->y1_output_freq > 0) {
			uint16_t divider = cfg->input_freq / data->y1_output_freq;

			if (divider != 0) {
				rc = set_y1_output_div(dev, divider);
			}
		}
	} else if (is_sys_valid(cfg->num_plls, sys)) {
		struct cdce9xx_output *output = &data->output[OUTPUT_INDEX(sys)];
		struct cdce9xx_pll_config *pll = &data->pll[PLL_INDEX(sys)];
		uint8_t divider = pdiv_calc_divider(pll->vco_rate, output->rate);

		if (divider != 0) {
			rc = set_output_pdiv(dev, output, divider);
		}
	}

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int cdce9xx_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = -EINVAL;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) {
		if (cfg->keep_y1_enabled == false) {
			set_y1_output_div(dev, 0);
			rc = 0;
		}
	} else if (is_sys_valid(cfg->num_plls, sys)) {
		struct cdce9xx_output *output = &data->output[OUTPUT_INDEX(sys)];

		rc = set_output_pdiv(dev, output, 0);
	}

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int cdce9xx_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	const struct cdce9xx_dts_config *cfg = dev->config;
	int rc = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) {
		*rate = data->y1_output_freq;
	} else if (is_sys_valid(cfg->num_plls, sys)) {
		*rate = data->output[OUTPUT_INDEX(sys)].rate;
	} else {
		rc = -EINVAL;
	}

	k_mutex_unlock(&data->mutex);

	return rc;
}

enum clock_control_status cdce9xx_get_status(const struct device *dev, clock_control_subsys_t sys)
{
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	const struct cdce9xx_dts_config *cfg = dev->config;
	enum clock_control_status rc = CLOCK_CONTROL_STATUS_UNKNOWN;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) {
		rc = data->y1_pdiv1 == 0 ? CLOCK_CONTROL_STATUS_OFF : CLOCK_CONTROL_STATUS_ON;
	} else if (is_sys_valid(cfg->num_plls, sys)) {
		rc = data->output[OUTPUT_INDEX(sys)].pdiv == 0 ? CLOCK_CONTROL_STATUS_OFF
							       : CLOCK_CONTROL_STATUS_ON;
	}

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int y1_set_rate(const struct device *dev, uint32_t rate)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = 0;

	if (rate == data->y1_output_freq) {
		return -EALREADY;
	}

	uint16_t divider = pdiv1_calc_divider(cfg->input_freq, rate);

	/* Allow disabling the divider when keep_y1_enabled is not set. */
	if (divider != 0 || cfg->keep_y1_enabled == false) {
		rc = set_y1_output_div(dev, divider);
		if (rc == 0) {
			data->y1_output_freq = divider > 0 ? cfg->input_freq / divider : 0;
		}
	} else {
		rc = -EINVAL;
	}

	return rc;
}

static int set_pll_output(const struct device *dev, struct cdce9xx_output *output,
			  uint32_t vco_rate, uint8_t divider)
{
	output->rate = vco_rate / divider;
	int rc = set_output_pdiv(dev, output, divider);

	if (rc < 0) {
		return rc;
	}
	return set_output_mux(dev, output);
}

static bool verify_common_vco_rate(uint32_t lcm_vco_rate, uint32_t rate_a, uint32_t rate_b)
{
	uint8_t divider = 0;
	bool vco_rate_ok = true;

	if (lcm_vco_rate == 0) {
		vco_rate_ok = false;
	} else {
		divider = pdiv_calc_divider(lcm_vco_rate, rate_a);
		if (divider == 0) {
			vco_rate_ok = false;
		} else {
			divider = pdiv_calc_divider(lcm_vco_rate, rate_b);
			if (divider == 0) {
				vco_rate_ok = false;
			}
		}
	}

	LOG_INF("lco_freq %u Hz can%sbe divided to get %u and %u", lcm_vco_rate,
		vco_rate_ok == true ? " " : " not ", rate_a, rate_b);

	return vco_rate_ok;
}

static int pll_output_set_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t rate)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = 0;

	if (rate == data->output[OUTPUT_INDEX(sys)].req_rate) {
		return -EALREADY;
	}

	struct cdce9xx_pll_config *pll = &data->pll[PLL_INDEX(sys)];
	struct cdce9xx_output *output = &data->output[OUTPUT_INDEX(sys)];
	struct cdce9xx_output *other_output = get_other_output(output, sys);
	struct cdce9xx_pll_config pll_config = {0};
	uint8_t divider = 0;

	/* Disable output by setting rate == 0. */
	if (rate == 0) {
		rc = set_output_pdiv(dev, output, 0);
		if (rc < 0) {
			return rc;
		}
		output->pll_owner = false;
		output->rate = 0;
		output->req_rate = 0;

		if (other_output->req_rate != 0) {
			/* Transfer PLL ownership to the other output: Calculate new parameter for
			 * the pll and set the ownership flag.
			 */
			divider = pll_calculate_parameter(cfg->input_freq, other_output->req_rate,
							  &pll_config);

			if (divider == 0) {
				LOG_ERR("We should not reach this here! We have been able to "
					"calculate a divider as non-owner before, so divider "
					"should not be 0 when we own the pll.");
				/* Only return and keep the current setting. */
				return 0;
			}
			pll->vco_rate = pll_config.vco_rate;
			pll->m = pll_config.m;
			pll->n = pll_config.n;
			rc = configure_pll(dev, pll);
			if (rc < 0) {
				return rc;
			}
			other_output->rate = pll->vco_rate / divider;
			other_output->pll_owner = true;

			rc = set_pll_output(dev, other_output, pll->vco_rate, divider);
		} else {
			/* None of the outputs is active, disable the pll. */
			rc = disable_pll(dev, pll);
		}

		return rc;
	}

	divider = pll_calculate_parameter(cfg->input_freq, rate, &pll_config);
	/* Cannot generate this rate even if the PLL is exclusivly available. */
	if (divider == 0) {
		return -EINVAL;
	}

	output->req_rate = rate;

	/* PLL is not in use or used only by current output. */
	if (pll->vco_rate == 0 || other_output->rate == 0) {
		pll->vco_rate = pll_config.vco_rate;
		pll->m = pll_config.m;
		pll->n = pll_config.n;
		rc = configure_pll(dev, pll);
		if (rc < 0) {
			return rc;
		}
		output->pll_owner = true;

		rc = set_pll_output(dev, output, pll->vco_rate, divider);
	} else {
		memset(&pll_config, 0, sizeof(pll_config));

		/* Try to find a common multiple vco_rate with perfect dividers for both outputs. */
		uint32_t lcm_vco_rate =
			get_common_vco_rate(cfg->input_freq, rate, other_output->rate, &pll_config);

		bool lcm_vco_rate_ok =
			verify_common_vco_rate(lcm_vco_rate, rate, other_output->rate);

		/* If that does not work, make sure the owner of the PLL gets the right vco_rate. */
		if (lcm_vco_rate_ok == false) {
			memset(&pll_config, 0, sizeof(pll_config));
			pll_config.vco_rate = pll->vco_rate;

			/* Prefer PLL owner when calculating the vco frequency. */
			if (other_output->pll_owner) {
				pll_calculate_parameter(cfg->input_freq, other_output->rate,
							&pll_config);
			} else {
				pll_calculate_parameter(cfg->input_freq, rate, &pll_config);
			}
		}

		pll->vco_rate = pll_config.vco_rate;
		pll->m = pll_config.m;
		pll->n = pll_config.n;

		/* Reconfigure pll with the calculated common vco freq. */
		rc = configure_pll(dev, pll);
		if (rc < 0) {
			return rc;
		}

		/* Configure the output for which set_request was invoked. */
		divider = pdiv_calc_divider(pll->vco_rate, rate);
		output->rate = pll->vco_rate / divider;
		rc = set_pll_output(dev, output, pll->vco_rate, divider);
		if (rc < 0) {
			return rc;
		}
		/* Reconfigure the output for the second output of this PLL. */
		divider = pdiv_calc_divider(pll->vco_rate, other_output->rate);
		other_output->rate = pll->vco_rate / divider;
		rc = set_pll_output(dev, other_output, pll->vco_rate, divider);
	}
	return rc;
}

static int cdce9xx_set_rate(const struct device *dev, clock_control_subsys_t sys,
			    clock_control_subsys_rate_t rate_param)
{
	const struct cdce9xx_dts_config *cfg = dev->config;
	struct cdce9xx_data *data = (struct cdce9xx_data *)dev->data;
	int rc = 0;
	uint32_t rate = (uint32_t)rate_param;

	if (sys == CLOCK_CONTROL_TI_CDCE9XX_ALL) {
		return -ENOTSUP;
	}

	if (!is_sys_valid(cfg->num_plls, sys) ||
	    (sys != CLOCK_CONTROL_TI_CDCE9XX_Y1 && rate > CDCE9XX_VCO_MAX_HZ)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Y1 output handling. */
	if (sys == CLOCK_CONTROL_TI_CDCE9XX_Y1) {
		rc = y1_set_rate(dev, rate);
	} else {
		rc = pll_output_set_rate(dev, sys, rate);
	}

	k_mutex_unlock(&data->mutex);

	dump_current_state(dev);

	return rc;
}

static DEVICE_API(clock_control, cdce9xx_clock_driver_api) = {
	.on = cdce9xx_on,
	.off = cdce9xx_off,
	.async_on = NULL,
	.get_rate = cdce9xx_get_rate,
	.get_status = cdce9xx_get_status,
	.set_rate = cdce9xx_set_rate,
	.configure = NULL,
};

/* clang-format off */
#define CHECK_KEEP_Y1_ENABLED(inst) \
	BUILD_ASSERT(!DT_INST_PROP(inst, keep_y1_enabled) || DT_INST_NODE_HAS_PROP(inst, pdiv1), \
		     "pdiv1 must be configured when keep-y1-enabled is set")

#define CHECK_PLL_DIVIDER_COUNT(pll_node) \
	BUILD_ASSERT(DT_NODE_HAS_PROP(pll_node, first_divider) || \
			     DT_NODE_HAS_PROP(pll_node, second_divider), \
		     "PLL requires at least one divider");

#define CHECK_PLL_NAMES(inst) \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_DRV_INST(inst)) == \
			     (DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(inst), pll1)) + \
			      DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(inst), pll2)) + \
			      DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(inst), pll3)) + \
			      DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(inst), pll4))), \
		     "Only child nodes named 'pll1', 'pll2', 'pll3', 'pll4' are allowed");

#define CHECK_INST_PLLS(inst) DT_INST_FOREACH_CHILD(inst, CHECK_PLL_DIVIDER_COUNT)

#define PLL_INIT(node_id, register_base) \
	{                                                         \
		.reg_base = register_base,                            \
		.n = DT_PROP_OR(node_id, clock_mult, 0),              \
		.m = DT_PROP_OR(node_id, clock_div, 0),               \
		.ssc = DT_ENUM_IDX_OR(node_id, ssc, 0),               \
		.first_div = DT_PROP_OR(node_id, first_divider, 0),   \
		.second_div = DT_PROP_OR(node_id, second_divider, 0), \
	}

#define PLL_OR_ZERO(inst, pll_name, register_base)                    \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, pll_name)),        \
		    (PLL_INIT(DT_INST_CHILD(inst, pll_name), register_base)), \
		    ({0}))

/* Lookup tables: number_plls (1..4) -> "does this variant have PLLn?" as a bare 0/1 token. */
#define CDCE9XX_HAS_PLL2_1 0
#define CDCE9XX_HAS_PLL2_2 1
#define CDCE9XX_HAS_PLL2_3 1
#define CDCE9XX_HAS_PLL2_4 1

#define CDCE9XX_HAS_PLL3_1 0
#define CDCE9XX_HAS_PLL3_2 0
#define CDCE9XX_HAS_PLL3_3 1
#define CDCE9XX_HAS_PLL3_4 1

#define CDCE9XX_HAS_PLL4_1 0
#define CDCE9XX_HAS_PLL4_2 0
#define CDCE9XX_HAS_PLL4_3 0
#define CDCE9XX_HAS_PLL4_4 1

/* UTIL_CAT expands its arguments (n=1/2/3/4) before pasting, so this
 * resolves to e.g. CDCE9XX_HAS_PLL3_3 -> 1 at preprocessor time.
 */
#define CDCE9XX_HAS_PLL2(n) UTIL_CAT(CDCE9XX_HAS_PLL2_, n)
#define CDCE9XX_HAS_PLL3(n) UTIL_CAT(CDCE9XX_HAS_PLL3_, n)
#define CDCE9XX_HAS_PLL4(n) UTIL_CAT(CDCE9XX_HAS_PLL4_, n)

#define CDCE9XX_DEFINE(inst, number_plls)                                       \
	CHECK_KEEP_Y1_ENABLED(inst);                                                \
	CHECK_INST_PLLS(inst);                                                      \
	CHECK_PLL_NAMES(inst);                                                      \
                                                                                \
	static const struct cdce9xx_dts_config cdce9xx_config_##inst = {            \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                      \
		.input_clock_type = DT_INST_ENUM_IDX(inst, input_clock_type),           \
		.input_freq = DT_INST_PROP(inst, input_frequency),                      \
		.xtal_load_pf = DT_INST_PROP(inst, xtal_load_pf),                       \
		.keep_y1_enabled = DT_INST_PROP(inst, keep_y1_enabled),                 \
		.pdiv1 = DT_INST_PROP_OR(inst, pdiv1, 0),                               \
		.num_plls = number_plls,                                                \
		.plls_dts = {PLL_OR_ZERO(inst, pll1, CDCE9XX_PLL1_REG_START)            \
					IF_ENABLED(CDCE9XX_HAS_PLL2(number_plls), \
				   (, PLL_OR_ZERO(inst, pll2, CDCE9XX_PLL2_REG_START))) \
					IF_ENABLED(CDCE9XX_HAS_PLL3(number_plls), \
				   (, PLL_OR_ZERO(inst, pll3, CDCE9XX_PLL3_REG_START))) \
					IF_ENABLED(CDCE9XX_HAS_PLL4(number_plls), \
				   (, PLL_OR_ZERO(inst, pll4, CDCE9XX_PLL4_REG_START))) }, \
	};  \
                                                                                \
	struct cdce9xx_pll_config pll_##inst[number_plls];                          \
	struct cdce9xx_output output_##inst[number_plls * 2];                       \
                                                           \
	static struct cdce9xx_data cdce9xx_data_##inst = {.pll = pll_##inst,  \
							  .output = output_##inst}; \
                                                                                \
	DEVICE_DT_INST_DEFINE(   \
		inst, cdce9xx_init, NULL, &cdce9xx_data_##inst, &cdce9xx_config_##inst, \
		POST_KERNEL, CONFIG_CLOCK_CONTROL_TI_CDCE9XX_PRIORITY, &cdce9xx_clock_driver_api);

/* clang-format on */

#define DT_DRV_COMPAT ti_cdce913
DT_INST_FOREACH_STATUS_OKAY_VARGS(CDCE9XX_DEFINE, 1)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_cdce925
DT_INST_FOREACH_STATUS_OKAY_VARGS(CDCE9XX_DEFINE, 2)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_cdce937
DT_INST_FOREACH_STATUS_OKAY_VARGS(CDCE9XX_DEFINE, 3)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_cdce949
DT_INST_FOREACH_STATUS_OKAY_VARGS(CDCE9XX_DEFINE, 4)
#undef DT_DRV_COMPAT
