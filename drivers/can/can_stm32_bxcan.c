/*
 * Copyright (c) 2018 Alexander Wachter
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include soc.h prior to Zephyr CAN headers to pull in HAL fixups */
#include <soc.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_stm32, CONFIG_CAN_LOG_LEVEL);

#define CAN_INIT_TIMEOUT  (10 * (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC))

#define DT_DRV_COMPAT st_stm32_bxcan

#define CAN_STM32_NUM_FILTER_BANKS (14)
#define CAN_STM32_MAX_FILTER_ID \
	(CONFIG_CAN_MAX_EXT_ID_FILTER + CONFIG_CAN_MAX_STD_ID_FILTER * 2)

#define CAN_STM32_FIRX_STD_IDE_POS   (3U)
#define CAN_STM32_FIRX_STD_RTR_POS   (4U)
#define CAN_STM32_FIRX_STD_ID_POS    (5U)

#define CAN_STM32_FIRX_EXT_IDE_POS    (2U)
#define CAN_STM32_FIRX_EXT_RTR_POS    (1U)
#define CAN_STM32_FIRX_EXT_STD_ID_POS (21U)
#define CAN_STM32_FIRX_EXT_EXT_ID_POS (3U)

#if (CONFIG_CAN_MAX_STD_ID_FILTER + CONFIG_CAN_MAX_EXT_ID_FILTER * 2) > \
						(CAN_STM32_NUM_FILTER_BANKS * 2)
#error Number of configured filters exceeds available filter bank slots.
#endif

struct can_stm32_mailbox {
	can_tx_callback_t tx_callback;
	void *callback_arg;
};

struct can_stm32_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	struct can_stm32_mailbox mb0;
	struct can_stm32_mailbox mb1;
	struct can_stm32_mailbox mb2;
	can_rx_callback_t rx_cb_std[CONFIG_CAN_MAX_STD_ID_FILTER];
	can_rx_callback_t rx_cb_ext[CONFIG_CAN_MAX_EXT_ID_FILTER];
	void *cb_arg_std[CONFIG_CAN_MAX_STD_ID_FILTER];
	void *cb_arg_ext[CONFIG_CAN_MAX_EXT_ID_FILTER];
	enum can_state state;
};

struct can_stm32_config {
	const struct can_driver_config common;
	CAN_TypeDef *can;   /*!< CAN Registers*/
	CAN_TypeDef *master_can;   /*!< CAN Registers for shared filter */
	struct stm32_pclken pclken;
	void (*config_irq)(CAN_TypeDef *can);
	const struct pinctrl_dev_config *pcfg;
};

/*
 * Mutex to prevent simultaneous access to filter registers shared between CAN1
 * and CAN2.
 */
static struct k_mutex filter_mutex;

static void can_stm32_signal_tx_complete(const struct device *dev, struct can_stm32_mailbox *mb,
					 int status)
{
	can_tx_callback_t callback = mb->tx_callback;

	if (callback != NULL) {
		callback(dev, status, mb->callback_arg);
		mb->tx_callback = NULL;
	}
}

static void can_stm32_rx_fifo_pop(CAN_FIFOMailBox_TypeDef *mbox, struct can_frame *frame)
{
	memset(frame, 0, sizeof(*frame));

	if (mbox->RIR & CAN_RI0R_IDE) {
		frame->id = mbox->RIR >> CAN_RI0R_EXID_Pos;
		frame->flags |= CAN_FRAME_IDE;
	} else {
		frame->id = mbox->RIR >> CAN_RI0R_STID_Pos;
	}

	if ((mbox->RIR & CAN_RI0R_RTR) != 0) {
		frame->flags |= CAN_FRAME_RTR;
	} else {
		frame->data_32[0] = mbox->RDLR;
		frame->data_32[1] = mbox->RDHR;
	}

	frame->dlc = mbox->RDTR & (CAN_RDT0R_DLC >> CAN_RDT0R_DLC_Pos);
#ifdef CONFIG_CAN_RX_TIMESTAMP
	frame->timestamp = ((mbox->RDTR & CAN_RDT0R_TIME) >> CAN_RDT0R_TIME_Pos);
#endif
}

static inline void can_stm32_rx_isr_handler(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	CAN_FIFOMailBox_TypeDef *mbox;
	int filter_id, index;
	struct can_frame frame;
	can_rx_callback_t callback = NULL;
	void *cb_arg;

	while (can->RF0R & CAN_RF0R_FMP0) {
		mbox = &can->sFIFOMailBox[0];
		filter_id = ((mbox->RDTR & CAN_RDT0R_FMI) >> CAN_RDT0R_FMI_Pos);

		LOG_DBG("Message on filter_id %d", filter_id);

		can_stm32_rx_fifo_pop(mbox, &frame);

		if (filter_id < CONFIG_CAN_MAX_EXT_ID_FILTER) {
			callback = data->rx_cb_ext[filter_id];
			cb_arg = data->cb_arg_ext[filter_id];
		} else if (filter_id < CAN_STM32_MAX_FILTER_ID) {
			index = filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER;
			callback = data->rx_cb_std[index];
			cb_arg = data->cb_arg_std[index];
		}

		if (callback) {
			callback(dev, &frame, cb_arg);
		}

		/* Release message */
		can->RF0R |= CAN_RF0R_RFOM0;
	}

	if (can->RF0R & CAN_RF0R_FOVR0) {
		LOG_ERR("RX FIFO Overflow");
		CAN_STATS_RX_OVERRUN_INC(dev);
	}
}

static int can_stm32_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if (can->ESR & CAN_ESR_BOFF) {
			*state = CAN_STATE_BUS_OFF;
		} else if (can->ESR & CAN_ESR_EPVF) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if (can->ESR & CAN_ESR_EWGF) {
			*state = CAN_STATE_ERROR_WARNING;
		} else {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt =
			((can->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
		err_cnt->rx_err_cnt =
			((can->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
	}

	return 0;
}

static inline void can_stm32_bus_state_change_isr(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	const can_state_change_callback_t cb = data->common.state_change_cb;
	void *state_change_cb_data = data->common.state_change_cb_user_data;

#ifdef CONFIG_CAN_STATS
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	switch (can->ESR & CAN_ESR_LEC) {
	case (CAN_ESR_LEC_0):
		CAN_STATS_STUFF_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_1):
		CAN_STATS_FORM_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_1 | CAN_ESR_LEC_0):
		CAN_STATS_ACK_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2):
		CAN_STATS_BIT1_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2 | CAN_ESR_LEC_0):
		CAN_STATS_BIT0_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2 | CAN_ESR_LEC_1):
		CAN_STATS_CRC_ERROR_INC(dev);
		break;
	default:
		break;
	}

	/* Clear last error code flag */
	can->ESR |= CAN_ESR_LEC;
#endif /* CONFIG_CAN_STATS */

	(void)can_stm32_get_state(dev, &state, &err_cnt);

	if (state != data->state) {
		data->state = state;

		if (cb != NULL) {
			cb(dev, state, err_cnt, state_change_cb_data);
		}
	}
}

static inline void can_stm32_tx_isr_handler(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	uint32_t bus_off;
	int status;

	bus_off = can->ESR & CAN_ESR_BOFF;

	if ((can->TSR & CAN_TSR_RQCP0) | bus_off) {
		status = can->TSR & CAN_TSR_TXOK0 ? 0  :
			 can->TSR & CAN_TSR_TERR0 ? -EIO :
			 can->TSR & CAN_TSR_ALST0 ? -EBUSY :
					  bus_off ? -ENETUNREACH :
						    -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP0;
		can_stm32_signal_tx_complete(dev, &data->mb0, status);
	}

	if ((can->TSR & CAN_TSR_RQCP1) | bus_off) {
		status = can->TSR & CAN_TSR_TXOK1 ? 0  :
			 can->TSR & CAN_TSR_TERR1 ? -EIO :
			 can->TSR & CAN_TSR_ALST1 ? -EBUSY :
			 bus_off                  ? -ENETUNREACH :
						    -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP1;
		can_stm32_signal_tx_complete(dev, &data->mb1, status);
	}

	if ((can->TSR & CAN_TSR_RQCP2) | bus_off) {
		status = can->TSR & CAN_TSR_TXOK2 ? 0  :
			 can->TSR & CAN_TSR_TERR2 ? -EIO :
			 can->TSR & CAN_TSR_ALST2 ? -EBUSY :
			 bus_off                  ? -ENETUNREACH :
						    -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP2;
		can_stm32_signal_tx_complete(dev, &data->mb2, status);
	}

	if (can->TSR & CAN_TSR_TME) {
		k_sem_give(&data->tx_int_sem);
	}
}

#ifdef CONFIG_SOC_SERIES_STM32F0X

static void can_stm32_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	can_stm32_tx_isr_handler(dev);
	can_stm32_rx_isr_handler(dev);

	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#else

static void can_stm32_rx_isr(const struct device *dev)
{
	can_stm32_rx_isr_handler(dev);
}

static void can_stm32_tx_isr(const struct device *dev)
{
	can_stm32_tx_isr_handler(dev);
}

static void can_stm32_state_change_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	/* Signal bus-off to waiting tx */
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_tx_isr_handler(dev);
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#endif

static int can_stm32_enter_init_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR |= CAN_MCR_INRQ;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_INAK) == 0U) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			can->MCR &= ~CAN_MCR_INRQ;
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_stm32_leave_init_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR &= ~CAN_MCR_INRQ;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_INAK) != 0U) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_stm32_leave_sleep_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR &= ~CAN_MCR_SLEEP;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_SLAK) != 0) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_stm32_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

static int can_stm32_start(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_enable(cfg->common.phy, data->common.mode);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	CAN_STATS_RESET(dev);

	ret = can_stm32_leave_init_mode(can);
	if (ret < 0) {
		LOG_ERR("Failed to leave init mode");

		if (cfg->common.phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(cfg->common.phy);
		}

		ret = -EIO;
		goto unlock;
	}

	data->common.started = true;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_stm32_stop(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	ret = can_stm32_enter_init_mode(can);
	if (ret < 0) {
		LOG_ERR("Failed to enter init mode");
		ret = -EIO;
		goto unlock;
	}

	/* Abort any pending transmissions */
	can_stm32_signal_tx_complete(dev, &data->mb0, -ENETDOWN);
	can_stm32_signal_tx_complete(dev, &data->mb1, -ENETDOWN);
	can_stm32_signal_tx_complete(dev, &data->mb2, -ENETDOWN);
	can->TSR |= CAN_TSR_ABRQ2 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ0;

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_disable(cfg->common.phy);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	data->common.started = false;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_stm32_set_mode(const struct device *dev, can_mode_t mode)
{
	can_mode_t supported = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = dev->data;

	LOG_DBG("Set mode %d", mode);

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		supported |= CAN_MODE_MANUAL_RECOVERY;
	}

	if ((mode & ~(supported)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* Loopback mode */
		can->BTR |= CAN_BTR_LBKM;
	} else {
		can->BTR &= ~CAN_BTR_LBKM;
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		/* Silent mode */
		can->BTR |= CAN_BTR_SILM;
	} else {
		can->BTR &= ~CAN_BTR_SILM;
	}

	if ((mode & CAN_MODE_ONE_SHOT) != 0) {
		/* No automatic retransmission */
		can->MCR |= CAN_MCR_NART;
	} else {
		can->MCR &= ~CAN_MCR_NART;
	}

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		if ((mode & CAN_MODE_MANUAL_RECOVERY) != 0) {
			/* No automatic recovery from bus-off */
			can->MCR &= ~CAN_MCR_ABOM;
		} else {
			can->MCR |= CAN_MCR_ABOM;
		}
	}

	data->common.mode = mode;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_stm32_set_timing(const struct device *dev,
				const struct can_timing *timing)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = dev->data;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		k_mutex_unlock(&data->inst_mutex);
		return -EBUSY;
	}

	can->BTR = (can->BTR & ~(CAN_BTR_SJW_Msk | CAN_BTR_BRP_Msk |
				 CAN_BTR_TS1_Msk | CAN_BTR_TS2_Msk)) |
	     (((timing->sjw        - 1) << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk) |
	     (((timing->phase_seg1 - 1) << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk) |
	     (((timing->phase_seg2 - 1) << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk) |
	     (((timing->prescaler  - 1) << CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_stm32_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_stm32_config *cfg = dev->config;
	const struct device *clock;
	int ret;

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = clock_control_get_rate(clock,
				     (clock_control_subsys_t) &cfg->pclken,
				     rate);
	if (ret != 0) {
		LOG_ERR("Failed call clock_control_get_rate: return [%d]", ret);
		return -EIO;
	}

	return 0;
}

static int can_stm32_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);

	if (ide) {
		return CONFIG_CAN_MAX_EXT_ID_FILTER;
	} else {
		return CONFIG_CAN_MAX_STD_ID_FILTER;
	}
}

static int can_stm32_init(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	struct can_timing timing = { 0 };
	const struct device *clock;
	uint32_t bank_offset;
	int ret;

	k_mutex_init(&filter_mutex);
	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, 0, 1);

	if (cfg->common.phy != NULL) {
		if (!device_is_ready(cfg->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	if (!device_is_ready(clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clock, (clock_control_subsys_t) &cfg->pclken);
	if (ret != 0) {
		LOG_ERR("HAL_CAN_Init clock control on failed: %d", ret);
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32_leave_sleep_mode(can);
	if (ret) {
		LOG_ERR("Failed to exit sleep mode");
		return ret;
	}

	ret = can_stm32_enter_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		return ret;
	}

	/* configure scale of filter banks < CONFIG_CAN_MAX_EXT_ID_FILTER for ext ids */
	bank_offset = (cfg->can == cfg->master_can) ? 0 : CAN_STM32_NUM_FILTER_BANKS;
	cfg->master_can->FMR |= CAN_FMR_FINIT;
	cfg->master_can->FS1R |= ((1U << CONFIG_CAN_MAX_EXT_ID_FILTER) - 1) << bank_offset;
	cfg->master_can->FMR &= ~CAN_FMR_FINIT;

	can->MCR &= ~CAN_MCR_TTCM & ~CAN_MCR_ABOM & ~CAN_MCR_AWUM &
		    ~CAN_MCR_NART & ~CAN_MCR_RFLM & ~CAN_MCR_TXFP;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	can->MCR |= CAN_MCR_TTCM;
#endif

	/* Enable automatic bus-off recovery */
	can->MCR |= CAN_MCR_ABOM;

	ret = can_calc_timing(dev, &timing, cfg->common.bus_speed,
			      cfg->common.sample_point);
	if (ret == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}
	LOG_DBG("Presc: %d, TS1: %d, TS2: %d",
		timing.prescaler, timing.phase_seg1, timing.phase_seg2);
	LOG_DBG("Sample-point err : %d", ret);

	ret = can_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}

	ret = can_stm32_set_mode(dev, CAN_MODE_NORMAL);
	if (ret) {
		return ret;
	}

	(void)can_stm32_get_state(dev, &data->state, NULL);

	cfg->config_irq(can);
	can->IER |= CAN_IER_TMEIE;

	return 0;
}

static void can_stm32_set_state_change_callback(const struct device *dev,
						can_state_change_callback_t cb,
						void *user_data)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	data->common.state_change_cb = cb;
	data->common.state_change_cb_user_data = user_data;

	if (cb == NULL) {
		can->IER &= ~(CAN_IER_BOFIE | CAN_IER_EPVIE | CAN_IER_EWGIE);
	} else {
		can->IER |= CAN_IER_BOFIE | CAN_IER_EPVIE | CAN_IER_EWGIE;
	}
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_stm32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	int ret = -EAGAIN;
	int64_t start_time;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	if (!(can->ESR & CAN_ESR_BOFF)) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	ret = can_stm32_enter_init_mode(can);
	if (ret) {
		goto done;
	}

	can_stm32_leave_init_mode(can);

	start_time = k_uptime_ticks();

	while (can->ESR & CAN_ESR_BOFF) {
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			goto done;
		}
	}

	ret = 0;

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static int can_stm32_send(const struct device *dev, const struct can_frame *frame,
			  k_timeout_t timeout, can_tx_callback_t callback,
			  void *user_data)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	uint32_t transmit_status_register = can->TSR;
	CAN_TxMailBox_TypeDef *mailbox = NULL;
	struct can_stm32_mailbox *mb = NULL;

	LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , frame->dlc, dev->name
		    , frame->id
		    , (frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard"
		    , (frame->flags & CAN_FRAME_RTR) != 0 ? "yes" : "no");

	__ASSERT_NO_MSG(callback != NULL);
	__ASSERT(frame->dlc == 0U || frame->data != NULL, "Dataptr is null");

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (can->ESR & CAN_ESR_BOFF) {
		return -ENETUNREACH;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSR_TME)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_DBG("Transmit buffer full");
		if (k_sem_take(&data->tx_int_sem, timeout)) {
			return -EAGAIN;
		}

		k_mutex_lock(&data->inst_mutex, K_FOREVER);
		transmit_status_register = can->TSR;
	}

	if (transmit_status_register & CAN_TSR_TME0) {
		LOG_DBG("Using TX mailbox 0");
		mailbox = &can->sTxMailBox[0];
		mb = &(data->mb0);
	} else if (transmit_status_register & CAN_TSR_TME1) {
		LOG_DBG("Using TX mailbox 1");
		mailbox = &can->sTxMailBox[1];
		mb = &data->mb1;
	} else if (transmit_status_register & CAN_TSR_TME2) {
		LOG_DBG("Using TX mailbox 2");
		mailbox = &can->sTxMailBox[2];
		mb = &data->mb2;
	}

	mb->tx_callback = callback;
	mb->callback_arg = user_data;

	/* mailbox identifier register setup */
	mailbox->TIR &= CAN_TI0R_TXRQ;

	if ((frame->flags & CAN_FRAME_IDE) != 0) {
		mailbox->TIR |= (frame->id << CAN_TI0R_EXID_Pos)
				| CAN_TI0R_IDE;
	} else {
		mailbox->TIR |= (frame->id << CAN_TI0R_STID_Pos);
	}

	if ((frame->flags & CAN_FRAME_RTR) != 0) {
		mailbox->TIR |= CAN_TI1R_RTR;
	} else {
		mailbox->TDLR = frame->data_32[0];
		mailbox->TDHR = frame->data_32[1];
	}

	mailbox->TDTR = (mailbox->TDTR & ~CAN_TDT1R_DLC) |
			((frame->dlc & 0xF) << CAN_TDT1R_DLC_Pos);

	mailbox->TIR |= CAN_TI0R_TXRQ;
	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static void can_stm32_set_filter_bank(int filter_id, CAN_FilterRegister_TypeDef *filter_reg,
				      bool ide, uint32_t id, uint32_t mask)
{
	if (ide) {
		filter_reg->FR1 = id;
		filter_reg->FR2 = mask;
	} else {
		if ((filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER) % 2 == 0) {
			/* even std filter id: first 1/2 bank */
			filter_reg->FR1 = id | (mask << 16);
		} else {
			/* uneven std filter id: first 1/2 bank */
			filter_reg->FR2 = id | (mask << 16);
		}
	}
}

static inline uint32_t can_stm32_filter_to_std_mask(const struct can_filter *filter)
{
	uint32_t rtr_mask = !IS_ENABLED(CONFIG_CAN_ACCEPT_RTR);

	return  (filter->mask << CAN_STM32_FIRX_STD_ID_POS) |
		(rtr_mask << CAN_STM32_FIRX_STD_RTR_POS) |
		(1U << CAN_STM32_FIRX_STD_IDE_POS);
}

static inline uint32_t can_stm32_filter_to_ext_mask(const struct can_filter *filter)
{
	uint32_t rtr_mask = !IS_ENABLED(CONFIG_CAN_ACCEPT_RTR);

	return  (filter->mask << CAN_STM32_FIRX_EXT_EXT_ID_POS) |
		(rtr_mask << CAN_STM32_FIRX_EXT_RTR_POS) |
		(1U << CAN_STM32_FIRX_EXT_IDE_POS);
}

static inline uint32_t can_stm32_filter_to_std_id(const struct can_filter *filter)
{
	return  (filter->id  << CAN_STM32_FIRX_STD_ID_POS);
}

static inline uint32_t can_stm32_filter_to_ext_id(const struct can_filter *filter)
{
	return  (filter->id << CAN_STM32_FIRX_EXT_EXT_ID_POS) |
		(1U << CAN_STM32_FIRX_EXT_IDE_POS);
}

static inline int can_stm32_set_filter(const struct device *dev, const struct can_filter *filter)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->master_can;
	uint32_t mask = 0U;
	uint32_t id = 0U;
	int filter_id = -ENOSPC;
	int bank_offset = 0;
	int bank_num;

	if (cfg->can != cfg->master_can) {
		/* CAN slave instance: start with offset */
		bank_offset = CAN_STM32_NUM_FILTER_BANKS;
	}

	if ((filter->flags & CAN_FILTER_IDE) != 0) {
		for (int i = 0; i < CONFIG_CAN_MAX_EXT_ID_FILTER; i++) {
			if (data->rx_cb_ext[i] == NULL) {
				id = can_stm32_filter_to_ext_id(filter);
				mask = can_stm32_filter_to_ext_mask(filter);
				filter_id = i;
				bank_num = bank_offset + i;
				break;
			}
		}
	} else {
		for (int i = 0; i < CONFIG_CAN_MAX_STD_ID_FILTER; i++) {
			if (data->rx_cb_std[i] == NULL) {
				id = can_stm32_filter_to_std_id(filter);
				mask = can_stm32_filter_to_std_mask(filter);
				filter_id = CONFIG_CAN_MAX_EXT_ID_FILTER + i;
				bank_num = bank_offset + CONFIG_CAN_MAX_EXT_ID_FILTER + i / 2;
				break;
			}
		}
	}

	if (filter_id != -ENOSPC) {
		LOG_DBG("Adding filter_id %d, CAN ID: 0x%x, mask: 0x%x",
			filter_id, filter->id, filter->mask);

		/* set the filter init mode */
		can->FMR |= CAN_FMR_FINIT;

		can_stm32_set_filter_bank(filter_id, &can->sFilterRegister[bank_num],
					  (filter->flags & CAN_FILTER_IDE) != 0,
					  id, mask);

		can->FA1R |= 1U << bank_num;
		can->FMR &= ~(CAN_FMR_FINIT);
	} else {
		LOG_WRN("No free filter left");
	}

	return filter_id;
}


/*
 * This driver uses masked mode for all filters (CAN_FM1R left at reset value
 * 0x00) in order to simplify mapping between filter match index from the FIFOs
 * and array index for the callbacks. All ext ID filters are stored in the
 * banks below CONFIG_CAN_MAX_EXT_ID_FILTER, followed by the std ID filters,
 * which consume only 1/2 bank per filter.
 *
 * The more complicated list mode must be implemented if someone requires more
 * than 28 std ID or 14 ext ID filters.
 *
 * Currently, all filter banks are assigned to FIFO 0 and FIFO 1 is not used.
 */
static int can_stm32_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				   void *cb_arg, const struct can_filter *filter)
{
	struct can_stm32_data *data = dev->data;
	int filter_id;

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&filter_mutex, K_FOREVER);
	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	filter_id = can_stm32_set_filter(dev, filter);
	if (filter_id >= 0) {
		if ((filter->flags & CAN_FILTER_IDE) != 0) {
			data->rx_cb_ext[filter_id] = cb;
			data->cb_arg_ext[filter_id] = cb_arg;
		} else {
			data->rx_cb_std[filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER] = cb;
			data->cb_arg_std[filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER] = cb_arg;
		}
	}

	k_mutex_unlock(&data->inst_mutex);
	k_mutex_unlock(&filter_mutex);

	return filter_id;
}

static void can_stm32_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->master_can;
	bool ide;
	int bank_offset = 0;
	int bank_num;
	bool bank_unused;

	if (filter_id < 0 || filter_id >= CAN_STM32_MAX_FILTER_ID) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	k_mutex_lock(&filter_mutex, K_FOREVER);
	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (cfg->can != cfg->master_can) {
		bank_offset = CAN_STM32_NUM_FILTER_BANKS;
	}

	if (filter_id < CONFIG_CAN_MAX_EXT_ID_FILTER) {
		ide = true;
		bank_num = bank_offset + filter_id;

		data->rx_cb_ext[filter_id] = NULL;
		data->cb_arg_ext[filter_id] = NULL;

		bank_unused = true;
	} else {
		int filter_index = filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER;

		ide = false;
		bank_num = bank_offset + CONFIG_CAN_MAX_EXT_ID_FILTER +
			  (filter_id - CONFIG_CAN_MAX_EXT_ID_FILTER) / 2;

		data->rx_cb_std[filter_index] = NULL;
		data->cb_arg_std[filter_index] = NULL;

		if (filter_index % 2 == 1) {
			bank_unused = data->rx_cb_std[filter_index - 1] == NULL;
		} else if (filter_index + 1 < CONFIG_CAN_MAX_STD_ID_FILTER) {
			bank_unused = data->rx_cb_std[filter_index + 1] == NULL;
		} else {
			bank_unused = true;
		}
	}

	LOG_DBG("Removing filter_id %d, ide %d", filter_id, ide);

	can->FMR |= CAN_FMR_FINIT;

	can_stm32_set_filter_bank(filter_id, &can->sFilterRegister[bank_num],
				  ide, 0, 0xFFFFFFFF);

	if (bank_unused) {
		can->FA1R &= ~(1U << bank_num);
		LOG_DBG("Filter bank %d is unused -> deactivate", bank_num);
	}

	can->FMR &= ~(CAN_FMR_FINIT);

	k_mutex_unlock(&data->inst_mutex);
	k_mutex_unlock(&filter_mutex);
}

static const struct can_driver_api can_api_funcs = {
	.get_capabilities = can_stm32_get_capabilities,
	.start = can_stm32_start,
	.stop = can_stm32_stop,
	.set_mode = can_stm32_set_mode,
	.set_timing = can_stm32_set_timing,
	.send = can_stm32_send,
	.add_rx_filter = can_stm32_add_rx_filter,
	.remove_rx_filter = can_stm32_remove_rx_filter,
	.get_state = can_stm32_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_stm32_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.set_state_change_callback = can_stm32_set_state_change_callback,
	.get_core_clock = can_stm32_get_core_clock,
	.get_max_filters = can_stm32_get_max_filters,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x04,
		.prop_seg = 0x00,
		.phase_seg1 = 0x10,
		.phase_seg2 = 0x08,
		.prescaler = 0x400
	}
};

#ifdef CONFIG_SOC_SERIES_STM32F0X
#define CAN_STM32_IRQ_INST(inst)                                     \
static void config_can_##inst##_irq(CAN_TypeDef *can)                \
{                                                                    \
	IRQ_CONNECT(DT_INST_IRQN(inst),                              \
		    DT_INST_IRQ(inst, priority),                     \
		    can_stm32_isr, DEVICE_DT_INST_GET(inst), 0);     \
	irq_enable(DT_INST_IRQN(inst));                              \
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_FMPIE0 | \
		    CAN_IER_FMPIE1 | CAN_IER_BOFIE;                  \
	if (IS_ENABLED(CONFIG_CAN_STATS)) {                          \
		can->IER |= CAN_IER_LECIE;                           \
	}                                                            \
}
#else
#define CAN_STM32_IRQ_INST(inst)                                     \
static void config_can_##inst##_irq(CAN_TypeDef *can)                \
{                                                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, rx0, irq),             \
		    DT_INST_IRQ_BY_NAME(inst, rx0, priority),        \
		    can_stm32_rx_isr, DEVICE_DT_INST_GET(inst), 0);  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, rx0, irq));             \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, tx, irq),              \
		    DT_INST_IRQ_BY_NAME(inst, tx, priority),         \
		    can_stm32_tx_isr, DEVICE_DT_INST_GET(inst), 0);  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, tx, irq));              \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, sce, irq),             \
		    DT_INST_IRQ_BY_NAME(inst, sce, priority),        \
		    can_stm32_state_change_isr,                      \
		    DEVICE_DT_INST_GET(inst), 0);                    \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, sce, irq));             \
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_FMPIE0 | \
		    CAN_IER_FMPIE1 | CAN_IER_BOFIE;                  \
	if (IS_ENABLED(CONFIG_CAN_STATS)) {                          \
		can->IER |= CAN_IER_LECIE;                           \
	}                                                            \
}
#endif /* CONFIG_SOC_SERIES_STM32F0X */

#define CAN_STM32_CONFIG_INST(inst)                                      \
PINCTRL_DT_INST_DEFINE(inst);                                            \
static const struct can_stm32_config can_stm32_cfg_##inst = {            \
	.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 1000000),          \
	.can = (CAN_TypeDef *)DT_INST_REG_ADDR(inst),                    \
	.master_can = (CAN_TypeDef *)DT_INST_PROP_OR(inst,               \
		master_can_reg, DT_INST_REG_ADDR(inst)),                 \
	.pclken = {                                                      \
		.enr = DT_INST_CLOCKS_CELL(inst, bits),                  \
		.bus = DT_INST_CLOCKS_CELL(inst, bus),                   \
	},                                                               \
	.config_irq = config_can_##inst##_irq,                           \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	                 \
};

#define CAN_STM32_DATA_INST(inst) \
static struct can_stm32_data can_stm32_dev_data_##inst;

#define CAN_STM32_DEFINE_INST(inst)                                      \
CAN_DEVICE_DT_INST_DEFINE(inst, can_stm32_init, NULL,                    \
			  &can_stm32_dev_data_##inst, &can_stm32_cfg_##inst, \
			  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,         \
			  &can_api_funcs);

#define CAN_STM32_INST(inst)      \
CAN_STM32_IRQ_INST(inst)          \
CAN_STM32_CONFIG_INST(inst)       \
CAN_STM32_DATA_INST(inst)         \
CAN_STM32_DEFINE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32_INST)
