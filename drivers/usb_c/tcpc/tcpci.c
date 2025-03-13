/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/usb_c/tcpci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb_c/tcpci_priv.h>

LOG_MODULE_REGISTER(tcpci, CONFIG_USBC_LOG_LEVEL);

#define LOG_COMM_ERR_STR "Can't communicate with TCPC %s@%x (%s %x = %04x)"

const struct tcpci_reg_dump_map tcpci_std_regs[TCPCI_STD_REGS_SIZE] = {
	{
		.addr = TCPC_REG_VENDOR_ID,
		.name = "VENDOR_ID",
		.size = 2,
	},
	{
		.addr = TCPC_REG_PRODUCT_ID,
		.name = "PRODUCT_ID",
		.size = 2,
	},
	{
		.addr = TCPC_REG_BCD_DEV,
		.name = "DEVICE_ID",
		.size = 2,
	},
	{
		.addr = TCPC_REG_TC_REV,
		.name = "USBTYPEC_REV",
		.size = 2,
	},
	{
		.addr = TCPC_REG_PD_REV,
		.name = "USBPD_REV_VER",
		.size = 2,
	},
	{
		.addr = TCPC_REG_PD_INT_REV,
		.name = "PD_INTERFACE_REV",
		.size = 2,
	},
	{
		.addr = TCPC_REG_ALERT,
		.name = "ALERT",
		.size = 2,
	},
	{
		.addr = TCPC_REG_ALERT_MASK,
		.name = "ALERT_MASK",
		.size = 2,
	},
	{
		.addr = TCPC_REG_POWER_STATUS_MASK,
		.name = "POWER_STATUS_MASK",
		.size = 1,
	},
	{
		.addr = TCPC_REG_FAULT_STATUS_MASK,
		.name = "FAULT_STATUS_MASK",
		.size = 1,
	},
	{
		.addr = TCPC_REG_EXT_STATUS_MASK,
		.name = "EXTENDED_STATUS_MASK",
		.size = 1,
	},
	{
		.addr = TCPC_REG_ALERT_EXT_MASK,
		.name = "ALERT_EXTENDED_MASK",
		.size = 1,
	},
	{
		.addr = TCPC_REG_CONFIG_STD_OUTPUT,
		.name = "CFG_STANDARD_OUTPUT",
		.size = 1,
	},
	{
		.addr = TCPC_REG_TCPC_CTRL,
		.name = "TCPC_CONTROL",
		.size = 1,
	},
	{
		.addr = TCPC_REG_ROLE_CTRL,
		.name = "ROLE_CONTROL",
		.size = 1,
	},
	{
		.addr = TCPC_REG_FAULT_CTRL,
		.name = "FAULT_CONTROL",
		.size = 1,
	},
	{
		.addr = TCPC_REG_POWER_CTRL,
		.name = "POWER_CONTROL",
		.size = 1,
	},
	{
		.addr = TCPC_REG_CC_STATUS,
		.name = "CC_STATUS",
		.size = 1,
	},
	{
		.addr = TCPC_REG_POWER_STATUS,
		.name = "POWER_STATUS",
		.size = 1,
	},
	{
		.addr = TCPC_REG_FAULT_STATUS,
		.name = "FAULT_STATUS",
		.size = 1,
	},
	{
		.addr = TCPC_REG_EXT_STATUS,
		.name = "EXTENDED_STATUS",
		.size = 1,
	},
	{
		.addr = TCPC_REG_ALERT_EXT,
		.name = "ALERT_EXTENDED",
		.size = 1,
	},
	{
		.addr = TCPC_REG_DEV_CAP_1,
		.name = "DEVICE_CAPABILITIES_1",
		.size = 2,
	},
	{
		.addr = TCPC_REG_DEV_CAP_2,
		.name = "DEVICE_CAPABILITIES_2",
		.size = 2,
	},
	{
		.addr = TCPC_REG_STD_INPUT_CAP,
		.name = "STANDARD_INPUT_CAPABILITIES",
		.size = 1,
	},
	{
		.addr = TCPC_REG_STD_OUTPUT_CAP,
		.name = "STANDARD_OUTPUT_CAPABILITIES",
		.size = 1,
	},
	{
		.addr = TCPC_REG_CONFIG_EXT_1,
		.name = "CFG_EXTENDED1",
		.size = 1,
	},
	{
		.addr = TCPC_REG_GENERIC_TIMER,
		.name = "GENERIC_TIMER",
		.size = 2,
	},
	{
		.addr = TCPC_REG_MSG_HDR_INFO,
		.name = "MESSAGE_HEADER_INFO",
		.size = 1,
	},
	{
		.addr = TCPC_REG_RX_DETECT,
		.name = "RECEIVE_DETECT",
		.size = 1,
	},
	{
		.addr = TCPC_REG_TRANSMIT,
		.name = "TRANSMIT",
		.size = 1,
	},
	{
		.addr = TCPC_REG_VBUS_VOLTAGE,
		.name = "VBUS_VOLTAGE",
		.size = 2,
	},
	{
		.addr = TCPC_REG_VBUS_SINK_DISCONNECT_THRESH,
		.name = "VBUS_SINK_DISCONNECT_THRESHOLD",
		.size = 2,
	},
	{
		.addr = TCPC_REG_VBUS_STOP_DISCHARGE_THRESH,
		.name = "VBUS_STOP_DISCHARGE_THRESHOLD",
		.size = 2,
	},
	{
		.addr = TCPC_REG_VBUS_VOLTAGE_ALARM_HI_CFG,
		.name = "VBUS_VOLTAGE_ALARM_HI_CFG",
		.size = 2,
	},
	{
		.addr = TCPC_REG_VBUS_VOLTAGE_ALARM_LO_CFG,
		.name = "VBUS_VOLTAGE_ALARM_LO_CFG",
		.size = 2,
	},
	{
		.addr = TCPC_REG_VBUS_NONDEFAULT_TARGET,
		.name = "VBUS_NONDEFAULT_TARGET",
		.size = 2,
	},
	{
		.addr = TCPC_REG_DEV_CAP_3,
		.name = "DEVICE_CAPABILITIES_3",
		.size = 2,
	},
};

int tcpci_read_reg8(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *value)
{
	int ret;

	for (int a = 0; a < CONFIG_USBC_TCPC_TCPCI_I2C_RETRIES; a++) {
		ret = i2c_write_read(i2c->bus, i2c->addr, &reg, sizeof(reg), value, sizeof(*value));

		if (ret == 0) {
			break;
		}
	}

	if (ret != 0) {
		LOG_ERR(LOG_COMM_ERR_STR, i2c->bus->name, i2c->addr, "r8", reg, *value);
	}

	return ret;
}

int tcpci_write_reg8(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = {reg, value};
	int ret;

	for (int a = 0; a < CONFIG_USBC_TCPC_TCPCI_I2C_RETRIES; a++) {
		ret = i2c_write(i2c->bus, buf, 2, i2c->addr);

		if (ret == 0) {
			break;
		}
	}

	if (ret != 0) {
		LOG_ERR(LOG_COMM_ERR_STR, i2c->bus->name, i2c->addr, "w8", reg, value);
	}

	return ret;
}

int tcpci_update_reg8(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t old_value;
	int ret;

	ret = tcpci_read_reg8(i2c, reg, &old_value);
	if (ret != 0) {
		return ret;
	}

	old_value &= ~mask;
	old_value |= (value & mask);

	ret = tcpci_write_reg8(i2c, reg, old_value);

	return ret;
}

int tcpci_read_reg16(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t *value)
{
	int ret;

	for (int a = 0; a < CONFIG_USBC_TCPC_TCPCI_I2C_RETRIES; a++) {
		ret = i2c_write_read(i2c->bus, i2c->addr, &reg, sizeof(reg), value, sizeof(*value));

		if (ret == 0) {
			*value = sys_le16_to_cpu(*value);
			break;
		}
	}

	if (ret != 0) {
		LOG_ERR(LOG_COMM_ERR_STR, i2c->bus->name, i2c->addr, "r16", reg, *value);
	}

	return ret;
}

int tcpci_write_reg16(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t value)
{
	value = sys_cpu_to_le16(value);
	uint8_t *value_ptr = (uint8_t *)&value;
	uint8_t buf[3] = {reg, value_ptr[0], value_ptr[1]};
	int ret;

	for (int a = 0; a < CONFIG_USBC_TCPC_TCPCI_I2C_RETRIES; a++) {
		ret = i2c_write(i2c->bus, buf, 3, i2c->addr);
		if (ret == 0) {
			break;
		}
	}

	if (ret != 0) {
		LOG_ERR(LOG_COMM_ERR_STR, i2c->bus->name, i2c->addr, "w16", reg, value);
	}

	return ret;
}

enum tcpc_alert tcpci_alert_reg_to_enum(uint16_t reg)
{
	/**
	 * Hard reset enum has priority since it causes other bits to be ignored. Other values
	 * are sorted by corresponding bits index in the register.
	 */
	if (reg & TCPC_REG_ALERT_RX_HARD_RST) {
		/** Received Hard Reset message */
		return TCPC_ALERT_HARD_RESET_RECEIVED;
	} else if (reg & TCPC_REG_ALERT_CC_STATUS) {
		/** CC status changed */
		return TCPC_ALERT_CC_STATUS;
	} else if (reg & TCPC_REG_ALERT_POWER_STATUS) {
		/** Power status changed */
		return TCPC_ALERT_POWER_STATUS;
	} else if (reg & TCPC_REG_ALERT_RX_STATUS) {
		/** Receive Buffer register changed */
		return TCPC_ALERT_MSG_STATUS;
	} else if (reg & TCPC_REG_ALERT_TX_FAILED) {
		/** SOP* message transmission not successful */
		return TCPC_ALERT_TRANSMIT_MSG_FAILED;
	} else if (reg & TCPC_REG_ALERT_TX_DISCARDED) {
		/**
		 * Reset or SOP* message transmission not sent
		 * due to an incoming receive message
		 */
		return TCPC_ALERT_TRANSMIT_MSG_DISCARDED;
	} else if (reg & TCPC_REG_ALERT_TX_SUCCESS) {
		/** Reset or SOP* message transmission successful */
		return TCPC_ALERT_TRANSMIT_MSG_SUCCESS;
	} else if (reg & TCPC_REG_ALERT_V_ALARM_HI) {
		/** A high-voltage alarm has occurred */
		return TCPC_ALERT_VBUS_ALARM_HI;
	} else if (reg & TCPC_REG_ALERT_V_ALARM_LO) {
		/** A low-voltage alarm has occurred */
		return TCPC_ALERT_VBUS_ALARM_LO;
	} else if (reg & TCPC_REG_ALERT_FAULT) {
		/** A fault has occurred. Read the FAULT_STATUS register */
		return TCPC_ALERT_FAULT_STATUS;
	} else if (reg & TCPC_REG_ALERT_RX_BUF_OVF) {
		/** TCPC RX buffer has overflowed */
		return TCPC_ALERT_RX_BUFFER_OVERFLOW;
	} else if (reg & TCPC_REG_ALERT_VBUS_DISCNCT) {
		/** The TCPC in Attached.SNK state has detected a sink disconnect */
		return TCPC_ALERT_VBUS_SNK_DISCONNECT;
	} else if (reg & TCPC_REG_ALERT_RX_BEGINNING) {
		/** Receive buffer register changed */
		return TCPC_ALERT_BEGINNING_MSG_STATUS;
	} else if (reg & TCPC_REG_ALERT_EXT_STATUS) {
		/** Extended status changed */
		return TCPC_ALERT_EXTENDED_STATUS;
	} else if (reg & TCPC_REG_ALERT_ALERT_EXT) {
		/**
		 * An extended interrupt event has occurred. Read the alert_extended
		 * register
		 */
		return TCPC_ALERT_EXTENDED;
	} else if (reg & TCPC_REG_ALERT_VENDOR_DEF) {
		/** A vendor defined alert has been detected */
		return TCPC_ALERT_VENDOR_DEFINED;
	}

	LOG_ERR("Invalid alert register value");
	return -1;
}

int tcpci_tcpm_get_cc(const struct i2c_dt_spec *bus, enum tc_cc_voltage_state *cc1,
		      enum tc_cc_voltage_state *cc2)
{
	uint8_t role;
	uint8_t status;
	int cc1_present_rd, cc2_present_rd;
	int rv;

	if (cc1 == NULL || cc2 == NULL) {
		return -EINVAL;
	}

	/* errors will return CC as open */
	*cc1 = TC_CC_VOLT_OPEN;
	*cc2 = TC_CC_VOLT_OPEN;

	/* Get the ROLE CONTROL and CC STATUS values */
	rv = tcpci_read_reg8(bus, TCPC_REG_ROLE_CTRL, &role);
	if (rv != 0) {
		return rv;
	}

	rv = tcpci_read_reg8(bus, TCPC_REG_CC_STATUS, &status);
	if (rv != 0) {
		return rv;
	}

	/* Get the current CC values from the CC STATUS */
	*cc1 = TCPC_REG_CC_STATUS_CC1_STATE(status);
	*cc2 = TCPC_REG_CC_STATUS_CC2_STATE(status);

	/* Determine if we are presenting Rd */
	cc1_present_rd = 0;
	cc2_present_rd = 0;
	if (role & TCPC_REG_ROLE_CTRL_DRP_MASK) {
		/*
		 * We are doing DRP.  We will use the CC STATUS
		 * ConnectResult to determine if we are presenting
		 * Rd or Rp.
		 */
		int term;

		term = !!(status & TCPC_REG_CC_STATUS_CONNECT_RESULT);

		if (*cc1 != TC_CC_VOLT_OPEN) {
			cc1_present_rd = term;
		}
		if (*cc2 != TC_CC_VOLT_OPEN) {
			cc2_present_rd = term;
		}
	} else {
		/*
		 * We are not doing DRP.  We will use the ROLE CONTROL
		 * CC values to determine if we are presenting Rd or Rp.
		 */
		int role_cc1, role_cc2;

		role_cc1 = TCPC_REG_ROLE_CTRL_CC1(role);
		role_cc2 = TCPC_REG_ROLE_CTRL_CC2(role);

		if (*cc1 != TC_CC_VOLT_OPEN) {
			cc1_present_rd = (role_cc1 == TC_CC_RD);
		}
		if (*cc2 != TC_CC_VOLT_OPEN) {
			cc2_present_rd = (role_cc2 == TC_CC_RD);
		}
	}

	*cc1 |= cc1_present_rd << 2;
	*cc2 |= cc2_present_rd << 2;

	return 0;
}

int tcpci_tcpm_get_chip_info(const struct i2c_dt_spec *bus, struct tcpc_chip_info *chip_info)
{
	int ret;

	if (chip_info == NULL) {
		return -EIO;
	}

	ret = tcpci_read_reg16(bus, TCPC_REG_VENDOR_ID, &chip_info->vendor_id);
	if (ret != 0) {
		return ret;
	}

	ret = tcpci_read_reg16(bus, TCPC_REG_PRODUCT_ID, &chip_info->product_id);
	if (ret != 0) {
		return ret;
	}

	return tcpci_read_reg16(bus, TCPC_REG_BCD_DEV, &chip_info->device_id);
}

int tcpci_tcpm_dump_std_reg(const struct i2c_dt_spec *bus)
{
	uint16_t value;

	for (unsigned int a = 0; a < ARRAY_SIZE(tcpci_std_regs); a++) {
		switch (tcpci_std_regs[a].size) {
		case 1:
			tcpci_read_reg8(bus, tcpci_std_regs[a].addr, (uint8_t *)&value);
			LOG_INF("- %-30s(0x%02x) =   0x%02x", tcpci_std_regs[a].name,
				tcpci_std_regs[a].addr, (uint8_t)value);
			break;
		case 2:
			tcpci_read_reg16(bus, tcpci_std_regs[a].addr, &value);
			LOG_INF("- %-30s(0x%02x) = 0x%04x", tcpci_std_regs[a].name,
				tcpci_std_regs[a].addr, value);
			break;
		}
	}

	return 0;
}

int tcpci_tcpm_set_bist_test_mode(const struct i2c_dt_spec *bus, bool enable)
{
	return tcpci_update_reg8(bus, TCPC_REG_TCPC_CTRL, TCPC_REG_TCPC_CTRL_BIST_TEST_MODE,
				 enable ? TCPC_REG_TCPC_CTRL_BIST_TEST_MODE : 0);
}

int tcpci_tcpm_transmit_data(const struct i2c_dt_spec *bus, struct pd_msg *msg,
			     const uint8_t retries)
{
	int reg = TCPC_REG_TX_BUFFER;
	int rv;
	int cnt = 4 * msg->header.number_of_data_objects;

	/* If not SOP* transmission, just write to the transmit register */
	if (msg->header.message_type >= NUM_SOP_STAR_TYPES) {
		/*
		 * Per TCPCI spec, do not specify retry (although the TCPC
		 * should ignore retry field for these 3 types).
		 */
		return tcpci_write_reg8(
			bus, TCPC_REG_TRANSMIT,
			TCPC_REG_TRANSMIT_SET_WITHOUT_RETRY(msg->header.message_type));
	}

	if (cnt > 0) {
		reg = TCPC_REG_TX_BUFFER;
		/* TX_BYTE_CNT includes extra bytes for message header */
		cnt += sizeof(msg->header.raw_value);

		struct i2c_msg buf[3];

		uint8_t tmp[2] = {TCPC_REG_TX_BUFFER, cnt};

		buf[0].buf = tmp;
		buf[0].len = 2;
		buf[0].flags = I2C_MSG_WRITE;

		buf[1].buf = (uint8_t *)&msg->header.raw_value;
		buf[1].len = sizeof(msg->header.raw_value);
		buf[1].flags = I2C_MSG_WRITE;

		buf[2].buf = (uint8_t *)msg->data;
		buf[2].len = msg->len;
		buf[2].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		if (cnt > sizeof(msg->header.raw_value)) {
			rv = i2c_transfer(bus->bus, buf, 3, bus->addr);
		} else {
			buf[1].flags |= I2C_MSG_STOP;
			rv = i2c_transfer(bus->bus, buf, 2, bus->addr);
		}

		/* If tcpc write fails, return error */
		if (rv) {
			return rv;
		}
	}

	/*
	 * We always retry in TCPC hardware since the TCPM is too slow to
	 * respond within tRetry (~195 usec).
	 *
	 * The retry count used is dependent on the maximum PD revision
	 * supported at build time.
	 */
	rv = tcpci_write_reg8(bus, TCPC_REG_TRANSMIT,
			      TCPC_REG_TRANSMIT_SET_WITH_RETRY(retries, msg->type));

	return rv;
}

int tcpci_tcpm_select_rp_value(const struct i2c_dt_spec *bus, enum tc_rp_value rp)
{
	return tcpci_update_reg8(bus, TCPC_REG_ROLE_CTRL, TCPC_REG_ROLE_CTRL_RP_MASK,
				 TCPC_REG_ROLE_CTRL_SET(0, rp, 0, 0));
}

int tcpci_tcpm_get_rp_value(const struct i2c_dt_spec *bus, enum tc_rp_value *rp)
{
	uint8_t reg_value = 0;
	int ret;

	ret = tcpci_read_reg8(bus, TCPC_REG_ROLE_CTRL, &reg_value);
	*rp = TCPC_REG_ROLE_CTRL_RP(reg_value);

	return ret;
}

int tcpci_tcpm_set_cc(const struct i2c_dt_spec *bus, enum tc_cc_pull pull)
{
	return tcpci_update_reg8(bus, TCPC_REG_ROLE_CTRL,
				 TCPC_REG_ROLE_CTRL_CC1_MASK | TCPC_REG_ROLE_CTRL_CC2_MASK,
				 TCPC_REG_ROLE_CTRL_SET(0, 0, pull, pull));
}

int tcpci_tcpm_set_vconn(const struct i2c_dt_spec *bus, bool enable)
{
	return tcpci_update_reg8(bus, TCPC_REG_POWER_CTRL, TCPC_REG_POWER_CTRL_VCONN_EN,
				 enable ? TCPC_REG_POWER_CTRL_VCONN_EN : 0);
}

int tcpci_tcpm_set_roles(const struct i2c_dt_spec *bus, enum pd_rev_type pd_rev,
			 enum tc_power_role power_role, enum tc_data_role data_role)
{
	return tcpci_update_reg8(bus, TCPC_REG_MSG_HDR_INFO, TCPC_REG_MSG_HDR_INFO_ROLES_MASK,
				 TCPC_REG_MSG_HDR_INFO_SET(pd_rev, data_role, power_role));
}

int tcpci_tcpm_set_drp_toggle(const struct i2c_dt_spec *bus, bool enable)
{
	return tcpci_update_reg8(bus, TCPC_REG_ROLE_CTRL, TCPC_REG_ROLE_CTRL_DRP_MASK,
				 TCPC_REG_ROLE_CTRL_SET(enable, 0, 0, 0));
}

int tcpci_tcpm_set_rx_type(const struct i2c_dt_spec *bus, uint8_t rx_type)
{
	return tcpci_write_reg8(bus, TCPC_REG_RX_DETECT, rx_type);
}

int tcpci_tcpm_set_cc_polarity(const struct i2c_dt_spec *bus, enum tc_cc_polarity polarity)
{
	return tcpci_update_reg8(
		bus, TCPC_REG_TCPC_CTRL, TCPC_REG_TCPC_CTRL_PLUG_ORIENTATION,
		(polarity == TC_POLARITY_CC1) ? 0 : TCPC_REG_TCPC_CTRL_PLUG_ORIENTATION);
}

int tcpci_tcpm_get_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				   uint16_t *status)
{
	switch (reg) {
	case TCPC_ALERT_STATUS:
		return tcpci_read_reg16(bus, TCPC_REG_ALERT, status);
	case TCPC_CC_STATUS:
		return tcpci_read_reg8(bus, TCPC_REG_CC_STATUS, (uint8_t *)status);
	case TCPC_POWER_STATUS:
		return tcpci_read_reg8(bus, TCPC_REG_POWER_STATUS, (uint8_t *)status);
	case TCPC_FAULT_STATUS:
		return tcpci_read_reg8(bus, TCPC_REG_FAULT_STATUS, (uint8_t *)status);
	case TCPC_EXTENDED_STATUS:
		return tcpci_read_reg8(bus, TCPC_REG_EXT_STATUS, (uint8_t *)status);
	case TCPC_EXTENDED_ALERT_STATUS:
		return tcpci_read_reg8(bus, TCPC_REG_ALERT_EXT, (uint8_t *)status);
	default:
		LOG_ERR("Not a TCPCI-specified reg address");
		return -EINVAL;
	}
}

int tcpci_tcpm_clear_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				     uint16_t mask)
{
	switch (reg) {
	case TCPC_ALERT_STATUS:
		return tcpci_write_reg16(bus, TCPC_REG_ALERT, mask);
	case TCPC_CC_STATUS:
		LOG_ERR("CC_STATUS is cleared by the TCPC");
		return -EINVAL;
	case TCPC_POWER_STATUS:
		LOG_ERR("POWER_STATUS is cleared by the TCPC");
		return -EINVAL;
	case TCPC_FAULT_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_FAULT_STATUS, (uint8_t)mask);
	case TCPC_EXTENDED_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_EXT_STATUS, (uint8_t)mask);
	case TCPC_EXTENDED_ALERT_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_ALERT_EXT, (uint8_t)mask);
	default:
		LOG_ERR("Not a TCPCI-specified reg address");
		return -EINVAL;
	}
}

int tcpci_tcpm_mask_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				    uint16_t mask)
{
	switch (reg) {
	case TCPC_ALERT_STATUS:
		return tcpci_write_reg16(bus, TCPC_REG_ALERT_MASK, mask);
	case TCPC_CC_STATUS:
		LOG_ERR("CC_STATUS does not have a corresponding mask register");
		return -EINVAL;
	case TCPC_POWER_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_POWER_STATUS_MASK, (uint8_t)mask);
	case TCPC_FAULT_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_FAULT_STATUS_MASK, (uint8_t)mask);
	case TCPC_EXTENDED_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_EXT_STATUS_MASK, (uint8_t)mask);
	case TCPC_EXTENDED_ALERT_STATUS:
		return tcpci_write_reg8(bus, TCPC_REG_ALERT_EXT_MASK, (uint8_t)mask);
	default:
		LOG_ERR("Not a TCPCI-specified reg address");
		return -EINVAL;
	}
}
