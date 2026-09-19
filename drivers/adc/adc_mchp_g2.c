/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ADC driver for the multi-shared-channel SAR converter, ATDF module ADC id
 * 03620, found on PIC32CM SG/GC.
 *
 * The shape of the block: one SAR core, sixteen channel slots of which
 * num-channels are wired to pads, per-channel configuration spread across
 * CHNCFG1..5, and results read indirectly - write the core and channel index
 * into CORCHDATAID, then read CHRDYDAT.
 *
 * CORCTRL, CALCTRL and CTRLD are Enable Protected: a write to any of them
 * while CTRLA.ENABLE is set is ignored and raises a bus error. CHNCFG4 and
 * CHNCFG5 carry no such note in the data sheet, but writing them with the
 * converter enabled was measured to raise a bus error too, so the driver
 * treats all five registers the same: the channel trigger assignment is
 * written once at init and never touched again while the converter is
 * enabled; the only two paths that reprogram an Enable Protected register
 * (resolution and the voltage reference) disable the converter first and
 * re-enable it afterwards.
 */

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_adc_g2

LOG_MODULE_REGISTER(adc_mchp_g2, CONFIG_ADC_LOG_LEVEL);

/*
 * The bandgap has to settle before the converter reports its reference ready,
 * and each SAR core then runs a warm-up counter of about 20 us. 10 ms is far
 * above both and still short enough that a dead peripheral fails the boot
 * rather than hanging it.
 */
#define ADC_MCHP_G2_SYNC_TIMEOUT_US 10000

/* The converter always presents its result in the top of a 12-bit field. */
#define ADC_MCHP_G2_RESULT_BITS 12
#define ADC_MCHP_G2_RESULT_MASK BIT_MASK(ADC_MCHP_G2_RESULT_BITS)

/* CORCTRL.SELRES, data sheet 36.7.5. */
#define ADC_MCHP_G2_SELRES_6BIT  0
#define ADC_MCHP_G2_SELRES_8BIT  1
#define ADC_MCHP_G2_SELRES_10BIT 2
#define ADC_MCHP_G2_SELRES_12BIT 3

/* CTRLD.VREFSEL, data sheet 36.5.2.5. */
#define ADC_MCHP_G2_VREFSEL_AVDD  0
#define ADC_MCHP_G2_VREFSEL_VREFH 1

/*
 * Sentinels for data->resolution and data->vref_sel while a protected
 * register write is in flight and its outcome is not yet known. Neither
 * value is ever a real SELRES/VREFSEL setting, so a retry with the old,
 * cached value cannot be mistaken for one that still matches hardware.
 */
#define ADC_MCHP_G2_RESOLUTION_INVALID 0
#define ADC_MCHP_G2_VREFSEL_INVALID    0xFFU

/* CHNCFG4/5.TRGSRCk, data sheet 36.5.2.11.2. Four bits per channel. */
#define ADC_MCHP_G2_TRGSRC_NONE   0
#define ADC_MCHP_G2_TRGSRC_GSWTRG 1
#define ADC_MCHP_G2_TRGSRC_BITS   4

struct adc_mchp_g2_config {
	adc_registers_t *regs;
	fuses_calotp_registers_t *fuses;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	void (*irq_config)(void);
	/*
	 * The channels the devicetree declares. Fixed for the life of the
	 * device: writing CHNCFG4/5 while the converter runs is ignored and
	 * raises a bus error, the same as the data sheet's Enable Protected
	 * registers.
	 */
	uint16_t declared_channels;
	uint8_t num_channels;
	uint8_t ctl_clk_div;
	uint8_t adc_clk_div;
	uint16_t sample_count;
	uint8_t wakeup_exp;
};

struct adc_mchp_g2_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *buffer_repeat;
	/* Channels channel_setup() has accepted. */
	uint16_t configured_channels;
	/* Channels the current conversion is waiting on, cleared as CHRDY arrives. */
	uint16_t pending_channels;
	/* Channels the current sequence asked for, so the rest are discarded. */
	uint16_t active_channels;
	/* Bits currently programmed into CORCTRL.SELRES, as a resolution. */
	uint8_t resolution;
	/* CTRLD.VREFSEL currently programmed. Locked once a channel is configured. */
	uint8_t vref_sel;
};

/*
 * Wait for a bit field to reach a value, and say which register gave up.
 * The polarity and the bound are both spelled out at every call site rather
 * than hidden in a helper that could be read either way.
 */
static bool adc_mchp_g2_wait(volatile const uint32_t *reg, uint32_t mask, bool want_set,
			      const char *what)
{
	if (WAIT_FOR(((*reg & mask) != 0) == want_set, ADC_MCHP_G2_SYNC_TIMEOUT_US,
		     k_busy_wait(1))) {
		return true;
	}

	LOG_ERR("timeout waiting for %s: register reads 0x%08x, mask 0x%08x", what, *reg, mask);

	return false;
}

/*
 * Give every declared channel the global software trigger and leave the rest
 * on no trigger at all. Only ever called with the converter disabled.
 */
static void adc_mchp_g2_program_triggers(adc_registers_t *regs, uint16_t channels)
{
	uint32_t cfg4 = 0;
	uint32_t cfg5 = 0;

	for (uint8_t k = 0; k < 8; k++) {
		if ((channels & BIT(k)) != 0) {
			cfg4 |= ADC_MCHP_G2_TRGSRC_GSWTRG << (k * ADC_MCHP_G2_TRGSRC_BITS);
		}
	}

	for (uint8_t k = 8; k < 16; k++) {
		if ((channels & BIT(k)) != 0) {
			cfg5 |= ADC_MCHP_G2_TRGSRC_GSWTRG
				<< ((k - 8) * ADC_MCHP_G2_TRGSRC_BITS);
		}
	}

	regs->CONFIG[0].ADC_CHNCFG4 = cfg4;
	regs->CONFIG[0].ADC_CHNCFG5 = cfg5;
}

static int adc_mchp_g2_disable(adc_registers_t *regs)
{
	/* ANAEN stays set so the analog side keeps its bias and its warm-up. */
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk;

	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_ENABLE_Msk, false,
			      "SYNCBUSY.ENABLE on disable")) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int adc_mchp_g2_enable(adc_registers_t *regs)
{
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk | ADC_CTRLA_ENABLE_Msk;

	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_ENABLE_Msk, false,
			      "SYNCBUSY.ENABLE")) {
		return -ETIMEDOUT;
	}

	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, ADC_CTLINTFLAG_VREFRDY_Msk, true,
			      "CTLINTFLAG.VREFRDY")) {
		return -ETIMEDOUT;
	}

	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, BIT(ADC_CTLINTFLAG_CRRDY_Pos), true,
			      "CTLINTFLAG.CRRDY0")) {
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * CORCTRL.SELRES is enable-protected, so a resolution change is one of the two
 * paths in this driver that has to stop the converter and start it again.
 */
static int adc_mchp_g2_set_resolution(const struct device *dev, uint8_t resolution)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	uint32_t selres;
	int ret;

	if (data->resolution == resolution) {
		return 0;
	}

	switch (resolution) {
	case 8:
		selres = ADC_MCHP_G2_SELRES_8BIT;
		break;
	case 10:
		selres = ADC_MCHP_G2_SELRES_10BIT;
		break;
	case 12:
		selres = ADC_MCHP_G2_SELRES_12BIT;
		break;
	default:
		LOG_ERR("resolution %u is not one of 8, 10 or 12", resolution);
		return -EINVAL;
	}

	ret = adc_mchp_g2_disable(regs);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The write below always takes once ENABLE is confirmed clear; what can
	 * still fail is the re-enable below. If it times out, hardware already
	 * holds this new SELRES while the cache would still name the old one,
	 * so invalidate it now and only restore a real value once enable has
	 * actually succeeded.
	 */
	data->resolution = ADC_MCHP_G2_RESOLUTION_INVALID;

	regs->CONFIG[0].ADC_CORCTRL = ADC_CORCTRL_SAMC(cfg->sample_count) |
				      ADC_CORCTRL_SELRES(selres) |
				      ADC_CORCTRL_ADCDIV(cfg->adc_clk_div);

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	data->resolution = resolution;

	return 0;
}

/*
 * CTRLD.VREFSEL is enable-protected too, so a reference change goes through
 * the same disable, write, re-enable cycle as the resolution above. Callers
 * only reach this while no channel is configured yet; see channel_setup().
 *
 * Two data sheet facts (DS60001916A 36.7.22, CTLINTFLAG) rule out just
 * writing VREFSEL and re-enabling as-is. VREFRDY is a hardware status bit,
 * not write-1-to-clear, and is only updated while CTRLA.ENABLE is set: right
 * after the disable above it still reads back 1 from the old reference, so
 * the post-enable VREFRDY wait would pass immediately without ever seeing
 * the new reference settle. CRRDY0, on the other hand, is cleared by
 * hardware when CTRLD.ANLEN0 is de-asserted. So ANLEN0 is dropped here, both
 * while still disabled, and this function waits for hardware to actually
 * report CRRDY0 clear before setting ANLEN0 again: that forces a real
 * warm-up, and the CRRDY0 wait after re-enable then covers an actual
 * 2^WKUPEXP cycle against the new VREFSEL. CALCTRL is untouched; the factory
 * calibration does not depend on the reference.
 */
static int adc_mchp_g2_set_vref(const struct device *dev, uint32_t vref_sel)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	int ret;

	if (data->vref_sel == vref_sel) {
		return 0;
	}

	ret = adc_mchp_g2_disable(regs);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The writes below always take once ENABLE is confirmed clear; what
	 * can still fail is the re-enable further down. If it times out,
	 * hardware already holds this new VREFSEL while the cache would still
	 * name the old one, so invalidate it now and only restore a real value
	 * once enable has actually succeeded.
	 */
	data->vref_sel = ADC_MCHP_G2_VREFSEL_INVALID;

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) | ADC_CTRLD_VREFSEL(vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_CHNEN0_Msk;

	/*
	 * Wait for hardware to actually clear CRRDY0 before asserting ANLEN0
	 * again below. Without this, a CRRDY0 still set from before the
	 * disable could be mistaken by the post-enable wait for the new
	 * warm-up having already completed.
	 */
	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, BIT(ADC_CTLINTFLAG_CRRDY_Pos), false,
			      "CTLINTFLAG.CRRDY0 clearing")) {
		return -ETIMEDOUT;
	}

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) | ADC_CTRLD_VREFSEL(vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_ANLEN0_Msk |
			  ADC_CTRLD_CHNEN0_Msk;

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	data->vref_sel = vref_sel;

	return 0;
}

/*
 * Every check that can reject the call runs before the one hardware write
 * this function can make (the reference reprogram at the very end), so a
 * rejected channel_setup() never leaves VREFSEL touched.
 */
static int adc_mchp_g2_channel_setup_locked(const struct device *dev,
					     const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	uint32_t wanted_vref;
	int ret;

	/*
	 * A channel that has no devicetree node has no pad configured and no
	 * trigger programmed, and neither can be fixed at runtime.
	 */
	if (channel_cfg->channel_id >= cfg->num_channels ||
	    (cfg->declared_channels & BIT(channel_cfg->channel_id)) == 0) {
		LOG_ERR("channel %u is not declared in devicetree", channel_cfg->channel_id);
		return -EINVAL;
	}

	/*
	 * -EINVAL, not -ENOTSUP: adc_channel_setup() documents no other failure
	 * code, and tests/drivers/adc/adc_error_cases holds every driver to it.
	 */
	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("this converter has no input gain stage");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential inputs are not implemented");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		wanted_vref = ADC_MCHP_G2_VREFSEL_AVDD;
		break;
	case ADC_REF_EXTERNAL0:
		wanted_vref = ADC_MCHP_G2_VREFSEL_VREFH;
		break;
	default:
		LOG_ERR("reference must be ADC_REF_VDD_1 (AVDD) or ADC_REF_EXTERNAL0 (VREFH)");
		return -EINVAL;
	}

	/*
	 * VREFSEL is a property of the SAR core, not of a channel, and it is
	 * enable-protected on top of that: reprogramming it once another
	 * channel is already running against the old value would retime every
	 * sample already in flight against it. So the first channel_setup()
	 * picks the reference for the life of the device; every later one
	 * either agrees or is refused, before anything is touched.
	 */
	if (wanted_vref != data->vref_sel && data->configured_channels != 0) {
		LOG_ERR("reference does not match the one already programmed");
		return -EINVAL;
	}

	/*
	 * CORCTRL.SAMC is a property of the SAR core, not of a channel, and it
	 * is enable-protected on top of that. Accepting a per-channel value
	 * would silently retime every other channel, so the devicetree owns it
	 * and a channel may only agree with it. The real sampling window is
	 * (SAMC + 2) TAD, not SAMC alone.
	 */
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT &&
	    channel_cfg->acquisition_time !=
		    ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, cfg->sample_count + 2)) {
		LOG_ERR("acquisition time belongs to the core, set sample-count instead");
		return -EINVAL;
	}

	if (wanted_vref != data->vref_sel) {
		ret = adc_mchp_g2_set_vref(dev, wanted_vref);
		if (ret < 0) {
			return ret;
		}
	}

	data->configured_channels |= BIT(channel_cfg->channel_id);

	return 0;
}

/*
 * channel_setup() can disable and re-enable the converter (the reference
 * reprogram above), which races a read in flight or a second channel_setup()
 * on another thread just like a resolution change already does inside
 * start_read(). adc_context's lock is what start_read() takes too, so it
 * serializes against both.
 */
static int adc_mchp_g2_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mchp_g2_channel_setup_locked(dev, channel_cfg);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_mchp_g2_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_g2_data *data = dev->data;
	uint32_t needed;
	int ret;

	if (sequence->channels == 0 || (sequence->channels & ~data->configured_channels) != 0) {
		LOG_ERR("sequence names channels 0x%04x that were never set up",
			sequence->channels & ~data->configured_channels);
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("hardware oversampling is not implemented");
		return -EINVAL;
	}

	/*
	 * sequence->calibrate is deliberately not rejected. This converter takes
	 * its calibration from OTP at init and has nothing to run on demand, and
	 * the adc_sequence documentation says implementations that do not
	 * support calibration shall ignore the flag rather than fail the read.
	 */

	needed = POPCOUNT(sequence->channels) * sizeof(uint16_t);
	if (sequence->options != NULL) {
		needed *= 1 + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("buffer holds %zu bytes, the sequence needs %u", sequence->buffer_size,
			needed);
		return -ENOMEM;
	}

	ret = adc_mchp_g2_set_resolution(dev, sequence->resolution);
	if (ret < 0) {
		return ret;
	}

	data->active_channels = sequence->channels;
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_mchp_g2_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mchp_g2_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mchp_g2_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_mchp_g2_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mchp_g2_data *data = CONTAINER_OF(ctx, struct adc_mchp_g2_data, ctx);
	const struct adc_mchp_g2_config *cfg = data->dev->config;
	adc_registers_t *regs = cfg->regs;
	uint32_t chrdy = (uint32_t)cfg->declared_channels << ADC_INTFLAG_CHRDY_Pos;

	data->buffer_repeat = data->buffer;
	data->pending_channels = cfg->declared_channels;

	/* A flag left over from an aborted conversion would end this one early. */
	regs->INT[0].ADC_INTFLAG = chrdy;
	regs->INT[0].ADC_INTENSET = chrdy;

	/*
	 * CTRLB is write-synchronized: a GSWTRG written while SYNCBUSY.CTRLB is
	 * still set from a previous write is discarded rather than queued.
	 */
	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_CTRLB_Msk, false,
			      "SYNCBUSY.CTRLB")) {
		adc_context_complete(ctx, -ETIMEDOUT);
		return;
	}

	/*
	 * GSWTRG is bit 8. Bit 0 is ADCHSEL, so the obvious write of 1 selects a
	 * channel for the software-conversion debug path and triggers nothing.
	 */
	regs->ADC_CTRLB = ADC_CTRLB_GSWTRG_Msk;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mchp_g2_data *data = CONTAINER_OF(ctx, struct adc_mchp_g2_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_repeat;
	}
}

/*
 * The per-channel CHRDY interrupt.
 *
 * One GSWTRG converts every declared channel, so this fires once per channel
 * and the conversion is over when the last of them has reported. Results are
 * read here rather than handed to a work item: the read is a register write
 * and a register read per channel, at most num-channels of them.
 */
static void adc_mchp_g2_isr_core0(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	uint32_t flags = regs->INT[0].ADC_INTFLAG;
	uint16_t ready = (flags & ADC_INTFLAG_CHRDY_Msk) >> ADC_INTFLAG_CHRDY_Pos;

	if (ready == 0) {
		/* Something other than a channel result. Clear it and move on. */
		regs->INT[0].ADC_INTFLAG = flags;
		return;
	}

	regs->INT[0].ADC_INTFLAG = (uint32_t)ready << ADC_INTFLAG_CHRDY_Pos;
	data->pending_channels &= ~ready;

	if (data->pending_channels != 0) {
		return;
	}

	regs->INT[0].ADC_INTENCLR = (uint32_t)cfg->declared_channels << ADC_INTFLAG_CHRDY_Pos;

	for (uint8_t k = 0; k < cfg->num_channels; k++) {
		if ((data->active_channels & BIT(k)) == 0) {
			continue;
		}

		regs->ADC_CORCHDATAID = ADC_CORCHDATAID_CORDYID(0) | ADC_CORCHDATAID_CHRDYID(k);

		/*
		 * A lower resolution stops the successive approximation early
		 * and leaves the unresolved bits at zero; it does not move the
		 * result down. Measured: with SELRES at 8 bits, a reading of
		 * 115 arrives as 0x730, which is 115 shifted up by four. The
		 * data sheet only ever documents the 12-bit alignment, so this
		 * shift is the part that has to come from the board.
		 */
		*data->buffer++ = (regs->ADC_CHRDYDAT & ADC_MCHP_G2_RESULT_MASK) >>
				  (ADC_MCHP_G2_RESULT_BITS - data->resolution);
	}

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

/*
 * The controller interrupt: VREFRDY, core ready and the FIFO flags. Nothing
 * here enables any of them, so reaching this handler means something else did.
 * Disable what fired rather than spinning in it.
 */
static void adc_mchp_g2_isr_global(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	uint32_t flags = cfg->regs->ADC_CTLINTFLAG;

	LOG_WRN("unexpected controller interrupt, CTLINTFLAG 0x%08x", flags);

	cfg->regs->ADC_CTLINTENCLR = flags;
	cfg->regs->ADC_CTLINTFLAG = flags;
}

static int adc_mchp_g2_init(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	int ret;

	data->dev = dev;

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("cannot enable the APB clock: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->gclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("cannot enable the generic clock: %d", ret);
		return ret;
	}

	/*
	 * The binding does not require pinctrl-0: every channel this converter
	 * reaches is an analog pad, but nothing stops a board from wiring none
	 * of them up yet. -ENOENT means the node names no default state at
	 * all, which is fine; anything else is a real pinctrl failure.
	 */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("cannot apply pinctrl: %d", ret);
		return ret;
	}

	regs->ADC_CTRLA = ADC_CTRLA_SWRST_Msk;
	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_SWRST_Msk, false,
			      "SYNCBUSY.SWRST")) {
		return -ETIMEDOUT;
	}

	/*
	 * Order matters here and the data sheet is explicit about all of it.
	 * CTRLA.ANAEN has to be set before any CTRLD.ANLENn (36.7.4 bit 20
	 * note 2), and CALCTRL can only be written while ANLEN0 is still zero
	 * (36.7.11). So: analog enable, then calibration, then the register
	 * that turns the analog bias on.
	 */
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk;

	regs->CONFIG[0].ADC_CALCTRL = cfg->fuses->FUSES_FCCFG65;

	data->vref_sel = ADC_MCHP_G2_VREFSEL_AVDD;

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) |
			  ADC_CTRLD_VREFSEL(data->vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_ANLEN0_Msk |
			  ADC_CTRLD_CHNEN0_Msk;

	regs->CONFIG[0].ADC_CORCTRL = ADC_CORCTRL_SAMC(cfg->sample_count) |
				      ADC_CORCTRL_SELRES(ADC_MCHP_G2_SELRES_12BIT) |
				      ADC_CORCTRL_ADCDIV(cfg->adc_clk_div);

	/*
	 * Every channel single-ended, unsigned, right-aligned and in no scan.
	 * The trigger assignment is the one thing that varies, and it is fixed
	 * here for good: these registers cannot be touched again until the
	 * converter is disabled.
	 */
	regs->CONFIG[0].ADC_CHNCFG1 = 0;
	regs->CONFIG[0].ADC_CHNCFG2 = 0;
	regs->CONFIG[0].ADC_CHNCFG3 = 0;
	adc_mchp_g2_program_triggers(regs, cfg->declared_channels);

	data->resolution = 12;

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	cfg->irq_config();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_mchp_g2_api) = {
	.channel_setup = adc_mchp_g2_channel_setup,
	.read = adc_mchp_g2_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mchp_g2_read_async,
#endif
};

#define ADC_MCHP_G2_CHANNEL_BIT(node) | BIT(DT_REG_ADDR(node))

#define ADC_MCHP_G2_DECLARED_CHANNELS(n)                                                         \
	(0 DT_INST_FOREACH_CHILD_STATUS_OKAY(n, ADC_MCHP_G2_CHANNEL_BIT))

/* A channel's reg must name one of the pads this instance actually reaches. */
#define ADC_MCHP_G2_CHANNEL_ASSERT(node)                                                         \
	BUILD_ASSERT(DT_REG_ADDR(node) < DT_PROP(DT_PARENT(node), num_channels),                  \
		     "channel reg must be less than num-channels");

#define ADC_MCHP_G2_INIT(n)                                                                       \
	BUILD_ASSERT(DT_INST_PROP(n, num_channels) <= 16, "num-channels must be at most 16");    \
	BUILD_ASSERT(DT_INST_PROP(n, ctrl_clock_div) <= 63,                                       \
		     "ctrl-clock-div must be between 0 and 63");                                  \
	BUILD_ASSERT(DT_INST_PROP(n, adc_div_ratio) >= 1 && DT_INST_PROP(n, adc_div_ratio) <= 127,\
		     "adc-div-ratio must be between 1 and 127");                                  \
	BUILD_ASSERT(DT_INST_PROP(n, adc_wkup_clock_count) <= 15,                                 \
		     "adc-wkup-clock-count must be between 0 and 15");                            \
	BUILD_ASSERT(DT_INST_PROP(n, sample_count) <= 1023,                                       \
		     "sample-count must be between 0 and 1023");                                  \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, ADC_MCHP_G2_CHANNEL_ASSERT)                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                \
                                                                                                   \
	static void adc_mchp_g2_irq_config_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, global, irq),                                  \
			    DT_INST_IRQ_BY_NAME(n, global, priority), adc_mchp_g2_isr_global,     \
			    DEVICE_DT_INST_GET(n), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, global, irq));                                  \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, core0, irq),                                   \
			    DT_INST_IRQ_BY_NAME(n, core0, priority), adc_mchp_g2_isr_core0,      \
			    DEVICE_DT_INST_GET(n), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, core0, irq));                                   \
	}                                                                                          \
                                                                                                   \
	static const struct adc_mchp_g2_config adc_mchp_g2_config_##n = {                          \
		.regs = (adc_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.fuses = (fuses_calotp_registers_t *)DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_calib)),  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                        \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                  \
		.mclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),              \
		.gclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),              \
		.irq_config = adc_mchp_g2_irq_config_##n,                                         \
		.declared_channels = ADC_MCHP_G2_DECLARED_CHANNELS(n),                            \
		.num_channels = DT_INST_PROP(n, num_channels),                                    \
		.ctl_clk_div = DT_INST_PROP(n, ctrl_clock_div),                                   \
		.adc_clk_div = DT_INST_PROP(n, adc_div_ratio),                                    \
		.sample_count = DT_INST_PROP(n, sample_count),                                    \
		.wakeup_exp = DT_INST_PROP(n, adc_wkup_clock_count),                              \
	};                                                                                         \
                                                                                                   \
	static struct adc_mchp_g2_data adc_mchp_g2_data_##n = {                                    \
		ADC_CONTEXT_INIT_TIMER(adc_mchp_g2_data_##n, ctx),                                \
		ADC_CONTEXT_INIT_LOCK(adc_mchp_g2_data_##n, ctx),                                 \
		ADC_CONTEXT_INIT_SYNC(adc_mchp_g2_data_##n, ctx),                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_mchp_g2_init, NULL,                                          \
			      &adc_mchp_g2_data_##n, &adc_mchp_g2_config_##n, POST_KERNEL,        \
			      CONFIG_ADC_INIT_PRIORITY, &adc_mchp_g2_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_MCHP_G2_INIT)
