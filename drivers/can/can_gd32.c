/*
 * Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <devicetree.h>
#include <drivers/can.h>
#include <drivers/pinctrl.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(can_gd32, CONFIG_CAN_LOG_LEVEL);

#include "can_gd32_filter.h"

#if defined CONFIG_CAN_FD_MODE
#define DT_DRV_COMPAT gd_gd32_can_fd
#else
#define DT_DRV_COMPAT gd_gd32_can
#endif

#define CAN_INIT_TIMEOUT (10 * sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC)

#define SP_IS_SET(inst) DT_INST_NODE_HAS_PROP(inst, sample_point) ||
/* Macro to exclude the sample point algorithm from compilation if not used
 * Without the macro, the algorithm would always waste ROM
 */
#define USE_SP_ALGO 0
/* #define USE_SP_ALGO (DT_INST_FOREACH_STATUS_OKAY(SP_IS_SET) 0) */

#define CAN_TSTAT_TME (CAN_TSTAT_TME0 | CAN_TSTAT_TME1 | CAN_TSTAT_TME2)

/* define some marco for log easily */
#define CAN_GD32_CHECK_LOG(errno, level, ...)                                                      \
	if (errno) {                                                                               \
		Z_LOG(level, __VA_ARGS__);                                                         \
	}
#define CAN_GD32_CHECK_RET(errno, level, ...)                                                      \
	if (errno) {                                                                               \
		Z_LOG(level, __VA_ARGS__);                                                         \
		return errno;                                                                      \
	}
#define CAN_GD32_CHECK_GOTO(errno, tag, level, ...)                                                \
	if (errno) {                                                                               \
		Z_LOG(level, __VA_ARGS__);                                                         \
		goto tag;                                                                          \
	}

#define CAN_GD32_WAIT_REGCHANGE(start_time, cond, error_proc)                                      \
	start_time = k_cycle_get_32();                                                             \
	while (cond) {                                                                             \
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {                            \
			error_proc;                                                                \
			return -EAGAIN;                                                            \
		}                                                                                  \
	}

struct can_gd32_timing {
	uint32_t bus_speed;
	uint16_t sjw;
	uint16_t sample_point;
	uint16_t prop_ts1;
	uint16_t ts2;
};

struct can_gd32_config {
	uint32_t reg;
	uint32_t rcu_periph_clock;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(uint32_t can_periph);

	const struct can_gd32_filter *filter;

	struct can_gd32_timing timing_arbit;
#ifdef CONFIG_CAN_FD_MODE
	struct can_gd32_timing timing_data;
	uint8_t tx_delay_comp_offset;
#endif

	bool fdmode;
	bool esimode;

	bool one_shot;
	// bool is_main_controller;
};

struct can_mailbox {
	can_tx_callback_t tx_callback;
	void *callback_arg;
	struct k_sem tx_int_sem;
	int error;
};

struct can_gd32_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	struct can_mailbox mb0;
	struct can_mailbox mb1;
	struct can_mailbox mb2;
	uint64_t filter_usage;
	can_rx_callback_t rx_cb[CONFIG_CAN_MAX_FILTER];
	void *cb_arg[CONFIG_CAN_MAX_FILTER];
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
};

static int can_enter_init_mode(uint32_t can_periph)
{
	uint32_t start_time;

	CAN_CTL(can_periph) |= CAN_CTL_IWMOD;
	CAN_GD32_WAIT_REGCHANGE(start_time, CAN_STAT_IWS != (CAN_STAT(can_periph) & CAN_STAT_IWS),
				CAN_CTL(can_periph) &= ~CAN_CTL_IWMOD);

	return 0;
}

static int can_leave_init_mode(uint32_t can_periph)
{
	uint32_t start_time;

	CAN_CTL(can_periph) &= ~CAN_CTL_IWMOD;
	CAN_GD32_WAIT_REGCHANGE(start_time, 0 != (CAN_STAT(can_periph) & CAN_STAT_IWS), ;);

	return 0;
}

/* No used for now.prepare for power-off
   static int can_enter_sleep_mode(uint32_t can_periph)
   {
        uint32_t start_time;

        CAN_CTL(can_periph) |= CAN_CTL_SLPWMOD;
        CAN_GD32_WAIT_REGCHANGE(start_time,
                                CAN_STAT_SLPWS != (CAN_STAT(can_periph) & CAN_STAT_SLPWS),
                                CAN_CTL(can_periph) &= ~CAN_CTL_SLPWMOD; );

        return 0;
   }
 */

static int can_leave_sleep_mode(uint32_t can_periph)
{
	uint32_t start_time;

	CAN_CTL(can_periph) &= ~CAN_CTL_SLPWMOD;
	CAN_GD32_WAIT_REGCHANGE(start_time, 0 != (CAN_STAT(can_periph) & CAN_STAT_SLPWS), ;);

	return 0;
}

int can_gd32_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	struct can_gd32_data *data = dev->data;
	int ret;

	const uint8_t can_mode_lut[] = { GD32_CAN_NORMAL_MODE, GD32_CAN_SILENT_MODE,
					 GD32_CAN_LOOPBACK_MODE, GD32_CAN_SILENT_LOOPBACK_MODE };

	LOG_DBG("Set mode %d", mode);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	ret = can_enter_init_mode(can_periph);
	CAN_GD32_CHECK_GOTO(ret, done, LOG_LEVEL_ERR, "Failed to enter init mode (%d)", ret);

	__ASSERT(mode < sizeof(can_mode_lut) / sizeof(uint8_t), "can mode lut overflow.");
	CAN_BT(can_periph) &= ~(BT_MODE(UINT32_MAX));
	CAN_BT(can_periph) |= BT_MODE(can_mode_lut[mode]);

done:
	ret = can_leave_init_mode(can_periph);
	CAN_GD32_CHECK_LOG(ret, LOG_LEVEL_ERR, "Failed to leave init mode");

	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static inline enum can_state can_gd32_get_bus_state(uint32_t can_periph)
{
	return CAN_ERR(can_periph) & CAN_ERR_BOERR ? CAN_BUS_OFF :
	       CAN_ERR(can_periph) & CAN_ERR_PERR  ? CAN_ERROR_PASSIVE :
	       CAN_ERR(can_periph) & CAN_ERR_WERR  ? CAN_ERROR_WARNING :
							   CAN_ERROR_ACTIVE;
}

static int can_gd32_get_state(const struct device *dev, enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;

	if (state != NULL) {
		*state = can_gd32_get_bus_state(can_periph);
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = GET_ERR_TECNT(can_periph);
		err_cnt->rx_err_cnt = GET_ERR_RECNT(can_periph);
	}

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_gd32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_gd32_config *cfg = dev->config;
	struct can_gd32_data *data = dev->data;
	uint32_t can_periph = cfg->reg;
	int ret = -EAGAIN;
	int64_t start_time;

	/* no error, don't need recovery */
	if (!(CAN_ERR(can_periph) & CAN_ERR_BOERR)) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	/* enter init mode then enter normal mode, to recovery bus when CAN_CTL->ABOR == 0 */
	/* when CAN_CTL->ABOR == 1, bus will auto recovery by hardware. */
	ret = can_enter_init_mode(can_periph);
	CAN_GD32_CHECK_GOTO(ret, done, LOG_LEVEL_ERR, "recovery failed");

	ret = can_leave_init_mode(can_periph);
	CAN_GD32_CHECK_GOTO(ret, done, LOG_LEVEL_ERR, "recovery failed");

	start_time = k_uptime_ticks();

	while (CAN_ERR(can_periph) & CAN_ERR_BOERR) {
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
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

int can_gd32_set_timing(const struct device *dev, const struct can_timing *timing,
			const struct can_timing *timing_data)
{
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	struct can_gd32_data *data = dev->data;
	int ret = -EIO;

#if !defined CONFIG_CAN_FD_MODE
	ARG_UNUSED(timing_data);
#endif /* !CONFIG_CAN_FD_MODE*/

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	ret = can_enter_init_mode(can_periph);
	CAN_GD32_CHECK_GOTO(ret, done, LOG_LEVEL_ERR, "Failed to enter init mode");

	/* set arbit timing */
	/* clear timing but hold value of can mode and sjw */
	CAN_BT(can_periph) &= BT_MODE(UINT32_MAX) | BT_SJW(UINT32_MAX);
	/* set timing */
	CAN_BT(can_periph) |= (BT_BS1(timing->phase_seg1 - 1) | BT_BS2(timing->phase_seg2 - 1) |
			       BT_BAUDPSC(timing->prescaler - 1));
	/* set sjw if needed */
	if (timing->sjw != CAN_SJW_NO_CHANGE) {
		CAN_BT(can_periph) =
			(CAN_BT(can_periph) & ~BT_SJW(UINT32_MAX)) | BT_SJW(timing->sjw - 1);
	}

	/* set data timing */
#if defined CONFIG_CAN_FD_MODE
	/*clear timing but hold value of can mode and sjw */
	CAN_DBT(can_periph) &= BT_DSJW(UINT32_MAX);
	/* set timing */
	CAN_DBT(can_periph) |=
		(BT_DBS1(timing_data->phase_seg1 - 1) | BT_DBS2(timing_data->phase_seg2 - 1) |
		 BT_DBAUDPSC(timing_data->prescaler - 1));
	/* set sjw if needed */
	if (timing->sjw != CAN_SJW_NO_CHANGE) {
		CAN_DBT(can_periph) =
			(CAN_DBT(can_periph) & ~BT_DSJW(0xff)) | BT_DSJW(timing_data->sjw - 1);
	}
#endif /* CONFIG_CAN_FD_MODE */

	ret = can_leave_init_mode(can_periph);
	CAN_GD32_CHECK_GOTO(ret, done, LOG_LEVEL_ERR, "Failed to leave init mode");

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static void can_gd32_set_state_change_callback(const struct device *dev,
					       can_state_change_callback_t cb, void *user_data)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;

	data->state_change_cb = cb;
	data->state_change_cb_data = user_data;

	if (cb == NULL) {
		CAN_INTEN(can_periph) &= ~(CAN_INTEN_BOIE | CAN_INTEN_PERRIE | CAN_INTEN_WERRIE);
	} else {
		CAN_INTEN(can_periph) |= CAN_INTEN_BOIE | CAN_INTEN_PERRIE | CAN_INTEN_WERRIE;
	}
}

int can_gd32_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	/* todo: return from DTS -> /cpus/cpu0 -> clock-frequency */
	*rate = 120000000;

	return 0;
}

int can_gd32_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	const struct can_gd32_config *cfg = dev->config;
	return can_gd32_filter_getsize(cfg->filter, id_type);
}

int can_gd32_add_rx_filter(const struct device *dev, can_rx_callback_t cb, void *cb_arg,
			   const struct zcan_filter *filter)
{
	/* todo: add filter. */
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(cb_arg);
	ARG_UNUSED(filter);

	__ASSERT(0, "todo");
	return 0;
}

void can_gd32_remove_rx_filter(const struct device *dev, int filter_id)
{
	/* todo: add filter. */
	ARG_UNUSED(dev);
	ARG_UNUSED(filter_id);

	__ASSERT(0, "todo");
}

int can_gd32_send(const struct device *dev, const struct zcan_frame *frame, k_timeout_t timeout,
		  can_tx_callback_t callback, void *user_data)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	uint32_t transmit_status_register = CAN_TSTAT(can_periph);
	uint8_t mailbox = (uint8_t)CAN_NOMAILBOX;
	struct can_mailbox *mb = NULL;

	LOG_DBG("Sending %d bytes on %s. "
		"Id: 0x%x, "
		"ID type: %s, "
		"Remote Frame: %s",
		frame->dlc, dev->name, frame->id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ? "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "no" : "yes");

	__ASSERT(frame->dlc == 0U || frame->data != NULL, "Dataptr is null");

#if defined CONFIG_CAN_FD_MODE
	/* check dlc correct */
#else
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}
#endif /* CONFIG_CAN_FD_MODE */

	/* check busoff */
	if (CAN_ERR(can_periph) & CAN_ERR_BOERR) {
		return -ENETDOWN;
	}

	/* lock for write send mail box */
	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSTAT_TME)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_DBG("Transmit buffer full");
		if (k_sem_take(&data->tx_int_sem, timeout)) {
			return -EAGAIN;
		}

		k_mutex_lock(&data->inst_mutex, K_FOREVER);
		transmit_status_register = CAN_TSTAT(can_periph);
	}
	if (transmit_status_register & CAN_TSTAT_TME0) {
		LOG_DBG("Using mailbox 0");
		mailbox = CAN_MAILBOX0;
		mb = &(data->mb0);
	} else if (transmit_status_register & CAN_TSTAT_TME1) {
		LOG_DBG("Using mailbox 1");
		mailbox = CAN_MAILBOX1;
		mb = &data->mb1;
	} else if (transmit_status_register & CAN_TSTAT_TME2) {
		LOG_DBG("Using mailbox 2");
		mailbox = CAN_MAILBOX2;
		mb = &data->mb2;
	}

	mb->tx_callback = callback;
	mb->callback_arg = user_data;
	k_sem_reset(&mb->tx_int_sem);

	/* mailbox identifier register setup */
	CAN_TMI(can_periph, mailbox) &= CAN_TMI_TEN;
	if (frame->id_type == CAN_STANDARD_IDENTIFIER) {
		CAN_TMI(can_periph, mailbox) |= (uint32_t)(TMI_SFID(frame->id)) | CAN_FF_STANDARD;
	} else {
		CAN_TMI(can_periph, mailbox) |= (uint32_t)(TMI_EFID(frame->id)) | CAN_FF_EXTENDED;
	}

	if (frame->rtr == CAN_REMOTEREQUEST) {
		CAN_TMI(can_periph, mailbox) |= CAN_FT_REMOTE;
	} else {
		CAN_TMI(can_periph, mailbox) |= CAN_FT_DATA;
	}

	/* todo: support TTC */
	CAN_TMP(can_periph, mailbox) = CAN_TMP_DLENC & frame->dlc;

	if (frame->fd) {
#if defined CONFIG_CAN_FD_MODE
		CAN_TMP(can_periph, mailbox) |= CAN_TMP_FDF;
		if (frame->brs) {
			CAN_TMP(can_periph, mailbox) |= CAN_TMP_BRS;
		}
		/* todo: add support software esi */
#else
		LOG_ERR("Device Not Supported.");
		return -ENOTSUP;
#endif /* CONFIG_CAN_FD_MODE */
	}

	if (frame->fd) {
#if defined CONFIG_CAN_FD_MODE
		for (size_t i = 0; i < can_dlc_to_bytes(frame->dlc); i++) {
			/* when send can-fd frame, write all byte to `CAN_TMDATA0x`, x = mailbox */
			CAN_TMDATA0(can_periph, mailbox) = frame->data_32[i];
		}
#else
		LOG_ERR("Device Not Supported.");
		return -ENOTSUP;
#endif /* CONFIG_CAN_FD_MODE */
	} else {
		CAN_TMDATA0(can_periph, mailbox) = frame->data_32[0];
		CAN_TMDATA1(can_periph, mailbox) = frame->data_32[1];
	}

	CAN_TMI(can_periph, mailbox) |= CAN_TMI_TEN;
	k_mutex_unlock(&data->inst_mutex);

	if (callback == NULL) {
		k_sem_take(&mb->tx_int_sem, K_FOREVER);
		return mb->error;
	}

	return 0;
}

static int can_gd32_init_timing(const struct device *dev, struct can_timing *timing,
				struct can_timing *timing_data)
{
	const struct can_gd32_config *cfg = dev->config;
	int ret = 0;

	timing->sjw = cfg->timing_arbit.sjw;
	if (cfg->timing_arbit.sample_point && USE_SP_ALGO) {
		ret = can_calc_timing(dev, timing, cfg->timing_arbit.bus_speed,
				      cfg->timing_arbit.sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, TS1: %d, TS2: %d", timing->prescaler, timing->phase_seg1,
			timing->phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing->prop_seg = 0;
		timing->phase_seg1 = cfg->timing_arbit.prop_ts1;
		timing->phase_seg2 = cfg->timing_arbit.ts2;
		ret = can_calc_prescaler(dev, timing, cfg->timing_arbit.bus_speed);
		CAN_GD32_CHECK_RET(ret, LOG_LEVEL_WRN, "Bitrate error: %d", ret);
	}

#if defined CONFIG_CAN_FD_MODE
	timing_data->sjw = cfg->timing_data.sjw;
	if (cfg->timing_data.sample_point && USE_SP_ALGO) {
		ret = can_calc_timing_data(dev, timing_data, cfg->timing_data.bus_speed,
					   cfg->timing_data.sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing_data for given param");
			return -EIO;
		}
		LOG_DBG("DPresc: %d, DTS1: %d, DTS2: %d", timing_data->prescaler,
			timing_data->phase_seg1, timing_data->phase_seg2);
		LOG_DBG("Data Sample-point err : %d", ret);
	} else {
		timing_data->prop_seg = 0;
		timing_data->phase_seg1 = cfg->timing_data.prop_ts1;
		timing_data->phase_seg2 = cfg->timing_data.ts2;
		ret = can_calc_prescaler(dev, timing_data, cfg->timing_data.bus_speed);
		CAN_GD32_CHECK_RET(ret, LOG_LEVEL_WRN, "Data Bitrate error: %d", ret);
	}
#endif /* CONFIG_CAN_FD_MODE */

	return ret;
}

static void can_gd32_init_data(struct can_gd32_data *data)
{
	/* initial mutex and semaphone for mailbox */
	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, 0, 1);
	k_sem_init(&data->mb0.tx_int_sem, 0, 1);
	k_sem_init(&data->mb1.tx_int_sem, 0, 1);
	k_sem_init(&data->mb2.tx_int_sem, 0, 1);
	data->mb0.tx_callback = NULL;
	data->mb1.tx_callback = NULL;
	data->mb2.tx_callback = NULL;
	data->state_change_cb = NULL;
	data->state_change_cb_data = NULL;

	/* todo: data->filter_usage = (1ULL << CAN_MAX_NUMBER_OF_FILTERS) - 1ULL; */
	(void)memset(data->rx_cb, 0, sizeof(data->rx_cb));
	(void)memset(data->cb_arg, 0, sizeof(data->cb_arg));
}

static int can_gd32_init(const struct device *dev)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	int ret;

	struct can_timing timing;

#if defined CONFIG_CAN_FD_MODE
	struct can_timing timing_data;
#endif /* CONFIG_CAN_FD_MODE */

	/* enable filter */
	can_gd32_filter_initial(cfg->filter);

	/* initial mutex and semaphone for mailbox */
	can_gd32_init_data(data);

	/* todo: enable clock of can if not enabled */
	rcu_periph_clock_enable(cfg->rcu_periph_clock);

	/* set pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to enable can pinctrl");

	/* exit sleep mode and enter init mode */
	ret = can_leave_sleep_mode(can_periph);
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to exit sleep mode");
	ret = can_enter_init_mode(can_periph);
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to enter init mode");

	/* setup can controller property */
	CAN_CTL(can_periph) &= ~CAN_CTL_TTC & ~CAN_CTL_ABOR & ~CAN_CTL_AWU & ~CAN_CTL_ARD &
			       ~CAN_CTL_TFO & ~CAN_CTL_RFOD;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	CAN_CTL(can_periph) |= CAN_MCR_TTCM;
#endif
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	CAN_CTL(can_periph) |= CAN_CTL_ABOR;
#endif
#ifdef CAN_GD32_DEBUG_FREEZE
	CAN_CTL(can_periph) |= CAN_CTL_DFZ;
/* todo: set DBG_CTL -> CANx_HOLD */
#endif
	if (cfg->one_shot) {
		CAN_CTL(can_periph) |= CAN_CTL_ARD;
	}
#if defined CONFIG_CAN_FD_MODE
	CAN_FDCTL(can_periph) |= CAN_FDCTL_FDEN;
	CAN_FDCTL(can_periph) |= cfg->fdmode ? CAN_FDMOD_BOSCH : CAN_FDMOD_ISO;
	CAN_FDCTL(can_periph) |= cfg->esimode ? CAN_ESIMOD_SOFTWARE : CAN_ESIMOD_HARDWARE;
	/* todo: PRED */
	/* todo: TDC */
#endif /* CONFIG_CAN_FD_MODE */

	/* setup timeing */
#if defined CONFIG_CAN_FD_MODE
	ret = can_gd32_init_timing(dev, &timing, &timing_data);
#else /* CONFIG_CAN_FD_MODE */
	ret = can_gd32_init_timing(dev, &timing, NULL);
#endif /* CONFIG_CAN_FD_MODE */
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to init timing");
#if defined CONFIG_CAN_FD_MODE
	ret = can_gd32_set_timing(dev, &timing, &timing_data);
#else /* CONFIG_CAN_FD_MODE */
	ret = can_gd32_set_timing(dev, &timing, NULL);
#endif /* CONFIG_CAN_FD_MODE */
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to set timing");

	ret = can_gd32_set_mode(dev, CAN_NORMAL_MODE);
	CAN_GD32_CHECK_RET(ret, LOG_LEVEL_ERR, "Failed to set mode");

	cfg->irq_cfg_func(can_periph);
	LOG_INF("Init of %s done", dev->name);

	return 0;
}

static struct can_driver_api can_gd32_driver_api = {
	.set_mode = can_gd32_set_mode,
	.set_timing = can_gd32_set_timing,
	.send = can_gd32_send,
	.add_rx_filter = can_gd32_add_rx_filter,
	.remove_rx_filter = can_gd32_remove_rx_filter,
	.get_state = can_gd32_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_gd32_recover,
#endif
	.set_state_change_callback = can_gd32_set_state_change_callback,
	.get_core_clock = can_gd32_get_core_clock,
	.get_max_filters = can_gd32_get_max_filters,
	.timing_min = { .sjw = 0x1,
			.prop_seg = 0x00,
			.phase_seg1 = 0x01,
			.phase_seg2 = 0x01,
			.prescaler = 0x01 },
	.timing_max = { .sjw = 0x07,
			.prop_seg = 0x00,
			.phase_seg1 = 0x0F,
			.phase_seg2 = 0x07,
			.prescaler = 0x400 },
#if defined(CONFIG_CAN_FD_MODE) || defined(__DOXYGEN__)
	.timing_min_data = { /* todo: calc later */
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max_data = { /* todo: calc later */
		.sjw = 0x10,
		.prop_seg = 0x00,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x10,
		.prescaler = 0x20
	}
#endif /* CONFIG_CAN_FD_MODE */
};

static void can_gd32_signal_tx_complete(struct can_mailbox *mb)
{
	if (mb->tx_callback) {
		mb->tx_callback(mb->error, mb->callback_arg);
	} else {
		k_sem_give(&mb->tx_int_sem);
	}
}

static void can_gd32_tx_isr(const struct device *dev)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	uint32_t bus_off;

	/* check bus off */
	bus_off = CAN_ERR(can_periph) & CAN_ERR_BOERR;

	/* check if mailbox send complete and get error info then clear the request */
#define CAN_GD32_CHECK_TX_COMPLETE(mailbox)                                                        \
	if ((CAN_TSTAT(can_periph) & UTIL_CAT(CAN_TSTAT_MTF, mailbox)) | bus_off) {                \
		data->mb0.error =                                                                  \
			CAN_TSTAT(can_periph) & UTIL_CAT(CAN_TSTAT_MTFNERR, mailbox) ? 0 :         \
			CAN_TSTAT(can_periph) & UTIL_CAT(CAN_TSTAT_MTE, mailbox)     ? -EIO :      \
			CAN_TSTAT(can_periph) & UTIL_CAT(CAN_TSTAT_MAL, mailbox)     ? -EBUSY :    \
			bus_off							     ? -ENETDOWN : \
											     -EIO;                                                            \
                                                                                                   \
		/* write 1 to clear the request. */                                                \
		CAN_TSTAT(can_periph) |=                                                           \
			UTIL_CAT(CAN_TSTAT_MTE, mailbox) | UTIL_CAT(CAN_TSTAT_MTF, mailbox) |      \
			UTIL_CAT(CAN_TSTAT_MAL, mailbox) | UTIL_CAT(CAN_TSTAT_MTFNERR, mailbox);   \
                                                                                                   \
		/* invoke callback function or give semaphone. */                                  \
		can_gd32_signal_tx_complete(&UTIL_CAT(data->mb, mailbox));                         \
	}
	FOR_EACH (CAN_GD32_CHECK_TX_COMPLETE, (;), 0, 1, 2)
		;

	/* send semaphone to tell send function re-check mailbox */
	if (CAN_TSTAT(can_periph) & CAN_TSTAT_TME) {
		k_sem_give(&data->tx_int_sem);
	}
}

/* CAN_RFIFO0 */
#define CAN_RFIFO(canx, bank)                                                                      \
	REG32((canx) + 0x0CU + ((bank)*0x04U)) /*!< CAN receive FIFO mailbox identifier register */
#define CAN_RFIFO_RFL BITS(0, 1) /*!< receive FIFO0 length */
#define CAN_RFIFO_RFF BIT(3) /*!< receive FIFO0 full */
#define CAN_RFIFO_RFO BIT(4) /*!< receive FIFO0 overfull */
#define CAN_RFIFO_RFD BIT(5) /*!< receive FIFO0 dequeue */
#define CAN_RFIFOMP_FI_GET(canx, bank)                                                             \
	((CAN_RFIFOMP((can_periph), (fifo_num)) & CAN_RFIFOMP_FI) >> 8)
#define CAN_RFIFO_MAILBOX(canx, bank) CAN_RFIFOMI((can_periph), (fifo_num))

struct can_gd32_recv_mailbox {
	union {
		uint32_t identifier;
		struct {
			uint32_t reserve0 : 1;
			uint32_t is_remote : 1; /* frame type */
			uint32_t is_extended : 1; /* frame format */
			union {
				uint32_t extended_id : 29;
				struct {
					uint32_t reserve1 : 18;
					uint32_t standard_id : 11;
				};
			};
		};
	};
	union {
		uint32_t property;
		struct {
			uint32_t dlc : 4;
			uint32_t error_status : 1;
			uint32_t bit_rate_switch : 1;
			uint32_t reserve2 : 1;
			uint32_t is_canfd : 1;
			uint32_t filter_index : 8;
			uint32_t time_stamp : 16;
		};
	};
	uint32_t data0;
	uint32_t data1;
};

static void can_gd32_get_msg_fifo(const struct device *dev, uint32_t fifo_num,
				  struct zcan_frame *frame)
{
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;
	size_t index; /* loop index */

	struct can_gd32_recv_mailbox *mbox =
		(struct can_gd32_recv_mailbox *)CAN_RFIFO_MAILBOX(can_periph, fifo_num);

	if (mbox->is_extended) {
		frame->id = mbox->extended_id;
		frame->id_type = CAN_EXTENDED_IDENTIFIER;
	} else {
		frame->id = mbox->standard_id;
		frame->id_type = CAN_STANDARD_IDENTIFIER;
	}

	frame->rtr = mbox->is_remote ? CAN_REMOTEREQUEST : CAN_DATAFRAME;
	frame->dlc = mbox->dlc;

#if defined CONFIG_CAN_FD_MODE
	frame->fd = mbox->is_canfd;
#else /* CONFIG_CAN_FD_MODE */
	frame->fd = 0;
#endif /* CONFIG_CAN_FD_MODE */

	/* copy data */
	if (frame->fd) {
		frame->brs = mbox->bit_rate_switch;
		for (index = 0; index < (can_dlc_to_bytes(frame->dlc) / 4); index++) {
			frame->data_32[index] = mbox->data0;
		}
	} else {
		frame->data_32[0] = mbox->data0;
		frame->data_32[1] = mbox->data1;
	}

#ifdef CONFIG_CAN_RX_TIMESTAMP
	frame->timestamp = mbox->time_stamp;
#endif
}

static void can_gd32_rxn_isr(const struct device *dev, uint32_t fifo_num)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;

	int filter_match_index;
	struct zcan_frame frame;
	can_rx_callback_t callback;

	ARG_UNUSED(fifo_num); /* Fixme: debug only */

	while (CAN_RFIFO(can_periph, fifo_num) & CAN_RFIFO_RFL) {
		/* read mailbox to fifo and release message */
		filter_match_index = CAN_RFIFOMP_FI_GET(can_periph, fifo_num);
		if (filter_match_index >= CONFIG_CAN_MAX_FILTER) {
			LOG_ERR("fileter %d not found", filter_match_index);
			break;
		}

		LOG_DBG("Message on filter index %d", filter_match_index);
		can_gd32_get_msg_fifo(dev, fifo_num, &frame);

		/* Release message */
		CAN_RFIFO(can_periph, fifo_num) |= CAN_RFIFO_RFD;

		/* invoke callback */
		callback = data->rx_cb[filter_match_index];
		if (callback) {
			callback(&frame, data->cb_arg[filter_match_index]);
		}
	}

	/* check fifo overflow */
	if (CAN_RFIFO(can_periph, fifo_num) & CAN_RFIFO_RFO) {
		LOG_ERR("RX FIFO Overflow");
		CAN_RFIFO(can_periph, fifo_num) |= CAN_RFIFO_RFO;
	}
}

static void can_gd32_rx0_isr(const struct device *dev)
{
	can_gd32_rxn_isr(dev, 0);
}

static void can_gd32_rx1_isr(const struct device *dev)
{
	__ASSERT(0, "fifo1 not used yet");
	can_gd32_rxn_isr(dev, 1);
}

static inline void can_gd32_set_state_change(const struct device *dev)
{
	struct can_gd32_data *data = dev->data;
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;

	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *state_change_cb_data = data->state_change_cb_data;

	if (!(CAN_ERR(can_periph) & CAN_ERR_PERR) && !(CAN_ERR(can_periph) & CAN_ERR_BOERR) &&
	    !(CAN_ERR(can_periph) & CAN_ERR_WERR)) {
		return;
	}

	err_cnt.tx_err_cnt = GET_ERR_TECNT(can_periph);
	err_cnt.rx_err_cnt = GET_ERR_RECNT(can_periph);

	state = can_gd32_get_bus_state(can_periph);

	if (cb != NULL) {
		cb(state, err_cnt, state_change_cb_data);
	}
}

static void can_gd32_ewmc_isr(const struct device *dev)
{
	const struct can_gd32_config *cfg = dev->config;
	uint32_t can_periph = cfg->reg;

	/*Signal bus-off to waiting tx*/
	if (CAN_STAT(can_periph) & CAN_STAT_ERRIF) {
		can_gd32_tx_isr(dev);
		can_gd32_set_state_change(dev);
		CAN_STAT(can_periph) |= CAN_STAT_ERRIF;
	}
}

#define CAN_GD32_IRQ_SET(inst, event, isr)                                                         \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, event, irq),                                         \
		    DT_INST_IRQ_BY_NAME(inst, event, priority), isr, DEVICE_DT_INST_GET(inst), 0); \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, event, irq));

#define CAN_GD32_IRQ_INST(inst)                                                                    \
	static void can_gd32_irq_cfg_func_##inst(uint32_t can_periph)                              \
	{                                                                                          \
		/* regist isr*/                                                                    \
		CAN_GD32_IRQ_SET(inst, tx, can_gd32_tx_isr);                                       \
		CAN_GD32_IRQ_SET(inst, rx0, can_gd32_rx0_isr);                                     \
		CAN_GD32_IRQ_SET(inst, rx1, can_gd32_rx1_isr);                                     \
		CAN_GD32_IRQ_SET(inst, ewmc, can_gd32_ewmc_isr);                                   \
		/* enable irq */                                                                   \
		CAN_INTEN(can_periph) |= CAN_INTEN_TMEIE | CAN_INTEN_ERRIE | CAN_INTEN_RFNEIE0 |   \
					 CAN_INTEN_RFNEIE1 | CAN_INTEN_BOIE;                       \
	}

#define CAN_GD32_DATA_INST(inst) static struct can_gd32_data can_gd32_data_##inst;

#define CAN_GD32_DT_INST_GET_MAIN_CONTROLLER_FILTER(inst)                                          \
	DT_CHILD(DT_INST_PROP_BY_IDX(inst, main_controller, 0), filter)
#define CAN_GD32_CFG_INST_FILTER_CECLARE(inst)                                                     \
	COND_CODE_1(                                                                               \
		DT_NODE_HAS_STATUS(DT_INST_CHILD(inst, filter), okay),                             \
		(extern struct can_gd32_filter DT_INST_CHILD(inst, filter)),                       \
		(extern struct can_gd32_filter CAN_GD32_DT_INST_GET_MAIN_CONTROLLER_FILTER(inst)))
#define CAN_GD32_CFG_INST_FILTER(inst)                                                             \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(inst, filter), okay),                         \
		    (.filter = &DT_INST_CHILD(inst, filter)),                                      \
		    (.filter = &CAN_GD32_DT_INST_GET_MAIN_CONTROLLER_FILTER(inst)))
#define CAN_GD32_CFG_INST_CAN(inst)                                                                \
	.reg = DT_INST_REG_ADDR(inst), .rcu_periph_clock = DT_INST_PROP(inst, rcu_periph_clock),   \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                              \
	.irq_cfg_func = can_gd32_irq_cfg_func_##inst, .one_shot = DT_INST_PROP(inst, one_shot),    \
	CAN_GD32_CFG_INST_FILTER(inst),                                                            \
	.timing_arbit = {                                                                          \
		.bus_speed = DT_INST_PROP(inst, bus_speed),                                        \
		.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),                            \
		.sjw = DT_INST_PROP_OR(inst, sjw, 1),                                              \
		.prop_ts1 =                                                                        \
			DT_INST_PROP_OR(inst, prop_seg, 0) + DT_INST_PROP_OR(inst, phase_seg1, 0), \
		.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                                       \
	},

#if defined CONFIG_CAN_FD_MODE
#define CAN_GD32_CFG_INST(inst)                                                                    \
	const static struct can_gd32_config can_gd32_cfg_##inst = {		     \
		CAN_GD32_CFG_INST_CAN(inst)/* no comma here */			     \
		.fdmode = DT_INST_PROP_OR(inst, fd_standard, 0),		     \
		.esimode = DT_INST_PROP_OR(inst, esi_mode, 0),			     \
		.timing_data = {						     \
			.bus_speed = DT_INST_PROP(inst, bus_speed_data),	     \
			.sample_point = DT_INST_PROP_OR(inst, sample_point_data, 0), \
			.sjw = DT_INST_PROP_OR(inst, sjw_data, 1),		     \
			.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg_data, 0) +	     \
				    DT_INST_PROP_OR(inst, phase_seg1_data, 0),	     \
			.ts2 = DT_INST_PROP_OR(inst, phase_seg2_data, 0),	     \
		},								     \
	};
#else /*CONFIG_CAN_FD_MODE*/
#define CAN_GD32_CFG_INST(inst)                                                                    \
	const static struct can_gd32_config can_gd32_cfg_##inst = {                                \
		CAN_GD32_CFG_INST_CAN(inst) /* no comma here */                                    \
	};
#endif /*CONFIG_CAN_FD_MODE*/

#define CAN_GD32_BUILD_ASSERT(inst)                                                                \
	BUILD_ASSERT(UTIL_OR(DT_NODE_HAS_STATUS(DT_INST_CHILD(inst, filter), okay),                \
			     DT_INST_NODE_HAS_PROP(inst, main_controller)),                        \
		     "must have `filter` child or `main-controller` property in dts");             \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, main_controller),                                   \
		   (BUILD_ASSERT(DT_NODE_HAS_STATUS(                                               \
					 CAN_GD32_DT_INST_GET_MAIN_CONTROLLER_FILTER(inst), okay), \
				 "status of filter in `main-controller` must be ok")));

#define CAN_GD32_INIT(inst)                                                                        \
	CAN_GD32_BUILD_ASSERT(inst);                                                               \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	CAN_GD32_IRQ_INST(inst);                                                                   \
	CAN_GD32_DATA_INST(inst);                                                                  \
	CAN_GD32_CFG_INST_FILTER_CECLARE(inst);                                                    \
	CAN_GD32_CFG_INST(inst);                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_gd32_init, NULL, &can_gd32_data_##inst,                \
				  &can_gd32_cfg_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,     \
				  &can_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_GD32_INIT)
