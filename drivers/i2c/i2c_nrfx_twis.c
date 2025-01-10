/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <nrfx_twis.h>
#include <string.h>

#define DT_DRV_COMPAT nordic_nrf_twis

#define SHIM_NRF_TWIS_NODE(id) \
	DT_NODELABEL(_CONCAT(i2c, id))

#define SHIM_NRF_TWIS_DEVICE_GET(id) \
	DEVICE_DT_GET(SHIM_NRF_TWIS_NODE(id))

#define SHIM_NRF_TWIS_IRQ_HANDLER(id) \
	_CONCAT_3(nrfx_twis_, id, _irq_handler)

#define SHIM_NRF_TWIS_IRQN(id) \
	DT_IRQN(SHIM_NRF_TWIS_NODE(id))

#define SHIM_NRF_TWIS_IRQ_PRIO(id) \
	DT_IRQ(SHIM_NRF_TWIS_NODE(id), priority)

#define SHIM_NRF_TWIS_HAS_MEMORY_REGIONS(id) \
	DT_NODE_HAS_PROP(SHIM_NRF_TWIS_NODE(id), memory_regions)

#define SHIM_NRF_TWIS_LINKER_REGION_NAME(id) \
	LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(SHIM_NRF_TWIS_NODE(id), memory_regions))

#define SHIM_NRF_TWIS_BUF_ATTR_SECTION(id) \
	__attribute__((__section__(SHIM_NRF_TWIS_LINKER_REGION_NAME(id))))

#define SHIM_NRF_TWIS_BUF_ATTR(id)								\
	COND_CODE_1(										\
		SHIM_NRF_TWIS_HAS_MEMORY_REGIONS(id),						\
		(SHIM_NRF_TWIS_BUF_ATTR_SECTION(id)),						\
		()										\
	)

#define SHIM_NRF_TWIS_BUF_SIZE \
	CONFIG_I2C_NRFX_TWIS_BUF_SIZE

LOG_MODULE_REGISTER(i2c_nrfx_twis, CONFIG_I2C_LOG_LEVEL);

struct shim_nrf_twis_config {
	nrfx_twis_t twis;
	void (*irq_connect)(void);
	void (*event_handler)(nrfx_twis_evt_t const *event);
	const struct pinctrl_dev_config *pcfg;
	uint8_t *buf;
};

struct shim_nrf_twis_data {
	struct i2c_target_config *target_config;
	bool enabled;
};

#if CONFIG_PM_DEVICE
static bool shim_nrf_twis_is_resumed(const struct device *dev)
{
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_ACTIVE;
}
#else
static bool shim_nrf_twis_is_resumed(const struct device *dev)
{
	ARG_UNUSED(dev);

	return true;
}
#endif

static void shim_nrf_twis_enable(const struct device *dev)
{
	struct shim_nrf_twis_data *dev_data = dev->data;
	const struct shim_nrf_twis_config *dev_config = dev->config;

	if (dev_data->enabled) {
		return;
	}

	if (dev_data->target_config == NULL) {
		return;
	}

	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	nrfx_twis_enable(&dev_config->twis);
	dev_data->enabled = true;
}

static void shim_nrf_twis_disable(const struct device *dev)
{
	struct shim_nrf_twis_data *dev_data = dev->data;
	const struct shim_nrf_twis_config *dev_config = dev->config;

	if (!dev_data->enabled) {
		return;
	}

	dev_data->enabled = false;
	nrfx_twis_disable(&dev_config->twis);
	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
}

static void shim_nrf_twis_handle_read_req(const struct device *dev)
{
	struct shim_nrf_twis_data *dev_data = dev->data;
	const struct shim_nrf_twis_config *dev_config = dev->config;
	struct i2c_target_config *target_config = dev_data->target_config;
	const struct i2c_target_callbacks *callbacks = target_config->callbacks;
	const nrfx_twis_t *twis = &dev_config->twis;
	uint8_t *buf;
	uint32_t buf_size;
	nrfx_err_t err;

	if (callbacks->buf_read_requested(target_config, &buf, &buf_size)) {
		LOG_ERR("no buffer provided");
		return;
	}

	if (SHIM_NRF_TWIS_BUF_SIZE < buf_size) {
		LOG_ERR("provided buffer too large");
		return;
	}

	memcpy(dev_config->buf, buf, buf_size);

	err = nrfx_twis_tx_prepare(twis, dev_config->buf, buf_size);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("tx prepare failed");
		return;
	}
}

static void shim_nrf_twis_handle_write_req(const struct device *dev)
{
	const struct shim_nrf_twis_config *dev_config = dev->config;
	const nrfx_twis_t *twis = &dev_config->twis;
	nrfx_err_t err;

	err = nrfx_twis_rx_prepare(twis, dev_config->buf, SHIM_NRF_TWIS_BUF_SIZE);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("rx prepare failed");
		return;
	}
}

static void shim_nrf_twis_handle_write_done(const struct device *dev)
{
	struct shim_nrf_twis_data *dev_data = dev->data;
	const struct shim_nrf_twis_config *dev_config = dev->config;
	struct i2c_target_config *target_config = dev_data->target_config;
	const struct i2c_target_callbacks *callbacks = target_config->callbacks;
	const nrfx_twis_t *twis = &dev_config->twis;

	callbacks->buf_write_received(target_config, dev_config->buf, nrfx_twis_rx_amount(twis));
}

static void shim_nrf_twis_event_handler(const struct device *dev,
					nrfx_twis_evt_t const *event)
{
	switch (event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		shim_nrf_twis_handle_read_req(dev);
		break;

	case NRFX_TWIS_EVT_WRITE_REQ:
		shim_nrf_twis_handle_write_req(dev);
		break;

	case NRFX_TWIS_EVT_WRITE_DONE:
		shim_nrf_twis_handle_write_done(dev);
		break;

	default:
		break;
	}
}

static int shim_nrf_twis_pm_action_cb(const struct device *dev,
				      enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		shim_nrf_twis_enable(dev);
		break;

#if CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		shim_nrf_twis_disable();
		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int shim_nrf_twis_target_register(const struct device *dev,
					 struct i2c_target_config *target_config)
{
	struct shim_nrf_twis_data *dev_data = dev->data;
	const struct shim_nrf_twis_config *dev_config = dev->config;
	const nrfx_twis_t *twis = &dev_config->twis;
	nrfx_err_t err;
	const nrfx_twis_config_t config = {
		.addr = {
			target_config->address,
		},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	if (target_config->flags) {
		LOG_ERR("16-bit address unsupported");
		return -EINVAL;
	}

	shim_nrf_twis_disable(dev);

	err = nrfx_twis_reconfigure(twis, &config);
	if (err != NRFX_SUCCESS) {
		return -ENODEV;
	}

	dev_data->target_config = target_config;

	if (shim_nrf_twis_is_resumed(dev)) {
		shim_nrf_twis_enable(dev);
	}

	return 0;
}

static int shim_nrf_twis_target_unregister(const struct device *dev,
					   struct i2c_target_config *target_config)
{
	struct shim_nrf_twis_data *dev_data = dev->data;

	if (dev_data->target_config != target_config) {
		return -EINVAL;
	}

	shim_nrf_twis_disable(dev);
	dev_data->target_config = NULL;
	return 0;
}

const struct i2c_driver_api shim_nrf_twis_api = {
	.target_register = shim_nrf_twis_target_register,
	.target_unregister = shim_nrf_twis_target_unregister,
};

static int shim_nrf_twis_init(const struct device *dev)
{
	const struct shim_nrf_twis_config *dev_config = dev->config;
	nrfx_err_t err;
	const nrfx_twis_config_t config = {
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	err = nrfx_twis_init(&dev_config->twis, &config, dev_config->event_handler);
	if (err != NRFX_SUCCESS) {
		return -ENODEV;
	}

	dev_config->irq_connect();
	return pm_device_driver_init(dev, shim_nrf_twis_pm_action_cb);
}

#define SHIM_NRF_TWIS_NAME(id, name) \
	_CONCAT_4(shim_nrf_twis_, name, _, id)

#define SHIM_NRF_TWIS_DEVICE_DEFINE(id)								\
	static void SHIM_NRF_TWIS_NAME(id, irq_connect)(void)					\
	{											\
		IRQ_CONNECT(									\
			SHIM_NRF_TWIS_IRQN(id),							\
			SHIM_NRF_TWIS_IRQ_PRIO(id),						\
			nrfx_isr,								\
			SHIM_NRF_TWIS_IRQ_HANDLER(id),						\
			0									\
		);										\
	}											\
												\
	static void SHIM_NRF_TWIS_NAME(id, event_handler)(nrfx_twis_evt_t const *event)		\
	{											\
		shim_nrf_twis_event_handler(SHIM_NRF_TWIS_DEVICE_GET(id), event);		\
	}											\
												\
	static struct shim_nrf_twis_data SHIM_NRF_TWIS_NAME(id, data);				\
												\
	PINCTRL_DT_DEFINE(SHIM_NRF_TWIS_NODE(id));						\
												\
	static uint8_t SHIM_NRF_TWIS_NAME(id, buf)						\
		[SHIM_NRF_TWIS_BUF_SIZE] SHIM_NRF_TWIS_BUF_ATTR(id);				\
												\
	static const struct shim_nrf_twis_config SHIM_NRF_TWIS_NAME(id, config) = {		\
		.twis = NRFX_TWIS_INSTANCE(id),							\
		.irq_connect = SHIM_NRF_TWIS_NAME(id, irq_connect),				\
		.event_handler = SHIM_NRF_TWIS_NAME(id, event_handler),				\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SHIM_NRF_TWIS_NODE(id)),			\
		.buf = SHIM_NRF_TWIS_NAME(id, buf),						\
	};											\
												\
	PM_DEVICE_DT_DEFINE(									\
		SHIM_NRF_TWIS_NODE(id),								\
		shim_nrf_twis_pm_action_cb,							\
	);											\
												\
	DEVICE_DT_DEFINE(									\
		SHIM_NRF_TWIS_NODE(id),								\
		shim_nrf_twis_init,								\
		PM_DEVICE_DT_GET(SHIM_NRF_TWIS_NODE(id)),					\
		&SHIM_NRF_TWIS_NAME(id, data),							\
		&SHIM_NRF_TWIS_NAME(id, config),						\
		POST_KERNEL,									\
		CONFIG_I2C_INIT_PRIORITY,							\
		&shim_nrf_twis_api								\
	);

#ifdef CONFIG_HAS_HW_NRF_TWIS0
SHIM_NRF_TWIS_DEVICE_DEFINE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS1
SHIM_NRF_TWIS_DEVICE_DEFINE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS2
SHIM_NRF_TWIS_DEVICE_DEFINE(2);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS3
SHIM_NRF_TWIS_DEVICE_DEFINE(3);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS20
SHIM_NRF_TWIS_DEVICE_DEFINE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS21
SHIM_NRF_TWIS_DEVICE_DEFINE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS22
SHIM_NRF_TWIS_DEVICE_DEFINE(22);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS30
SHIM_NRF_TWIS_DEVICE_DEFINE(30);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS130
SHIM_NRF_TWIS_DEVICE_DEFINE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS131
SHIM_NRF_TWIS_DEVICE_DEFINE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS133
SHIM_NRF_TWIS_DEVICE_DEFINE(133);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS134
SHIM_NRF_TWIS_DEVICE_DEFINE(134);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS135
SHIM_NRF_TWIS_DEVICE_DEFINE(135);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS136
SHIM_NRF_TWIS_DEVICE_DEFINE(136);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIS137
SHIM_NRF_TWIS_DEVICE_DEFINE(137);
#endif
