/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm_scmi_smc);

#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/drivers/firmware/scmi/transport.h>

#if defined(CONFIG_DT_HAS_ARM_SCMI_SMC_ENABLED)
#define DT_DRV_COMPAT arm_scmi_smc
#elif defined(CONFIG_DT_HAS_ARM_SCMI_SMC_PARAM_ENABLED)
#define DT_DRV_COMPAT arm_scmi_smc_param
#else
BUILD_ASSERT(0, "unsupported scmi interface");
#endif

struct scmi_smc_channel {
	/* SHMEM area bound to the channel */
	const struct device *shmem;
	/* ARM  smc-id */
	uint32_t smc_func_id;
	uint32_t param_page;
	uint32_t param_offset;
	uint16_t xfer_seq;
};

#define SHMEM_SIZE      (4096)
#define SHMEM_SHIFT     12
#define SHMEM_PAGE(x)   ((x) >> SHMEM_SHIFT)
#define SHMEM_OFFSET(x) ((x) & (SHMEM_SIZE - 1))

#if defined(CONFIG_ARM_SCMI_SMC_METHOD_SMC)
static void scmi_smccc_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}
#else
static void scmi_smccc_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_hvc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}
#endif /* CONFIG_ARM_SCMI_SMC_METHOD_SMC */

static int scmi_smc_send_message(const struct device *transport, struct scmi_channel *chan,
				 struct scmi_message *msg)
{
	struct scmi_smc_channel *smc_chan;
	struct arm_smccc_res res;
	unsigned long offset;
	unsigned long page;
	int ret;

	smc_chan = chan->data;
	page = smc_chan->param_page;
	offset = smc_chan->param_offset;

	LOG_DBG("smc send seq:%u prot:%02x msg:%02x", SCMI_MSG_XTRACT_TOKEN(msg->hdr),
		SCMI_MSG_XTRACT_PROT_ID(msg->hdr), SCMI_MSG_XTRACT_ID(msg->hdr));

	ret = scmi_shmem_write_message(smc_chan->shmem, msg);
	if (ret) {
		LOG_ERR("failed to write message to shmem: %d", ret);
		return ret;
	}

	scmi_smccc_call(smc_chan->smc_func_id, page, offset, 0, 0, 0, 0, 0, &res);

	/* Only SMCCC_RET_NOT_SUPPORTED is valid error code */
	if (res.a0) {
		ret = -EOPNOTSUPP;
	}

	return ret;
}

static int scmi_smc_read_message(const struct device *transport, struct scmi_channel *chan,
				 struct scmi_message *msg)
{
	struct scmi_smc_channel *smc_chan;
	int ret;

	smc_chan = chan->data;

	ret = scmi_shmem_read_message(smc_chan->shmem, msg);
	LOG_DBG("smc done seq:%u prot:%02x msg:%02x status:%d (%d)",
		SCMI_MSG_XTRACT_TOKEN(msg->hdr), SCMI_MSG_XTRACT_PROT_ID(msg->hdr),
		SCMI_MSG_XTRACT_ID(msg->hdr), msg->status, ret);

	if (!ret && msg->status != SCMI_SUCCESS) {
		return scmi_status_to_errno(msg->status);
	}

	return ret;
}

static bool scmi_smc_channel_is_free(const struct device *transport, struct scmi_channel *chan)
{
	struct scmi_smc_channel *smc_chan = chan->data;

	return scmi_shmem_channel_status(smc_chan->shmem) & SCMI_SHMEM_CHAN_STATUS_BUSY_BIT;
}

static bool scmi_smc_channel_is_polling(const struct device *transport, struct scmi_channel *chan)
{
	return true;
}

static uint16_t scmi_smc_channel_get_token(const struct device *transport,
					   struct scmi_channel *chan)
{
	struct scmi_smc_channel *smc_chan = chan->data;

	return smc_chan->xfer_seq++;
}

static int scmi_smc_setup_chan(const struct device *transport, struct scmi_channel *chan, bool tx)
{
	struct scmi_smc_channel *smc_chan;

	smc_chan = chan->data;
#if !defined(CONFIG_DT_HAS_ARM_SCMI_SMC_PARAM_ENABLED)
	smc_chan->param_page = 0;
	smc_chan->param_offset = 0;
#endif /* CONFIG_DT_HAS_ARM_SCMI_SMC_PARAM_ENABLED */

	/* disable interrupt-based communication */
	scmi_shmem_update_flags(smc_chan->shmem, SCMI_SHMEM_CHAN_FLAG_IRQ_BIT, 0);

	return 0;
}

static const struct scmi_transport_api scmi_smc_api = {
	.setup_chan = scmi_smc_setup_chan,
	.send_message = scmi_smc_send_message,
	.read_message = scmi_smc_read_message,
	.channel_is_free = scmi_smc_channel_is_free,
	.channel_get_token = scmi_smc_channel_get_token,
	.channel_is_polling = scmi_smc_channel_is_polling,
};

#define _SCMI_SMC_CHAN_NAME(proto, idx) CONCAT(SCMI_TRANSPORT_CHAN_NAME(proto, idx), _, priv)

#define _SCMI_SMC_CHAN_DEFINE_PRIV_TX(node_id, proto)                                              \
	static struct scmi_smc_channel _SCMI_SMC_CHAN_NAME(proto, 0) = {                           \
		.shmem = DEVICE_DT_GET(DT_PROP_BY_IDX(node_id, shmem, 0)),                         \
		.smc_func_id = DT_PROP(node_id, arm_smc_id),                                       \
		.param_page = SHMEM_PAGE(DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, shmem, 0))),       \
		.param_offset = SHMEM_OFFSET(DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, shmem, 0))),   \
		.xfer_seq = 1,                                                                     \
	}

#define _SCMI_SMC_CHAN_DEFINE(node_id, proto, idx)                                                 \
	_SCMI_SMC_CHAN_DEFINE_PRIV_TX(node_id, proto);                                             \
	DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto, &(_SCMI_SMC_CHAN_NAME(proto, idx)));

#define DT_INST_SCMI_SMC_BASE_CHAN_DEFINE(inst)                                                    \
	_SCMI_SMC_CHAN_DEFINE(DT_DRV_INST(inst), SCMI_PROTOCOL_BASE, 0)

DT_INST_SCMI_SMC_BASE_CHAN_DEFINE(0);
DT_INST_SCMI_TRANSPORT_DEFINE(0, NULL, NULL, NULL, PRE_KERNEL_1,
			      CONFIG_ARM_SCMI_TRANSPORT_INIT_PRIORITY, &scmi_smc_api);
