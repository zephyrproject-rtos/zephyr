/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/drivers/firmware/scmi/transport.h>
#include <zephyr/drivers/firmware/scmi/util.h>
#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(scmi_smc);

#define DT_DRV_COMPAT arm_scmi_smc

/* SMC channel structure */
struct scmi_smc_channel {
	/* SHMEM area bound to the channel */
	const struct device *shmem;
	/* SMC function ID */
	uint32_t func_id;
};

/*
 * SMC transport uses a single shared channel for all protocols.
 * Unlike mailbox transport, SMC doesn't support per-protocol shmem areas
 * or function IDs - all protocols use the same SMC call and shared memory.
 */

/*
 * Define the SMC transport with a single base channel shared by all protocols.
 * This creates:
 *   1) One scmi_smc_channel structure containing shmem device and func_id
 *   2) One scmi_channel structure for the base protocol
 * All protocols will reference this single shared channel.
 */

#define SCMI_SMC_CHAN_NAME(proto, idx) CONCAT(SCMI_TRANSPORT_CHAN_NAME(proto, idx), _, priv)

#define SCMI_SMC_SHMEM_BY_IDX(node_id, idx)                                                        \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, shmem, idx),			\
		    (DEVICE_DT_GET(DT_PROP_BY_IDX(node_id, shmem, idx))),	\
		    (NULL))

#define SCMI_SMC_BASE_CHAN_DEFINE_PRIV(node_id, proto, idx)                                        \
	static struct scmi_smc_channel SCMI_SMC_CHAN_NAME(proto, idx) = {                          \
		.shmem = SCMI_SMC_SHMEM_BY_IDX(node_id, 0),                                        \
		.func_id = DT_PROP(node_id, arm_smc_id),                                           \
	};

#define SCMI_SMC_CHAN_DEFINE(node_id, proto, idx)                                                  \
	SCMI_SMC_BASE_CHAN_DEFINE_PRIV(node_id, proto, idx);                                       \
	DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto, &(SCMI_SMC_CHAN_NAME(proto, idx)));

#define DT_INST_SCMI_BASE_CHAN_DEFINE(inst)                                                        \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, shmem), "SMC transport must have shmem phandle"); \
                                                                                                   \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, arm_smc_id),                                      \
		     "SMC transport must have arm,smc-id");                                        \
                                                                                                   \
	SCMI_SMC_CHAN_DEFINE(DT_INST(inst, DT_DRV_COMPAT), SCMI_PROTOCOL_BASE, 0)

#define DT_INST_SCMI_SMC_DEFINE(inst, level, prio, api)                                            \
	DT_INST_SCMI_BASE_CHAN_DEFINE(inst)                                                        \
	DT_INST_SCMI_TRANSPORT_DEFINE(inst, NULL, NULL, NULL, level, prio, api)

static int scmi_smc_send_message(const struct device *transport, struct scmi_channel *chan,
				 struct scmi_message *msg)
{
	struct scmi_smc_channel *smc_chan;
	struct arm_smccc_res res;
	int ret;

	smc_chan = chan->data;

	ret = scmi_shmem_write_message(smc_chan->shmem, msg);
	if (ret < 0) {
		LOG_ERR("failed to write message to shmem: %d", ret);
		return ret;
	}

	/* Make SMC call to notify ATF */
	arm_smccc_smc(smc_chan->func_id, 0, 0, 0, 0, 0, 0, 0, &res);

	/* Check SMC result */
	if (res.a0 != 0) {
		LOG_ERR("SMC call failed: 0x%lx", res.a0);
		return -ENOTSUP;
	}

	return 0;
}

static int scmi_smc_read_message(const struct device *transport, struct scmi_channel *chan,
				 struct scmi_message *msg)
{
	struct scmi_smc_channel *smc_chan;

	smc_chan = chan->data;

	return scmi_shmem_read_message(smc_chan->shmem, msg);
}

static bool scmi_smc_channel_is_free(const struct device *transport, struct scmi_channel *chan)
{
	struct scmi_smc_channel *smc_chan = chan->data;

	/* Per SCMI spec: Bit[0]=1 means FREE, Bit[0]=0 means BUSY */
	return scmi_shmem_channel_status(smc_chan->shmem) & SCMI_SHMEM_CHAN_STATUS_BUSY_BIT;
}

static int scmi_smc_setup_chan(const struct device *transport, struct scmi_channel *chan, bool tx)
{
	struct scmi_smc_channel *smc_chan;

	smc_chan = chan->data;

	if (!tx) {
		return -ENOTSUP;
	}

	chan->polling_only = true;

	/*
	 * Disable interrupt flag in shared memory to indicate
	 * polling-only operation to platform.
	 */
	scmi_shmem_update_flags(smc_chan->shmem, SCMI_SHMEM_CHAN_FLAG_IRQ_BIT, 0);

	LOG_DBG("SMC channel setup complete (polling mode)");

	return 0;
}

static struct scmi_transport_api scmi_smc_api = {
	.setup_chan = scmi_smc_setup_chan,
	.send_message = scmi_smc_send_message,
	.read_message = scmi_smc_read_message,
	.channel_is_free = scmi_smc_channel_is_free,
};

DT_INST_SCMI_SMC_DEFINE(0, PRE_KERNEL_1, CONFIG_ARM_SCMI_TRANSPORT_INIT_PRIORITY, &scmi_smc_api);
