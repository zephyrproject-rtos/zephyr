/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbh_dfu.h>
#include <zephyr/ztest.h>
#include <sample_usbd.h>

#include <zephyr/drivers/disk.h>
#include <zephyr/usb/class/usbd_dfu.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

USBD_DEVICE_DEFINE(dfu_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "DFU FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "DFU HS Configuration");

static const uint8_t attributes =
	(IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0) |
	(IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);
/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config, attributes, CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(sample_hs_config, attributes, CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);

/* NOTE:
 * wTransferSize in functional descriptor is equal to 'CONFIG_USBD_DFU_TRANSFER_SIZE'.
 * The upload request must therefore handle a buffer size of at least
 * 'CONFIG_USBD_DFU_TRANSFER_SIZE' amount of data.
 */
#define DFUTEST_FIRMWARE_SIZE (256)
#define DFUTEST_SECTOR_SIZE   (CONFIG_USBD_DFU_TRANSFER_SIZE)

#if DFUTEST_SECTOR_SIZE > CONFIG_USBD_DFU_TRANSFER_SIZE
#error "CONFIG_USBD_DFU_TRANSFER_SIZE too small"
#endif

static void switch_to_dfu_mode(struct usbd_context *const ctx);

static char fw0[DFUTEST_FIRMWARE_SIZE] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praese "
	"t placerat pretium odio et tincidunt. Integer non fermentum enim";

char fw1[DFUTEST_FIRMWARE_SIZE] =
	"Curabitur consequat dui dignissim quam egestas, a tristique eni "
	"vestibulum. Nulla facilisi. Donec hendrerit hendrerit nulla, ut "
	"convallis metus mollis ultricies. Duis eu sem non sapien dignis "
	"im condimentum sed et nibh. Sed velit nunc, maximus sed sollicit";

char fw2[DFUTEST_FIRMWARE_SIZE] =
	"Phasellus nec mauris elit. In tristique turpis quis tortor maxi "
	"us, a placerat eros facilisis. Duis vel odio non mi consectetur "
	"odales nec sed erat. Nam fermentum leo eget risus ultricies dign";

struct dfu_ramdisk_data {
	const char *name;
	uint32_t last_block;
	uint32_t sector_size;
	uint32_t sector_count;
	uint32_t cursor;
	uint32_t complete;
	char *fw;
};

static struct dfu_ramdisk_data ramdisk0_data = {
	.name = "image0",
	.sector_size = DFUTEST_SECTOR_SIZE,
	.sector_count = DIV_ROUND_UP(DFUTEST_FIRMWARE_SIZE, DFUTEST_SECTOR_SIZE),
	.fw = fw0};

static int ramdisk_read(void *const priv, const uint32_t block, const uint16_t size,
			uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct dfu_ramdisk_data *const data = priv;
	uint32_t copy_size, copy_remain;

	if (size == 0) {
		/* There is nothing to upload */
		return 0;
	}

	if (block == 0) {
		data->last_block = 0;
		data->cursor = 0;
		data->complete = false;
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}
	}

	if (size == 0) {
		return 0;
	}

	copy_size = MIN(size, data->sector_size);

	if (data->cursor >= DFUTEST_FIRMWARE_SIZE) {
		data->complete = true;
		return 0;
	}

	copy_remain = DFUTEST_FIRMWARE_SIZE - data->cursor;
	if (copy_remain < copy_size) {
		copy_size = copy_remain;
	}

	memcpy(buf, &data->fw[data->cursor], copy_size);

	data->last_block = block;
	data->cursor += copy_size;
	LOG_INF("block %u size %u uploaded %u", block, size, data->cursor);

	return copy_size;
}

static int ramdisk_write(void *const priv, const uint32_t block, const uint16_t size,
			 const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct dfu_ramdisk_data *const data = priv;
	uint32_t copy_size, copy_remain;

	if (block == 0) {
		data->last_block = 0;
		data->cursor = 0;
		data->complete = false;
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}
	}

	if (size == 0) {
		data->complete = true;
		return 0;
	}

	copy_size = MIN(size, data->sector_size);

	if (data->cursor >= DFUTEST_FIRMWARE_SIZE) {
		return 0;
	}

	copy_remain = DFUTEST_FIRMWARE_SIZE - data->cursor;
	if (copy_remain < copy_size) {
		copy_size = copy_remain;
	}

	memcpy(&data->fw[data->cursor], buf, copy_size);

	data->last_block = block;
	data->cursor += size;
	LOG_INF("block %u size %u downloaded %u", block, size, data->cursor);

	return 0;
}

USBD_DFU_DEFINE_IMG(ramdisk0, "ramdisk0", &ramdisk0_data, ramdisk_read, ramdisk_write, NULL);

K_SEM_DEFINE(dfu_ready_sem, 0, 1);

struct usbh_context *const uhs_ctx = &test_uhs_ctx;
static struct usbd_context *test_usbd;

static void msg_dfurt_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_ERR("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_DFU_APP_DETACH) {
		switch_to_dfu_mode(usbd_ctx);
	}
}

static void msg_dfu_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_ERR("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_CONFIGURATION) {
		/* DFU probe should be already called */
		k_sem_give(&dfu_ready_sem);
	}
}

static void switch_to_dfu_mode(struct usbd_context *const ctx)
{
	int err;

	LOG_INF("Detach USB device");
	usbd_disable(ctx);
	usbd_shutdown(ctx);

	err = usbd_add_descriptor(&dfu_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return;
	}

	if (usbd_caps_speed(&dfu_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_HS, &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return;
		}

		err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return;
		}

		usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_HS, 0, 0, 0);
	}

	err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_FS, &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return;
	}

	err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_FS, 1);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return;
	}

	usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_FS, 0, 0, 0);

	err = usbd_init(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to initialize USB device support");
		return;
	}

	err = usbd_msg_register_cb(&dfu_usbd, msg_dfu_cb);
	if (err) {
		LOG_ERR("Failed to register message callback");
		return;
	}

	err = usbd_enable(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to enable USB device support");
	}
}

struct dfu_cb_context {
	bool complete;
	uint16_t cursor;
	uint16_t max_size;
	char *fw;
};

/* Download data and compare with expected FW */
int test_dfu_upload_cb(void *arg, char *data, size_t len)
{
	struct dfu_cb_context *dfu_ctx = (struct dfu_cb_context *)arg;
	size_t copy_len;

	if (len == 0) {
		LOG_DBG("Receive complete");
		dfu_ctx->complete = true;
		return 0;
	}

	if (dfu_ctx->cursor >= dfu_ctx->max_size) {
		return -EINVAL;
	}

	copy_len = dfu_ctx->max_size - dfu_ctx->cursor;
	copy_len = MIN(len, copy_len);

	LOG_DBG("Received: %.*s", len, data);
	if (memcmp(data, &dfu_ctx->fw[dfu_ctx->cursor], copy_len)) {
		return -EBADMSG;
	}
	dfu_ctx->cursor += copy_len;

	return 0;
}

/* Report back custom error, upper layer should perform cleanup and put the
 * state machine into DFU_IDLE state
 */
int test_dfu_upload_custom_err_cb(void *arg, char *data, size_t len)
{
	struct dfu_cb_context *dfu_ctx = (struct dfu_cb_context *)arg;
	uint16_t cursor = dfu_ctx->cursor++;

	/* Report error on second call */
	return cursor ? -0x0DEADBEE : 0;
}

/* Upload data */
int test_dfu_dnload_cb(void *arg, char *data, size_t len)
{
	struct dfu_cb_context *dfu_ctx = (struct dfu_cb_context *)arg;
	size_t copy_len;

	if (dfu_ctx->cursor >= dfu_ctx->max_size) {
		LOG_DBG("Transmit complete");
		return 0;
	}

	copy_len = dfu_ctx->max_size - dfu_ctx->cursor;
	copy_len = MIN(len, copy_len);

	memcpy(data, &dfu_ctx->fw[dfu_ctx->cursor], copy_len);
	dfu_ctx->cursor += copy_len;
	LOG_DBG("Transmitted: %.*s", copy_len, data);

	return copy_len;
}

ZTEST(dfu_host_test, test_runtime)
{
	const struct device *dfurt_dev;
	const struct device *dfu_dev;
	int ret, i;

	dfurt_dev = DEVICE_DT_GET(DT_NODELABEL(any_dfurt_device));
	zassert_not_null(dfurt_dev, "No USB host DFU-runtime instance available");

	struct usbh_dfu_settings settings = {.alternate_idx = 0, .detach_timeout_ms = 1000};

	ret = usbh_dfurt_settings(dfurt_dev, &settings);
	zassert_equal(ret, 0, "USB DFU-runtime could prepare settings");

	ret = usbh_dfurt_enter_dfu(dfurt_dev);
	zassert_equal(ret, 0, "USB DFU-runtime could not enter DFU mode");

	ret = k_sem_take(&dfu_ready_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "USB DFU did not receive 'USBD_MSG_CONFIGURATION' msg");

	dfu_dev = DEVICE_DT_GET(DT_NODELABEL(any_dfu_device));
	ret = device_is_ready(dfu_dev);
	zassert_equal(ret, 1, "USB DFU device is not ready");

	/* Try several times, the driver of DFU device may not be probed yet */
	ret = -EAGAIN;
	for (i = 0; (i < 7) && (ret == -EAGAIN); i++) {
		ret = usbh_dfu_settings(dfu_dev, &settings);
		/* Device was not probed yet */
		if (ret == -EAGAIN) {
			k_msleep(100);
		}
	}
	zassert_equal(ret, 0, "USB DFU could prepare settings");

	struct dfu_cb_context dfu_cb_ctx;

	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw0};
	ret = usbh_dfu_upload(dfu_dev, test_dfu_upload_cb, &dfu_cb_ctx);
	zassert_equal(ret, 0, "USB DFU could not upload data");

	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw0};
	ret = usbh_dfu_upload(dfu_dev, test_dfu_upload_cb, &dfu_cb_ctx);
	zassert_equal(ret, 0, "USB DFU could not repeat the data upload");

	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw0};
	ret = usbh_dfu_upload(dfu_dev, test_dfu_upload_custom_err_cb, &dfu_cb_ctx);
	zassert_equal(ret, -0x0DEADBEE, "USB DFU upload callback could not report custom retor");

	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw0};
	ret = usbh_dfu_upload(dfu_dev, test_dfu_upload_cb, &dfu_cb_ctx);
	zassert_equal(ret, 0, "USB DFU could not repeat the data upload after custom retor");

	/* Use quirks because DFU Zephyr device has broken state machine and does not follow the
	 * spec
	 */
	settings.quirks = USBH_DFU_QUIRK_IGNORE_DNLOAD_COMPLETE_CHECK;
	ret = usbh_dfu_settings(dfu_dev, &settings);
	zassert_equal(ret, 0, "USB DFU could populate DFU settings");

	/* Download 'fw1' to 'fw0' */
	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw1};
	ret = usbh_dfu_dnload(dfu_dev, test_dfu_dnload_cb, &dfu_cb_ctx);
	zassert_equal(ret, 0, "USB DFU could not download the data");

	/* Check downloaded data, 'fw0' sholuld be the same as 'fw1'*/
	dfu_cb_ctx = (struct dfu_cb_context){.max_size = DFUTEST_FIRMWARE_SIZE, .fw = fw1};
	ret = usbh_dfu_upload(dfu_dev, test_dfu_upload_cb, &dfu_cb_ctx);
	zassert_equal(ret, 0, "USB DFU comparison of downloaded firmware failed");

	/* Try to switch to invalid alternate function 42 */
	settings.alternate_idx = 42;
	ret = usbh_dfu_settings(dfu_dev, &settings);
	zassert_not_equal(ret, 0, "USB DFU could populate DFU settings");

	/* Cleanup */
	ret = usbd_disable(&dfu_usbd);
	zassert_ok(ret, "Failed to disable device support");

	ret = usbd_shutdown(&dfu_usbd);
	zassert_ok(ret, "Failed to shutdown device support");
}

void *dfu_host_test_enable(void)
{
	int ret;

	ret = usbh_init(uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus reset");

	ret = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus resume");

	ret = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(ret, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_setup_device(msg_dfurt_cb);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	ret = usbd_init(test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	return NULL;
}

void dfu_host_test_shutdown(void *f)
{
	int ret;

	/* Do not check, might be already disabled */
	usbd_disable(test_usbd);
	usbd_shutdown(test_usbd);

	ret = usbh_disable(uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(dfu_host_test, NULL, dfu_host_test_enable, NULL, NULL, dfu_host_test_shutdown);
