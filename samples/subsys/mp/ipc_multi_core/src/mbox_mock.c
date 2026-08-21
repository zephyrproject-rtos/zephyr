#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/ipm.h>

#define DT_DRV_COMPAT zephyr_mbox_mock

struct mbox_mock_data {
	const struct device *ipm_dev;
	mbox_callback_t cb;
	void *user_data;
};

static void ipm_callback_shim(const struct device *ipmdev, void *user_data, uint32_t id, volatile void *data)
{
	struct mbox_mock_data *drv_data = user_data;

	if (drv_data->cb) {
		drv_data->cb(NULL, 0, drv_data->user_data, NULL);
		drv_data->cb(NULL, 1, drv_data->user_data, NULL);
	}
}

static int mbox_mock_send(const struct device *dev, mbox_channel_id_t channel_id, const struct mbox_msg *msg)
{
	struct mbox_mock_data *drv_data = dev->data;

	if (!drv_data->ipm_dev) {
		drv_data->ipm_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ipc));
		if (!drv_data->ipm_dev) {
			return -ENODEV;
		}
	}

#if defined(CONFIG_BOARD_MPS2_AN521_CPU0)
	uint32_t target_cpu = 1; /* CPU1 */
#else
	uint32_t target_cpu = 0; /* CPU0 */
#endif

	return ipm_send(drv_data->ipm_dev, 0, target_cpu, NULL, 0);
}

static int mbox_mock_register_callback(const struct device *dev, mbox_channel_id_t channel_id, mbox_callback_t cb, void *user_data)
{
	struct mbox_mock_data *drv_data = dev->data;

	drv_data->cb = cb;
	drv_data->user_data = user_data;
	if (!drv_data->ipm_dev) {
		drv_data->ipm_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ipc));
		if (!drv_data->ipm_dev) {
			return -ENODEV;
		}
	}
	ipm_register_callback(drv_data->ipm_dev, ipm_callback_shim, drv_data);
	return 0;
}

static int mbox_mock_mtu_get(const struct device *dev)
{
	return 0;
}

static uint32_t mbox_mock_max_channels_get(const struct device *dev)
{
	return 2;
}

static int mbox_mock_set_enabled(const struct device *dev, mbox_channel_id_t channel_id, bool enabled)
{
	struct mbox_mock_data *drv_data = dev->data;

	if (!drv_data->ipm_dev) {
		drv_data->ipm_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ipc));
		if (!drv_data->ipm_dev) {
			return -ENODEV;
		}
	}
	return ipm_set_enabled(drv_data->ipm_dev, enabled ? 1 : 0);
}

static const struct mbox_driver_api mbox_mock_api = {
	.send = mbox_mock_send,
	.register_callback = mbox_mock_register_callback,
	.mtu_get = mbox_mock_mtu_get,
	.max_channels_get = mbox_mock_max_channels_get,
	.set_enabled = mbox_mock_set_enabled,
};

static int mbox_mock_init(const struct device *dev)
{
	return 0;
}

#define MBOX_MOCK_INIT(inst) \
	static struct mbox_mock_data mbox_mock_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, mbox_mock_init, NULL, &mbox_mock_data_##inst, NULL, \
			      POST_KERNEL, 2, &mbox_mock_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_MOCK_INIT)