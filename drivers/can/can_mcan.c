/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <string.h>
#include <kernel.h>
#include <drivers/can.h>
#include "can_mcan.h"
#include "can_mcan_int.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

#define CAN_INIT_TIMEOUT (100)
#define CAN_DIV_CEIL(val, div) (((val) + (div) - 1) / (div))

#ifdef CONFIG_CAN_FD_MODE
#define MCAN_MAX_DLC CANFD_MAX_DLC
#else
#define MCAN_MAX_DLC CAN_MAX_DLC
#endif

static int can_exit_sleep_mode(struct can_mcan_reg *can)
{
	uint32_t start_time;

	can->cccr &= ~CAN_MCAN_CCCR_CSR;
	start_time = k_cycle_get_32();

	while ((can->cccr & CAN_MCAN_CCCR_CSA) == CAN_MCAN_CCCR_CSA) {
		if (k_cycle_get_32() - start_time >
			k_ms_to_cyc_ceil32(CAN_INIT_TIMEOUT)) {
			can->cccr |= CAN_MCAN_CCCR_CSR;
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_enter_init_mode(struct can_mcan_reg  *can, k_timeout_t timeout)
{
	int64_t start_time;

	can->cccr |= CAN_MCAN_CCCR_INIT;
	start_time = k_uptime_ticks();

	while ((can->cccr & CAN_MCAN_CCCR_INIT) == 0U) {
		if (k_uptime_ticks() - start_time > timeout.ticks) {
			can->cccr &= ~CAN_MCAN_CCCR_INIT;
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_leave_init_mode(struct can_mcan_reg  *can, k_timeout_t timeout)
{
	int64_t start_time;

	can->cccr &= ~CAN_MCAN_CCCR_INIT;
	start_time = k_uptime_ticks();

	while ((can->cccr & CAN_MCAN_CCCR_INIT) != 0U) {
		if (k_uptime_ticks() - start_time > timeout.ticks) {
			return -EAGAIN;
		}
	}

	return 0;
}

void can_mcan_configure_timing(struct can_mcan_reg  *can,
			       const struct can_timing *timing,
			       const struct can_timing *timing_data)
{
	if (timing) {
		uint32_t nbtp_sjw = can->nbtp & CAN_MCAN_NBTP_NSJW_MSK;

		__ASSERT_NO_MSG(timing->prop_seg == 0);
		__ASSERT_NO_MSG(timing->phase_seg1 <= 0x100 &&
				timing->phase_seg1 > 0);
		__ASSERT_NO_MSG(timing->phase_seg2 <= 0x80 &&
				timing->phase_seg2 > 0);
		__ASSERT_NO_MSG(timing->prescaler <= 0x200 &&
				timing->prescaler > 0);
		__ASSERT_NO_MSG(timing->sjw <= 0x80 && timing->sjw > 0);

		can->nbtp = (((uint32_t)timing->phase_seg1 - 1UL) & 0xFF) <<
				CAN_MCAN_NBTP_NTSEG1_POS |
			    (((uint32_t)timing->phase_seg2 - 1UL) & 0x7F) <<
				CAN_MCAN_NBTP_NTSEG2_POS |
			    (((uint32_t)timing->prescaler  - 1UL) & 0x1FF) <<
				CAN_MCAN_NBTP_NBRP_POS;

		if (timing->sjw == CAN_SJW_NO_CHANGE) {
			can->nbtp |= nbtp_sjw;
		} else {
			can->nbtp |= (((uint32_t)timing->sjw - 1UL) & 0x7F) <<
				     CAN_MCAN_NBTP_NSJW_POS;
		}
	}

#ifdef CONFIG_CAN_FD_MODE
	if (timing_data) {
		uint32_t dbtp_sjw = can->dbtp & CAN_MCAN_DBTP_DSJW_MSK;

		__ASSERT_NO_MSG(timing_data->prop_seg == 0);
		__ASSERT_NO_MSG(timing_data->phase_seg1 <= 0x20 &&
				timing_data->phase_seg1 > 0);
		__ASSERT_NO_MSG(timing_data->phase_seg2 <= 0x10 &&
				timing_data->phase_seg2 > 0);
		__ASSERT_NO_MSG(timing_data->prescaler <= 20 &&
				timing_data->prescaler > 0);
		__ASSERT_NO_MSG(timing_data->sjw <= 0x80 &&
				timing_data->sjw > 0);

		can->dbtp = (((uint32_t)timing_data->phase_seg1 - 1UL) & 0x1F) <<
				CAN_MCAN_DBTP_DTSEG1_POS |
			    (((uint32_t)timing_data->phase_seg2 - 1UL) & 0x0F) <<
				CAN_MCAN_DBTP_DTSEG2_POS |
			    (((uint32_t)timing_data->prescaler  - 1UL) & 0x1F) <<
				CAN_MCAN_DBTP_DBRP_POS;

		if (timing_data->sjw == CAN_SJW_NO_CHANGE) {
			can->dbtp |= dbtp_sjw;
		} else {
			can->dbtp |= (((uint32_t)timing_data->sjw - 1UL) & 0x0F) <<
				     CAN_MCAN_DBTP_DSJW_POS;
		}
	}
#endif
}

int can_mcan_set_timing(const struct can_mcan_config *cfg,
			const struct can_timing *timing,
			const struct can_timing *timing_data)
{
	struct can_mcan_reg *can = cfg->can;
	int ret;

	ret = can_enter_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	can_mcan_configure_timing(can, timing, timing_data);

	ret = can_leave_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to leave init mode");
		return -EIO;
	}

	return 0;
}

int can_mcan_set_mode(const struct can_mcan_config *cfg, enum can_mode mode)
{
	struct can_mcan_reg *can = cfg->can;
	int ret;

	ret = can_enter_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	/* Configuration Change Enable */
	can->cccr |= CAN_MCAN_CCCR_CCE;

	switch (mode) {
	case CAN_NORMAL_MODE:
		LOG_DBG("Config normal mode");
		can->cccr &= ~(CAN_MCAN_CCCR_TEST | CAN_MCAN_CCCR_MON);
		break;

	case CAN_SILENT_MODE:
		LOG_DBG("Config silent mode");
		can->cccr &= ~CAN_MCAN_CCCR_TEST;
		can->cccr |= CAN_MCAN_CCCR_MON;
		break;

	case CAN_LOOPBACK_MODE:
		LOG_DBG("Config loopback mode");
		can->cccr &= ~CAN_MCAN_CCCR_MON;
		can->cccr |= CAN_MCAN_CCCR_TEST;
		can->test |= CAN_MCAN_TEST_LBCK;
		break;

	case CAN_SILENT_LOOPBACK_MODE:
		LOG_DBG("Config silent loopback mode");
		can->cccr |= (CAN_MCAN_CCCR_TEST | CAN_MCAN_CCCR_MON);
		can->test |= CAN_MCAN_TEST_LBCK;
		break;
	default:
		break;
	}

	ret = can_leave_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to leave init mode");
	}

	return 0;
}

int can_mcan_init(const struct device *dev, const struct can_mcan_config *cfg,
		  struct can_mcan_msg_sram *msg_ram,
		  struct can_mcan_data *data)
{
	struct can_mcan_reg  *can = cfg->can;
	struct can_timing timing;
#ifdef CONFIG_CAN_FD_MODE
	struct can_timing timing_data;
#endif
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_mutex_init(&data->tx_mtx);
	k_sem_init(&data->tx_sem, NUM_TX_BUF_ELEMENTS, NUM_TX_BUF_ELEMENTS);
	for (int i = 0; i < ARRAY_SIZE(data->tx_fin_sem); ++i) {
		k_sem_init(&data->tx_fin_sem[i], 0, 1);
	}

	ret = can_exit_sleep_mode(can);
	if (ret) {
		LOG_ERR("Failed to exit sleep mode");
		return -EIO;
	}

	ret = can_enter_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	/* Configuration Change Enable */
	can->cccr |= CAN_MCAN_CCCR_CCE;

	LOG_DBG("IP rel: %lu.%lu.%lu %02lu.%lu.%lu",
		(can->crel & CAN_MCAN_CREL_REL) >> CAN_MCAN_CREL_REL_POS,
		(can->crel & CAN_MCAN_CREL_STEP) >> CAN_MCAN_CREL_STEP_POS,
		(can->crel & CAN_MCAN_CREL_SUBSTEP) >>
			     CAN_MCAN_CREL_SUBSTEP_POS,
		(can->crel & CAN_MCAN_CREL_YEAR) >> CAN_MCAN_CREL_YEAR_POS,
		(can->crel & CAN_MCAN_CREL_MON) >> CAN_MCAN_CREL_MON_POS,
		(can->crel & CAN_MCAN_CREL_DAY) >> CAN_MCAN_CREL_DAY_POS);

#ifndef CONFIG_CAN_STM32FD
	can->sidfc = ((uint32_t)msg_ram->std_filt & CAN_MCAN_SIDFC_FLSSA_MSK) |
		     (ARRAY_SIZE(msg_ram->std_filt) << CAN_MCAN_SIDFC_LSS_POS);
	can->xidfc = ((uint32_t)msg_ram->ext_filt & CAN_MCAN_XIDFC_FLESA_MSK) |
		     (ARRAY_SIZE(msg_ram->ext_filt) << CAN_MCAN_XIDFC_LSS_POS);
	can->rxf0c = ((uint32_t)msg_ram->rx_fifo0 & CAN_MCAN_RXF0C_F0SA) |
		     (ARRAY_SIZE(msg_ram->rx_fifo0) << CAN_MCAN_RXF0C_F0S_POS);
	can->rxf1c = ((uint32_t)msg_ram->rx_fifo1 & CAN_MCAN_RXF1C_F1SA) |
		     (ARRAY_SIZE(msg_ram->rx_fifo1) << CAN_MCAN_RXF1C_F1S_POS);
	can->rxbc = ((uint32_t)msg_ram->rx_buffer & CAN_MCAN_RXBC_RBSA);
	can->txefc = ((uint32_t)msg_ram->tx_event_fifo & CAN_MCAN_TXEFC_EFSA_MSK) |
		     (ARRAY_SIZE(msg_ram->tx_event_fifo) <<
		     CAN_MCAN_TXEFC_EFS_POS);
	can->txbc = ((uint32_t)msg_ram->tx_buffer & CAN_MCAN_TXBC_TBSA) |
		    (ARRAY_SIZE(msg_ram->tx_buffer) << CAN_MCAN_TXBC_TFQS_POS);
	if (sizeof(msg_ram->tx_buffer[0].data) <= 24) {
		can->txesc = (sizeof(msg_ram->tx_buffer[0].data) - 8) / 4;
	} else {
		can->txesc = (sizeof(msg_ram->tx_buffer[0].data) - 32) / 16 + 5;
	}

	if (sizeof(msg_ram->rx_fifo0[0].data) <= 24) {
		can->rxesc = (((sizeof(msg_ram->rx_fifo0[0].data) - 8) / 4) <<
				CAN_MCAN_RXESC_F0DS_POS) |
			     (((sizeof(msg_ram->rx_fifo1[0].data) - 8) / 4) <<
				CAN_MCAN_RXESC_F1DS_POS) |
			     (((sizeof(msg_ram->rx_buffer[0].data) - 8) / 4) <<
				CAN_MCAN_RXESC_RBDS_POS);
	} else {
		can->rxesc = (((sizeof(msg_ram->rx_fifo0[0].data) - 32)
				/ 16 + 5) << CAN_MCAN_RXESC_F0DS_POS) |
			     (((sizeof(msg_ram->rx_fifo1[0].data) - 32)
				/ 16 + 5) << CAN_MCAN_RXESC_F1DS_POS) |
			     (((sizeof(msg_ram->rx_buffer[0].data) - 32)
				/ 16 + 5) << CAN_MCAN_RXESC_RBDS_POS);
	}
#endif

#ifdef CONFIG_CAN_FD_MODE
	can->cccr |= CAN_MCAN_CCCR_FDOE | CAN_MCAN_CCCR_BRSE;
#else
	can->cccr &= ~(CAN_MCAN_CCCR_FDOE | CAN_MCAN_CCCR_BRSE);
#endif
	can->cccr &= ~(CAN_MCAN_CCCR_TEST | CAN_MCAN_CCCR_MON |
		       CAN_MCAN_CCCR_ASM);
	can->test &= ~(CAN_MCAN_TEST_LBCK);

#if defined(CONFIG_CAN_DELAY_COMP) && defined(CONFIG_CAN_FD_MODE)
	can->dbtp |= CAN_MCAN_DBTP_TDC;
	can->tdcr |=  cfg->tx_delay_comp_offset << CAN_MCAN_TDCR_TDCO_POS;

#endif

#ifdef CONFIG_CAN_STM32FD
	can->rxgfc |= (CONFIG_CAN_MAX_STD_ID_FILTER << CAN_MCAN_RXGFC_LSS_POS) |
		      (CONFIG_CAN_MAX_EXT_ID_FILTER << CAN_MCAN_RXGFC_LSE_POS) |
		      (0x2 << CAN_MCAN_RXGFC_ANFS_POS) |
		      (0x2 << CAN_MCAN_RXGFC_ANFE_POS);
#else
	can->gfc |= (0x2 << CAN_MCAN_GFC_ANFE_POS) |
		    (0x2 << CAN_MCAN_GFC_ANFS_POS);
#endif /* CONFIG_CAN_STM32FD */

	if (cfg->sample_point) {
		ret = can_calc_timing(dev, &timing, cfg->bus_speed,
				      cfg->sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, TS1: %d, TS2: %d",
			timing.prescaler, timing.phase_seg1, timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else if (cfg->prop_ts1) {
		timing.prop_seg = 0;
		timing.phase_seg1 = cfg->prop_ts1;
		timing.phase_seg2 = cfg->ts2;
		ret = can_calc_prescaler(dev, &timing, cfg->bus_speed);
		if (ret) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}
#ifdef CONFIG_CAN_FD_MODE
	if (cfg->sample_point_data) {
		ret = can_calc_timing_data(dev, &timing_data,
					   cfg->bus_speed_data,
					   cfg->sample_point_data);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given dataphase param");
			return -EIO;
		}

		LOG_DBG("Sample-point err data phase: %d", ret);
	} else if (cfg->prop_ts1_data) {
		timing_data.prop_seg = 0;
		timing_data.phase_seg1 = cfg->prop_ts1_data;
		timing_data.phase_seg2 = cfg->ts2_data;
		ret = can_calc_prescaler(dev, &timing_data,
					 cfg->bus_speed_data);
		if (ret) {
			LOG_WRN("Dataphase bitrate error: %d", ret);
		}
	}
#endif

	timing.sjw = cfg->sjw;
#ifdef CONFIG_CAN_FD_MODE
	timing_data.sjw = cfg->sjw_data;
	can_mcan_configure_timing(can, &timing, &timing_data);
#else
	can_mcan_configure_timing(can, &timing, NULL);
#endif

	can->ie = CAN_MCAN_IE_BO | CAN_MCAN_IE_EW | CAN_MCAN_IE_EP |
		  CAN_MCAN_IE_MRAF | CAN_MCAN_IE_TEFL | CAN_MCAN_IE_TEFN |
		  CAN_MCAN_IE_RF0N | CAN_MCAN_IE_RF1N | CAN_MCAN_IE_RF0L |
		  CAN_MCAN_IE_RF1L;

#ifdef CONFIG_CAN_STM32FD
	can->ils = CAN_MCAN_ILS_RXFIFO0 | CAN_MCAN_ILS_RXFIFO1;
#else
	can->ils = CAN_MCAN_ILS_RF0N | CAN_MCAN_ILS_RF1N;
#endif
	can->ile = CAN_MCAN_ILE_EINT0 | CAN_MCAN_ILE_EINT1;
	/* Interrupt on every TX fifo element*/
	can->txbtie = CAN_MCAN_TXBTIE_TIE;

	ret = can_leave_init_mode(can, K_MSEC(CAN_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Failed to leave init mode");
		return -EIO;
	}

	/* No memset because only aligned ptr are allowed */
	for (uint32_t *ptr = (uint32_t *)msg_ram;
	     ptr < (uint32_t *)msg_ram +
		   sizeof(struct can_mcan_msg_sram) / sizeof(uint32_t);
	     ptr++) {
		*ptr = 0;
	}

	return 0;
}

static void can_mcan_state_change_handler(const struct can_mcan_config *cfg,
					  struct can_mcan_data *data)
{
	enum can_state state;
	struct can_bus_err_cnt err_cnt;

	state = can_mcan_get_state(cfg, &err_cnt);

	if (data->state_change_isr) {
		data->state_change_isr(state, err_cnt);
	}
}

static void can_mcan_tc_event_handler(struct can_mcan_reg *can,
				      struct can_mcan_msg_sram *msg_ram,
				      struct can_mcan_data *data)
{
	volatile struct can_mcan_tx_event_fifo *tx_event;
	can_tx_callback_t tx_cb;
	uint32_t event_idx, tx_idx;

	while (can->txefs & CAN_MCAN_TXEFS_EFFL) {
		event_idx = (can->txefs & CAN_MCAN_TXEFS_EFGI) >>
			    CAN_MCAN_TXEFS_EFGI_POS;
		tx_event = &msg_ram->tx_event_fifo[event_idx];
		tx_idx = tx_event->mm.idx;
		/* Acknowledge TX event */
		can->txefa = event_idx;

		k_sem_give(&data->tx_sem);

		tx_cb = data->tx_fin_cb[tx_idx];
		if (tx_cb == NULL) {
			k_sem_give(&data->tx_fin_sem[tx_idx]);
		} else {
			tx_cb(0, data->tx_fin_cb_arg[tx_idx]);
		}
	}
}

void can_mcan_line_0_isr(const struct can_mcan_config *cfg,
			 struct can_mcan_msg_sram *msg_ram,
			 struct can_mcan_data *data)
{
	struct can_mcan_reg *can = cfg->can;

	do {
		if (can->ir & (CAN_MCAN_IR_BO | CAN_MCAN_IR_EP |
			       CAN_MCAN_IR_EW)) {
			can->ir = CAN_MCAN_IR_BO | CAN_MCAN_IR_EP |
				  CAN_MCAN_IR_EW;
			can_mcan_state_change_handler(cfg, data);
		}
		/* TX event FIFO new entry */
		if (can->ir & CAN_MCAN_IR_TEFN) {
			can->ir = CAN_MCAN_IR_TEFN;
			can_mcan_tc_event_handler(can, msg_ram, data);
		}

		if (can->ir & CAN_MCAN_IR_TEFL) {
			can->ir = CAN_MCAN_IR_TEFL;
			LOG_ERR("TX FIFO element lost");
			k_sem_give(&data->tx_sem);
		}

		if (can->ir & CAN_MCAN_IR_ARA) {
			can->ir  = CAN_MCAN_IR_ARA;
			LOG_ERR("Access to reserved address");
		}

		if (can->ir & CAN_MCAN_IR_MRAF) {
			can->ir  = CAN_MCAN_IR_MRAF;
			LOG_ERR("Message RAM access failure");
		}

	} while (can->ir & (CAN_MCAN_IR_BO | CAN_MCAN_IR_EW | CAN_MCAN_IR_EP |
			    CAN_MCAN_IR_TEFL | CAN_MCAN_IR_TEFN));
}

static void can_mcan_get_message(struct can_mcan_data *data,
				 volatile struct can_mcan_rx_fifo *fifo,
				 volatile uint32_t *fifo_status_reg,
				 volatile uint32_t *fifo_ack_reg)
{
	uint32_t get_idx, filt_idx;
	struct zcan_frame frame;
	can_rx_callback_t cb;
	volatile uint32_t *src, *dst, *end;
	int data_length;
	void *cb_arg;
	struct can_mcan_rx_fifo_hdr hdr;

	while ((*fifo_status_reg & CAN_MCAN_RXF0S_F0FL)) {
		get_idx = (*fifo_status_reg & CAN_MCAN_RXF0S_F0GI) >>
			   CAN_MCAN_RXF0S_F0GI_POS;
		hdr = fifo[get_idx].hdr;

		if (hdr.xtd) {
			frame.id = hdr.ext_id;
		} else {
			frame.id = hdr.std_id;
		}
		frame.fd = hdr.fdf;
		frame.rtr = hdr.rtr ? CAN_REMOTEREQUEST :
				      CAN_DATAFRAME;
		frame.id_type = hdr.xtd ? CAN_EXTENDED_IDENTIFIER :
					  CAN_STANDARD_IDENTIFIER;
		frame.dlc = hdr.dlc;
		frame.brs = hdr.brs;
#if defined(CONFIG_CAN_RX_TIMESTAMP)
		frame.timestamp = hdr.rxts;
#endif

		filt_idx = hdr.fidx;

		/* Check if RTR must match */
		if ((hdr.xtd && data->ext_filt_rtr_mask & (1U << filt_idx) &&
		     ((data->ext_filt_rtr >> filt_idx) & 1U) != frame.rtr) ||
		    (data->std_filt_rtr_mask &  (1U << filt_idx) &&
		     ((data->std_filt_rtr >> filt_idx) & 1U) != frame.rtr)) {
			continue;
		}

		data_length = can_dlc_to_bytes(frame.dlc);
		if (data_length <= sizeof(frame.data)) {
			/* data needs to be written in 32 bit blocks!*/
			for (src = fifo[get_idx].data_32,
			     dst = frame.data_32,
			     end = dst + CAN_DIV_CEIL(data_length, sizeof(uint32_t));
			     dst < end;
			     src++, dst++) {
				*dst = *src;
			}

			if (frame.id_type == CAN_STANDARD_IDENTIFIER) {
				LOG_DBG("Frame on filter %d, ID: 0x%x",
					filt_idx, frame.id);
				cb = data->rx_cb_std[filt_idx];
				cb_arg = data->cb_arg_std[filt_idx];
			} else {
				LOG_DBG("Frame on filter %d, ID: 0x%x",
					filt_idx + NUM_STD_FILTER_DATA,
					frame.id);
				cb = data->rx_cb_ext[filt_idx];
				cb_arg = data->cb_arg_ext[filt_idx];
			}

			if (cb) {
				cb(&frame, cb_arg);
			} else {
				LOG_DBG("cb missing");
			}
		} else {
			LOG_ERR("Frame is too big");
		}

		*fifo_ack_reg = get_idx;
	}
}

void can_mcan_line_1_isr(const struct can_mcan_config *cfg,
			 struct can_mcan_msg_sram *msg_ram,
			 struct can_mcan_data *data)
{
	struct can_mcan_reg *can = cfg->can;

	do {
		if (can->ir & CAN_MCAN_IR_RF0N) {
			can->ir  = CAN_MCAN_IR_RF0N;
			LOG_DBG("RX FIFO0 INT");
			can_mcan_get_message(data, msg_ram->rx_fifo0,
					     &can->rxf0s, &can->rxf0a);
		}

		if (can->ir & CAN_MCAN_IR_RF1N) {
			can->ir  = CAN_MCAN_IR_RF1N;
			LOG_DBG("RX FIFO1 INT");
			can_mcan_get_message(data, msg_ram->rx_fifo1,
					     &can->rxf1s, &can->rxf1a);
		}

		if (can->ir & CAN_MCAN_IR_RF0L) {
			can->ir  = CAN_MCAN_IR_RF0L;
			LOG_ERR("Message lost on FIFO0");
		}

		if (can->ir & CAN_MCAN_IR_RF1L) {
			can->ir  = CAN_MCAN_IR_RF1L;
			LOG_ERR("Message lost on FIFO1");
		}

	} while (can->ir & (CAN_MCAN_IR_RF0N | CAN_MCAN_IR_RF1N |
			    CAN_MCAN_IR_RF0L | CAN_MCAN_IR_RF1L));
}

enum can_state can_mcan_get_state(const struct can_mcan_config *cfg,
				  struct can_bus_err_cnt *err_cnt)
{
	struct can_mcan_reg *can = cfg->can;

	err_cnt->rx_err_cnt = (can->ecr & CAN_MCAN_ECR_TEC_MSK) <<
			      CAN_MCAN_ECR_TEC_POS;

	err_cnt->tx_err_cnt = (can->ecr & CAN_MCAN_ECR_REC_MSK) <<
			      CAN_MCAN_ECR_REC_POS;

	if (can->psr & CAN_MCAN_PSR_BO) {
		return CAN_BUS_OFF;
	}

	if (can->psr & CAN_MCAN_PSR_EP) {
		return CAN_ERROR_PASSIVE;
	}

	return CAN_ERROR_ACTIVE;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_mcan_recover(struct can_mcan_reg *can, k_timeout_t timeout)
{
	return can_leave_init_mode(can, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */


int can_mcan_send(const struct can_mcan_config *cfg,
		  struct can_mcan_data *data,
		  struct can_mcan_msg_sram *msg_ram,
		  const struct zcan_frame *frame,
		  k_timeout_t timeout,
		  can_tx_callback_t callback, void *user_data)
{
	struct can_mcan_reg  *can = cfg->can;
	size_t data_length = can_dlc_to_bytes(frame->dlc);
	struct can_mcan_tx_buffer_hdr tx_hdr = {
		.rtr = frame->rtr  == CAN_REMOTEREQUEST,
		.xtd = frame->id_type == CAN_EXTENDED_IDENTIFIER,
		.esi = 0,
		.dlc = frame->dlc,
#ifdef CONFIG_CAN_FD_MODE
		.brs = frame->brs == true,
#endif
		.fdf = frame->fd,
		.efc = 1,
	};
	uint32_t put_idx;
	int ret;
	struct can_mcan_mm mm;
	volatile uint32_t *dst, *end;
	const uint32_t *src;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s %s %s",
		data_length, frame->id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "" : "RTR",
		frame->fd == CAN_DATAFRAME ? "" : "FD frame",
		frame->brs == CAN_DATAFRAME ? "" : "BRS");

	if (data_length > sizeof(frame->data)) {
		LOG_ERR("data length (%zu) > max frame data length (%zu)",
			data_length, sizeof(frame->data));
		return -EINVAL;
	}

	if (frame->fd != 1 && frame->dlc > MCAN_MAX_DLC) {
		LOG_ERR("DLC of %d without fd flag set.", frame->dlc);
		return -EINVAL;
	}

	if (can->psr & CAN_MCAN_PSR_BO) {
		return -ENETDOWN;
	}

	ret = k_sem_take(&data->tx_sem, timeout);
	if (ret != 0) {
		return -EAGAIN;
	}

	__ASSERT_NO_MSG((can->txfqs & CAN_MCAN_TXFQS_TFQF) !=
			CAN_MCAN_TXFQS_TFQF);

	k_mutex_lock(&data->tx_mtx, K_FOREVER);

	put_idx = ((can->txfqs & CAN_MCAN_TXFQS_TFQPI) >>
		   CAN_MCAN_TXFQS_TFQPI_POS);

	mm.idx = put_idx;
	mm.cnt = data->mm.cnt++;
	tx_hdr.mm = mm;

	if (frame->id_type == CAN_STANDARD_IDENTIFIER) {
		tx_hdr.std_id = frame->id & CAN_STD_ID_MASK;
	} else {
		tx_hdr.ext_id = frame->id;
	}

	msg_ram->tx_buffer[put_idx].hdr = tx_hdr;

	for (src = frame->data_32,
		dst = msg_ram->tx_buffer[put_idx].data_32,
		end = dst + CAN_DIV_CEIL(data_length, sizeof(uint32_t));
		dst < end;
		src++, dst++) {
		*dst = *src;
	}

	data->tx_fin_cb[put_idx] = callback;
	data->tx_fin_cb_arg[put_idx] = user_data;

	can->txbar = (1U << put_idx);

	k_mutex_unlock(&data->tx_mtx);

	if (callback == NULL) {
		LOG_DBG("Waiting for TX complete");
		k_sem_take(&data->tx_fin_sem[put_idx], K_FOREVER);
	}

	return 0;
}

static int can_mcan_get_free_std(volatile struct can_mcan_std_filter *filters)
{
	for (int i = 0; i < NUM_STD_FILTER_DATA; ++i) {
		if (filters[i].sfce == CAN_MCAN_FCE_DISABLE) {
			return i;
		}
	}

	return -ENOSPC;
}

/* Use masked configuration only for simplicity. If someone needs more than
 * 28 standard filters, dual mode needs to be implemented.
 * Dual mode gets tricky, because we can only activate both filters.
 * If one of the IDs is not used anymore, we would need to mark it as unused.
 */
int can_mcan_attach_std(struct can_mcan_data *data,
			struct can_mcan_msg_sram *msg_ram,
			can_rx_callback_t isr, void *cb_arg,
			const struct zcan_filter *filter)
{
	struct can_mcan_std_filter filter_element = {
		.id1 = filter->id,
		.id2 = filter->id_mask,
		.sft = CAN_MCAN_SFT_MASKED
	};
	int filter_nr;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	filter_nr = can_mcan_get_free_std(msg_ram->std_filt);

	if (filter_nr == -ENOSPC) {
		LOG_INF("No free standard id filter left");
		return -ENOSPC;
	}

	/* TODO propper fifo balancing */
	filter_element.sfce = filter_nr & 0x01 ? CAN_MCAN_FCE_FIFO1 :
						 CAN_MCAN_FCE_FIFO0;

	msg_ram->std_filt[filter_nr] = filter_element;

	k_mutex_unlock(&data->inst_mutex);

	LOG_DBG("Attached std filter at %d", filter_nr);

	if (filter->rtr) {
		data->std_filt_rtr |= (1U << filter_nr);
	} else {
		data->std_filt_rtr &= ~(1U << filter_nr);
	}

	if (filter->rtr_mask) {
		data->std_filt_rtr_mask |= (1U << filter_nr);
	} else {
		data->std_filt_rtr_mask &= ~(1U << filter_nr);
	}

	data->rx_cb_std[filter_nr] = isr;
	data->cb_arg_std[filter_nr] = cb_arg;

	return filter_nr;
}

static int can_mcan_get_free_ext(volatile struct can_mcan_ext_filter *filters)
{
	for (int i = 0; i < NUM_EXT_FILTER_DATA; ++i) {
		if (filters[i].efce == CAN_MCAN_FCE_DISABLE) {
			return i;
		}
	}

	return -ENOSPC;
}

static int can_mcan_attach_ext(struct can_mcan_data *data,
			       struct can_mcan_msg_sram *msg_ram,
			       can_rx_callback_t isr, void *cb_arg,
			       const struct zcan_filter *filter)
{
	struct can_mcan_ext_filter filter_element = {
		.id2 = filter->id_mask,
		.id1 = filter->id,
		.eft = CAN_MCAN_EFT_MASKED
	};
	int filter_nr;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	filter_nr = can_mcan_get_free_ext(msg_ram->ext_filt);

	if (filter_nr == -ENOSPC) {
		LOG_INF("No free extender id filter left");
		return -ENOSPC;
	}

	/* TODO propper fifo balancing */
	filter_element.efce = filter_nr & 0x01 ? CAN_MCAN_FCE_FIFO1 :
						 CAN_MCAN_FCE_FIFO0;

	msg_ram->ext_filt[filter_nr] = filter_element;

	k_mutex_unlock(&data->inst_mutex);

	LOG_DBG("Attached ext filter at %d", filter_nr);

	if (filter->rtr) {
		data->ext_filt_rtr |= (1U << filter_nr);
	} else {
		data->ext_filt_rtr &= ~(1U << filter_nr);
	}

	if (filter->rtr_mask) {
		data->ext_filt_rtr_mask |= (1U << filter_nr);
	} else {
		data->ext_filt_rtr_mask &= ~(1U << filter_nr);
	}

	data->rx_cb_ext[filter_nr] = isr;
	data->cb_arg_ext[filter_nr] = cb_arg;

	return filter_nr;
}

int can_mcan_attach_isr(struct can_mcan_data *data,
			struct can_mcan_msg_sram *msg_ram,
			can_rx_callback_t isr, void *cb_arg,
			const struct zcan_filter *filter)
{
	int filter_nr;

	if (!isr) {
		return -EINVAL;
	}

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		filter_nr = can_mcan_attach_std(data, msg_ram, isr, cb_arg,
						filter);
	} else {
		filter_nr = can_mcan_attach_ext(data, msg_ram, isr, cb_arg,
						filter);
		filter_nr += NUM_STD_FILTER_DATA;
	}

	if (filter_nr == -ENOSPC) {
		LOG_INF("No free filter left");
	}

	return filter_nr;
}

void can_mcan_detach(struct can_mcan_data *data,
		     struct can_mcan_msg_sram *msg_ram, int filter_nr)
{
	const struct can_mcan_ext_filter ext_filter = {0};
	const struct can_mcan_std_filter std_filter = {0};

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	if (filter_nr >= NUM_STD_FILTER_DATA) {
		filter_nr -= NUM_STD_FILTER_DATA;
		if (filter_nr >= NUM_STD_FILTER_DATA) {
			LOG_ERR("Wrong filter id");
			return;
		}

		msg_ram->ext_filt[filter_nr] = ext_filter;
		data->rx_cb_ext[filter_nr] = NULL;
	} else {
		msg_ram->std_filt[filter_nr] = std_filter;
		data->rx_cb_std[filter_nr] = NULL;
	}

	k_mutex_unlock(&data->inst_mutex);
}
