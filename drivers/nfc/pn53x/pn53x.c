/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "pn53x.h"

LOG_MODULE_REGISTER(nfc_pn53x, CONFIG_NFC_DRIVERS_LOG_LEVEL);

#define PN53X_PREAMBLE   0x00U
#define PN53X_STARTCODE1 0x00U
#define PN53X_STARTCODE2 0xFFU
#define PN53X_POSTAMBLE  0x00U
#define PN53X_TFI_HOST   0xD4U
#define PN53X_TFI_TARGET 0xD5U

#define PN53X_CMD_GET_FIRMWARE_VERSION   0x02U
#define PN53X_CMD_SAM_CONFIGURATION      0x14U
#define PN53X_CMD_RF_CONFIGURATION       0x32U
#define PN53X_CMD_IN_LIST_PASSIVE_TARGET 0x4AU
#define PN53X_CMD_IN_DATA_EXCHANGE       0x40U
#define PN53X_CMD_IN_RELEASE             0x52U

#define PN53X_RF_CFG_MAX_RETRIES 0x05U
#define PN53X_MX_RTY_ATR         0xFFU
#define PN53X_MX_RTY_PSL         0x01U
#define PN53X_MX_RTY_PASSIVE_ACT 0x0AU

#define PN53X_ACTIVATION_ATTEMPT_MS 8U
#define PN53X_FRAME_OVERHEAD_MS     30U
#define PN53X_POLL_TIMEOUT_MS                                                                      \
	(PN53X_FRAME_OVERHEAD_MS + PN53X_ACTIVATION_ATTEMPT_MS * (PN53X_MX_RTY_PASSIVE_ACT + 1U))

#define PN53X_BRTY_106_TYPE_A 0x00U

#define PN53X_ERR_FRAME_LEN   1U
#define PN53X_ERR_FRAME_CODE  0x7FU
#define PN53X_ACK_LEN         6U
#define PN53X_BOOT_TIMEOUT_MS 500U
#define PN53X_BOOT_POLL_MS    5U

static const uint8_t pn53x_ack[PN53X_ACK_LEN] = {0x00U, 0x00U, 0xFFU, 0x00U, 0xFFU, 0x00U};

static inline int pn53x_write(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct pn53x_config *cfg = dev->config;

	return cfg->transport->write(dev, buf, len);
}

static inline int pn53x_read(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct pn53x_config *cfg = dev->config;

	return cfg->transport->read(dev, buf, len);
}

static void pn53x_arm_irq(const struct device *dev)
{
	const struct pn53x_config *cfg = dev->config;
	struct pn53x_data *data = dev->data;

	if (cfg->irq_gpio.port != NULL) {
		k_sem_reset(&data->irq_sem);
	}
}

static inline int pn53x_wait_ready(const struct device *dev, k_timeout_t timeout)
{
	const struct pn53x_config *cfg = dev->config;

	return cfg->transport->wait_ready(dev, timeout);
}

static size_t pn53x_build_frame(const uint8_t *payload, uint8_t payload_len, uint8_t *frame)
{
	uint8_t len = payload_len + 1U;
	uint8_t dcs = PN53X_TFI_HOST;
	size_t i = 0;

	frame[i++] = PN53X_PREAMBLE;
	frame[i++] = PN53X_STARTCODE1;
	frame[i++] = PN53X_STARTCODE2;
	frame[i++] = len;
	frame[i++] = (uint8_t)(-len);
	frame[i++] = PN53X_TFI_HOST;

	for (uint8_t j = 0; j < payload_len; j++) {
		frame[i++] = payload[j];
		dcs += payload[j];
	}

	frame[i++] = (uint8_t)(-dcs);
	frame[i++] = PN53X_POSTAMBLE;

	return i;
}

static int pn53x_send(const struct device *dev, const uint8_t *payload, uint8_t payload_len)
{
	struct pn53x_data *data = dev->data;
	uint8_t ack[PN53X_ACK_LEN];
	size_t frame_len;
	int ret;

	frame_len = pn53x_build_frame(payload, payload_len, data->buf);

	pn53x_arm_irq(dev);

	ret = pn53x_write(dev, data->buf, frame_len);
	if (ret < 0) {
		return ret;
	}

	ret = pn53x_wait_ready(dev, K_MSEC(100));
	if (ret < 0) {
		return ret;
	}

	ret = pn53x_read(dev, ack, sizeof(ack));
	if (ret < 0) {
		return ret;
	}

	if (memcmp(ack, pn53x_ack, sizeof(ack)) != 0) {
		LOG_ERR("Missing ACK");
		return -EIO;
	}

	return 0;
}

static int pn53x_recv(const struct device *dev, uint8_t resp_cmd, uint8_t *resp, size_t resp_max,
		      size_t *resp_len, k_timeout_t timeout)
{
	struct pn53x_data *data = dev->data;
	size_t read_len = MIN(PN53X_FRAME_MAXLEN, resp_max + 9U);
	uint8_t len;
	uint8_t dcs;
	int ret;

	ret = pn53x_wait_ready(dev, timeout);
	if (ret < 0) {
		return ret;
	}

	pn53x_arm_irq(dev);

	ret = pn53x_read(dev, data->buf, read_len);
	if (ret < 0) {
		return ret;
	}

	if (data->buf[0] != PN53X_PREAMBLE || data->buf[1] != PN53X_STARTCODE1 ||
	    data->buf[2] != PN53X_STARTCODE2) {
		LOG_ERR("Bad frame start");
		return -EIO;
	}

	len = data->buf[3];
	if (((len + data->buf[4]) & 0xFFU) != 0U) {
		LOG_ERR("Bad length checksum");
		return -EIO;
	}

	if (len == PN53X_ERR_FRAME_LEN && data->buf[5] == PN53X_ERR_FRAME_CODE) {
		LOG_DBG("controller rejected the frame");
		return -EPROTO;
	}

	if (len < 2U || (6U + len) > read_len) {
		LOG_ERR("Bad length %u", len);
		return -EIO;
	}

	if (data->buf[5] != PN53X_TFI_TARGET || data->buf[6] != resp_cmd) {
		LOG_ERR("Unexpected response 0x%02x", data->buf[6]);
		return -EIO;
	}

	dcs = 0U;
	for (uint8_t i = 0; i < len; i++) {
		dcs += data->buf[5U + i];
	}
	if (((dcs + data->buf[5U + len]) & 0xFFU) != 0U) {
		LOG_ERR("Bad data checksum");
		return -EIO;
	}

	*resp_len = len - 2U;
	if (*resp_len > resp_max) {
		return -ENOMEM;
	}

	if (*resp_len > 0U) {
		memcpy(resp, &data->buf[7], *resp_len);
	}

	return 0;
}

static int pn53x_cmd(const struct device *dev, const uint8_t *payload, uint8_t payload_len,
		     uint8_t *resp, size_t resp_max, size_t *resp_len, k_timeout_t timeout)
{
	uint8_t resp_cmd = payload[0] + 1U;
	int ret;

	ret = pn53x_send(dev, payload, payload_len);
	if (ret < 0) {
		return ret;
	}

	return pn53x_recv(dev, resp_cmd, resp, resp_max, resp_len, timeout);
}

static void pn53x_abort(const struct device *dev)
{
	(void)pn53x_write(dev, pn53x_ack, sizeof(pn53x_ack));
}

static int pn53x_poll_once(const struct device *dev, struct nfc_target *target)
{
	uint8_t cmd[] = {PN53X_CMD_IN_LIST_PASSIVE_TARGET, 0x01U, PN53X_BRTY_106_TYPE_A};
	uint8_t resp[64];
	size_t resp_len;
	size_t ats_len;
	uint8_t nfcid_len;
	int ret;

	ret = pn53x_cmd(dev, cmd, sizeof(cmd), resp, sizeof(resp), &resp_len,
			K_MSEC(PN53X_POLL_TIMEOUT_MS));
	if (ret < 0) {
		if (ret == -ETIMEDOUT) {
			pn53x_abort(dev);
		}
		return ret;
	}

	if (resp_len < 1U || resp[0] == 0U) {
		return -EAGAIN;
	}

	if (resp_len < 6U) {
		return -EIO;
	}

	memset(target, 0, sizeof(*target));
	target->tech = NFC_TECH_A;
	target->proto = NFC_PROTO_ISO14443A;
	target->a.atqa[0] = resp[2];
	target->a.atqa[1] = resp[3];
	target->a.sak = resp[4];

	nfcid_len = resp[5];
	if (nfcid_len > NFC_UID_MAXLEN || (6U + nfcid_len) > resp_len) {
		return -EIO;
	}

	target->a.uid_len = nfcid_len;
	memcpy(target->a.uid, &resp[6], nfcid_len);

	ats_len = resp_len - (6U + nfcid_len);
	if (ats_len > 0U && ats_len <= NFC_ATS_MAXLEN) {
		target->a.ats_len = (uint8_t)ats_len;
		memcpy(target->a.ats, &resp[6U + nfcid_len], ats_len);
	}

	return 0;
}

static void pn53x_poll_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct pn53x_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct nfc_target target;
		int ret;

		k_sem_take(&data->poll_kick, K_FOREVER);

		while (atomic_get(&data->polling) == 1) {
			k_mutex_lock(&data->lock, K_FOREVER);
			ret = pn53x_poll_once(dev, &target);
			if (ret == 0 && data->target_cb != NULL) {
				data->target_cb(dev, &target, data->target_ud);
			}
			k_mutex_unlock(&data->lock);

			if (ret < 0 && ret != -EAGAIN) {
				LOG_WRN("Poll error (%d)", ret);
				k_sleep(K_MSEC(50));
			}
		}
	}
}

static int pn53x_offload_poll_start(const struct device *dev, nfc_proto_t protos,
				    const struct nfc_poll_config *cfg, nfc_target_cb_t cb,
				    void *user_data)
{
	struct pn53x_data *data = dev->data;

	ARG_UNUSED(cfg);

	if ((protos & NFC_PROTO_ISO14443A) == 0U) {
		return -ENOTSUP;
	}

	if (!atomic_cas(&data->polling, 0, 1)) {
		return -EALREADY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->poll_protos = protos;
	data->target_cb = cb;
	data->target_ud = user_data;
	k_mutex_unlock(&data->lock);

	k_sem_give(&data->poll_kick);

	return 0;
}

static int pn53x_offload_poll_stop(const struct device *dev)
{
	struct pn53x_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	atomic_set(&data->polling, 0);
	data->target_cb = NULL;
	data->target_ud = NULL;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int pn53x_offload_exchange(const struct device *dev, const struct nfc_target *target,
				  const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
				  uint16_t *rx_len, uint32_t timeout_ms)
{
	struct pn53x_data *data = dev->data;
	size_t resp_len;
	int ret;

	ARG_UNUSED(target);

	if ((size_t)tx_len + 2U + 8U > PN53X_FRAME_MAXLEN) {
		return -ENOMEM;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->xchg_cmd[0] = PN53X_CMD_IN_DATA_EXCHANGE;
	data->xchg_cmd[1] = 0x01U;
	memcpy(&data->xchg_cmd[2], tx_data, tx_len);

	ret = pn53x_cmd(dev, data->xchg_cmd, tx_len + 2U, data->xchg_resp,
			MIN((size_t)*rx_len + 1U, sizeof(data->xchg_resp)), &resp_len,
			K_MSEC(timeout_ms));
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		return ret;
	}

	if (resp_len < 1U) {
		return -EIO;
	}
	if ((data->xchg_resp[0] & 0x3FU) != 0U) {
		LOG_ERR("Exchange status 0x%02x", data->xchg_resp[0]);
		return -EIO;
	}

	resp_len -= 1U;
	if (resp_len > *rx_len) {
		return -ENOMEM;
	}

	memcpy(rx_data, &data->xchg_resp[1], resp_len);
	*rx_len = (uint16_t)resp_len;

	return 0;
}

static int pn53x_offload_release(const struct device *dev, const struct nfc_target *target)
{
	struct pn53x_data *data = dev->data;
	uint8_t cmd[] = {PN53X_CMD_IN_RELEASE, 0x01U};
	uint8_t resp[4];
	size_t resp_len;
	int ret;

	ARG_UNUSED(target);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = pn53x_cmd(dev, cmd, sizeof(cmd), resp, sizeof(resp), &resp_len, K_MSEC(100));
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		return ret;
	}

	if (resp_len < 1U) {
		return -EIO;
	}
	if ((resp[0] & 0x3FU) != 0U) {
		LOG_DBG("release status 0x%02x", resp[0]);
		return -EIO;
	}

	return 0;
}

static nfc_proto_t pn53x_supported_protocols(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NFC_PROTO_ISO14443A;
}

DEVICE_API(nfc, pn53x_nfc_api) = {
	.supported_protocols = pn53x_supported_protocols,
	.offload_poll_start = pn53x_offload_poll_start,
	.offload_poll_stop = pn53x_offload_poll_stop,
	.offload_exchange = pn53x_offload_exchange,
	.offload_release = pn53x_offload_release,
};

static int pn53x_configure_gpio(const struct gpio_dt_spec *spec, gpio_flags_t flags)
{
	if (spec->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(spec)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(spec, flags);
}

static int pn53x_reset(const struct device *dev)
{
	const struct pn53x_config *cfg = dev->config;
	int ret;

	ret = pn53x_configure_gpio(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(2));

	if (cfg->reset_gpio.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->reset_gpio, 0);
	}

	k_sleep(K_MSEC(5));

	return 0;
}

static int pn53x_sam_configure(const struct device *dev)
{
	uint8_t cmd[] = {PN53X_CMD_SAM_CONFIGURATION, 0x01U, 0x14U, 0x01U};
	uint8_t resp[4];
	size_t resp_len;

	return pn53x_cmd(dev, cmd, sizeof(cmd), resp, sizeof(resp), &resp_len, K_MSEC(100));
}

static int pn53x_set_retries(const struct device *dev, uint8_t passive)
{
	uint8_t cmd[] = {PN53X_CMD_RF_CONFIGURATION, PN53X_RF_CFG_MAX_RETRIES, PN53X_MX_RTY_ATR,
			 PN53X_MX_RTY_PSL, passive};
	uint8_t resp[4];
	size_t resp_len;

	return pn53x_cmd(dev, cmd, sizeof(cmd), resp, sizeof(resp), &resp_len, K_MSEC(100));
}

static int pn53x_set_max_retries(const struct device *dev)
{
	return pn53x_set_retries(dev, PN53X_MX_RTY_PASSIVE_ACT);
}

static int pn53x_get_firmware(const struct device *dev)
{
	uint8_t cmd[] = {PN53X_CMD_GET_FIRMWARE_VERSION};
	uint8_t resp[4];
	size_t resp_len;
	int ret;

	ret = pn53x_cmd(dev, cmd, sizeof(cmd), resp, sizeof(resp), &resp_len, K_MSEC(100));
	if (ret < 0) {
		return ret;
	}

	if (resp_len < 2U) {
		return -EIO;
	}

	LOG_INF("PN53x IC 0x%02x firmware %u.%u", resp[0], resp[1], resp_len > 2U ? resp[2] : 0U);

	return 0;
}

static void pn53x_irq_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct pn53x_data *data = CONTAINER_OF(cb, struct pn53x_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->irq_sem);
}

static int pn53x_setup_irq(const struct device *dev)
{
	const struct pn53x_config *cfg = dev->config;
	struct pn53x_data *data = dev->data;
	int ret;

	if (cfg->irq_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->irq_cb, pn53x_irq_handler, BIT(cfg->irq_gpio.pin));
	ret = gpio_add_callback(cfg->irq_gpio.port, &data->irq_cb);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static int pn53x_wait_boot(const struct device *dev)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(PN53X_BOOT_TIMEOUT_MS));
	int ret;

	do {
		ret = pn53x_get_firmware(dev);
		if (ret == 0) {
			return 0;
		}

		k_sleep(K_MSEC(PN53X_BOOT_POLL_MS));
	} while (!sys_timepoint_expired(end));

	return ret;
}

int pn53x_init_common(const struct device *dev)
{
	const struct pn53x_config *cfg = dev->config;
	struct pn53x_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_mutex_init(&data->lock);
	k_sem_init(&data->poll_kick, 0, 1);
	k_sem_init(&data->irq_sem, 0, 1);
	atomic_set(&data->polling, 0);

	ret = pn53x_reset(dev);
	if (ret < 0) {
		LOG_ERR("Reset failed (%d)", ret);
		return ret;
	}

	ret = pn53x_setup_irq(dev);
	if (ret < 0) {
		LOG_ERR("IRQ setup failed (%d)", ret);
		return ret;
	}

	ret = pn53x_wait_boot(dev);
	if (ret < 0) {
		LOG_ERR("Controller not responding (%d)", ret);
		return ret;
	}

	ret = pn53x_sam_configure(dev);
	if (ret < 0) {
		LOG_ERR("SAM configuration failed (%d)", ret);
		return ret;
	}

	ret = pn53x_set_max_retries(dev);
	if (ret < 0) {
		LOG_ERR("Cannot bound the activation retries (%d)", ret);
		return ret;
	}

	k_thread_create(&data->poll_thread, cfg->poll_stack, cfg->poll_stack_size,
			pn53x_poll_thread, (void *)dev, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
	k_thread_name_set(&data->poll_thread, "pn53x_poll");

	return 0;
}
