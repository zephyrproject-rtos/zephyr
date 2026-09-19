/*
 * Copyright 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/transport.h>
#include <zephyr/drivers/firmware/scmi/util.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/tee.h>

LOG_MODULE_REGISTER(scmi_optee);

#define DT_DRV_COMPAT linaro_scmi_optee

#define SCMI_OPTEE_MAX_MSG_SIZE			128U

/*
 * Page size used in non-contiguous buffer entries
 */
#define SCMI_OPTEE_MSG_PAGE_SIZE		4096U

/* Private login method for REE kernel clients */
#define TEE_LOGIN_REE_KERNEL			0x80000000

/*
 * Defines the number of available memory references in an open session or
 * invoke command operation payload.
 */
#define TEEC_CONFIG_PAYLOAD_REF_COUNT		4U

#define PTA_SCMI_UUID { 0xa8cfe406, 0xd4f5, 0x4a2e, \
			{ 0x9f, 0x8d, 0xa2, 0x5d, 0xc7, 0x54, 0xc0, 0x99 } }

/*
 * OP-TEE SCMI service capabilities bit flags (32bit)
 *
 * PTA_SCMI_CAPS_SMT_HEADER
 * When set, OP-TEE supports command using SMT header protocol (SCMI shmem) in
 * shared memory buffers to carry SCMI protocol synchronisation information.
 *
 * PTA_SCMI_CAPS_MSG_HEADER
 * When set, OP-TEE supports command using MSG header protocol in an OP-TEE
 * shared memory to carry SCMI protocol synchronisation information and SCMI
 * message payload.
 */
#define PTA_SCMI_CAPS_NONE		0
#define PTA_SCMI_CAPS_SMT_HEADER	BIT(0)
#define PTA_SCMI_CAPS_MSG_HEADER	BIT(1)
#define PTA_SCMI_CAPS_MASK		(PTA_SCMI_CAPS_SMT_HEADER | PTA_SCMI_CAPS_MSG_HEADER)

enum scmi_optee_pta_cmd {
	PTA_SCMI_CMD_CAPABILITIES = 0,
	PTA_SCMI_CMD_PROCESS_SMT_CHANNEL = 1,
	PTA_SCMI_CMD_PROCESS_SMT_CHANNEL_MESSAGE = 2,
	PTA_SCMI_CMD_GET_CHANNEL = 3,
	PTA_SCMI_CMD_PROCESS_MSG_CHANNEL = 4,
};

struct scmi_optee_msg_layout {
	uint32_t msg_hdr;
	uint8_t payload[];
} __packed;

/**
 * struct scmi_optee_channel - Description of an OP-TEE SCMI channel
 *
 * @channel_id: OP-TEE channel ID used for this transport
 * @tee_session: TEE session identifier
 * @caps: OP-TEE SCMI channel capabilities
 * @rx_len: Response size
 * @lock: Mutex protection on channel access
 * @msg: Shared memory protocol handle for SCMI request and
 *   synchronous response
 * @tee_shm: TEE shared memory handle @req or NULL if using IOMEM shmem
 */
struct scmi_optee_channel {
	uint32_t channel_id;
	uint32_t tee_session;
	uint32_t caps;
	uint32_t rx_len;
	struct k_mutex lock;
	struct scmi_optee_msg_layout *msg;
	struct tee_shm *tee_shm;
};

/**
 * This type contains a Universally Unique Resource Identifier (UUID) type as
 * defined in RFC4122. These UUID values are used to identify Trusted
 * Applications.
 */
typedef struct {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[8];
} TEEC_UUID;

static int setup_dynamic_shmem(const struct device *tee_dev, struct scmi_optee_channel *optee_chan)
{
	int ret;

	ret = tee_add_shm(tee_dev, NULL, SCMI_OPTEE_MSG_PAGE_SIZE, SCMI_OPTEE_MAX_MSG_SIZE,
			  TEE_SHM_ALLOC | TEE_SHM_REGISTER, &optee_chan->tee_shm);
	if (ret < 0) {
		LOG_ERR("shmem allocation failed: %d", ret);
		return ret;
	}

	memset(optee_chan->tee_shm->addr, 0, SCMI_OPTEE_MAX_MSG_SIZE);
	optee_chan->msg = optee_chan->tee_shm->addr;
	optee_chan->rx_len = SCMI_OPTEE_MAX_MSG_SIZE;

	return 0;
}

static void uuid_to_octets(uint8_t d[TEE_UUID_LEN], const TEEC_UUID *s)
{
	d[0] = s->timeLow >> 24;
	d[1] = s->timeLow >> 16;
	d[2] = s->timeLow >> 8;
	d[3] = s->timeLow;
	d[4] = s->timeMid >> 8;
	d[5] = s->timeMid;
	d[6] = s->timeHiAndVersion >> 8;
	d[7] = s->timeHiAndVersion;
	memcpy(d + 8, s->clockSeqAndNode, sizeof(s->clockSeqAndNode));
}

/* Open a session toward SCMI OP-TEE service with REE_KERNEL identity */
static int open_session(const struct device *tee_dev, uint32_t *tee_session)
{
	struct tee_open_session_arg arg = { 0 };
	uint32_t session_id = 0;
	int ret;
	struct tee_param params[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	TEEC_UUID pta_scmi_uuid = PTA_SCMI_UUID;

	memset(params, 0, sizeof(struct tee_param) *
				TEEC_CONFIG_PAYLOAD_REF_COUNT);

	uuid_to_octets(arg.uuid, &pta_scmi_uuid);
	arg.clnt_login = TEE_LOGIN_REE_KERNEL;

	ret = tee_open_session(tee_dev, &arg, 0, params, &session_id);
	if (ret < 0) {
		LOG_ERR("TEE driver open session failed: %d / %d", ret, arg.ret);
		return ret;
	}

	if (arg.ret != 0) {
		LOG_ERR("OP-TEE PTA returned error: %#x", arg.ret);
		return -EACCES;
	}

	*tee_session = session_id;
	return 0;
}

static void close_session(const struct device *tee_dev, uint32_t *tee_session)
{
	tee_close_session(tee_dev, *tee_session);
}

static int get_channel(const struct device *dev, struct scmi_optee_channel *optee_chan)
{
	struct tee_invoke_func_arg arg = { };
	struct tee_param param[1] = { };
	int ret;

	arg.func = PTA_SCMI_CMD_GET_CHANNEL;
	arg.session = optee_chan->tee_session;

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].a = optee_chan->channel_id;
	param[0].b = PTA_SCMI_CAPS_MSG_HEADER;

	ret = tee_invoke_func(dev, &arg, 1, param);

	if (ret < 0 || arg.ret != 0) {
		LOG_ERR("Can't get channel %u: %d / %#x\n", optee_chan->channel_id, ret, arg.ret);
		return -EOPNOTSUPP;
	}

	optee_chan->channel_id = param[0].a;
	optee_chan->caps = param[0].b;

	return 0;
}

static int scmi_optee_setup_chan(const struct device *transport, struct scmi_channel *chan,
				 bool tx)
{
	if (!tx) {
		return -ENOTSUP;
	}

	const struct device *const tee_dev = DEVICE_DT_GET(
		DT_COMPAT_GET_ANY_STATUS_OKAY(linaro_optee_tz));
	struct scmi_optee_channel *optee_chan = chan->data;
	int ret;

	k_mutex_init(&optee_chan->lock);

	ret = setup_dynamic_shmem(tee_dev, optee_chan);
	if (ret != 0) {
		return ret;
	}

	ret = open_session(tee_dev, &optee_chan->tee_session);
	if (ret != 0) {
		goto err_free_shm;
	}

	ret = get_channel(tee_dev, optee_chan);
	if (ret != 0) {
		goto err_close_sess;
	}

	/* OP-TEE SMC calls are synchronous traps into Secure EL1 */
	chan->polling_only = true;

	LOG_DBG("OP-TEE channel setup complete (polling mode)");

	return 0;

err_close_sess:
	close_session(tee_dev, &optee_chan->tee_session);
err_free_shm:
	if (optee_chan->tee_shm) {
		tee_rm_shm(tee_dev, optee_chan->tee_shm);
	}

	return ret;
}

static int invoke_process_msg_channel(const struct device *tee_dev,
				      struct scmi_optee_channel *optee_chan,
				      size_t msg_size)
{
	struct tee_invoke_func_arg arg = {
		.func = PTA_SCMI_CMD_PROCESS_MSG_CHANNEL,
		.session = optee_chan->tee_session,
	};

	struct tee_param param[3] = {0};
	int ret;

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].a = optee_chan->channel_id;

	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].a = 0;
	param[1].b = msg_size;
	param[1].c = (uint64_t)(uintptr_t)optee_chan->tee_shm;

	param[2].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[2].a = 0;
	param[2].b = SCMI_OPTEE_MAX_MSG_SIZE;
	param[2].c = (uint64_t)(uintptr_t)optee_chan->tee_shm;

	ret = tee_invoke_func(tee_dev, &arg, 3, param);
	if (ret < 0 || arg.ret != 0) {
		LOG_ERR("PTA_SCMI_CMD_PROCESS_MSG_CHANNEL failed for channel %u: %d / %#x",
				optee_chan->channel_id, ret, arg.ret);
		return -EIO;
	}

	optee_chan->rx_len = param[2].b;

	return 0;
}

static int scmi_optee_write_message(struct scmi_optee_channel *optee_chan,
							 struct scmi_message *msg)
{
	struct scmi_optee_msg_layout *layout;

	if (!msg->content && msg->len) {
		return -EINVAL;
	}

	if ((sizeof(*layout) + msg->len) > SCMI_OPTEE_MAX_MSG_SIZE) {
		return -EINVAL;
	}

	layout = optee_chan->msg;
	layout->msg_hdr = msg->hdr;

	if (msg->content) {
		memcpy(layout->payload, msg->content, msg->len);
	}

	return 0;
}

static int scmi_optee_send_message(const struct device *transport,
				   struct scmi_channel *chan,
				   struct scmi_message *msg, bool use_polling)
{
	const struct device *const tee_dev = DEVICE_DT_GET(
		DT_COMPAT_GET_ANY_STATUS_OKAY(linaro_optee_tz));
	struct scmi_optee_channel *optee_chan;
	size_t cmd_size;
	int ret;

	if (!chan || !chan->data || !msg) {
		return -EINVAL;
	}
	optee_chan = chan->data;

	if (!device_is_ready(tee_dev)) {
		LOG_ERR("OP-TEE TEE device not ready");
		return -ENODEV;
	}

	k_mutex_lock(&optee_chan->lock, K_FOREVER);

	ret = scmi_optee_write_message(optee_chan, msg);
	if (ret < 0) {
		LOG_ERR("failed to write message to optee shmem: %d", ret);
		goto unlock_err;
	}

	cmd_size = sizeof(struct scmi_optee_msg_layout) + msg->len;

	ret = invoke_process_msg_channel(tee_dev, optee_chan, cmd_size);
	if (ret < 0) {
		LOG_ERR("failed to invoke PTA_SCMI_CMD_PROCESS_MSG_CHANNEL: %d", ret);
		goto unlock_err;
	}
	return 0;

unlock_err:
	k_mutex_unlock(&optee_chan->lock);
	return ret;
}

static int scmi_optee_read_message(const struct device *transport, struct scmi_channel *chan,
								   struct scmi_message *msg)
{
	struct scmi_optee_channel *optee_chan;
	struct scmi_optee_msg_layout *layout;
	size_t rx_payload_len;
	int ret = 0;

	if (!chan || !chan->data || !msg) {
		return -EINVAL;
	}

	optee_chan = chan->data;
	layout = (struct scmi_optee_msg_layout *)optee_chan->msg;

	/* Verify response buffer contains at least the 4-byte SCMI message header */
	if (optee_chan->rx_len < sizeof(struct scmi_optee_msg_layout)) {
		LOG_ERR("Invalid SCMI OP-TEE response length: %d", optee_chan->rx_len);
		ret = -EIO;
		goto out_unlock;
	}

	msg->hdr = layout->msg_hdr;
	rx_payload_len = optee_chan->rx_len - sizeof(struct scmi_optee_msg_layout);

	if (msg->content && msg->len > 0) {
		size_t copy_len = MIN(msg->len, rx_payload_len);

		memcpy(msg->content, layout->payload, copy_len);
		msg->len = copy_len;
	} else if (rx_payload_len > 0) {
		/* Content buffer was NULL or size 0 despite receiving response data */
		msg->len = 0;
	}

out_unlock:
	k_mutex_unlock(&optee_chan->lock);

	return ret;
}


static bool scmi_optee_channel_is_free(const struct device *transport, struct scmi_channel *chan)
{
	/*
	 * OP-TEE PTA transport is fully synchronous. The thread blocks inside
	 * tee_invoke_func() until OP-TEE finishes processing, so the channel
	 * is guaranteed to be idle when queried.
	 */
	return true;
}

static struct scmi_transport_api scmi_optee_api = {
	.setup_chan = scmi_optee_setup_chan,
	.send_message = scmi_optee_send_message,
	.read_message = scmi_optee_read_message,
	.channel_is_free = scmi_optee_channel_is_free,
};

#define SCMI_OPTEE_CHAN_NAME(proto, idx)					\
	CONCAT(SCMI_TRANSPORT_CHAN_NAME(proto, idx), _, priv)

#define SCMI_OPTEE_BASE_CHAN_DEFINE_PRIV(node_id, proto, idx)			\
	static struct scmi_optee_channel SCMI_OPTEE_CHAN_NAME(proto, idx) = {	\
		.channel_id = DT_PROP_OR(node_id, linaro_optee_channel_id, 0),	\
	};

#define SCMI_OPTEE_CHAN_DEFINE(node_id, proto, idx)				\
	SCMI_OPTEE_BASE_CHAN_DEFINE_PRIV(node_id, proto, idx);			\
	DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto,			\
				      &(SCMI_OPTEE_CHAN_NAME(proto, idx)));

#define DT_INST_SCMI_OPTEE_BASE_CHAN_DEFINE(inst)				\
	SCMI_OPTEE_CHAN_DEFINE(DT_INST(inst, DT_DRV_COMPAT), SCMI_PROTOCOL_BASE, 0)

#define DT_INST_SCMI_OPTEE_DEFINE(inst, level, prio, api)			\
	DT_INST_SCMI_OPTEE_BASE_CHAN_DEFINE(inst)				\
	DT_INST_SCMI_TRANSPORT_DEFINE(inst, NULL, NULL, NULL, level, prio, api)

/*
 * NOTE: the OP-TEE TEE subsystem driver (linaro,optee-tz) must initialize before this transport.
 */
DT_INST_SCMI_OPTEE_DEFINE(0, POST_KERNEL, CONFIG_ARM_SCMI_TRANSPORT_INIT_PRIORITY, &scmi_optee_api);
