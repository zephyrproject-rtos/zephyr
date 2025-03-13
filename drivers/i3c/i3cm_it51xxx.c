/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_i3cm

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3cm_it51xxx, CONFIG_I3C_IT51XXX_LOG_LEVEL);

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/policy.h>

#include <soc_common.h>

#define LOG_DEV_ERR(dev, format, ...) LOG_ERR("%s:" #format, (dev)->name, ##__VA_ARGS__)
#define LOG_DEV_WRN(dev, format, ...) LOG_WRN("%s:" #format, (dev)->name, ##__VA_ARGS__)
#define LOG_DEV_INF(dev, format, ...) LOG_INF("%s:" #format, (dev)->name, ##__VA_ARGS__)
#define LOG_DEV_DBG(dev, format, ...) LOG_DBG("%s:" #format, (dev)->name, ##__VA_ARGS__)

#define BYTE_0(x) FIELD_GET(GENMASK(7, 0), x)
#define BYTE_1(x) FIELD_GET(GENMASK(15, 8), x)

/* it51xxx i3cm registers definition */
#define I3CM00_CYCLE_TYPE                0x00
#define MORE_I3CM_TRANSFER               BIT(6)
#define I3CM_PRIV_TRANS_WITHOUT_7EH_ADDR BIT(5)
#define I3CM_CYCLE_TYPE_SELECT(n)        FIELD_PREP(GENMASK(3, 0), n)

#define I3CM01_STATUS  0x01
#define START_TRANSFER BIT(7)
#define PARITY_ERROR   BIT(5)
#define CRC5_ERROR     BIT(4)
#define IBI_INTERRUPT  BIT(3)
#define TARGET_NACK    BIT(2)
#define TRANSFER_END   BIT(1)
#define NEXT_TRANSFER  BIT(0)

#define I3CM02_TARGET_ADDRESS  0x02
#define I3CM_TARGET_ADDRESS(n) FIELD_PREP(GENMASK(7, 1), n)
#define I3CM_TARGET_RW_FIELD   BIT(0)

#define I3CM03_COMMON_COMMAND_CODE 0x03
#define I3CM04_WRITE_LENGTH_LB     0x04
#define I3CM05_WRITE_LENGTH_HB     0x05
#define I3CM06_READ_LENGTH_LB      0x06
#define I3CM07_READ_LENGTH_HB      0x07
#define I3CM0E_DATA_COUNT_LB       0x0E
#define I3CM0F_IBI_ADDRESS         0x0F
#define I3CM_IBI_ADDR_MASK         GENMASK(7, 1)
#define I3CM_IBI_RNW               BIT(0)

#define I3CM10_CONTROL        0x10
#define I3CM_INTERRUPT_ENABLE BIT(7)
#define I3CM_REFUSE_IBI       BIT(0)

#define I3CM15_CONTROL_2            0x15
#define I3CM_CCC_WITH_DEFINING_BYTE BIT(0)

#define I3CM16_CCC_DEFINING_BYTE    0x16
#define I3CM1E_DATA_COUNT_HB        0x1E
#define I3CM20_TCAS                 0x20 /* clock after start condition */
#define I3CM21_TCBP                 0x21 /* clock before stop condition */
#define I3CM22_TCBSR                0x22 /* clock before repeated start condition */
#define I3CM23_TCASR                0x23 /* clock after repeated start condition */
#define I3CM24_THDDAT_LB            0x24 /* low byte of data hold time */
#define I3CM26_TLOW_LB              0x26 /* low byte of scl clock low period */
#define I3CM27_TLOW_HB              0x27 /* high byte of scl clock low period */
#define I3CM28_THIGH_LB             0x28 /* low byte of scl clock high period */
#define I3CM29_THIGH_HB             0x29 /* high byte of scl clock high period */
#define I3CM2A_TLOW_OD_LB           0x2A /* low byte of open drain scl clock low period */
#define I3CM2B_TLOW_OD_HB           0x2B /* high byte of open drain scl clock low period */
#define I3CM2F_I2C_CONTROL          0x2F
#define I3CM_USE_I2C_TIMING_SETTING BIT(1)

#define I3CM50_CONTROL_3         0x50
#define I3CM_DLM_SIZE_MASK       GENMASK(5, 4)
#define I3CM_CHANNEL_SELECT_MASK GENMASK(3, 2)
#define I3CM_PULL_UP_RESISTOR    BIT(1)
#define I3CM_ENABLE              BIT(0)

#define I3CM52_DLM_BASE_ADDRESS_LB 0x52 /* dlm base address[15:8] */
#define I3CM53_DLM_BASE_ADDRESS_HB 0x53 /* dlm base address[17:16] */

#define IT51XXX_I3C_TRANSFER_TIMEOUT K_MSEC(1000)
#define I3C_IBI_HJ_ADDR              0x02

#define I3C_BUS_TLOW_PP_MIN_NS  24  /* T_LOW period in push-pull mode */
#define I3C_BUS_THIGH_PP_MIN_NS 24  /* T_HIGH period in push-pull mode */
#define I3C_BUS_TLOW_OD_MIN_NS  200 /* T_LOW period in open-drain mode */

enum it51xxx_cycle_types {
	PRIVATE_WRITE_TRANSFER = 0,
	PRIVATE_READ_TRANSFER,
	PRIVATE_WRITE_READ_TRANSFER,
	LEGACE_I2C_WRITE_TRANSFER,
	LEGACE_I2C_READ_TRANSFER,
	LEGACE_I2C_WRITE_READ_TRANSFER,
	BROADCAST_CCC_WRITE_TRANSFER,
	DIRECT_CCC_WRITE_TRANSFER,
	DIRECT_CCC_READ_TRANSFER,
	DAA_TRANSFER,
	IBI_READ_TRANSFER,
	HDR_TRANSFER,
};

enum it51xxx_message_state {
	IT51XXX_I3CM_MSG_BROADCAST_CCC = 0,
	IT51XXX_I3CM_MSG_DIRECT_CCC,
	IT51XXX_I3CM_MSG_DAA,
	IT51XXX_I3CM_MSG_PRIVATE_XFER,
	IT51XXX_I3CM_MSG_IBI,
	IT51XXX_I3CM_MSG_IDLE,
	IT51XXX_I3CM_MSG_ERROR,
};

struct it51xxx_i3cm_data {
	/* common i3c driver data */
	struct i3c_driver_data common;

	enum it51xxx_message_state msg_state;

	struct {
		struct i3c_ccc_payload *ccc;
		size_t target_idx;
	} direct_msgs;

	struct {
		uint8_t target_addr;

		uint8_t num_msgs;
		uint8_t curr_idx;
		struct i3c_msg *msg;
	} i3c_msgs;

#ifdef CONFIG_I3C_USE_IBI
	bool ibi_hj_response;
	uint8_t ibi_target_addr;
	struct k_sem ibi_lock_sem;

	struct {
		uint8_t addr[4];
		uint8_t num_addr;
	} ibi;
#endif /* CONFIG_I3C_USE_IBI */

	struct k_sem msg_sem;
	struct k_mutex lock;

	bool is_initialized;

	struct {
		uint8_t tx_data[CONFIG_I3CM_IT51XXX_DLM_SIZE / 2];
		uint8_t rx_data[CONFIG_I3CM_IT51XXX_DLM_SIZE / 2];
	} dlm_data __aligned(CONFIG_I3CM_IT51XXX_DLM_SIZE);
};

struct it51xxx_i3cm_config {
	/* common i3c driver config */
	struct i3c_driver_config common;

	const struct pinctrl_dev_config *pcfg;
	mm_reg_t base;
	const uint8_t io_channel;

	struct {
		uint8_t i3c_pp_duty_cycle;
		uint32_t i3c_od_scl_hz;
		uint32_t i3c_scl_hddat;
		uint32_t i3c_scl_tcas;
		uint32_t i3c_scl_tcbs;
		uint32_t i3c_scl_tcasr;
		uint32_t i3c_scl_tcbsr;
	} clocks;

	void (*irq_config_func)(const struct device *dev);
};

static void it51xxx_enable_standby_state(const struct device *dev, const bool enable)
{
	ARG_UNUSED(dev);

	if (enable) {
		chip_permit_idle();
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	} else {
		chip_block_idle();
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
}

static inline int it51xxx_set_tx_rx_length(const struct device *dev, const size_t tx_len,
					   const size_t rx_len)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;

	if (tx_len > (CONFIG_I3CM_IT51XXX_DLM_SIZE / 2) ||
	    rx_len > (CONFIG_I3CM_IT51XXX_DLM_SIZE / 2)) {
		LOG_DEV_ERR(dev, "invalid tx(%d) or rx(%d) length", tx_len, rx_len);
		return -EINVAL;
	}

	sys_write8(BYTE_0(rx_len), cfg->base + I3CM06_READ_LENGTH_LB);
	sys_write8(BYTE_1(rx_len), cfg->base + I3CM07_READ_LENGTH_HB);
	sys_write8(BYTE_0(tx_len), cfg->base + I3CM04_WRITE_LENGTH_LB);
	sys_write8(BYTE_1(tx_len), cfg->base + I3CM05_WRITE_LENGTH_HB);
	return 0;
}

static inline size_t it51xxx_get_received_data_count(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;

	return sys_read8(cfg->base + I3CM0E_DATA_COUNT_LB) +
	       ((sys_read8(cfg->base + I3CM1E_DATA_COUNT_HB) & 0x03) << 8);
}

static inline void it51xxx_set_target_address(const struct device *dev, const uint8_t addr,
					      const bool read)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	uint8_t reg_val = 0x0;

	reg_val = I3CM_TARGET_ADDRESS(addr);
	if (read) {
		reg_val |= I3CM_TARGET_RW_FIELD;
	}
	sys_write8(reg_val, cfg->base + I3CM02_TARGET_ADDRESS);
}

static void it51xxx_set_op_type(const struct device *dev, const uint8_t cycle_type,
				const bool more_transfer, const bool broadcast_address)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	uint8_t reg_val = 0x0;

	if (more_transfer) {
		reg_val |= MORE_I3CM_TRANSFER;
	}
	if (!broadcast_address) {
		reg_val |= I3CM_PRIV_TRANS_WITHOUT_7EH_ADDR;
	}
	reg_val |= I3CM_CYCLE_TYPE_SELECT(cycle_type);
	sys_write8(reg_val, cfg->base + I3CM00_CYCLE_TYPE);
	LOG_DEV_DBG(dev, "set cycle type(%d) %s broadcast address %s", cycle_type,
		    broadcast_address ? "with" : "without",
		    more_transfer ? "and more transfer flag" : "");
}

static int it51xxx_wait_to_complete(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;

	if (k_sem_take(&data->msg_sem, IT51XXX_I3C_TRANSFER_TIMEOUT) != 0) {
		LOG_DEV_ERR(dev, "timeout: message status(%d)", data->msg_state);
		return -ETIMEDOUT;
	}

	if (data->msg_state == IT51XXX_I3CM_MSG_ERROR) {
		LOG_DEV_ERR(dev, "message status error");
		return -EIO;
	}

	if (data->msg_state != IT51XXX_I3CM_MSG_IDLE) {
		LOG_DEV_WRN(dev, "message status(%d) is not idle", data->msg_state);
	}
	return 0;
}

static int it51xxx_start_i3c_private_xfer(const struct device *dev, const uint8_t cycle_type,
					  const uint8_t dynamic_addr, const bool more_transfer,
					  const bool broadcast_address)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *msgs = data->i3c_msgs.msg;
	size_t tx_length = 0, rx_length = 0;
	int ret;

	switch (cycle_type) {
	case PRIVATE_WRITE_TRANSFER:
		rx_length = 0;
		tx_length = msgs[data->i3c_msgs.curr_idx].len;
		break;
	case PRIVATE_READ_TRANSFER:
		rx_length = msgs[data->i3c_msgs.curr_idx].len;
		tx_length = 0;
		break;
	case PRIVATE_WRITE_READ_TRANSFER:
		tx_length = msgs[data->i3c_msgs.curr_idx].len;
		rx_length = msgs[data->i3c_msgs.curr_idx + 1].len;
		break;
	case LEGACE_I2C_WRITE_TRANSFER:
		__fallthrough;
	case LEGACE_I2C_READ_TRANSFER:
		__fallthrough;
	case LEGACE_I2C_WRITE_READ_TRANSFER:
		__fallthrough;
	default:
		LOG_DEV_ERR(dev, "unsupported cycle type(0x%x)", cycle_type);
		return -ENOTSUP;
	}

	ret = it51xxx_set_tx_rx_length(dev, tx_length, rx_length);
	if (ret) {
		return ret;
	}

	if (tx_length) {
		memcpy(data->dlm_data.tx_data, msgs[data->i3c_msgs.curr_idx].buf, tx_length);
	}

	it51xxx_set_target_address(dev, dynamic_addr, false);

	/* set cycle type register */
	it51xxx_set_op_type(dev, cycle_type, more_transfer, broadcast_address);
	data->msg_state = IT51XXX_I3CM_MSG_PRIVATE_XFER;
	return 0;
}

static int it51xxx_i3cm_init_clock(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_config_controller *config_cntlr = &data->common.ctrl_config;
	uint32_t pp_freq, od_freq;
	uint32_t odlow_ns, odhigh_ns, pplow_ns, pphigh_ns;
	uint16_t pphigh, pplow, odhigh, odlow;
	uint8_t pp_duty_cycle =
		(cfg->clocks.i3c_pp_duty_cycle > 100) ? 100 : cfg->clocks.i3c_pp_duty_cycle;
	uint8_t hddat = (cfg->clocks.i3c_scl_hddat > 63) ? 63 : cfg->clocks.i3c_scl_hddat;
	uint8_t tcas = (cfg->clocks.i3c_scl_tcas > 0xff) ? 0xff : cfg->clocks.i3c_scl_tcas;
	uint8_t tcbs = (cfg->clocks.i3c_scl_tcbs > 0xff) ? 0xff : cfg->clocks.i3c_scl_tcbs;
	uint8_t tcasr = (cfg->clocks.i3c_scl_tcasr > 0xff) ? 0xff : cfg->clocks.i3c_scl_tcasr;
	uint8_t tcbsr = (cfg->clocks.i3c_scl_tcbsr > 0xff) ? 0xff : cfg->clocks.i3c_scl_tcbsr;

	pp_freq = config_cntlr->scl.i3c;
	od_freq = cfg->clocks.i3c_od_scl_hz;
	if (pp_freq == 0 || od_freq == 0) {
		LOG_DEV_ERR(dev, "invalid freq pp(%dHz) or od(%dHz)", pp_freq, od_freq);
		return -EINVAL;
	}

	/* use i3c timing setting */
	sys_write8(sys_read8(cfg->base + I3CM2F_I2C_CONTROL) & ~I3CM_USE_I2C_TIMING_SETTING,
		   cfg->base + I3CM2F_I2C_CONTROL);

	/* pphigh_ns = odhigh_ns = (1 / pp_freq) * duty_cycle
	 * pplow_ns = (1 / pp_freq) - pphigh_ns
	 * odlow_ns = (1 / od_freq) - odhigh_ns
	 */
	pphigh_ns = (NSEC_PER_SEC / pp_freq) * pp_duty_cycle / 100;
	pplow_ns = (NSEC_PER_SEC / pp_freq) - pphigh_ns;
	odhigh_ns = pphigh_ns;
	odlow_ns = (NSEC_PER_SEC / od_freq) - odhigh_ns;
	if (odlow_ns < I3C_BUS_TLOW_OD_MIN_NS) {
		LOG_DEV_ERR(dev, "od low period(%dns) is out of spec", odlow_ns);
		return -EINVAL;
	}
	if (pphigh_ns < I3C_BUS_THIGH_PP_MIN_NS) {
		LOG_DEV_ERR(dev, "pp high period(%dns) is out of spec", pphigh_ns);
		return -EINVAL;
	}
	if (pplow_ns < I3C_BUS_TLOW_PP_MIN_NS) {
		LOG_DEV_ERR(dev, "pp low period(%dns) is out of spec", pplow_ns);
		return -EINVAL;
	}

	/* odlow_ns = (odlow + 1) * 20.8 + (hddat + 1) * 20.8
	 * odlow = ((odlow_ns - (hddat + 1) * 20.8) / 20.8) - 1
	 */
	odlow = DIV_ROUND_UP((odlow_ns - (hddat + 1) * 20.8f), 20.8f) - 1;
	odlow = (odlow > 0x1ff) ? 0x1ff : odlow;
	sys_write8(BYTE_0(odlow), cfg->base + I3CM2A_TLOW_OD_LB);
	sys_write8(BYTE_1(odlow), cfg->base + I3CM2B_TLOW_OD_HB);

	/* pphigh_ns = (pphigh + 1) * 20.8
	 * pphigh = (pphigh_ns / 20.8) - 1
	 * odhigh = pphigh
	 */
	pphigh = DIV_ROUND_UP(pphigh_ns, 20.8f) - 1;
	pphigh = (pphigh > 0x1ff) ? 0x1ff : pphigh;
	odhigh = pphigh;
	sys_write8(BYTE_0(pphigh), cfg->base + I3CM28_THIGH_LB);
	sys_write8(BYTE_1(pphigh), cfg->base + I3CM29_THIGH_HB);

	/* pplow_ns = (pplow + 1) * 20.8 + (hddat + 1) * 20.8
	 * pplow = ((pplow_ns - (hddat + 1) * 20.8) / 20.8) - 1
	 */
	pplow = DIV_ROUND_UP((pplow_ns - (hddat + 1) * 20.8f), 20.8f) - 1;
	pplow = (pplow > 0x1ff) ? 0x1ff : pplow;
	sys_write8(BYTE_0(pplow), cfg->base + I3CM26_TLOW_LB);
	sys_write8(BYTE_1(pplow), cfg->base + I3CM27_TLOW_HB);

	sys_write8(hddat, cfg->base + I3CM24_THDDAT_LB);
	sys_write8(tcas, cfg->base + I3CM20_TCAS);
	sys_write8(tcbs, cfg->base + I3CM21_TCBP);
	sys_write8(tcasr, cfg->base + I3CM23_TCASR);
	sys_write8(tcbsr, cfg->base + I3CM22_TCBSR);

	LOG_DEV_INF(dev, "i3c pp_freq: %dHz, od_freq %dHz", pp_freq, od_freq);
	LOG_DEV_DBG(dev, "i3c pphigh_ns: %dns, pplow_ns %dns", pphigh_ns, pplow_ns);
	LOG_DEV_DBG(dev, "i3c odhigh_ns: %dns, odlow_ns %dns", odhigh_ns, odlow_ns);
	LOG_DEV_DBG(dev, "pphigh: %d, pplow %d odhigh: %d, odlow %d", pphigh, pplow, odhigh, odlow);
	LOG_DEV_DBG(dev, "i2c freq: %dHz", config_cntlr->scl.i2c);
	return 0;
}

static int it51xxx_i3cm_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
					 uint8_t num_msgs, uint16_t addr)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	if (!msgs) {
		return -EINVAL;
	}

	if (num_msgs == 0) {
		return 0;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (!msgs[i].buf) {
			return -EINVAL;
		}
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_DEV_ERR(dev, "unsupported i2c extended address");
			return -ENOTSUP;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* TODO: here is ongoing */
	LOG_DEV_ERR(dev, "%s ITE Debug %d", __func__, __LINE__);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int it51xxx_i3cm_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_config_controller *cntlr_cfg = config;
	int ret;

	if (type != I3C_CONFIG_CONTROLLER) {
		LOG_DEV_ERR(dev, "support controller mode only");
		return -ENOTSUP;
	}

	if (cntlr_cfg->is_secondary || cntlr_cfg->scl.i3c == 0U || cntlr_cfg->scl.i2c == 0U) {
		return -EINVAL;
	}

	(void)memcpy(&data->common.ctrl_config, cntlr_cfg, sizeof(*cntlr_cfg));
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = it51xxx_i3cm_init_clock(dev);
	if (ret) {
		return ret;
	}
	k_mutex_unlock(&data->lock);
	return 0;
}

static int it51xxx_i3cm_config_get(const struct device *dev, enum i3c_config_type type,
				   void *config)
{
	struct it51xxx_i3cm_data *data = dev->data;

	if (type != I3C_CONFIG_CONTROLLER) {
		return -ENOTSUP;
	}

	if (!config) {
		return -EINVAL;
	}

	(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));
	return 0;
}

static int it51xxx_i3cm_do_daa(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	LOG_DEV_DBG(dev, "start daa");

	k_mutex_lock(&data->lock, K_FOREVER);

	data->msg_state = IT51XXX_I3CM_MSG_DAA;

	it51xxx_enable_standby_state(dev, false);
	it51xxx_set_op_type(dev, DAA_TRANSFER, false, true);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);

	ret = it51xxx_wait_to_complete(dev);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int it51xxx_broadcast_ccc_xfer(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret;

	ret = it51xxx_set_tx_rx_length(dev, payload->ccc.data_len, 0);
	if (ret) {
		return ret;
	}

	if (payload->ccc.data_len > 0) {
		memcpy(data->dlm_data.tx_data, payload->ccc.data, payload->ccc.data_len);
	}

	data->msg_state = IT51XXX_I3CM_MSG_BROADCAST_CCC;
	it51xxx_set_op_type(dev, BROADCAST_CCC_WRITE_TRANSFER, false, true);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);

	ret = it51xxx_wait_to_complete(dev);
	if (ret) {
		return ret;
	}

	if (payload->ccc.data_len > 0) {
		payload->ccc.num_xfer = payload->ccc.data_len;
	}
	return 0;
}

static void it51xxx_direct_ccc_xfer_end(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = data->direct_msgs.ccc->targets.payloads;
	size_t target_idx = data->direct_msgs.target_idx;
	bool is_read = tgt_payload[target_idx].rnw == 1U;

	if (is_read) {
		tgt_payload[target_idx].data_len = it51xxx_get_received_data_count(dev);
		memcpy(tgt_payload[target_idx].data, data->dlm_data.rx_data,
		       tgt_payload[target_idx].data_len);
		LOG_HEXDUMP_DBG(tgt_payload[target_idx].data, tgt_payload[target_idx].data_len,
				"direct ccc rx:");
	} else {
		tgt_payload[target_idx].num_xfer = tgt_payload[target_idx].data_len;
	}
}

static int it51xxx_start_direct_ccc_xfer(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = &payload->targets.payloads[0];
	bool is_read = tgt_payload->rnw == 1U;
	bool more_transfer;
	uint8_t cycle_type;
	int ret = 0;

	if (is_read) {
		ret = it51xxx_set_tx_rx_length(dev, 0, tgt_payload->data_len);
		if (ret) {
			return ret;
		}
		cycle_type = DIRECT_CCC_READ_TRANSFER;
	} else {
		ret = it51xxx_set_tx_rx_length(dev, tgt_payload->data_len, 0);
		if (ret) {
			return ret;
		}

		memcpy(data->dlm_data.tx_data, tgt_payload->data, tgt_payload->data_len);
		cycle_type = DIRECT_CCC_WRITE_TRANSFER;
	}

	data->direct_msgs.ccc = payload;
	data->msg_state = IT51XXX_I3CM_MSG_DIRECT_CCC;
	more_transfer = (payload->targets.num_targets > 1) ? true : false;
	it51xxx_set_op_type(dev, cycle_type, more_transfer, true);
	it51xxx_set_target_address(dev, tgt_payload->addr, false);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);

	ret = it51xxx_wait_to_complete(dev);
	if (ret) {
		return ret;
	}

	payload->ccc.num_xfer = payload->ccc.data_len;
	it51xxx_direct_ccc_xfer_end(dev);
	data->direct_msgs.target_idx = 0;
	return 0;
}

static int it51xxx_i3cm_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	if (!payload) {
		return -EINVAL;
	}

	LOG_DEV_DBG(dev, "send %s ccc(0x%x)",
		    i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
		    payload->ccc.id);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* disable ccc defining byte */
	sys_write8(sys_read8(cfg->base + I3CM15_CONTROL_2) & ~I3CM_CCC_WITH_DEFINING_BYTE,
		   cfg->base + I3CM15_CONTROL_2);

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		if (payload->ccc.data_len > 1) {
			LOG_DEV_ERR(dev, "only support 1 ccc defining byte");
			ret = -ENOTSUP;
			goto out;
		}
		if (payload->ccc.data_len > 0 && payload->ccc.data == NULL) {
			ret = -EINVAL;
			goto out;
		}
		if (payload->targets.payloads == NULL || payload->targets.num_targets == 0) {
			ret = -EINVAL;
			goto out;
		}
		if (payload->ccc.data_len) {
			/* set ccc defining byte */
			sys_write8(sys_read8(cfg->base + I3CM15_CONTROL_2) |
					   I3CM_CCC_WITH_DEFINING_BYTE,
				   cfg->base + I3CM15_CONTROL_2);
			sys_write8(payload->ccc.data[0], cfg->base + I3CM16_CCC_DEFINING_BYTE);
		}
	} else {
		if (payload->ccc.data_len > 0 && payload->ccc.data == NULL) {
			ret = -EINVAL;
			goto out;
		}
	}

	it51xxx_enable_standby_state(dev, false);

	sys_write8(payload->ccc.id, cfg->base + I3CM03_COMMON_COMMAND_CODE);

	if (i3c_ccc_is_payload_broadcast(payload)) {
		ret = it51xxx_broadcast_ccc_xfer(dev, payload);
	} else {
		ret = it51xxx_start_direct_ccc_xfer(dev, payload);
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static struct i3c_device_desc *it51xxx_i3cm_device_find(const struct device *dev,
							const struct i3c_device_id *id)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;

	return i3c_dev_list_find(&cfg->common.dev_list, id);
}

static int it51xxx_prepare_priv_xfer(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *i3c_msgs = data->i3c_msgs.msg;
	bool more_transfer = false, send_broadcast = false;
	bool emit_stop = i3c_msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_STOP;
	bool is_read = (i3c_msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ;
	int ret = 0;
	uint8_t cycle_type;

	if (emit_stop) {
		if ((data->i3c_msgs.curr_idx + 1) != data->i3c_msgs.num_msgs) {
			LOG_DEV_ERR(dev, "invalid message: too much messages");
			return -EINVAL;
		}
		cycle_type = is_read ? PRIVATE_READ_TRANSFER : PRIVATE_WRITE_TRANSFER;
	} else {
		bool next_is_read;
		bool next_is_restart;

		if ((data->i3c_msgs.curr_idx + 1) > data->i3c_msgs.num_msgs) {
			LOG_DEV_ERR(dev, "invalid message: too few messages");
			return -EINVAL;
		}

		if (is_read) {
			LOG_DEV_ERR(dev,
				    "invalid message: multiple msgs initiated from the read flag");
			return -EINVAL;
		}

		next_is_read = (i3c_msgs[data->i3c_msgs.curr_idx + 1].flags & I3C_MSG_RW_MASK) ==
			       I3C_MSG_READ;
		next_is_restart = ((i3c_msgs[data->i3c_msgs.curr_idx + 1].flags &
				    I3C_MSG_RESTART) == I3C_MSG_RESTART);
		if (!next_is_read && !next_is_restart) {
			/* write then write */
			cycle_type = PRIVATE_WRITE_TRANSFER;
			more_transfer = true;
		} else if (next_is_read) {
			/* write then read */
			cycle_type = PRIVATE_WRITE_READ_TRANSFER;
		} else {
			LOG_DEV_ERR(dev, "invalid message");
			return -EINVAL;
		}
	}

	if (data->i3c_msgs.curr_idx == 0 &&
	    !(i3c_msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_NBCH)) {
		send_broadcast = true;
	}

	ret = it51xxx_start_i3c_private_xfer(dev, cycle_type, data->i3c_msgs.target_addr,
					     more_transfer, send_broadcast);
	if (ret) {
		return ret;
	}
	return 0;
}

static int it51xxx_i3cm_transfer(const struct device *dev, struct i3c_device_desc *target,
				 struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	if (!msgs || target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	if (num_msgs == 0) {
		return 0;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (!msgs[i].buf) {
			return -EINVAL;
		}
		if ((msgs[i].flags & I3C_MSG_HDR) && (msgs[i].hdr_mode != 0)) {
			LOG_DEV_ERR(dev, "unsupported hdr mode");
			return -ENOTSUP;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	it51xxx_enable_standby_state(dev, false);

	data->i3c_msgs.target_addr = target->dynamic_addr;
	data->i3c_msgs.msg = msgs;
	data->i3c_msgs.num_msgs = num_msgs;
	data->i3c_msgs.curr_idx = 0;

	ret = it51xxx_prepare_priv_xfer(dev);
	if (ret) {
		goto out;
	}

	/* start transfer */
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);

	ret = it51xxx_wait_to_complete(dev);
	if (ret) {
		goto out;
	}

	if ((msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE) {
		data->i3c_msgs.msg[data->i3c_msgs.curr_idx].num_xfer =
			data->i3c_msgs.msg[data->i3c_msgs.curr_idx].len;
	}

	if ((msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
		size_t data_count;

		data_count = it51xxx_get_received_data_count(dev);
		msgs[data->i3c_msgs.curr_idx].len = data_count;
		memcpy(msgs[data->i3c_msgs.curr_idx].buf, data->dlm_data.rx_data, data_count);
		LOG_DEV_DBG(dev, "private rx %d bytes", data_count);
		LOG_HEXDUMP_DBG(msgs[data->i3c_msgs.curr_idx].buf,
				msgs[data->i3c_msgs.curr_idx].len, "private xfer rx:");
	}

	if (data->i3c_msgs.curr_idx != data->i3c_msgs.num_msgs - 1) {
		if ((msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE &&
		    msgs[data->i3c_msgs.curr_idx + 1].flags & I3C_MSG_READ) {
			size_t data_count;

			data_count = it51xxx_get_received_data_count(dev);
			msgs[data->i3c_msgs.curr_idx + 1].len = data_count;
			memcpy(msgs[data->i3c_msgs.curr_idx + 1].buf, data->dlm_data.rx_data,
			       data_count);
			LOG_DEV_DBG(dev, "private tx-then-rx %d bytes", data_count);
			LOG_HEXDUMP_DBG(msgs[data->i3c_msgs.curr_idx + 1].buf,
					msgs[data->i3c_msgs.curr_idx + 1].len,
					"private xfer tx-then-rx:");
		}
	}
	data->i3c_msgs.curr_idx = 0;

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

#ifdef CONFIG_I3C_USE_IBI
static inline void it51xxx_accept_ibi(const struct device *dev, bool accept)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;

	if (accept) {
		sys_write8(sys_read8(cfg->base + I3CM10_CONTROL) & ~I3CM_REFUSE_IBI,
			   cfg->base + I3CM10_CONTROL);
	} else {
		sys_write8(sys_read8(cfg->base + I3CM10_CONTROL) | I3CM_REFUSE_IBI,
			   cfg->base + I3CM10_CONTROL);
	}
}

static int it51xxx_i3cm_ibi_hj_response(const struct device *dev, bool ack)
{
	struct it51xxx_i3cm_data *data = dev->data;

	data->ibi_hj_response = ack;
	return 0;
}

static int it51xxx_i3cm_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_events i3c_events;
	int ret;
	uint8_t idx;

	if (!i3c_ibi_has_payload(target)) {
		LOG_DEV_ERR(dev, "i3cm only supports ibi with payload");
		return -ENOTSUP;
	}

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	if (data->ibi.num_addr >= ARRAY_SIZE(data->ibi.addr)) {
		LOG_DEV_ERR(dev, "no more free space in the ibi list");
		return -ENOMEM;
	}

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (data->ibi.addr[idx] == target->dynamic_addr) {
			LOG_DEV_ERR(dev, "selected target is already in the ibi list");
			return -EINVAL;
		}
	}

	if (data->ibi.num_addr > 0) {
		for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
			if (data->ibi.addr[idx] == 0U) {
				break;
			}
		}

		if (idx >= ARRAY_SIZE(data->ibi.addr)) {
			LOG_DEV_ERR(dev, "cannot support more ibis");
			return -ENOTSUP;
		}
	} else {
		idx = 0;
	}

	LOG_DEV_DBG(dev, "ibi enabling for 0x%x (bcr 0x%x)", target->dynamic_addr, target->bcr);

	/* enable target ibi event by ENEC command */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_DEV_ERR(dev, "error sending IBI ENEC for 0x%x(%d)", target->dynamic_addr, ret);
		return ret;
	}

	data->ibi.addr[idx] = target->dynamic_addr;
	data->ibi.num_addr += 1U;

	if (data->ibi.num_addr == 1U) {
		it51xxx_enable_standby_state(dev, false);
	}
	return 0;
}

static int it51xxx_i3cm_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_events i3c_events;
	int ret;
	uint8_t idx;

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (target->dynamic_addr == data->ibi.addr[idx]) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(data->ibi.addr)) {
		LOG_DEV_ERR(dev, "selected target is not in ibi list");
		return -ENODEV;
	}

	data->ibi.addr[idx] = 0U;
	data->ibi.num_addr -= 1U;

	if (data->ibi.num_addr == 0U) {
		it51xxx_enable_standby_state(dev, true);
	}

	LOG_DEV_DBG(dev, "ibi disabling for 0x%x (bcr 0x%x)", target->dynamic_addr, target->bcr);

	/* disable target ibi event by ENEC command */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_DEV_ERR(dev, "error sending IBI ENEC for 0x%x(%d)", target->dynamic_addr, ret);
	}
	return ret;
}
#endif /* CONFIG_I3C_USE_IBI */

static enum i3c_bus_mode i3c_bus_mode(const struct i3c_dev_list *dev_list)
{
	enum i3c_bus_mode mode = I3C_BUS_MODE_PURE;

	for (int i = 0; i < dev_list->num_i2c; i++) {
		switch (I3C_LVR_I2C_DEV_IDX(dev_list->i2c[i].lvr)) {
		case I3C_LVR_I2C_DEV_IDX_0:
			if (mode < I3C_BUS_MODE_MIXED_FAST) {
				mode = I3C_BUS_MODE_MIXED_FAST;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_1:
			if (mode < I3C_BUS_MODE_MIXED_LIMITED) {
				mode = I3C_BUS_MODE_MIXED_LIMITED;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_2:
			if (mode < I3C_BUS_MODE_MIXED_SLOW) {
				mode = I3C_BUS_MODE_MIXED_SLOW;
			}
			break;
		default:
			mode = I3C_BUS_MODE_INVALID;
			break;
		}
	}
	return mode;
}

static int it51xxx_i3cm_init(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	uint8_t reg_val;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_DEV_ERR(dev, "failed to apply pinctrl, ret %d", ret);
		return ret;
	}

	ctrl_config->is_secondary = false;
	ctrl_config->supported_hdr = 0x0;

#ifdef CONFIG_I3C_USE_IBI
	k_sem_init(&data->ibi_lock_sem, 1, 1);
#endif
	k_sem_init(&data->msg_sem, 0, 1);
	k_mutex_init(&data->lock);

	if (i3c_bus_mode(&cfg->common.dev_list) != I3C_BUS_MODE_PURE) {
		LOG_DEV_ERR(dev, "only support pure mode currently");
		return -ENOTSUP;
	}

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		LOG_DEV_ERR(dev, "failed to init slots, ret %d", ret);
		return ret;
	}

	/* clear status, enable the interrupt and refuse ibi bits */
	sys_write8(sys_read8(cfg->base + I3CM01_STATUS) & ~START_TRANSFER,
		   cfg->base + I3CM01_STATUS);
	sys_write8(sys_read8(cfg->base + I3CM10_CONTROL) |
			   (I3CM_REFUSE_IBI | I3CM_INTERRUPT_ENABLE),
		   cfg->base + I3CM10_CONTROL);
	cfg->irq_config_func(dev);

	reg_val = sys_read8(cfg->base + I3CM50_CONTROL_3);
	reg_val &= ~I3CM_DLM_SIZE_MASK;
	switch (CONFIG_I3CM_IT51XXX_DLM_SIZE) {
	case 256:
		reg_val |= FIELD_PREP(I3CM_DLM_SIZE_MASK, 0);
		break;
	case 512:
		reg_val |= FIELD_PREP(I3CM_DLM_SIZE_MASK, 1);
		break;
	case 1024:
		reg_val |= FIELD_PREP(I3CM_DLM_SIZE_MASK, 2);
		break;
	default:
		LOG_DEV_ERR(dev, "invalid dlm size(%d)", CONFIG_I3CM_IT51XXX_DLM_SIZE);
		return -EINVAL;
	};

	/* set i3cm channel selection */
	reg_val &= ~I3CM_CHANNEL_SELECT_MASK;
	LOG_DEV_DBG(dev, "channel %d is selected", cfg->io_channel);
	reg_val |= FIELD_PREP(I3CM_CHANNEL_SELECT_MASK, cfg->io_channel);

	/* select 4k pull-up resistor and enable i3c engine*/
	reg_val |= (I3CM_PULL_UP_RESISTOR | I3CM_ENABLE);
	sys_write8(reg_val, cfg->base + I3CM50_CONTROL_3);

	LOG_DEV_DBG(dev, "dlm base address 0x%x", (uint32_t)&data->dlm_data);
	sys_write8(FIELD_GET(GENMASK(17, 16), (uint32_t)&data->dlm_data),
		   cfg->base + I3CM53_DLM_BASE_ADDRESS_HB);
	sys_write8(BYTE_1((uint32_t)&data->dlm_data), cfg->base + I3CM52_DLM_BASE_ADDRESS_LB);

	ret = it51xxx_i3cm_init_clock(dev);
	if (ret) {
		return ret;
	}

	data->msg_state = IT51XXX_I3CM_MSG_IDLE;
	data->is_initialized = true;

#ifdef CONFIG_I3C_USE_IBI
	data->ibi_hj_response = true;
#endif

	if (cfg->common.dev_list.num_i3c > 0) {
		ret = i3c_bus_init(dev, &cfg->common.dev_list);
		if (ret != 0) {
			LOG_DEV_ERR(dev, "failed to init i3c bus, ret %d", ret);
			return ret;
		}
	}
	return 0;
}

static int it51xxx_daa_next_xfer(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_device_desc *target;
	int ret;
	uint64_t pid;
	uint32_t part_no;
	uint16_t vendor_id;
	uint8_t dyn_addr = 0;
	size_t rx_count;

	rx_count = it51xxx_get_received_data_count(dev);
	if (rx_count != 8) {
		LOG_DEV_ERR(dev, "daa: rx count (%d) not as expected", rx_count);
		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(data->dlm_data.rx_data, rx_count, "6pid/1bcr/1dcr:");
	vendor_id = (((uint16_t)data->dlm_data.rx_data[0] << 8U) |
		     (uint16_t)data->dlm_data.rx_data[1]) &
		    0xFFFEU;
	part_no = (uint32_t)data->dlm_data.rx_data[2] << 24U |
		  (uint32_t)data->dlm_data.rx_data[3] << 16U |
		  (uint32_t)data->dlm_data.rx_data[4] << 8U | (uint32_t)data->dlm_data.rx_data[5];
	pid = (uint64_t)vendor_id << 32U | (uint64_t)part_no;

	/* Find the device in the device list */
	ret = i3c_dev_list_daa_addr_helper(&data->common.attached_dev.addr_slots,
					   &cfg->common.dev_list, pid, false, false, &target,
					   &dyn_addr);
	if (ret != 0) {
		LOG_DEV_ERR(dev, "no dynamic address could be assigned to target");
		return -EINVAL;
	}

	it51xxx_set_target_address(dev, dyn_addr, false);

	if (target != NULL) {
		target->dynamic_addr = dyn_addr;
		target->bcr = data->dlm_data.rx_data[6];
		target->dcr = data->dlm_data.rx_data[7];
	} else {
		LOG_DEV_INF(
			dev,
			"pid 0x%04x%08x is not in registered device list, given dynamic address "
			"0x%x",
			vendor_id, part_no, dyn_addr);
	}

	/* Mark the address as used */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

	/* Mark the static address as free */
	if ((target != NULL) && (target->static_addr != 0) && (dyn_addr != target->static_addr)) {
		i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
					 target->static_addr);
	}
	return 0;
}

static int it51xxx_direct_ccc_next_xfer(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = data->direct_msgs.ccc->targets.payloads;
	uint8_t cycle_type;
	bool more_transfer;
	bool is_read = tgt_payload[data->direct_msgs.target_idx].rnw == 1U;
	int ret = 0;

	it51xxx_direct_ccc_xfer_end(dev);

	/* start next transfer */
	data->direct_msgs.target_idx++;
	if (is_read) {
		ret = it51xxx_set_tx_rx_length(dev, 0,
					       tgt_payload[data->direct_msgs.target_idx].data_len);
		if (ret) {
			return ret;
		}
		cycle_type = PRIVATE_READ_TRANSFER;
	} else {
		ret = it51xxx_set_tx_rx_length(
			dev, tgt_payload[data->direct_msgs.target_idx].data_len, 0);
		if (ret) {
			return ret;
		}
		memcpy(data->dlm_data.tx_data, tgt_payload[data->direct_msgs.target_idx].data,
		       tgt_payload[data->direct_msgs.target_idx].data_len);
		cycle_type = PRIVATE_WRITE_TRANSFER;
	}
	more_transfer =
		(data->direct_msgs.target_idx == data->direct_msgs.ccc->targets.num_targets - 1)
			? false
			: true;
	it51xxx_set_op_type(dev, cycle_type, more_transfer, false);
	it51xxx_set_target_address(dev, tgt_payload[data->direct_msgs.target_idx].addr, false);
	return ret;
}

static int it51xxx_private_next_xfer(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *msgs = data->i3c_msgs.msg;
	bool is_write = ((msgs[data->i3c_msgs.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE);
	bool next_is_write =
		((msgs[data->i3c_msgs.curr_idx + 1].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE);
	bool next_is_restart =
		((msgs[data->i3c_msgs.curr_idx + 1].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);
	int ret;

	if (is_write && next_is_write && !next_is_restart) {
		data->i3c_msgs.curr_idx++;
	} else {
		LOG_DEV_ERR(dev, "unknown next private xfer message");
		return -EINVAL;
	}

	/* prepare the next transfer */
	ret = it51xxx_prepare_priv_xfer(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static void it51xxx_process_ibi_payload(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	size_t payload_sz = 0;
	struct i3c_device_desc *target = i3c_dev_list_i3c_addr_find(dev, data->ibi_target_addr);

	if (i3c_ibi_has_payload(target)) {
		payload_sz = it51xxx_get_received_data_count(dev);
		if (payload_sz == 0) {
			/* Wrong ibi transaction due to missing payload.
			 * A 100us timeout on the targe side may cause this
			 * situation.
			 */
			return;
		} else {
			memcpy(payload, data->dlm_data.rx_data, payload_sz);
			if (payload_sz > CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE) {
				LOG_DEV_ERR(dev, "ibi payloads(%d) is too much", payload_sz);
			}
		}
	}

	if (i3c_ibi_work_enqueue_target_irq(target, &payload[0], payload_sz) != 0) {
		LOG_DEV_ERR(dev, "failed to enqueue tir work");
	}
}

/* TODO: NEED-TO-CHECK */
static inline void it51xxx_check_error(const struct device *dev, const uint8_t int_status)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;

	if (int_status & PARITY_ERROR) {
		LOG_DEV_ERR(dev, "isr: transaction(%d) parity error", data->msg_state);
		data->msg_state = IT51XXX_I3CM_MSG_ERROR;
		sys_write8(PARITY_ERROR, cfg->base + I3CM01_STATUS);
	}

	if (int_status & CRC5_ERROR) {
		/* the crc5 check occurs only in HDR mode */
		LOG_DEV_ERR(dev, "isr: transaction(%d) crc5 error", data->msg_state);
		data->msg_state = IT51XXX_I3CM_MSG_ERROR;
		sys_write8(CRC5_ERROR, cfg->base + I3CM01_STATUS);
	}
}

static void it51xxx_i3cm_isr(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	uint8_t int_status;

	int_status = sys_read8(cfg->base + I3CM01_STATUS);
	int_status &= ~START_TRANSFER;

	if (!data->is_initialized) {
		LOG_DEV_DBG(dev, "i3cm interrupt(0x%x) occurs before initialization was complete",
			    int_status);
	}

	it51xxx_check_error(dev, int_status);

	if (int_status & IBI_INTERRUPT) {
		LOG_DEV_DBG(dev, "isr: ibi interrupt is detected");

		data->msg_state = IT51XXX_I3CM_MSG_IBI;
#ifdef CONFIG_I3C_USE_IBI
		uint8_t ibi_value, ibi_address;

		k_sem_take(&data->ibi_lock_sem, K_FOREVER);

		ibi_value = sys_read8(cfg->base + I3CM0F_IBI_ADDRESS);
		ibi_address = FIELD_GET(I3CM_IBI_ADDR_MASK, ibi_value);
		if (ibi_value & I3CM_IBI_RNW) {
			struct i3c_device_desc *target = NULL;

			target = i3c_dev_list_i3c_addr_find(dev, ibi_address);
			if (target != NULL) {
				data->ibi_target_addr = ibi_address;
				if (i3c_ibi_has_payload(target)) {
					it51xxx_set_tx_rx_length(dev, 0,
								 CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE);
					it51xxx_set_op_type(dev, IBI_READ_TRANSFER, false, true);
				}
				it51xxx_accept_ibi(dev, true);
			} else {
				it51xxx_accept_ibi(dev, false);
			}
			sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
		} else if (ibi_address == I3C_IBI_HJ_ADDR) {
			it51xxx_accept_ibi(dev, data->ibi_hj_response);
			sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
			if (data->ibi_hj_response) {
				if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
					LOG_DEV_ERR(dev, "failed to enqueue hot-join work");
				}
			}
			k_sem_give(&data->ibi_lock_sem);
		} else {
			it51xxx_accept_ibi(dev, false);
			sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
			LOG_DEV_ERR(dev, "unsupported controller role request");
			k_sem_give(&data->ibi_lock_sem);
		}
#else
		LOG_DEV_ERR(dev, "isr: Kconfig I3C_USE_IBI is disabled");
		sys_write8(sys_read8(cfg->base + I3CM10_CONTROL) | I3CM_REFUSE_IBI,
			   cfg->base + I3CM10_CONTROL);
		sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
#endif /* CONFIG_I3C_USE_IBI */
	}

	if (int_status & TRANSFER_END) {
		bool is_sem_lock = data->msg_state == IT51XXX_I3CM_MSG_IBI ? false : true;

		LOG_DEV_DBG(dev, "isr: end transfer is detected");
		/* clear tx and rx length */
		it51xxx_set_tx_rx_length(dev, 0, 0);
		if (int_status & TARGET_NACK) {
			LOG_DEV_DBG(dev, "isr: target nack is detected");
			if (data->msg_state == IT51XXX_I3CM_MSG_DAA) {
				LOG_DEV_DBG(dev, "isr: no target should be assigned address");
				data->msg_state = IT51XXX_I3CM_MSG_IDLE;
			} else {
				LOG_DEV_ERR(dev, "isr: no target responses");
				data->msg_state = IT51XXX_I3CM_MSG_ERROR;
			}
		} else {
			if (data->msg_state != IT51XXX_I3CM_MSG_ERROR) {
				data->msg_state = IT51XXX_I3CM_MSG_IDLE;
			}
		}

		if (is_sem_lock) {
			k_sem_give(&data->msg_sem);
			it51xxx_enable_standby_state(dev, true);
		} else {
			if (data->ibi_target_addr) {
				it51xxx_process_ibi_payload(dev);
				data->ibi_target_addr = 0x0;
				k_sem_give(&data->ibi_lock_sem);
			}
		}
		sys_write8(TARGET_NACK | TRANSFER_END, cfg->base + I3CM01_STATUS);
	}

	if (int_status & NEXT_TRANSFER) {
		int ret = 0;

		LOG_DEV_DBG(dev, "isr: next transfer is detected");
		switch (data->msg_state) {
		case IT51XXX_I3CM_MSG_DAA:
			ret = it51xxx_daa_next_xfer(dev);
			break;
		case IT51XXX_I3CM_MSG_DIRECT_CCC:
			ret = it51xxx_direct_ccc_next_xfer(dev);
			break;
		case IT51XXX_I3CM_MSG_PRIVATE_XFER:
			ret = it51xxx_private_next_xfer(dev);
			break;
		default:
			ret = -EINVAL;
			LOG_DEV_ERR(dev, "isr: next transfer, unknown msg status(0x%x)",
				    data->msg_state);
			break;
		};

		if (ret) {
			/* TODO: figure out what is the correct sequence to abort next transfer */
			data->msg_state = IT51XXX_I3CM_MSG_ERROR;
		}
		sys_write8(NEXT_TRANSFER, cfg->base + I3CM01_STATUS);
	}
}

static DEVICE_API(i3c, it51xxx_i3cm_api) = {
	.i2c_api.transfer = it51xxx_i3cm_i2c_api_transfer,

	.configure = it51xxx_i3cm_configure,
	.config_get = it51xxx_i3cm_config_get,

	.do_daa = it51xxx_i3cm_do_daa,
	.do_ccc = it51xxx_i3cm_do_ccc,

	.i3c_device_find = it51xxx_i3cm_device_find,

	.i3c_xfers = it51xxx_i3cm_transfer,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_hj_response = it51xxx_i3cm_ibi_hj_response,
	.ibi_enable = it51xxx_i3cm_ibi_enable,
	.ibi_disable = it51xxx_i3cm_ibi_disable,
#endif /* CONFIG_I3C_USE_IBI */
};

#define IT51XXX_I3CM_INIT(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct i3c_device_desc it51xxx_i3cm_device_array_##n[] =                            \
		I3C_DEVICE_ARRAY_DT_INST(n);                                                       \
	static struct i3c_i2c_device_desc it51xxx_i3cm_i2c_device_array_##n[] =                    \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);                                                   \
	static void it51xxx_i3cm_config_func_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), it51xxx_i3cm_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static const struct it51xxx_i3cm_config i3c_config_##n = {                                 \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = it51xxx_i3cm_config_func_##n,                                   \
		.common.dev_list.i3c = it51xxx_i3cm_device_array_##n,                              \
		.common.dev_list.num_i3c = ARRAY_SIZE(it51xxx_i3cm_device_array_##n),              \
		.common.dev_list.i2c = it51xxx_i3cm_i2c_device_array_##n,                          \
		.common.dev_list.num_i2c = ARRAY_SIZE(it51xxx_i3cm_i2c_device_array_##n),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.io_channel = DT_INST_PROP(n, io_channel),                                         \
		.clocks.i3c_pp_duty_cycle = DT_INST_PROP_OR(n, i3c_pp_duty_cycle, 0),              \
		.clocks.i3c_od_scl_hz = DT_INST_PROP_OR(n, i3c_od_scl_hz, 0),                      \
		.clocks.i3c_scl_hddat = DT_INST_PROP_OR(n, i3c_scl_hddat, 0),                      \
		.clocks.i3c_scl_tcas = DT_INST_PROP_OR(n, i3c_scl_tcas, 1),                        \
		.clocks.i3c_scl_tcbs = DT_INST_PROP_OR(n, i3c_scl_tcbs, 0),                        \
		.clocks.i3c_scl_tcasr = DT_INST_PROP_OR(n, i3c_scl_tcasr, 1),                      \
		.clocks.i3c_scl_tcbsr = DT_INST_PROP_OR(n, i3c_scl_tcbsr, 0),                      \
	};                                                                                         \
	static struct it51xxx_i3cm_data i3c_data_##n = {                                           \
		.common.ctrl_config.scl.i3c = DT_INST_PROP_OR(n, i3c_scl_hz, 0),                   \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, it51xxx_i3cm_init, NULL, &i3c_data_##n, &i3c_config_##n,          \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,                    \
			      &it51xxx_i3cm_api);

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_I3CM_INIT)
