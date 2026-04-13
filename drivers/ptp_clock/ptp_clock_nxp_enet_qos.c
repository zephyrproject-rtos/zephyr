/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PTP clock driver for NXP ENET QoS.
 *
 * The ENET QoS PTP subsystem operates in fine-update mode.
 * A 32-bit accumulator is incremented by ADDEND each reference-clock
 * cycle; every time it overflows the system-time sub-second register
 * is bumped by SNSINC nanoseconds.  Rate discipline is achieved by
 * adjusting ADDEND via the TSADDREG mechanism.
 *
 * Register macros (ENET_MAC_*) come from the MCXN CMSIS device header
 * (PERI_ENET.h) included transitively via eth_nxp_enet_qos.h.
 * The fsl_enet_qos SDK HAL is intentionally NOT used here.
 */

#define DT_DRV_COMPAT nxp_enet_qos_ptp_clock

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet_qos.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ptp_clock_nxp_enet_qos);

/*
 * Desired PTP system-time clock frequency in Hz.
 * Must be strictly less than the reference (bus) clock so that
 * ADDEND = 2^32 * ptpClk / refClk fits in a 32-bit register.
 * 50 MHz matches ENET_QOS_SYSTIME_REQUIRED_CLK_MHZ in the NXP HAL.
 */
#define PTP_CLOCK_NXP_ENET_QOS_PTPCLK_HZ 50000000U

struct ptp_clock_nxp_enet_qos_config {
	const struct device *enet_qos_dev;	/* device of the parent ENET QoSmodule */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct ptp_clock_nxp_enet_qos_data {
	enet_qos_t *base;	/* base address of the parent ENET QoS module */
	struct k_mutex ptp_mutex;
	uint32_t nominal_addend;
	uint32_t ref_clk_hz;	/* the actual PTP clock frequency */
};

static int ptp_clock_nxp_enet_qos_set(const struct device *dev, struct net_ptp_time *tm)
{
	LOG_DBG("PTP set time: %u s, %u ns", (unsigned int)tm->second, tm->nanosecond);

	struct ptp_clock_nxp_enet_qos_data *data = dev->data;

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	data->base->MAC_SYSTEM_TIME_SECONDS_UPDATE = (uint32_t)tm->second;
	/* ADDSUB = 0: absolute initialise */
	data->base->MAC_SYSTEM_TIME_NANOSECONDS_UPDATE =
		tm->nanosecond & ENET_MAC_SYSTEM_TIME_NANOSECONDS_UPDATE_TSSS_MASK;

	/* Use TSINIT (absolute load), valid because the accumulator is already
	 * running so it self-clears on the next accumulator overflow (~20 ns).
	 */
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSINIT_MASK;
	while (data->base->MAC_TIMESTAMP_CONTROL & ENET_MAC_TIMESTAMP_CONTROL_TSINIT_MASK) {
	}

	k_mutex_unlock(&data->ptp_mutex);
	return 0;
}

static int ptp_clock_nxp_enet_qos_get(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_clock_nxp_enet_qos_data *data = dev->data;
	uint32_t ns1, ns2, sec;

	/*
	 * Guard against a seconds roll-over between the two nanosecond reads:
	 * re-read if nanoseconds decreased (wrap occurred).
	 */
	do {
		ns1 = data->base->MAC_SYSTEM_TIME_NANOSECONDS &
		      ENET_MAC_SYSTEM_TIME_NANOSECONDS_TSSS_MASK;
		sec = data->base->MAC_SYSTEM_TIME_SECONDS;
		ns2 = data->base->MAC_SYSTEM_TIME_NANOSECONDS &
		      ENET_MAC_SYSTEM_TIME_NANOSECONDS_TSSS_MASK;
	} while (ns2 < ns1);

	tm->second = sec;
	tm->nanosecond = ns2;
	return 0;
}

/**
 * adjust PTP clock by increment nanoseconds, positive or negative.
 */
static int ptp_clock_nxp_enet_qos_adjust(const struct device *dev, int increment)
{
	LOG_DBG("PTP rate adjust increment: %d", increment);

	struct ptp_clock_nxp_enet_qos_data *data = dev->data;
	uint32_t ns_update;

	if ((increment <= (-(int32_t)NSEC_PER_SEC)) || (increment >= (int32_t)NSEC_PER_SEC)) {
		return -EINVAL;
	}

	if (increment >= 0) {
		/* ADDSUB = 0: add */
		ns_update = (uint32_t)increment & ENET_MAC_SYSTEM_TIME_NANOSECONDS_UPDATE_TSSS_MASK;
	} else {
		/* ADDSUB = 1: subtract */
		ns_update = ((uint32_t)(-increment) &
			     ENET_MAC_SYSTEM_TIME_NANOSECONDS_UPDATE_TSSS_MASK) |
			    ENET_MAC_SYSTEM_TIME_NANOSECONDS_UPDATE_ADDSUB_MASK;
	}

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	data->base->MAC_SYSTEM_TIME_SECONDS_UPDATE = 0;
	data->base->MAC_SYSTEM_TIME_NANOSECONDS_UPDATE = ns_update;
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSUPDT_MASK;
	while (data->base->MAC_TIMESTAMP_CONTROL & ENET_MAC_TIMESTAMP_CONTROL_TSUPDT_MASK) {
	}

	k_mutex_unlock(&data->ptp_mutex);
	return 0;
}

static int ptp_clock_nxp_enet_qos_rate_adjust(const struct device *dev, double ratio)
{
	LOG_DBG("PTP rate adjust ratio: %f", ratio);

	struct ptp_clock_nxp_enet_qos_data *data = dev->data;
	uint32_t new_addend;

	/* No meaningful change */
	if ((ratio > 1.0 && ratio - 1.0 < 1e-9) || (ratio < 1.0 && 1.0 - ratio < 1e-9)) {
		return 0;
	}

	new_addend = (uint32_t)((double)data->nominal_addend * ratio);

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	data->base->MAC_TIMESTAMP_ADDEND = new_addend;
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSADDREG_MASK;
	while (data->base->MAC_TIMESTAMP_CONTROL & ENET_MAC_TIMESTAMP_CONTROL_TSADDREG_MASK) {
	}

	k_mutex_unlock(&data->ptp_mutex);
	return 0;
}

static int ptp_clock_nxp_enet_qos_init(const struct device *dev)
{
	LOG_INF("Initializing NXP ENET QoS PTP clock on device %s", dev->name);
	const struct ptp_clock_nxp_enet_qos_config *config = dev->config;
	struct ptp_clock_nxp_enet_qos_data *data = dev->data;
	struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_qos_dev);
	const uint32_t snsinc = NSEC_PER_SEC / PTP_CLOCK_NXP_ENET_QOS_PTPCLK_HZ;
	uint32_t clk_rate;
	int ret;

	data->base = module_cfg->base;
	k_mutex_init(&data->ptp_mutex);

	LOG_INF("Starting NXP ENET QoS PTP hardware");

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clk_rate);
	if (ret) {
		LOG_ERR("Failed to get PTP clock");
		return ret;
	}

	data->ref_clk_hz = clk_rate;
	LOG_INF("PTP reference clock %u Hz", data->ref_clk_hz);
	__ASSERT(data->ref_clk_hz != 0, "Failed to get PTP clock");

	data->nominal_addend =
		(uint32_t)((double)(1ULL << 32) * (double)PTP_CLOCK_NXP_ENET_QOS_PTPCLK_HZ /
			   (double)data->ref_clk_hz);
	LOG_INF("PTP accumulator addend %u nsec", data->nominal_addend);

	/*
	 * Step 1: Enable timestamping in COARSE update mode (no TSCFUPDT yet).
	 * In coarse mode TSINIT self-clears on the next mac_ptp_ref_clk edge,
	 * not on an accumulator overflow, so it completes immediately.
	 * nanosecond rollover in digital logic
	 */
	data->base->MAC_TIMESTAMP_CONTROL =
		ENET_MAC_TIMESTAMP_CONTROL_TSENA_MASK | ENET_MAC_TIMESTAMP_CONTROL_TSIPV4ENA_MASK |
		ENET_MAC_TIMESTAMP_CONTROL_TSIPV6ENA_MASK |
		ENET_MAC_TIMESTAMP_CONTROL_TSENALL_MASK |
		ENET_MAC_TIMESTAMP_CONTROL_TSEVNTENA_MASK |
		ENET_MAC_TIMESTAMP_CONTROL_SNAPTYPSEL_MASK |
		ENET_MAC_TIMESTAMP_CONTROL_TSCTRLSSR(1) |
		ENET_MAC_TIMESTAMP_CONTROL_TSVER2ENA_MASK | ENET_MAC_TIMESTAMP_CONTROL_TSIPENA_MASK;

	/* Step 2: initialize system time to zero (coarse mode — completes quickly) */
	data->base->MAC_SYSTEM_TIME_NANOSECONDS_UPDATE = 0;
	data->base->MAC_SYSTEM_TIME_SECONDS_UPDATE = 0;
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSINIT_MASK;
	while (data->base->MAC_TIMESTAMP_CONTROL & ENET_MAC_TIMESTAMP_CONTROL_TSINIT_MASK) {
	}

	/* Step 3: switch to fine update mode */
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSCFUPDT_MASK;

	/* Step 4: set sub-second increment for 50 MHz PTP clock → 20 ns/tick */
	data->base->MAC_SUB_SECOND_INCREMENT = ENET_MAC_SUB_SECOND_INCREMENT_SNSINC(snsinc);

	/*
	 * Step 5: load the nominal addend into the fine accumulator.
	 * TSENA has already propagated to mac_ptp_ref_clk domain (step 1),
	 * so TSADDREG self-clears promptly.
	 */
	data->base->MAC_TIMESTAMP_ADDEND = data->nominal_addend;
	data->base->MAC_TIMESTAMP_CONTROL |= ENET_MAC_TIMESTAMP_CONTROL_TSADDREG_MASK;
	while (data->base->MAC_TIMESTAMP_CONTROL & ENET_MAC_TIMESTAMP_CONTROL_TSADDREG_MASK) {
	}

	return 0;
}

static DEVICE_API(ptp_clock, ptp_clock_nxp_enet_qos_api) = {
	.set = ptp_clock_nxp_enet_qos_set,
	.get = ptp_clock_nxp_enet_qos_get,
	.adjust = ptp_clock_nxp_enet_qos_adjust,
	.rate_adjust = ptp_clock_nxp_enet_qos_rate_adjust,
};

#define PTP_CLOCK_NXP_ENET_QOS_INIT(n)                                                             \
	static const struct ptp_clock_nxp_enet_qos_config ptp_clock_nxp_enet_qos_##n##_config = {  \
		.enet_qos_dev = DEVICE_DT_GET(DT_INST_GPARENT(n)),                                 \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
	};                                                                                         \
                                                                                                   \
	static struct ptp_clock_nxp_enet_qos_data ptp_clock_nxp_enet_qos_##n##_data;               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ptp_clock_nxp_enet_qos_init, NULL,                               \
			      &ptp_clock_nxp_enet_qos_##n##_data,                                  \
			      &ptp_clock_nxp_enet_qos_##n##_config, POST_KERNEL,                   \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &ptp_clock_nxp_enet_qos_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_NXP_ENET_QOS_INIT)
