/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for bq27z746 fuel gauge
 */

#include <string.h>
#define DT_DRV_COMPAT ti_bq27z746

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(EMUL_BQ27Z746);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

#include "bq27z746.h"

#define BQ27Z746_MAC_DATA_LEN     32
#define BQ27Z746_MAC_OVERHEAD_LEN 4 /* 2 cmd bytes, 1 length byte, 1 checksum byte */
#define BQ27Z746_MAC_COMPLETE_LEN (BQ27Z746_MAC_DATA_LEN + BQ27Z746_MAC_OVERHEAD_LEN)

struct bq27z746_emul_data {
	uint16_t mac_cmd;
};

/** Static configuration for the emulator */
struct bq27z746_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

static int emul_bq27z746_read_altmac(const struct emul *target, uint8_t *buf, size_t len)
{
	const uint8_t manufacturer_name[] = "Texas Instruments";
	const uint8_t device_name[] = "BQ27Z746";
	const uint8_t device_chemistry[] = "LION";
	const struct bq27z746_emul_data *data = target->data;

	if (len < BQ27Z746_MAC_COMPLETE_LEN) {
		LOG_ERR("When reading the ALTMAC, one must read the full %u byte",
			BQ27Z746_MAC_COMPLETE_LEN);
		return -EIO;
	}

	memset(buf, 0, len);

	/*
	 * The data read from BQ27Z746_ALTMANUFACTURERACCESS is:
	 * 0..1: The command (for verification)
	 * 2..33: The data
	 * 34: Checksum calculated as (uint8_t)(0xFF - (sum of all command and data bytes))
	 * 35: Length including command, checksum and length (e.g. data length + 4)
	 */

	/* Put the command in the first two byte */
	sys_put_le16(data->mac_cmd, buf);

	/* Based on the command, put some data and the length into the buffer. */
	/* In all of the operations, don't consider the zero-terminator. */
	switch (data->mac_cmd) {
	case BQ27Z746_MAC_CMD_MANUFACTURER_NAME:
		memcpy(&buf[2], manufacturer_name, sizeof(manufacturer_name) - 1);
		buf[35] = sizeof(manufacturer_name) - 1 + BQ27Z746_MAC_OVERHEAD_LEN;
		break;
	case BQ27Z746_MAC_CMD_DEVICE_NAME:
		memcpy(&buf[2], device_name, sizeof(device_name) - 1);
		buf[35] = sizeof(device_name) - 1 + BQ27Z746_MAC_OVERHEAD_LEN;
		break;
	case BQ27Z746_MAC_CMD_DEVICE_CHEM:
		memcpy(&buf[2], device_chemistry, sizeof(device_chemistry) - 1);
		buf[35] = sizeof(device_chemistry) - 1 + BQ27Z746_MAC_OVERHEAD_LEN;
		break;
	default:
		LOG_ERR("ALTMAC command 0x%x is not supported", data->mac_cmd);
		return -EIO;
	}

	/* Calculate the checksum */
	uint8_t sum = 0; /* Intentionally 8 bit wide and overflowing */

	for (int i = 0; i < BQ27Z746_MAC_COMPLETE_LEN - 2; i++) {
		sum += buf[i];
	}
	buf[34] = 0xFF - sum;

	return 0;
}

static int emul_bq27z746_write(const struct emul *target, uint8_t *buf, size_t len)
{
	struct bq27z746_emul_data *data = target->data;
	const uint8_t reg = buf[0];

	switch (reg) {
	case BQ27Z746_ALTMANUFACTURERACCESS:
		data->mac_cmd = sys_get_le16(&buf[1]);
		return 0;
	default:
		LOG_ERR("Writing is only supported to ALTMAC currently");
		return -EIO;
	}
}

static int emul_bq27z746_reg_read(const struct emul *target, int reg, int *val)
{
	switch (reg) {
	case BQ27Z746_MANUFACTURERACCESS:
		*val = 1;
		break;
	case BQ27Z746_ATRATE:
		*val = -2;
		break;
	case BQ27Z746_ATRATETIMETOEMPTY:
		*val = 1;
		break;
	case BQ27Z746_TEMPERATURE:
		*val = 1;
		break;
	case BQ27Z746_VOLTAGE:
		*val = 1;
		break;
	case BQ27Z746_BATTERYSTATUS:
		*val = 1;
		break;
	case BQ27Z746_CURRENT:
		*val = -2;
		break;
	case BQ27Z746_REMAININGCAPACITY:
		*val = 1;
		break;
	case BQ27Z746_FULLCHARGECAPACITY:
		*val = 1;
		break;
	case BQ27Z746_AVERAGECURRENT:
		*val = -2;
		break;
	case BQ27Z746_AVERAGETIMETOEMPTY:
		*val = 1;
		break;
	case BQ27Z746_AVERAGETIMETOFULL:
		*val = 1;
		break;
	case BQ27Z746_MAXLOADCURRENT:
		*val = 1;
		break;
	case BQ27Z746_MAXLOADTIMETOEMPTY:
		*val = 1;
		break;
	case BQ27Z746_AVERAGEPOWER:
		*val = 1;
		break;
	case BQ27Z746_BTPDISCHARGESET:
		*val = 1;
		break;
	case BQ27Z746_BTPCHARGESET:
		*val = 1;
		break;
	case BQ27Z746_INTERNALTEMPERATURE:
		*val = 1;
		break;
	case BQ27Z746_CYCLECOUNT:
		*val = 1;
		break;
	case BQ27Z746_RELATIVESTATEOFCHARGE:
		*val = 1;
		break;
	case BQ27Z746_STATEOFHEALTH:
		*val = 1;
		break;
	case BQ27Z746_CHARGINGVOLTAGE:
		*val = 1;
		break;
	case BQ27Z746_CHARGINGCURRENT:
		*val = 1;
		break;
	case BQ27Z746_TERMINATEVOLTAGE:
		*val = 1;
		break;
	case BQ27Z746_TIMESTAMPUPPER:
		*val = 1;
		break;
	case BQ27Z746_TIMESTAMPLOWER:
		*val = 1;
		break;
	case BQ27Z746_QMAXCYCLES:
		*val = 1;
		break;
	case BQ27Z746_DESIGNCAPACITY:
		*val = 1;
		break;
	case BQ27Z746_ALTMANUFACTURERACCESS:
		*val = 1;
		break;
	case BQ27Z746_MACDATA:
		*val = 1;
		break;
	case BQ27Z746_MACDATASUM:
		*val = 1;
		break;
	case BQ27Z746_MACDATALEN:
		*val = 1;
		break;
	case BQ27Z746_VOLTHISETTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_VOLTHICLEARTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_VOLTLOSETTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_VOLTLOCLEARTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_TEMPHISETTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_TEMPHICLEARTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_TEMPLOSETTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_TEMPLOCLEARTHRESHOLD:
		*val = 1;
		break;
	case BQ27Z746_INTERRUPTSTATUS:
		*val = 1;
		break;
	case BQ27Z746_SOCDELTASETTHRESHOLD:
		*val = 1;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, *val);

	return 0;
}

static int emul_bq27z746_read(const struct emul *target, int reg, uint8_t *buf, size_t len)
{
	if (len == 2) {
		unsigned int val;
		int rc = emul_bq27z746_reg_read(target, reg, &val);

		if (rc) {
			return rc;
		}

		sys_put_le16(val, buf);
	} else {
		switch (reg) {
		case BQ27Z746_ALTMANUFACTURERACCESS:
			LOG_DBG("Reading %u byte from ALTMAC", len);
			emul_bq27z746_read_altmac(target, buf, len);
			break;
		default:
			LOG_ERR("Reading is only supported from ALTMAC currently");
			return -EIO;
		}
	}

	return 0;
}

static int bq27z746_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				      int addr)
{
	int reg;
	int rc;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 1:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}

		return emul_bq27z746_write(target, msgs->buf, msgs->len);
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			rc = emul_bq27z746_read(target, reg, msgs->buf, msgs->len);
			if (rc) {
				return rc;
			}
		} else {
			LOG_ERR("Second message must be an I2C write");
			return -EIO;
		}
		return rc;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

static const struct i2c_emul_api bq27z746_emul_api_i2c = {
	.transfer = bq27z746_emul_transfer_i2c,
};

/**
 * Set up a new emulator (I2C)
 *
 * @param emul Emulation information
 * @param parent Device to emulate
 * @return 0 indicating success (always)
 */
static int emul_bq27z746_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * Main instantiation macro.
 */
#define BQ27Z746_EMUL(n)                                                                           \
	static struct bq27z746_emul_data bq27z746_emul_data_##n;                                   \
	static const struct bq27z746_emul_cfg bq27z746_emul_cfg_##n = {                            \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_bq27z746_init, &bq27z746_emul_data_##n,                        \
			    &bq27z746_emul_cfg_##n, &bq27z746_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(BQ27Z746_EMUL)
