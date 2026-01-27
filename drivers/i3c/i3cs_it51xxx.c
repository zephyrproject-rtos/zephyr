/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_i3cs

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>
LOG_MODULE_REGISTER(i3cs_it51xxx);

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/policy.h>

#include <soc_common.h>

#define BYTE_0(x) FIELD_GET(GENMASK(7, 0), x)
#define BYTE_1(x) FIELD_GET(GENMASK(15, 8), x)
#define BYTE_2(x) FIELD_GET(GENMASK(23, 16), x)
#define BYTE_3(x) FIELD_GET(GENMASK(31, 24), x)

/* used for tx/rx fifo base address setting */
#define FIFO_ADDR_LB(x) FIELD_GET(GENMASK(10, 3), x)
#define FIFO_ADDR_HB(x) FIELD_GET(GENMASK(18, 11), x)

#define I3CS05_CONFIG_1 0x05
#define ID_RANDOM       BIT(0)

#define I3CS07_CONFIG_2        0x07
#define I3CS_TARGET_ADDRESS(n) FIELD_PREP(GENMASK(7, 1), n)

#define I3CS08_STATUS_0 0x08
#define BUS_IS_BUSY     BIT(0)

#define I3CS09_STATUS_1     0x09
#define INT_ERROR_WARNING   BIT(7)
#define INT_CCC             BIT(6)
#define INT_DYN_ADDR_CHANGE BIT(5)
#define INT_RX_PENDING      BIT(3)
#define INT_STOP            BIT(2)
#define INT_ADDR_MATCHED    BIT(1)

#define I3CS0A_STATUS_2   0x0A
#define EVENT_DETECT_MASK GENMASK(5, 4)
#define INT_TARGET_RST    BIT(3)
#define INT_EVENT         BIT(2)

#define I3CS0B_STATUS_3 0x0B
#define HJ_DISABLED     BIT(3)
#define IBI_DISABLED    BIT(0)

#define I3CS0C_CONTROL_0     0x0C
#define EXTENDED_IBI_DATA    BIT(3)
#define I3CS_EVENT_SELECT(n) FIELD_PREP(GENMASK(1, 0), n)

#define I3CS0D_CONTROL_1               0x0D
#define I3CS0F_CONTROL_3               0x0F
#define I3CS11_INTERRUPT_ENABLE_CTRL_0 0x11

#define I3CS14_DIRECT_TX_FIFO_BASE_ADDR_LB 0x14
#define I3CS15_DIRECT_TX_FIFO_BASE_ADDR_HB 0x15
#define I3CS16_DIRECT_RX_FIFO_BASE_ADDR_LB 0x16
#define I3CS17_DIRECT_RX_FIFO_BASE_ADDR_HB 0x17
#define I3CS1A_DIRECT_TX_LENGTH_LB         0x1A
#define I3CS1B_DIRECT_TX_LENGTH_HB         0x1B

#define I3CS1C_ERROR_WARNING_REG_0 0x1C
#define INVALID_START              BIT(4)
#define CONTROLLER_TERMINATED      BIT(3)
#define TX_FIFO_UNDERRUN           (BIT(2) | BIT(1))
#define RX_FIFO_OVERRUN            BIT(0)

#define I3CS1D_ERROR_WARNING_REG_1 0x1D
#define S0_OR_S1_ERROR             BIT(3)
#define SDR_PARITY_ERROR           BIT(0)

#define I3CS2C_DATA_CTRL_0 0x2C
#define FLUSH_TX_FIFO      BIT(0)

#define I3CS41_TX_RX_FIFO_BASE_ADDR_HB 0x41
#define I3CS42_TX_FIFO_BASE_ADDR_LB    0x42
#define I3CS43_RX_FIFO_BASE_ADDR_LB    0x43
#define I3CS45_RX_FIFO_READ_PTR        0x45
#define I3CS4A_TX_FIFO_SIZE            0x4A
#define I3CS_TX_FIFO_SIZE_MASK         GENMASK(3, 0)

#define I3CS4D_CONTROL_REG_4             0x4D
#define I3CS_DIRECT_MODE_AUTO_CLR_TX_CNT BIT(6)
#define I3CS_DIRECT_MODE_ENABLE          BIT(5) | BIT(4)

#define I3CS4E_DIRECT_FIFO_STATUS 0x4E
#define I3CS_DIRECT_TX_DONE       BIT(1)
#define I3CS_DIRECT_RX_DONE       BIT(0)

#define I3CS58_TX_FIFO_BYTE_COUNT_LB 0x58
#define I3CS59_TX_FIFO_BYTE_COUNT_HB 0x59
#define I3CS5A_RX_FIFO_BYTE_COUNT_LB 0x5A
#define I3CS5B_RX_FIFO_BYTE_COUNT_HB 0x5B
#define I3CS64_DYNAMIC_ADDRESS       0x64
#define DYNAMIC_ADDRESS(x)           FIELD_GET(GENMASK(7, 1), x)
#define DYNAMIC_ADDRESS_VALID        BIT(0)

#define I3CS68_MRL_SET_BY_CTRL_LB 0x68
#define I3CS69_MRL_SET_BY_CTRL_HB 0x69
#define I3CS6A_MWL_SET_BY_CTRL_LB 0x6A
#define I3CS6B_MWL_SET_BY_CTRL_HB 0x6B
#define I3CS6C_PRAT_NUMBER_0      0x6C
#define I3CS6D_PRAT_NUMBER_1      0x6D
#define I3CS6E_PRAT_NUMBER_2      0x6E
#define I3CS6F_PRAT_NUMBER_3      0x6F
#define I3CS71_DCR                0x71
#define I3CS72_BCR                0x72
#define I3CS76_TX_FIFO_READ_PTR   0x76
#define I3CS7A_RX_FIFO_SIZE       0x7A
#define I3CS_RX_FIFO_SIZE_MASK    GENMASK(3, 0)

#define IBI_MDB_GROUP_MASK              GENMASK(7, 5)
#define IBI_MDB_GROUP_PENDING_READ_NOTI 5

#define IT51XXX_DIRECT_MODE_FIFO_SIZE 4096
#define IT51XXX_I3CS_MAX_MRL_MWL      0xFFF /* 4095 bytes */

enum it51xxx_i3cs_event_type {
	EVT_NORMAL_MODE = 0,
	EVT_IBI,
	EVT_CONTROL_REQUEST,
	EVT_HOT_JOIN,
};

enum it51xxx_i3cs_request_event {
	NONE = 0,
	REQUEST_NOT_SENT,
	REQUEST_NACK_EVT,
	REQUEST_ACK_EVT,
};

static const struct fifo_size_mapping_t {
	uint16_t fifo_size;
	uint8_t value;
} fifo_size_table[5] = {[0] = {.fifo_size = 16, .value = 0x0},
			[1] = {.fifo_size = 32, .value = 0x5},
			[2] = {.fifo_size = 64, .value = 0x6},
			[3] = {.fifo_size = 128, .value = 0x7},
			[4] = {.fifo_size = 4096, .value = 0xC}};

struct it51xxx_i3cs_data {
	struct i3c_driver_data common;
	struct i3c_target_config *target_config;

	/* configuration parameters for I3C hardware to act as target device */
	struct i3c_config_target config_target;

#ifdef CONFIG_I3C_USE_IBI
	struct k_sem ibi_sync_sem;
#endif /* CONFIG_I3C_USE_IBI */

	struct k_mutex lock;

	struct {
		uint8_t tx_data[CONFIG_I3CS_IT51XXX_TX_FIFO_SIZE];
		uint8_t rx_data[CONFIG_I3CS_IT51XXX_RX_FIFO_SIZE];
	} fifo __aligned(IT51XXX_DIRECT_MODE_FIFO_SIZE);
};

struct it51xxx_i3cs_config {
	/* common i3c driver config */
	struct i3c_driver_config common;

	const struct pinctrl_dev_config *pcfg;
	mm_reg_t base;
	uint8_t io_channel;
	uint8_t vendor_info;

	struct {
		mm_reg_t addr;
		uint8_t bit_mask;
	} extern_enable;

	void (*irq_config_func)(const struct device *dev);

	LOG_INSTANCE_PTR_DECLARE(log);
};

static inline bool rx_direct_mode_is_enabled(const struct device *dev)
{
	struct it51xxx_i3cs_data *data = dev->data;

	return sizeof(data->fifo.rx_data) == IT51XXX_DIRECT_MODE_FIFO_SIZE;
}

static inline bool tx_direct_mode_is_enabled(const struct device *dev)
{
	struct it51xxx_i3cs_data *data = dev->data;

	return sizeof(data->fifo.tx_data) == IT51XXX_DIRECT_MODE_FIFO_SIZE;
}

static inline uint16_t rx_byte_cnt_in_fifo(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;

	return (sys_read8(cfg->base + I3CS5B_RX_FIFO_BYTE_COUNT_HB) << 8) +
	       sys_read8(cfg->base + I3CS5A_RX_FIFO_BYTE_COUNT_LB);
}

static inline uint16_t tx_byte_cnt_in_fifo(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;

	return (sys_read8(cfg->base + I3CS59_TX_FIFO_BYTE_COUNT_HB) << 8) +
	       sys_read8(cfg->base + I3CS58_TX_FIFO_BYTE_COUNT_LB);
}

static inline void set_mrl_value(const struct device *dev, const uint16_t value)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	uint16_t mrl;

	mrl = (value > IT51XXX_I3CS_MAX_MRL_MWL) ? IT51XXX_I3CS_MAX_MRL_MWL : value;

	sys_write8(BYTE_0(mrl), cfg->base + I3CS68_MRL_SET_BY_CTRL_LB);
	sys_write8(BYTE_1(mrl), cfg->base + I3CS69_MRL_SET_BY_CTRL_HB);
}

static inline void set_mwl_value(const struct device *dev, const uint16_t value)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	uint16_t mwl;

	mwl = (value > IT51XXX_I3CS_MAX_MRL_MWL) ? IT51XXX_I3CS_MAX_MRL_MWL : value;

	sys_write8(BYTE_0(mwl), cfg->base + I3CS6A_MWL_SET_BY_CTRL_LB);
	sys_write8(BYTE_1(mwl), cfg->base + I3CS6B_MWL_SET_BY_CTRL_HB);
}

static int it51xxx_i3cs_prepare_tx_fifo(const struct device *dev, uint8_t *buf, uint16_t len)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	uint16_t tx_count;

	if (len > sizeof(data->fifo.tx_data)) {
		return -ENOSPC;
	}

	tx_count = tx_byte_cnt_in_fifo(dev);
	if (tx_count) {
		LOG_INST_WRN(cfg->log, "dropped the remaining %d bytes in the tx fifo", tx_count);
	}

	/* flush tx fifo */
	sys_write8(sys_read8(cfg->base + I3CS2C_DATA_CTRL_0) | FLUSH_TX_FIFO,
		   cfg->base + I3CS2C_DATA_CTRL_0);

	/* set tx length */
	if (tx_direct_mode_is_enabled(dev)) {
		sys_write8(BYTE_0(len), cfg->base + I3CS1A_DIRECT_TX_LENGTH_LB);
		sys_write8(BYTE_1(len), cfg->base + I3CS1B_DIRECT_TX_LENGTH_HB);
	} else {
		sys_write8(len, cfg->base + I3CS76_TX_FIFO_READ_PTR);
	}

	/* fill tx fifo with data */
	memcpy(data->fifo.tx_data, buf, len);

	return 0;
}

static int it51xxx_i3cs_target_register(const struct device *dev, struct i3c_target_config *tgt_cfg)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;

	if (!data->target_config) {
		data->target_config = tgt_cfg;

		chip_block_idle();
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	} else {
		LOG_INST_WRN(cfg->log, "the target has already been registered");
	}

	return 0;
}

static int it51xxx_i3cs_target_unregister(const struct device *dev,
					  struct i3c_target_config *tgt_cfg)
{
	ARG_UNUSED(tgt_cfg);

	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;

	if (data->target_config) {
		data->target_config = NULL;

		/* Permit to enter power policy and idle mode. */
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		chip_permit_idle();
	} else {
		LOG_INST_WRN(cfg->log, "the target has not been registered");
	}

	return 0;
}

static int it51xxx_i3cs_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
					uint8_t hdr_mode)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	int ret;

	if (!buf || len == 0) {
		LOG_INST_ERR(cfg->log, "null buffer or zero length");
		return -EINVAL;
	}

	if (hdr_mode != 0) {
		LOG_INST_ERR(cfg->log, "unsupported hdr mode");
		return -ENOTSUP;
	}

	if (len > sizeof(data->fifo.tx_data)) {
		LOG_INST_ERR(cfg->log, "invalid tx length(%d)", len);
		return -ENOSPC;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = it51xxx_i3cs_prepare_tx_fifo(dev, buf, len);
	k_mutex_unlock(&data->lock);

	return ret ? ret : len;
}

static inline bool it51xxx_i3cs_dynamic_addr_valid(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;

	return ((sys_read8(cfg->base + I3CS64_DYNAMIC_ADDRESS) & DYNAMIC_ADDRESS_VALID) ==
		DYNAMIC_ADDRESS_VALID);
}

#ifdef CONFIG_I3C_USE_IBI
static inline bool it51xxx_i3cs_is_ibi_disable(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;

	return ((sys_read8(cfg->base + I3CS0B_STATUS_3) & IBI_DISABLED) == IBI_DISABLED);
}

static inline bool it51xxx_i3cs_is_hj_disable(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;

	return ((sys_read8(cfg->base + I3CS0B_STATUS_3) & HJ_DISABLED) == HJ_DISABLED);
}

static int it51xxx_i3cs_wait_to_complete(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;

	if (k_sem_take(&data->ibi_sync_sem, K_MSEC(CONFIG_I3CS_IT51XXX_IBI_TIMEOUT_MS)) != 0) {
		LOG_INST_ERR(cfg->log, "ibi event transmission timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

static int it51xxx_i3cs_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	struct i3c_config_target *config_target = &data->config_target;
	int ret = 0;
	uint8_t reg_val;

	if (!request) {
		LOG_INST_ERR(cfg->log, "ibi request is null");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	reg_val = sys_read8(cfg->base + I3CS08_STATUS_0);
	if (reg_val & BUS_IS_BUSY) {
		LOG_INST_ERR(cfg->log, "bus is busy");
		ret = -EBUSY;
		goto out;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		if (it51xxx_i3cs_is_ibi_disable(dev) || !it51xxx_i3cs_dynamic_addr_valid(dev)) {
			LOG_INST_ERR(cfg->log, "ibi is disabled or dynamic address is invalid");
			ret = -EINVAL;
			goto out;
		}
		if (request->payload_len > sizeof(data->fifo.tx_data) + 1) {
			LOG_INST_ERR(cfg->log, "payload too large for ibi tir");
			ret = -ENOMEM;
			goto out;
		}
		if (config_target->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE &&
		    request->payload_len == 0) {
			LOG_INST_ERR(cfg->log, "ibi should be with payload");
			ret = -EINVAL;
			goto out;
		}
		if (!(config_target->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) &&
		    request->payload_len != 0) {
			LOG_INST_ERR(cfg->log, "ibi should not be with payload");
			ret = -EINVAL;
			goto out;
		}

		if (request->payload_len == 0) {
			LOG_INST_DBG(cfg->log, "send ibi without payload");
			sys_write8(I3CS_EVENT_SELECT(EVT_IBI), cfg->base + I3CS0C_CONTROL_0);
		} else {
			/* set mandatory data byte */
			sys_write8(request->payload[0], cfg->base + I3CS0D_CONTROL_1);

			if (request->payload_len > 1) {
				if (FIELD_GET(IBI_MDB_GROUP_MASK, request->payload[0]) ==
				    IBI_MDB_GROUP_PENDING_READ_NOTI) {
					/* since the fifo for ibi payload and pending data is
					 * shared, the i3cs controller cannot issue an ibi with
					 * pending data notification if the ibi payload size
					 * exceeds 1.
					 */
					LOG_INST_ERR(cfg->log, "unsupported multiple payloads with "
							       "pending read noti. group");
					ret = -ENOTSUP;
					goto out;
				}

				ret = it51xxx_i3cs_prepare_tx_fifo(dev, &request->payload[1],
								   request->payload_len - 1);
				if (ret) {
					goto out;
				}

				sys_write8(EXTENDED_IBI_DATA | I3CS_EVENT_SELECT(EVT_IBI),
					   cfg->base + I3CS0C_CONTROL_0);
			} else {
				sys_write8(I3CS_EVENT_SELECT(EVT_IBI),
					   cfg->base + I3CS0C_CONTROL_0);
			}
		}
		break;
	case I3C_IBI_HOTJOIN:
		if (it51xxx_i3cs_is_hj_disable(dev) || it51xxx_i3cs_dynamic_addr_valid(dev)) {
			LOG_INST_ERR(cfg->log,
				     "hj is disabled or dynamic address is already assigned");
			ret = -EINVAL;
			goto out;
		}
		sys_write8(I3CS_EVENT_SELECT(EVT_HOT_JOIN), cfg->base + I3CS0C_CONTROL_0);
		break;
	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		LOG_INST_ERR(cfg->log, "unsupported controller role request");
		ret = -ENOTSUP;
		goto out;
	default:
		LOG_INST_ERR(cfg->log, "invalid ibi type(0x%x)", request->ibi_type);
		ret = -EINVAL;
		goto out;
	}

	ret = it51xxx_i3cs_wait_to_complete(dev);
	if (ret) {
		LOG_INST_WRN(cfg->log, "failed to issue ibi. maybe the controller is offline");
		sys_write8(I3CS_EVENT_SELECT(EVT_NORMAL_MODE), cfg->base + I3CS0C_CONTROL_0);
	}
out:
	k_mutex_unlock(&data->lock);

	return ret;
}
#endif /* CONFIG_I3C_USE_IBI */

static int it51xxx_i3cs_set_fifo_address(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;

	if (sizeof(data->fifo.rx_data) <= 128 && sizeof(data->fifo.tx_data) <= 128) {
		if (FIFO_ADDR_HB(((uint32_t)&data->fifo.tx_data)) !=
		    FIFO_ADDR_HB(((uint32_t)&data->fifo.rx_data))) {
			LOG_INST_ERR(cfg->log,
				     "the msb of tx and rx fifo address should be the same");
			return -EINVAL;
		}
		sys_write8(FIFO_ADDR_LB((uint32_t)&data->fifo.rx_data),
			   cfg->base + I3CS43_RX_FIFO_BASE_ADDR_LB);
		sys_write8(FIFO_ADDR_LB((uint32_t)&data->fifo.tx_data),
			   cfg->base + I3CS42_TX_FIFO_BASE_ADDR_LB);
		sys_write8(FIFO_ADDR_HB((uint32_t)&data->fifo.tx_data),
			   cfg->base + I3CS41_TX_RX_FIFO_BASE_ADDR_HB);
		return 0;
	}

	if (!rx_direct_mode_is_enabled(dev) || !tx_direct_mode_is_enabled(dev)) {
		/* The tx and rx direct modes must be enabled simultaneously. */
		LOG_INST_ERR(cfg->log, "tx or rx fifo size is invalid for direct mode");
		return -EINVAL;
	}

	LOG_INST_DBG(cfg->log, "direct mode is enabled");
	sys_write8(sys_read8(cfg->base + I3CS4D_CONTROL_REG_4) | I3CS_DIRECT_MODE_ENABLE,
		   cfg->base + I3CS4D_CONTROL_REG_4);
	sys_write8(FIFO_ADDR_LB((uint32_t)&data->fifo.rx_data),
		   cfg->base + I3CS16_DIRECT_RX_FIFO_BASE_ADDR_LB);
	sys_write8(FIFO_ADDR_HB((uint32_t)&data->fifo.rx_data),
		   cfg->base + I3CS17_DIRECT_RX_FIFO_BASE_ADDR_HB);
	sys_write8(FIFO_ADDR_LB((uint32_t)&data->fifo.tx_data),
		   cfg->base + I3CS14_DIRECT_TX_FIFO_BASE_ADDR_LB);
	sys_write8(FIFO_ADDR_HB((uint32_t)&data->fifo.tx_data),
		   cfg->base + I3CS15_DIRECT_TX_FIFO_BASE_ADDR_HB);

	return 0;
}

static int it51xxx_i3cs_init(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	struct i3c_config_target *config_target = &data->config_target;
	uint8_t reg_val;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_INST_ERR(cfg->log, "failed to apply pinctrl, ret %d", ret);
		return ret;
	}

	/* set i3cs channel selection */
	sys_write8(cfg->io_channel, cfg->base + I3CS4D_CONTROL_REG_4);
	LOG_INST_DBG(cfg->log, "select io channel %d", cfg->io_channel);

	/* set extern enable bit */
	if (cfg->extern_enable.bit_mask > 7) {
		LOG_INST_ERR(cfg->log, "invalid bit mask %d for extern enable setting",
			     cfg->extern_enable.bit_mask);
		return -EINVAL;
	}
	sys_write8(sys_read8(cfg->extern_enable.addr) | BIT(cfg->extern_enable.bit_mask),
		   cfg->extern_enable.addr);

	/* set static address */
	sys_write8(I3CS_TARGET_ADDRESS(config_target->static_addr), cfg->base + I3CS07_CONFIG_2);

	/* set msb(vendor info) of get device status ccc */
	sys_write8(cfg->vendor_info, cfg->base + I3CS0F_CONTROL_3);

	/* set pid, bcr and dcr */
	if (config_target->pid_random) {
		sys_write8(sys_read8(cfg->base + I3CS05_CONFIG_1) | ID_RANDOM,
			   cfg->base + I3CS05_CONFIG_1);
		sys_write8(BYTE_0(config_target->pid), cfg->base + I3CS6C_PRAT_NUMBER_0);
		sys_write8(BYTE_1(config_target->pid), cfg->base + I3CS6D_PRAT_NUMBER_1);
		sys_write8(BYTE_2(config_target->pid), cfg->base + I3CS6E_PRAT_NUMBER_2);
		sys_write8(BYTE_3(config_target->pid), cfg->base + I3CS6F_PRAT_NUMBER_3);
		LOG_INST_INF(cfg->log, "set pid random value: %#llx", config_target->pid);
	}
	if (I3C_BCR_DEVICE_ROLE(config_target->bcr) == I3C_BCR_DEVICE_ROLE_I3C_CONTROLLER_CAPABLE) {
		LOG_INST_ERR(cfg->log, "i3cs doesn't support controller capability");
		return -ENOTSUP;
	}
	sys_write8(config_target->bcr, cfg->base + I3CS72_BCR);
	sys_write8(config_target->dcr, cfg->base + I3CS71_DCR);

	LOG_INST_INF(cfg->log, "tx fifo size(%d), address(0x%x)", sizeof(data->fifo.tx_data),
		     (uint32_t)&data->fifo.tx_data);
	LOG_INST_INF(cfg->log, "rx fifo size(%d), address(0x%x)", sizeof(data->fifo.rx_data),
		     (uint32_t)&data->fifo.rx_data);

	/* set tx and rx fifo size */
	for (uint8_t i = 0; i <= ARRAY_SIZE(fifo_size_table); i++) {
		if (i == ARRAY_SIZE(fifo_size_table)) {
			LOG_INST_ERR(cfg->log, "unknown rx fifo size %d",
				     sizeof(data->fifo.rx_data));
			return -ENOTSUP;
		}
		if (sizeof(data->fifo.rx_data) == fifo_size_table[i].fifo_size) {
			sys_write8(FIELD_PREP(I3CS_RX_FIFO_SIZE_MASK, fifo_size_table[i].value),
				   cfg->base + I3CS7A_RX_FIFO_SIZE);
			set_mwl_value(dev, sizeof(data->fifo.rx_data));
			break;
		}
	}
	for (uint8_t i = 0; i <= ARRAY_SIZE(fifo_size_table); i++) {
		if (i == ARRAY_SIZE(fifo_size_table)) {
			LOG_INST_ERR(cfg->log, "unknown tx fifo size %d",
				     sizeof(data->fifo.tx_data));
			return -ENOTSUP;
		}
		if (sizeof(data->fifo.tx_data) == fifo_size_table[i].fifo_size) {
			sys_write8(FIELD_PREP(I3CS_TX_FIFO_SIZE_MASK, fifo_size_table[i].value),
				   cfg->base + I3CS4A_TX_FIFO_SIZE);
			set_mrl_value(dev, sizeof(data->fifo.tx_data));
			break;
		}
	}

	ret = it51xxx_i3cs_set_fifo_address(dev);
	if (ret) {
		return ret;
	}

	if (tx_direct_mode_is_enabled(dev)) {
		sys_write8(sys_read8(cfg->base + I3CS4D_CONTROL_REG_4) |
				   I3CS_DIRECT_MODE_AUTO_CLR_TX_CNT,
			   cfg->base + I3CS4D_CONTROL_REG_4);
	}

#ifdef CONFIG_I3C_USE_IBI
	k_sem_init(&data->ibi_sync_sem, 0, 1);
#endif /*CONFIG_I3C_USE_IBI */

	k_mutex_init(&data->lock);

	/* clear interrupt/errwarn status and enable interrupt */
	sys_write8(sys_read8(cfg->base + I3CS1C_ERROR_WARNING_REG_0),
		   cfg->base + I3CS1C_ERROR_WARNING_REG_0);
	sys_write8(sys_read8(cfg->base + I3CS1D_ERROR_WARNING_REG_1),
		   cfg->base + I3CS1D_ERROR_WARNING_REG_1);
	sys_write8(sys_read8(cfg->base + I3CS09_STATUS_1), cfg->base + I3CS09_STATUS_1);
	reg_val = INT_STOP | INT_ERROR_WARNING;
	sys_write8(reg_val, cfg->base + I3CS11_INTERRUPT_ENABLE_CTRL_0);

	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(i3c, it51xxx_i3cs_api) = {
	.target_tx_write = it51xxx_i3cs_target_tx_write,
	.target_register = it51xxx_i3cs_target_register,
	.target_unregister = it51xxx_i3cs_target_unregister,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_raise = it51xxx_i3cs_target_ibi_raise,
#endif /* CONFIG_I3C_USE_IBI */
};

static void it51xxx_i3cs_check_errwarn(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	uint8_t errwarn0_val, errwarn1_val;

	errwarn0_val = sys_read8(cfg->base + I3CS1C_ERROR_WARNING_REG_0);
	errwarn1_val = sys_read8(cfg->base + I3CS1D_ERROR_WARNING_REG_1);
	if (errwarn0_val & INVALID_START) {
		LOG_INST_ERR(cfg->log, "isr: invalid start");
	}
	if (errwarn0_val & CONTROLLER_TERMINATED) {
		LOG_INST_WRN(cfg->log,
			     "isr: terminated by controller, flush the remaining %d bytes",
			     tx_byte_cnt_in_fifo(dev));
		sys_write8(sys_read8(cfg->base + I3CS2C_DATA_CTRL_0) | FLUSH_TX_FIFO,
			   cfg->base + I3CS2C_DATA_CTRL_0);
	}
	if (errwarn0_val & TX_FIFO_UNDERRUN) {
		LOG_INST_ERR(cfg->log, "isr: the tx fifo is underrun");
	}
	if (errwarn0_val & RX_FIFO_OVERRUN) {
		LOG_INST_ERR(cfg->log, "isr: the rx fifo is overrun");
	}
	if (errwarn1_val & S0_OR_S1_ERROR) {
		LOG_INST_ERR(cfg->log, "isr: s0 or s1 error is detected");
	}
	if (errwarn1_val & SDR_PARITY_ERROR) {
		LOG_INST_ERR(cfg->log, "isr: sdr parity error");
	}
	LOG_INST_DBG(cfg->log, "isr: error/warning is detected(0x%x, 0x%x)", errwarn0_val,
		     errwarn1_val);

	/* write 1 to clear the error and warning registers */
	sys_write8(errwarn0_val, cfg->base + I3CS1C_ERROR_WARNING_REG_0);
	sys_write8(errwarn1_val, cfg->base + I3CS1D_ERROR_WARNING_REG_1);
}

static inline void invoke_rx_cb(const struct device *dev, const bool ccc, uint8_t *buf,
				const size_t buf_len)
{
	struct it51xxx_i3cs_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb =
		data->target_config ? data->target_config->callbacks : NULL;

	if (!ccc) {
		LOG_HEXDUMP_DBG(buf, buf_len, "isr: rx:");
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		if (target_cb && target_cb->buf_write_received_cb) {
			target_cb->buf_write_received_cb(data->target_config, buf, buf_len);
		}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
	} else {
		LOG_HEXDUMP_WRN(buf, buf_len, "isr: unhandled ccc:");
	}
}

static void it51xxx_i3cs_process_rx_fifo(const struct device *dev, const bool ccc)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	uint16_t byte_count = rx_byte_cnt_in_fifo(dev);

	if (rx_direct_mode_is_enabled(dev)) {
		uint8_t dfifo_status = sys_read8(cfg->base + I3CS4E_DIRECT_FIFO_STATUS);

		if (dfifo_status & I3CS_DIRECT_RX_DONE) {
			sys_write8(I3CS_DIRECT_RX_DONE, cfg->base + I3CS4E_DIRECT_FIFO_STATUS);
		} else {
			LOG_INST_WRN(cfg->log, "isr: rx pending, but rx not completed");
			return;
		}
		invoke_rx_cb(dev, ccc, data->fifo.rx_data, byte_count);
	} else {
		uint8_t read_ptr, idx;
		uint8_t rx_buf[sizeof(data->fifo.rx_data)];
		const size_t rx_fifo_sz = sizeof(data->fifo.rx_data);

		read_ptr = sys_read8(cfg->base + I3CS45_RX_FIFO_READ_PTR);
		idx = read_ptr % rx_fifo_sz;
		for (size_t i = 0; i < byte_count; i++) {
			rx_buf[i] =
				data->fifo.rx_data[(idx + i) >= rx_fifo_sz ? (idx + i) - rx_fifo_sz
									   : (idx + i)];
		}
		sys_write8(read_ptr + byte_count, cfg->base + I3CS45_RX_FIFO_READ_PTR);
		invoke_rx_cb(dev, ccc, rx_buf, byte_count);
	}
}

static void it51xxx_i3cs_process_tx_fifo(const struct device *dev, const bool ccc)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb =
		data->target_config ? data->target_config->callbacks : NULL;

	if (tx_direct_mode_is_enabled(dev)) {
		uint8_t dfifo_status = sys_read8(cfg->base + I3CS4E_DIRECT_FIFO_STATUS);

		if (dfifo_status & I3CS_DIRECT_TX_DONE) {
			sys_write8(I3CS_DIRECT_TX_DONE, cfg->base + I3CS4E_DIRECT_FIFO_STATUS);
		} else {
			return;
		}
	}

	if (!ccc) {
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		if (target_cb && target_cb->buf_read_requested_cb) {
			target_cb->buf_read_requested_cb(data->target_config, NULL, NULL, NULL);
		}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
	}
}

static void it51xxx_i3cs_isr(const struct device *dev)
{
	const struct it51xxx_i3cs_config *cfg = dev->config;
	struct it51xxx_i3cs_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb =
		data->target_config ? data->target_config->callbacks : NULL;
	uint8_t int_status_1, int_status_2;

	int_status_1 = sys_read8(cfg->base + I3CS09_STATUS_1);
	int_status_2 = sys_read8(cfg->base + I3CS0A_STATUS_2);
	LOG_INST_DBG(cfg->log, "isr: interrupt status 0x%x 0x%x", int_status_1, int_status_2);

	if (int_status_1 & INT_DYN_ADDR_CHANGE) {
		if (it51xxx_i3cs_dynamic_addr_valid(dev)) {
			if (data->target_config) {
				data->target_config->address = DYNAMIC_ADDRESS(
					sys_read8(cfg->base + I3CS64_DYNAMIC_ADDRESS));
			}
			LOG_INST_DBG(cfg->log, "dynamic address is assigned");
		} else {
			if (data->target_config) {
				data->target_config->address = 0;
			}
			LOG_INST_DBG(cfg->log, "dynamic address is reset");
		}
	}

	if (int_status_1 & INT_ERROR_WARNING) {
		it51xxx_i3cs_check_errwarn(dev);
	}

	if (int_status_1 & INT_STOP) {
		bool is_unhandled_ccc =
			(!(int_status_1 & INT_ADDR_MATCHED) || (int_status_1 & INT_CCC));

		if (int_status_1 & INT_RX_PENDING) {
			it51xxx_i3cs_process_rx_fifo(dev, is_unhandled_ccc);
		} else {
			it51xxx_i3cs_process_tx_fifo(dev, is_unhandled_ccc);
		}

		if (!is_unhandled_ccc) {
			if (target_cb != NULL && target_cb->stop_cb) {
				target_cb->stop_cb(data->target_config);
			}
		}
	}

	switch (int_status_2 & EVENT_DETECT_MASK) {
	case FIELD_PREP(EVENT_DETECT_MASK, REQUEST_NACK_EVT):
		LOG_INST_ERR(cfg->log, "isr: nack is detected");
		break;
	case FIELD_PREP(EVENT_DETECT_MASK, REQUEST_NOT_SENT):
		LOG_INST_ERR(cfg->log, "isr: request is not sent yet");
		break;
	case FIELD_PREP(EVENT_DETECT_MASK, REQUEST_ACK_EVT):
		if (int_status_2 & INT_EVENT) {
			LOG_INST_DBG(cfg->log, "isr: tir/hj is completed");
		}
		break;
	default:
		break;
	};

	if (int_status_2 & EVENT_DETECT_MASK) {
#ifdef CONFIG_I3C_USE_IBI
		k_sem_give(&data->ibi_sync_sem);
#endif /* CONFIG_I3C_USE_IBI */
	}

	if (int_status_2 & INT_TARGET_RST) {
		LOG_INST_INF(cfg->log, "isr: target reset pattern is detected");
	}

	sys_write8(int_status_1, cfg->base + I3CS09_STATUS_1);
	sys_write8(int_status_2, cfg->base + I3CS0A_STATUS_2);
}

#define IT51XXX_I3CS_EXTERN_ENABLE(n)                                                              \
	{                                                                                          \
		.addr = DT_INST_PROP_BY_IDX(n, extern_enable, 0),                                  \
		.bit_mask = DT_INST_PROP_BY_IDX(n, extern_enable, 1),                              \
	}

#define IT51XXX_I3CS_INIT(n)                                                                       \
	LOG_INSTANCE_REGISTER(DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(n)), n,                          \
			      CONFIG_I3C_IT51XXX_LOG_LEVEL);                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void it51xxx_i3cs_config_func_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, it51xxx_i3cs_isr, DEVICE_DT_INST_GET(n), 0);       \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static const struct it51xxx_i3cs_config i3c_config_##n = {                                 \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = it51xxx_i3cs_config_func_##n,                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.io_channel = DT_INST_PROP(n, io_channel),                                         \
		.extern_enable = IT51XXX_I3CS_EXTERN_ENABLE(n),                                    \
		.vendor_info = DT_INST_PROP_OR(n, vendor_info_fields, 0x0),                        \
		LOG_INSTANCE_PTR_INIT(log, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(n)), n)};           \
	static struct it51xxx_i3cs_data i3c_data_##n = {                                           \
		.config_target.static_addr = DT_INST_PROP_OR(n, static_address, 0),                \
		.config_target.pid = DT_INST_PROP_OR(n, pid_random_value, 0),                      \
		.config_target.pid_random = DT_INST_NODE_HAS_PROP(n, pid_random_value),            \
		.config_target.bcr = DT_INST_PROP_OR(n, bcr, 0x0F),                                \
		.config_target.dcr = DT_INST_PROP_OR(n, dcr, 0),                                   \
		.config_target.supported_hdr = false,                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, it51xxx_i3cs_init, NULL, &i3c_data_##n, &i3c_config_##n,          \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,                    \
			      &it51xxx_i3cs_api);

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_I3CS_INIT)
