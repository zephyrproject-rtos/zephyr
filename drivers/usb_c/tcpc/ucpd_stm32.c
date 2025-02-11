/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ucpd

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ucpd_stm32, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stddef.h>
#include <zephyr/math/ilog2.h>
#include <stm32_ll_system.h>
#include <zephyr/irq.h>

#include "ucpd_stm32_priv.h"

static void config_tcpc_irq(void);

/**
 * @brief UCPD TX ORDSET values
 */
static int ucpd_txorderset[] = {
	/* SOP ORDSET */
	LL_UCPD_ORDERED_SET_SOP,
	/* SOP PRIME ORDSET */
	LL_UCPD_ORDERED_SET_SOP1,
	/* SOP PRIME PRIME ORDSET */
	LL_UCPD_ORDERED_SET_SOP2,
	/* SOP PRIME DEBUG ORDSET */
	LL_UCPD_ORDERED_SET_SOP1_DEBUG,
	/* SOP PRIME PRIME DEBUG ORDSET */
	LL_UCPD_ORDERED_SET_SOP2_DEBUG,
	/* HARD RESET ORDSET */
	LL_UCPD_ORDERED_SET_HARD_RESET,
	/* CABLE RESET ORDSET */
	LL_UCPD_ORDERED_SET_CABLE_RESET,
};

/**
 * @brief Test for a goodCRC message
 *
 * @retval true if message is goodCRC, else false
 */
static bool ucpd_msg_is_good_crc(union pd_header header)
{
	/*
	 * Good CRC is a control message (no data objects) with GOOD_CRC
	 * message type in the header.
	 */
	return (header.number_of_data_objects == 0 &&
		header.extended == 0 &&
		header.message_type == PD_CTRL_GOOD_CRC);
}

#ifdef CONFIG_SOC_SERIES_STM32G0X
/**
 * @brief Apply the UCPD CC1 and CC2 pin configurations.
 *
 *        UCPDx_STROBE: UCPDx pull-down configuration strobe:
 *                      when UCPDx is enabled, with CC1 and CC2 pin UCPD
 *                      control bits configured: apply that configuration.
 */
static void update_stm32g0x_cc_line(UCPD_TypeDef *ucpd_port)
{
	if ((uint32_t)(ucpd_port) == UCPD1_BASE) {
		SYSCFG->CFGR1 |= SYSCFG_CFGR1_UCPD1_STROBE_Msk;
	} else {
		SYSCFG->CFGR1 |= SYSCFG_CFGR1_UCPD2_STROBE_Msk;
	}
}
#endif

/**
 * @brief Transmits a data byte from the TX data buffer
 */
static void ucpd_tx_data_byte(const struct device *dev)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;
	int index = data->ucpd_tx_active_buffer->msg_index++;

	LL_UCPD_WriteData(config->ucpd_port,
			  data->ucpd_tx_active_buffer->data.msg[index]);
}

/**
 * @brief Receives a data byte and store it in the RX data buffer
 */
static void ucpd_rx_data_byte(const struct device *dev)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;

	if (data->ucpd_rx_byte_count < UCPD_BUF_LEN) {
		data->ucpd_rx_buffer[data->ucpd_rx_byte_count++] =
			LL_UCPD_ReadData(config->ucpd_port);
	}
}

/**
 * @brief Enables or Disables TX interrupts
 */
static void ucpd_tx_interrupts_enable(const struct device *dev, bool enable)
{
	const struct tcpc_config *const config = dev->config;
	uint32_t imr;

	imr = LL_UCPD_ReadReg(config->ucpd_port, IMR);

	if (enable) {
		LL_UCPD_WriteReg(config->ucpd_port, ICR, UCPD_ICR_TX_INT_MASK);
		LL_UCPD_WriteReg(config->ucpd_port, IMR,
				 imr | UCPD_IMR_TX_INT_MASK);
	} else {
		LL_UCPD_WriteReg(config->ucpd_port, IMR,
				 imr & ~UCPD_IMR_TX_INT_MASK);
	}
}

/**
 * @brief Initializes the RX and TX state machine variables
 */
static void stm32_ucpd_state_init(const struct device *dev)
{
	struct tcpc_data *data = dev->data;

	/* Init variables used to manage tx process */
	data->ucpd_tx_request = 0;
	data->tx_retry_count = 0;
	data->ucpd_tx_state = STATE_IDLE;

	/* Init variables used to manage rx */
	data->ucpd_rx_sop_prime_enabled = false;
	data->ucpd_rx_msg_active = false;
	data->ucpd_rx_bist_mode = false;

	/* Vconn tracking variable */
	data->ucpd_vconn_enable = false;
}

/**
 * @brief Get the CC enable mask. The mask indicates which CC line
 *        is enabled.
 *
 * @retval CC Enable mask (bit 0: CC1, bit 1: CC2)
 */
static uint32_t ucpd_get_cc_enable_mask(const struct device *dev)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;
	uint32_t mask = UCPD_CR_CCENABLE_Msk;

	/*
	 * When VCONN is enabled, it is supplied on the CC line that's
	 * not being used for Power Delivery messages.
	 */
	if (data->ucpd_vconn_enable) {
		uint32_t cr = LL_UCPD_ReadReg(config->ucpd_port, CR);
		int pol = (cr & UCPD_CR_PHYCCSEL);

		/* Dissable CC line that's used for VCONN */
		mask &= ~BIT(UCPD_CR_CCENABLE_Pos + !pol);
	}

	return mask;
}

/**
 * @brief Get the state of the CC1 and CC2 lines
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_get_cc(const struct device *dev,
		       enum tc_cc_voltage_state *cc1,
		       enum tc_cc_voltage_state *cc2)
{
	const struct tcpc_config *const config = dev->config;
	int vstate_cc1;
	int vstate_cc2;
	int anamode;
	uint32_t sr;
	uint32_t cc_msk;

	/*
	 * cc_voltage_state is determined from vstate_cc bit field in the
	 * status register. The meaning of the value vstate_cc depends on
	 * current value of ANAMODE (src/snk).
	 *
	 * vstate_cc maps directly to cc_state from tcpci spec when
	 * ANAMODE(snk) = 1, but needs to be modified slightly for case
	 * ANAMODE(src) = 0.
	 *
	 * If presenting Rp (source), then need to do a circular shift of
	 * vstate_ccx value:
	 *     vstate_cc | cc_state
	 *     ------------------
	 *        0     ->    1
	 *        1     ->    2
	 *        2     ->    0
	 */

	/* Get vstate_ccx values and power role */
	sr = LL_UCPD_ReadReg(config->ucpd_port, SR);

	/* Get Rp or Rd active */
	anamode = LL_UCPD_GetRole(config->ucpd_port);
	vstate_cc1 = (sr & UCPD_SR_TYPEC_VSTATE_CC1_Msk) >>
		     UCPD_SR_TYPEC_VSTATE_CC1_Pos;

	vstate_cc2 = (sr & UCPD_SR_TYPEC_VSTATE_CC2_Msk) >>
		     UCPD_SR_TYPEC_VSTATE_CC2_Pos;

	/* Do circular shift if port == source */
	if (anamode) {
		if (vstate_cc1 != STM32_UCPD_SR_VSTATE_RA) {
			vstate_cc1 += 4;
		}
		if (vstate_cc2 != STM32_UCPD_SR_VSTATE_RA) {
			vstate_cc2 += 4;
		}
	} else {
		if (vstate_cc1 != STM32_UCPD_SR_VSTATE_OPEN) {
			vstate_cc1 = (vstate_cc1 + 1) % 3;
		}
		if (vstate_cc2 != STM32_UCPD_SR_VSTATE_OPEN) {
			vstate_cc2 = (vstate_cc2 + 1) % 3;
		}
	}

	/* CC connection detection */
	cc_msk = ucpd_get_cc_enable_mask(dev);

	/* CC1 connection detection */
	if (cc_msk & UCPD_CR_CCENABLE_0) {
		*cc1 = vstate_cc1;
	} else {
		*cc1 = TC_CC_VOLT_OPEN;
	}

	/* CC2 connection detection */
	if (cc_msk & UCPD_CR_CCENABLE_1) {
		*cc2 = vstate_cc2;
	} else {
		*cc2 = TC_CC_VOLT_OPEN;
	}

	return 0;
}

/**
 * @brief Enable or Disable VCONN
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOTSUP if not supported
 */
static int ucpd_set_vconn(const struct device *dev, bool enable)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;
	int cr;
	int ret;

	if (data->vconn_cb == NULL) {
		return -ENOTSUP;
	}

	/* Update VCONN on/off status. Do this before getting cc enable mask */
	data->ucpd_vconn_enable = enable;

	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);
	cr &= ~UCPD_CR_CCENABLE_Msk;
	cr |= ucpd_get_cc_enable_mask(dev);

	/* Apply cc pull resistor change */
	LL_UCPD_WriteReg(config->ucpd_port, CR, cr);

#ifdef CONFIG_SOC_SERIES_STM32G0X
	update_stm32g0x_cc_line(config->ucpd_port);
#endif

	/* Get CC line that VCONN is active on */
	data->ucpd_vconn_cc = (cr & UCPD_CR_CCENABLE_0) ?
				TC_POLARITY_CC2 : TC_POLARITY_CC1;

	/* Call user supplied callback to set vconn */
	ret = data->vconn_cb(dev, data->ucpd_vconn_cc, enable);

	return ret;
}

/**
 * @brief Discharge VCONN
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOTSUP if not supported
 */
static int ucpd_vconn_discharge(const struct device *dev, bool enable)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;

	/* VCONN should not be discharged while it's enabled */
	if (data->ucpd_vconn_enable) {
		return -EIO;
	}

	if (data->vconn_discharge_cb) {
		/* Use DPM supplied VCONN Discharge */
		return data->vconn_discharge_cb(dev, data->ucpd_vconn_cc, enable);
	}

	/* Use TCPC VCONN Discharge */
	if (enable) {
		LL_UCPD_VconnDischargeEnable(config->ucpd_port);
	} else {
		LL_UCPD_VconnDischargeDisable(config->ucpd_port);
	}

#ifdef CONFIG_SOC_SERIES_STM32G0X
	update_stm32g0x_cc_line(config->ucpd_port);
#endif

	return 0;
}

/**
 * @brief Sets the value of the CC pull up resistor used when operating as a Source
 *
 * @retval 0 on success
 */
static int ucpd_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	struct tcpc_data *data = dev->data;

	data->rp = rp;

	return 0;
}

/**
 * @brief Gets the value of the CC pull up resistor used when operating as a Source
 *
 * @retval 0 on success
 */
static int ucpd_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	struct tcpc_data *data = dev->data;

	*rp = data->rp;

	return 0;
}

/**
 * @brief Enable or disable Dead Battery resistors
 */
static void dead_battery(const struct device *dev, bool en)
{
	struct tcpc_data *data = dev->data;

#ifdef CONFIG_SOC_SERIES_STM32G0X
	const struct tcpc_config *const config = dev->config;
	uint32_t cr;

	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);

	if (en) {
		cr |= UCPD_CR_DBATTEN;
	} else {
		cr &= ~UCPD_CR_DBATTEN;
	}

	LL_UCPD_WriteReg(config->ucpd_port, CR, cr);
	update_stm32g0x_cc_line(config->ucpd_port);
#else
	if (en) {
		CLEAR_BIT(PWR->CR3, PWR_CR3_UCPD_DBDIS);
	} else {
		SET_BIT(PWR->CR3, PWR_CR3_UCPD_DBDIS);
	}
#endif
	data->dead_battery_active = en;
}

/**
 * @brief Set the CC pull up or pull down resistors
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_set_cc(const struct device *dev,
		       enum tc_cc_pull cc_pull)
{
	const struct tcpc_config *const config = dev->config;
	struct tcpc_data *data = dev->data;
	uint32_t cr;

	/* Disable dead battery if it's active */
	if (data->dead_battery_active) {
		dead_battery(dev, false);
	}

	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);

	/*
	 * Always set ANASUBMODE to match desired Rp. TCPM layer has a valid
	 * range of 0, 1, or 2. This range maps to 1, 2, or 3 in ucpd for
	 * ANASUBMODE.
	 */
	cr &= ~UCPD_CR_ANASUBMODE_Msk;
	cr |= STM32_UCPD_CR_ANASUBMODE_VAL(UCPD_RP_TO_ANASUB(data->rp));

	/* Disconnect both pull from both CC lines for R_open case */
	cr &= ~UCPD_CR_CCENABLE_Msk;
	/* Set ANAMODE if cc_pull is Rd */
	if (cc_pull == TC_CC_RD) {
		cr |= (UCPD_CR_ANAMODE | UCPD_CR_CCENABLE_Msk);
		/* Clear ANAMODE if cc_pull is Rp */
	} else if (cc_pull == TC_CC_RP) {
		cr &= ~(UCPD_CR_ANAMODE);
		cr |= ucpd_get_cc_enable_mask(dev);
	}

	/* Update pull values */
	LL_UCPD_WriteReg(config->ucpd_port, CR, cr);

#ifdef CONFIG_SOC_SERIES_STM32G0X
	update_stm32g0x_cc_line(config->ucpd_port);
#endif

	return 0;
}

/**
 * @brief Set the polarity of the CC line, which is the active CC line
 *        used for power delivery.
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOTSUP if polarity is not supported
 */
static int ucpd_cc_set_polarity(const struct device *dev,
				enum tc_cc_polarity polarity)
{
	const struct tcpc_config *const config = dev->config;
	uint32_t cr;

	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);

	/*
	 * Polarity impacts the PHYCCSEL, CCENABLE, and CCxTCDIS fields. This
	 * function is called when polarity is updated at TCPM layer. STM32Gx
	 * only supports POLARITY_CC1 or POLARITY_CC2 and this is stored in the
	 * PHYCCSEL bit in the CR register.
	 */

	if (polarity == TC_POLARITY_CC1) {
		cr &= ~UCPD_CR_PHYCCSEL;
	} else if (polarity == TC_POLARITY_CC2) {
		cr |= UCPD_CR_PHYCCSEL;
	} else {
		return -ENOTSUP;
	}

	/* Update polarity */
	LL_UCPD_WriteReg(config->ucpd_port, CR, cr);

	return 0;
}

/**
 * @brief Enable or Disable Power Delivery
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_set_rx_enable(const struct device *dev, bool enable)
{
	const struct tcpc_config *const config = dev->config;
	uint32_t imr;
	uint32_t cr;

	imr = LL_UCPD_ReadReg(config->ucpd_port, IMR);
	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);

	/*
	 * USB PD receiver enable is controlled by the bit PHYRXEN in
	 * UCPD_CR. Enable Rx interrupts when RX PD decoder is active.
	 */
	if (enable) {
		/* Clear the RX alerts bits */
		LL_UCPD_WriteReg(config->ucpd_port, ICR, UCPD_ICR_RX_INT_MASK);
		imr |= UCPD_IMR_RX_INT_MASK;
		cr |= UCPD_CR_PHYRXEN;
		LL_UCPD_WriteReg(config->ucpd_port, IMR, imr);
		LL_UCPD_WriteReg(config->ucpd_port, CR, cr);
	} else {
		imr &= ~UCPD_IMR_RX_INT_MASK;
		cr &= ~UCPD_CR_PHYRXEN;
		LL_UCPD_WriteReg(config->ucpd_port, CR, cr);
		LL_UCPD_WriteReg(config->ucpd_port, IMR, imr);
	}

	return 0;
}

/**
 * @brief Set the Power and Data role used when sending goodCRC messages
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_set_roles(const struct device *dev,
			  enum tc_power_role power_role,
			  enum tc_data_role data_role)
{
	struct tcpc_data *data = dev->data;

	data->msg_header.pr = power_role;
	data->msg_header.dr = data_role;

	return 0;
}

/**
 * @brief Enable the reception of SOP Prime messages
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_sop_prime_enable(const struct device *dev, bool enable)
{
	struct tcpc_data *data = dev->data;

	/* Update static variable used to filter SOP//SOP'' messages */
	data->ucpd_rx_sop_prime_enabled = enable;

	return 0;
}

/**
 * @brief State transmitting a message
 */
static void ucpd_start_transmit(const struct device *dev,
				enum ucpd_tx_msg msg_type)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;
	enum pd_packet_type type;
	uint32_t cr;
	uint32_t imr;

	cr = LL_UCPD_ReadReg(config->ucpd_port, CR);

	/* Select the correct tx descriptor */
	data->ucpd_tx_active_buffer = &data->ucpd_tx_buffers[msg_type];
	type = data->ucpd_tx_active_buffer->type;

	if (type == PD_PACKET_TX_HARD_RESET) {
		/*
		 * From RM0440 45.4.4:
		 * In order to facilitate generation of a Hard Reset, a special
		 * code of TXMODE field is used. No other fields need to be
		 * written. On writing the correct code, the hardware forces
		 * Hard Reset Tx under the correct (optimal) timings with
		 * respect to an on-going Tx message, which (if still in
		 * progress) is cleanly terminated by truncating the current
		 * sequence and directly appending an EOP K-code sequence. No
		 * specific interrupt is generated relating to this truncation
		 * event.
		 *
		 * Because Hard Reset can interrupt ongoing Tx operations, it is
		 * started differently than all other tx messages. Only need to
		 * enable hard reset interrupts, and then set a bit in the CR
		 * register to initiate.
		 */
		/* Enable interrupt for Hard Reset sent/discarded */
		LL_UCPD_WriteReg(config->ucpd_port, ICR,
				 UCPD_ICR_HRSTDISCCF | UCPD_ICR_HRSTSENTCF);

		imr = LL_UCPD_ReadReg(config->ucpd_port, IMR);
		imr |= UCPD_IMR_HRSTDISCIE | UCPD_IMR_HRSTSENTIE;
		LL_UCPD_WriteReg(config->ucpd_port, IMR, imr);

		/* Initiate Hard Reset */
		cr |= UCPD_CR_TXHRST;
		LL_UCPD_WriteReg(config->ucpd_port, CR, cr);
	} else if (type != PD_PACKET_MSG_INVALID) {
		int msg_len = 0;
		int mode;

		/*
		 * These types are normal transmission, TXMODE = 0. To transmit
		 * regular message, control or data, requires the following:
		 *     1. Set TXMODE:
		 *          Normal -> 0
		 *          Cable Reset -> 1
		 *          Bist -> 2
		 *     2. Set TX_ORDSETR based on message type
		 *     3. Set TX_PAYSZR which must account for 2 bytes of header
		 *     4. Configure DMA (optional if DMA is desired)
		 *     5. Enable transmit interrupts
		 *     6. Start TX by setting TXSEND in CR
		 *
		 */

		/*
		 * Set tx length parameter (in bytes). Note the count field in
		 * the header is number of 32 bit objects. Also, the length
		 * field must account for the 2 header bytes.
		 */
		if (type == PD_PACKET_TX_BIST_MODE_2) {
			mode = LL_UCPD_TXMODE_BIST_CARRIER2;
		} else if (type == PD_PACKET_CABLE_RESET) {
			mode = LL_UCPD_TXMODE_CABLE_RESET;
		} else {
			mode = LL_UCPD_TXMODE_NORMAL;
			msg_len = data->ucpd_tx_active_buffer->msg_len;
		}

		LL_UCPD_WriteTxPaySize(config->ucpd_port, msg_len);

		/* Set tx mode */
		cr &= ~UCPD_CR_TXMODE_Msk;
		cr |= mode;
		LL_UCPD_WriteReg(config->ucpd_port, CR, cr);

		/* Index into ordset enum for start of packet */
		if (type <= PD_PACKET_CABLE_RESET) {
			LL_UCPD_WriteTxOrderSet(config->ucpd_port,
						ucpd_txorderset[type]);
		}

		/* Reset msg byte index */
		data->ucpd_tx_active_buffer->msg_index = 0;

		/* Enable interrupts */
		ucpd_tx_interrupts_enable(dev, 1);

		/* Trigger ucpd peripheral to start pd message transmit */
		LL_UCPD_SendMessage(config->ucpd_port);
	}
}

/**
 * @brief Set the current state of the TX state machine
 */
static void ucpd_set_tx_state(const struct device *dev, enum ucpd_state state)
{
	struct tcpc_data *data = dev->data;

	data->ucpd_tx_state = state;
}

/**
 * @brief Wrapper function for calling alert handler
 */
static void ucpd_notify_handler(struct alert_info *info, enum tcpc_alert alert)
{
	if (info->handler) {
		info->handler(info->dev, info->data, alert);
	}
}

/**
 * @brief This is the TX state machine
 */
static void ucpd_manage_tx(struct alert_info *info)
{
	struct tcpc_data *data = info->dev->data;
	enum ucpd_tx_msg msg_src = TX_MSG_NONE;
	union pd_header hdr;

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_HR_REQ)) {
		/*
		 * Hard reset control messages are treated as a priority. The
		 * control message will already be set up as it comes from the
		 * PRL layer like any other PD ctrl/data message. So just need
		 * to indicate the correct message source and set the state to
		 * hard reset here.
		 */
		ucpd_set_tx_state(info->dev, STATE_HARD_RESET);
		msg_src = TX_MSG_TCPM;
		data->ucpd_tx_request &= ~BIT(msg_src);
	}

	switch (data->ucpd_tx_state) {
	case STATE_IDLE:
		if (data->ucpd_tx_request & MSG_GOOD_CRC_MASK) {
			ucpd_set_tx_state(info->dev, STATE_ACTIVE_CRC);
			msg_src = TX_MSG_GOOD_CRC;
		} else if (data->ucpd_tx_request & MSG_TCPM_MASK) {
			if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_RX_MSG)) {
				/*
				 * USB-PD Specification rev 3.0, section 6.10
				 * On receiving a received message, the protocol
				 * layer shall discard any pending message.
				 *
				 * Since the pending message from the PRL has
				 * not been sent yet, it needs to be discarded
				 * based on the received message event.
				 */
				ucpd_notify_handler(info, TCPC_ALERT_TRANSMIT_MSG_DISCARDED);
				data->ucpd_tx_request &= ~MSG_TCPM_MASK;
			} else if (!data->ucpd_rx_msg_active) {
				ucpd_set_tx_state(info->dev, STATE_ACTIVE_TCPM);
				msg_src = TX_MSG_TCPM;
				/* Save msgID required for GoodCRC check */
				hdr.raw_value =
					data->ucpd_tx_buffers[TX_MSG_TCPM].data.header;
				data->msg_id_match = hdr.message_id;
				data->tx_retry_max = hdr.specification_revision == PD_REV30 ?
						     UCPD_N_RETRY_COUNT_REV30 :
						     UCPD_N_RETRY_COUNT_REV20;
			}
		}

		/* If state is not idle, then start tx message */
		if (data->ucpd_tx_state != STATE_IDLE) {
			data->ucpd_tx_request &= ~BIT(msg_src);
			data->tx_retry_count = 0;
		}
		break;

	case STATE_ACTIVE_TCPM:
		/*
		 * Check if tx msg has finished. For TCPM messages
		 * transmit is not complete until a GoodCRC message
		 * matching the msgID just sent is received. But, a tx
		 * message can fail due to collision or underrun,
		 * etc. If that failure occurs, dont' wait for GoodCrc
		 * and just go to failure path.
		 */
		if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TX_MSG_SUCCESS)) {
			ucpd_set_tx_state(info->dev, STATE_WAIT_CRC_ACK);
			/* Start the GoodCRC RX Timer */
			k_timer_start(&data->goodcrc_rx_timer, K_USEC(1000), K_NO_WAIT);
		} else if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TX_MSG_DISC) ||
			  atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TX_MSG_FAIL)) {
			if (data->tx_retry_count < data->tx_retry_max) {
				if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_RX_MSG)) {
					/*
					 * A message was received so there is no
					 * need to retry this tx message which
					 * had failed to send previously.
					 * Likely, due to the wire
					 * being active from the message that
					 * was just received.
					 */
					ucpd_set_tx_state(info->dev,
							  STATE_IDLE);
					ucpd_notify_handler(info,
							TCPC_ALERT_TRANSMIT_MSG_DISCARDED);
					ucpd_set_tx_state(info->dev,
							  STATE_IDLE);
				} else {
					/*
					 * Tx attempt failed. Remain in this
					 * state, but trigger new tx attempt.
					 */
					msg_src = TX_MSG_TCPM;
					data->tx_retry_count++;
				}
			} else {
				enum tcpc_alert status;

				status = (atomic_test_and_clear_bit(&info->evt,
								UCPD_EVT_TX_MSG_FAIL)) ?
					 TCPC_ALERT_TRANSMIT_MSG_FAILED :
					 TCPC_ALERT_TRANSMIT_MSG_DISCARDED;
				ucpd_set_tx_state(info->dev, STATE_IDLE);
				ucpd_notify_handler(info, status);
			}
		}
		break;

	case STATE_ACTIVE_CRC:
		if (atomic_test_bit(&info->evt, UCPD_EVT_TX_MSG_SUCCESS) ||
				atomic_test_bit(&info->evt, UCPD_EVT_TX_MSG_FAIL) ||
				atomic_test_bit(&info->evt, UCPD_EVT_TX_MSG_DISC)) {
			atomic_clear_bit(&info->evt, UCPD_EVT_TX_MSG_SUCCESS);
			atomic_clear_bit(&info->evt, UCPD_EVT_TX_MSG_FAIL);
			atomic_clear_bit(&info->evt, UCPD_EVT_TX_MSG_DISC);
			ucpd_set_tx_state(info->dev, STATE_IDLE);
			if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TX_MSG_FAIL)) {
				LOG_INF("ucpd: Failed to send GoodCRC!");
			} else if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TX_MSG_DISC)) {
				LOG_INF("ucpd: GoodCRC message discarded!");
			}
		}
		break;

	case STATE_WAIT_CRC_ACK:
		if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_RX_GOOD_CRC) &&
		    data->ucpd_crc_id == data->msg_id_match) {
			/* GoodCRC with matching ID was received */
			ucpd_notify_handler(info, TCPC_ALERT_TRANSMIT_MSG_SUCCESS);
			ucpd_set_tx_state(info->dev, STATE_IDLE);
		} else if (k_timer_status_get(&data->goodcrc_rx_timer)) {
			/* Stop the GoodCRC RX Timer */
			k_timer_stop(&data->goodcrc_rx_timer);

			/* GoodCRC w/out match or timeout waiting */
			if (data->tx_retry_count < data->tx_retry_max) {
				ucpd_set_tx_state(info->dev, STATE_ACTIVE_TCPM);
				msg_src = TX_MSG_TCPM;
				data->tx_retry_count++;
			} else {
				ucpd_set_tx_state(info->dev, STATE_IDLE);
				ucpd_notify_handler(info, TCPC_ALERT_TRANSMIT_MSG_FAILED);
			}
		} else if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_RX_MSG)) {
			/*
			 * In the case of a collision, it's possible the port
			 * partner may not send a GoodCRC and instead send the
			 * message that was colliding. If a message is received
			 * in this state, then treat it as a discard from an
			 * incoming message.
			 */
			ucpd_notify_handler(info, TCPC_ALERT_TRANSMIT_MSG_DISCARDED);
			ucpd_set_tx_state(info->dev, STATE_IDLE);
		}
		break;

	case STATE_HARD_RESET:
		if (atomic_test_bit(&info->evt, UCPD_EVT_HR_DONE) ||
		    atomic_test_bit(&info->evt, UCPD_EVT_HR_FAIL)) {
			atomic_clear_bit(&info->evt, UCPD_EVT_HR_DONE);
			atomic_clear_bit(&info->evt, UCPD_EVT_HR_FAIL);
			/* HR complete, reset tx state values */
			ucpd_set_tx_state(info->dev, STATE_IDLE);
			data->ucpd_tx_request = 0;
			data->tx_retry_count = 0;
		}
		break;
	}

	/*
	 * NOTE: TX_MSG_GOOD_CRC messages are sent from the ISR to reduce latency
	 * when sending those messages, so don't resend them here.
	 *
	 * If msg_src is valid and not a TX_MSG_GOOD_CRC, then start transmit.
	 */
	if (msg_src != TX_MSG_GOOD_CRC && msg_src > TX_MSG_NONE) {
		ucpd_start_transmit(info->dev, msg_src);
	}
}

/**
 * @brief Alert handler
 */
static void ucpd_alert_handler(struct k_work *item)
{
	struct alert_info *info = CONTAINER_OF(item, struct alert_info, work);
	struct tcpc_data *data = info->dev->data;

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_EVENT_CC)) {
		ucpd_notify_handler(info, TCPC_ALERT_CC_STATUS);
	}

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_HARD_RESET_RECEIVED)) {
		ucpd_notify_handler(info, TCPC_ALERT_HARD_RESET_RECEIVED);
	}

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_RX_MSG)) {
		ucpd_notify_handler(info, TCPC_ALERT_MSG_STATUS);
	}

	/*
	 * USB-PD messages are initiated in TCPM stack (PRL
	 * layer). However, GoodCRC messages are initiated within the
	 * UCPD driver based on USB-PD rx messages. These 2 types of
	 * transmit paths are managed via events.
	 *
	 * UCPD generated GoodCRC messages, are the priority path as
	 * they must be sent immediately following a successful USB-PD
	 * rx message. As long as a transmit operation is not underway,
	 * then a transmit message will be started upon request. The ISR
	 * routine sets the event to indicate that the transmit
	 * operation is complete.
	 *
	 * Hard reset requests are sent as a TCPM message, but in terms
	 * of the ucpd transmitter, they are treated as a 3rd tx msg
	 * source since they can interrupt an ongoing tx msg, and there
	 * is no requirement to wait for a GoodCRC reply message.
	 */

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_GOOD_CRC_REQ)) {
		data->ucpd_tx_request |= MSG_GOOD_CRC_MASK;
	}

	if (atomic_test_and_clear_bit(&info->evt, UCPD_EVT_TCPM_MSG_REQ)) {
		data->ucpd_tx_request |= MSG_TCPM_MASK;
	}

	/*
	 * Manage PD tx messages. The state machine may need to be
	 * called more than once. For instance, if
	 * the task is woken at the completion of sending a GoodCRC,
	 * there may be a TCPM message request pending and just changing
	 * the state back to idle would not trigger start of transmit.
	 */
	do {
		ucpd_manage_tx(info);
	} while (data->ucpd_tx_state != STATE_IDLE);
}

/**
 * @brief Sends a goodCRC message
 */
static void ucpd_send_good_crc(const struct device *dev,
			       union pd_header rx_header)
{
	struct tcpc_data *data = dev->data;
	const struct tcpc_config *const config = dev->config;
	union pd_header tx_header;
	enum pd_packet_type tx_type;
	struct alert_info *info = &data->alert_info;

	/*
	 * A GoodCRC message shall be sent by receiver to ack that the previous
	 * message was correctly received. The GoodCRC message shall return the
	 * rx message's msg_id field. The one exception is for GoodCRC messages,
	 * which do not generate a GoodCRC response
	 */
	if (ucpd_msg_is_good_crc(rx_header)) {
		return;
	}

	/*
	 * Get the rx ordered set code just detected. SOP -> SOP''_Debug are in
	 * the same order as enum tcpc_packet_type and so can be used
	 * directly.
	 */
	tx_type = LL_UCPD_ReadRxOrderSet(config->ucpd_port);

	/*
	 * PD Header(SOP):
	 *   Extended   b15    -> set to 0 for control messages
	 *   Count      b14:12 -> number of 32 bit data objects = 0 for ctrl msg
	 *   MsgID      b11:9  -> running byte counter (extracted from rx msg)
	 *   Power Role b8     -> stored in static, from set_msg_header()
	 *   Spec Rev   b7:b6  -> PD spec revision (extracted from rx msg)
	 *   Data Role  b5     -> stored in static, from set_msg_header
	 *   Msg Type   b4:b0  -> data or ctrl type = PD_CTRL_GOOD_CRC
	 */
	/* construct header message */
	tx_header.message_type = PD_CTRL_GOOD_CRC;
	if (tx_type == PD_PACKET_SOP) {
		tx_header.port_power_role = data->msg_header.pr;
		tx_header.port_data_role = data->msg_header.dr;
	} else {
		tx_header.port_power_role = 0;
		tx_header.port_data_role = 0;
	}
	tx_header.message_id = rx_header.message_id;
	tx_header.number_of_data_objects = 0;
	tx_header.specification_revision = rx_header.specification_revision;
	tx_header.extended = 0;

	/* Good CRC is header with no other objects */
	data->ucpd_tx_buffers[TX_MSG_GOOD_CRC].msg_len = MSG_HEADER_SIZE;
	data->ucpd_tx_buffers[TX_MSG_GOOD_CRC].data.header =
		tx_header.raw_value;
	data->ucpd_tx_buffers[TX_MSG_GOOD_CRC].type = tx_type;

	/* Notify ucpd task that a GoodCRC message tx request is pending */
	atomic_set_bit(&info->evt, UCPD_EVT_GOOD_CRC_REQ);

	/* Send TX_MSG_GOOD_CRC message here to reduce latency */
	ucpd_start_transmit(dev, TX_MSG_GOOD_CRC);
}

/**
 * @brief Transmit a power delivery message
 *
 * @retval 0 on success
 * @retval -EFAULT on failure
 */
static int ucpd_transmit_data(const struct device *dev,
			      struct pd_msg *msg)
{
	struct tcpc_data *data = dev->data;

	/* Length in bytes = (4 * object len) + 2 header bytes */
	int len = PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(msg->header.number_of_data_objects) + 2;

	if (len > UCPD_BUF_LEN) {
		return -EFAULT;
	}

	/* Store tx msg info in TCPM msg descriptor */
	data->ucpd_tx_buffers[TX_MSG_TCPM].msg_len = len;
	data->ucpd_tx_buffers[TX_MSG_TCPM].type = msg->type;
	data->ucpd_tx_buffers[TX_MSG_TCPM].data.header = msg->header.raw_value;

	/* Copy msg objects to ucpd data buffer, after 2 header bytes */
	memcpy(data->ucpd_tx_buffers[TX_MSG_TCPM].data.msg + 2,
	       (uint8_t *)msg->data, len - 2);

	/*
	 * Check for hard reset message here. A different event is used for hard
	 * resets as they are able to interrupt ongoing transmit, and should
	 * have priority over any pending message.
	 */
	if (msg->type == PD_PACKET_TX_HARD_RESET) {
		atomic_set_bit(&data->alert_info.evt, UCPD_EVT_HR_REQ);
	} else {
		atomic_set_bit(&data->alert_info.evt, UCPD_EVT_TCPM_MSG_REQ);
	}

	/* Start transmission */
	k_work_submit(&data->alert_info.work);

	return 0;
}

/**
 * @brief Retrieves the Power Delivery message from the TCPC
 *
 * @retval number of bytes received if msg parameter is provided
 * @retval 0 if there is a message pending and the msg parameter is NULL
 * @retval -ENODATA if there is no pending message
 */
static int ucpd_get_rx_pending_msg(const struct device *dev, struct pd_msg *msg)
{
	struct tcpc_data *data = dev->data;
	int ret = 0;

	/* Make sure we have a message to retrieve */
	if (*(uint32_t *)data->ucpd_rx_buffer == 0) {
		return -ENODATA;
	}

	if (msg == NULL) {
		return 0;
	}

	msg->type = *(uint16_t *)data->ucpd_rx_buffer;
	msg->header.raw_value = *((uint16_t *)data->ucpd_rx_buffer + 1);
	msg->len = PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(msg->header.number_of_data_objects);
	memcpy(msg->data, (data->ucpd_rx_buffer +
			   PACKET_TYPE_SIZE +
			   MSG_HEADER_SIZE), msg->len);
	ret = msg->len + MSG_HEADER_SIZE;

	/* All done. Clear type and header */
	*(uint32_t *)data->ucpd_rx_buffer = 0;

	return ret;
}

/**
 * @brief Enable or Disable BIST Test mode
 *
 * return 0 on success
 * return -EIO on failure
 */
static int ucpd_set_bist_test_mode(const struct device *dev,
				   bool enable)
{
	struct tcpc_data *data = dev->data;

	data->ucpd_rx_bist_mode = enable;
	LOG_INF("ucpd: Bist test mode = %d", enable);

	return 0;
}

/**
 * @brief UCPD interrupt handler
 */
static void ucpd_isr(const struct device *dev_inst[])
{
	const struct device *dev;
	const struct tcpc_config *config;
	struct tcpc_data *data;
	uint32_t sr;
	struct alert_info *info;
	uint32_t tx_done_mask = UCPD_SR_TXUND |
				UCPD_SR_TXMSGSENT |
				UCPD_SR_TXMSGABT |
				UCPD_SR_TXMSGDISC |
				UCPD_SR_HRSTSENT |
				UCPD_SR_HRSTDISC;

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
	/*
	 * Multiple UCPD ports are available
	 */

	uint32_t sr0;
	uint32_t sr1;

	/*
	 * Since the UCPD peripherals share the same interrupt line, determine
	 * which one generated the interrupt.
	 */

	/* Read UCPD1 and UCPD2 Status Registers */

	sr0 =
	LL_UCPD_ReadReg(((const struct tcpc_config *)dev_inst[0]->config)->ucpd_port, SR);
	sr1 =
	LL_UCPD_ReadReg(((const struct tcpc_config *)dev_inst[1]->config)->ucpd_port, SR);

	if (sr0) {
		dev = dev_inst[0];
	} else if (sr1) {
		dev = dev_inst[1];
	} else {
		/*
		 * The interrupt was triggered by some other device sharing this
		 * interrupt line.
		 */
		return;
	}
#else
	/*
	 * Only one UCPD port available
	 */

	dev = dev_inst[0];
#endif /* Get the UCPD port that initiated that interrupt  */

	config = dev->config;
	data = dev->data;
	info = &data->alert_info;

	/* Read the status register */
	sr = LL_UCPD_ReadReg(config->ucpd_port, SR);

	/* Check for CC events, set event to wake PD task */
	if (sr & (UCPD_SR_TYPECEVT1 | UCPD_SR_TYPECEVT2)) {
		/* Set CC event bit */
		atomic_set_bit(&info->evt, UCPD_EVT_EVENT_CC);
	}

	/*
	 * Check for Tx events. tx_mask includes all status bits related to the
	 * end of a USB-PD tx message. If any of these bits are set, the
	 * transmit attempt is completed. Set an event to notify ucpd tx state
	 * machine that transmit operation is complete.
	 */
	if (sr & tx_done_mask) {
		/* Check for tx message complete */
		if (sr & UCPD_SR_TXMSGSENT) {
			atomic_set_bit(&info->evt, UCPD_EVT_TX_MSG_SUCCESS);
		} else if (sr & (UCPD_SR_TXMSGABT | UCPD_SR_TXUND)) {
			atomic_set_bit(&info->evt, UCPD_EVT_TX_MSG_FAIL);
		} else if (sr & (UCPD_SR_TXMSGDISC | UCPD_SR_HRSTDISC)) {
			atomic_set_bit(&info->evt, UCPD_EVT_TX_MSG_DISC);
		} else if (sr & UCPD_SR_HRSTSENT) {
			atomic_set_bit(&info->evt, UCPD_EVT_HR_DONE);
		} else if (sr & UCPD_SR_HRSTDISC) {
			atomic_set_bit(&info->evt, UCPD_EVT_HR_FAIL);
		}

		/* Disable Tx interrupts */
		ucpd_tx_interrupts_enable(dev, 0);
	}

	/* Check for data register empty */
	if (sr & UCPD_SR_TXIS) {
		ucpd_tx_data_byte(dev);
	}

	/* Check for Rx Events */
	/* Check first for start of new message */
	if (sr & UCPD_SR_RXORDDET) {
		/* Add message type to pd message buffer */
		*(uint16_t *)data->ucpd_rx_buffer =
			LL_UCPD_ReadRxOrderSet(config->ucpd_port);

		data->ucpd_rx_byte_count = 2;
		data->ucpd_rx_msg_active = true;
	}
	/* Check for byte received */
	if (sr & UCPD_SR_RXNE) {
		ucpd_rx_data_byte(dev);
	}

	/* Check for end of message */
	if (sr & UCPD_SR_RXMSGEND) {
		data->ucpd_rx_msg_active = false;
		/* Check for errors */
		if (!(sr & UCPD_SR_RXERR)) {
			enum pd_packet_type type;
			union pd_header rx_header;
			int good_crc;

			type = *(uint16_t *)data->ucpd_rx_buffer;
			rx_header.raw_value =
				*((uint16_t *)data->ucpd_rx_buffer + 1);
			good_crc = ucpd_msg_is_good_crc(rx_header);

			/*
			 * Don't pass GoodCRC control messages to the TCPM
			 * layer. In addition, need to filter for SOP'/SOP''
			 * packets if those are not enabled. SOP'/SOP''
			 * reception is controlled by a static variable. The
			 * hardware orderset detection pattern can't be changed
			 * without disabling the ucpd peripheral.
			 */
			if (!good_crc && (data->ucpd_rx_sop_prime_enabled ||
					  type == PD_PACKET_SOP)) {

				/*
				 * If BIST test mode is active, then still need
				 * to send GoodCRC reply, but there is no need
				 * to send the message up to the tcpm layer.
				 */
				if (!data->ucpd_rx_bist_mode) {
					atomic_set_bit(&info->evt, UCPD_EVT_RX_MSG);
				}
				/* Send GoodCRC message (if required) */
				ucpd_send_good_crc(dev, rx_header);
			} else if (good_crc) {
				atomic_set_bit(&info->evt, UCPD_EVT_RX_GOOD_CRC);
				data->ucpd_crc_id = rx_header.message_id;
			}
		} else {
			/* Rx message is complete, but there were bit errors */
			LOG_ERR("ucpd: rx message error");
		}
	}
	/* Check for fault conditions */
	if (sr & UCPD_SR_RXHRSTDET) {
		/* hard reset received */
		atomic_set_bit(&info->evt, UCPD_EVT_HARD_RESET_RECEIVED);
	}

	/* Clear interrupts now that PD events have been set */
	LL_UCPD_WriteReg(config->ucpd_port, ICR, sr & UCPD_ICR_ALL_INT_MASK);

	/* Notify application of events */
	k_work_submit(&info->work);
}

/**
 * @brief Dump a set of TCPC registers
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_dump_std_reg(const struct device *dev)
{
	const struct tcpc_config *const config = dev->config;

	LOG_INF("CFGR1: %08x", LL_UCPD_ReadReg(config->ucpd_port, CFG1));
	LOG_INF("CFGR2: %08x", LL_UCPD_ReadReg(config->ucpd_port, CFG2));
	LOG_INF("CR:    %08x", LL_UCPD_ReadReg(config->ucpd_port, CR));
	LOG_INF("IMR:   %08x", LL_UCPD_ReadReg(config->ucpd_port, IMR));
	LOG_INF("SR:    %08x", LL_UCPD_ReadReg(config->ucpd_port, SR));
	LOG_INF("ICR:   %08x\n", LL_UCPD_ReadReg(config->ucpd_port, ICR));

	return 0;
}

/**
 * @brief Sets the alert function that's called when an interrupt is triggered
 *        due to a TCPC alert
 *
 * @retval 0 on success
 * @retval -EINVAL on failure
 */
static int ucpd_set_alert_handler_cb(const struct device *dev,
				     tcpc_alert_handler_cb_t handler, void *alert_data)
{
	struct tcpc_data *data = dev->data;

	data->alert_info.handler = handler;
	data->alert_info.data = alert_data;

	return 0;
}

/**
 * @brief Sets a callback that can enable or disable VCONN if the TCPC is
 *        unable to or the system is configured in a way that does not use
 *        the VCONN control capabilities of the TCPC
 *
 */
static void ucpd_set_vconn_cb(const struct device *dev,
			      tcpc_vconn_control_cb_t vconn_cb)
{
	struct tcpc_data *data = dev->data;

	data->vconn_cb = vconn_cb;
}

/**
 * @brief Sets a callback that can discharge VCONN if the TCPC is
 *        unable to or the system is configured in a way that does not use
 *        the VCONN discharge capabilities of the TCPC
 *
 */
static void ucpd_set_vconn_discharge_cb(const struct device *dev,
					tcpc_vconn_discharge_cb_t cb)
{
	struct tcpc_data *data = dev->data;

	data->vconn_discharge_cb = cb;
}

/**
 * @brief UCPD interrupt init
 */
static void ucpd_isr_init(const struct device *dev)
{
	const struct tcpc_config *const config = dev->config;
	struct tcpc_data *data = dev->data;
	struct alert_info *info = &data->alert_info;

	/* Init GoodCRC Receive timer */
	k_timer_init(&data->goodcrc_rx_timer, NULL, NULL);

	/* Disable all alert bits */
	LL_UCPD_WriteReg(config->ucpd_port, IMR, 0);

	/* Clear all alert handler */
	ucpd_set_alert_handler_cb(dev, NULL, NULL);

	/* Save device structure for use in the alert handlers */
	info->dev = dev;

	/* Initialize the work handler */
	k_work_init(&info->work, ucpd_alert_handler);

	/* Configure CC change alerts */
	LL_UCPD_WriteReg(config->ucpd_port, IMR,
			 UCPD_IMR_TYPECEVT1IE | UCPD_IMR_TYPECEVT2IE);
	LL_UCPD_WriteReg(config->ucpd_port, ICR,
			 UCPD_ICR_TYPECEVT1CF | UCPD_ICR_TYPECEVT2CF);

	/* SOP'/SOP'' must be enabled via TCPCI call */
	data->ucpd_rx_sop_prime_enabled = false;

	stm32_ucpd_state_init(dev);

	/* Configure and enable the IRQ */
	config_tcpc_irq();
}

/**
 * @brief Initializes the TCPC
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int ucpd_init(const struct device *dev)
{
	const struct tcpc_config *const config = dev->config;
	struct tcpc_data *data = dev->data;
	uint32_t cfg1;
	int ret;

	LOG_DBG("Pinctrl signals configuration");
	ret = pinctrl_apply_state(config->ucpd_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", ret);
		return ret;
	}

	/*
	 * The UCPD port is disabled in the LL_UCPD_Init function
	 *
	 * NOTE: For proper Power Management operation, this function
	 *       should not be used because it circumvents the zephyr
	 *	 clock API. Instead, DTS clock settings and the zephyr
	 *	 clock API should be used to enable clocks.
	 */
	ret = LL_UCPD_Init(config->ucpd_port,
			   (LL_UCPD_InitTypeDef *)&config->ucpd_params);

	if (ret == SUCCESS) {
		/* Init Rp to USB */
		data->rp = TC_RP_USB;

		/*
		 * Set RXORDSETEN field to control which types of ordered sets the PD
		 * receiver must receive.
		 */
		cfg1 = LL_UCPD_ReadReg(config->ucpd_port, CFG1);
		cfg1 |= LL_UCPD_ORDERSET_SOP | LL_UCPD_ORDERSET_SOP1 |
			LL_UCPD_ORDERSET_SOP2 | LL_UCPD_ORDERSET_HARDRST;
		LL_UCPD_WriteReg(config->ucpd_port, CFG1, cfg1);

		/* Enable UCPD port */
		LL_UCPD_Enable(config->ucpd_port);

		/* Enable Dead Battery Support */
		if (config->ucpd_dead_battery) {
			dead_battery(dev, true);
		} else {
			/*
			 * Some devices have dead battery enabled by default
			 * after power up, so disable it
			 */
			dead_battery(dev, false);
		}

		/* Initialize the isr */
		ucpd_isr_init(dev);
	} else {
		return -EIO;
	}

	return 0;
}

static const struct tcpc_driver_api driver_api = {
	.init = ucpd_init,
	.set_alert_handler_cb = ucpd_set_alert_handler_cb,
	.get_cc = ucpd_get_cc,
	.set_rx_enable = ucpd_set_rx_enable,
	.get_rx_pending_msg = ucpd_get_rx_pending_msg,
	.transmit_data = ucpd_transmit_data,
	.select_rp_value = ucpd_select_rp_value,
	.get_rp_value = ucpd_get_rp_value,
	.set_cc = ucpd_set_cc,
	.set_roles = ucpd_set_roles,
	.set_vconn_cb = ucpd_set_vconn_cb,
	.set_vconn_discharge_cb = ucpd_set_vconn_discharge_cb,
	.set_vconn = ucpd_set_vconn,
	.vconn_discharge = ucpd_vconn_discharge,
	.set_cc_polarity = ucpd_cc_set_polarity,
	.dump_std_reg = ucpd_dump_std_reg,
	.set_bist_test_mode = ucpd_set_bist_test_mode,
	.sop_prime_enable = ucpd_sop_prime_enable,
};

#define DEV_INST_INIT(n) dev_inst[n] = DEVICE_DT_INST_GET(n);
static void config_tcpc_irq(void)
{
	static int inst_num;
	static const struct device
	*dev_inst[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

	/* Initialize and enable shared irq on last instance */
	if (++inst_num == DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)) {
		DT_INST_FOREACH_STATUS_OKAY(DEV_INST_INIT)

		IRQ_CONNECT(DT_INST_IRQN(0),
			    DT_INST_IRQ(0, priority),
			    ucpd_isr, dev_inst, 0);

		irq_enable(DT_INST_IRQN(0));
	}
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible STM32 TCPC instance found");

#define TCPC_DRIVER_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
	static struct tcpc_data drv_data_##inst;					\
	static const struct tcpc_config drv_config_##inst = {				\
		.ucpd_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.ucpd_port = (UCPD_TypeDef *)DT_INST_REG_ADDR(inst),			\
		.ucpd_params.psc_ucpdclk = ilog2(DT_INST_PROP(inst, psc_ucpdclk)),	\
		.ucpd_params.transwin = DT_INST_PROP(inst, transwin) - 1,		\
		.ucpd_params.IfrGap = DT_INST_PROP(inst, ifrgap) - 1,			\
		.ucpd_params.HbitClockDiv = DT_INST_PROP(inst, hbitclkdiv) - 1,		\
		.ucpd_dead_battery = DT_INST_PROP(inst, dead_battery),			\
	};										\
	DEVICE_DT_INST_DEFINE(inst,							\
			      &ucpd_init,						\
			      NULL,							\
			      &drv_data_##inst,						\
			      &drv_config_##inst,					\
			      POST_KERNEL,						\
			      CONFIG_USBC_TCPC_INIT_PRIORITY,				\
			      &driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCPC_DRIVER_INIT)
