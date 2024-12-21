/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/util_macro.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#define LOG_DOMAIN dai_intel_ssp
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include "ssp.h"


#define dai_set_drvdata(dai, data) (dai->priv_data = data)
#define dai_get_drvdata(dai) dai->priv_data
#define dai_get_plat_data(dai) dai->ssp_plat_data
#define dai_get_mn(dai) dai->ssp_plat_data->mn_inst
#define dai_get_ftable(dai) dai->ssp_plat_data->ftable
#define dai_get_fsources(dai) dai->ssp_plat_data->fsources
#define dai_mn_base(dai) dai->ssp_plat_data->mn_inst->base
#define dai_base(dai) dai->ssp_plat_data->base
#define dai_ip_base(dai) dai->ssp_plat_data->ip_base
#define dai_shim_base(dai) dai->ssp_plat_data->shim_base
#define dai_hdamlssp_base(dai) dai->ssp_plat_data->hdamlssp_base
#define dai_i2svss_base(dai) dai->ssp_plat_data->i2svss_base

#define DAI_DIR_PLAYBACK 0
#define DAI_DIR_CAPTURE 1
#define SSP_ARRAY_INDEX(dir) dir == DAI_DIR_RX ? DAI_DIR_CAPTURE : DAI_DIR_PLAYBACK

static const char irq_name_level5_z[] = "level5";

static struct dai_intel_ssp_freq_table ssp_freq_table[] = {
	{ DT_PROP(DT_NODELABEL(audioclk), clock_frequency),
	  DT_PROP(DT_NODELABEL(audioclk), clock_frequency) / 1000},
	{ DT_PROP(DT_NODELABEL(sysclk), clock_frequency),
	  DT_PROP(DT_NODELABEL(sysclk), clock_frequency) / 1000},
	{ DT_PROP(DT_NODELABEL(pllclk), clock_frequency),
	  DT_PROP(DT_NODELABEL(pllclk), clock_frequency) / 1000},
};

static uint32_t ssp_freq_sources[] = {
	DAI_INTEL_SSP_CLOCK_AUDIO_CARDINAL,
	DAI_INTEL_SSP_CLOCK_XTAL_OSCILLATOR,
	DAI_INTEL_SSP_CLOCK_PLL_FIXED,
};

static struct dai_intel_ssp_mn ssp_mn_divider = {
	.base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(ssp0), 1),
};


#define INTEL_SSP_INST_DEFINE(node_id) {				\
		.is_initialized = false,				\
		.is_power_en = false,					\
		.acquire_count = 0,					\
		.ssp_index = DT_PROP(node_id, ssp_index),		\
		.base =	DT_REG_ADDR_BY_IDX(node_id, 0),			\
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(sspbase)),	\
		(.ip_base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(sspbase), 0),))	\
		.shim_base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(shim), 0),		\
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(hdamlssp)),		\
			(.hdamlssp_base = DT_REG_ADDR(DT_NODELABEL(hdamlssp)),))\
		IF_ENABLED(DT_PROP_HAS_IDX(node_id, i2svss, 0),			\
			(.i2svss_base = DT_PROP_BY_IDX(node_id, i2svss, 0),))	\
		.irq = DT_NUM_IRQS(node_id),				\
		.irq_name = irq_name_level5_z,				\
		.fifo[DAI_DIR_PLAYBACK].offset =			\
			DT_REG_ADDR_BY_IDX(node_id, 0) + OUT_FIFO,		\
		.fifo[DAI_DIR_PLAYBACK].handshake =			\
			DT_DMAS_CELL_BY_NAME(node_id, tx, channel),	\
		.fifo[DAI_DIR_CAPTURE].offset =				\
			DT_REG_ADDR_BY_IDX(node_id, 0) + IN_FIFO,		\
		.fifo[DAI_DIR_CAPTURE].handshake =			\
			DT_DMAS_CELL_BY_NAME(node_id, rx, channel),	\
		.mn_inst = &ssp_mn_divider,				\
		.ftable = ssp_freq_table,				\
		.fsources = ssp_freq_sources,				\
		.clk_active = 0,					\
},

static struct dai_intel_ssp_plat_data ssp_plat_data_table[] = {
	DT_FOREACH_STATUS_OKAY(intel_ssp, INTEL_SSP_INST_DEFINE)
};


static uint32_t ssp_get_instance_count(void)
{
	return ARRAY_SIZE(ssp_plat_data_table);
}


static struct dai_intel_ssp_plat_data *ssp_get_device_instance(uint32_t ssp_index)
{
	uint32_t ssp_instance = ssp_get_instance_count();
	uint32_t i;

	for (i = 0; i < ssp_instance; i++) {
		if (ssp_plat_data_table[i].ssp_index == ssp_index) {
			return &ssp_plat_data_table[i];
		}
	}

	return NULL;
}


static void dai_ssp_update_bits(struct dai_intel_ssp *dp, uint32_t reg, uint32_t mask, uint32_t val)
{
	uint32_t dest = dai_base(dp) + reg;

	LOG_DBG("base %x, reg %x, mask %x, value %x", dai_base(dp), reg, mask, val);

	sys_write32((sys_read32(dest) & (~mask)) | (val & mask), dest);
}

#if SSP_IP_VER == SSP_IP_VER_1_0
static int dai_ssp_gcd(int a, int b)
{
	int aux;
	int k;

	if (a == 0) {
		return b;
	}

	if (b == 0) {
		return a;
	}

	/* If the numbers are negative, convert them to positive numbers
	 * gcd(a, b) = gcd(-a, -b) = gcd(-a, b) = gcd(a, -b)
	 */
	if (a < 0) {
		a = -a;
	}

	if (b < 0) {
		b = -b;
	}

	/* Find the greatest power of 2 that devides both a and b */
	for (k = 0; ((a | b) & 1) == 0; k++) {
		a >>= 1;
		b >>= 1;
	}

	/* divide by 2 until a becomes odd */
	while ((a & 1) == 0) {
		a >>= 1;
	}

	do {
		/*if b is even, remove all factors of 2*/
		while ((b & 1) == 0) {
			b >>= 1;
		}

		/* both a and b are odd now. Swap so a <= b
		 * then set b = b - a, which is also even
		 */
		if (a > b) {
			aux = a;
			a = b;
			b = aux;
		}

		b = b - a;

	} while (b != 0);

	/* restore common factors of 2 */
	return a << k;
}
#endif

/**
 * \brief Checks if given clock is used as source for any MCLK.
 *
 * \return true if any port use given clock source, false otherwise.
 */
static bool dai_ssp_is_mclk_source_in_use(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	bool ret = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(mp->mclk_sources_ref); i++) {
		if (mp->mclk_sources_ref[i] > 0) {
			ret = true;
			break;
		}
	}

	return ret;
}

/**
 * \brief Configures source clock for MCLK.
 *	  All MCLKs share the same source, so it should be changed
 *	  only if there are no other ports using it already.
 * \param[in] mclk_rate main clock frequency.
 * \return 0 on success, error code otherwise.
 */
static int dai_ssp_setup_initial_mclk_source(struct dai_intel_ssp *dp, uint32_t mclk_id,
					     uint32_t mclk_rate)
{
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	uint32_t *fs = dai_get_fsources(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	int clk_index = -1;
	uint32_t mdivc;
	int ret = 0;
	int i;

	if (mclk_id >= DAI_INTEL_SSP_NUM_MCLK) {
		LOG_ERR("can't configure MCLK %d, only %d mclk[s] existed!",
			mclk_id, DAI_INTEL_SSP_NUM_MCLK);
		ret = -EINVAL;
		goto out;
	}

	/* searching the smallest possible mclk source */
	for (i = 0; i <= DAI_INTEL_SSP_MAX_FREQ_INDEX; i++) {
		if (ft[i].freq % mclk_rate == 0) {
			clk_index = i;
			break;
		}
	}

	if (clk_index < 0) {
		LOG_ERR("MCLK %d, no valid source", mclk_rate);
		ret = -EINVAL;
		goto out;
	}

	mp->mclk_source_clock = clk_index;

	mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

	/* enable MCLK divider */
	mdivc |= MN_MDIVCTRL_M_DIV_ENABLE(mclk_id);

	/* clear source mclk clock - bits 17-16 */
	mdivc &= ~MCDSS(MN_SOURCE_CLKS_MASK);

	/* select source clock */
	mdivc |= MCDSS(fs[clk_index]);

	sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);

	mp->mclk_sources_ref[mclk_id]++;
out:

	return ret;
}

/**
 * \brief Checks if requested MCLK can be achieved with current source.
 * \param[in] mclk_rate main clock frequency.
 * \return 0 on success, error code otherwise.
 */
static int dai_ssp_check_current_mclk_source(struct dai_intel_ssp *dp, uint16_t mclk_id,
					     uint32_t mclk_rate)
{
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	uint32_t mdivc;
	int ret = 0;

	LOG_INF("MCLK %d, source = %d", mclk_rate, mp->mclk_source_clock);

	if (ft[mp->mclk_source_clock].freq % mclk_rate != 0) {
		LOG_ERR("MCLK %d, no valid configuration for already selected source = %d",
			mclk_rate, mp->mclk_source_clock);
		ret = -EINVAL;
	}

	/* if the mclk is already used, can't change its divider, just increase ref count */
	if (mp->mclk_sources_ref[mclk_id] > 0) {
		if (mp->mclk_rate[mclk_id] != mclk_rate) {
			LOG_ERR("Can't set MCLK %d to %d, it is already configured to %d",
				mclk_id, mclk_rate, mp->mclk_rate[mclk_id]);
			return -EINVAL;
		}

		mp->mclk_sources_ref[mclk_id]++;
	} else {
		mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

		/* enable MCLK divider */
		mdivc |= MN_MDIVCTRL_M_DIV_ENABLE(mclk_id);
		sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);

		mp->mclk_sources_ref[mclk_id]++;
	}

	return ret;
}

/**
 * \brief Sets MCLK divider to given value.
 * \param[in] mclk_id ID of MCLK.
 * \param[in] mdivr_val divider value.
 * \return 0 on success, error code otherwise.
 */
static int dai_ssp_set_mclk_divider(struct dai_intel_ssp *dp, uint16_t mclk_id, uint32_t mdivr_val)
{
	uint32_t mdivr;

	LOG_INF("mclk_id %d mdivr_val %d", mclk_id, mdivr_val);
	switch (mdivr_val) {
	case 1:
		mdivr = 0x00000fff; /* bypass divider for MCLK */
		break;
	case 2 ... 8:
		mdivr = mdivr_val - 2; /* 1/n */
		break;
	default:
		LOG_ERR("invalid mdivr_val %d", mdivr_val);
		return -EINVAL;
	}

	sys_write32(mdivr, dai_mn_base(dp) + MN_MDIVR(mclk_id));

	return 0;
}

static int dai_ssp_mn_set_mclk(struct dai_intel_ssp *dp, uint16_t mclk_id, uint32_t mclk_rate)
{
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	k_spinlock_key_t key;
	int ret = 0;

	if (mclk_id >= DAI_INTEL_SSP_NUM_MCLK) {
		LOG_ERR("mclk ID (%d) >= %d", mclk_id, DAI_INTEL_SSP_NUM_MCLK);
		return -EINVAL;
	}

	key = k_spin_lock(&mp->lock);

	if (dai_ssp_is_mclk_source_in_use(dp)) {
		ret = dai_ssp_check_current_mclk_source(dp, mclk_id, mclk_rate);
	} else {
		ret = dai_ssp_setup_initial_mclk_source(dp, mclk_id, mclk_rate);
	}

	if (ret < 0) {
		goto out;
	}

	LOG_INF("mclk_rate %d, mclk_source_clock %d", mclk_rate, mp->mclk_source_clock);

	ret = dai_ssp_set_mclk_divider(dp, mclk_id, ft[mp->mclk_source_clock].freq / mclk_rate);
	if (!ret) {
		mp->mclk_rate[mclk_id] = mclk_rate;
	}

out:
	k_spin_unlock(&mp->lock, key);

	return ret;
}

static int dai_ssp_mn_set_mclk_blob(struct dai_intel_ssp *dp, uint32_t mdivc, uint32_t mdivr)
{
	sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);
	sys_write32(mdivr, dai_mn_base(dp) + MN_MDIVR(0));

	return 0;
}

static void dai_ssp_mn_release_mclk(struct dai_intel_ssp *dp, uint32_t mclk_id)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	k_spinlock_key_t key;
	uint32_t mdivc;

	key = k_spin_lock(&mp->lock);

	mp->mclk_sources_ref[mclk_id]--;

	/* disable MCLK divider if nobody use it */
	if (!mp->mclk_sources_ref[mclk_id]) {
		mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

		mdivc &= ~MN_MDIVCTRL_M_DIV_ENABLE(mclk_id);
		sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);
	}

	/* release the clock source if all mclks are released */
	if (!dai_ssp_is_mclk_source_in_use(dp)) {
		mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

		/* clear source mclk clock - bits 17-16 */
		mdivc &= ~MCDSS(MN_SOURCE_CLKS_MASK);

		sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);

		mp->mclk_source_clock = 0;
	}
	k_spin_unlock(&mp->lock, key);
}

#if SSP_IP_VER == SSP_IP_VER_1_0
/**
 * \brief Finds valid M/(N * SCR) values for given frequencies.
 * \param[in] freq SSP clock frequency.
 * \param[in] bclk Bit clock frequency.
 * \param[out] out_scr_div SCR divisor.
 * \param[out] out_m M value of M/N divider.
 * \param[out] out_n N value of M/N divider.
 * \return true if found suitable values, false otherwise.
 */
static bool dai_ssp_find_mn(uint32_t freq, uint32_t bclk, uint32_t *out_scr_div, uint32_t *out_m,
			    uint32_t *out_n)
{
	uint32_t m, n, mn_div;
	uint32_t scr_div = freq / bclk;

	LOG_INF("for freq %d bclk %d", freq, bclk);
	/* check if just SCR is enough */
	if (freq % bclk == 0 && scr_div < (SSCR0_SCR_MASK >> 8) + 1) {
		*out_scr_div = scr_div;
		*out_m = 1;
		*out_n = 1;

		return true;
	}

	/* M/(N * scr_div) has to be less than 1/2 */
	if ((bclk * 2) >= freq) {
		return false;
	}

	/* odd SCR gives lower duty cycle */
	if (scr_div > 1 && scr_div % 2 != 0) {
		--scr_div;
	}

	/* clamp to valid SCR range */
	scr_div = MIN(scr_div, (SSCR0_SCR_MASK >> 8) + 1);

	/* find highest even divisor */
	while (scr_div > 1 && freq % scr_div != 0) {
		scr_div -= 2;
	}

	/* compute M/N with smallest dividend and divisor */
	mn_div = dai_ssp_gcd(bclk, freq / scr_div);

	m = bclk / mn_div;
	n = freq / scr_div / mn_div;

	/* M/N values can be up to 24 bits */
	if (n & (~0xffffff)) {
		return false;
	}

	*out_scr_div = scr_div;
	*out_m = m;
	*out_n = n;

	LOG_INF("m %d n %d", m, n);
	return true;
}

/**
 * \brief Finds index of clock valid for given BCLK rate.
 *	  Clock that can use just SCR is preferred.
 *	  M/N other than 1/1 is used only if there are no other possibilities.
 * \param[in] bclk Bit clock frequency.
 * \param[out] scr_div SCR divisor.
 * \param[out] m M value of M/N divider.
 * \param[out] n N value of M/N divider.
 * \return index of suitable clock if could find it, -EINVAL otherwise.
 */
static int dai_ssp_find_bclk_source(struct dai_intel_ssp *dp, uint32_t bclk, uint32_t *scr_div,
				    uint32_t *m, uint32_t *n)
{
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	int i;

	/* check if we can use MCLK source clock */
	if (dai_ssp_is_mclk_source_in_use(dp)) {
		if (dai_ssp_find_mn(ft[mp->mclk_source_clock].freq, bclk, scr_div, m, n)) {
			return mp->mclk_source_clock;
		}

		LOG_WRN("BCLK %d warning: cannot use MCLK source %d", bclk,
			ft[mp->mclk_source_clock].freq);
	}

	/* searching the smallest possible bclk source */
	for (i = 0; i <= DAI_INTEL_SSP_MAX_FREQ_INDEX; i++) {
		if (ft[i].freq % bclk == 0) {
			*scr_div = ft[i].freq / bclk;
			return i;
		}
	}

	/* check if we can get target BCLK with M/N */
	for (i = 0; i <= DAI_INTEL_SSP_MAX_FREQ_INDEX; i++) {
		if (dai_ssp_find_mn(ft[i].freq, bclk, scr_div, m, n)) {
			return i;
		}
	}

	return -EINVAL;
}

/**
 * \brief Finds index of SSP clock with the given clock source encoded index.
 * \return the index in ssp_freq if could find it, -EINVAL otherwise.
 */
static int dai_ssp_find_clk_ssp_index(struct dai_intel_ssp *dp, uint32_t src_enc)
{
	uint32_t *fs = dai_get_fsources(dp);
	int i;

	/* searching for the encode value matched bclk source */
	for (i = 0; i <= DAI_INTEL_SSP_MAX_FREQ_INDEX; i++) {
		if (fs[i] == src_enc) {
			return i;
		}
	}

	return -EINVAL;
}

/**
 * \brief Checks if given clock is used as source for any BCLK.
 * \param[in] clk_src Bit clock source.
 * \return true if any port use given clock source, false otherwise.
 */
static bool dai_ssp_is_bclk_source_in_use(struct dai_intel_ssp *dp, enum bclk_source clk_src)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	bool ret = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(mp->bclk_sources); i++) {
		if (mp->bclk_sources[i] == clk_src) {
			ret = true;
			break;
		}
	}

	return ret;
}

/**
 * \brief Configures M/N source clock for BCLK.
 *	  All ports that use M/N share the same source, so it should be changed
 *	  only if there are no other ports using M/N already.
 * \param[in] bclk Bit clock frequency.
 * \param[out] scr_div SCR divisor.
 * \param[out] m M value of M/N divider.
 * \param[out] n N value of M/N divider.
 * \return 0 on success, error code otherwise.
 */
static int dai_ssp_setup_initial_bclk_mn_source(struct dai_intel_ssp *dp, uint32_t bclk,
						uint32_t *scr_div, uint32_t *m, uint32_t *n)
{
	uint32_t *fs = dai_get_fsources(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	uint32_t mdivc;
	int clk_index = dai_ssp_find_bclk_source(dp, bclk, scr_div, m, n);

	if (clk_index < 0) {
		LOG_ERR("BCLK %d, no valid source", bclk);
		return -EINVAL;
	}

	mp->bclk_source_mn_clock = clk_index;

	mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

	/* clear source bclk clock - 21-20 bits */
	mdivc &= ~MNDSS(MN_SOURCE_CLKS_MASK);

	/* select source clock */
	mdivc |= MNDSS(fs[clk_index]);

	sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);

	return 0;
}

/**
 * \brief Reset M/N source clock for BCLK.
 *	  If no port is using bclk, reset to use SSP_CLOCK_XTAL_OSCILLATOR
 *	  as the default clock source.
 */
static void dai_ssp_reset_bclk_mn_source(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	uint32_t mdivc;
	int clk_index = dai_ssp_find_clk_ssp_index(dp, DAI_INTEL_SSP_CLOCK_XTAL_OSCILLATOR);

	if (clk_index < 0) {
		LOG_ERR("BCLK reset failed, no SSP_CLOCK_XTAL_OSCILLATOR source!");
		return;
	}

	mdivc = sys_read32(dai_mn_base(dp) + MN_MDIVCTRL);

	/* reset to use XTAL Oscillator */
	mdivc &= ~MNDSS(MN_SOURCE_CLKS_MASK);
	mdivc |= MNDSS(DAI_INTEL_SSP_CLOCK_XTAL_OSCILLATOR);

	sys_write32(mdivc, dai_mn_base(dp) + MN_MDIVCTRL);

	mp->bclk_source_mn_clock = clk_index;
}

/**
 * \brief Finds valid M/(N * SCR) values for source clock that is already locked
 *	  because other ports use it.
 * \param[in] bclk Bit clock frequency.
 * \param[out] scr_div SCR divisor.
 * \param[out] m M value of M/N divider.
 * \param[out] n N value of M/N divider.
 * \return 0 on success, error code otherwise.
 */
static int dai_ssp_setup_current_bclk_mn_source(struct dai_intel_ssp *dp, uint32_t bclk,
						uint32_t *scr_div, uint32_t *m, uint32_t *n)
{
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	int ret = 0;

	/* source for M/N is already set, no need to do it */
	if (dai_ssp_find_mn(ft[mp->bclk_source_mn_clock].freq, bclk, scr_div, m, n)) {
		goto out;
	}

	LOG_ERR("BCLK %d, no valid configuration for already selected source = %d",
		bclk, mp->bclk_source_mn_clock);
	ret = -EINVAL;

out:

	return ret;
}

static bool dai_ssp_check_bclk_xtal_source(uint32_t bclk, bool mn_in_use,
					   uint32_t *scr_div)
{
	/* since cAVS 2.0 bypassing XTAL (ECS=0) is not supported */
	return false;
}

static int dai_ssp_mn_set_bclk(struct dai_intel_ssp *dp, uint32_t dai_index, uint32_t bclk_rate,
			       uint32_t *out_scr_div, bool *out_need_ecs)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	k_spinlock_key_t key;
	uint32_t m = 1;
	uint32_t n = 1;
	int ret = 0;
	bool mn_in_use;

	key = k_spin_lock(&mp->lock);

	mp->bclk_sources[dai_index] = MN_BCLK_SOURCE_NONE;

	mn_in_use = dai_ssp_is_bclk_source_in_use(dp, MN_BCLK_SOURCE_MN);

	if (dai_ssp_check_bclk_xtal_source(bclk_rate, mn_in_use, out_scr_div)) {
		mp->bclk_sources[dai_index] = MN_BCLK_SOURCE_XTAL;
		*out_need_ecs = false;
		goto out;
	}

	*out_need_ecs = true;

	if (mn_in_use) {
		ret = dai_ssp_setup_current_bclk_mn_source(dp, bclk_rate, out_scr_div, &m, &n);
	} else {
		ret = dai_ssp_setup_initial_bclk_mn_source(dp, bclk_rate, out_scr_div, &m, &n);
	}

	if (ret >= 0) {
		mp->bclk_sources[dai_index] = MN_BCLK_SOURCE_MN;

		LOG_INF("bclk_rate %d, *out_scr_div %d, m %d, n %d", bclk_rate, *out_scr_div, m, n);

		sys_write32(m, dai_mn_base(dp) + MN_MDIV_M_VAL(dai_index));
		sys_write32(n, dai_mn_base(dp) + MN_MDIV_N_VAL(dai_index));
	}

out:

	k_spin_unlock(&mp->lock, key);

	return ret;
}

static void dai_ssp_mn_release_bclk(struct dai_intel_ssp *dp, uint32_t ssp_index)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	k_spinlock_key_t key;
	bool mn_in_use;

	key = k_spin_lock(&mp->lock);
	mp->bclk_sources[ssp_index] = MN_BCLK_SOURCE_NONE;

	mn_in_use = dai_ssp_is_bclk_source_in_use(dp, MN_BCLK_SOURCE_MN);
	/* release the M/N clock source if not used */
	if (!mn_in_use) {
		dai_ssp_reset_bclk_mn_source(dp);
	}

	k_spin_unlock(&mp->lock, key);
}

static void dai_ssp_mn_reset_bclk_divider(struct dai_intel_ssp *dp, uint32_t ssp_index)
{
	struct dai_intel_ssp_mn *mp = dai_get_mn(dp);
	k_spinlock_key_t key;

	key = k_spin_lock(&mp->lock);

	sys_write32(1, dai_mn_base(dp) + MN_MDIV_M_VAL(ssp_index));
	sys_write32(1, dai_mn_base(dp) + MN_MDIV_N_VAL(ssp_index));

	k_spin_unlock(&mp->lock, key);
}
#endif

static int dai_ssp_poll_for_register_delay(uint32_t reg, uint32_t mask,
					   uint32_t val, uint64_t us)
{
	if (!WAIT_FOR((sys_read32(reg) & mask) == val, us, k_busy_wait(1))) {
		LOG_ERR("poll timeout reg %u mask %u val %u us %u", reg, mask, val, (uint32_t)us);
		return -EIO;
	}

	return 0;
}

static inline void dai_ssp_pm_runtime_dis_ssp_clk_gating(struct dai_intel_ssp *dp,
						uint32_t ssp_index)
{
#if CONFIG_DAI_SSP_CLK_FORCE_DYNAMIC_CLOCK_GATING
	uint32_t shim_reg;

	shim_reg = sys_read32(dai_shim_base(dp) + SHIM_CLKCTL) |
		(ssp_index < CONFIG_DAI_INTEL_SSP_NUM_BASE ?
			SHIM_CLKCTL_I2SFDCGB(ssp_index) :
			SHIM_CLKCTL_I2SEFDCGB(ssp_index -
					      CONFIG_DAI_INTEL_SSP_NUM_BASE));

	sys_write32(shim_reg, dai_shim_base(dp) + SHIM_CLKCTL);

	LOG_INF("ssp_index %d CLKCTL %08x", ssp_index, shim_reg);
#endif
}

static inline void dai_ssp_pm_runtime_en_ssp_clk_gating(struct dai_intel_ssp *dp,
						uint32_t ssp_index)
{
#if CONFIG_DAI_SSP_CLK_FORCE_DYNAMIC_CLOCK_GATING
	uint32_t shim_reg;

	shim_reg = sys_read32(dai_shim_base(dp) + SHIM_CLKCTL) &
		~(ssp_index < CONFIG_DAI_INTEL_SSP_NUM_BASE ?
			SHIM_CLKCTL_I2SFDCGB(ssp_index) :
			SHIM_CLKCTL_I2SEFDCGB(ssp_index -
					      CONFIG_DAI_INTEL_SSP_NUM_BASE));

	sys_write32(shim_reg, dai_shim_base(dp) + SHIM_CLKCTL);

	LOG_INF("ssp_index %d CLKCTL %08x", ssp_index, shim_reg);
#endif
}

static void dai_ssp_pm_runtime_en_ssp_power(struct dai_intel_ssp *dp, uint32_t ssp_index)
{
#if CONFIG_DAI_SSP_HAS_POWER_CONTROL
	int ret;

	LOG_INF("SSP%d", ssp_index);
#if SSP_IP_VER < SSP_IP_VER_2_0
	sys_write32(sys_read32(dai_ip_base(dp) + I2SLCTL_OFFSET) | I2SLCTL_SPA(ssp_index),
		    dai_ip_base(dp) + I2SLCTL_OFFSET);

	/* Check if powered on. */
	ret = dai_ssp_poll_for_register_delay(dai_ip_base(dp) + I2SLCTL_OFFSET,
					      I2SLCTL_CPA(ssp_index), I2SLCTL_CPA(ssp_index),
					      DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);
#elif SSP_IP_VER > SSP_IP_VER_1_5
	sys_write32(sys_read32(dai_hdamlssp_base(dp) + I2SLCTL_OFFSET) |
			       I2SLCTL_SPA(ssp_index),
			       dai_hdamlssp_base(dp) + I2SLCTL_OFFSET);
	/* Check if powered on. */
	ret = dai_ssp_poll_for_register_delay(dai_hdamlssp_base(dp) + I2SLCTL_OFFSET,
					      I2SLCTL_CPA(ssp_index), I2SLCTL_CPA(ssp_index),
					      DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);
#else
#error need to define SOC
#endif
	if (ret) {
		LOG_WRN("SSP%d: timeout", ssp_index);
	}
#else
	ARG_UNUSED(dp);
	ARG_UNUSED(ssp_index);
#endif /* CONFIG_DAI_SSP_HAS_POWER_CONTROL */
}

static void dai_ssp_pm_runtime_dis_ssp_power(struct dai_intel_ssp *dp, uint32_t ssp_index)
{
#if CONFIG_DAI_SSP_HAS_POWER_CONTROL
	int ret;

	LOG_INF("SSP%d", ssp_index);
#if SSP_IP_VER < SSP_IP_VER_2_0
	sys_write32(sys_read32(dai_ip_base(dp) + I2SLCTL_OFFSET) & (~I2SLCTL_SPA(ssp_index)),
		    dai_ip_base(dp) + I2SLCTL_OFFSET);

	/* Check if powered off. */
	ret = dai_ssp_poll_for_register_delay(dai_ip_base(dp) + I2SLCTL_OFFSET,
					      I2SLCTL_CPA(ssp_index), 0,
					      DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);

#elif SSP_IP_VER > SSP_IP_VER_1_5
	sys_write32(sys_read32(dai_hdamlssp_base(dp) + I2SLCTL_OFFSET) & (~I2SLCTL_SPA(ssp_index)),
			dai_hdamlssp_base(dp) + I2SLCTL_OFFSET);

	/* Check if powered off. */
	ret = dai_ssp_poll_for_register_delay(dai_hdamlssp_base(dp) + I2SLCTL_OFFSET,
					      I2SLCTL_CPA(ssp_index), 0,
					      DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);
#else
#error need to define SOC
#endif
	if (ret) {
		LOG_WRN("SSP%d: timeout", ssp_index);
	}
#else
	ARG_UNUSED(dp);
	ARG_UNUSED(ssp_index);
#endif /* CONFIG_DAI_SSP_HAS_POWER_CONTROL */
}

static void dai_ssp_program_channel_map(struct dai_intel_ssp *dp,
		const struct dai_config *cfg, uint32_t ssp_index, const void *spec_config)
{
#if defined(CONFIG_SOC_INTEL_ACE20_LNL)
	ARG_UNUSED(spec_config);
	uint16_t pcmsycm = cfg->link_config;
	 /* Set upper slot number from configuration */
	pcmsycm = pcmsycm | (dp->ssp_plat_data->params.tdm_slots - 1) << 4;

	if (DAI_INTEL_SSP_IS_BIT_SET(cfg->link_config, 15)) {
		uint32_t reg_add = dai_ip_base(dp) + 0x1000 * ssp_index + PCMS0CM_OFFSET;
		/* Program HDA output stream parameters */
		sys_write16((pcmsycm & 0xffff), reg_add);
	} else {
		uint32_t reg_add = dai_ip_base(dp) + 0x1000 * ssp_index + PCMS1CM_OFFSET;
		/* Program HDA input stream parameters */
		sys_write16((pcmsycm & 0xffff), reg_add);
	}
#elif SSP_IP_VER > SSP_IP_VER_2_0
	const struct dai_intel_ipc4_ssp_configuration_blob_ver_3_0 *blob30 = spec_config;
	const struct dai_intel_ipc4_ssp_configuration_blob *blob = spec_config;
	uint64_t time_slot_map = 0;
	uint16_t pcmsycm = cfg->link_config;
	uint8_t slot_count = 0;

	if (DAI_INTEL_SSP_IS_BIT_SET(cfg->link_config, 15)) {
		if (blob30->version == SSP_BLOB_VER_3_0) {
			time_slot_map =
				blob30->i2s_ssp_config.ssmidytsa[cfg->tdm_slot_group];
		} else {
			time_slot_map =
				blob->i2s_driver_config.i2s_config.ssmidytsa[cfg->tdm_slot_group];
		}
		slot_count = POPCOUNT(time_slot_map >> 32) + POPCOUNT(time_slot_map & 0xFFFFFFFF);
		pcmsycm = cfg->link_config | (slot_count - 1) << 4;
		uint32_t reg_add = dai_ip_base(dp) + 0x1000 * ssp_index +
			PCMSyCM_OFFSET(cfg->tdm_slot_group);

		/* Program HDA output stream parameters */
		sys_write16((pcmsycm & 0xffff), reg_add);

	} else {
		if (blob30->version == SSP_BLOB_VER_3_0) {
			time_slot_map =
				blob30->i2s_ssp_config.ssmodytsa[cfg->tdm_slot_group];
		} else {
			time_slot_map =
				blob->i2s_driver_config.i2s_config.ssmodytsa[cfg->tdm_slot_group];
		}
		slot_count = POPCOUNT(time_slot_map >> 32) + POPCOUNT(time_slot_map & 0xFFFFFFFF);
		pcmsycm = cfg->link_config | (slot_count - 1) << 4;
		uint32_t reg_add = dai_ip_base(dp) + 0x1000 * ssp_index +
			PCMSyCM_OFFSET(cfg->tdm_slot_group + I2SOPCMC);

		/* Program HDA input stream parameters */
		sys_write16((pcmsycm & 0xffff), reg_add);
	}
#else
	ARG_UNUSED(dp);
	ARG_UNUSED(cfg);
	ARG_UNUSED(ssp_index);
	ARG_UNUSED(spec_config);
#endif /* CONFIG_SOC_INTEL_ACE20_LNL */
}

/* empty SSP transmit FIFO */
static void dai_ssp_empty_tx_fifo(struct dai_intel_ssp *dp)
{
	int ret;
	uint32_t sssr;

	/*
	 * SSSR_TNF is cleared when TX FIFO is empty or full,
	 * so wait for set TNF then for TFL zero - order matter.
	 */
#if SSP_IP_VER > SSP_IP_VER_2_0
	ret = dai_ssp_poll_for_register_delay(dai_base(dp) + SSMODyCS(dp->tdm_slot_group),
							SSMODyCS_TNF, SSMODyCS_TNF,
							DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);

	ret |= dai_ssp_poll_for_register_delay(dai_base(dp) + SSMODyCS(dp->tdm_slot_group),
					       SSMODyCS_TFL, 0,
					       DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE *
					       (DAI_INTEL_SSP_FIFO_DEPTH - 1) / 2);
#else
	ret = dai_ssp_poll_for_register_delay(dai_base(dp) + SSSR, SSSR_TNF, SSSR_TNF,
					      DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE);
	ret |= dai_ssp_poll_for_register_delay(dai_base(dp) + SSCR3, SSCR3_TFL_MASK, 0,
					       DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE *
					       (DAI_INTEL_SSP_FIFO_DEPTH - 1) / 2);
#endif

	if (ret) {
		LOG_WRN("timeout");
	}

	sssr = sys_read32(dai_base(dp) + SSSR);

	/* clear interrupt */
	if (sssr & SSSR_TUR) {
		sys_write32(sssr, dai_base(dp) + SSSR);
	}
}

#if SSP_IP_VER > SSP_IP_VER_2_0
static void ssp_empty_rx_fifo_on_start(struct dai_intel_ssp *dp)
{
	uint32_t retry = DAI_INTEL_SSP_RX_FLUSH_RETRY_MAX;
	uint32_t i, sssr;

	sssr = sys_read32(dai_base(dp) + SSSR);

	if (sssr & SSSR_ROR) {
		/* The RX FIFO is in overflow condition, empty it */
		for (uint32_t idx = 0; idx < I2SIPCMC; ++idx) {
			for (i = 0; i < DAI_INTEL_SSP_FIFO_DEPTH; i++) {
				sys_read32(dai_base(dp) + SSMIDyD(idx));
			}
		}

		/* Clear the overflow status */
		dai_ssp_update_bits(dp, SSSR, SSSR_ROR, SSSR_ROR);
		/* Re-read the SSSR register */
		sssr = sys_read32(dai_base(dp) + SSSR);
	}

	for (uint32_t idx = 0; idx < I2SIPCMC; ++idx) {
		while ((sys_read32(dai_base(dp) + SSMIDyCS(idx)) & SSMIDyCS_RNE) && retry--) {
			uint32_t entries = SSMIDyCS_RFL_VAL(sys_read32(dai_base(dp) +
				SSMIDyCS(idx)));

			/* Empty the RX FIFO (the DMA is not running at this point) */
			for (i = 0; i < entries + 1; i++) {
				sys_read32(dai_base(dp) + SSMIDyD(idx));
			}

			sssr = sys_read32(dai_base(dp) + SSSR);
		}
	}
}

static void ssp_empty_rx_fifo_on_stop(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	uint64_t sample_ticks = ssp_plat_data->params.fsync_rate ?
		1000000 / ssp_plat_data->params.fsync_rate : 0;
	uint32_t retry = DAI_INTEL_SSP_RX_FLUSH_RETRY_MAX;
	uint32_t i, sssr, ssmidycs;
	uint32_t entries[2];

	sssr = sys_read32(dai_base(dp) + SSSR);

	entries[0] = SSMIDyCS_RFL_VAL(sys_read32(dai_base(dp) + SSMIDyCS(dp->tdm_slot_group)));

	while ((sys_read32(dai_base(dp) + SSMIDyCS(dp->tdm_slot_group)) &
		SSMIDyCS_RNE) && retry--) {
		/* Wait one sample time */
		k_busy_wait(sample_ticks);

		entries[1] = SSMIDyCS_RFL_VAL(sys_read32(dai_base(dp) +
					      SSMIDyCS(dp->tdm_slot_group)));
		sssr = sys_read32(dai_base(dp) + SSSR);
		ssmidycs = sys_read32(dai_base(dp) + SSMIDyCS(dp->tdm_slot_group));

		if (entries[0] > entries[1]) {
			/*
			 * The DMA is reading the FIFO, check the status in the
			 * next loop
			 */
			entries[0] = entries[1];
		} else if (!(ssmidycs & SSMIDyCS_RFS)) {
			/*
			 * The DMA request is not asserted, read the FIFO
			 * directly, otherwise let the next loop iteration to
			 * check the status
			 */
			for (i = 0; i < entries[1] + 1; i++) {
				sys_read32(dai_base(dp) + SSMIDyD(dp->tdm_slot_group));
			}
		}
		sssr = sys_read32(dai_base(dp) + SSSR);
	}

	/* Just in case clear the overflow status */
	dai_ssp_update_bits(dp, SSSR, SSSR_ROR, SSSR_ROR);
}

#else
static void ssp_empty_rx_fifo_on_start(struct dai_intel_ssp *dp)
{
	uint32_t retry = DAI_INTEL_SSP_RX_FLUSH_RETRY_MAX;
	uint32_t i, sssr;

	sssr = sys_read32(dai_base(dp) + SSSR);

	if (sssr & SSSR_ROR) {
		/* The RX FIFO is in overflow condition, empty it */
		for (i = 0; i < DAI_INTEL_SSP_FIFO_DEPTH; i++) {
			sys_read32(dai_base(dp) + SSDR);
		}

		/* Clear the overflow status */
		dai_ssp_update_bits(dp, SSSR, SSSR_ROR, SSSR_ROR);
		/* Re-read the SSSR register */
		sssr = sys_read32(dai_base(dp) + SSSR);
	}

	while ((sssr & SSSR_RNE) && retry--) {
		uint32_t entries = SSCR3_RFL_VAL(sys_read32(dai_base(dp) + SSCR3));

		/* Empty the RX FIFO (the DMA is not running at this point) */
		for (i = 0; i < entries + 1; i++) {
			sys_read32(dai_base(dp) + SSDR);
		}

		sssr = sys_read32(dai_base(dp) + SSSR);
	}
}

static void ssp_empty_rx_fifo_on_stop(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	uint64_t sample_ticks = ssp_plat_data->params.fsync_rate ?
		1000000 / ssp_plat_data->params.fsync_rate : 0;
	uint32_t retry = DAI_INTEL_SSP_RX_FLUSH_RETRY_MAX;
	uint32_t entries[2];
	uint32_t i, sssr;

	sssr = sys_read32(dai_base(dp) + SSSR);
	entries[0] = SSCR3_RFL_VAL(sys_read32(dai_base(dp) + SSCR3));

	while ((sssr & SSSR_RNE) && retry--) {
		/* Wait one sample time */
		k_busy_wait(sample_ticks);

		entries[1] = SSCR3_RFL_VAL(sys_read32(dai_base(dp) + SSCR3));
		sssr = sys_read32(dai_base(dp) + SSSR);

		if (entries[0] > entries[1]) {
			/*
			 * The DMA is reading the FIFO, check the status in the
			 * next loop
			 */
			entries[0] = entries[1];
		} else if (!(sssr & SSSR_RFS)) {
			/*
			 * The DMA request is not asserted, read the FIFO
			 * directly, otherwise let the next loop iteration to
			 * check the status
			 */
			for (i = 0; i < entries[1] + 1; i++) {
				sys_read32(dai_base(dp) + SSDR);
			}
		}

		sssr = sys_read32(dai_base(dp) + SSSR);
	}

	/* Just in case clear the overflow status */
	dai_ssp_update_bits(dp, SSSR, SSSR_ROR, SSSR_ROR);
}

#endif

static int dai_ssp_mclk_prepare_enable(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	int ret;

	if (ssp_plat_data->clk_active & SSP_CLK_MCLK_ACTIVE) {
		return 0;
	}

	/* MCLK config */
	ret = dai_ssp_mn_set_mclk(dp, ssp_plat_data->params.mclk_id,
					ssp_plat_data->params.mclk_rate);
	if (ret < 0) {
		LOG_ERR("invalid mclk_rate = %d for mclk_id = %d", ssp_plat_data->params.mclk_rate,
			ssp_plat_data->params.mclk_id);
	} else {
		ssp_plat_data->clk_active |= SSP_CLK_MCLK_ACTIVE;
	}

	return ret;
}

static void dai_ssp_mclk_disable_unprepare(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);

	if (!(ssp_plat_data->clk_active & SSP_CLK_MCLK_ACTIVE)) {
		return;
	}

	dai_ssp_mn_release_mclk(dp, ssp_plat_data->params.mclk_id);

	ssp_plat_data->clk_active &= ~SSP_CLK_MCLK_ACTIVE;
}

static int dai_ssp_bclk_prepare_enable(struct dai_intel_ssp *dp)
{
#if SSP_IP_VER != SSP_IP_VER_1_0
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
#endif
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	uint32_t sscr0;
	uint32_t mdiv;
	int ret = 0;

	if (ssp_plat_data->clk_active & SSP_CLK_BCLK_ACTIVE) {
		return 0;
	}

	sscr0 = sys_read32(dai_base(dp) + SSCR0);

#if SSP_IP_VER == SSP_IP_VER_1_0
	bool need_ecs = false;
	/* BCLK config */
	ret = dai_ssp_mn_set_bclk(dp, dp->dai_index, ssp_plat_data->params.bclk_rate,
				  &mdiv, &need_ecs);
	if (ret < 0) {
		LOG_ERR("invalid bclk_rate = %d for ssp_index = %d",
			ssp_plat_data->params.bclk_rate, dp->dai_index);
		goto out;
	}
#else
	if (ft[DAI_INTEL_SSP_DEFAULT_IDX].freq % ssp_plat_data->params.bclk_rate != 0) {
		LOG_ERR("invalid bclk_rate = %d for ssp_index = %d",
			ssp_plat_data->params.bclk_rate, dp->dai_index);
		ret = -EINVAL;
		goto out;
	}

	mdiv = ft[DAI_INTEL_SSP_DEFAULT_IDX].freq / ssp_plat_data->params.bclk_rate;
#endif

#if SSP_IP_VER == SSP_IP_VER_1_0
	if (need_ecs) {
		sscr0 |= SSCR0_ECS;
	}
#endif

	/* clock divisor is SCR + 1 */
	mdiv -= 1;

	/* divisor must be within SCR range */
	if (mdiv > (SSCR0_SCR_MASK >> 8)) {
		LOG_ERR("divisor %d is not within SCR range", mdiv);
		ret = -EINVAL;
		goto out;
	}

	/* set the SCR divisor */
	sscr0 &= ~SSCR0_SCR_MASK;
	sscr0 |= SSCR0_SCR(mdiv);

	sys_write32(sscr0, dai_base(dp) + SSCR0);

	LOG_INF("sscr0 = 0x%08x", sscr0);
out:
	if (!ret) {
		ssp_plat_data->clk_active |= SSP_CLK_BCLK_ACTIVE;
	}

	return ret;
}

static void dai_ssp_bclk_disable_unprepare(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);

	if (!(ssp_plat_data->clk_active & SSP_CLK_BCLK_ACTIVE)) {
		return;
	}
#if SSP_IP_VER == SSP_IP_VER_1_0
	dai_ssp_mn_release_bclk(dp, ssp_plat_data->ssp_index);
#endif
	ssp_plat_data->clk_active &= ~SSP_CLK_BCLK_ACTIVE;
}

static void dai_ssp_log_ssp_data(struct dai_intel_ssp *dp)
{
	LOG_INF("dai index: %u", dp->dai_index);
	LOG_INF("plat_data base: %u", dp->ssp_plat_data->base);
	LOG_INF("plat_data irq: %u", dp->ssp_plat_data->irq);
	LOG_INF("plat_data fifo playback offset: %u",
		dp->ssp_plat_data->fifo[DAI_DIR_PLAYBACK].offset);
	LOG_INF("plat_data fifo playback handshake: %u",
		dp->ssp_plat_data->fifo[DAI_DIR_PLAYBACK].handshake);
	LOG_INF("plat_data fifo capture offset: %u",
		dp->ssp_plat_data->fifo[DAI_DIR_CAPTURE].offset);
	LOG_INF("plat_data fifo capture handshake: %u",
		dp->ssp_plat_data->fifo[DAI_DIR_CAPTURE].handshake);
}

/* Digital Audio interface formatting */
static int dai_ssp_set_config_tplg(struct dai_intel_ssp *dp, const struct dai_config *config,
				   const void *bespoke_cfg)
{
	struct dai_intel_ssp_pdata *ssp = dai_get_drvdata(dp);
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	struct dai_intel_ssp_freq_table *ft = dai_get_ftable(dp);
	uint32_t sscr0;
	uint32_t sscr1;
	uint32_t sscr2;
	uint32_t sscr3;
	uint32_t sspsp;
	uint32_t sspsp2;
	uint32_t sstsa;
	uint32_t ssrsa;
	uint32_t ssto;
	uint32_t ssioc;
	uint32_t bdiv;
	uint32_t data_size;
	uint32_t frame_end_padding;
	uint32_t slot_end_padding;
	uint32_t frame_len = 0;
	uint32_t bdiv_min;
	uint32_t tft;
	uint32_t rft;
	uint32_t active_tx_slots = 2;
	uint32_t active_rx_slots = 2;
	uint32_t sample_width = 2;

	bool inverted_bclk = false;
	bool inverted_frame = false;
	bool cfs = false;
	bool start_delay = false;
	k_spinlock_key_t key;
	int ret = 0;

	dai_ssp_log_ssp_data(dp);

	key = k_spin_lock(&dp->lock);

	/* ignore config if SSP is already configured */
	if (dp->state[DAI_DIR_PLAYBACK] > DAI_STATE_READY ||
		dp->state[DAI_DIR_CAPTURE] > DAI_STATE_READY) {
		if (!memcmp(&ssp_plat_data->params, bespoke_cfg,
			sizeof(struct dai_intel_ipc3_ssp_params))) {
			LOG_INF("Already configured. Ignore config");
			goto clk;
		}

		if (ssp_plat_data->clk_active &
			(SSP_CLK_MCLK_ACTIVE | SSP_CLK_BCLK_ACTIVE)) {
			LOG_WRN("SSP active, cannot change config");
			goto clk;
		}

		/* safe to proceed and change HW config */
	}

	LOG_INF("SSP%d", dp->dai_index);

	/* reset SSP settings */
	/* sscr0 dynamic settings are DSS, EDSS, SCR, FRDC, ECS */
	/*
	 * FIXME: MOD, ACS, NCS are not set,
	 * no support for network mode for now
	 */
	sscr0 = SSCR0_PSP | SSCR0_RIM | SSCR0_TIM;

	/* sscr1 dynamic settings are SFRMDIR, SCLKDIR, SCFR, RSRE, TSRE */
	sscr1 = SSCR1_TTE | SSCR1_TTELP | SSCR1_TRAIL;

	/* sscr2 dynamic setting is LJDFD */
	sscr2 = SSCR2_SDFD | SSCR2_TURM1;

	/* sscr3 dynamic settings are TFT, RFT */
	sscr3 = 0;

	/* sspsp dynamic settings are SCMODE, SFRMP, DMYSTRT, SFRMWDTH */
	sspsp = 0;

	ssp->config = *config;
	memcpy(&ssp_plat_data->params, bespoke_cfg, sizeof(struct dai_intel_ipc3_ssp_params));

	/* sspsp2 no dynamic setting */
	sspsp2 = 0x0;

	/* ssioc dynamic setting is SFCR */
	ssioc = SSIOC_SCOE;

	/* ssto no dynamic setting */
	ssto = 0x0;

	/* sstsa dynamic setting is TTSA, default 2 slots */
	sstsa = SSTSA_SSTSA(ssp_plat_data->params.tx_slots);

	/* ssrsa dynamic setting is RTSA, default 2 slots */
	ssrsa = SSRSA_SSRSA(ssp_plat_data->params.rx_slots);

	switch (config->format & DAI_INTEL_IPC3_SSP_FMT_CLOCK_PROVIDER_MASK) {
	case DAI_INTEL_IPC3_SSP_FMT_CBP_CFP:
		sscr1 |= SSCR1_SCLKDIR | SSCR1_SFRMDIR;
		break;
	case DAI_INTEL_IPC3_SSP_FMT_CBC_CFC:
		sscr1 |= SSCR1_SCFR;
		cfs = true;
		break;
	case DAI_INTEL_IPC3_SSP_FMT_CBP_CFC:
		sscr1 |= SSCR1_SCLKDIR;
		/* FIXME: this mode has not been tested */

		cfs = true;
		break;
	case DAI_INTEL_IPC3_SSP_FMT_CBC_CFP:
		sscr1 |= SSCR1_SCFR | SSCR1_SFRMDIR;
		/* FIXME: this mode has not been tested */
		break;
	default:
		LOG_ERR("format & PROVIDER_MASK EINVAL");
		ret = -EINVAL;
		goto out;
	}

	/* clock signal polarity */
	switch (config->format & DAI_INTEL_IPC3_SSP_FMT_INV_MASK) {
	case DAI_INTEL_IPC3_SSP_FMT_NB_NF:
		break;
	case DAI_INTEL_IPC3_SSP_FMT_NB_IF:
		inverted_frame = true; /* handled later with format */
		break;
	case DAI_INTEL_IPC3_SSP_FMT_IB_IF:
		inverted_bclk = true; /* handled later with bclk idle */
		inverted_frame = true; /* handled later with format */
		break;
	case DAI_INTEL_IPC3_SSP_FMT_IB_NF:
		inverted_bclk = true; /* handled later with bclk idle */
		break;
	default:
		LOG_ERR("format & INV_MASK EINVAL");
		ret = -EINVAL;
		goto out;
	}

	/* supporting bclk idle state */
	if (ssp_plat_data->params.clks_control & DAI_INTEL_IPC3_SSP_CLKCTRL_BCLK_IDLE_HIGH) {
		/* bclk idle state high */
		sspsp |= SSPSP_SCMODE((inverted_bclk ^ 0x3) & 0x3);
	} else {
		/* bclk idle state low */
		sspsp |= SSPSP_SCMODE(inverted_bclk);
	}

	sscr0 |= SSCR0_MOD | SSCR0_ACS;

	/* Additional hardware settings */

	/* Receiver Time-out Interrupt Disabled/Enabled */
	sscr1 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_TINTE) ?
		SSCR1_TINTE : 0;

	/* Peripheral Trailing Byte Interrupts Disable/Enable */
	sscr1 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_PINTE) ?
		SSCR1_PINTE : 0;

	/* Enable/disable internal loopback. Output of transmit serial
	 * shifter connected to input of receive serial shifter, internally.
	 */
	sscr1 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_LBM) ?
		SSCR1_LBM : 0;

	if (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_LBM) {
		LOG_INF("going for loopback!");
	} else {
		LOG_INF("no loopback!");
	}

	/* Transmit data are driven at the same/opposite clock edge specified
	 * in SSPSP.SCMODE[1:0]
	 */
	sscr2 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_SMTATF) ?
		SSCR2_SMTATF : 0;

	/* Receive data are sampled at the same/opposite clock edge specified
	 * in SSPSP.SCMODE[1:0]
	 */
	sscr2 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_MMRATF) ?
		SSCR2_MMRATF : 0;

	/* Enable/disable the fix for PSP consumer mode TXD wait for frame
	 * de-assertion before starting the second channel
	 */
	sscr2 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_PSPSTWFDFD) ?
		SSCR2_PSPSTWFDFD : 0;

	/* Enable/disable the fix for PSP provider mode FSRT with dummy stop &
	 * frame end padding capability
	 */
	sscr2 |= (ssp_plat_data->params.quirks & DAI_INTEL_IPC3_SSP_QUIRK_PSPSRWFDFD) ?
		SSCR2_PSPSRWFDFD : 0;

	if (!ssp_plat_data->params.mclk_rate ||
	    ssp_plat_data->params.mclk_rate > ft[DAI_INTEL_SSP_MAX_FREQ_INDEX].freq) {
		LOG_ERR("invalid MCLK = %d Hz (valid < %d)", ssp_plat_data->params.mclk_rate,
			ft[DAI_INTEL_SSP_MAX_FREQ_INDEX].freq);
		ret = -EINVAL;
		goto out;
	}

	if (!ssp_plat_data->params.bclk_rate ||
		ssp_plat_data->params.bclk_rate > ssp_plat_data->params.mclk_rate) {
		LOG_ERR("BCLK %d Hz = 0 or > MCLK %d Hz", ssp_plat_data->params.bclk_rate,
			ssp_plat_data->params.mclk_rate);
		ret = -EINVAL;
		goto out;
	}

	/* calc frame width based on BCLK and rate - must be divisible */
	if (ssp_plat_data->params.bclk_rate % ssp_plat_data->params.fsync_rate) {
		LOG_ERR("BCLK %d is not divisible by rate %d", ssp_plat_data->params.bclk_rate,
			ssp_plat_data->params.fsync_rate);
		ret = -EINVAL;
		goto out;
	}

	/* must be enough BCLKs for data */
	bdiv = ssp_plat_data->params.bclk_rate / ssp_plat_data->params.fsync_rate;
	if (bdiv < ssp_plat_data->params.tdm_slot_width * ssp_plat_data->params.tdm_slots) {
		LOG_ERR("not enough BCLKs need %d",
			ssp_plat_data->params.tdm_slot_width * ssp_plat_data->params.tdm_slots);
		ret = -EINVAL;
		goto out;
	}

	/* tdm_slot_width must be <= 38 for SSP */
	if (ssp_plat_data->params.tdm_slot_width > 38) {
		LOG_ERR("tdm_slot_width %d > 38", ssp_plat_data->params.tdm_slot_width);
		ret = -EINVAL;
		goto out;
	}

	bdiv_min = ssp_plat_data->params.tdm_slots *
		   (ssp_plat_data->params.tdm_per_slot_padding_flag ?
		    ssp_plat_data->params.tdm_slot_width : ssp_plat_data->params.sample_valid_bits);
	if (bdiv < bdiv_min) {
		LOG_ERR("bdiv(%d) < bdiv_min(%d)", bdiv, bdiv_min);
		ret = -EINVAL;
		goto out;
	}

	frame_end_padding = bdiv - bdiv_min;
	if (frame_end_padding > SSPSP2_FEP_MASK) {
		LOG_ERR("frame_end_padding too big: %u", frame_end_padding);
		ret = -EINVAL;
		goto out;
	}

	/* format */
	switch (config->format & DAI_INTEL_IPC3_SSP_FMT_FORMAT_MASK) {
	case DAI_INTEL_IPC3_SSP_FMT_I2S:

		start_delay = true;

		sscr0 |= SSCR0_FRDC(ssp_plat_data->params.tdm_slots);

		if (bdiv % 2) {
			LOG_ERR("bdiv %d is not divisible by 2", bdiv);
			ret = -EINVAL;
			goto out;
		}

		/* set asserted frame length to half frame length */
		frame_len = bdiv / 2;

		/*
		 * handle frame polarity, I2S default is falling/active low,
		 * non-inverted(inverted_frame=0) -- active low(SFRMP=0),
		 * inverted(inverted_frame=1) -- rising/active high(SFRMP=1),
		 * so, we should set SFRMP to inverted_frame.
		 */
		sspsp |= SSPSP_SFRMP(inverted_frame);

		/*
		 *  for I2S/LEFT_J, the padding has to happen at the end
		 * of each slot
		 */
		if (frame_end_padding % 2) {
			LOG_ERR("frame_end_padding %d is not divisible by 2", frame_end_padding);
			ret = -EINVAL;
			goto out;
		}

		slot_end_padding = frame_end_padding / 2;

		if (slot_end_padding > DAI_INTEL_IPC3_SSP_SLOT_PADDING_MAX) {
			/* too big padding */
			LOG_ERR("slot_end_padding > %d", DAI_INTEL_IPC3_SSP_SLOT_PADDING_MAX);
			ret = -EINVAL;
			goto out;
		}

		sspsp |= SSPSP_DMYSTOP(slot_end_padding);
		slot_end_padding >>= SSPSP_DMYSTOP_BITS;
		sspsp |= SSPSP_EDMYSTOP(slot_end_padding);

		break;

	case DAI_INTEL_IPC3_SSP_FMT_LEFT_J:

		/* default start_delay value is set to false */

		sscr0 |= SSCR0_FRDC(ssp_plat_data->params.tdm_slots);

		/* LJDFD enable */
		sscr2 &= ~SSCR2_LJDFD;

		if (bdiv % 2) {
			LOG_ERR("bdiv %d is not divisible by 2", bdiv);
			ret = -EINVAL;
			goto out;
		}

		/* set asserted frame length to half frame length */
		frame_len = bdiv / 2;

		/*
		 * handle frame polarity, LEFT_J default is rising/active high,
		 * non-inverted(inverted_frame=0) -- active high(SFRMP=1),
		 * inverted(inverted_frame=1) -- falling/active low(SFRMP=0),
		 * so, we should set SFRMP to !inverted_frame.
		 */
		sspsp |= SSPSP_SFRMP(!inverted_frame);

		/*
		 *  for I2S/LEFT_J, the padding has to happen at the end
		 * of each slot
		 */
		if (frame_end_padding % 2) {
			LOG_ERR("frame_end_padding %d is not divisible by 2", frame_end_padding);
			ret = -EINVAL;
			goto out;
		}

		slot_end_padding = frame_end_padding / 2;

		if (slot_end_padding > 15) {
			/* can't handle padding over 15 bits */
			LOG_ERR("slot_end_padding %d > 15 bits", slot_end_padding);
			ret = -EINVAL;
			goto out;
		}

		sspsp |= SSPSP_DMYSTOP(slot_end_padding);
		slot_end_padding >>= SSPSP_DMYSTOP_BITS;
		sspsp |= SSPSP_EDMYSTOP(slot_end_padding);

		break;
	case DAI_INTEL_IPC3_SSP_FMT_DSP_A:

		start_delay = true;

		/* fallthrough */

	case DAI_INTEL_IPC3_SSP_FMT_DSP_B:

		/* default start_delay value is set to false */

		sscr0 |= SSCR0_MOD | SSCR0_FRDC(ssp_plat_data->params.tdm_slots);

		/* set asserted frame length */
		frame_len = 1; /* default */

		if (cfs && ssp_plat_data->params.frame_pulse_width > 0 &&
		    ssp_plat_data->params.frame_pulse_width <=
		    DAI_INTEL_IPC3_SSP_FRAME_PULSE_WIDTH_MAX) {
			frame_len = ssp_plat_data->params.frame_pulse_width;
		}

		/* frame_pulse_width must less or equal 38 */
		if (ssp_plat_data->params.frame_pulse_width >
			DAI_INTEL_IPC3_SSP_FRAME_PULSE_WIDTH_MAX) {
			LOG_ERR("frame_pulse_width > %d", DAI_INTEL_IPC3_SSP_FRAME_PULSE_WIDTH_MAX);
			ret = -EINVAL;
			goto out;
		}
		/*
		 * handle frame polarity, DSP_B default is rising/active high,
		 * non-inverted(inverted_frame=0) -- active high(SFRMP=1),
		 * inverted(inverted_frame=1) -- falling/active low(SFRMP=0),
		 * so, we should set SFRMP to !inverted_frame.
		 */
		sspsp |= SSPSP_SFRMP(!inverted_frame);

		active_tx_slots = POPCOUNT(ssp_plat_data->params.tx_slots);
		active_rx_slots = POPCOUNT(ssp_plat_data->params.rx_slots);

		/*
		 * handle TDM mode, TDM mode has padding at the end of
		 * each slot. The amount of padding is equal to result of
		 * subtracting slot width and valid bits per slot.
		 */
		if (ssp_plat_data->params.tdm_per_slot_padding_flag) {
			frame_end_padding = bdiv - ssp_plat_data->params.tdm_slots *
				ssp_plat_data->params.tdm_slot_width;

			slot_end_padding = ssp_plat_data->params.tdm_slot_width -
				ssp_plat_data->params.sample_valid_bits;

			if (slot_end_padding >
				DAI_INTEL_IPC3_SSP_SLOT_PADDING_MAX) {
				LOG_ERR("slot_end_padding > %d",
					DAI_INTEL_IPC3_SSP_SLOT_PADDING_MAX);
				ret = -EINVAL;
				goto out;
			}

			sspsp |= SSPSP_DMYSTOP(slot_end_padding);
			slot_end_padding >>= SSPSP_DMYSTOP_BITS;
			sspsp |= SSPSP_EDMYSTOP(slot_end_padding);
		}

		sspsp2 |= (frame_end_padding & SSPSP2_FEP_MASK);

		break;
	default:
		LOG_ERR("invalid format 0x%04x", config->format);
		ret = -EINVAL;
		goto out;
	}

	if (start_delay) {
		sspsp |= SSPSP_FSRT;
	}

	sspsp |= SSPSP_SFRMWDTH(frame_len);

	data_size = ssp_plat_data->params.sample_valid_bits;

	if (data_size > 16) {
		sscr0 |= (SSCR0_EDSS | SSCR0_DSIZE(data_size - 16));
	} else {
		sscr0 |= SSCR0_DSIZE(data_size);
	}

	/* setting TFT and RFT */
	switch (ssp_plat_data->params.sample_valid_bits) {
	case 16:
		/* use 2 bytes for each slot */
		sample_width = 2;
		break;
	case 24:
	case 32:
		/* use 4 bytes for each slot */
		sample_width = 4;
		break;
	default:
		LOG_ERR("sample_valid_bits %d", ssp_plat_data->params.sample_valid_bits);
		ret = -EINVAL;
		goto out;
	}

	tft = MIN(DAI_INTEL_SSP_FIFO_DEPTH - DAI_INTEL_SSP_FIFO_WATERMARK,
		  sample_width * active_tx_slots);
	rft = MIN(DAI_INTEL_SSP_FIFO_DEPTH - DAI_INTEL_SSP_FIFO_WATERMARK,
		  sample_width * active_rx_slots);

	sscr3 |= SSCR3_TX(tft) | SSCR3_RX(rft);

	sys_write32(sscr0, dai_base(dp) + SSCR0);
	sys_write32(sscr1, dai_base(dp) + SSCR1);
	sys_write32(sscr2, dai_base(dp) + SSCR2);
	sys_write32(sscr3, dai_base(dp) + SSCR3);
	sys_write32(sspsp, dai_base(dp) + SSPSP);
	sys_write32(sspsp2, dai_base(dp) + SSPSP2);
	sys_write32(ssioc, dai_base(dp) + SSIOC);
	sys_write32(ssto, dai_base(dp) + SSTO);

#if SSP_IP_VER > SSP_IP_VER_2_0
	for (uint32_t idx = 0; idx < I2SIPCMC; ++idx) {
		sys_write64(sstsa, dai_base(dp) + SSMODyTSA(idx));
	}

	for (uint32_t idx = 0; idx < I2SOPCMC; ++idx) {
		sys_write64(ssrsa, dai_base(dp) + SSMIDyTSA(idx));
	}
#else
	sys_write32(sstsa, dai_base(dp) + SSTSA);
	sys_write32(ssrsa, dai_base(dp) + SSRSA);
#endif

	LOG_INF("sscr0 = 0x%08x, sscr1 = 0x%08x, ssto = 0x%08x, sspsp = 0x%0x",
		sscr0, sscr1, ssto, sspsp);
	LOG_INF("sscr2 = 0x%08x, sspsp2 = 0x%08x, sscr3 = 0x%08x, ssioc = 0x%08x",
		sscr2, sspsp2, sscr3, ssioc);
	LOG_INF("ssrsa = 0x%08x, sstsa = 0x%08x",
		ssrsa, sstsa);

	dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_PRE_RUNNING;
	dp->state[DAI_DIR_CAPTURE] = DAI_STATE_PRE_RUNNING;

clk:
	switch (config->options & DAI_INTEL_IPC3_SSP_CONFIG_FLAGS_CMD_MASK) {
	case DAI_INTEL_IPC3_SSP_CONFIG_FLAGS_HW_PARAMS:
		if (ssp_plat_data->params.clks_control & DAI_INTEL_IPC3_SSP_CLKCTRL_MCLK_ES) {
			ret = dai_ssp_mclk_prepare_enable(dp);
			if (ret < 0) {
				goto out;
			}

			ssp_plat_data->clk_active |= SSP_CLK_MCLK_ES_REQ;

			LOG_INF("hw_params stage: enabled MCLK clocks for SSP%d...",
					dp->dai_index);
		}

		if (ssp_plat_data->params.clks_control & DAI_INTEL_IPC3_SSP_CLKCTRL_BCLK_ES) {
			bool enable_sse = false;

			if (!(ssp_plat_data->clk_active & SSP_CLK_BCLK_ACTIVE)) {
				enable_sse = true;
			}

			ret = dai_ssp_bclk_prepare_enable(dp);
			if (ret < 0) {
				goto out;
			}

			ssp_plat_data->clk_active |= SSP_CLK_BCLK_ES_REQ;

			if (enable_sse) {

#if SSP_IP_VER > SSP_IP_VER_2_0
				dai_ssp_update_bits(dp, SSMIDyCS(dp->tdm_slot_group),
						    SSMIDyCS_RSRE, SSMIDyCS_RSRE);
				dai_ssp_update_bits(dp, SSMODyCS(dp->tdm_slot_group),
						    SSMODyCS_TSRE, SSMODyCS_TSRE);
#endif
				/* enable port */
				dai_ssp_update_bits(dp, SSCR0, SSCR0_SSE, SSCR0_SSE);

				LOG_INF("SSE set for SSP%d", ssp_plat_data->ssp_index);
			}

			LOG_INF("hw_params stage: enabled BCLK clocks for SSP%d...",
					ssp_plat_data->ssp_index);
		}
		break;
	case DAI_INTEL_IPC3_SSP_CONFIG_FLAGS_HW_FREE:
		/* disable SSP port if no users */
		if (dp->state[DAI_DIR_CAPTURE] != DAI_STATE_PRE_RUNNING ||
		    dp->state[DAI_DIR_PLAYBACK] != DAI_STATE_PRE_RUNNING) {
			LOG_INF("hw_free stage: ignore since SSP%d still in use",
					dp->dai_index);
			break;
		}

		if (ssp_plat_data->params.clks_control & DAI_INTEL_IPC3_SSP_CLKCTRL_BCLK_ES) {
			LOG_INF("hw_free stage: releasing BCLK clocks for SSP%d...",
					dp->dai_index);
			if (ssp_plat_data->clk_active & SSP_CLK_BCLK_ACTIVE) {

#if SSP_IP_VER > SSP_IP_VER_2_0
				for (uint32_t idx = 0; idx < I2SOPCMC; ++idx) {
					dai_ssp_update_bits(dp, SSMODyCS(idx), SSMODyCS_TSRE, 0);
				}

				for (uint32_t idx = 0; idx < I2SIPCMC; ++idx) {
					dai_ssp_update_bits(dp, SSMIDyCS(idx), SSMIDyCS_RSRE, 0);
				}
#endif
				dai_ssp_update_bits(dp, SSCR0, SSCR0_SSE, 0);
				LOG_INF("SSE clear for SSP%d", dp->dai_index);
			}
			dai_ssp_bclk_disable_unprepare(dp);
			ssp_plat_data->clk_active &= ~SSP_CLK_BCLK_ES_REQ;
		}
		if (ssp_plat_data->params.clks_control & DAI_INTEL_IPC3_SSP_CLKCTRL_MCLK_ES) {
			LOG_INF("hw_free stage: releasing MCLK clocks for SSP%d...",
					dp->dai_index);
			dai_ssp_mclk_disable_unprepare(dp);
			ssp_plat_data->clk_active &= ~SSP_CLK_MCLK_ES_REQ;
		}
		break;
	default:
		break;
	}
out:

	k_spin_unlock(&dp->lock, key);

	return ret;
}

static int dai_ssp_check_aux_data(struct ssp_intel_aux_tlv *aux_tlv, int aux_len, int parsed_len)
{
	struct ssp_intel_sync_ctl *sync;
	int size, size_left;

	switch (aux_tlv->type) {
	case SSP_MN_DIVIDER_CONTROLS:
		size = sizeof(struct ssp_intel_mn_ctl);
		break;
	case SSP_DMA_CLK_CONTROLS:
		size = sizeof(struct ssp_intel_clk_ctl);
		break;
	case SSP_DMA_TRANSMISSION_START:
	case SSP_DMA_TRANSMISSION_STOP:
		size = sizeof(struct ssp_intel_tr_ctl);
	case SSP_DMA_ALWAYS_RUNNING_MODE:
		size = sizeof(struct ssp_intel_run_ctl);
		break;
	case SSP_DMA_SYNC_DATA:
		size = sizeof(struct ssp_intel_sync_ctl);
		sync = (struct ssp_intel_sync_ctl *)&aux_tlv->val;
		size += sync->count * sizeof(struct ssp_intel_node_ctl);
		break;
	case SSP_DMA_CLK_CONTROLS_EXT:
		size = sizeof(struct ssp_intel_ext_ctl);
		break;
	case SSP_LINK_CLK_SOURCE:
#if SSP_IP_VER >= SSP_IP_VER_1_5
		size = sizeof(struct ssp_intel_link_ctl);
		break;
#else
		return 0;
#endif
	default:
		LOG_ERR("undefined aux data type %u", aux_tlv->type);
		return -EINVAL;
	}

	/* check for malformed struct, size greater than aux_data or described in tlv */
	size_left = aux_len - parsed_len - sizeof(struct ssp_intel_aux_tlv);
	if (size > size_left || size != aux_tlv->size) {
		LOG_ERR("malformed struct, size %d, size_left %d, tlv_size %d", size,
			size_left, aux_tlv->size);
		return -EINVAL;
	}

	return 0;
}

/**
 * This function checks if the provided buffer contains valid DMA control
 * TLV (Type-Length-Value) entries. It ensures that only specific types
 * of DMA control settings are allowed to be modified at runtime.
 */
static int dai_ssp_check_dma_control(const uint8_t *aux_ptr, int aux_len)
{
	int hop;
	struct ssp_intel_aux_tlv *aux_tlv;

	for (int i = 0; i < aux_len; i += hop) {
		aux_tlv = (struct ssp_intel_aux_tlv *)(aux_ptr);
		switch (aux_tlv->type) {
		case SSP_DMA_CLK_CONTROLS:
		case SSP_DMA_TRANSMISSION_START:
		case SSP_DMA_TRANSMISSION_STOP:
		case SSP_DMA_ALWAYS_RUNNING_MODE:
		case SSP_DMA_SYNC_DATA:
		case SSP_DMA_CLK_CONTROLS_EXT:
		case SSP_LINK_CLK_SOURCE:
			break;
		default:
			LOG_ERR("incorect config type %u", aux_tlv->type);
			return -EINVAL;
		}

		hop = aux_tlv->size + sizeof(struct ssp_intel_aux_tlv);
		aux_ptr += hop;
	}

	return 0;
}

static int dai_ssp_parse_tlv(struct dai_intel_ssp *dp, const uint8_t *aux_ptr, size_t aux_len)
{
	int hop, i, j;
	struct ssp_intel_aux_tlv *aux_tlv;
	struct ssp_intel_mn_ctl *mn;
	struct ssp_intel_clk_ctl *clk;
	struct ssp_intel_tr_ctl *tr;
	struct ssp_intel_run_ctl *run;
	struct ssp_intel_node_ctl *node;
	struct ssp_intel_sync_ctl *sync;
	struct ssp_intel_ext_ctl *ext;
#if SSP_IP_VER >= SSP_IP_VER_1_5
	struct ssp_intel_link_ctl *link;
#if SSP_IP_VER > SSP_IP_VER_1_5
	struct dai_intel_ssp_plat_data *ssp = dai_get_plat_data(dp);
#endif
#endif

	for (i = 0; i < aux_len; i += hop) {
		aux_tlv = (struct ssp_intel_aux_tlv *)(aux_ptr);
		if (dai_ssp_check_aux_data(aux_tlv, aux_len, i)) {
			return -EINVAL;
		}
		switch (aux_tlv->type) {
		case SSP_MN_DIVIDER_CONTROLS:
			mn = (struct ssp_intel_mn_ctl *)&aux_tlv->val;
			LOG_INF("mn div_m %u, div_n %u", mn->div_m, mn->div_n);
			break;
		case SSP_DMA_CLK_CONTROLS:
			clk = (struct ssp_intel_clk_ctl *)&aux_tlv->val;
			LOG_INF("clk start %u, stop %u", clk->start, clk->stop);
			break;
		case SSP_DMA_TRANSMISSION_START:
		case SSP_DMA_TRANSMISSION_STOP:
			tr = (struct ssp_intel_tr_ctl *)&aux_tlv->val;
			LOG_INF("tr sampling frequency %u, bit_depth %u, channel_map %u,",
				tr->sampling_frequency, tr->bit_depth, tr->channel_map);
			LOG_INF("channel_config %u, interleaving_style %u, format %u",
				tr->channel_config, tr->interleaving_style, tr->format);
			break;
		case SSP_DMA_ALWAYS_RUNNING_MODE:
			run = (struct ssp_intel_run_ctl *)&aux_tlv->val;
			LOG_INF("run enabled %u", run->enabled);
			break;
		case SSP_DMA_SYNC_DATA:
			sync = (struct ssp_intel_sync_ctl *)&aux_tlv->val;
			LOG_INF("sync sync_denominator %u, count %u",
				sync->sync_denominator, sync->count);
			node = (struct ssp_intel_node_ctl *)((uint8_t *)sync +
							     sizeof(struct ssp_intel_sync_ctl));
			for (j = 0; j < sync->count; j++) {
				LOG_INF("node node_id %u, sampling_rate %u",
					node->node_id, node->sampling_rate);
				node++;
			}
			break;
		case SSP_DMA_CLK_CONTROLS_EXT:
			ext = (struct ssp_intel_ext_ctl *)&aux_tlv->val;
			LOG_INF("ext ext_data %u", ext->ext_data);
			break;
		case SSP_LINK_CLK_SOURCE:
#if SSP_IP_VER >= SSP_IP_VER_1_5
			link = (struct ssp_intel_link_ctl *)&aux_tlv->val;
#if SSP_IP_VER < SSP_IP_VER_2_0
			sys_write32((sys_read32(dai_ip_base(dp) + I2SLCTL_OFFSET) &
				    ~I2CLCTL_MLCS(0x7)) |
				    I2CLCTL_MLCS(link->clock_source), dai_ip_base(dp) +
				    I2SLCTL_OFFSET);
#elif SSP_IP_VER > SSP_IP_VER_1_5
			ssp->link_clock = link->clock_source;
			sys_write32((sys_read32(dai_i2svss_base(dp) + I2SLCTL_OFFSET) &
				    ~I2CLCTL_MLCS(0x7)) |
				    I2CLCTL_MLCS(link->clock_source),
				    dai_i2svss_base(dp) + I2SLCTL_OFFSET);
#endif
			LOG_INF("link clock_source %u", link->clock_source);
#endif
			break;
		default:
			LOG_ERR("undefined aux data type %u", aux_tlv->type);
			return -EINVAL;
		}

		hop = aux_tlv->size + sizeof(struct ssp_intel_aux_tlv);
		aux_ptr += hop;
	}

	return 0;
}

static int dai_ssp_parse_aux_data(struct dai_intel_ssp *dp, const void *spec_config)
{
	const struct dai_intel_ipc4_ssp_configuration_blob_ver_1_5 *blob15 = spec_config;
	const struct dai_intel_ipc4_ssp_configuration_blob_ver_3_0 *blob30 = spec_config;
	int cfg_len, pre_aux_len, aux_len;
	uint8_t *aux_ptr;

	if (blob15->version == SSP_BLOB_VER_1_5) {
		cfg_len = blob15->size;
		pre_aux_len = sizeof(*blob15) +
			      blob15->i2s_mclk_control.mdivrcnt * sizeof(uint32_t);
		aux_len = cfg_len - pre_aux_len;
		aux_ptr = (uint8_t *)blob15 + pre_aux_len;
	} else if (blob30->version == SSP_BLOB_VER_3_0) {
		cfg_len = blob30->size;
		pre_aux_len = sizeof(*blob30) +
			      blob30->i2s_mclk_control.mdivrcnt * sizeof(uint32_t);
		aux_len = cfg_len - pre_aux_len;
		aux_ptr = (uint8_t *)blob30 + pre_aux_len;
	} else {
		LOG_ERR("unsupported blob version");
		return -EINVAL;
	}

	if (aux_len <= 0) {
		return 0;
	}

	return dai_ssp_parse_tlv(dp, aux_ptr, aux_len);
}

static int dai_ssp_set_clock_control_ver_1_5(struct dai_intel_ssp *dp,
					     const struct dai_intel_ipc4_ssp_mclk_config_2 *cc)
{
	/* currently we only support 1 divider */
	if (cc->mdivrcnt != 1) {
		LOG_ERR("bad clock divider count %u", cc->mdivrcnt);
		return -EINVAL;
	}

	/* ssp blob is set by pcm_hw_params for ipc4 stream, so enable
	 * mclk and bclk at this time.
	 */
	dai_ssp_mn_set_mclk_blob(dp, cc->mdivctlr, cc->mdivr[0]);

	return 0;
}

static int dai_ssp_set_clock_control_ver_1(struct dai_intel_ssp *dp,
					   const struct dai_intel_ipc4_ssp_mclk_config *cc)
{
	/* ssp blob is set by pcm_hw_params for ipc4 stream, so enable
	 * mclk and bclk at this time.
	 */
	dai_ssp_mn_set_mclk_blob(dp, cc->mdivc, cc->mdivr);

	return 0;
}

#if SSP_IP_VER > SSP_IP_VER_2_0
static void dai_ssp_set_reg_config(struct dai_intel_ssp *dp, const struct dai_config *cfg,
				   const void *spec_config)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	const struct dai_intel_ipc4_ssp_config_ver_3_0 *regs = spec_config;
	uint32_t sscr1 = 0;
	uint32_t ssc0 = regs->ssc0;
	sscr1 = regs->ssc1 & ~(SSCR1_RSVD21);

	LOG_INF("SSP%d configuration:", dp->dai_index);

	sys_write32(ssc0, dai_base(dp) + SSCR0);
	sys_write32(regs->ssc2 & ~SSCR2_SFRMEN, dai_base(dp) + SSCR2); /* hardware specific flow */
	sys_write32(sscr1, dai_base(dp) + SSCR1);
	sys_write32(regs->ssc2 | SSCR2_SFRMEN, dai_base(dp) + SSCR2); /* hardware specific flow */
	sys_write32(regs->ssc2, dai_base(dp) + SSCR2);
	sys_write32(regs->sspsp, dai_base(dp) + SSPSP);
	sys_write32(regs->sspsp2, dai_base(dp) + SSPSP2);
	sys_write32(regs->ssioc, dai_base(dp) + SSIOC);
	sys_write32(regs->sscto, dai_base(dp) + SSTO);

	for (uint32_t idx = 0; idx < I2SIPCMC; ++idx) {
		sys_write64(regs->ssmidytsa[idx], dai_base(dp) + SSMIDyTSA(idx));
	}

	for (uint32_t idx = 0; idx < I2SOPCMC; ++idx) {
		sys_write64(regs->ssmodytsa[idx], dai_base(dp) + SSMODyTSA(idx));
	}

	LOG_INF(" sscr0 = 0x%08x, sscr1 = 0x%08x, ssto = 0x%08x, sspsp = 0x%0x",
		ssc0, sscr1, regs->sscto, regs->sspsp);
	LOG_INF(" sscr2 = 0x%08x, sspsp2 = 0x%08x, sscr3 = 0x%08x",
		regs->ssc2, regs->sspsp2, regs->ssc3);
	LOG_INF(" ssioc = 0x%08x", regs->ssioc);

	ssp_plat_data->params.sample_valid_bits = SSCR0_DSIZE_GET(ssc0);
	if (ssc0 & SSCR0_EDSS) {
		ssp_plat_data->params.sample_valid_bits += 16;
	}

	ssp_plat_data->params.tx_slots = regs->ssmodytsa[dp->tdm_slot_group];
	ssp_plat_data->params.rx_slots = regs->ssmidytsa[dp->tdm_slot_group];

	ssp_plat_data->params.fsync_rate = cfg->rate;
	dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_PRE_RUNNING;
	dp->state[DAI_DIR_CAPTURE] = DAI_STATE_PRE_RUNNING;
}
#else
static void dai_ssp_set_reg_config(struct dai_intel_ssp *dp, const struct dai_config *cfg,
				   const void *spec_config)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	const struct dai_intel_ipc4_ssp_config *regs = spec_config;
	uint32_t sscr1 = 0;
	uint32_t sstsa = 0;
	uint32_t ssrsa = 0;
	uint32_t ssc0 = regs->ssc0;

	sscr1 = regs->ssc1 & ~(SSCR1_RSRE | SSCR1_TSRE);
	sstsa = SSTSA_GET(regs->sstsa);
	ssrsa = SSRSA_GET(regs->ssrsa);

	LOG_INF("SSP%d configuration:", dp->dai_index);

	if (regs->sstsa & SSTSA_TXEN || regs->ssrsa & SSRSA_RXEN ||
	    regs->ssc1 & (SSCR1_RSRE | SSCR1_TSRE)) {
		LOG_INF(" Ignoring %s%s%s%sfrom blob",
			regs->sstsa & SSTSA_TXEN ? "SSTSA:TXEN " : "",
			regs->ssrsa & SSRSA_RXEN ? "SSRSA:RXEN " : "",
			regs->ssc1 & SSCR1_TSRE ? "SSCR1:TSRE " : "",
			regs->ssc1 & SSCR1_RSRE ? "SSCR1:RSRE " : "");
	}

	sys_write32(ssc0, dai_base(dp) + SSCR0);
	sys_write32(regs->ssc2 & ~SSCR2_SFRMEN, dai_base(dp) + SSCR2); /* hardware specific flow */
	sys_write32(sscr1, dai_base(dp) + SSCR1);
	sys_write32(regs->ssc2 | SSCR2_SFRMEN, dai_base(dp) + SSCR2); /* hardware specific flow */
	sys_write32(regs->ssc2, dai_base(dp) + SSCR2);
	sys_write32(regs->ssc3, dai_base(dp) + SSCR3);
	sys_write32(regs->sspsp, dai_base(dp) + SSPSP);
	sys_write32(regs->sspsp2, dai_base(dp) + SSPSP2);
	sys_write32(regs->ssioc, dai_base(dp) + SSIOC);
	sys_write32(regs->sscto, dai_base(dp) + SSTO);
	sys_write32(sstsa, dai_base(dp) + SSTSA);
	sys_write32(ssrsa, dai_base(dp) + SSRSA);

	LOG_INF(" sscr0 = 0x%08x, sscr1 = 0x%08x, ssto = 0x%08x, sspsp = 0x%0x",
		ssc0, sscr1, regs->sscto, regs->sspsp);
	LOG_INF(" sscr2 = 0x%08x, sspsp2 = 0x%08x, sscr3 = 0x%08x",
		regs->ssc2, regs->sspsp2, regs->ssc3);
	LOG_INF(" ssioc = 0x%08x, ssrsa = 0x%08x, sstsa = 0x%08x",
		regs->ssioc, ssrsa, sstsa);

	ssp_plat_data->params.sample_valid_bits = SSCR0_DSIZE_GET(ssc0);
	if (ssc0 & SSCR0_EDSS) {
		ssp_plat_data->params.sample_valid_bits += 16;
	}

	ssp_plat_data->params.tdm_slots = SSCR0_FRDC_GET(ssc0);
	ssp_plat_data->params.tx_slots = SSTSA_GET(sstsa);
	ssp_plat_data->params.rx_slots = SSRSA_GET(ssrsa);
	ssp_plat_data->params.fsync_rate = cfg->rate;
	dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_PRE_RUNNING;
	dp->state[DAI_DIR_CAPTURE] = DAI_STATE_PRE_RUNNING;
}
#endif

static int dai_ssp_set_config_blob(struct dai_intel_ssp *dp, const struct dai_config *cfg,
				   const void *spec_config)
{
	const struct dai_intel_ipc4_ssp_configuration_blob_ver_1_5 *blob15 = spec_config;
	const struct dai_intel_ipc4_ssp_configuration_blob_ver_3_0 *blob30 = spec_config;
	const struct dai_intel_ipc4_ssp_configuration_blob *blob = spec_config;
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	int err;

#if SSP_IP_VER > SSP_IP_VER_2_0
	dp->tdm_slot_group = cfg->tdm_slot_group;
#endif

	/* set config only once for playback or capture */
	if (ssp_plat_data->is_initialized) {
		dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_PRE_RUNNING;
		dp->state[DAI_DIR_CAPTURE] = DAI_STATE_PRE_RUNNING;
		return 0;
	}

	if (blob15->version == SSP_BLOB_VER_1_5) {
		err = dai_ssp_parse_aux_data(dp, spec_config);
		if (err) {
			return err;
		}
		dai_ssp_set_reg_config(dp, cfg, (void *)&blob15->i2s_ssp_config);
		err = dai_ssp_set_clock_control_ver_1_5(dp, &blob15->i2s_mclk_control);
		if (err) {
			return err;
		}
	} else if (blob30->version == SSP_BLOB_VER_3_0) {
		err = dai_ssp_parse_aux_data(dp, spec_config);
		if (err) {
			return err;
		}
		dai_ssp_set_reg_config(dp, cfg, (void *)&blob30->i2s_ssp_config);
		err = dai_ssp_set_clock_control_ver_1_5(dp, &blob30->i2s_mclk_control);
		if (err) {
			return err;
		}
	} else {
		dai_ssp_set_reg_config(dp, cfg, (void *)&blob->i2s_driver_config.i2s_config);
		err = dai_ssp_set_clock_control_ver_1(dp, &blob->i2s_driver_config.mclk_config);
		if (err) {
			return err;
		}
	}

	ssp_plat_data->clk_active |= SSP_CLK_MCLK_ES_REQ;

	/* enable port */
	dai_ssp_update_bits(dp, SSCR0, SSCR0_SSE, SSCR0_SSE);
	ssp_plat_data->clk_active |= SSP_CLK_BCLK_ES_REQ;
	ssp_plat_data->is_initialized = true;

	return 0;
}

/*
 * Portion of the SSP configuration should be applied just before the
 * SSP dai is activated, for either power saving or params runtime
 * configurable flexibility.
 */
static int dai_ssp_pre_start(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	int ret = 0;

	/*
	 * We will test if mclk/bclk is configured in
	 * ssp_mclk/bclk_prepare_enable/disable functions
	 */
	if (!(ssp_plat_data->clk_active & SSP_CLK_MCLK_ES_REQ)) {
		/* MCLK config */
		ret = dai_ssp_mclk_prepare_enable(dp);
		if (ret < 0) {
			return ret;
		}
	}

	if (!(ssp_plat_data->clk_active & SSP_CLK_BCLK_ES_REQ)) {
		ret = dai_ssp_bclk_prepare_enable(dp);
	}

	return ret;
}

/*
 * For power saving, we should do kinds of power release when the SSP
 * dai is changed to inactive, though the runtime param configuration
 * don't have to be reset.
 */
static void dai_ssp_post_stop(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);

	/* release clocks if SSP is inactive */
	if (dp->state[DAI_DIR_PLAYBACK] != DAI_STATE_RUNNING &&
	    dp->state[DAI_DIR_CAPTURE] != DAI_STATE_RUNNING) {
		if (!(ssp_plat_data->clk_active & SSP_CLK_BCLK_ES_REQ)) {
			LOG_INF("releasing BCLK clocks for SSP%d...", dp->dai_index);
			dai_ssp_bclk_disable_unprepare(dp);
		}
		if (!(ssp_plat_data->clk_active & SSP_CLK_MCLK_ES_REQ)) {
			LOG_INF("releasing MCLK clocks for SSP%d...", dp->dai_index);
			dai_ssp_mclk_disable_unprepare(dp);
		}
	}
}

static void dai_ssp_early_start(struct dai_intel_ssp *dp, int direction)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	k_spinlock_key_t key;

	key = k_spin_lock(&dp->lock);

	/* RX fifo must be cleared before start */
	if (direction == DAI_DIR_CAPTURE) {
		LOG_INF("SSP%d RX", dp->dai_index);
		ssp_empty_rx_fifo_on_start(dp);
	} else {
		LOG_INF("SSP%d TX", dp->dai_index);
	}

	/* request mclk/bclk */
	dai_ssp_pre_start(dp);

	if (!(ssp_plat_data->clk_active & SSP_CLK_BCLK_ES_REQ)) {
		/* enable port */
		LOG_INF("SSP%d: set SSE", dp->dai_index);
		dai_ssp_update_bits(dp, SSCR0, SSCR0_SSE, SSCR0_SSE);
	}

	k_spin_unlock(&dp->lock, key);
}

/* start the SSP for either playback or capture */
static void dai_ssp_start(struct dai_intel_ssp *dp, int direction)
{
	struct dai_intel_ssp_pdata *ssp = dai_get_drvdata(dp);
	k_spinlock_key_t key;

	key = k_spin_lock(&dp->lock);


	/* enable DMA */
#if SSP_IP_VER > SSP_IP_VER_2_0
	if (direction == DAI_DIR_PLAYBACK) {
		dai_ssp_update_bits(dp, SSMODyCS(dp->tdm_slot_group),
				    SSMODyCS_TSRE, SSMODyCS_TSRE);
		dai_ssp_update_bits(dp, SSMODyCS(dp->tdm_slot_group),
				    SSMODyCS_TXEN, SSMODyCS_TXEN);
	} else {
		dai_ssp_update_bits(dp, SSMIDyCS(dp->tdm_slot_group),
				    SSMIDyCS_RSRE, SSMIDyCS_RSRE);
		dai_ssp_update_bits(dp, SSMIDyCS(dp->tdm_slot_group),
				    SSMIDyCS_RXEN, SSMIDyCS_RXEN);
	}
#else
	if (direction == DAI_DIR_PLAYBACK) {
		LOG_INF("SSP%d TX", dp->dai_index);
		dai_ssp_update_bits(dp, SSCR1, SSCR1_TSRE, SSCR1_TSRE);
		dai_ssp_update_bits(dp, SSTSA, SSTSA_TXEN, SSTSA_TXEN);
	} else {
		LOG_INF("SSP%d RX", dp->dai_index);
		dai_ssp_update_bits(dp, SSCR1, SSCR1_RSRE, SSCR1_RSRE);
		dai_ssp_update_bits(dp, SSRSA, SSRSA_RXEN, SSRSA_RXEN);
	}
#endif

	dp->state[direction] = DAI_STATE_RUNNING;

	/*
	 * Wait to get valid fifo status in clock consumer mode. TODO it's
	 * uncertain which SSP clock consumer modes need the delay atm, but
	 * these can be added here when confirmed.
	 */
	switch (ssp->config.format & DAI_INTEL_IPC3_SSP_FMT_CLOCK_PROVIDER_MASK) {
	case DAI_INTEL_IPC3_SSP_FMT_CBC_CFC:
		break;
	default:
		/* delay for all SSP consumed clocks atm - see above */
		/* ssp_wait_delay(PLATFORM_SSP_DELAY); */
		k_busy_wait(DAI_INTEL_SSP_PLATFORM_DELAY_US);
		break;
	}

	k_spin_unlock(&dp->lock, key);
}

/* stop the SSP for either playback or capture */
static void dai_ssp_stop(struct dai_intel_ssp *dp, int direction)
{
	struct dai_intel_ssp_pdata *ssp = dai_get_drvdata(dp);
	k_spinlock_key_t key;

	key = k_spin_lock(&dp->lock);

	/*
	 * Wait to get valid fifo status in clock consumer mode. TODO it's
	 * uncertain which SSP clock consumer modes need the delay atm, but
	 * these can be added here when confirmed.
	 */
	switch (ssp->config.format & DAI_INTEL_IPC3_SSP_FMT_CLOCK_PROVIDER_MASK) {
	case DAI_INTEL_IPC3_SSP_FMT_CBC_CFC:
		break;
	default:
		/* delay for all SSP consumed clocks atm - see above */
		k_busy_wait(DAI_INTEL_SSP_PLATFORM_DELAY_US);
		break;
	}

	/* stop Rx if neeed */
	if (direction == DAI_DIR_CAPTURE &&
	    dp->state[DAI_DIR_CAPTURE] != DAI_STATE_PRE_RUNNING) {
		LOG_INF("SSP%d RX", dp->dai_index);

#if SSP_IP_VER > SSP_IP_VER_2_0
		dai_ssp_update_bits(dp, SSMIDyCS(dp->tdm_slot_group), SSMIDyCS_RXEN, 0);
		dai_ssp_update_bits(dp, SSMIDyCS(dp->tdm_slot_group), SSMIDyCS_RSRE, 0);
#else
		dai_ssp_update_bits(dp, SSRSA, SSRSA_RXEN, 0);
		dai_ssp_update_bits(dp, SSCR1, SSCR1_RSRE, 0);
#endif
		ssp_empty_rx_fifo_on_stop(dp);
		dp->state[DAI_DIR_CAPTURE] = DAI_STATE_PRE_RUNNING;
	}

	/* stop Tx if needed */
	if (direction == DAI_DIR_PLAYBACK &&
	    dp->state[DAI_DIR_PLAYBACK] != DAI_STATE_PRE_RUNNING) {
		LOG_INF("SSP%d TX", dp->dai_index);

#if SSP_IP_VER > SSP_IP_VER_2_0
		dai_ssp_update_bits(dp, SSMODyCS(dp->tdm_slot_group), SSMODyCS_TSRE, 0);
		dai_ssp_empty_tx_fifo(dp);
		dai_ssp_update_bits(dp, SSMODyCS(dp->tdm_slot_group), SSMODyCS_TXEN, 0);
#else
		dai_ssp_update_bits(dp, SSCR1, SSCR1_TSRE, 0);
		dai_ssp_empty_tx_fifo(dp);
		dai_ssp_update_bits(dp, SSTSA, SSTSA_TXEN, 0);
#endif
		dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_PRE_RUNNING;
	}

	k_spin_unlock(&dp->lock, key);
}

static void dai_ssp_pause(struct dai_intel_ssp *dp, int direction)
{
	if (direction == DAI_DIR_CAPTURE) {
		LOG_INF("SSP%d RX", dp->dai_index);
	} else {
		LOG_INF("SSP%d TX", dp->dai_index);
	}

	dp->state[direction] = DAI_STATE_PAUSED;
}

static int dai_ssp_trigger(const struct device *dev, enum dai_dir dir,
			   enum dai_trigger_cmd cmd)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;
	int array_index = SSP_ARRAY_INDEX(dir);

	LOG_DBG("SSP%d: cmd %d", dp->dai_index, cmd);

	switch (cmd) {
	case DAI_TRIGGER_START:
		if (dp->state[array_index] == DAI_STATE_PAUSED ||
		    dp->state[array_index] == DAI_STATE_PRE_RUNNING) {
			dai_ssp_start(dp, array_index);
		}
		break;
	case DAI_TRIGGER_STOP:
		dai_ssp_stop(dp, array_index);
		break;
	case DAI_TRIGGER_PAUSE:
		dai_ssp_pause(dp, array_index);
		break;
	case DAI_TRIGGER_PRE_START:
		dai_ssp_early_start(dp, array_index);
		break;
	default:
		break;
	}

	return 0;
}

static int dai_ssp_config_get(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_config *params = (struct dai_config *)dev->config;
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;
	struct dai_intel_ssp_pdata *ssp = dai_get_drvdata(dp);
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);

	if (!cfg) {
		return -EINVAL;
	}

	if (!ssp) {
		*cfg = *params;
		return 0;
	}

	params->rate = ssp_plat_data->params.fsync_rate;

	if (dir == DAI_DIR_PLAYBACK) {
		params->channels = POPCOUNT(ssp_plat_data->params.tx_slots);
	} else {
		params->channels = POPCOUNT(ssp_plat_data->params.rx_slots);
	}

	params->word_size = ssp_plat_data->params.sample_valid_bits;

	*cfg = *params;

	return 0;
}

static int dai_ssp_config_set(const struct device *dev, const struct dai_config *cfg,
			      const void *bespoke_cfg)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	int ret;

	if (cfg->type == DAI_INTEL_SSP) {
		ret = dai_ssp_set_config_tplg(dp, cfg, bespoke_cfg);
	} else {
		ret = dai_ssp_set_config_blob(dp, cfg, bespoke_cfg);
	}
	dai_ssp_program_channel_map(dp, cfg, ssp_plat_data->ssp_index, bespoke_cfg);

	return ret;
}

static const struct dai_properties *dai_ssp_get_properties(const struct device *dev,
							   enum dai_dir dir, int stream_id)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;
	struct dai_intel_ssp_pdata *ssp = dai_get_drvdata(dp);
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	struct dai_properties *prop = &ssp->props;
	int array_index = SSP_ARRAY_INDEX(dir);

	prop->fifo_address = ssp_plat_data->fifo[array_index].offset;
	prop->dma_hs_id = ssp_plat_data->fifo[array_index].handshake;

	if (ssp_plat_data->clk_active & SSP_CLK_BCLK_ACTIVE) {
		prop->reg_init_delay = 0;
	} else {
		prop->reg_init_delay = ssp_plat_data->params.bclk_delay;
	}

	LOG_INF("SSP%u: fifo %u, handshake %u, init delay %u", dp->dai_index, prop->fifo_address,
		prop->dma_hs_id, prop->reg_init_delay);

	return prop;
}

static void ssp_acquire_ip(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp = dai_get_plat_data(dp);

	ssp->acquire_count++;

	if (ssp->acquire_count == 1) {
		/* Enable SSP power */
		dai_ssp_pm_runtime_en_ssp_power(dp, ssp->ssp_index);

		/* Disable dynamic clock gating before touching any register */
		dai_ssp_pm_runtime_dis_ssp_clk_gating(dp, ssp->ssp_index);

#if SSP_IP_VER >= SSP_IP_VER_2_0
		/* Switch to selected clock source */
		sys_write32((sys_read32(dai_i2svss_base(dp) + I2SLCTL_OFFSET) &
			    ~I2CLCTL_MLCS(0x7)) |
			    I2CLCTL_MLCS(ssp->link_clock),
			    dai_i2svss_base(dp) + I2SLCTL_OFFSET);
#endif
	}
}

static void ssp_release_ip(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp = dai_get_plat_data(dp);

	if (ssp->acquire_count == 0) {
		return;
	}

	--ssp->acquire_count;

	if (ssp->acquire_count == 0) {
		/* disable SSP port if no users */
		if (dp->state[DAI_DIR_CAPTURE] == DAI_STATE_PRE_RUNNING &&
		    dp->state[DAI_DIR_PLAYBACK] == DAI_STATE_PRE_RUNNING &&
		    COND_CODE_1(CONFIG_INTEL_ADSP_CAVS,
				(!(ssp->clk_active & SSP_CLK_BCLK_ES_REQ)), (true))) {
			dai_ssp_update_bits(dp, SSCR0, SSCR0_SSE, 0);
			LOG_INF("%s SSE clear SSP%d", __func__, ssp->ssp_index);
		}

		dai_ssp_post_stop(dp);

#if SSP_IP_VER >= SSP_IP_VER_2_0
		/* Restore default XTAL clock source */
		sys_write32((sys_read32(dai_i2svss_base(dp) + I2SLCTL_OFFSET) &
			    ~I2CLCTL_MLCS(0x7)) |
			    I2CLCTL_MLCS(DAI_INTEL_SSP_CLOCK_XTAL_OSCILLATOR),
			    dai_i2svss_base(dp) + I2SLCTL_OFFSET);
#endif

		dai_ssp_pm_runtime_en_ssp_clk_gating(dp, ssp->ssp_index);

		dai_ssp_mclk_disable_unprepare(dp);
		dai_ssp_bclk_disable_unprepare(dp);

		/* Disable SSP power */
		dai_ssp_pm_runtime_dis_ssp_power(dp, ssp->ssp_index);
		ssp->is_initialized = false;
	}
}

static int dai_ssp_probe(struct dai_intel_ssp *dp)
{
	struct dai_intel_ssp_plat_data *ssp_plat_data = dai_get_plat_data(dp);
	struct dai_intel_ssp_pdata *ssp;

	if (dai_get_drvdata(dp)) {
		return -EEXIST; /* already created */
	}

	/* allocate private data */
	ssp = k_calloc(1, sizeof(*ssp));
	if (!ssp) {
		LOG_ERR("SSP%d: alloc failed", ssp_plat_data->ssp_index);
		return -ENOMEM;
	}
	dai_set_drvdata(dp, ssp);

	dp->state[DAI_DIR_PLAYBACK] = DAI_STATE_READY;
	dp->state[DAI_DIR_CAPTURE] = DAI_STATE_READY;

#if SSP_IP_VER == SSP_IP_VER_1_0
	/* Reset M/N, power-gating functions need it */
	dai_ssp_mn_reset_bclk_divider(dp, ssp_plat_data->ssp_index);
#endif

	ssp_acquire_ip(dp);

	return 0;
}

static int dai_ssp_remove(struct dai_intel_ssp *dp)
{
	ssp_release_ip(dp);

	k_free(dai_get_drvdata(dp));
	dai_set_drvdata(dp, NULL);

	return 0;
}

static int ssp_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		dai_ssp_remove(dp);
		break;
	case PM_DEVICE_ACTION_RESUME:
		dai_ssp_probe(dp);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* All device pm is handled during resume and suspend */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dai_intel_ssp_init_device(const struct device *dev)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;

	dp->ssp_plat_data = ssp_get_device_instance(dp->ssp_index);

	return 0;
};

static int ssp_init(const struct device *dev)
{
	dai_intel_ssp_init_device(dev);
	return pm_device_driver_init(dev, ssp_pm_action);
}

static int dai_ssp_dma_control_set(const struct device *dev,
				   const void *bespoke_cfg,
				   size_t size)
{
	struct dai_intel_ssp *dp = (struct dai_intel_ssp *)dev->data;

	LOG_INF("SSP%d: tlv addr = 0x%x, tlv size = %d",
		dp->dai_index, (uint32_t)bespoke_cfg, size);
	if (size < sizeof(struct ssp_intel_aux_tlv)) {
		return -EINVAL;
	}

	if (dp->state[DAI_DIR_PLAYBACK] != DAI_STATE_READY ||
		dp->state[DAI_DIR_CAPTURE] != DAI_STATE_READY) {
		return -EIO;
	}

	if (dai_ssp_check_dma_control(bespoke_cfg, size)) {
		return -EINVAL;
	}

	return dai_ssp_parse_tlv(dp, bespoke_cfg, size);
}

static DEVICE_API(dai, dai_intel_ssp_api_funcs) = {
	.probe			= pm_device_runtime_get,
	.remove			= pm_device_runtime_put,
	.config_set		= dai_ssp_config_set,
	.config_get		= dai_ssp_config_get,
	.trigger		= dai_ssp_trigger,
	.get_properties		= dai_ssp_get_properties,
	.config_update		= dai_ssp_dma_control_set,
};


#define DT_DRV_COMPAT		intel_ssp_dai

#define DAI_INTEL_SSP_DEVICE_INIT(n)					\
	static struct dai_config dai_intel_ssp_config_##n = {		\
		.type = DAI_INTEL_SSP,					\
		.dai_index = DT_INST_REG_ADDR(n),			\
	};								\
	static struct dai_intel_ssp dai_intel_ssp_data_##n = {		\
		.dai_index = DT_INST_REG_ADDR(n),			\
		.ssp_index = DT_PROP(DT_INST_PARENT(n), ssp_index),	\
		.tdm_slot_group = 0,					\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, ssp_pm_action);			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			ssp_init, PM_DEVICE_DT_INST_GET(n),		\
			&dai_intel_ssp_data_##n,			\
			&dai_intel_ssp_config_##n,			\
			POST_KERNEL, 42,				\
			&dai_intel_ssp_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_SSP_DEVICE_INIT)
