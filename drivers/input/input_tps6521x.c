/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Power button on the EN/PB/VSENSE pin of the TI TPS65214 (SLVSHK7A) and
 * TPS65215 (SLVSHQ9), reported from the INT_PB register.
 */

#define DT_DRV_COMPAT ti_tps6521x_power_button

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_tps6521x, CONFIG_INPUT_LOG_LEVEL);

/* MFP_2_CONFIG EN_PB_VSENSE_CONFIG; the push-button encoding differs */
#define TPS6521X_EN_PB_VSENSE_CONFIG GENMASK(5, 4)
#define TPS65214_EN_PB_VSENSE_PB     0x0U
#define TPS65215_EN_PB_VSENSE_PB     0x1U

/* MASK_CONFIG MASK_INT_FOR_PB, 1 = push-button events do not assert nINT */
#define TPS6521X_MASK_INT_FOR_PB BIT(7)

struct input_tps6521x_config {
	const struct device *mfd;
	uint16_t code;
};

struct input_tps6521x_data {
	const struct device *dev;
	struct mfd_tps6521x_int_callback cb;
};

static void input_tps6521x_report(const struct device *dev, bool pressed)
{
	const struct input_tps6521x_config *config = dev->config;

	input_report_key(dev, config->code, pressed ? 1 : 0, true, K_FOREVER);
}

static void input_tps6521x_handler(const struct device *mfd, struct mfd_tps6521x_int_callback *cb,
				   uint8_t value)
{
	struct input_tps6521x_data *data = CONTAINER_OF(cb, struct input_tps6521x_data, cb);
	bool pressed_edge = (value & MFD_TPS6521X_INT_PB_FALLING_EDGE) != 0U;
	bool released_edge = (value & MFD_TPS6521X_INT_PB_RISING_EDGE) != 0U;
	bool pressed_now = (value & MFD_TPS6521X_INT_PB_REAL_TIME_STATUS) == 0U;

	ARG_UNUSED(mfd);

	if (pressed_edge && released_edge) {
		/*
		 * Both edges since the last clear: the real-time status tells
		 * which came last.
		 */
		input_tps6521x_report(data->dev, !pressed_now);
		input_tps6521x_report(data->dev, pressed_now);
	} else if (pressed_edge) {
		input_tps6521x_report(data->dev, true);
	} else {
		input_tps6521x_report(data->dev, false);
	}
}

static int input_tps6521x_init(const struct device *dev)
{
	const struct input_tps6521x_config *config = dev->config;
	struct input_tps6521x_data *data = dev->data;
	uint8_t pb_value;
	uint8_t mfp;
	int ret;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_MFP_2_CONFIG, &mfp);
	if (ret < 0) {
		return ret;
	}

	/* the pin function is NVM-only: "do not change via I2C after NVM load" */
	pb_value = mfd_tps6521x_get_variant(config->mfd) == MFD_TPS6521X_VARIANT_TPS65215
			   ? TPS65215_EN_PB_VSENSE_PB
			   : TPS65214_EN_PB_VSENSE_PB;
	if (FIELD_GET(TPS6521X_EN_PB_VSENSE_CONFIG, mfp) != pb_value) {
		LOG_ERR("EN/PB/VSENSE pin is not configured as a push button");
		return -ENOTSUP;
	}

	data->dev = dev;
	data->cb.handler = input_tps6521x_handler;
	data->cb.reg = MFD_TPS6521X_REG_INT_PB;
	data->cb.mask = MFD_TPS6521X_INT_PB_FALLING_EDGE | MFD_TPS6521X_INT_PB_RISING_EDGE;

	ret = mfd_tps6521x_add_int_callback(config->mfd, &data->cb);
	if (ret < 0) {
		LOG_ERR("no PMIC interrupt (%d)", ret);
		return ret;
	}

	/* the NVM may mask PB events; the datasheets give no default per part */
	ret = mfd_tps6521x_reg_update(config->mfd, MFD_TPS6521X_REG_MASK_CONFIG,
				      TPS6521X_MASK_INT_FOR_PB, 0U);
	if (ret < 0) {
		(void)mfd_tps6521x_remove_int_callback(config->mfd, &data->cb);
		return ret;
	}

	return 0;
}

#define INPUT_TPS6521X_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST_PARENT(inst), int_gpios),                            \
		     "the TPS6521x power button needs int-gpios on the PMIC node");                \
	BUILD_ASSERT(IS_ENABLED(CONFIG_MFD_TPS6521X_INTERRUPT),                                    \
		     "the TPS6521x power button needs CONFIG_GPIO for the PMIC interrupt");        \
	static const struct input_tps6521x_config input_tps6521x_config_##inst = {                 \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.code = DT_INST_PROP(inst, zephyr_code),                                           \
	};                                                                                         \
	static struct input_tps6521x_data input_tps6521x_data_##inst;                              \
	DEVICE_DT_INST_DEFINE(inst, input_tps6521x_init, NULL, &input_tps6521x_data_##inst,        \
			      &input_tps6521x_config_##inst, POST_KERNEL,                          \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_TPS6521X_DEFINE)
