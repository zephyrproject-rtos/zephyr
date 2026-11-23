/*
 * Copyright (c) 2025 Siratul Islam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_biometrics_emul

#include <zephyr/device.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(biometrics_emul, CONFIG_BIOMETRICS_LOG_LEVEL);

#define BIOMETRICS_EMUL_MAX_TEMPLATES     100
#define BIOMETRICS_EMUL_TEMPLATE_SIZE     512
#define BIOMETRICS_EMUL_ENROLL_SAMPLES    2
#define BIOMETRICS_EMUL_DEFAULT_THRESHOLD 50
#define BIOMETRICS_EMUL_DEFAULT_QUALITY   60
#define BIOMETRICS_EMUL_DEFAULT_SECURITY  5
#define BIOMETRICS_EMUL_DEFAULT_TIMEOUT   5000

struct biometrics_emul_template {
	bool valid;
	uint16_t id;
	uint8_t data[BIOMETRICS_EMUL_TEMPLATE_SIZE];
	size_t size;
};

struct biometrics_emul_data {
	struct k_mutex lock;

	int32_t match_threshold;
	int32_t enrollment_quality;
	int32_t security_level;
	int32_t timeout_ms;
	int32_t anti_spoof_level;
	int32_t last_image_quality;
	int32_t last_matched_id;

	struct biometrics_emul_template templates[BIOMETRICS_EMUL_MAX_TEMPLATES];
	uint16_t template_count;

	bool enrolling;
	uint16_t enroll_id;
	uint8_t enroll_samples_captured;
	uint8_t enroll_temp_data[BIOMETRICS_EMUL_TEMPLATE_SIZE];

	enum biometric_led_state led_state;

	/* Emulator control - allows tests to inject behavior */
	int next_match_score;
	int next_match_id;
	bool force_match_fail;
	bool force_capture_timeout;
	int next_image_quality;
};

struct biometrics_emul_config {
	enum biometric_sensor_type type;
};

static int biometrics_emul_get_capabilities(const struct device *dev,
					    struct biometric_capabilities *caps)
{
	const struct biometrics_emul_config *cfg = dev->config;

	caps->type = cfg->type;
	caps->max_templates = BIOMETRICS_EMUL_MAX_TEMPLATES;
	caps->template_size = BIOMETRICS_EMUL_TEMPLATE_SIZE;
	caps->storage_modes = BIOMETRIC_STORAGE_DEVICE | BIOMETRIC_STORAGE_HOST;
	caps->enrollment_samples_required = BIOMETRICS_EMUL_ENROLL_SAMPLES;

	return 0;
}

static int biometrics_emul_attr_set(const struct device *dev, enum biometric_attribute attr,
				    int32_t val)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (attr) {
	case BIOMETRIC_ATTR_MATCH_THRESHOLD:
		data->match_threshold = val;
		break;
	case BIOMETRIC_ATTR_ENROLLMENT_QUALITY:
		data->enrollment_quality = val;
		break;
	case BIOMETRIC_ATTR_SECURITY_LEVEL:
		if (val < 1 || val > 10) {
			ret = -EINVAL;
		} else {
			data->security_level = val;
		}
		break;
	case BIOMETRIC_ATTR_TIMEOUT_MS:
		data->timeout_ms = val;
		break;
	case BIOMETRIC_ATTR_ANTI_SPOOF_LEVEL:
		if (val < 1 || val > 10) {
			ret = -EINVAL;
		} else {
			data->anti_spoof_level = val;
		}
		break;
	case BIOMETRIC_ATTR_IMAGE_QUALITY:
		/* Read-only attribute */
		ret = -EACCES;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_attr_get(const struct device *dev, enum biometric_attribute attr,
				    int32_t *val)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (attr) {
	case BIOMETRIC_ATTR_MATCH_THRESHOLD:
		*val = data->match_threshold;
		break;
	case BIOMETRIC_ATTR_ENROLLMENT_QUALITY:
		*val = data->enrollment_quality;
		break;
	case BIOMETRIC_ATTR_SECURITY_LEVEL:
		*val = data->security_level;
		break;
	case BIOMETRIC_ATTR_TIMEOUT_MS:
		*val = data->timeout_ms;
		break;
	case BIOMETRIC_ATTR_ANTI_SPOOF_LEVEL:
		*val = data->anti_spoof_level;
		break;
	case BIOMETRIC_ATTR_IMAGE_QUALITY:
		*val = data->last_image_quality;
		break;
	case BIOMETRIC_ATTR_PRIV_START:
		/* Return last matched ID for identify mode */
		*val = data->last_matched_id;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_enroll_start(const struct device *dev, uint16_t template_id)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enrolling) {
		ret = -EBUSY;
		goto unlock;
	}

	/* Check if ID already exists */
	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (data->templates[i].valid && data->templates[i].id == template_id) {
			ret = -EEXIST;
			goto unlock;
		}
	}

	/* Check if we have space */
	if (data->template_count >= BIOMETRICS_EMUL_MAX_TEMPLATES) {
		ret = -ENOSPC;
		goto unlock;
	}

	data->enrolling = true;
	data->enroll_id = template_id;
	data->enroll_samples_captured = 0;
	memset(data->enroll_temp_data, 0, sizeof(data->enroll_temp_data));

	LOG_INF("Enrollment started for ID %u", template_id);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_enroll_capture(const struct device *dev, k_timeout_t timeout,
					  struct biometric_capture_result *result)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling) {
		ret = -EINVAL;
		goto unlock;
	}

	if (data->enroll_samples_captured >= BIOMETRICS_EMUL_ENROLL_SAMPLES) {
		ret = -EALREADY;
		goto unlock;
	}

	if (data->force_capture_timeout) {
		ret = -ETIMEDOUT;
		goto unlock;
	}

	/* Simulate capture delay */
	k_mutex_unlock(&data->lock);
	k_sleep(K_MSEC(50));
	k_mutex_lock(&data->lock, K_FOREVER);

	/* Generate simulated template data */
	data->enroll_temp_data[data->enroll_samples_captured * 32] = data->enroll_id & 0xFF;
	data->enroll_temp_data[data->enroll_samples_captured * 32 + 1] =
		(data->enroll_id >> 8) & 0xFF;
	data->enroll_samples_captured++;

	data->last_image_quality = data->next_image_quality > 0 ? data->next_image_quality
								: BIOMETRICS_EMUL_DEFAULT_QUALITY;

	if (result != NULL) {
		result->samples_captured = data->enroll_samples_captured;
		result->samples_required = BIOMETRICS_EMUL_ENROLL_SAMPLES;
		result->quality = (uint8_t)data->last_image_quality;
	}

	LOG_INF("Captured sample %u/%u for enrollment", data->enroll_samples_captured,
		BIOMETRICS_EMUL_ENROLL_SAMPLES);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_enroll_finalize(const struct device *dev)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;
	int slot = -1;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling) {
		ret = -EINVAL;
		goto unlock;
	}

	if (data->enroll_samples_captured < BIOMETRICS_EMUL_ENROLL_SAMPLES) {
		ret = -EINVAL;
		goto unlock;
	}

	/* Find empty slot */
	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (!data->templates[i].valid) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		ret = -ENOSPC;
		goto unlock;
	}

	/* Store template */
	data->templates[slot].valid = true;
	data->templates[slot].id = data->enroll_id;
	memcpy(data->templates[slot].data, data->enroll_temp_data, sizeof(data->enroll_temp_data));
	data->templates[slot].size = BIOMETRICS_EMUL_TEMPLATE_SIZE;
	data->template_count++;

	data->enrolling = false;

	LOG_INF("Enrollment finalized for ID %u (slot %d)", data->enroll_id, slot);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_enroll_abort(const struct device *dev)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->enrolling) {
		ret = -EALREADY;
		goto unlock;
	}

	data->enrolling = false;
	data->enroll_samples_captured = 0;

	LOG_INF("Enrollment aborted");

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_template_store(const struct device *dev, uint16_t id,
					  const uint8_t *template_data, size_t size)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = 0;
	int slot = -1;

	if (size > BIOMETRICS_EMUL_TEMPLATE_SIZE) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Check if ID already exists */
	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (data->templates[i].valid && data->templates[i].id == id) {
			slot = i;
			break;
		}
	}

	/* Find empty slot if not updating */
	if (slot < 0) {
		if (data->template_count >= BIOMETRICS_EMUL_MAX_TEMPLATES) {
			ret = -ENOSPC;
			goto unlock;
		}
		for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
			if (!data->templates[i].valid) {
				slot = i;
				data->template_count++;
				break;
			}
		}
	}

	data->templates[slot].valid = true;
	data->templates[slot].id = id;
	memcpy(data->templates[slot].data, template_data, size);
	data->templates[slot].size = size;

	LOG_INF("Template %u stored in slot %d", id, slot);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_template_read(const struct device *dev, uint16_t id,
					 uint8_t *template_data, size_t size)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = -ENOENT;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (data->templates[i].valid && data->templates[i].id == id) {
			if (size < data->templates[i].size) {
				ret = -EINVAL;
			} else {
				memcpy(template_data, data->templates[i].data,
				       data->templates[i].size);
				ret = data->templates[i].size;
			}
			break;
		}
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_template_delete(const struct device *dev, uint16_t id)
{
	struct biometrics_emul_data *data = dev->data;
	int ret = -ENOENT;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (data->templates[i].valid && data->templates[i].id == id) {
			data->templates[i].valid = false;
			data->template_count--;
			ret = 0;
			LOG_INF("Template %u deleted", id);
			break;
		}
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_template_delete_all(const struct device *dev)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		data->templates[i].valid = false;
	}
	data->template_count = 0;

	LOG_INF("All templates deleted");

	k_mutex_unlock(&data->lock);
	return 0;
}

static int biometrics_emul_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
					 size_t *actual_count)
{
	struct biometrics_emul_data *data = dev->data;
	size_t count = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (count >= max_count) {
			break;
		}

		if (data->templates[i].valid) {
			ids[count++] = data->templates[i].id;
		}
	}

	*actual_count = count;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int biometrics_emul_match_verify(struct biometrics_emul_data *data, uint16_t template_id)
{
	int score;

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (!data->templates[i].valid || data->templates[i].id != template_id) {
			continue;
		}

		score = data->next_match_score > 0 ? data->next_match_score : 80;
		if (score >= data->match_threshold) {
			data->last_matched_id = template_id;
			return score;
		}
		return -ENOENT;
	}
	return -ENOENT;
}

static int biometrics_emul_match_identify(struct biometrics_emul_data *data)
{
	uint16_t target_id = 0;
	int score;

	/* Determine which template to match against */
	if (data->next_match_id > 0) {
		target_id = data->next_match_id;
	} else if (data->template_count == 0) {
		return -ENOENT;
	}

	for (int i = 0; i < BIOMETRICS_EMUL_MAX_TEMPLATES; i++) {
		if (!data->templates[i].valid) {
			continue;
		}

		/* Match specific ID or first valid template */
		if (target_id != 0 && data->templates[i].id != target_id) {
			continue;
		}

		score = data->next_match_score > 0 ? data->next_match_score : 80;
		data->last_matched_id = data->templates[i].id;
		return score;
	}

	return -ENOENT;
}

static int biometrics_emul_match(const struct device *dev, enum biometric_match_mode mode,
				 uint16_t template_id, k_timeout_t timeout,
				 struct biometric_match_result *result)
{
	struct biometrics_emul_data *data = dev->data;
	int ret;
	int score;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->force_capture_timeout) {
		ret = -ETIMEDOUT;
		goto unlock;
	}

	if (data->force_match_fail) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Simulate capture delay */
	k_mutex_unlock(&data->lock);
	k_sleep(K_MSEC(100));
	k_mutex_lock(&data->lock, K_FOREVER);

	data->last_image_quality = data->next_image_quality > 0 ? data->next_image_quality
								: BIOMETRICS_EMUL_DEFAULT_QUALITY;

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		score = biometrics_emul_match_verify(data, template_id);
	} else {
		score = biometrics_emul_match_identify(data);
	}

	if (score >= 0) {
		if (result != NULL) {
			result->confidence = score;
			result->template_id = data->last_matched_id;
			result->image_quality = (uint8_t)data->last_image_quality;
		}
		ret = 0;
	} else {
		ret = score;
	}

	LOG_INF("Match result: %d (mode=%d, template_id=%u)", score, mode, template_id);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int biometrics_emul_led_control(const struct device *dev, enum biometric_led_state state)
{
	struct biometrics_emul_data *data = dev->data;

	if (state > BIOMETRIC_LED_BREATHE) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->led_state = state;
	k_mutex_unlock(&data->lock);

	LOG_INF("LED state set to %d", state);
	return 0;
}

static DEVICE_API(biometric, biometrics_emul_api) = {
	.get_capabilities = biometrics_emul_get_capabilities,
	.attr_set = biometrics_emul_attr_set,
	.attr_get = biometrics_emul_attr_get,
	.enroll_start = biometrics_emul_enroll_start,
	.enroll_capture = biometrics_emul_enroll_capture,
	.enroll_finalize = biometrics_emul_enroll_finalize,
	.enroll_abort = biometrics_emul_enroll_abort,
	.template_store = biometrics_emul_template_store,
	.template_read = biometrics_emul_template_read,
	.template_delete = biometrics_emul_template_delete,
	.template_delete_all = biometrics_emul_template_delete_all,
	.template_list = biometrics_emul_template_list,
	.match = biometrics_emul_match,
	.led_control = biometrics_emul_led_control,
};

/* Emulator control functions for tests */

void biometrics_emul_set_match_score(const struct device *dev, int score)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->next_match_score = score;
	k_mutex_unlock(&data->lock);
}

void biometrics_emul_set_match_id(const struct device *dev, int id)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->next_match_id = id;
	k_mutex_unlock(&data->lock);
}

void biometrics_emul_set_match_fail(const struct device *dev, bool fail)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->force_match_fail = fail;
	k_mutex_unlock(&data->lock);
}

void biometrics_emul_set_capture_timeout(const struct device *dev, bool timeout)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->force_capture_timeout = timeout;
	k_mutex_unlock(&data->lock);
}

void biometrics_emul_set_image_quality(const struct device *dev, int quality)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->next_image_quality = quality;
	k_mutex_unlock(&data->lock);
}

enum biometric_led_state biometrics_emul_get_led_state(const struct device *dev)
{
	struct biometrics_emul_data *data = dev->data;
	enum biometric_led_state state;

	k_mutex_lock(&data->lock, K_FOREVER);
	state = data->led_state;
	k_mutex_unlock(&data->lock);

	return state;
}

static int biometrics_emul_init(const struct device *dev)
{
	struct biometrics_emul_data *data = dev->data;

	k_mutex_init(&data->lock);

	data->match_threshold = BIOMETRICS_EMUL_DEFAULT_THRESHOLD;
	data->enrollment_quality = BIOMETRICS_EMUL_DEFAULT_QUALITY;
	data->security_level = BIOMETRICS_EMUL_DEFAULT_SECURITY;
	data->timeout_ms = BIOMETRICS_EMUL_DEFAULT_TIMEOUT;
	data->anti_spoof_level = 5;
	data->last_image_quality = 0;
	data->last_matched_id = -1;

	data->template_count = 0;
	data->enrolling = false;
	data->led_state = BIOMETRIC_LED_OFF;

	data->next_match_score = 0;
	data->next_match_id = 0;
	data->force_match_fail = false;
	data->force_capture_timeout = false;
	data->next_image_quality = 0;

	LOG_INF("Biometrics emulator initialized");

	return 0;
}

#define BIOMETRICS_EMUL_DEFINE(inst)                                                               \
	static struct biometrics_emul_data biometrics_emul_data_##inst;                            \
                                                                                                   \
	static const struct biometrics_emul_config biometrics_emul_config_##inst = {               \
		.type = BIOMETRIC_TYPE_FINGERPRINT,                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, biometrics_emul_init, NULL, &biometrics_emul_data_##inst,      \
			      &biometrics_emul_config_##inst, POST_KERNEL,                         \
			      CONFIG_BIOMETRICS_INIT_PRIORITY, &biometrics_emul_api);

DT_INST_FOREACH_STATUS_OKAY(BIOMETRICS_EMUL_DEFINE)
