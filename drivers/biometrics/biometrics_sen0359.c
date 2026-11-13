/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT dfrobot_sen0359

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sen0359, CONFIG_BIOMETRICS_LOG_LEVEL);

/* Command codes */
#define SEN0359_CMD_TEST_CONNECTION      0x0001U
#define SEN0359_CMD_SET_PARAM            0x0002U
#define SEN0359_CMD_GET_PARAM            0x0003U
#define SEN0359_CMD_DEVICE_INFO          0x0004U
#define SEN0359_CMD_GET_IMAGE            0x0020U
#define SEN0359_CMD_FINGER_DETECT        0x0021U
#define SEN0359_CMD_SLED_CTRL            0x0024U
#define SEN0359_CMD_STORE_CHAR           0x0040U
#define SEN0359_CMD_DEL_CHAR             0x0044U
#define SEN0359_CMD_GET_ENROLLED_ID_LIST 0x0049U
#define SEN0359_CMD_GENERATE             0x0060U
#define SEN0359_CMD_MERGE                0x0061U
#define SEN0359_CMD_SEARCH               0x0063U
#define SEN0359_CMD_VERIFY               0x0064U

/* Time the module needs before its response can be read, in ms */
#define SEN0359_DELAY_SHORT_MS  50U
#define SEN0359_DELAY_LIST_MS   120U
#define SEN0359_DELAY_DETECT_MS 240U
#define SEN0359_DELAY_PARAM_MS  240U
#define SEN0359_DELAY_LONG_MS   360U

/* Device result codes (low byte of RET) */
#define SEN0359_OK                 0x00U
#define SEN0359_ERR_VERIFY         0x10U
#define SEN0359_ERR_IDENTIFY       0x11U
#define SEN0359_ERR_TMPL_EMPTY     0x12U
#define SEN0359_ERR_TMPL_NOT_EMPTY 0x13U
#define SEN0359_ERR_ALL_TMPL_EMPTY 0x14U
#define SEN0359_ERR_NO_EMPTY_ID    0x15U
#define SEN0359_ERR_DUPLICATE      0x18U
#define SEN0359_ERR_BAD_QUALITY    0x19U
#define SEN0359_ERR_NOT_AUTHORIZED 0x1BU
#define SEN0359_ERR_INVALID_ID     0x1DU
#define SEN0359_ERR_INVALID_PARAM  0x22U
#define SEN0359_ERR_TIMEOUT        0x23U
#define SEN0359_ERR_GEN_COUNT      0x25U
#define SEN0359_ERR_BUFFER_ID      0x26U
#define SEN0359_ERR_NO_FINGER      0x28U

/* Framing */
#define SEN0359_CMD_PREFIX       0xAA55U /* 55 AA on the wire */
#define SEN0359_READY            0xEEU
#define SEN0359_CMD_HDR_SIZE     8U
#define SEN0359_CMD_PARAM_SIZE   16U
#define SEN0359_RSP_HDR_SIZE     10U
#define SEN0359_RSP_PAYLOAD_SIZE 14U
#define SEN0359_CKS_SIZE         2U
#define SEN0359_DATA_MAX_SIZE    64U
#define SEN0359_IO_TIMEOUT       K_SECONDS(2)

#define SEN0359_PARAM_SECURITY_LEVEL 1U
#define SEN0359_ENROLL_SAMPLES       3U
#define SEN0359_SEARCH_BUFFER        0U

struct sen0359_config {
	struct i2c_dt_spec i2c;
};

struct sen0359_data {
	struct k_mutex lock;
	uint16_t capacity;
	uint16_t enroll_id;
	uint8_t enroll_samples;
	uint8_t security_level;
	bool enrolling;
};

static int sen0359_result_to_errno(uint8_t code)
{
	switch (code) {
	case SEN0359_OK:
		return 0;
	case SEN0359_ERR_VERIFY:
	case SEN0359_ERR_IDENTIFY:
	case SEN0359_ERR_TMPL_EMPTY:
	case SEN0359_ERR_ALL_TMPL_EMPTY:
		return -ENOENT;
	case SEN0359_ERR_TMPL_NOT_EMPTY:
	case SEN0359_ERR_DUPLICATE:
		return -EEXIST;
	case SEN0359_ERR_NO_EMPTY_ID:
		return -ENOSPC;
	case SEN0359_ERR_BAD_QUALITY:
	case SEN0359_ERR_NO_FINGER:
		return -EAGAIN;
	case SEN0359_ERR_NOT_AUTHORIZED:
		return -EACCES;
	case SEN0359_ERR_INVALID_ID:
	case SEN0359_ERR_INVALID_PARAM:
	case SEN0359_ERR_GEN_COUNT:
	case SEN0359_ERR_BUFFER_ID:
		return -EINVAL;
	case SEN0359_ERR_TIMEOUT:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

/* 0xFF plus the byte sum of SID through the last payload byte */
static uint16_t sen0359_checksum(const uint8_t *buf, size_t len)
{
	uint16_t cks = 0xFFU;

	for (size_t i = 0; i < len; i++) {
		cks += buf[i];
	}

	return cks;
}

/* One byte per transaction, as the vendor library reads responses */
static int sen0359_read(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct sen0359_config *cfg = dev->config;
	int ret;

	for (size_t i = 0; i < len; i++) {
		ret = i2c_read_dt(&cfg->i2c, &buf[i], 1);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int sen0359_send(const struct device *dev, uint16_t cmd, const uint8_t *param,
			size_t param_len)
{
	const struct sen0359_config *cfg = dev->config;
	uint8_t pkt[SEN0359_CMD_HDR_SIZE + SEN0359_CMD_PARAM_SIZE + SEN0359_CKS_SIZE] = {0};
	k_timepoint_t end = sys_timepoint_calc(SEN0359_IO_TIMEOUT);
	uint8_t status;
	int ret;

	__ASSERT_NO_MSG(param_len <= SEN0359_CMD_PARAM_SIZE);

	sys_put_le16(SEN0359_CMD_PREFIX, &pkt[0]);
	/* pkt[2] (source ID) and pkt[3] (destination ID) stay 0 */
	sys_put_le16(cmd, &pkt[4]);
	sys_put_le16((uint16_t)param_len, &pkt[6]);
	if (param_len > 0U) {
		memcpy(&pkt[SEN0359_CMD_HDR_SIZE], param, param_len);
	}
	sys_put_le16(sen0359_checksum(&pkt[2], SEN0359_CMD_HDR_SIZE - 2U + param_len),
		     &pkt[SEN0359_CMD_HDR_SIZE + SEN0359_CMD_PARAM_SIZE]);

	/* The module returns 0xEE when it can accept a command; a NACK means not ready yet */
	for (;;) {
		ret = i2c_read_dt(&cfg->i2c, &status, 1);
		if ((ret == 0) && (status == SEN0359_READY)) {
			break;
		}

		if (sys_timepoint_expired(end)) {
			LOG_ERR("Module not ready (%d)", ret);
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}

	return i2c_write_dt(&cfg->i2c, pkt, sizeof(pkt));
}

/*
 * Receive a response (AA 55, fixed 14-byte payload) or, when @p data_pkt is set,
 * a response data packet (A5 5A, LEN-byte payload). Both carry LEN - 2
 * meaningful payload bytes: fewer than @p min_len is an error, and up to
 * @p max_len of them are copied to @p out.
 */
static int sen0359_recv(const struct device *dev, bool data_pkt, uint8_t *out, size_t min_len,
			size_t max_len, size_t *out_len)
{
	const struct sen0359_config *cfg = dev->config;
	uint8_t pkt[SEN0359_RSP_HDR_SIZE + SEN0359_DATA_MAX_SIZE + SEN0359_CKS_SIZE];
	k_timepoint_t end = sys_timepoint_calc(SEN0359_IO_TIMEOUT);
	uint8_t prev = 0U;
	uint8_t byte = 0U;
	size_t payload_size;
	size_t data_len;
	uint16_t len;
	uint16_t cks;
	bool got_data_pkt = false;
	int ret;

	for (;;) {
		ret = i2c_read_dt(&cfg->i2c, &byte, 1);
		if (ret == 0) {
			if ((prev == 0xAAU) && (byte == 0x55U)) {
				break;
			}

			if ((prev == 0xA5U) && (byte == 0x5AU)) {
				got_data_pkt = true;
				break;
			}

			prev = byte;
		}

		if (sys_timepoint_expired(end)) {
			LOG_ERR("Response timeout (%d)", ret);
			return -ETIMEDOUT;
		}
	}

	pkt[0] = prev;
	pkt[1] = byte;

	ret = sen0359_read(dev, &pkt[2], SEN0359_RSP_HDR_SIZE - 2U);
	if (ret < 0) {
		return ret;
	}

	len = sys_get_le16(&pkt[6]);
	payload_size = got_data_pkt ? len : (SEN0359_RSP_PAYLOAD_SIZE + SEN0359_CKS_SIZE);

	if ((len < SEN0359_CKS_SIZE) || (len > payload_size) ||
	    (payload_size > (SEN0359_DATA_MAX_SIZE + SEN0359_CKS_SIZE))) {
		LOG_ERR("Invalid response length %u", len);
		return -EBADMSG;
	}

	ret = sen0359_read(dev, &pkt[SEN0359_RSP_HDR_SIZE], payload_size);
	if (ret < 0) {
		return ret;
	}

	if (got_data_pkt != data_pkt) {
		LOG_ERR("Unexpected packet type for command 0x%04x", sys_get_le16(&pkt[4]));
		return -EBADMSG;
	}

	if (pkt[8] != SEN0359_OK) {
		LOG_DBG("Command 0x%04x failed: 0x%02x", sys_get_le16(&pkt[4]), pkt[8]);
		return sen0359_result_to_errno(pkt[8]);
	}

	data_len = len - SEN0359_CKS_SIZE;
	cks = sys_get_le16(&pkt[SEN0359_RSP_HDR_SIZE + payload_size - SEN0359_CKS_SIZE]);
	if (sen0359_checksum(&pkt[2], SEN0359_RSP_HDR_SIZE - 2U + data_len) != cks) {
		LOG_ERR("Checksum mismatch");
		return -EIO;
	}

	if (data_len < min_len) {
		LOG_ERR("Short response (%u < %u bytes)", (uint32_t)data_len, (uint32_t)min_len);
		return -EBADMSG;
	}

	if (out != NULL) {
		memcpy(out, &pkt[SEN0359_RSP_HDR_SIZE], MIN(data_len, max_len));
	}

	if (out_len != NULL) {
		*out_len = data_len;
	}

	return 0;
}

static int sen0359_cmd(const struct device *dev, uint16_t cmd, const uint8_t *param,
		       size_t param_len, uint32_t delay_ms, uint8_t *rsp, size_t rsp_size)
{
	int ret;

	ret = sen0359_send(dev, cmd, param, param_len);
	if (ret < 0) {
		return ret;
	}

	k_msleep(delay_ms);

	return sen0359_recv(dev, false, rsp, rsp_size, rsp_size, NULL);
}

/* Commands whose response announces a data packet that follows it */
static int sen0359_cmd_data(const struct device *dev, uint16_t cmd, uint32_t delay_ms, uint8_t *buf,
			    size_t size, size_t *len)
{
	uint8_t rsp[2];
	int ret;

	ret = sen0359_cmd(dev, cmd, NULL, 0, delay_ms, rsp, sizeof(rsp));
	if (ret < 0) {
		return ret;
	}

	return sen0359_recv(dev, true, buf, 0U, size, len);
}

static int sen0359_validate_id(const struct device *dev, uint16_t id)
{
	const struct sen0359_data *data = dev->data;

	if ((id < 1U) || (id > data->capacity)) {
		LOG_ERR("Invalid ID %u (valid: 1-%u)", id, data->capacity);
		return -EINVAL;
	}

	return 0;
}

static int sen0359_finger_present(const struct device *dev, bool *present)
{
	uint8_t rsp[1];
	int ret;

	ret = sen0359_cmd(dev, SEN0359_CMD_FINGER_DETECT, NULL, 0, SEN0359_DELAY_DETECT_MS, rsp,
			  sizeof(rsp));
	if (ret < 0) {
		return ret;
	}

	*present = (rsp[0] == 1U);

	return 0;
}

static int sen0359_wait_finger(const struct device *dev, bool present, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	bool state;
	int ret;

	for (;;) {
		ret = sen0359_finger_present(dev, &state);
		if (ret < 0) {
			return ret;
		}

		if (state == present) {
			return 0;
		}

		if (sys_timepoint_expired(end)) {
			return -ETIMEDOUT;
		}
	}
}

/*
 * Wait for a finger, image it and extract its features into RAM buffer @p buffer_id.
 * With @p new_touch, a finger already on the sensor must be lifted first.
 */
static int sen0359_capture(const struct device *dev, uint8_t buffer_id, bool new_touch,
			   k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t param[2] = {buffer_id, 0U};
	int ret;

	if (new_touch) {
		ret = sen0359_wait_finger(dev, false, timeout);
		if (ret < 0) {
			return ret;
		}
	}

	ret = sen0359_wait_finger(dev, true, sys_timepoint_timeout(end));
	if (ret < 0) {
		return ret;
	}

	ret = sen0359_cmd(dev, SEN0359_CMD_GET_IMAGE, NULL, 0, SEN0359_DELAY_LONG_MS, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return sen0359_cmd(dev, SEN0359_CMD_GENERATE, param, sizeof(param), SEN0359_DELAY_LONG_MS,
			   NULL, 0);
}

static int sen0359_get_capabilities(const struct device *dev, struct biometric_capabilities *caps)
{
	const struct sen0359_data *data = dev->data;

	caps->type = BIOMETRIC_TYPE_FINGERPRINT;
	caps->max_templates = data->capacity;
	caps->template_size = 0U;
	caps->storage_modes = BIOMETRIC_STORAGE_DEVICE;
	caps->enrollment_samples_required = SEN0359_ENROLL_SAMPLES;

	return 0;
}

/*
 * The module has security levels 1-5; the API levels 1-10 map onto them in pairs.
 * The last level set is reported back while the module still holds its mapping.
 */
static int sen0359_attr_set(const struct device *dev, enum biometric_attribute attr, int32_t val)
{
	struct sen0359_data *data = dev->data;
	uint8_t param[5] = {SEN0359_PARAM_SECURITY_LEVEL, 0U, 0U, 0U, 0U};
	int ret;

	if (attr != BIOMETRIC_ATTR_SECURITY_LEVEL) {
		return -ENOTSUP;
	}

	if ((val < 1) || (val > 10)) {
		return -EINVAL;
	}

	param[1] = (uint8_t)DIV_ROUND_UP(val, 2);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sen0359_cmd(dev, SEN0359_CMD_SET_PARAM, param, sizeof(param), SEN0359_DELAY_PARAM_MS,
			  NULL, 0);
	if (ret == 0) {
		data->security_level = (uint8_t)val;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0359_attr_get(const struct device *dev, enum biometric_attribute attr, int32_t *val)
{
	struct sen0359_data *data = dev->data;
	uint8_t param[1] = {SEN0359_PARAM_SECURITY_LEVEL};
	uint8_t rsp[1];
	int ret;

	if (attr != BIOMETRIC_ATTR_SECURITY_LEVEL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sen0359_cmd(dev, SEN0359_CMD_GET_PARAM, param, sizeof(param), SEN0359_DELAY_SHORT_MS,
			  rsp, sizeof(rsp));
	if (ret == 0) {
		if ((data->security_level != 0U) &&
		    (DIV_ROUND_UP(data->security_level, 2U) == rsp[0])) {
			*val = data->security_level;
		} else {
			*val = (int32_t)rsp[0] * 2;
		}
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0359_enroll_start(const struct device *dev, uint16_t template_id)
{
	struct sen0359_data *data = dev->data;
	int ret;

	ret = sen0359_validate_id(dev, template_id);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enrolling) {
		ret = -EINVAL;
	} else {
		data->enrolling = true;
		data->enroll_id = template_id;
		data->enroll_samples = 0U;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0359_enroll_capture(const struct device *dev, k_timeout_t timeout,
				  struct biometric_capture_result *result)
{
	struct sen0359_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling || (data->enroll_samples >= SEN0359_ENROLL_SAMPLES)) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	ret = sen0359_capture(dev, data->enroll_samples, data->enroll_samples > 0U, timeout);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->enroll_samples++;

	if (result != NULL) {
		result->samples_captured = data->enroll_samples;
		result->samples_required = SEN0359_ENROLL_SAMPLES;
		result->quality = 0U;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int sen0359_enroll_finalize(const struct device *dev)
{
	struct sen0359_data *data = dev->data;
	uint8_t merge[3] = {0U, 0U, SEN0359_ENROLL_SAMPLES};
	uint8_t store[4] = {0};
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling || (data->enroll_samples < SEN0359_ENROLL_SAMPLES)) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->enrolling = false;

	ret = sen0359_cmd(dev, SEN0359_CMD_MERGE, merge, sizeof(merge), SEN0359_DELAY_LONG_MS, NULL,
			  0);
	if (ret == 0) {
		sys_put_le16(data->enroll_id, &store[0]);
		ret = sen0359_cmd(dev, SEN0359_CMD_STORE_CHAR, store, sizeof(store),
				  SEN0359_DELAY_LONG_MS, NULL, 0);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0359_enroll_abort(const struct device *dev)
{
	struct sen0359_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling) {
		ret = -EALREADY;
	}

	data->enrolling = false;

	k_mutex_unlock(&data->lock);

	return ret;
}

/* Delete the templates with IDs @p first to @p last */
static int sen0359_delete_range(const struct device *dev, uint16_t first, uint16_t last)
{
	struct sen0359_data *data = dev->data;
	uint8_t param[4];
	int ret;

	sys_put_le16(first, &param[0]);
	sys_put_le16(last, &param[2]);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sen0359_cmd(dev, SEN0359_CMD_DEL_CHAR, param, sizeof(param), SEN0359_DELAY_LONG_MS,
			  NULL, 0);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0359_template_delete(const struct device *dev, uint16_t id)
{
	int ret;

	ret = sen0359_validate_id(dev, id);
	if (ret < 0) {
		return ret;
	}

	return sen0359_delete_range(dev, id, id);
}

static int sen0359_template_delete_all(const struct device *dev)
{
	const struct sen0359_data *data = dev->data;
	int ret;

	ret = sen0359_delete_range(dev, 1U, data->capacity);

	/* The module rejects deleting from an empty database */
	return (ret == -ENOENT) ? 0 : ret;
}

static int sen0359_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
				 size_t *actual_count)
{
	struct sen0359_data *data = dev->data;
	uint8_t bitmap[SEN0359_DATA_MAX_SIZE];
	size_t count = 0U;
	size_t len;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sen0359_cmd_data(dev, SEN0359_CMD_GET_ENROLLED_ID_LIST, SEN0359_DELAY_LIST_MS, bitmap,
			       sizeof(bitmap), &len);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		return ret;
	}

	/* Bit n of the bitmap (LSB first) is set when ID n holds a template */
	len = MIN(len, sizeof(bitmap));
	for (uint16_t id = 1U; (id <= data->capacity) && (id < (len * 8U)); id++) {
		if (count >= max_count) {
			break;
		}

		if ((bitmap[id / 8U] & BIT(id % 8U)) != 0U) {
			ids[count] = id;
			count++;
		}
	}

	*actual_count = count;

	return 0;
}

static int sen0359_match(const struct device *dev, enum biometric_match_mode mode,
			 uint16_t template_id, k_timeout_t timeout,
			 struct biometric_match_result *result)
{
	struct sen0359_data *data = dev->data;
	uint8_t param[6] = {0};
	uint8_t rsp[1];
	uint16_t matched_id;
	int ret;

	if ((mode != BIOMETRIC_MATCH_VERIFY) && (mode != BIOMETRIC_MATCH_IDENTIFY)) {
		return -EINVAL;
	}

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		ret = sen0359_validate_id(dev, template_id);
		if (ret < 0) {
			return ret;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Matching reuses the RAM buffer that holds the first enrollment sample */
	if (data->enrolling) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	ret = sen0359_capture(dev, SEN0359_SEARCH_BUFFER, false, timeout);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		sys_put_le16(template_id, &param[0]);
		ret = sen0359_cmd(dev, SEN0359_CMD_VERIFY, param, 4U, SEN0359_DELAY_SHORT_MS, rsp,
				  sizeof(rsp));
	} else {
		param[0] = SEN0359_SEARCH_BUFFER;
		sys_put_le16(1U, &param[2]);
		sys_put_le16(data->capacity, &param[4]);
		ret = sen0359_cmd(dev, SEN0359_CMD_SEARCH, param, sizeof(param),
				  SEN0359_DELAY_LONG_MS, rsp, sizeof(rsp));
	}

	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		return ret;
	}

	matched_id = rsp[0];
	if ((matched_id < 1U) || (matched_id > data->capacity)) {
		LOG_ERR("Invalid matched ID %u", matched_id);
		return -EBADMSG;
	}

	if (result != NULL) {
		result->confidence = 0;
		result->template_id = matched_id;
		result->image_quality = 0U;
	}

	return 0;
}

static int sen0359_led_control(const struct device *dev, enum biometric_led_state state)
{
	struct sen0359_data *data = dev->data;
	/* Mode codes differ between the 80- and 200-template firmware */
	static const uint8_t modes_80[BIOMETRIC_LED_BREATHE + 1] = {
		[BIOMETRIC_LED_OFF] = 4U,
		[BIOMETRIC_LED_ON] = 3U,
		[BIOMETRIC_LED_BLINK] = 2U,
		[BIOMETRIC_LED_BREATHE] = 1U,
	};
	static const uint8_t modes_200[BIOMETRIC_LED_BREATHE + 1] = {
		[BIOMETRIC_LED_OFF] = 0U,
		[BIOMETRIC_LED_ON] = 1U,
		[BIOMETRIC_LED_BLINK] = 4U,
		[BIOMETRIC_LED_BREATHE] = 2U,
	};
	uint8_t param[4] = {0};
	bool fw_80 = (data->capacity == 80U);
	int ret;

	if ((uint32_t)state >= ARRAY_SIZE(modes_80)) {
		return -EINVAL;
	}

	/* Blue; blink and breathe repeat until the next command */
	param[0] = fw_80 ? modes_80[state] : modes_200[state];
	param[1] = fw_80 ? 0x04U : 0x81U;
	param[2] = param[1];

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sen0359_cmd(dev, SEN0359_CMD_SLED_CTRL, param, sizeof(param), SEN0359_DELAY_SHORT_MS,
			  NULL, 0);
	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(biometric, sen0359_api) = {
	.get_capabilities = sen0359_get_capabilities,
	.attr_set = sen0359_attr_set,
	.attr_get = sen0359_attr_get,
	.enroll_start = sen0359_enroll_start,
	.enroll_capture = sen0359_enroll_capture,
	.enroll_finalize = sen0359_enroll_finalize,
	.enroll_abort = sen0359_enroll_abort,
	.template_delete = sen0359_template_delete,
	.template_delete_all = sen0359_template_delete_all,
	.template_list = sen0359_template_list,
	.match = sen0359_match,
	.led_control = sen0359_led_control,
};

static int sen0359_init(const struct device *dev)
{
	const struct sen0359_config *cfg = dev->config;
	struct sen0359_data *data = dev->data;
	char info[SEN0359_DATA_MAX_SIZE + 1U];
	size_t len;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	data->capacity = 80U;

	ret = sen0359_cmd(dev, SEN0359_CMD_TEST_CONNECTION, NULL, 0, SEN0359_DELAY_SHORT_MS, NULL,
			  0);
	if (ret < 0) {
		LOG_ERR("No response from module: %d", ret);
		return ret;
	}

	/* The firmware string ends in '3' on 200-template modules, '4' on 80-template ones */
	ret = sen0359_cmd_data(dev, SEN0359_CMD_DEVICE_INFO, SEN0359_DELAY_SHORT_MS,
			       (uint8_t *)info, SEN0359_DATA_MAX_SIZE, &len);
	if (ret < 0) {
		LOG_ERR("Failed to read device info: %d", ret);
		return ret;
	}

	info[MIN(len, SEN0359_DATA_MAX_SIZE)] = '\0';
	len = strlen(info);
	if ((len > 0U) && (info[len - 1U] == '3')) {
		data->capacity = 200U;
	}

	LOG_DBG("%s, %u templates", info, data->capacity);

	return 0;
}

#define SEN0359_DEFINE(inst)                                                                       \
	static struct sen0359_data sen0359_data_##inst;                                            \
                                                                                                   \
	static const struct sen0359_config sen0359_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sen0359_init, NULL, &sen0359_data_##inst,                      \
			      &sen0359_config_##inst, POST_KERNEL,                                 \
			      CONFIG_BIOMETRICS_INIT_PRIORITY, &sen0359_api);

DT_INST_FOREACH_STATUS_OKAY(SEN0359_DEFINE)
