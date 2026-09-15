/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63.c
 * @brief CS47L63 audio codec driver (SPI, audio_codec API)
 *
 * Class-API glue. Every register access happens in one of the units this file
 * calls: cs47l63_boot.c (reset, identify, trim), cs47l63_clock.c (FLL1 and
 * SYSCLK), cs47l63_dai.c (ASP1), cs47l63_out.c (mixer, amplifier, volume) and
 * cs47l63_fault.c, all of which reach the chip through cs47l63_bus.c.
 *
 * The part's DSP cores and loadable firmware are not supported; the DSP-memory
 * patch the vendor driver applies alongside the trim block is therefore not
 * applied here.
 */

#define DT_DRV_COMPAT cirrus_cs47l63

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include "cs47l63_boot.h"
#include "cs47l63_bus.h"
#include "cs47l63_clock.h"
#include "cs47l63_dai.h"
#include "cs47l63_fault.h"
#include "cs47l63_in.h"
#include "cs47l63_out.h"
#include "cs47l63_priv.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cs47l63);

/**
 * @brief Read the FLL lock and finish the amplifier enable, once both can work.
 *
 * Deferred rather than done inline because the FLL's reference is the SoC's
 * I2S master clock: that pin starts with the I2S transfer, which the caller
 * sets up after it has told this codec to play. At every point the class API
 * is entered there is therefore no clock at all - nothing for the loop to lock
 * to, and no SYSCLK for the output stage to come up on. The only honest moment
 * for either is a while after start_output returned.
 *
 * A locked loop and an enabled amplifier say nothing. An unlocked loop is the
 * whole diagnostic for a reference that never arrived; an amplifier that is
 * still not enabled once the clock is there is a dead output stage. Both are
 * latched as faults, so neither can pass as silence with no explanation.
 */
static void start_check_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cs47l63_data *data = CONTAINER_OF(dwork, struct cs47l63_data, start_check);
	const struct device *dev = data->dev;
	bool locked;
	int ret;

	ret = cs47l63_clock_locked(dev, &locked);
	if (ret < 0) {
		LOG_ERR("Failed to read FLL1 lock state: %d", ret);
	} else if (!locked) {
		LOG_ERR("FLL1 still unlocked %d ms after output start - no MCLK1 reference; "
			"the output stage is running on a clock that never started",
			CS47L63_FLL_LOCK_SETTLE_MS);
		cs47l63_fault_raise(dev, CS47L63_ERROR_CLOCK);
	}

	/* Attempted whatever the lock said: the amplifier is what the listener
	 * hears, and its own status bit is a better witness than an inference
	 * from the loop's.
	 */
	ret = cs47l63_out_confirm_start(dev);
	if (ret < 0) {
		LOG_ERR("Output stage never came up %d ms after start: %d",
			CS47L63_FLL_LOCK_SETTLE_MS, ret);
		cs47l63_fault_raise(dev, CS47L63_ERROR_OUTPUT);
	}
}

static int codec_initialize(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	struct cs47l63_data *data = dev->data;

	if (!cs47l63_bus_is_ready(dev)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		LOG_ERR("IRQ GPIO not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init_delayable(&data->start_check, start_check_work);

	return 0;
}

/**
 * @brief Leave the output stage disabled after a failed configure.
 *
 * A configure that gives up partway must not leave a live amplifier behind on
 * a part whose clock or serial port is in an unknown state.
 */
static int configure_failed(const struct device *dev, int ret)
{
	(void)cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);

	return ret;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct cs47l63_fll_solution clock_sol;
	struct cs47l63_dai_solution dai_sol;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	/* Solve the clock and the serial port before touching any hardware.
	 * Both are pure computation, so an unreachable rate or a request the
	 * part cannot express is refused here - with the solver's own errno
	 * intact - and the part is left exactly as it was.
	 */
	ret = cs47l63_clock_solve(cfg->mclk_freq, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1,
				  &clock_sol);
	if (ret < 0) {
		LOG_ERR("No FLL1 solution for MCLK %u Hz: %d", cfg->mclk_freq, ret);
		return ret;
	}

	ret = cs47l63_dai_solve(cfg->dai_type, &cfg->dai_cfg.i2s, &dai_sol);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_boot_bringup(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_clock_apply(dev, &clock_sol);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_dai_apply(dev, &dai_sol);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	/* Output configured, not started: the mixer is routed and the level
	 * cached, the amplifier stays down until start_output.
	 */
	ret = cs47l63_out_init(dev);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_out_route_output(dev, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_HP);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	/* Same treatment for the capture side: the line pair is left disabled
	 * and muted, so an image that only plays back still leaves the ADC
	 * pins quiet rather than at their reset defaults.
	 */
	ret = cs47l63_in_init(dev);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	ret = cs47l63_in_route_input(dev, AUDIO_CHANNEL_ALL, CS47L63_INPUT_LINE);
	if (ret < 0) {
		return configure_failed(dev, ret);
	}

	return cs47l63_fault_clear(dev);
}

/**
 * @brief Enable the output and arm the check that will finish the job.
 *
 * Shared by the two class entry points that start audio, because the deferred
 * half is not optional: without it the amplifier is enabled on the part but
 * the driver never learns it came up, so the output stays muted forever.
 */
static int start_output_tx(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret = cs47l63_out_start(dev);

	if (ret < 0) {
		return ret;
	}

	/* By the time this runs the caller's stream - and with it the FLL's
	 * reference, SYSCLK and the amplifier - has had time to come up.
	 */
	(void)k_work_reschedule(&data->start_check, K_MSEC(CS47L63_FLL_LOCK_SETTLE_MS));

	return 0;
}

/**
 * @brief Drop a pending start check and take the output down.
 *
 * The cancel is synchronous: a check that ran to completion after the stop
 * would mark the output running and unmute a stage the caller has just
 * disabled.
 */
static int stop_output_tx(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&data->start_check, &sync);

	return cs47l63_out_stop(dev);
}

static void codec_start_output(const struct device *dev)
{
	int ret = start_output_tx(dev);

	if (ret < 0) {
		LOG_ERR("Failed to start output: %d", ret);
	}

	/* Opportunistic fault poll: the driver does not service the part's
	 * interrupt line, so the class operations that already touch the output
	 * stage stand in for one. Best-effort - a bus error here must not
	 * change what start_output did.
	 */
	(void)cs47l63_fault_check(dev);
}

static void codec_stop_output(const struct device *dev)
{
	int ret = stop_output_tx(dev);

	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
	}
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int ret;

	/* One physical output channel, fed by both receive slots. A per-side
	 * level or mute cannot be expressed, so it is refused rather than
	 * applied to the whole output - which is indistinguishable, at the far
	 * end, from the driver having got the channel wrong.
	 */
	if (channel != AUDIO_CHANNEL_ALL) {
		LOG_ERR("Only AUDIO_CHANNEL_ALL is addressable on this part");
		return -EINVAL;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		ret = cs47l63_out_set_volume(dev, val.vol);
		(void)cs47l63_fault_check(dev);
		return ret;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		ret = cs47l63_out_set_mute(dev, val.mute);
		(void)cs47l63_fault_check(dev);
		return ret;
	default:
		break;
	}

	/* Refused, not accepted and dropped: a silently ignored property is
	 * indistinguishable from a hardware fault at the far end.
	 */
	return -ENOTSUP;
}

/**
 * @brief Class API @c apply_properties.
 *
 * Nothing to commit: every property this driver accepts is latched on the part
 * by the volume-update strobe as it is written, so there is no batch waiting
 * here for a commit.
 */
static int codec_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0) {
		ret = cs47l63_in_start(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if ((dir & AUDIO_DAI_DIR_TX) != 0) {
		return start_output_tx(dev);
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0) {
		ret = cs47l63_in_stop(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if ((dir & AUDIO_DAI_DIR_TX) != 0) {
		return stop_output_tx(dev);
	}

	return 0;
}

/* .write and .register_done_callback are left unset: this part has no
 * streaming-write path in the class API contract this driver implements.
 */
static DEVICE_API(audio_codec, codec_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
	.clear_errors = cs47l63_fault_clear,
	.register_error_callback = cs47l63_fault_register_callback,
	.route_input = cs47l63_in_route_input,
	.route_output = cs47l63_out_route_output,
	.start = codec_start,
	.stop = codec_stop,
};

#define CS47L63_DEFINE(inst)                                                                       \
	static struct cs47l63_data cs47l63_data_##inst;                                            \
	static const struct cs47l63_config cs47l63_config_##inst = {                               \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB),             \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                \
		.gpio9_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, gpio9_gpios, {0}),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &cs47l63_data_##inst,                  \
			      &cs47l63_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS47L63_DEFINE)
