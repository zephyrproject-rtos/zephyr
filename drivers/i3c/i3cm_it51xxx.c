/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_i3cm

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>
LOG_MODULE_REGISTER(i3cm_it51xxx);

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/policy.h>

#include <soc_common.h>

#define BYTE_0(x) FIELD_GET(GENMASK(7, 0), x)
#define BYTE_1(x) FIELD_GET(GENMASK(15, 8), x)

#define CALC_FREQUENCY(t_low, t_hddat, t_high)                                                     \
	(NSEC_PER_SEC / (t_high + t_low + t_hddat + 3) / 208 * 10)

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
#define I3CM20_TCAS                 0x20 /* i3c: clock after start condition */
#define I3CM21_TCBP                 0x21 /* i3c: clock before stop condition */
#define I3CM22_TCBSR                0x22 /* i3c: clock before repeated start condition */
#define I3CM23_TCASR                0x23 /* i3c: clock after repeated start condition */
#define I3CM24_THDDAT_LB            0x24 /* i3c: low byte of data hold time */
#define I3CM26_TLOW_LB              0x26 /* i3c: low byte of scl clock low period */
#define I3CM27_TLOW_HB              0x27 /* i3c: high byte of scl clock low period */
#define I3CM28_THIGH_LB             0x28 /* i3c: low byte of scl clock high period */
#define I3CM29_THIGH_HB             0x29 /* i3c: high byte of scl clock high period */
#define I3CM2A_TLOW_OD_LB           0x2A /* i3c: low byte of open drain scl clock low period */
#define I3CM2B_TLOW_OD_HB           0x2B /* i3c: high byte of open drain scl clock low period */
#define I3CM2F_I2C_CONTROL          0x2F
#define I3CM_USE_I2C_TIMING_SETTING BIT(1)

#define I3CM30_I2C_THDSTA_SUSTO_LB                                                                 \
	0x30 /* i2c: low byte of "hold time for a (repeated) start"/"setup time for stop" */
#define I3CM31_I2C_THDSTA_SUSTO_HB                                                                 \
	0x31 /* i2c: high byte of "hold time for a (repeated) start"/"setup time for stop" */
#define I3CM34_I2C_THDDAT_LB 0x34 /* i2c: low byte of data hold time */
#define I3CM35_I2C_THDDAT_HB 0x35 /* i2c: high byte of data hold time */
#define I3CM36_I2C_TLOW_LB   0x36 /* i2c: low byte of scl clock low period */
#define I3CM37_I2C_TLOW_HB   0x37 /* i2c: high byte of scl clock low period */
#define I3CM38_I2C_THIGH_LB  0x38 /* i2c: low byte of scl clock high period */
#define I3CM39_I2C_THIGH_HB  0x39 /* i2c: high byte of scl clock high period */

#define I3CM50_CONTROL_3         0x50
#define I3CM_DLM_SIZE_MASK       GENMASK(5, 4)
#define I3CM_CHANNEL_SELECT_MASK GENMASK(3, 2)
#define I3CM_PULL_UP_RESISTOR    BIT(1)
#define I3CM_ENABLE              BIT(0)

#define I3CM52_DLM_BASE_ADDRESS_LB 0x52 /* dlm base address[15:8] */
#define I3CM53_DLM_BASE_ADDRESS_HB 0x53 /* dlm base address[17:16] */

#define I3C_IBI_HJ_ADDR 0x02

#define I3C_BUS_TLOW_PP_MIN_NS  24  /* T_LOW period in push-pull mode */
#define I3C_BUS_THIGH_PP_MIN_NS 24  /* T_HIGH period in push-pull mode */
#define I3C_BUS_TLOW_OD_MIN_NS  200 /* T_LOW period in open-drain mode */

enum it51xxx_cycle_types {
	PRIVATE_WRITE_TRANSFER = 0,
	PRIVATE_READ_TRANSFER,
	PRIVATE_WRITE_READ_TRANSFER,
	LEGACY_I2C_WRITE_TRANSFER,
	LEGACY_I2C_READ_TRANSFER,
	LEGACY_I2C_WRITE_READ_TRANSFER,
	BROADCAST_CCC_WRITE_TRANSFER,
	DIRECT_CCC_WRITE_TRANSFER,
	DIRECT_CCC_READ_TRANSFER,
	DAA_TRANSFER,
	IBI_READ_TRANSFER,
	HDR_TRANSFER,
};

enum it51xxx_message_state {
	IT51XXX_I3CM_MSG_IDLE = 0,
	IT51XXX_I3CM_MSG_BROADCAST_CCC,
	IT51XXX_I3CM_MSG_DIRECT_CCC,
	IT51XXX_I3CM_MSG_DAA,
	IT51XXX_I3CM_MSG_PRIVATE_XFER,
	IT51XXX_I3CM_MSG_IBI,
	IT51XXX_I3CM_MSG_ABORT,
	IT51XXX_I3CM_MSG_ERROR,
};

struct it51xxx_i3cm_data {
	/* common i3c driver data */
	struct i3c_driver_data common;

	enum it51xxx_message_state msg_state;

	struct {
		struct i3c_ccc_payload *payload;
		size_t target_idx;
	} ccc_msgs;

	struct {
		uint8_t target_addr;

		uint8_t num_msgs;
		uint8_t curr_idx;
		struct i3c_msg *i3c_msgs;
		struct i2c_msg *i2c_msgs;
	} curr_msg;

#ifdef CONFIG_I3C_USE_IBI
	bool ibi_hj_response;
	uint8_t ibi_target_addr;

	struct {
		uint8_t addr[4];
		uint8_t num_addr;
	} ibi;
#endif /* CONFIG_I3C_USE_IBI */

	struct k_sem msg_sem;
	struct k_mutex lock;

	bool is_initialized;
	bool error_is_detected;
	bool transfer_is_aborted; /* record that the transfer was aborted due to ibi transaction. */

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
	uint8_t io_channel;
	uint8_t irq_num;

	struct {
		uint8_t i3c_pp_duty_cycle;
		uint32_t i3c_od_scl_hz;
		uint32_t i3c_scl_hddat;
		uint32_t i3c_scl_tcas;
		uint32_t i3c_scl_tcbs;
		uint32_t i3c_scl_tcasr;
		uint32_t i3c_scl_tcbsr;
		uint32_t i2c_scl_hddat;
	} clocks;

	void (*irq_config_func)(const struct device *dev);

	LOG_INSTANCE_PTR_DECLARE(log);
};

static inline bool bus_is_idle(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;

	return (data->msg_state == IT51XXX_I3CM_MSG_IDLE);
}

static int it51xxx_curr_msg_init(const struct device *dev, struct i3c_msg *i3c_msgs,
				 struct i2c_msg *i2c_msgs, uint8_t num_msgs, uint8_t tgt_addr)
{
	struct it51xxx_i3cm_data *data = dev->data;

	__ASSERT(!(i3c_msgs == NULL && i2c_msgs == NULL), "both i3c_msgs and i2c_msgs are null");
	__ASSERT(!(i3c_msgs != NULL && i2c_msgs != NULL),
		 "both i3c_msgs and i2c_msgs are not null");

	data->curr_msg.target_addr = tgt_addr;
	data->curr_msg.num_msgs = num_msgs;
	data->curr_msg.curr_idx = 0;
	data->curr_msg.i3c_msgs = i3c_msgs;
	data->curr_msg.i2c_msgs = i2c_msgs;

	return 0;
}

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
		LOG_INST_ERR(cfg->log, "invalid tx(%d) or rx(%d) length", tx_len, rx_len);
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
	LOG_INST_DBG(cfg->log, "set cycle type(%d) %s broadcast address %s", cycle_type,
		     broadcast_address ? "with" : "without",
		     more_transfer ? "and more transfer flag" : "");
}

static int it51xxx_wait_to_complete(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	if (k_sem_take(&data->msg_sem, K_MSEC(CONFIG_I3CM_IT51XXX_TRANSFER_TIMEOUT_MS)) != 0) {
		LOG_INST_ERR(cfg->log, "timeout: message status(%d)", data->msg_state);
		ret = -ETIMEDOUT;
	}

	irq_disable(cfg->irq_num);
	if (data->transfer_is_aborted) {
		data->transfer_is_aborted = false;
		ret = -EBUSY;
	}
	if (data->error_is_detected) {
		data->error_is_detected = false;
		ret = -EIO;
	}
	irq_enable(cfg->irq_num);

	return ret;
}

static bool it51xxx_curr_msg_is_i3c(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;

	return (data->curr_msg.i3c_msgs != NULL);
}

static int it51xxx_start_i3c_i2c_private_xfer(const struct device *dev, const uint8_t cycle_type,
					      const uint8_t dynamic_addr, const bool more_transfer,
					      const bool broadcast_address)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i2c_msg *i2c_msgs = data->curr_msg.i2c_msgs;
	struct i3c_msg *i3c_msgs = data->curr_msg.i3c_msgs;
	size_t tx_length = 0, rx_length = 0;
	int ret;

	switch (cycle_type) {
	case LEGACY_I2C_WRITE_TRANSFER:
		__fallthrough;
	case PRIVATE_WRITE_TRANSFER:
		rx_length = 0;
		tx_length = it51xxx_curr_msg_is_i3c(dev) ? i3c_msgs[data->curr_msg.curr_idx].len
							 : i2c_msgs[data->curr_msg.curr_idx].len;
		break;
	case LEGACY_I2C_READ_TRANSFER:
		__fallthrough;
	case PRIVATE_READ_TRANSFER:
		rx_length = it51xxx_curr_msg_is_i3c(dev) ? i3c_msgs[data->curr_msg.curr_idx].len
							 : i2c_msgs[data->curr_msg.curr_idx].len;
		tx_length = 0;
		break;
	case LEGACY_I2C_WRITE_READ_TRANSFER:
		__fallthrough;
	case PRIVATE_WRITE_READ_TRANSFER:
		tx_length = it51xxx_curr_msg_is_i3c(dev) ? i3c_msgs[data->curr_msg.curr_idx].len
							 : i2c_msgs[data->curr_msg.curr_idx].len;
		rx_length = it51xxx_curr_msg_is_i3c(dev)
				    ? i3c_msgs[data->curr_msg.curr_idx + 1].len
				    : i2c_msgs[data->curr_msg.curr_idx + 1].len;
		break;
	default:
		LOG_INST_ERR(cfg->log, "unsupported cycle type(0x%x)", cycle_type);
		return -ENOTSUP;
	}

	ret = it51xxx_set_tx_rx_length(dev, tx_length, rx_length);
	if (ret) {
		return ret;
	}

	if (tx_length) {
		if (it51xxx_curr_msg_is_i3c(dev)) {
			memcpy(data->dlm_data.tx_data, i3c_msgs[data->curr_msg.curr_idx].buf,
			       tx_length);
		} else {
			memcpy(data->dlm_data.tx_data, i2c_msgs[data->curr_msg.curr_idx].buf,
			       tx_length);
		}
	}

	sys_write8(I3CM_TARGET_ADDRESS(dynamic_addr), cfg->base + I3CM02_TARGET_ADDRESS);

	/* set cycle type register */
	it51xxx_set_op_type(dev, cycle_type, more_transfer, broadcast_address);
	data->msg_state = IT51XXX_I3CM_MSG_PRIVATE_XFER;

	return 0;
}

static inline int it51xxx_set_i2c_clock(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_config_controller *config_cntlr = &data->common.ctrl_config;
	uint32_t t_high_period_ns, t_low_period_ns;
	uint32_t t_high, t_low;
	uint16_t t_hddat =
		(cfg->clocks.i2c_scl_hddat > 0xFFFF) ? 0xFFFF : cfg->clocks.i2c_scl_hddat;

	/* high_period_ns = ns_per_sec / config_cntlr->scl.i2c / 2;
	 * high_period_ns = (t_high + 1) * 20.8
	 * t_high = ((ns_per_sec / config_cntlr->scl.i2c / 2) / 20.8) - 1
	 */
	t_high_period_ns = NSEC_PER_SEC / config_cntlr->scl.i2c / 2;
	t_high = DIV_ROUND_UP(t_high_period_ns * 10, 208) - 1;

	/* t_low_period_ns = (ns_per_sec / config_cntlr->scl.i2c) - high_period_ns
	 * t_low_period_ns = (t_low + 1 + t_hddat + 1) * 20.8
	 * t_low = (t_low_period_ns / 20.8) - t_hddat - 2
	 */
	t_low_period_ns = (NSEC_PER_SEC / config_cntlr->scl.i2c) - t_high_period_ns;
	t_low = DIV_ROUND_UP(t_low_period_ns * 10, 208) - t_hddat - 2;

	if (t_high > 0xFFFF || t_low > 0xFFFF) {
		LOG_INST_ERR(cfg->log, "invalid t_high(0x%x) or t_low(0x%x) setting", t_high,
			     t_low);
	}

	sys_write8(BYTE_0(t_high), cfg->base + I3CM30_I2C_THDSTA_SUSTO_LB);
	sys_write8(BYTE_1(t_high), cfg->base + I3CM31_I2C_THDSTA_SUSTO_HB);
	sys_write8(BYTE_0(t_hddat), cfg->base + I3CM34_I2C_THDDAT_LB);
	sys_write8(BYTE_1(t_hddat), cfg->base + I3CM35_I2C_THDDAT_HB);
	sys_write8(BYTE_0(t_low), cfg->base + I3CM36_I2C_TLOW_LB);
	sys_write8(BYTE_1(t_low), cfg->base + I3CM37_I2C_TLOW_HB);
	sys_write8(BYTE_0(t_high), cfg->base + I3CM38_I2C_THIGH_LB);
	sys_write8(BYTE_1(t_high), cfg->base + I3CM39_I2C_THIGH_HB);

	LOG_INST_DBG(cfg->log, "i2c: t_high 0x%x, t_low 0x%x t_hddat 0x%x", t_high, t_low, t_hddat);
	LOG_INST_DBG(cfg->log, "i2c: high period: %dns, low period: %dns", t_high_period_ns,
		     t_low_period_ns);
	LOG_INST_INF(cfg->log, "i2c: freq: %dHz -> %dHz", config_cntlr->scl.i2c,
		     CALC_FREQUENCY(t_low, t_hddat, t_high));

	return 0;
}

static inline int it51xxx_set_i3c_clock(const struct device *dev)
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
		LOG_INST_ERR(cfg->log, "invalid freq pp(%dHz) or od(%dHz)", pp_freq, od_freq);
		return -EINVAL;
	}

	/* use i3c timing setting */
	sys_write8(sys_read8(cfg->base + I3CM2F_I2C_CONTROL) & ~I3CM_USE_I2C_TIMING_SETTING,
		   cfg->base + I3CM2F_I2C_CONTROL);

	/* pphigh_ns = odhigh_ns = (ns_per_sec / pp_freq) * duty_cycle
	 * pplow_ns = (ns_per_sec / pp_freq) - pphigh_ns
	 * odlow_ns = (ns_per_sec / od_freq) - odhigh_ns
	 */
	pphigh_ns = DIV_ROUND_UP(DIV_ROUND_UP(NSEC_PER_SEC, pp_freq) * pp_duty_cycle, 100);
	pplow_ns = DIV_ROUND_UP(NSEC_PER_SEC, pp_freq) - pphigh_ns;
	odhigh_ns = pphigh_ns;
	odlow_ns = DIV_ROUND_UP(NSEC_PER_SEC, od_freq) - odhigh_ns;
	if (odlow_ns < I3C_BUS_TLOW_OD_MIN_NS) {
		LOG_INST_ERR(cfg->log, "od low period(%dns) is out of spec", odlow_ns);
		return -EINVAL;
	}
	if (pphigh_ns < I3C_BUS_THIGH_PP_MIN_NS) {
		LOG_INST_ERR(cfg->log, "pp high period(%dns) is out of spec", pphigh_ns);
		return -EINVAL;
	}
	if (pplow_ns < I3C_BUS_TLOW_PP_MIN_NS) {
		LOG_INST_ERR(cfg->log, "pp low period(%dns) is out of spec", pplow_ns);
		return -EINVAL;
	}

	/* odlow_ns = (odlow + 1) * 20.8 + (hddat + 1) * 20.8
	 * odlow = (odlow_ns / 20.8) - hddat - 2
	 */
	odlow = DIV_ROUND_UP(odlow_ns * 10, 208) - hddat - 2;
	odlow = (odlow > 0x1ff) ? 0x1ff : odlow;
	sys_write8(BYTE_0(odlow), cfg->base + I3CM2A_TLOW_OD_LB);
	sys_write8(BYTE_1(odlow), cfg->base + I3CM2B_TLOW_OD_HB);

	/* pphigh_ns = (pphigh + 1) * 20.8
	 * pphigh = (pphigh_ns / 20.8) - 1
	 * odhigh = pphigh
	 */
	pphigh = DIV_ROUND_UP(pphigh_ns * 10, 208) - 1;
	pphigh = (pphigh > 0x1ff) ? 0x1ff : pphigh;
	odhigh = pphigh;
	sys_write8(BYTE_0(pphigh), cfg->base + I3CM28_THIGH_LB);
	sys_write8(BYTE_1(pphigh), cfg->base + I3CM29_THIGH_HB);

	/* pplow_ns = (pplow + 1) * 20.8 + (hddat + 1) * 20.8
	 * pplow = (pplow_ns / 20.8) - hddat - 2
	 */
	pplow = DIV_ROUND_UP(pplow_ns * 10, 208) - hddat - 2;
	pplow = (pplow > 0x1ff) ? 0x1ff : pplow;
	sys_write8(BYTE_0(pplow), cfg->base + I3CM26_TLOW_LB);
	sys_write8(BYTE_1(pplow), cfg->base + I3CM27_TLOW_HB);

	sys_write8(hddat, cfg->base + I3CM24_THDDAT_LB);
	sys_write8(tcas, cfg->base + I3CM20_TCAS);
	sys_write8(tcbs, cfg->base + I3CM21_TCBP);
	sys_write8(tcasr, cfg->base + I3CM23_TCASR);
	sys_write8(tcbsr, cfg->base + I3CM22_TCBSR);

	LOG_INST_DBG(cfg->log, "i3c: pphigh_ns: %dns, pplow_ns %dns", pphigh_ns, pplow_ns);
	LOG_INST_DBG(cfg->log, "i3c: odhigh_ns: %dns, odlow_ns %dns", odhigh_ns, odlow_ns);
	LOG_INST_DBG(cfg->log, "i3c: pphigh: %d, pplow %d, odhigh: %d, odlow %d, hddat %d", pphigh,
		     pplow, odhigh, odlow, hddat);
	LOG_INST_INF(cfg->log, "i3c: pp_freq: %dHz -> %dHz, od_freq %dHz -> %dHz", pp_freq,
		     CALC_FREQUENCY(pplow, hddat, pphigh), od_freq,
		     CALC_FREQUENCY(odlow, hddat, odhigh));

	return 0;
}

static int it51xxx_set_frequency(const struct device *dev)
{
	int ret;

	ret = it51xxx_set_i3c_clock(dev);
	if (ret) {
		goto out;
	}

	ret = it51xxx_set_i2c_clock(dev);
	if (ret) {
		goto out;
	}

out:
	return ret;
}

static int it51xxx_prepare_priv_xfer(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *i3c_msgs = data->curr_msg.i3c_msgs;
	struct i2c_msg *i2c_msgs = data->curr_msg.i2c_msgs;
	bool more_transfer = false, send_broadcast = false, emit_stop, is_read;
	int ret = 0;
	uint8_t cycle_type;

	if (it51xxx_curr_msg_is_i3c(dev)) {
		emit_stop = i3c_msgs[data->curr_msg.curr_idx].flags & I3C_MSG_STOP;
		is_read =
			(i3c_msgs[data->curr_msg.curr_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ;
	} else {
		emit_stop = i2c_msgs[data->curr_msg.curr_idx].flags & I2C_MSG_STOP;
		is_read =
			(i2c_msgs[data->curr_msg.curr_idx].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
	}

	if (emit_stop) {
		if ((data->curr_msg.curr_idx + 1) != data->curr_msg.num_msgs) {
			LOG_INST_ERR(cfg->log, "invalid message: too much messages");
			return -EINVAL;
		}
		if (it51xxx_curr_msg_is_i3c(dev)) {
			cycle_type = is_read ? PRIVATE_READ_TRANSFER : PRIVATE_WRITE_TRANSFER;
		} else {
			cycle_type = is_read ? LEGACY_I2C_READ_TRANSFER : LEGACY_I2C_WRITE_TRANSFER;
		}
	} else {
		bool next_is_read;
		bool next_is_restart;

		if ((data->curr_msg.curr_idx + 1) > data->curr_msg.num_msgs) {
			LOG_INST_ERR(cfg->log, "invalid message: too few messages");
			return -EINVAL;
		}

		if (is_read) {
			LOG_INST_ERR(cfg->log,
				     "invalid message: multiple msgs initiated from the read flag");
			return -EINVAL;
		}

		if (it51xxx_curr_msg_is_i3c(dev)) {
			next_is_read = (i3c_msgs[data->curr_msg.curr_idx + 1].flags &
					I3C_MSG_RW_MASK) == I3C_MSG_READ;
			next_is_restart = ((i3c_msgs[data->curr_msg.curr_idx + 1].flags &
					    I3C_MSG_RESTART) == I3C_MSG_RESTART);
		} else {
			next_is_read = (i2c_msgs[data->curr_msg.curr_idx + 1].flags &
					I2C_MSG_RW_MASK) == I2C_MSG_READ;
			next_is_restart = ((i2c_msgs[data->curr_msg.curr_idx + 1].flags &
					    I2C_MSG_RESTART) == I2C_MSG_RESTART);
		}

		if (!next_is_read && !next_is_restart) {
			/* burst write */
			if (!it51xxx_curr_msg_is_i3c(dev)) {
				/* legacy i2c transfer doesn't support burst write */
				return -ENOTSUP;
			}
			cycle_type = PRIVATE_WRITE_TRANSFER;
			more_transfer = true;
		} else if (next_is_read) {
			/* write then read */
			cycle_type = it51xxx_curr_msg_is_i3c(dev) ? PRIVATE_WRITE_READ_TRANSFER
								  : LEGACY_I2C_WRITE_READ_TRANSFER;
		} else {
			LOG_INST_ERR(cfg->log, "invalid message");
			return -EINVAL;
		}
	}

	if (it51xxx_curr_msg_is_i3c(dev) && data->curr_msg.curr_idx == 0 &&
	    !(i3c_msgs[data->curr_msg.curr_idx].flags & I3C_MSG_NBCH)) {
		send_broadcast = true;
	}

	ret = it51xxx_start_i3c_i2c_private_xfer(dev, cycle_type, data->curr_msg.target_addr,
						 more_transfer, send_broadcast);
	if (ret) {
		return ret;
	}

	return 0;
}

static int it51xxx_i3cm_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
					 uint8_t num_msgs, uint16_t addr)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret;

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
			LOG_INST_ERR(cfg->log, "unsupported i2c extended address");
			return -ENOTSUP;
		}
	}

	irq_disable(cfg->irq_num);
	if (!bus_is_idle(dev)) {
		irq_enable(cfg->irq_num);
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	it51xxx_enable_standby_state(dev, false);

	it51xxx_curr_msg_init(dev, NULL, msgs, num_msgs, addr);
	ret = it51xxx_prepare_priv_xfer(dev);
	if (ret) {
		irq_enable(cfg->irq_num);
		goto out;
	}

	/* start transfer */
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);
	irq_enable(cfg->irq_num);

	ret = it51xxx_wait_to_complete(dev);

out:
	data->curr_msg.curr_idx = 0;
	it51xxx_enable_standby_state(dev, true);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int it51xxx_i3cm_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_config_controller *cntlr_cfg = config;
	int ret;

	if (type != I3C_CONFIG_CONTROLLER) {
		LOG_INST_ERR(cfg->log, "support controller mode only");
		return -ENOTSUP;
	}

	if (cntlr_cfg->is_secondary || cntlr_cfg->scl.i3c == 0U || cntlr_cfg->scl.i2c == 0U) {
		return -EINVAL;
	}

	(void)memcpy(&data->common.ctrl_config, cntlr_cfg, sizeof(*cntlr_cfg));
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = it51xxx_set_frequency(dev);
	k_mutex_unlock(&data->lock);

	return ret;
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

	LOG_INST_DBG(cfg->log, "start daa");

	irq_disable(cfg->irq_num);
	if (!bus_is_idle(dev)) {
		irq_enable(cfg->irq_num);
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->msg_state = IT51XXX_I3CM_MSG_DAA;

	it51xxx_enable_standby_state(dev, false);
	it51xxx_set_op_type(dev, DAA_TRANSFER, false, true);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);
	irq_enable(cfg->irq_num);

	ret = it51xxx_wait_to_complete(dev);

	it51xxx_enable_standby_state(dev, true);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int it51xxx_broadcast_ccc_xfer(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret;

	irq_disable(cfg->irq_num);
	if (!bus_is_idle(dev)) {
		irq_enable(cfg->irq_num);
		return -EBUSY;
	}

	ret = it51xxx_set_tx_rx_length(dev, payload->ccc.data_len, 0);
	if (ret) {
		irq_enable(cfg->irq_num);
		return ret;
	}

	if (payload->ccc.data_len > 0) {
		memcpy(data->dlm_data.tx_data, payload->ccc.data, payload->ccc.data_len);
	}

	data->ccc_msgs.payload = payload;
	data->msg_state = IT51XXX_I3CM_MSG_BROADCAST_CCC;
	it51xxx_set_op_type(dev, BROADCAST_CCC_WRITE_TRANSFER, false, true);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);
	irq_enable(cfg->irq_num);

	return it51xxx_wait_to_complete(dev);
}

static void it51xxx_direct_ccc_xfer_end(const struct device *dev)
{
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = data->ccc_msgs.payload->targets.payloads;
	size_t target_idx = data->ccc_msgs.target_idx;
	bool is_read = tgt_payload[target_idx].rnw == 1U;
	size_t data_count;

	if (is_read) {
		data_count = it51xxx_get_received_data_count(dev);
		memcpy(tgt_payload[target_idx].data, data->dlm_data.rx_data,
		       MIN(tgt_payload[target_idx].data_len, data_count));
		LOG_HEXDUMP_DBG(tgt_payload[target_idx].data, tgt_payload[target_idx].data_len,
				"direct ccc rx:");
	}
	tgt_payload[target_idx].num_xfer = is_read ? data_count : tgt_payload[target_idx].data_len;
}

static int it51xxx_start_direct_ccc_xfer(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = &payload->targets.payloads[0];
	bool is_read = tgt_payload->rnw == 1U;
	bool more_transfer;
	uint8_t cycle_type;
	int ret;

	irq_disable(cfg->irq_num);
	if (!bus_is_idle(dev)) {
		irq_enable(cfg->irq_num);
		return -EBUSY;
	}

	if (is_read) {
		ret = it51xxx_set_tx_rx_length(dev, 0, tgt_payload->data_len);
		if (ret) {
			irq_enable(cfg->irq_num);
			return ret;
		}
		cycle_type = DIRECT_CCC_READ_TRANSFER;
	} else {
		ret = it51xxx_set_tx_rx_length(dev, tgt_payload->data_len, 0);
		if (ret) {
			irq_enable(cfg->irq_num);
			return ret;
		}

		memcpy(data->dlm_data.tx_data, tgt_payload->data, tgt_payload->data_len);
		cycle_type = DIRECT_CCC_WRITE_TRANSFER;
	}

	data->ccc_msgs.payload = payload;
	data->msg_state = IT51XXX_I3CM_MSG_DIRECT_CCC;
	more_transfer = (payload->targets.num_targets > 1) ? true : false;
	it51xxx_set_op_type(dev, cycle_type, more_transfer, true);
	sys_write8(I3CM_TARGET_ADDRESS(tgt_payload->addr), cfg->base + I3CM02_TARGET_ADDRESS);
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);
	irq_enable(cfg->irq_num);

	return it51xxx_wait_to_complete(dev);
}

static int it51xxx_i3cm_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret = 0;

	if (!payload) {
		return -EINVAL;
	}

	LOG_INST_DBG(cfg->log, "send %s ccc(0x%x)",
		     i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
		     payload->ccc.id);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* disable ccc defining byte */
	sys_write8(sys_read8(cfg->base + I3CM15_CONTROL_2) & ~I3CM_CCC_WITH_DEFINING_BYTE,
		   cfg->base + I3CM15_CONTROL_2);

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		if (payload->ccc.data_len > 1) {
			LOG_INST_ERR(cfg->log, "only support 1 ccc defining byte");
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

	it51xxx_enable_standby_state(dev, true);

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

static int it51xxx_i3cm_transfer(const struct device *dev, struct i3c_device_desc *target,
				 struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	int ret;

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
			LOG_INST_ERR(cfg->log, "unsupported hdr mode");
			return -ENOTSUP;
		}
	}

	irq_disable(cfg->irq_num);
	if (!bus_is_idle(dev)) {
		irq_enable(cfg->irq_num);
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	it51xxx_enable_standby_state(dev, false);

	it51xxx_curr_msg_init(dev, msgs, NULL, num_msgs, target->dynamic_addr);
	ret = it51xxx_prepare_priv_xfer(dev);
	if (ret) {
		irq_enable(cfg->irq_num);
		goto out;
	}

	/* start transfer */
	sys_write8(START_TRANSFER, cfg->base + I3CM01_STATUS);
	irq_enable(cfg->irq_num);

	ret = it51xxx_wait_to_complete(dev);

out:
	it51xxx_enable_standby_state(dev, true);
	data->curr_msg.curr_idx = 0;
	k_mutex_unlock(&data->lock);

	return ret;
}

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

#ifdef CONFIG_I3C_USE_IBI
static int it51xxx_i3cm_ibi_hj_response(const struct device *dev, bool ack)
{
	struct it51xxx_i3cm_data *data = dev->data;

	data->ibi_hj_response = ack;

	return 0;
}

static int it51xxx_i3cm_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_events i3c_events;
	int ret;
	uint8_t idx;

	if (!i3c_ibi_has_payload(target)) {
		LOG_INST_ERR(cfg->log, "i3cm only supports ibi with payload");
		return -ENOTSUP;
	}

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	if (data->ibi.num_addr >= ARRAY_SIZE(data->ibi.addr)) {
		LOG_INST_ERR(cfg->log, "no more free space in the ibi list");
		return -ENOMEM;
	}

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (data->ibi.addr[idx] == target->dynamic_addr) {
			LOG_INST_ERR(cfg->log, "selected target is already in the ibi list");
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
			LOG_INST_ERR(cfg->log, "cannot support more ibis");
			return -ENOTSUP;
		}
	} else {
		idx = 0;
	}

	LOG_INST_DBG(cfg->log, "ibi enabling for 0x%x (bcr 0x%x)", target->dynamic_addr,
		     target->bcr);

	/* enable target ibi event by enec command */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_INST_ERR(cfg->log, "failed to send ibi enec for 0x%x(%d)", target->dynamic_addr,
			     ret);
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
	const struct it51xxx_i3cm_config *cfg = dev->config;
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
		LOG_INST_ERR(cfg->log, "selected target is not in ibi list");
		return -ENODEV;
	}

	data->ibi.addr[idx] = 0U;
	data->ibi.num_addr -= 1U;

	if (data->ibi.num_addr == 0U) {
		it51xxx_enable_standby_state(dev, true);
	}

	LOG_INST_DBG(cfg->log, "ibi disabling for 0x%x (bcr 0x%x)", target->dynamic_addr,
		     target->bcr);

	/* disable target ibi event by disec command */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_INST_ERR(cfg->log, "failed to send ibi disec for 0x%x(%d)",
			     target->dynamic_addr, ret);
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
		LOG_INST_ERR(cfg->log, "failed to apply pinctrl, ret %d", ret);
		return ret;
	}

	ctrl_config->is_secondary = false;
	ctrl_config->supported_hdr = 0x0;

	k_sem_init(&data->msg_sem, 0, 1);
	k_mutex_init(&data->lock);

	if (i3c_bus_mode(&cfg->common.dev_list) != I3C_BUS_MODE_PURE) {
		LOG_INST_ERR(cfg->log, "only support pure mode currently");
		return -ENOTSUP;
	}

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		LOG_INST_ERR(cfg->log, "failed to init slots, ret %d", ret);
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
		LOG_INST_ERR(cfg->log, "invalid dlm size(%d)", CONFIG_I3CM_IT51XXX_DLM_SIZE);
		return -EINVAL;
	};

	/* set i3cm channel selection */
	reg_val &= ~I3CM_CHANNEL_SELECT_MASK;
	LOG_INST_DBG(cfg->log, "channel %d is selected", cfg->io_channel);
	reg_val |= FIELD_PREP(I3CM_CHANNEL_SELECT_MASK, cfg->io_channel);

	/* select 4k pull-up resistor and enable i3c engine*/
	reg_val |= (I3CM_PULL_UP_RESISTOR | I3CM_ENABLE);
	sys_write8(reg_val, cfg->base + I3CM50_CONTROL_3);

	LOG_INST_DBG(cfg->log, "dlm base address 0x%x", (uint32_t)&data->dlm_data);
	sys_write8(FIELD_GET(GENMASK(17, 16), (uint32_t)&data->dlm_data),
		   cfg->base + I3CM53_DLM_BASE_ADDRESS_HB);
	sys_write8(BYTE_1((uint32_t)&data->dlm_data), cfg->base + I3CM52_DLM_BASE_ADDRESS_LB);

	ret = it51xxx_set_frequency(dev);
	if (ret) {
		return ret;
	}

	data->is_initialized = true;

#ifdef CONFIG_I3C_USE_IBI
	data->ibi_hj_response = true;
#endif

	if (cfg->common.dev_list.num_i3c > 0) {
		ret = i3c_bus_init(dev, &cfg->common.dev_list);
		if (ret != 0) {
			/* Perhaps the target device is offline. Avoid returning
			 * an error code to allow the application layer to
			 * reinitialize by sending ccc.
			 */
			LOG_INST_ERR(cfg->log, "failed to init i3c bus, ret %d", ret);
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
		LOG_INST_ERR(cfg->log, "daa: rx count (%d) not as expected", rx_count);
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

	/* find the device in the device list */
	ret = i3c_dev_list_daa_addr_helper(&data->common.attached_dev.addr_slots,
					   &cfg->common.dev_list, pid, false, false, &target,
					   &dyn_addr);
	if (ret != 0) {
		LOG_INST_ERR(cfg->log, "no dynamic address could be assigned to target");
		return -EINVAL;
	}

	sys_write8(I3CM_TARGET_ADDRESS(dyn_addr), cfg->base + I3CM02_TARGET_ADDRESS);

	if (target != NULL) {
		target->dynamic_addr = dyn_addr;
		target->bcr = data->dlm_data.rx_data[6];
		target->dcr = data->dlm_data.rx_data[7];
	} else {
		LOG_INST_INF(
			cfg->log,
			"pid 0x%04x%08x is not in registered device list, given dynamic address "
			"0x%x",
			vendor_id, part_no, dyn_addr);
	}

	/* mark the address as used */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

	/* mark the static address as free */
	if ((target != NULL) && (target->static_addr != 0) && (dyn_addr != target->static_addr)) {
		i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
					 target->static_addr);
	}

	return 0;
}

static int it51xxx_direct_ccc_next_xfer(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_ccc_target_payload *tgt_payload = data->ccc_msgs.payload->targets.payloads;
	uint8_t cycle_type;
	bool more_transfer;
	bool is_read = tgt_payload[data->ccc_msgs.target_idx].rnw == 1U;
	int ret;

	it51xxx_direct_ccc_xfer_end(dev);

	/* start next transfer */
	data->ccc_msgs.target_idx++;
	if (is_read) {
		ret = it51xxx_set_tx_rx_length(dev, 0,
					       tgt_payload[data->ccc_msgs.target_idx].data_len);
		if (ret) {
			return ret;
		}
		cycle_type = PRIVATE_READ_TRANSFER;
	} else {
		ret = it51xxx_set_tx_rx_length(dev, tgt_payload[data->ccc_msgs.target_idx].data_len,
					       0);
		if (ret) {
			return ret;
		}
		memcpy(data->dlm_data.tx_data, tgt_payload[data->ccc_msgs.target_idx].data,
		       tgt_payload[data->ccc_msgs.target_idx].data_len);
		cycle_type = PRIVATE_WRITE_TRANSFER;
	}
	more_transfer =
		(data->ccc_msgs.target_idx == data->ccc_msgs.payload->targets.num_targets - 1)
			? false
			: true;
	it51xxx_set_op_type(dev, cycle_type, more_transfer, false);
	sys_write8(I3CM_TARGET_ADDRESS(tgt_payload[data->ccc_msgs.target_idx].addr),
		   cfg->base + I3CM02_TARGET_ADDRESS);

	return 0;
}

static int it51xxx_private_next_xfer(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *i3c_msgs = data->curr_msg.i3c_msgs;
	struct i2c_msg *i2c_msgs = data->curr_msg.i2c_msgs;
	bool is_write, next_is_write, next_is_restart;
	int ret;

	if (it51xxx_curr_msg_is_i3c(dev)) {
		is_write = ((i3c_msgs[data->curr_msg.curr_idx].flags & I3C_MSG_RW_MASK) ==
			    I3C_MSG_WRITE);
		next_is_write = ((i3c_msgs[data->curr_msg.curr_idx + 1].flags & I3C_MSG_RW_MASK) ==
				 I3C_MSG_WRITE);
		next_is_restart = ((i3c_msgs[data->curr_msg.curr_idx + 1].flags &
				    I3C_MSG_RESTART) == I3C_MSG_RESTART);
	} else {
		is_write = ((i2c_msgs[data->curr_msg.curr_idx].flags & I2C_MSG_RW_MASK) ==
			    I2C_MSG_WRITE);
		next_is_write = ((i2c_msgs[data->curr_msg.curr_idx + 1].flags & I2C_MSG_RW_MASK) ==
				 I2C_MSG_WRITE);
		next_is_restart = ((i2c_msgs[data->curr_msg.curr_idx + 1].flags &
				    I2C_MSG_RESTART) == I2C_MSG_RESTART);
	}

	if (is_write && next_is_write && !next_is_restart) {
		data->curr_msg.curr_idx++;
	} else {
		LOG_INST_ERR(cfg->log, "unknown next private xfer message");
		return -EINVAL;
	}

	/* prepare the next transfer */
	ret = it51xxx_prepare_priv_xfer(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static void it51xxx_private_xfer_end(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	struct i3c_msg *i3c_msgs = data->curr_msg.i3c_msgs;
	struct i2c_msg *i2c_msgs = data->curr_msg.i2c_msgs;
	size_t data_count;
	uint8_t curr_msg_idx = data->curr_msg.curr_idx;

	if (it51xxx_curr_msg_is_i3c(dev)) {
		if ((i3c_msgs[curr_msg_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE) {
			i3c_msgs[curr_msg_idx].num_xfer = i3c_msgs[curr_msg_idx].len;
		}

		if ((i3c_msgs[curr_msg_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
			data_count = it51xxx_get_received_data_count(dev);
			i3c_msgs[curr_msg_idx].num_xfer = data_count;
			memcpy(i3c_msgs[curr_msg_idx].buf, data->dlm_data.rx_data,
			       MIN(i3c_msgs[curr_msg_idx].len, data_count));
			LOG_INST_DBG(cfg->log, "i3c: private rx %d bytes", data_count);
			LOG_HEXDUMP_DBG(i3c_msgs[curr_msg_idx].buf, i3c_msgs[curr_msg_idx].len,
					"i3c: private xfer rx:");
		}

		if (curr_msg_idx != data->curr_msg.num_msgs - 1) {
			if ((i3c_msgs[curr_msg_idx].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE &&
			    i3c_msgs[curr_msg_idx + 1].flags & I3C_MSG_READ) {
				data_count = it51xxx_get_received_data_count(dev);
				i3c_msgs[curr_msg_idx + 1].num_xfer = data_count;
				memcpy(i3c_msgs[curr_msg_idx + 1].buf, data->dlm_data.rx_data,
				       MIN(i3c_msgs[curr_msg_idx + 1].len, data_count));
				LOG_INST_DBG(cfg->log, "i3c: private tx-then-rx %d bytes",
					     data_count);
				LOG_HEXDUMP_DBG(i3c_msgs[curr_msg_idx + 1].buf,
						i3c_msgs[curr_msg_idx + 1].len,
						"i3c: private xfer tx-then-rx:");
			}
		}
	} else {
		if ((i2c_msgs[curr_msg_idx].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			data_count = it51xxx_get_received_data_count(dev);
			memcpy(i2c_msgs[curr_msg_idx].buf, data->dlm_data.rx_data,
			       MIN(i2c_msgs[curr_msg_idx].len, data_count));
			LOG_INST_DBG(cfg->log, "i2c: private rx %d bytes", data_count);
			LOG_HEXDUMP_DBG(i2c_msgs[curr_msg_idx].buf, i2c_msgs[curr_msg_idx].len,
					"i2c: private xfer rx:");
		}

		if (curr_msg_idx != data->curr_msg.num_msgs - 1) {
			if ((i2c_msgs[curr_msg_idx].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE &&
			    i2c_msgs[curr_msg_idx + 1].flags & I2C_MSG_READ) {
				data_count = it51xxx_get_received_data_count(dev);
				memcpy(i2c_msgs[curr_msg_idx + 1].buf, data->dlm_data.rx_data,
				       MIN(i2c_msgs[curr_msg_idx + 1].len, data_count));
				LOG_INST_DBG(cfg->log, "i2c: private tx-then-rx %d bytes",
					     data_count);
				LOG_HEXDUMP_DBG(i2c_msgs[curr_msg_idx + 1].buf,
						i2c_msgs[curr_msg_idx + 1].len,
						"i2c: private xfer tx-then-rx:");
			}
		}
	}
}

#ifdef CONFIG_I3C_USE_IBI
static void it51xxx_process_ibi_payload(const struct device *dev)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;
	size_t payload_sz = 0;
	struct i3c_device_desc *target = i3c_dev_list_i3c_addr_find(dev, data->ibi_target_addr);

	if (i3c_ibi_has_payload(target)) {
		payload_sz = it51xxx_get_received_data_count(dev);
		if (payload_sz == 0) {
			/* wrong ibi transaction due to missing payload.
			 * a 100us timeout on the targe side may cause this
			 * situation.
			 */
			return;
		}

		if (payload_sz > CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE) {
			LOG_INST_WRN(cfg->log, "ibi payloads(%d) is too much", payload_sz);
		}
	}

	if (i3c_ibi_work_enqueue_target_irq(target, data->dlm_data.rx_data,
					    MIN(payload_sz, CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE)) !=
	    0) {
		LOG_INST_ERR(cfg->log, "failed to enqueue tir work");
	}
}
#endif /* CONFIG_I3C_USE_IBI */

static inline void it51xxx_check_error(const struct device *dev, const uint8_t int_status)
{
	const struct it51xxx_i3cm_config *cfg = dev->config;
	struct it51xxx_i3cm_data *data = dev->data;

	if (int_status & PARITY_ERROR) {
		LOG_INST_ERR(cfg->log, "isr: transaction(%d) parity error", data->msg_state);
		data->msg_state = IT51XXX_I3CM_MSG_ERROR;
		sys_write8(PARITY_ERROR, cfg->base + I3CM01_STATUS);
	}

	if (int_status & CRC5_ERROR) {
		LOG_INST_ERR(cfg->log, "isr: transaction(%d) crc5 error", data->msg_state);
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
		LOG_INST_DBG(cfg->log,
			     "i3cm interrupt(0x%x) occurs before initialization was complete",
			     int_status);
	}

	it51xxx_check_error(dev, int_status);

	if (int_status & IBI_INTERRUPT) {
		LOG_INST_DBG(cfg->log, "isr: ibi interrupt is detected");

		data->msg_state = bus_is_idle(dev) ? IT51XXX_I3CM_MSG_IBI : IT51XXX_I3CM_MSG_ABORT;
#ifdef CONFIG_I3C_USE_IBI
		uint8_t ibi_value, ibi_address;

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
					LOG_INST_ERR(cfg->log, "failed to enqueue hot-join work");
				}
			}
		} else {
			it51xxx_accept_ibi(dev, false);
			sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
			LOG_INST_ERR(cfg->log, "unsupported controller role request");
		}
#else
		LOG_INST_ERR(cfg->log, "isr: Kconfig I3C_USE_IBI is disabled");
		it51xxx_accept_ibi(dev, false);
		sys_write8(IBI_INTERRUPT, cfg->base + I3CM01_STATUS);
#endif /* CONFIG_I3C_USE_IBI */
	}

	if (int_status & TRANSFER_END) {
		LOG_INST_DBG(cfg->log, "isr: end transfer is detected");
		/* clear tx and rx length */
		it51xxx_set_tx_rx_length(dev, 0, 0);
		if (int_status & TARGET_NACK) {
			LOG_INST_DBG(cfg->log, "isr: target nack is detected");
			if (data->msg_state == IT51XXX_I3CM_MSG_DAA) {
				LOG_INST_DBG(cfg->log, "isr: no target should be assigned address");
			} else {
				LOG_INST_ERR(cfg->log, "isr: no target responses");
				data->msg_state = IT51XXX_I3CM_MSG_ERROR;
			}
		}

		switch (data->msg_state) {
		case IT51XXX_I3CM_MSG_ABORT:
			LOG_INST_INF(cfg->log, "isr: transfer was aborted due to ibi transaction");
			data->transfer_is_aborted = true;
			__fallthrough;
		case IT51XXX_I3CM_MSG_IBI:
#ifdef CONFIG_I3C_USE_IBI
			if (data->ibi_target_addr) {
				it51xxx_process_ibi_payload(dev);
				data->ibi_target_addr = 0x0;
			}
#endif /* CONFIG_I3C_USE_IBI */
			break;
		case IT51XXX_I3CM_MSG_BROADCAST_CCC:
			if (data->ccc_msgs.payload->ccc.data_len > 0) {
				data->ccc_msgs.payload->ccc.num_xfer =
					data->ccc_msgs.payload->ccc.data_len;
			}
			break;
		case IT51XXX_I3CM_MSG_PRIVATE_XFER:
			it51xxx_private_xfer_end(dev);
			break;
		case IT51XXX_I3CM_MSG_DIRECT_CCC:
			data->ccc_msgs.payload->ccc.num_xfer = data->ccc_msgs.payload->ccc.data_len;
			it51xxx_direct_ccc_xfer_end(dev);
			data->ccc_msgs.target_idx = 0;
			break;
		case IT51XXX_I3CM_MSG_ERROR:
			LOG_INST_ERR(cfg->log, "isr: message status error");
			data->error_is_detected = true;
			break;
		case IT51XXX_I3CM_MSG_DAA:
			LOG_INST_DBG(cfg->log, "isr: daa finished");
			break;
		case IT51XXX_I3CM_MSG_IDLE:
			LOG_INST_WRN(cfg->log, "isr: end transfer occurs but bus is in idle");
			break;
		default:
			LOG_INST_ERR(cfg->log, "isr: unknown message status(%d)", data->msg_state);
			break;
		}

		if (data->msg_state != IT51XXX_I3CM_MSG_IBI) {
			k_sem_give(&data->msg_sem);
		}

		data->msg_state = IT51XXX_I3CM_MSG_IDLE;
		sys_write8(TARGET_NACK | TRANSFER_END, cfg->base + I3CM01_STATUS);
	}

	if (int_status & NEXT_TRANSFER) {
		int ret = 0;

		LOG_INST_DBG(cfg->log, "isr: next transfer is detected");
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
			LOG_INST_ERR(cfg->log, "isr: next transfer, unknown msg status(0x%x)",
				     data->msg_state);
			break;
		};

		if (ret) {
			data->msg_state = IT51XXX_I3CM_MSG_ERROR;
		}
		sys_write8(NEXT_TRANSFER, cfg->base + I3CM01_STATUS);
	}
}

static DEVICE_API(i3c, it51xxx_i3cm_api) = {
	.i2c_api.transfer = it51xxx_i3cm_i2c_api_transfer,
#ifdef CONFIG_I2C_RTIO
	.i2c_api.iodev_submit = i2c_iodev_submit_fallback,
#endif /* CONFIG_I2C_RTIO */

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
#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif /* CONFIG_I3C_RTIO */
};

#define IT51XXX_I3CM_INIT(n)                                                                       \
	LOG_INSTANCE_REGISTER(DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(n)), n,                          \
			      CONFIG_I3C_IT51XXX_LOG_LEVEL);                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct i3c_device_desc it51xxx_i3cm_device_array_##n[] =                            \
		I3C_DEVICE_ARRAY_DT_INST(n);                                                       \
	static struct i3c_i2c_device_desc it51xxx_i3cm_i2c_device_array_##n[] =                    \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);                                                   \
	static void it51xxx_i3cm_config_func_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, it51xxx_i3cm_isr, DEVICE_DT_INST_GET(n), 0);       \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static const struct it51xxx_i3cm_config i3c_config_##n = {                                 \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = it51xxx_i3cm_config_func_##n,                                   \
		.irq_num = DT_INST_IRQN(n),                                                        \
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
		.clocks.i2c_scl_hddat = DT_INST_PROP_OR(n, i2c_scl_hddat, 0),                      \
		LOG_INSTANCE_PTR_INIT(log, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(n)), n)};           \
	static struct it51xxx_i3cm_data i3c_data_##n = {                                           \
		.common.ctrl_config.scl.i3c = DT_INST_PROP_OR(n, i3c_scl_hz, 0),                   \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, it51xxx_i3cm_init, NULL, &i3c_data_##n, &i3c_config_##n,          \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,                    \
			      &it51xxx_i3cm_api);

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_I3CM_INIT)
