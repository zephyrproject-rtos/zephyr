/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_adc_b

#include <string.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/adc/adc.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "fsp_common_api.h"
#include <instances/r_adc_b.h>

LOG_MODULE_REGISTER(renesas_ra_adc_b, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_B_RA_MAX_VC            32U /* Max virtual channels addressable via Zephyr ADC API */
#define ADC_B_RA_MAX_VC_PER_GROUP  8U  /* Max virtual channels per scan group */
#define ADC_B_RA_MAX_SCAN_GROUPS   8U  /* Max scan groups */
#define ADC_B_RA_MAX_SAMPLE_TABLES 15U /* sample_table_id is 4-bit (tables 0..14) */

/* Sampling state table */
#define ADC_B_SST_MIN      2U
#define ADC_B_SST_MAX      1023U
#define ADC_B_NSEC_PER_SEC 1000000000L

#define ADC_B_CAL_TIMEOUT K_MSEC(10)

#if DT_NODE_EXISTS(DT_NODELABEL(adc0))
#define ADC0_NODE DT_NODELABEL(adc0)
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(adc1))
#define ADC1_NODE DT_NODELABEL(adc1)
#endif

/* FSP ISR symbols selected per unit / scan group at build time */
#define ADC_B_ERR_ISR(unit)     UTIL_CAT(UTIL_CAT(adc_b_err, unit), _isr)
#define ADC_B_CAL_END_ISR(unit) UTIL_CAT(UTIL_CAT(adc_b_calend, unit), _isr)
#define ADC_B_SCAN_END_ISR(grp) UTIL_CAT(UTIL_CAT(adc_b_adi, grp), _isr)

/* Irq/ipl macros */
#define ISR_CFG_CONVSERR_IRQ(unit) UTIL_CAT(conversion_error_irq_adc_, unit)
#define ISR_CFG_CONVSERR_IPL(unit) UTIL_CAT(conversion_error_ipl_adc_, unit)
#define ISR_CFG_SCANEND_IRQ(grp)   UTIL_CAT(scan_end_irq_group_, grp)
#define ISR_CFG_SCANEND_IPL(grp)   UTIL_CAT(scan_end_ipl_group_, grp)

/* Method definition */
#define ADC_B_DT_METHOD(idx)                                                                       \
	UTIL_CAT(ADC_B_CONVERSION_METHOD_, DT_INST_STRING_UPPER_TOKEN(idx, renesas_adc_method))

#define ADC_B_DFSEL_SINC3_ALL                                                                      \
	{                                                                                          \
		.settings = {                                                                      \
			.idx_0 = ADC_B_DIGITAL_FILTER_MODE_SINC3,                                  \
			.idx_1 = ADC_B_DIGITAL_FILTER_MODE_SINC3,                                  \
			.idx_2 = ADC_B_DIGITAL_FILTER_MODE_SINC3,                                  \
			.idx_3 = ADC_B_DIGITAL_FILTER_MODE_SINC3,                                  \
		}                                                                                  \
	}

/* extern FSP scan-end ISRs (one per hardware scan-group 0..7) */
extern void adc_b_adi0_isr(void);
extern void adc_b_adi1_isr(void);
extern void adc_b_adi2_isr(void);
extern void adc_b_adi3_isr(void);
extern void adc_b_adi4_isr(void);
extern void adc_b_adi5_isr(void);
extern void adc_b_adi6_isr(void);
extern void adc_b_adi7_isr(void);

/* Calibration ISRs */
extern void adc_b_calend0_isr(void);
extern void adc_b_calend1_isr(void);

/* Error ISRs */
extern void adc_b_err0_isr(void);
extern void adc_b_err1_isr(void);

/**
 * @brief Per virtual-channel configuration parsed from DTS.
 */
struct adc_ra_adc_b_channel_cfg {
	uint8_t vc_id;          /* reg of channel@N, virtual channel id */
	uint8_t input_positive; /* zephyr,input-positive */
	uint8_t resolution;     /* zephyr,resolution */
	uint8_t oversampling;   /* zephyr,oversampling */
	uint8_t scan_group_id;  /* DT_REG_ADDR of renesas,scan-group phandle */
	uint16_t acq_time;      /* zephyr,acquisition-time (ADC_ACQ_TIME encoding) */
};

/**
 * @brief Compile-time map of (scan-group ID, owning converter unit ID).
 *
 * Built directly from scan-group nodes: each scan-group declares its
 * owning converter via the renesas,converter phandle.
 */
struct adc_b_sg_unit_map {
	uint8_t scan_group_id;
	uint8_t unit_id;
};

#define ADC_B_SG_MAP_ENTRY(node_id)                                                                \
	{                                                                                          \
		.scan_group_id = DT_REG_ADDR_RAW(node_id),                                         \
		.unit_id = DT_PROP(node_id, renesas_converter_unit_id),                            \
	},

static const struct adc_b_sg_unit_map adc_b_sg_unit_map[] = {
	DT_FOREACH_STATUS_OKAY(renesas_ra_adc_b_scan_group, ADC_B_SG_MAP_ENTRY)};

/* Look up the owning converter unit ID for a given scan_group_id.
 * Returns UINT8_MAX if the scan-group is not present in the system
 * (should never happen for a channel-referenced scan-group, given the
 * BUILD_ASSERT enforced per channel below).
 */
static inline uint8_t adc_b_sg_to_unit(uint8_t scan_group_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(adc_b_sg_unit_map); i++) {
		if (adc_b_sg_unit_map[i].scan_group_id == scan_group_id) {
			return adc_b_sg_unit_map[i].unit_id;
		}
	}
	return UINT8_MAX;
}

struct adc_ra_adc_b_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	uint8_t clock_div;

	/* ADC unit id (0 for ADC0, 1 for ADC1). */
	uint8_t unit_id;

	/* Bitmask of valid zephyr,input-positive values for this unit. */
	uint32_t channel_available_mask;

	/* Channel config array built from this unit's DTS child nodes. */
	const struct adc_ra_adc_b_channel_cfg *dt_channels;
	uint8_t dt_channel_count;

	void (*irq_configure)(void);
};

struct adc_ra_adc_b_data {
	struct adc_context ctx;
	const struct device *dev;
	adc_b_instance_ctrl_t adc_ctrl;

	adc_cfg_t adc_cfg;
	adc_b_scan_cfg_t scan_cfg;

	/* Per scan-group FSP configuration */
	adc_b_group_cfg_t group_cfg[ADC_B_RA_MAX_SCAN_GROUPS];
	adc_b_group_cfg_t *groups[ADC_B_RA_MAX_SCAN_GROUPS];

	/* Virtual channel configurations */
	adc_b_virtual_channel_cfg_t vc_cfg[ADC_B_RA_MAX_VC];
	adc_b_virtual_channel_cfg_t *group_vcs[ADC_B_RA_MAX_SCAN_GROUPS][ADC_B_RA_MAX_VC_PER_GROUP];

	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	uint16_t buf_id;
	/** Mask with channels that will be sampled */
	uint32_t channels;

	/* Group-completion tracking for multi-group scans */
	uint32_t pending_group_mask;
	uint32_t completed_group_mask;

	/* Deferred scan-start work: when adc_context_start_sampling is called
	 * from ISR context (extra_samplings chaining with interval_us=0).
	 */
	struct k_work start_work;

	/* Semaphore for calibration */
	struct k_sem calibrate_sem;

	/* bitmask of configured channels (channel_setup) */
	uint32_t configured_channels;

	/* Set by channel_setup() when the channel set changed; cleared once
	 * the scan has been (re)programmed and calibrated in start_read().
	 */
	bool scan_cfg_dirty;
};

static int adc_ra_validate_phys_channel(uint32_t available_mask, uint8_t phys)
{
	if (phys >= 32U || (available_mask & BIT(phys)) == 0U) {
		return -EINVAL;
	}

	return 0;
}

static adc_b_data_format_t ra_adc_resolution_to_format(uint8_t resolution)
{
	switch (resolution) {
	case 10:
		return ADC_B_DATA_FORMAT_10_BIT;
	case 12:
		return ADC_B_DATA_FORMAT_12_BIT;
	case 14:
		return ADC_B_DATA_FORMAT_14_BIT;
	case 16:
		return ADC_B_DATA_FORMAT_16_BIT;
	default:
		LOG_WRN("Unsupported resolution %d, using 12-bit default", resolution);
		return ADC_B_DATA_FORMAT_12_BIT;
	}
}

/* This function maps the zephyr,oversampling count to the FSP average count
 * zephyr,oversampling is the number of additional samples to be taken (2^n total samples)
 */
static uint8_t adc_ra_ovs_to_avg_count(uint8_t n)
{
	switch (n) {
	case 1:
		return ADC_B_ADD_AVERAGE_2;
	case 2:
		return ADC_B_ADD_AVERAGE_4;
	case 3:
		return ADC_B_ADD_AVERAGE_8;
	case 4:
		return ADC_B_ADD_AVERAGE_16;
	case 5:
		return ADC_B_ADD_AVERAGE_32;
	case 6:
		return ADC_B_ADD_AVERAGE_64;
	case 7:
		return ADC_B_ADD_AVERAGE_128;
	case 8:
		return ADC_B_ADD_AVERAGE_256;
	case 9:
		return ADC_B_ADD_AVERAGE_512;
	case 10:
		return ADC_B_ADD_AVERAGE_1024;
	default:
		return ADC_B_ADD_AVERAGE_1; /* n == 0 → off */
	}
}

/* Return whether a virtual channel runs the oversampling path, given the
 * converter method and the channel's zephyr,oversampling value:
 *   - SAR:        never (always SAR)
 *   - OVERSAMPLE: always
 *   - HYBRID:     per-channel, oversample only if oversampling > 0
 */
static bool adc_ra_vc_uses_oversample(adc_b_conversion_method_t method, uint8_t oversampling)
{
	switch (method) {
	case ADC_B_CONVERSION_METHOD_OVERSAMPLE:
		return true;
	case ADC_B_CONVERSION_METHOD_HYBRID:
		return (oversampling > 0U);
	case ADC_B_CONVERSION_METHOD_SAR:
	default:
		return false;
	}
}

/* Initialize virtual-channel configuration from its DTS channel node. */
static void adc_ra_adc_b_vc_configure(const struct adc_ra_adc_b_channel_cfg *ch,
				      adc_b_virtual_channel_cfg_t *vc,
				      adc_b_conversion_method_t method, uint8_t sample_table_id)
{
	memset(vc, 0, sizeof(*vc));

	vc->channel_id = (adc_b_virtual_channel_t)ch->vc_id;
	vc->channel_cfg_bits.group = BIT(ch->scan_group_id);
	vc->channel_cfg_bits.channel = (adc_channel_t)ch->input_positive;
	vc->channel_cfg_bits.sample_table_id = sample_table_id;

	if (adc_ra_vc_uses_oversample(method, ch->oversampling)) {
		vc->channel_control_a_bits.digital_filter_id = 1;
		vc->channel_control_b_bits.addition_average_mode = ADC_B_ADD_AVERAGE_AVERAGE_ENABLE;
		vc->channel_control_b_bits.addition_average_count =
			adc_ra_ovs_to_avg_count(ch->oversampling);
	}

	vc->channel_control_c_bits.channel_data_format =
		ra_adc_resolution_to_format(ch->resolution);
	vc->channel_control_c_bits.data_is_unsigned = true;
}

/* Decode a Zephyr ADC_ACQ_TIME-encoded value to nanoseconds.
 * ADC_ACQ_TIME_DEFAULT yields 0 (caller falls back to the minimum SST).
 */
static int adc_ra_acq_time_to_ns(uint16_t acq_time, uint32_t *ns_out)
{
	uint16_t value = ADC_ACQ_TIME_VALUE(acq_time);
	uint32_t ns;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		ns = 0U;
	} else {
		switch (ADC_ACQ_TIME_UNIT(acq_time)) {
		case ADC_ACQ_TIME_NANOSECONDS:
			ns = value;
			break;
		case ADC_ACQ_TIME_MICROSECONDS:
			ns = (uint32_t)value * 1000U;
			break;
		default:
			return -ENOTSUP;
		}
	}

	if (ns_out != NULL) {
		*ns_out = ns;
	}

	return 0;
}

/* Convert a sampling time in nanoseconds to an ADC_B sampling-state-table
 * register value (SST cycles), clamped to the legal [MIN, MAX] range.
 * A 0 ns input maps to the minimum legal SST.
 */
static int adc_ra_sampling_to_states(const struct device *dev, uint32_t sampling_time_ns,
				     uint16_t *states_out)
{
	const struct adc_ra_adc_b_config *cfg = dev->config;
	uint32_t clock_source_hz;
	uint64_t tmp;
	int ret;

	if (states_out == NULL) {
		return -EINVAL;
	}

	if (sampling_time_ns == 0U) {
		*states_out = ADC_B_SST_MIN;
		return 0;
	}

	ret = clock_control_get_rate(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys,
				     &clock_source_hz);
	if (ret < 0) {
		return ret;
	}

	tmp = (uint64_t)sampling_time_ns * clock_source_hz / cfg->clock_div;
	tmp = (tmp + ADC_B_NSEC_PER_SEC - 1) / ADC_B_NSEC_PER_SEC;

	if (tmp > ADC_B_SST_MAX) {
		tmp = ADC_B_SST_MAX;
	} else if (tmp < ADC_B_SST_MIN) {
		tmp = ADC_B_SST_MIN;
	} else {
		/* do nothing */
	}

	*states_out = (uint16_t)tmp;
	return 0;
}

/* Find an existing sampling-state table matching ns, or allocate a new one
 * and program it into extend->sampling_state_table[]. Returns the table
 * index (== sample_table_id) or a negative errno.
 *
 * Each distinct acquisition-time gets its own table; up to
 * ADC_B_RA_MAX_SAMPLE_TABLES distinct values are supported.
 */
static int adc_ra_alloc_sample_table(const struct device *dev, adc_b_extended_cfg_t *extend,
				     uint32_t *table_ns, uint8_t *table_count, uint32_t ns)
{
	uint16_t sst;
	int ret;

	for (uint8_t i = 0U; i < *table_count; i++) {
		if (table_ns[i] == ns) {
			return i;
		}
	}

	if (*table_count >= ADC_B_RA_MAX_SAMPLE_TABLES) {
		LOG_ERR("too many distinct acquisition-times (max %u)", ADC_B_RA_MAX_SAMPLE_TABLES);
		return -ENOMEM;
	}

	ret = adc_ra_sampling_to_states(dev, ns, &sst);
	if (ret < 0) {
		return ret;
	}

	uint8_t idx = (*table_count)++;

	table_ns[idx] = ns;
	extend->sampling_state_table[idx] = sst;

	return idx;
}

/* Populate the virtual-channel list for one already-discovered scan-group g.
 * vc_global_idx is the running index into data->vc_cfg[] shared across groups.
 */
static int adc_ra_adc_b_populate_group(const struct device *dev, uint8_t g,
				       adc_b_conversion_method_t method, uint32_t *table_ns,
				       uint8_t *table_count, uint8_t *vc_global_idx)
{
	const struct adc_ra_adc_b_config *cfg = dev->config;
	struct adc_ra_adc_b_data *data = dev->data;
	adc_b_extended_cfg_t *extend = (adc_b_extended_cfg_t *)data->adc_cfg.p_extend;
	uint8_t sg_id = (uint8_t)data->group_cfg[g].scan_group_id;
	uint8_t vc_in_group = 0U;

	for (uint8_t i = 0U; i < cfg->dt_channel_count; i++) {
		const struct adc_ra_adc_b_channel_cfg *ch = &cfg->dt_channels[i];
		uint32_t acq_ns;
		int ret;

		if (!(data->configured_channels & BIT(ch->vc_id))) {
			continue;
		}
		if (ch->scan_group_id != sg_id) {
			continue;
		}

		ret = adc_ra_validate_phys_channel(cfg->channel_available_mask, ch->input_positive);
		if (ret < 0) {
			return ret;
		}

		/* SAR method only support into 12bits. */
		if (!adc_ra_vc_uses_oversample(method, ch->oversampling) && ch->resolution > 12U) {
			LOG_ERR("vc %u: SAR-path channel limited to 12-bit (got %u)", ch->vc_id,
				ch->resolution);
			return -EINVAL;
		}

		if (vc_in_group >= ADC_B_RA_MAX_VC_PER_GROUP) {
			LOG_ERR("Too many channels in scan-group %u (max %u)", sg_id,
				ADC_B_RA_MAX_VC_PER_GROUP);
			return -ENOMEM;
		}
		if (*vc_global_idx >= ADC_B_RA_MAX_VC) {
			return -ENOMEM;
		}

		/* Allocate a sampling-state table for this channel's
		 * acquisition-time (deduplicated across channels).
		 */
		ret = adc_ra_acq_time_to_ns(ch->acq_time, &acq_ns);
		if (ret < 0) {
			return ret;
		}

		ret = adc_ra_alloc_sample_table(dev, extend, table_ns, table_count, acq_ns);
		if (ret < 0) {
			return ret;
		}

		adc_ra_adc_b_vc_configure(ch, &data->vc_cfg[*vc_global_idx], method, (uint8_t)ret);
		data->group_vcs[g][vc_in_group] = &data->vc_cfg[*vc_global_idx];
		vc_in_group++;
		(*vc_global_idx)++;
	}

	data->group_cfg[g].virtual_channel_count = vc_in_group;
	data->group_cfg[g].p_virtual_channels = (adc_b_virtual_channel_cfg_t **)data->group_vcs[g];

	return 0;
}

/* Build adc_b_scan_cfg_t with one adc_b_group_cfg_t per scan-group that has
 * at least one configured virtual channel under this unit.
 */
static int adc_ra_adc_b_scan_cfg(const struct device *dev)
{
	const struct adc_ra_adc_b_config *cfg = dev->config;
	struct adc_ra_adc_b_data *data = dev->data;
	adc_b_extended_cfg_t *extend = (adc_b_extended_cfg_t *)data->adc_cfg.p_extend;
	uint8_t group_count = 0U;
	uint8_t vc_global_idx = 0U;
	uint32_t table_ns[ADC_B_RA_MAX_SAMPLE_TABLES];
	uint8_t table_count = 0U;

	if (cfg->dt_channel_count == 0U) {
		return -EINVAL;
	}

	adc_b_conversion_method_t method = extend->adc_b_converter_mode[cfg->unit_id].method;

	/* Discover unique scan-group IDs across configured channels */
	for (uint8_t i = 0U; i < cfg->dt_channel_count; i++) {
		const struct adc_ra_adc_b_channel_cfg *ch = &cfg->dt_channels[i];
		bool already_added = false;

		if (!(data->configured_channels & BIT(ch->vc_id))) {
			continue;
		}

		for (uint8_t j = 0U; j < group_count; j++) {
			if (data->group_cfg[j].scan_group_id == (adc_group_id_t)ch->scan_group_id) {
				already_added = true;
				break;
			}
		}
		if (already_added) {
			continue;
		}

		if (group_count >= ADC_B_RA_MAX_SCAN_GROUPS) {
			return -ENOMEM;
		}

		memset(&data->group_cfg[group_count], 0, sizeof(data->group_cfg[group_count]));
		data->group_cfg[group_count].scan_group_id = (adc_group_id_t)ch->scan_group_id;

		data->group_cfg[group_count].converter_selection =
			(adc_b_unit_id_t)adc_b_sg_to_unit(ch->scan_group_id);
		data->group_cfg[group_count].scan_group_enable = true;
		data->group_cfg[group_count].scan_end_interrupt_enable = true;
		data->group_cfg[group_count].external_trigger_enable_mask =
			(adc_b_external_trigger_t)ADC_B_EXTERNAL_TRIGGER_NONE;

		data->groups[group_count] = &data->group_cfg[group_count];
		group_count++;
	}

	if (group_count == 0U) {
		LOG_ERR("No channels configured");
		return -EINVAL;
	}

	/* for each discovered scan-group, populate its VC list */
	for (uint8_t g = 0U; g < group_count; g++) {
		int ret = adc_ra_adc_b_populate_group(dev, g, method, table_ns, &table_count,
						      &vc_global_idx);
		if (ret < 0) {
			return ret;
		}
	}

	memset(&data->scan_cfg, 0, sizeof(data->scan_cfg));
	data->scan_cfg.group_count = group_count;
	data->scan_cfg.p_adc_groups = (adc_b_group_cfg_t **)data->groups;

	return 0;
}

/* Populate extended_cfg.converter_selection_0/1/2 with the FULL system map of
 * scan-group -> owning converter.
 */
static void adc_ra_setup_converter_selection(adc_b_extended_cfg_t *extend)
{
	uint32_t cs[3] = {0U, 0U, 0U};

	for (size_t i = 0U; i < ARRAY_SIZE(adc_b_sg_unit_map); i++) {
		uint8_t gid = adc_b_sg_unit_map[i].scan_group_id;
		uint8_t uid = adc_b_sg_unit_map[i].unit_id;
		uint8_t reg_idx = gid / 4U;
		uint8_t bit_pos = (gid % 4U) * 8U;

		if (reg_idx < ARRAY_SIZE(cs)) {
			cs[reg_idx] |= ((uint32_t)uid << bit_pos);
		}
	}

	extend->converter_selection_0 = cs[0];
	extend->converter_selection_1 = cs[1];
	extend->converter_selection_2 = cs[2];
}

static const struct adc_ra_adc_b_channel_cfg *
adc_ra_find_dt_channel(const struct adc_ra_adc_b_config *config, uint8_t channel_id)
{
	for (uint8_t i = 0U; i < config->dt_channel_count; i++) {
		if (config->dt_channels[i].vc_id == channel_id) {
			return &config->dt_channels[i];
		}
	}

	return NULL;
}

static adc_b_clock_source_t adc_ra_get_clock_source(const struct device *clock_dev)
{
	if (clock_dev == NULL) {
		return ADC_B_CLOCK_SOURCE_PCLKA;
	}

	const char *name = clock_dev->name;

	if (strcmp(name, "pclka") == 0) {
		return ADC_B_CLOCK_SOURCE_PCLKA;
	} else if (strcmp(name, "gptclk") == 0) {
		return ADC_B_CLOCK_SOURCE_GPT;
	} else if (strcmp(name, "adcclk") == 0) {
		return ADC_B_CLOCK_SOURCE_ADC;
	}

	/* Default to PCLKA for unknown clock sources */
	LOG_DBG("Unknown clock source device '%s', defaulting to PCLKA", name);
	return ADC_B_CLOCK_SOURCE_PCLKA;
}

/* This function validates the virtual channel ID. The maximum virtual channel ID is defined by
 * ADC_B_RA_MAX_VC and the channel must be defined in the device tree.
 */
static int adc_ra_validate_vc_id(const struct adc_ra_adc_b_config *config, uint8_t vc_id)
{
	if (vc_id >= ADC_B_RA_MAX_VC) {
		return -EINVAL;
	}

	if (adc_ra_find_dt_channel(config, vc_id) == NULL) {
		return -EINVAL;
	}

	return 0;
}

/* Run a hardware calibration and block until the calibration-complete
 * event is signalled from the FSP callback. Must be called from thread
 * context (it sleeps on the semaphore).
 */
static int adc_ra_adc_b_calibrate(struct adc_ra_adc_b_data *data)
{
	fsp_err_t fsp_err;

	k_sem_reset(&data->calibrate_sem);

	fsp_err = R_ADC_B_Calibrate(&data->adc_ctrl, NULL);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_ADC_B_Calibrate failed: %d", fsp_err);
		return -EIO;
	}

	if (k_sem_take(&data->calibrate_sem, ADC_B_CAL_TIMEOUT) != 0) {
		LOG_ERR("calibration timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

/* Called from start_read() only when the channel
 * set changed, so repeated channel_setup() calls do not re-run ScanCfg or
 * trigger a calibration each time.
 */
static int adc_ra_adc_b_apply_scan_cfg(const struct device *dev)
{
	struct adc_ra_adc_b_data *data = dev->data;
	fsp_err_t fsp_err;
	int ret;

	ret = adc_ra_adc_b_scan_cfg(dev);
	if (ret < 0) {
		LOG_ERR("failed to configure scan: %d", ret);
		return ret;
	}

	fsp_err = R_ADC_B_ScanCfg(&data->adc_ctrl, &data->scan_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_ADC_B_ScanCfg failed: %d", fsp_err);
		return -EIO;
	}

	ret = adc_ra_adc_b_calibrate(data);
	if (ret < 0) {
		return ret;
	}

	data->scan_cfg_dirty = false;

	return 0;
}

static int adc_ra_adc_b_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	struct adc_ra_adc_b_data *data = dev->data;
	const struct adc_ra_adc_b_config *config = dev->config;
	int ret;

	ret = adc_ra_validate_vc_id(config, channel_cfg->channel_id);
	if (ret < 0) {
		LOG_ERR("invalid virtual channel id %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	/* Validate the acquisition-time encoding (table programmed at init). */
	if (adc_ra_acq_time_to_ns(channel_cfg->acquisition_time, NULL) < 0) {
		LOG_ERR("unsupported acquisition-time (use ns/us, not ticks)");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential mode not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("only ADC_GAIN_1 is supported");
		return -EINVAL;
	}

	/* Only record the channel here; the scan is (re)programmed and
	 * calibrated lazily on the next start_read().
	 */
	data->configured_channels |= BIT(channel_cfg->channel_id);
	data->scan_cfg_dirty = true;

	return 0;
}

static int adc_ra_adc_b_check_buffer_size(const struct device *dev,
					  const struct adc_sequence *sequence)
{
	struct adc_ra_adc_b_data *data = dev->data;
	size_t needed;

	uint8_t active = POPCOUNT(sequence->channels & data->configured_channels);

	needed = active * sizeof(uint16_t);

	if (sequence->options != NULL) {
		needed *= (1U + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_ra_adc_b_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_ra_adc_b_config *config = dev->config;
	struct adc_ra_adc_b_data *data = dev->data;
	int err;

	/* Each virtual channel converts with the resolution/oversampling fixed
	 * by its DTS channel node, so the sequence must match every selected
	 * channel (SAR-path channels are already validated against >12-bit in
	 * adc_ra_adc_b_scan_cfg()).
	 */
	for (uint8_t i = 0U; i < config->dt_channel_count; i++) {
		const struct adc_ra_adc_b_channel_cfg *ch = &config->dt_channels[i];

		if (!(sequence->channels & BIT(ch->vc_id))) {
			continue;
		}

		if (sequence->resolution != ch->resolution) {
			LOG_ERR("vc %u: sequence resolution %u does not match "
				"channel resolution %u",
				ch->vc_id, sequence->resolution, ch->resolution);
			return -EINVAL;
		}

		if (sequence->oversampling != 0U && sequence->oversampling != ch->oversampling) {
			LOG_ERR("vc %u: sequence oversampling %u does not match "
				"channel oversampling %u",
				ch->vc_id, sequence->oversampling, ch->oversampling);
			return -ENOTSUP;
		}
	}

	if ((sequence->channels & ~data->configured_channels) != 0) {
		LOG_ERR("channels 0x%08x not configured (configured: 0x%08x)", sequence->channels,
			data->configured_channels);
		return -EINVAL;
	}

	err = adc_ra_adc_b_check_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	/* Program the scan (and calibrate) only when the channel set changed
	 * since the last read; channel_setup() just marks it dirty.
	 */
	if (data->scan_cfg_dirty) {
		err = adc_ra_adc_b_apply_scan_cfg(dev);
		if (err < 0) {
			return err;
		}
	} else if (sequence->calibrate) {
		err = adc_ra_adc_b_calibrate(data);
		if (err < 0) {
			return err;
		}
	} else {
		/* do nothing */
	}

	data->buf_id = 0;
	data->buf = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_ra_adc_b_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct adc_ra_adc_b_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_ra_adc_b_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_ra_adc_b_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ra_adc_b_read_async(dev, sequence, NULL);
}

/* Compute the full pending scan-group bitmask from the channels selected
 * in the current adc_sequence. Caller resets completed_group_mask.
 */
static void adc_ra_adc_b_compute_pending(struct adc_ra_adc_b_data *data)
{
	const struct adc_ra_adc_b_config *cfg = data->dev->config;
	uint32_t group_mask = 0U;

	for (uint8_t i = 0U; i < cfg->dt_channel_count; i++) {
		const struct adc_ra_adc_b_channel_cfg *ch = &cfg->dt_channels[i];

		if (!(data->channels & BIT(ch->vc_id))) {
			continue;
		}
		group_mask |= BIT(ch->scan_group_id);
	}

	data->pending_group_mask = group_mask;
	data->completed_group_mask = 0U;
}

/* Trigger the next pending scan-group, one at a time. Multiple scan-groups
 * assigned to the same converter cannot run simultaneously: if both bits
 * are written to ADSYSTR together, only the lowest-numbered group's
 * scan-end IRQ fires and the other never runs. Serialize by triggering
 * the lowest pending bit, then trigger the next from the scan-end ISR.
 */
static int adc_ra_adc_b_scan_next_group(struct adc_ra_adc_b_data *data)
{
	uint32_t to_start = data->pending_group_mask & ~data->completed_group_mask;
	uint32_t next_group;
	fsp_err_t fsp_err;

	if (to_start == 0U) {
		return -ENODATA;
	}

	next_group = to_start & (uint32_t)(-(int32_t)to_start);

	fsp_err = R_ADC_B_ScanGroupStart(&data->adc_ctrl, (adc_group_mask_t)next_group);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_ADC_B_ScanGroupStart(0x%02x) failed: %d", next_group, fsp_err);
		adc_context_complete(&data->ctx, -EIO);
		return -EIO;
	}

	return 0;
}

static void adc_ra_adc_b_start_work_handler(struct k_work *work)
{
	struct adc_ra_adc_b_data *data = CONTAINER_OF(work, struct adc_ra_adc_b_data, start_work);

	(void)adc_ra_adc_b_scan_next_group(data);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ra_adc_b_data *data = CONTAINER_OF(ctx, struct adc_ra_adc_b_data, ctx);

	data->channels = ctx->sequence.channels;
	adc_ra_adc_b_compute_pending(data);

	if (data->pending_group_mask == 0U) {
		LOG_ERR("No scan-groups derived from sequence channels");
		adc_context_complete(ctx, -EINVAL);
		return;
	}

	if (k_is_in_isr()) {
		k_work_submit(&data->start_work);
	} else {
		(void)adc_ra_adc_b_scan_next_group(data);
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ra_adc_b_data *data = CONTAINER_OF(ctx, struct adc_ra_adc_b_data, ctx);

	if (repeat_sampling) {
		data->buf_id = 0;
	}
}

static int adc_ra_adc_b_init(const struct device *dev)
{
	const struct adc_ra_adc_b_config *config = dev->config;
	struct adc_ra_adc_b_data *data = dev->data;
	adc_b_extended_cfg_t *extend = (adc_b_extended_cfg_t *)data->adc_cfg.p_extend;
	adc_b_clock_source_t clock_source;
	uint8_t divr;
	fsp_err_t fsp_err;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->calibrate_sem, 0, 1);
	k_work_init(&data->start_work, adc_ra_adc_b_start_work_handler);

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	adc_ra_setup_converter_selection(extend);

	clock_source = adc_ra_get_clock_source(config->clock_dev);

	/* DT clock-div is the divider (1..8); the register
	 * field DIVR encodes (div - 1).
	 */
	divr = config->clock_div - 1U;
	extend->clock_control_data = ((clock_source << R_ADC_B0_ADCLKCR_CLKSEL_Pos) |
				      (divr << R_ADC_B0_ADCLKCR_DIVR_Pos));

	fsp_err = R_ADC_B_Open(&data->adc_ctrl, &data->adc_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_ADC_B_Open failed: %d", fsp_err);
		return -EIO;
	}

	config->irq_configure();

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

/* Read all configured-and-selected channels into the sequence buffer in
 * ascending channel-id order (the order applications expect when decoding
 * a multi-channel adc_sequence buffer). Called once per adc_sequence
 * sampling iteration, after all scan-groups in pending_group_mask have
 * completed.
 */
static int adc_ra_adc_b_drain_results(const struct device *dev)
{
	struct adc_ra_adc_b_data *data = dev->data;
	const struct adc_ra_adc_b_config *cfg = dev->config;
	uint16_t *sample_buffer = data->buf;
	fsp_err_t fsp_err;

	if (sample_buffer == NULL) {
		return 0;
	}

	for (uint8_t vc_id = 0U; vc_id < ADC_B_RA_MAX_VC; vc_id++) {
		const struct adc_ra_adc_b_channel_cfg *ch;
		uint32_t sample = 0;

		if (!(data->configured_channels & BIT(vc_id)) || !(data->channels & BIT(vc_id))) {
			continue;
		}

		ch = adc_ra_find_dt_channel(cfg, vc_id);
		if (ch == NULL) {
			continue;
		}

		fsp_err =
			R_ADC_B_Read32(&data->adc_ctrl, (adc_channel_t)ch->input_positive, &sample);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("R_ADC_B_Read32 failed for vc %u (phys %u): %d", vc_id,
				ch->input_positive, fsp_err);
			return -EIO;
		}

		sample_buffer[data->buf_id++] = (uint16_t)sample;
	}

	return 0;
}

static void renesas_ra_adc_callback(adc_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct adc_ra_adc_b_data *data = dev->data;

	switch (p_args->event) {
	case ADC_EVENT_SCAN_COMPLETE: {
		/* The shared scan-end ISR fires for ANY unit, so filter to
		 * the scan-groups this sequence is actually waiting on.
		 */
		uint32_t my_groups_done = (uint32_t)p_args->group_mask & data->pending_group_mask;

		if (my_groups_done == 0U) {
			return;
		}

		data->completed_group_mask |= my_groups_done;

		if ((data->completed_group_mask & data->pending_group_mask) !=
		    data->pending_group_mask) {
			/* More scan-groups in this sequence still need to run.
			 * Kick the next one from the workqueue (not this ISR)
			 * so FSP can finish clearing IRQ state first.
			 */
			k_work_submit(&data->start_work);
			return;
		}

		if (adc_ra_adc_b_drain_results(dev) < 0) {
			adc_context_complete(&data->ctx, -EIO);
			return;
		}

		adc_context_on_sampling_done(&data->ctx, dev);
		break;
	}

	case ADC_EVENT_CALIBRATION_COMPLETE:
		k_sem_give(&data->calibrate_sem);
		break;

	case ADC_EVENT_CONVERSION_ERROR:
		LOG_ERR("conversion error event");
		adc_context_complete(&data->ctx, -EIO);
		break;

	default:
		LOG_WRN("unhandled ADC event: %d", p_args->event);
		break;
	}
}

#define EVENT_ADC_SCAN_END(grp)  BSP_PRV_IELS_ENUM(CONCAT(EVENT_ADC_ADI, grp))
#define EVENT_ADC_CAL_END(unit)  BSP_PRV_IELS_ENUM(CONCAT(EVENT_ADC_CALEND, unit))
#define EVENT_ADC_CONV_ERR(unit) BSP_PRV_IELS_ENUM(CONCAT(EVENT_ADC_ERR, unit))

/* Per-scan-group node: program IELSR, IRQ_CONNECT, and enable scanend IRQ.
 * Called once from the global init below for every status="okay"
 * scan-group node in the system.
 */

#define ADC_RA_UNIT_CALEND_WIRE(node_id)                                                           \
	R_ICU->IELSR[DT_IRQ_BY_NAME(node_id, calend, irq)] =                                       \
		EVENT_ADC_CAL_END(DT_PROP(node_id, unit_id));                                      \
	IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, calend, irq),                                          \
		    DT_IRQ_BY_NAME(node_id, calend, priority),                                     \
		    ADC_B_CAL_END_ISR(DT_PROP(node_id, unit_id)), NULL, 0);                        \
	irq_enable(DT_IRQ_BY_NAME(node_id, calend, irq));

#define ADC_RA_SG_IRQ_WIRE_NODE(node_id)                                                           \
	R_ICU->IELSR[DT_IRQ_BY_NAME(node_id, scanend, irq)] =                                      \
		EVENT_ADC_SCAN_END(DT_REG_ADDR_RAW(node_id));                                      \
	IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, scanend, irq),                                         \
		    DT_IRQ_BY_NAME(node_id, scanend, priority),                                    \
		    ADC_B_SCAN_END_ISR(DT_REG_ADDR_RAW(node_id)), NULL, 0);                        \
	irq_enable(DT_IRQ_BY_NAME(node_id, scanend, irq));

static int adc_ra_adc_b_global_init(void)
{

	IF_ENABLED(DT_NODE_EXISTS(ADC0_NODE), (ADC_RA_UNIT_CALEND_WIRE(ADC0_NODE)))
	IF_ENABLED(DT_NODE_EXISTS(ADC1_NODE), (ADC_RA_UNIT_CALEND_WIRE(ADC1_NODE)))
	DT_FOREACH_STATUS_OKAY(renesas_ra_adc_b_scan_group, ADC_RA_SG_IRQ_WIRE_NODE)

	return 0;
}
SYS_INIT(adc_ra_adc_b_global_init, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY);

/* IRQ CONFIGURE FUNCTION */
#define IRQ_CONFIGURE_FUNC(idx)                                                                    \
	static void adc_ra_configure_func_##idx(void)                                              \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(idx, convserr, irq)] =                            \
			EVENT_ADC_CONV_ERR(DT_INST_PROP(idx, unit_id));                            \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, convserr, irq),                               \
			    DT_INST_IRQ_BY_NAME(idx, convserr, priority),                          \
			    ADC_B_ERR_ISR(DT_INST_PROP(idx, unit_id)), NULL, 0);                   \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, convserr, irq));                               \
	}

#define IRQ_CONFIGURE_DEFINE(idx) .irq_configure = adc_ra_configure_func_##idx

#define ADC_RA_ADC_B_DT_CH_INIT(node_id)                                                           \
	{                                                                                          \
		.vc_id = DT_REG_ADDR(node_id),                                                     \
		.input_positive = DT_PROP(node_id, zephyr_input_positive),                         \
		.resolution = DT_PROP_OR(node_id, zephyr_resolution, 12),                          \
		.scan_group_id = DT_REG_ADDR_RAW(DT_PHANDLE(node_id, renesas_scan_group)),         \
		.oversampling = DT_PROP_OR(node_id, zephyr_oversampling, 0),                       \
		.acq_time = DT_PROP_OR(node_id, zephyr_acquisition_time, ADC_ACQ_TIME_DEFAULT),    \
	},

/* Compile-time checks per channel node:
 *  - The converter assigned to this channel must equal the unit-id
 *    of the parent adc-unit.
 *  - Physical channel must be a bit set in the parent adc-unit's channel-available-mask.
 */
#define ADC_RA_VALIDATE_CH(node_id)                                                                \
	BUILD_ASSERT(                                                                              \
		DT_PROP(DT_PHANDLE(node_id, renesas_scan_group), renesas_converter_unit_id) ==     \
			DT_PROP(DT_PARENT(node_id), unit_id),                                      \
		"channel '" DT_NODE_FULL_NAME(node_id) "': scan-group's "                          \
						       "renesas,converter-unit-id must equal the " \
						       "parent adc-unit's unit-id");               \
	BUILD_ASSERT(((1U << DT_PROP(node_id, zephyr_input_positive)) &                            \
		      DT_PROP(DT_PARENT(node_id), channel_available_mask)) != 0U,                  \
		     "channel '" DT_NODE_FULL_NAME(                                                \
			     node_id) "': zephyr,input-positive "                                  \
				      "is not in the parent adc-unit's channel-available-mask");

#define ADC_RA_SG_ISR_CFG_FIELD_NODE(node_id)                                                      \
	.ISR_CFG_SCANEND_IRQ(DT_REG_ADDR_RAW(node_id)) =                                           \
		(IRQn_Type)DT_IRQ_BY_NAME(node_id, scanend, irq),                                  \
	.ISR_CFG_SCANEND_IPL(DT_REG_ADDR_RAW(node_id)) =                                           \
		DT_IRQ_BY_NAME(node_id, scanend, priority),

#define ADC_RA_ADC_B_INIT(idx)                                                                     \
	IRQ_CONFIGURE_FUNC(idx)                                                                    \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, ADC_RA_VALIDATE_CH)                                 \
	static const struct adc_ra_adc_b_channel_cfg adc_ra_adc_b_channels_##idx[] = {             \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, ADC_RA_ADC_B_DT_CH_INIT)};                  \
	static adc_b_isr_cfg_t g_adc_isr_cfg_##idx = {                                             \
		DT_FOREACH_STATUS_OKAY(renesas_ra_adc_b_scan_group, ADC_RA_SG_ISR_CFG_FIELD_NODE)  \
			.calibration_end_ipl_adc_0 = DT_IRQ_BY_NAME(ADC0_NODE, calend, priority),  \
		.calibration_end_ipl_adc_1 = DT_IRQ_BY_NAME(ADC1_NODE, calend, priority),          \
		.calibration_end_irq_adc_0 = DT_IRQ_BY_NAME(ADC0_NODE, calend, irq),               \
		.calibration_end_irq_adc_1 = DT_IRQ_BY_NAME(ADC1_NODE, calend, irq),               \
		.ISR_CFG_CONVSERR_IRQ(DT_INST_PROP(idx, unit_id)) =                                \
			(IRQn_Type)DT_INST_IRQ_BY_NAME(idx, convserr, irq),                        \
		.ISR_CFG_CONVSERR_IPL(DT_INST_PROP(idx, unit_id)) =                                \
			DT_INST_IRQ_BY_NAME(idx, convserr, priority),                              \
	};                                                                                         \
	static DEVICE_API(adc, adc_ra_adc_b_api_##idx) = {                                         \
		.channel_setup = adc_ra_adc_b_channel_setup,                                       \
		.read = adc_ra_adc_b_read,                                                         \
		.ref_internal = DT_INST_PROP_OR(idx, vref_mv, 3300),                               \
		IF_ENABLED(CONFIG_ADC_ASYNC,                                                       \
			(.read_async = adc_ra_adc_b_read_async))};               \
	static const struct adc_ra_adc_b_config adc_ra_adc_b_config_##idx = {                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(idx))),                   \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(idx), 0,    \
									mstp),                     \
				.stop_bit = (uint32_t)DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(idx),   \
									    0, stop_bit),          \
			},                                                                         \
		.clock_div = DT_PROP_OR(DT_INST_PARENT(idx), clock_div, 3),                        \
		.unit_id = DT_INST_PROP(idx, unit_id),                                             \
		.channel_available_mask = DT_INST_PROP_OR(idx, channel_available_mask, 0),         \
		.dt_channels = adc_ra_adc_b_channels_##idx,                                        \
		.dt_channel_count = ARRAY_SIZE(adc_ra_adc_b_channels_##idx),                       \
		IRQ_CONFIGURE_DEFINE(idx),                                                         \
	};                                                                                         \
                                                                                                   \
	static adc_b_extended_cfg_t g_adc_cfg_extend_##idx = {                                     \
		.sync_operation_control = 0,                                                       \
		.adc_b_converter_mode[DT_INST_PROP(idx, unit_id)] =                                \
			{                                                                          \
				.mode = (ADC_B_CONVERTER_MODE_SINGLE_SCAN),                        \
				.method = ADC_B_DT_METHOD(idx),                                    \
			},                                                                         \
		.fifo_enable_mask = 0,                                                             \
		.fifo_interrupt_enable_mask = 0,                                                   \
		.start_trigger_delay_table = {0},                                                  \
		.calibration_adc_state = ((256 << R_ADC_B0_ADCALSTCR_CALADSST_Pos) |               \
					  (20 << R_ADC_B0_ADCALSTCR_CALADCST_Pos)),                \
		.calibration_sample_and_hold = ((95 << R_ADC_B0_ADCALSHCR_CALSHSST_Pos) |          \
						(5 << R_ADC_B0_ADCALSHCR_CALSHHST_Pos)),           \
		.p_isr_cfg = &g_adc_isr_cfg_##idx,                                                 \
		.sampling_state_tables =                                                           \
			{                                                                          \
				0,                                                                 \
			},                                                                         \
		.sample_and_hold_enable_mask = (ADC_B_SAMPLE_AND_HOLD_MASK_NONE),                  \
		.conversion_state =                                                                \
			((20 << R_ADC_B0_ADCNVSTR_CST0_Pos) | (20 << R_ADC_B0_ADCNVSTR_CST1_Pos)), \
		.user_offset_tables = {0},                                                         \
		.user_gain_tables = {0},                                                           \
		.limiter_clip_interrupt_enable_mask = (0x00),                                      \
		.limiter_clip_tables = {0},                                                        \
		.adc_filter_selection =                                                            \
			{                                                                          \
				[DT_INST_PROP(idx, unit_id)] = ADC_B_DFSEL_SINC3_ALL,              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct adc_ra_adc_b_data adc_ra_adc_b_data_##idx = {                                \
		ADC_CONTEXT_INIT_TIMER(adc_ra_adc_b_data_##idx, ctx),                              \
		ADC_CONTEXT_INIT_LOCK(adc_ra_adc_b_data_##idx, ctx),                               \
		ADC_CONTEXT_INIT_SYNC(adc_ra_adc_b_data_##idx, ctx),                               \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.adc_cfg =                                                                         \
			{                                                                          \
				.unit = DT_INST_PROP(idx, unit_id),                                \
				.alignment = (adc_alignment_t)ADC_ALIGNMENT_RIGHT,                 \
				.p_callback = renesas_ra_adc_callback,                             \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(idx)),              \
				.p_extend = &g_adc_cfg_extend_##idx,                               \
			},                                                                         \
		.buf = NULL,                                                                       \
		.buf_id = 0,                                                                       \
		.channels = 0,                                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, adc_ra_adc_b_init, NULL, &adc_ra_adc_b_data_##idx,              \
			      &adc_ra_adc_b_config_##idx, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,   \
			      &adc_ra_adc_b_api_##idx)

DT_INST_FOREACH_STATUS_OKAY(ADC_RA_ADC_B_INIT);
