/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "battery.h"

LOG_MODULE_REGISTER(BATTERY, CONFIG_ADC_LOG_LEVEL);

struct io_channel_config {
	const char *label;
	u8_t channel;
};

struct gpio_channel_config {
	const char *label;
	u8_t pin;
	u8_t flags;
};

static const struct io_channel_config battery_channel[] = {
	DT_VOLTAGE_DIVIDER_VBATT_IO_CHANNELS
};

#define NCHANNELS ARRAY_SIZE(battery_channel)

struct divider_config {
	const struct io_channel_config *io_channel;
	const struct gpio_channel_config *power_gpios;
	const u32_t output_ohm;
	const u32_t full_ohm;
	u8_t nchannels;
};

#ifdef DT_VOLTAGE_DIVIDER_VBATT_POWER_GPIOS
static const struct gpio_channel_config divider_power_gpios =
	DT_VOLTAGE_DIVIDER_VBATT_POWER_GPIOS;
#endif

static const struct divider_config divider_config = {
	.io_channel = battery_channel,
#ifdef DT_VOLTAGE_DIVIDER_VBATT_POWER_GPIOS
	.power_gpios = &divider_power_gpios,
#endif
	.output_ohm = DT_VOLTAGE_DIVIDER_VBATT_OUTPUT_OHMS,
#ifdef DT_VOLTAGE_DIVIDER_VBATT_FULL_OHMS
	.full_ohm = DT_VOLTAGE_DIVIDER_VBATT_FULL_OHMS,
#endif
	.nchannels = NCHANNELS,
};

struct divider_data {
	struct device *adc;
	struct device *gpio;
	const struct divider_config *const cfg;
	struct adc_channel_cfg adc_cfg[NCHANNELS];
	struct adc_sequence adc_seq;
	s16_t raw[NCHANNELS];
};
static struct divider_data divider_data = {
	.cfg = &divider_config,
};

static struct divider_data *divider_setup(void)
{
	struct divider_data *ddp = &divider_data;
	struct adc_sequence *sp = &ddp->adc_seq;
	int rc;
	const struct io_channel_config *ch0p = ddp->cfg->io_channel;
	const struct gpio_channel_config *gcp = ddp->cfg->power_gpios;
	s32_t value = 0;

	ddp->adc = device_get_binding(ch0p->label);
	if (ddp->adc == NULL) {
		LOG_ERR("Failed to get ADC %s", ch0p->label);
		return NULL;
	}

	if (gcp) {
		ddp->gpio = device_get_binding(gcp->label);
		if (ddp->gpio == NULL) {
			LOG_ERR("Failed to get GPIO %s", gcp->label);
			return NULL;
		}
		rc = gpio_pin_write(ddp->gpio, gcp->pin, 0);
		if (rc == 0) {
			rc = gpio_pin_configure(ddp->gpio, gcp->pin,
						gcp->flags | GPIO_DIR_OUT);
		}
		if (rc != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcp->label, gcp->pin, rc);
			return NULL;
		}
	}

#ifdef CONFIG_ADC_NRFX_SAADC
	struct adc_channel_cfg init_cfg = {
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	};

	sp->resolution = 14;
#else /* CONFIG_ADC_var */
#error Unsupported ADC
#endif /* CONFIG_ADC_var */

	if (adc_gain_invert(init_cfg.gain, &value) != 0) {
		LOG_ERR("Unsupported gain\n");
		return NULL;
	}

	sp->buffer = ddp->raw;
	sp->buffer_size = sizeof(ddp->raw);
	sp->calibrate = true;

	for (unsigned int i = 0U; i < ddp->cfg->nchannels; ++i) {
		const struct io_channel_config *iop = ddp->cfg->io_channel + i;
		struct adc_channel_cfg *cp = ddp->adc_cfg + i;

		sp->channels |= (1U << i);
		*cp = init_cfg;
		cp->channel_id = i;
		cp->input_positive =
			SAADC_CH_PSELP_PSELP_AnalogInput0 + iop->channel;
		rc = adc_channel_setup(ddp->adc, cp);
		LOG_INF("Setup %u AIN%u got %d", i, iop->channel, rc);
	}

	return ddp;
}

/* Enable and disable power to the voltage divider. */
static int divider_power(struct divider_data *ddp,
			 bool enable)
{
	int rc = 0;

	if (ddp->gpio) {
		const struct gpio_channel_config *gcp = ddp->cfg->power_gpios;

		rc = gpio_pin_write(ddp->gpio, gcp->pin, enable);
	}
	return rc;
}

static int divider_convert(const struct divider_data *ddp,
			   u16_t *voltage_mv,
			   size_t count)
{
	u16_t *dp = voltage_mv;
	const u16_t *const dpe = dp + count;
	const struct divider_config *dcp = ddp->cfg;
	const u8_t resolution = ddp->adc_seq.resolution;
	const u16_t ref_mv = adc_ref_internal(ddp->adc);
	unsigned int ch = 0;

	while ((ch < dcp->nchannels)
	       && (dp < dpe)) {
		s32_t val = ddp->raw[ch];

		(void)adc_convert_millivolt(ref_mv, ddp->adc_cfg[ch].gain,
					    resolution, &val);

		*dp = val * (u64_t)dcp->full_ohm / dcp->output_ohm;
		LOG_DBG("raw %d => %d mV => %u mV",
			ddp->raw[ch], val, *dp);

		++ch;
		++dp;
	}
	return ch;
}

static bool battery_ok;

static int battery_setup(struct device *arg)
{
	int rc = -ENODEV;
	struct divider_data *ddp = divider_setup();

	if (ddp == &divider_data) {
		battery_ok = true;
	}
	LOG_INF("Battery setup: %d %d", rc, battery_ok);
	return rc;
}

SYS_INIT(battery_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int battery_measure_enable(bool enable)
{
	int rc = -ENOENT;

	if (battery_ok) {
		rc = divider_power(&divider_data, enable);
	}
	return rc;
}

int battery_sample(void)
{
	int rc = -ENOENT;

	if (battery_ok) {
		struct divider_data *ddp = &divider_data;
		const struct adc_sequence *sp = &ddp->adc_seq;

		rc = adc_read(ddp->adc, sp);
		ddp->adc_seq.calibrate = false;
		if (rc == 0) {
			u16_t batt_mV = 0;

			rc = divider_convert(ddp, &batt_mV, 1);
			if (rc > 0) {
				rc = batt_mV;
			} else if (rc == 0) {
				rc = -EINVAL;
			}
		}
	}

	return rc;
}

unsigned int battery_level_pptt(unsigned int batt_mV,
				const struct battery_level_point *curve)
{
	const struct battery_level_point *pb = curve;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	return pb->lvl_pptt
	       + ((pa->lvl_pptt - pb->lvl_pptt)
		  * (batt_mV - pb->lvl_mV)
		  / (pa->lvl_mV - pb->lvl_mV));
}
